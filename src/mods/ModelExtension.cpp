#include "ModelExtension.hpp"
#include "../sdk/CnsChain.hpp"
#include "../sdk/ModelAttach.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/MtDTI.hpp"
#include "../sdk/sUnit.hpp"
#include <pugixml.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#undef naked                     // mod.hpp's `naked` is `static __declspec(naked)`; the detours are declared in the header
#define naked __declspec(naked)

namespace model_extension {

bool mod_enabled = false;
char folder[128] = "nativePC\\model_ext";
uintptr_t jmp_ret_set_model = 0, jmp_ret_model_dtor = 0, jmp_ret_chain_dtor = 0;

namespace {

constexpr uintptr_t kRModel = 0xA30;            // uModel::mpRModel (the .mod resource; cResource::path at +4)
constexpr uintptr_t kResPath = 0x04;
constexpr uintptr_t kFaceModel = 0x20C0, kCoatModel = 0x20C4;   // uPlayer::mpFaceMdl / mpCoatMdl (members, not units)
constexpr int kCheckEvery = 10;                 // frames between the checks of the chains out (a room change kills them)

std::vector<ListEntry> g_entries;

// <attach path= to= [joint=] [mode=] [pos=] [rot=] [scale=] [line=]/>: an extra model on every model whose path is `to`
struct AttachEntry {
    std::string path, to;                       // normalized
    model_attach::Mode mode;
    int joint, line;
    model_attach::Offset offset;
    std::string source;
};
std::vector<AttachEntry> g_attach_entries;

struct Attached {
    size_t entry;                               // index into g_attach_entries
    uintptr_t owner, model;
};
std::vector<Attached> g_attached;

struct Spawned {
    size_t entry;                               // index into g_entries
    uintptr_t model, chain;
    int line;
};

// Shared with the hooks (setModel / the destructor can run inside the game's loading), so behind one lock; the spawning
// itself only happens in on_frame, on the main thread.
std::recursive_mutex g_lock;
std::vector<uintptr_t> g_pending;               // models given an rModel since the last frame (setModel's entry)
std::unordered_map<uintptr_t, std::string> g_models;   // the live models with their .mod path (setModel -> the destructor)
std::vector<Spawned> g_spawned;
int g_frame = 0;

std::string normalize(std::string s) {
    for (char& c : s) { c = (char)std::tolower((unsigned char)c); if (c == '/') c = '\\'; }
    while (!s.empty() && (s.back() == '\\' || s.back() == ' ')) s.pop_back();
    return s;
}

// the class or one of its bases is named `name` (MtDTI: +4 name, +0x10 parent); cached per DTI
bool derives_from(uintptr_t unit, const char* name, std::unordered_map<uintptr_t, bool>& cache) {
    MtDTI* dti = devil4_sdk::get_DTI((void*)unit);
    if (!dti) return false;
    auto it = cache.find((uintptr_t)dti);
    if (it != cache.end()) return it->second;
    bool yes = false;
    for (MtDTI* d = dti; d && !yes; d = d->mp_parent) if (d->m_name && std::strcmp(d->m_name, name) == 0) yes = true;
    cache[(uintptr_t)dti] = yes;
    return yes;
}

std::string model_path(uintptr_t model) {
    const uintptr_t res = model ? *(uintptr_t*)(model + kRModel) : 0;
    if (!res) return {};
    const char* p = (const char*)(res + kResPath);
    return normalize(std::string(p, strnlen(p, 64)));
}

// The move line `unit` is on, -1 when it is on none (a sub-model: a member of its owner, not a unit of its own).
int line_of(uintptr_t unit) {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return -1;
    for (int line = 0; line < 32; ++line)
        for (uintptr_t u = (uintptr_t)su->mMoveLine[line].mTop; u; u = *(uintptr_t*)(u + 8))
            if (u == unit) return line;
    return -1;
}

// The model a list's collision_model names: the owner of `model` when it is a player's coat / face model and the owner
// has that path (a coat's body), else the first live model with the path.
uintptr_t collision_model_for(uintptr_t model, const std::string& path) {
    static std::unordered_map<uintptr_t, bool> is_player;
    uintptr_t first = 0;
    for (const auto& [m, p] : g_models) {
        if (p != path) continue;
        if (derives_from(m, "uPlayer", is_player) && (*(uintptr_t*)(m + kCoatModel) == model || *(uintptr_t*)(m + kFaceModel) == model)) return m;
        if (!first) first = m;
    }
    return first;
}

bool chain_exists(const Spawned& s) { return devil4_sdk::check_exists_in_moveline((void*)s.chain, s.line); }

// Every listed chain `model` doesn't carry yet, on the line before the model's (at most 12; sub-models aren't on a line:
// 12, which runs before the players' 13 and the enemies' 15+).
void spawn_for(uintptr_t model, const std::string& path) {
    if (!mod_enabled || path.empty()) return;
    int line = -2;                                          // looked up once, when a chain is due
    for (size_t e = 0; e < g_entries.size(); ++e) {
        const ListEntry& le = g_entries[e];
        if (le.model != path) continue;
        if (std::any_of(g_spawned.begin(), g_spawned.end(), [&](const Spawned& s) { return s.entry == e && s.model == model; })) continue;
        if (line == -2) {
            const int own = line_of(model);
            line = own < 0 ? cns_chain::kDefaultLine : std::min(cns_chain::kDefaultLine, own - 1);
        }
        if (line < 0) { spdlog::warn("ModelExtension: {} is on line 0, no line runs before it", path); return; }
        const uintptr_t coll = le.collision_model.empty() ? 0 : collision_model_for(model, le.collision_model);
        const uintptr_t chain = cns_chain::spawn(model, le.chain.c_str(), le.col.empty() ? nullptr : le.col.c_str(), coll, line);
        if (!chain) { spdlog::warn("ModelExtension: {} didn't load ({})", le.chain, le.source); continue; }
        g_spawned.push_back({ e, model, chain, line });
        spdlog::info("ModelExtension: {} on {} (line {})", le.chain, path, line);
    }
}

// Every listed attachment `owner` doesn't carry yet. Not on an attachment made from the same entry (a list attaching a
// model to its own path would nest it without end).
void attach_for(uintptr_t owner, const std::string& path) {
    if (!mod_enabled || path.empty()) return;
    for (size_t e = 0; e < g_attach_entries.size(); ++e) {
        const AttachEntry& ae = g_attach_entries[e];
        if (ae.to != path) continue;
        if (std::any_of(g_attached.begin(), g_attached.end(), [&](const Attached& a) { return a.entry == e && (a.owner == owner || a.model == owner); })) continue;
        const uintptr_t m = model_attach::spawn(owner, ae.path.c_str(), ae.mode, ae.joint, &ae.offset, ae.line);
        if (!m) { spdlog::warn("ModelExtension: {} didn't load ({})", ae.path, ae.source); continue; }
        g_attached.push_back({ e, owner, m });
        spdlog::info("ModelExtension: {} attached to {} ({}, line {})", ae.path, path,
                     ae.mode == model_attach::SKIN ? "skin" : "joint " + std::to_string(ae.joint), ae.line);
    }
}

// Takes the attachments `owner` carries off (owner 0: all of them).
void detach_of(uintptr_t owner) {
    for (size_t i = 0; i < g_attached.size();) {
        if (owner && g_attached[i].owner != owner) { ++i; continue; }
        model_attach::despawn(g_attached[i].model);   // dying: its destructor finds no record any more
        g_attached.erase(g_attached.begin() + i);
    }
}

// Takes `model`'s chains off (its destructor is running: the joints are still there), or every chain (model 0).
void despawn_of(uintptr_t model) {
    for (size_t i = 0; i < g_spawned.size();) {
        const Spawned& s = g_spawned[i];
        if (model && s.model != model) { ++i; continue; }
        if (chain_exists(s)) cns_chain::despawn(s.chain, cns_chain::model_of(s.chain) == s.model);
        g_spawned.erase(g_spawned.begin() + i);
    }
}

void frame() {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    // the models setModel saw: their path now (the rModel is in place once setModel returned), chains for them
    std::vector<uintptr_t> pending;
    pending.swap(g_pending);
    for (uintptr_t m : pending) {
        const std::string path = model_path(m);
        auto it = g_models.find(m);
        if (it != g_models.end() && it->second != path) { despawn_of(m); detach_of(m); }   // another .mod: those were the old one's
        g_models[m] = path;
        spawn_for(m, path);
        attach_for(m, path);
    }
    if (++g_frame % kCheckEvery != 0) return;
    // the chains out: one a room change killed comes back while its model lives on (the player, his coat); one whose
    // model is dying (the chain nulled its pointer) goes, the destructor hook forgets the model
    for (size_t i = 0; i < g_spawned.size();) {
        const Spawned s = g_spawned[i];
        if (!chain_exists(s)) {
            g_spawned.erase(g_spawned.begin() + i);
            auto it = g_models.find(s.model);
            if (it != g_models.end()) spawn_for(s.model, it->second);   // pushes back a fresh record (checked next time)
            continue;
        }
        if (cns_chain::model_of(s.chain) != s.model) {
            cns_chain::despawn(s.chain, false);
            g_spawned.erase(g_spawned.begin() + i);
            continue;
        }
        ++i;
    }
}

} // namespace

// uModel::setModel 0x9E6D70 (this@ecx, rModel*), its entry: the model is queued, its chains spawn next frame.
void __stdcall on_set_model(uintptr_t model) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    if (std::find(g_pending.begin(), g_pending.end(), model) == g_pending.end()) g_pending.push_back(model);
}

// The uModel destructor body 0x9E6AB0 (this@ecx), its entry (every derived destructor calls it; the joints are freed
// right after): the model's chains and attachments come off while the joints are still there, and the model is forgotten.
// One of our attachments the game deletes itself (a room change kills its line) is forgotten, and its owner, when still
// alive, is queued again so the next frame attaches a new one.
void __stdcall on_model_dtor(uintptr_t model) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    g_pending.erase(std::remove(g_pending.begin(), g_pending.end(), model), g_pending.end());
    for (size_t i = 0; i < g_attached.size(); ++i)
        if (g_attached[i].model == model) {
            const uintptr_t owner = g_attached[i].owner;
            g_attached.erase(g_attached.begin() + i);
            if (g_models.count(owner) && std::find(g_pending.begin(), g_pending.end(), owner) == g_pending.end()) g_pending.push_back(owner);
            break;
        }
    detach_of(model);
    if (g_models.erase(model)) despawn_of(model);
}

// ~cCnsChain 0x42C7A0 (esi = the cCnsChain), its entry: the destructor clears the constraint of EVERY joint of its model,
// which would drop constraints that claim their joint only once (Nero's coat joint 0 cCnsMatrix, installed by
// uPlayerNero::initModel: its move is a nullsub). The game deletes our chains itself when a room change kills their line,
// so for one of ours the chain lets go of its own joints and its model pointer is cleared first: the sweep then sees no
// model. The record stays: the next check finds the unit gone and spawns the chain again for a model still alive.
void __stdcall on_chain_dtor(uintptr_t cns) {
    const uintptr_t unit = cns - cns_chain::kChainInUnit;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    for (const Spawned& s : g_spawned)
        if (s.chain == unit) { cns_chain::detach(unit, cns_chain::model_of(unit) == s.model && g_models.count(s.model) != 0); return; }
}

// 6 bytes replaced: `mov eax, [esi+0FB4h]` (the model pointer: read after the call, so a detached chain reads 0)
naked void detour_chain_dtor() {
    _asm {
        pushad
        push esi
        call on_chain_dtor          // __stdcall: pops its argument
        popad
        mov eax, [esi + 0x0FB4]
        jmp dword ptr [jmp_ret_chain_dtor]
    }
}

// 6 bytes replaced: `push ebp; mov ebp, esp; and esp, -16`
naked void detour_set_model() {
    _asm {
        pushad
        push ecx
        call on_set_model           // __stdcall: pops its argument
        popad
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [jmp_ret_set_model]
    }
}

// 6 bytes replaced: `sub esp, 0Ch; push ebp; mov ebp, ecx`
naked void detour_model_dtor() {
    _asm {
        pushad
        push ecx
        call on_model_dtor          // __stdcall: pops its argument
        popad
        sub esp, 0x0C
        push ebp
        mov ebp, ecx
        jmp dword ptr [jmp_ret_model_dtor]
    }
}

namespace {
// "x y z" -> 3 floats (fewer numbers keep the defaults; one number for a scale sets all three)
int parse_floats(const char* s, float* out, int max) {
    int n = 0;
    while (s && *s && n < max) {
        char* end = nullptr;
        const float v = std::strtof(s, &end);
        if (end == s) break;
        out[n++] = v;
        s = end;
        while (*s == ' ' || *s == ',' || *s == '\t') ++s;
    }
    return n;
}

// degrees about X, then Y, then Z (the joint's axes) -> an MT quaternion (x y z w): q = qz * qy * qx
void euler_quat(const float deg[3], float q[4]) {
    const float k = 3.14159265f / 360.0f;
    const float cx = std::cos(deg[0] * k), sx = std::sin(deg[0] * k);
    const float cy = std::cos(deg[1] * k), sy = std::sin(deg[1] * k);
    const float cz = std::cos(deg[2] * k), sz = std::sin(deg[2] * k);
    q[0] = sx * cy * cz - cx * sy * sz;
    q[1] = cx * sy * cz + sx * cy * sz;
    q[2] = cx * cy * sz - sx * sy * cz;
    q[3] = cx * cy * cz + sx * sy * sz;
}

void parse_attach(const pugi::xml_node a, const std::string& file, int n) {
    AttachEntry ae;
    ae.path = normalize(a.attribute("path").as_string());
    ae.to = normalize(a.attribute("to").as_string());
    if (ae.path.empty() || ae.to.empty()) { spdlog::warn("ModelExtension: {}: an <attach> needs path and to", file); return; }
    ae.joint = a.attribute("joint").as_int(-1);
    const std::string mode = a.attribute("mode").as_string(a.attribute("joint") ? "rigid" : "skin");   // a joint means rigid
    if (_stricmp(mode.c_str(), "skin") == 0) ae.mode = model_attach::SKIN;
    else if (_stricmp(mode.c_str(), "rigid") == 0) ae.mode = model_attach::RIGID;
    else { spdlog::warn("ModelExtension: {}: <attach> mode \"{}\" (rigid or skin)", file, mode); return; }
    ae.line = a.attribute("line").as_int(model_attach::kDefaultLine);
    parse_floats(a.attribute("pos").as_string(), ae.offset.pos, 3);
    float rot[3] = { 0.0f, 0.0f, 0.0f };
    if (parse_floats(a.attribute("rot").as_string(), rot, 3)) euler_quat(rot, ae.offset.quat);
    if (parse_floats(a.attribute("scale").as_string(), ae.offset.scale, 3) == 1) ae.offset.scale[1] = ae.offset.scale[2] = ae.offset.scale[0];
    ae.source = file + " attach#" + std::to_string(n);
    g_attach_entries.push_back(std::move(ae));
}
} // namespace

// The lists: nativePC\model_ext\*.xml (ModelExtension.hpp):
//   <extensions><model path= [collision_model=]><chain clt= [col=] [collision_model=]/> ...</model>
//               <attach path= to= [joint=] [mode=rigid|skin] [pos=] [rot=] [scale=] [line=]/> ...</extensions>
// A chain's collision_model overrides its model's; a col without any collision model is ignored (collision needs both).
void reload() {
    namespace fs = std::filesystem;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    despawn_of(0);                                   // the records index the old lists
    detach_of(0);
    g_entries.clear();
    g_attach_entries.clear();
    if (fs::exists(folder)) {
        std::vector<std::string> files;
        for (const fs::directory_entry& e : fs::directory_iterator(folder))
            if (e.is_regular_file() && _stricmp(e.path().extension().string().c_str(), ".xml") == 0) files.push_back(e.path().string());
        std::sort(files.begin(), files.end());
        for (const std::string& f : files) {
            const std::string name = fs::path(f).filename().string();
            pugi::xml_document doc;
            const pugi::xml_parse_result res = doc.load_file(f.c_str());
            if (!res) { spdlog::warn("ModelExtension: {}: {} (offset {})", name, res.description(), (long long)res.offset); continue; }
            const pugi::xml_node root = doc.child("extensions");
            if (!root) { spdlog::warn("ModelExtension: {}: no <extensions> root", name); continue; }
            for (const pugi::xml_node m : root.children("model")) {
                const std::string model = normalize(m.attribute("path").as_string());
                if (model.empty()) { spdlog::warn("ModelExtension: {}: a <model> without path", name); continue; }
                const std::string model_coll = normalize(m.attribute("collision_model").as_string());
                int n = 0;
                for (const pugi::xml_node c : m.children("chain")) {
                    ++n;
                    ListEntry le;
                    le.model = model;
                    le.chain = normalize(c.attribute("clt").as_string());
                    if (le.chain.empty()) { spdlog::warn("ModelExtension: {}: a <chain> of {} without clt", name, model); continue; }
                    le.col = normalize(c.attribute("col").as_string());
                    le.collision_model = c.attribute("collision_model") ? normalize(c.attribute("collision_model").as_string()) : model_coll;
                    if (!le.col.empty() && le.collision_model.empty())
                        spdlog::warn("ModelExtension: {}: {} has a col but no collision_model; the col is ignored", name, le.chain);
                    le.source = name + " " + fs::path(model).filename().string() + "#" + std::to_string(n);
                    g_entries.push_back(std::move(le));
                }
            }
            int n = 0;
            for (const pugi::xml_node a : root.children("attach")) parse_attach(a, name, ++n);
        }
        spdlog::info("ModelExtension: {} chain(s), {} attachment(s) listed in {}", g_entries.size(), g_attach_entries.size(), folder);
    } else {
        spdlog::info("ModelExtension: no {} folder, nothing listed", folder);
    }
    for (const auto& [m, p] : g_models)              // the models already out get the new lists next frame (not our old
        if (!model_attach::is_attachment(m)) g_pending.push_back(m);   // attachments: they're dying, new ones queue themselves)
}

} // namespace model_extension

// ---------------------------------------------------------------------------------------------------------------
using namespace model_extension;

std::optional<std::string> ModelExtension::on_initialize() {
    if (!install_hook_absolute(0x9E6D70, hook_set_model, &detour_set_model, &jmp_ret_set_model, 6)) {
        spdlog::error("ModelExtension: failed to hook uModel::setModel (0x9E6D70)");
        return "Failed to init ModelExtension (setModel hook)";
    }
    if (!install_hook_absolute(0x9E6AB0, hook_model_dtor, &detour_model_dtor, &jmp_ret_model_dtor, 6)) {
        spdlog::error("ModelExtension: failed to hook the uModel destructor (0x9E6AB0)");
        return "Failed to init ModelExtension (destructor hook)";
    }
    if (!install_hook_absolute(0x42C7A0, hook_chain_dtor, &detour_chain_dtor, &jmp_ret_chain_dtor, 6)) {
        spdlog::error("ModelExtension: failed to hook ~cCnsChain (0x42C7A0)");
        return "Failed to init ModelExtension (chain destructor hook)";
    }
    reload();
    return Mod::on_initialize();
}

// The present hook runs on the game's main thread between frames: the units added here first move next frame.
void ModelExtension::on_frame(fmilliseconds& dt) { frame(); }

void ModelExtension::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (ImGui::Checkbox(_("Model extension"), &mod_enabled)) {
        std::lock_guard<std::recursive_mutex> lock(g_lock);
        if (mod_enabled) { for (const auto& [m, p] : g_models) g_pending.push_back(m); }   // the models already out get theirs
        else { despawn_of(0); detach_of(0); }
    }
    ImGui::SameLine();
    help_marker(_("Extends the game's models from the lists in nativePC\\model_ext\\*.xml: physics chains (.clt: coats, hair, "
                  "tails) and extra models (hats, coats, accessories) on the models they name, when those appear. Game paths "
                  "without extension:\n"
                  "<extensions>\n"
                  "  <attach path=\"model\\game\\pl000\\my_jacket\" to=\"model\\game\\pl000\\pl000\"/>\n"
                  "  <attach path=\"model\\game\\pl000\\my_hat\" to=\"model\\game\\pl000\\pl000\" joint=\"4\" pos=\"0 5 0\" rot=\"0 90 0\"/>\n"
                  "  <model path=\"model\\game\\pl000\\my_jacket\" collision_model=\"model\\game\\pl000\\pl000\">\n"
                  "    <chain clt=\"model\\game\\pl000\\my_jacket_tail\" col=\"model\\game\\pl000\\pl000_03\"/>\n"
                  "  </model>\n"
                  "</extensions>\n"
                  "<attach>: without a joint the extra model is skinned to its owner (each of its joints follows the owner's joint "
                  "with the same number: clothing made on the body rig, with the body's bind pose); with joint= (a joint number) it "
                  "rides that joint rigidly, offset by pos (cm), rot (degrees about X, Y, Z) and scale. Its own extra joints can "
                  "carry chains (<model> with its path).\n"
                  "<chain>: col (the .col) and collision_model (the model its shapes sit on) are optional; a chain's "
                  "collision_model overrides its model's. The chain's joint numbers are the model's joints. Don't list joints "
                  "another chain or constraint already drives."));
    if (!mod_enabled) return;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    ImGui::Indent(lineIndent);
    ImGui::PushID("ModelExtension");
    ImGui::PushItemWidth(sameLineWidth);
    ImGui::InputText(_("Lists folder"), folder, sizeof(folder));
    ImGui::PopItemWidth();
    if (ImGui::Button(_("Reload lists"))) reload();   // also takes every chain off: they come back next frame from the new lists
    ImGui::SameLine();
    help_marker(_("Reads the .xml files again and spawns every chain and attachment again, which also loads edited .clt / .col / "
                  ".mod files (the game keeps a loaded file until nothing uses it)."));
    ImGui::Text(_("%d attachment(s) listed, %d out"), (int)g_attach_entries.size(), (int)g_attached.size());
    for (size_t e = 0; e < g_attach_entries.size(); ++e) {
        const AttachEntry& ae = g_attach_entries[e];
        const int out = (int)std::count_if(g_attached.begin(), g_attached.end(), [&](const Attached& a) { return a.entry == e; });
        if (ae.mode == model_attach::SKIN) ImGui::BulletText("%s on %s, skinned: %d  (%s)", ae.path.c_str(), ae.to.c_str(), out, ae.source.c_str());
        else ImGui::BulletText("%s on %s, joint %d: %d  (%s)", ae.path.c_str(), ae.to.c_str(), ae.joint, out, ae.source.c_str());
    }
    ImGui::Text(_("%d chain(s) listed, %d out, %d model(s) known"), (int)g_entries.size(), (int)g_spawned.size(), (int)g_models.size());
    for (size_t e = 0; e < g_entries.size(); ++e) {
        const ListEntry& le = g_entries[e];
        const int out = (int)std::count_if(g_spawned.begin(), g_spawned.end(), [&](const Spawned& s) { return s.entry == e; });
        ImGui::BulletText("%s -> %s: %d  (%s)", le.chain.c_str(), le.model.c_str(), out, le.source.c_str());
    }
    if (ImGui::TreeNode(_("Models"))) {   // the paths a list can name, as the game loaded them
        std::vector<std::string> paths;
        for (const auto& [m, p] : g_models) if (!p.empty() && std::find(paths.begin(), paths.end(), p) == paths.end()) paths.push_back(p);
        std::sort(paths.begin(), paths.end());
        for (const std::string& p : paths) {
            ImGui::TextUnformatted(p.c_str());
            ImGui::SameLine();
            ImGui::PushID(p.c_str());
            if (ImGui::SmallButton(_("Copy"))) ImGui::SetClipboardText(p.c_str());
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void ModelExtension::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("model_extension").value_or(false);
    const std::string f = cfg.get("model_extension_folder").value_or(std::string(folder));
    strncpy_s(folder, f.c_str(), sizeof(folder) - 1);
    reload();
}

void ModelExtension::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("model_extension", mod_enabled);
    cfg.set("model_extension_folder", std::string(folder));
}
