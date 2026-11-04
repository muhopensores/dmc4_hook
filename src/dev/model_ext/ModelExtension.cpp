#include "ModelExtension.hpp"
#include "../../sdk/CnsChain.hpp"
#include "../../sdk/ChainLink.hpp"
#include "../../sdk/ModelAttach.hpp"
#include "../../sdk/ModelCns.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MtDTI.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/World2Screen.hpp"
#include "../../sdk/uCollisionMgr.hpp"
#include <pugixml.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <Windows.h>
#include <shellapi.h>
#include <atomic>
#include <mutex>
#include <unordered_map>

#undef naked                     // mod.hpp's `naked` is `static __declspec(naked)`; the detours are declared in the header
#define naked __declspec(naked)

namespace model_extension {

bool mod_enabled = false;
bool show_collision = false;
char folder[128] = "nativePC\\model_ext";
uintptr_t jmp_ret_set_model = 0, jmp_ret_model_dtor = 0, jmp_ret_chain_dtor = 0, jmp_ret_joint_alloc = 0, orig_world_matrix = 0;

namespace {

constexpr uintptr_t kRModel = 0xA30;            // uModel::mpRModel (the .mod resource; cResource::path at +4)
constexpr uintptr_t kResPath = 0x04;
constexpr uintptr_t kFaceModel = 0x20C0, kCoatModel = 0x20C4;   // uPlayer::mpFaceMdl / mpCoatMdl (members, not units)
constexpr uintptr_t kTransparency = 0xE20;      // uDevil4Model::mTransparency: draw 0x522DC0 skips the model at <= 0
constexpr uintptr_t kCoordParent = 0x18;        // uCoord::mpParent (coats, attachments: their owner)
constexpr int kCheckEvery = 10;                 // frames between the checks of the chains out (a room change kills them)

std::vector<ListEntry> g_entries;

// <hide path=/>: every model with this path stays loaded, moving and animated, and isn't drawn
struct HideEntry {
    std::string path;                           // normalized
    std::string source;
};
std::vector<HideEntry> g_hide_entries;

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
    bool has_shapes;                            // its .col is set (the collision model may still be missing)
    bool missed;                                // the missing collision model was reported
};

// <pin> / <offset> / <aim> inside a <model>: a joint constraint on every model with that path
struct CnsEntry {
    std::string model;                          // normalized
    model_cns::Kind kind;
    int joint, to;                              // joint numbers: of the model, of the source model
    std::string of;                             // normalized source model; "" = the model's owner, "self" = the model itself
    model_cns::Pin pin;
    model_cns::Offset offset;
    model_cns::Aim aim;
    std::string source;
};
std::vector<CnsEntry> g_cns_entries;

struct CnsLive {
    size_t entry;                               // index into g_cns_entries
    uintptr_t model, source;                    // source 0: not found yet
    uintptr_t block;
    bool installed;
    bool warned;                                // a problem with it was logged
};
std::vector<CnsLive> g_cns;

// <link a= b= max= [stiffness=]/> inside a <model>: two joints of the model's chains at most `max` cm apart
struct LinkEntry {
    std::string model;                          // normalized
    int a, b;                                   // joint numbers of the model, held by chains listed on it
    float max, stiffness;
    std::string source;
    bool warned;                                // a problem with it was logged
};
std::vector<LinkEntry> g_link_entries;

// The links of each live model, rebuilt every frame (frame()) from the chains out; read by the world-pass hook, which
// runs wherever the model updates (a line job for some). Its own lock: the hook never takes g_lock.
std::mutex g_link_lock;
std::unordered_map<uintptr_t, std::vector<chain_link::Link>> g_links;
std::atomic<bool> g_any_links{ false };
int g_links_live = 0;                           // for the readout

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

// The known model `m` hangs under (uCoord::mpParent: a coat's player, an attachment's owner), 0 at the top.
uintptr_t owner_model(uintptr_t m) {
    const uintptr_t p = *(uintptr_t*)(m + kCoordParent);
    return g_models.count(p) ? p : 0;
}

uintptr_t top_owner(uintptr_t m) {
    for (int depth = 0; depth < 8; ++depth) {
        const uintptr_t p = owner_model(m);
        if (!p) break;
        m = p;
    }
    return m;
}

// The model a list names for `model`: the nearest owner with the path, else one on the same character (an attachment's
// sibling), else the first live model with the path.
uintptr_t related_model(uintptr_t model, const std::string& path) {
    static std::unordered_map<uintptr_t, bool> is_player;
    uintptr_t m = model;
    for (int depth = 0; depth < 8 && (m = owner_model(m)) != 0; ++depth) {
        auto it = g_models.find(m);
        if (it != g_models.end() && it->second == path) return m;
    }
    const uintptr_t top = top_owner(model);
    uintptr_t first = 0;
    for (const auto& [c, p] : g_models) {
        if (p != path) continue;
        if (top_owner(c) == top) return c;
        if (derives_from(c, "uPlayer", is_player) && (*(uintptr_t*)(c + kCoatModel) == model || *(uintptr_t*)(c + kFaceModel) == model)) return c;
        if (!first) first = c;
    }
    return first;
}

bool chain_exists(const Spawned& s) { return devil4_sdk::check_exists_in_moveline((void*)s.chain, s.line); }

bool is_hidden(const std::string& path) {
    return std::any_of(g_hide_entries.begin(), g_hide_entries.end(), [&](const HideEntry& h) { return h.path == path; });
}

// Written every frame: nothing in the game sets a player sub-model's transparency, but a model's own move can (the camera
// fade of a model on a move line).
void apply_hidden() {
    for (const auto& [m, p] : g_models) if (is_hidden(p)) *(float*)(m + kTransparency) = 0.0f;
}

void show_hidden() {
    for (const auto& [m, p] : g_models) if (is_hidden(p)) *(float*)(m + kTransparency) = 1.0f;
}

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
        const uintptr_t coll = le.collision_model.empty() ? 0 : related_model(model, le.collision_model);
        const uintptr_t chain = cns_chain::spawn(model, le.chain.c_str(), le.col.empty() ? nullptr : le.col.c_str(), coll, line);
        if (!chain) { spdlog::warn("ModelExtension: {} didn't load ({})", le.chain, le.source); continue; }
        g_spawned.push_back({ e, model, chain, line, coll != 0 && !le.col.empty(), false });
        spdlog::info("ModelExtension: {} on {} (line {})", le.chain, path, line);
    }
}

// A chain whose collision model wasn't out when it spawned, or died: `collision_model` is looked up until it is found.
void resolve_collision(Spawned& s) {
    const ListEntry& le = g_entries[s.entry];
    if (le.col.empty() || le.collision_model.empty() || cns_chain::collision_model_of(s.chain)) return;
    const uintptr_t coll = related_model(s.model, le.collision_model);
    if (!coll) {
        if (!s.missed) spdlog::warn("ModelExtension: {} waits for its collision model {} ({})", le.chain, le.collision_model, le.source);
        s.missed = true;
        return;
    }
    cns_chain::set_collision(s.chain, s.has_shapes ? nullptr : le.col.c_str(), coll);
    s.has_shapes = true;
    s.missed = false;
}

uintptr_t cns_source(const CnsLive& c) {
    const CnsEntry& ce = g_cns_entries[c.entry];
    if (ce.kind == model_cns::OFFSET) return 0;
    if (ce.of == "self") return c.model;
    if (ce.of.empty()) { const uintptr_t o = owner_model(c.model); return o ? o : c.model; }
    return related_model(c.model, ce.of);
}

// Every listed constraint `model` doesn't carry yet.
void cns_for(uintptr_t model, const std::string& path) {
    if (!mod_enabled || path.empty()) return;
    for (size_t e = 0; e < g_cns_entries.size(); ++e) {
        const CnsEntry& ce = g_cns_entries[e];
        if (ce.model != path) continue;
        if (std::any_of(g_cns.begin(), g_cns.end(), [&](const CnsLive& c) { return c.entry == e && c.model == model; })) continue;
        const uintptr_t block = ce.kind == model_cns::PIN ? model_cns::make_pin(ce.pin)
                              : ce.kind == model_cns::AIM ? model_cns::make_aim(ce.aim) : model_cns::make_offset(ce.offset);
        if (block) g_cns.push_back({ e, model, 0, block, false, false });
    }
}

// Run every frame: a source's joints move with its setModel, and a chain's destructor clears the constraints of its model.
void update_cns() {
    for (CnsLive& c : g_cns) {
        const CnsEntry& ce = g_cns_entries[c.entry];
        if (ce.kind != model_cns::OFFSET) {
            if (!c.source) c.source = cns_source(c);
            const float* m = c.source ? model_cns::joint_matrix(c.source, ce.to) : nullptr;
            model_cns::bind(c.block, m);
            if (!m) {
                if (!c.warned && c.source) spdlog::warn("ModelExtension: {}: the source model has no joint {}", ce.source, ce.to);
                c.warned = c.warned || c.source != 0;
                if (c.installed) { model_cns::remove(c.block, c.model, ce.joint); c.installed = false; }
                continue;
            }
        }
        const model_cns::Install r = model_cns::install(c.block, c.model, ce.joint);
        c.installed = r == model_cns::INSTALLED;
        if (!c.installed && !c.warned) {
            spdlog::warn("ModelExtension: {}: joint {} of {} {}", ce.source, ce.joint, ce.model,
                         r == model_cns::TAKEN ? "is driven by a chain or another constraint" : "doesn't exist");
            c.warned = true;
        }
    }
}

// Takes `model`'s constraints off (model 0: all of them). `alive`: its joints are still there.
void remove_cns(uintptr_t model, bool alive) {
    for (size_t i = 0; i < g_cns.size();) {
        CnsLive& c = g_cns[i];
        if (model && c.model != model) { ++i; continue; }
        if (alive && c.installed) model_cns::remove(c.block, c.model, g_cns_entries[c.entry].joint);
        model_cns::release(c.block);
        g_cns.erase(g_cns.begin() + i);
    }
}

// `model`'s joints are about to go: nothing may read them any more. update_cns looks the source up again.
void drop_cns_source(uintptr_t model) {
    for (CnsLive& c : g_cns)
        if (c.source == model) { model_cns::bind(c.block, nullptr); c.source = 0; }
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

// The live links: for each listed link and each model out with its path, the two joints' slots in the model's chains.
void update_links() {
    std::unordered_map<uintptr_t, std::vector<chain_link::Link>> live;
    int count = 0;
    if (mod_enabled)
        for (LinkEntry& le : g_link_entries)
            for (const auto& [m, p] : g_models) {
                if (p != le.model) continue;
                chain_link::Link l{ 0, 0, -1, -1, le.max, le.stiffness };
                for (const Spawned& s : g_spawned) {
                    if (s.model != m || !chain_exists(s) || cns_chain::model_of(s.chain) != m) continue;
                    if (l.slot_a < 0 && (l.slot_a = chain_link::slot_of(s.chain, le.a)) >= 0) l.chain_a = s.chain;
                    if (l.slot_b < 0 && (l.slot_b = chain_link::slot_of(s.chain, le.b)) >= 0) l.chain_b = s.chain;
                }
                const char* problem = nullptr;
                if (l.slot_a < 0 || l.slot_b < 0) problem = "isn't held by a chain listed on the model";
                else if (chain_link::is_root(l.chain_a, l.slot_a) && chain_link::is_root(l.chain_b, l.slot_b))
                    problem = "links two tendril roots (they keep their animated places: nothing to move)";
                if (problem) {
                    if (!le.warned) spdlog::warn("ModelExtension: {}: joint {} {}", le.source, l.slot_a < 0 ? le.a : le.b, problem);
                    le.warned = true;
                    continue;
                }
                live[m].push_back(l);
                ++count;
            }
    std::lock_guard<std::mutex> lock(g_link_lock);
    g_links.swap(live);
    g_links_live = count;
    g_any_links.store(!g_links.empty(), std::memory_order_release);
}

// A model whose joints go, or a chain that goes: its links are dropped at once (the world-pass hook may run before the
// next frame rebuilds them).
void drop_links(uintptr_t model, uintptr_t chain) {
    std::lock_guard<std::mutex> lock(g_link_lock);
    if (model) g_links.erase(model);
    if (chain)
        for (auto& [m, v] : g_links)
            v.erase(std::remove_if(v.begin(), v.end(), [&](const chain_link::Link& l) { return l.chain_a == chain || l.chain_b == chain; }), v.end());
}

void frame() {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    // the models setModel saw: their path now (the rModel is in place once setModel returned). All of them are known before
    // anything spawns, so what a list says doesn't depend on which model came first.
    std::vector<uintptr_t> pending;
    pending.swap(g_pending);
    for (uintptr_t m : pending) {
        const std::string path = model_path(m);
        auto it = g_models.find(m);
        if (it != g_models.end() && it->second != path) {   // another .mod: those were the old one's
            despawn_of(m);
            detach_of(m);
            remove_cns(m, true);
            if (is_hidden(it->second)) *(float*)(m + kTransparency) = 1.0f;
        }
        g_models[m] = path;
    }
    for (uintptr_t m : pending) {
        auto it = g_models.find(m);
        if (it == g_models.end()) continue;
        spawn_for(m, it->second);
        attach_for(m, it->second);
        cns_for(m, it->second);
    }
    if (mod_enabled) { apply_hidden(); update_cns(); }
    update_links();
    if (++g_frame % kCheckEvery != 0) return;
    for (Spawned& s : g_spawned) if (chain_exists(s)) resolve_collision(s);
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

// --- Show collision: the listed chains' .col shapes and segments, as the solver tests them ------------------------------
// cCnsChain::collide 0x438DB0 (per segment, every group and prim, no flag filter): type 0 sphere = mPos0 in the frame of
// collision-model joint mBone0; type 3 capsule = mPos0 in mBone0's frame to mPos1 in mBone1's; other types and joint 255
// are skipped. The chain segment is node i -> node i + 1 (the tip's tail sits in the -1 slot after it), radius
// mCollisionSize. rCollisionShape::load 0x4684F0 builds the groups.
constexpr uintptr_t kChainBones = cns_chain::kChainInUnit + 0x20;        // mBone[32]: s16 joint numbers, -1 between tendrils
constexpr uintptr_t kChainNodes = cns_chain::kChainInUnit + 0x590;       // mat[k] translation rows, 64 bytes apart (33 nodes)
constexpr uintptr_t kChainSize = cns_chain::kChainInUnit + 0xFC0;        // mCollisionSize
constexpr uintptr_t kChainMute = cns_chain::kChainInUnit + 0xFCD;        // mMuteCollision
constexpr uintptr_t kChainShape = cns_chain::kChainInUnit + 0xFBC;       // mpCollisionShape
constexpr uintptr_t kShapeGroupNum = 0x64, kShapeGroups = 0x68;          // rCollisionShape: count, cCollGroup*[]
constexpr uintptr_t kGroupPrimNum = 0x04, kGroupPrims = 0x08;            // cCollGroup: count, cCollPrim*[]
constexpr uintptr_t kPrimData = 0x0C;                                    // cCollPrim -> the file's 64-byte kCollPrim

constexpr ImU32 kShapeColor = IM_COL32(0, 200, 255, 255), kTouchColor = IM_COL32(255, 40, 40, 255);
constexpr ImU32 kChainColor = IM_COL32(255, 220, 0, 255), kMutedColor = IM_COL32(140, 140, 140, 255);

glm::vec3 joint_point(const float* m, const float* p) {   // a joint-local point in world space (MT rows: x, y, z, position)
    return { m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12], m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
             m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14] };
}

// The closest distance between segments p0-p1 and q0-q1 (Ericson, Real-Time Collision Detection 5.1.9).
float segment_distance(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& q0, const glm::vec3& q1) {
    const glm::vec3 d1 = p1 - p0, d2 = q1 - q0, r = p0 - q0;
    const float a = glm::dot(d1, d1), e = glm::dot(d2, d2), f = glm::dot(d2, r);
    float s = 0.0f, t = 0.0f;
    if (a <= 1e-6f && e <= 1e-6f) return glm::length(r);
    if (a <= 1e-6f) t = std::clamp(f / e, 0.0f, 1.0f);
    else {
        const float c = glm::dot(d1, r);
        if (e <= 1e-6f) s = std::clamp(-c / a, 0.0f, 1.0f);
        else {
            const float b = glm::dot(d1, d2), denom = a * e - b * b;
            s = denom > 1e-6f ? std::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
            t = (b * s + f) / e;
            if (t < 0.0f) { t = 0.0f; s = std::clamp(-c / a, 0.0f, 1.0f); }
            else if (t > 1.0f) { t = 1.0f; s = std::clamp((b - c) / a, 0.0f, 1.0f); }
        }
    }
    return glm::length((p0 + d1 * s) - (q0 + d2 * t));
}

void draw_ring(const glm::vec3& c, const glm::vec3& u, const glm::vec3& v, float r, ImU32 color) {
    constexpr int kSegments = 16;
    for (int i = 0; i < kSegments; ++i) {
        const float a0 = i * 6.2831853f / kSegments, a1 = (i + 1) * 6.2831853f / kSegments;
        w2s::DrawLine3D(c + (u * std::cos(a0) + v * std::sin(a0)) * r, c + (u * std::cos(a1) + v * std::sin(a1)) * r, color);
    }
}

// A wireframe capsule (a sphere when the ends meet): three rings at each end, four edges along the axis.
void draw_capsule(const glm::vec3& p0, const glm::vec3& p1, float r, ImU32 color) {
    glm::vec3 axis = p1 - p0;
    const float len = glm::length(axis);
    axis = len > 1e-3f ? axis / len : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 helper = std::fabs(axis.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 u = glm::normalize(glm::cross(axis, helper)), v = glm::cross(axis, u);
    if (len > 1e-3f)
        for (int k = 0; k < 4; ++k) {
            const glm::vec3 o = (u * std::cos(k * 1.5707963f) + v * std::sin(k * 1.5707963f)) * r;
            w2s::DrawLine3D(p0 + o, p1 + o, color);
        }
    for (const glm::vec3& c : { p0, p1 }) {
        draw_ring(c, u, v, r, color);
        draw_ring(c, axis, u, r, color);
        draw_ring(c, axis, v, r, color);
        if (len <= 1e-3f) break;
    }
}

struct Segment { glm::vec3 a, b; };

void draw_chain_collision(const Spawned& s) {
    const uintptr_t chain = s.chain;
    const uintptr_t coll = cns_chain::collision_model_of(chain);
    const uintptr_t shape = *(uintptr_t*)(chain + kChainShape);
    const float size = *(float*)(chain + kChainSize);
    const bool muted = *(uint8_t*)(chain + kChainMute) != 0;
    // the chain's segments, from the solved nodes
    std::vector<Segment> segs;
    const int16_t* bones = (const int16_t*)(chain + kChainBones);
    for (int i = 0; i < 32; ++i) {
        if (bones[i] < 0) continue;
        const float* a = (const float*)(chain + kChainNodes + i * 0x40);
        const float* b = (const float*)(chain + kChainNodes + (i + 1) * 0x40);
        segs.push_back({ { a[0], a[1], a[2] }, { b[0], b[1], b[2] } });
    }
    for (const Segment& g : segs) draw_capsule(g.a, g.b, size, muted ? kMutedColor : kChainColor);
    if (!coll || !shape || !g_models.count(coll)) return;
    const int groups = *(int*)(shape + kShapeGroupNum);
    const uintptr_t* group_array = *(const uintptr_t**)(shape + kShapeGroups);
    for (int gi = 0; group_array && gi < groups; ++gi) {
        const uintptr_t group = group_array[gi];
        if (!group) continue;
        const int prims = *(int*)(group + kGroupPrimNum);
        const uintptr_t* prim_array = *(const uintptr_t**)(group + kGroupPrims);
        for (int pi = 0; prim_array && pi < prims; ++pi) {
            const kCollPrim* p = prim_array[pi] ? *(const kCollPrim**)(prim_array[pi] + kPrimData) : nullptr;
            if (!p || (p->mType != PRIM_TYPE_SPHERE && p->mType != PRIM_TYPE_CAPSULE)) continue;
            const float* m0 = model_cns::joint_matrix(coll, p->mBone0);
            const float* m1 = p->mType == PRIM_TYPE_CAPSULE ? model_cns::joint_matrix(coll, p->mBone1) : m0;
            if (!m0 || !m1) continue;
            const glm::vec3 q0 = joint_point(m0, &p->mPos0.x);
            const glm::vec3 q1 = p->mType == PRIM_TYPE_CAPSULE ? joint_point(m1, &p->mPos1.x) : q0;
            bool touched = false;
            for (const Segment& g : segs)
                if (segment_distance(g.a, g.b, q0, q1) <= size + p->mRadius) { touched = true; break; }
            draw_capsule(q0, q1, p->mRadius, touched && !muted ? kTouchColor : kShapeColor);
        }
    }
}

void draw_collision() {
    for (const Spawned& s : g_spawned)
        if (chain_exists(s) && cns_chain::model_of(s.chain) == s.model) draw_chain_collision(s);
    // the links: green while held, magenta while over their max (the solved points, after the links' pass)
    std::lock_guard<std::mutex> lock(g_link_lock);
    for (const auto& [m, links] : g_links)
        for (const chain_link::Link& l : links) {
            const float* a = (const float*)(l.chain_a + kChainNodes + l.slot_a * 0x40);
            const float* b = (const float*)(l.chain_b + kChainNodes + l.slot_b * 0x40);
            const glm::vec3 pa(a[0], a[1], a[2]), pb(b[0], b[1], b[2]);
            w2s::DrawLine3D(pa, pb, glm::length(pb - pa) > l.max + 0.1f ? IM_COL32(255, 0, 255, 255) : IM_COL32(0, 255, 0, 255), 2.0f);
        }
}

} // namespace

// uModel::setModel 0x9E6D70 (this@ecx, rModel*), its entry: the model is queued, its chains spawn next frame.
void __stdcall on_set_model(uintptr_t model) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    drop_cns_source(model);                     // setModel frees the joint array
    drop_links(model, 0);
    if (std::find(g_pending.begin(), g_pending.end(), model) == g_pending.end()) g_pending.push_back(model);
}

// The uModel destructor body 0x9E6AB0 (this@ecx), its entry (every derived destructor calls it; the joints are freed
// right after): the model's chains and attachments come off while the joints are still there, and the model is forgotten.
// One of our attachments the game deletes itself (a room change kills its line) is forgotten, and its owner, when still
// alive, is queued again so the next frame attaches a new one.
void __stdcall on_model_dtor(uintptr_t model) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    g_pending.erase(std::remove(g_pending.begin(), g_pending.end(), model), g_pending.end());
    drop_cns_source(model);
    drop_links(model, 0);
    remove_cns(model, false);
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
    drop_links(0, unit);
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
// setModel frees the joint array and puts the new rModel's joint table in place (0x9E6EDB, nopped) before the new array
// is allocated (mpJoint 0 till 0x9E6F75). The enemies' job threads read the player's joints meanwhile (uActor::getCenterPos
// 0x4559B0: joint 0; getJointPos 0x455A60 and ~140 more by number), and their `mpJoint + index * 0x90 == 0` check only
// catches index 0: a body whose joint 0 isn't first (an own-size root before it) crashed at 0x4559F2 on a DT swap. So the
// table is the game's own no-joints table (0xB99EA0, all 0xFF: every reader falls back) until the array is there.
naked void detour_set_model() {
    _asm {
        mov dword ptr [ecx + 0x2E8], 0x00B99EA0     // uModel::mJointTable
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

// 0x9E6F75, 6 bytes replaced: `mov [ebx+2E4h], edi` (ebx = the model, edi = the new joint array or 0). The real table
// follows the array (x86 keeps store order, the readers load the table first). A failed allocation keeps the no-joints
// table. Flags: the next `jnz` reads ZF from `cmp edi, 0`, which `test edi, edi` gives the same; lea / mov keep them.
naked void detour_joint_alloc() {
    _asm {
        mov [ebx + 0x2E4], edi                      // uModel::mpJoint
        test edi, edi
        jz done
        push eax
        mov eax, [ebx + 0xA30]                      // mpRModel
        lea eax, [eax + 0x120]                      // its joint table
        mov [ebx + 0x2E8], eax
        pop eax
    done:
        jmp dword ptr [jmp_ret_joint_alloc]
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

// uModel::updateWorldMatrix 0x9EA8B0 (this@ecx, no stack arguments, retn), a function hook: the model's chains solve inside
// it (each chain joint's constraint), so the links run right after, before anything reads the joints (the draw, the
// models that follow this one). It runs for every model, on line jobs too: the lock is only taken while links exist.
void __fastcall detour_world_matrix(uintptr_t model, int edx) {
    ((void(__fastcall*)(uintptr_t, int))orig_world_matrix)(model, edx);
    if (!g_any_links.load(std::memory_order_acquire)) return;
    std::lock_guard<std::mutex> lock(g_link_lock);
    auto it = g_links.find(model);
    if (it != g_links.end() && !it->second.empty()) chain_link::solve(model, it->second.data(), it->second.size());
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

// "x" / "+x" / "-z" -> the class's axis code (0 +x, 1 +y, 2 +z, 3 -x, 4 -y, 5 -z), -1 when it is none of them
int parse_axis(const char* s) {
    const bool minus = *s == '-';
    if (*s == '+' || *s == '-') ++s;
    const char c = (char)std::tolower((unsigned char)*s);
    if ((c != 'x' && c != 'y' && c != 'z') || s[1]) return -1;
    return (c - 'x') + (minus ? 3 : 0);
}

void parse_scale(const pugi::xml_node c, float* scale) {
    if (parse_floats(c.attribute("scale").as_string(), scale, 3) == 1) scale[1] = scale[2] = scale[0];
}

// <pin joint= to= [of=] [mode=all|rot] [blend=] [pos=] [rot=] [scale=]/>, <aim joint= to= [of=] [axis=] [up=] [blend=] [limit=]/>,
// <offset joint= [pos=] [rot=] [scale=] [add=]/>
void parse_cns(const pugi::xml_node c, const std::string& model, const std::string& file) {
    CnsEntry ce;
    ce.model = model;
    const std::string tag = c.name();
    ce.kind = tag == "pin" ? model_cns::PIN : tag == "aim" ? model_cns::AIM : model_cns::OFFSET;
    ce.joint = c.attribute("joint").as_int(-1);
    ce.to = c.attribute("to").as_int(-1);
    ce.source = file + " <" + tag + "> joint " + std::to_string(ce.joint);
    if (ce.joint < 0 || ce.joint > 255) { spdlog::warn("ModelExtension: {}: needs joint=, a joint number 0-255", ce.source); return; }
    if (ce.kind != model_cns::OFFSET) {
        if (ce.to < 0 || ce.to > 255) { spdlog::warn("ModelExtension: {}: needs to=, a joint number 0-255", ce.source); return; }
        ce.of = normalize(c.attribute("of").as_string());
    }
    const float blend = std::clamp(c.attribute("blend").as_float(1.0f), 0.0f, 1.0f);
    if (ce.kind == model_cns::PIN) {
        const std::string mode = c.attribute("mode").as_string("all");
        if (mode != "all" && mode != "rot") { spdlog::warn("ModelExtension: {}: mode \"{}\" (all or rot)", ce.source, mode); return; }
        ce.pin.rotation_only = mode == "rot";
        ce.pin.blend = blend;
        parse_floats(c.attribute("pos").as_string(), ce.pin.pos, 3);
        parse_floats(c.attribute("rot").as_string(), ce.pin.rot, 3);
        parse_scale(c, ce.pin.scale);
    } else if (ce.kind == model_cns::AIM) {
        ce.aim.axis = parse_axis(c.attribute("axis").as_string("+z"));
        ce.aim.up = parse_axis(c.attribute("up").as_string("+y"));
        if (ce.aim.axis < 0 || ce.aim.up < 0 || ce.aim.axis % 3 == ce.aim.up % 3) {
            spdlog::warn("ModelExtension: {}: axis and up must be two different axes (x, y, z, with + or -)", ce.source);
            return;
        }
        ce.aim.blend = blend;
        if (const char* limit = c.attribute("limit").as_string(nullptr)) {
            ce.aim.limited = true;
            if (parse_floats(limit, ce.aim.limit, 3) == 1) ce.aim.limit[1] = ce.aim.limit[2] = ce.aim.limit[0];
        }
    } else {
        parse_floats(c.attribute("pos").as_string(), ce.offset.pos, 3);
        parse_floats(c.attribute("rot").as_string(), ce.offset.rot, 3);
        parse_scale(c, ce.offset.scale);
        ce.offset.add_motion = c.attribute("add").as_bool(true);
    }
    g_cns_entries.push_back(std::move(ce));
}

// <link a= b= max= [stiffness=]/>
void parse_link(const pugi::xml_node c, const std::string& model, const std::string& file) {
    LinkEntry le;
    le.model = model;
    le.a = c.attribute("a").as_int(-1);
    le.b = c.attribute("b").as_int(-1);
    le.max = c.attribute("max").as_float(-1.0f);
    le.stiffness = std::clamp(c.attribute("stiffness").as_float(1.0f), 0.0f, 1.0f);
    le.warned = false;
    le.source = file + " <link> " + std::to_string(le.a) + "-" + std::to_string(le.b);
    if (le.a < 0 || le.a > 255 || le.b < 0 || le.b > 255 || le.a == le.b) {
        spdlog::warn("ModelExtension: {}: needs a= and b=, two different joint numbers 0-255", le.source);
        return;
    }
    if (le.max < 0.0f) { spdlog::warn("ModelExtension: {}: needs max=, a distance in cm", le.source); return; }
    g_link_entries.push_back(std::move(le));
}
} // namespace

// The lists: nativePC\model_ext\*.xml (ModelExtension.hpp):
//   <extensions><model path= [collision_model=]><chain clt= [col=] [collision_model=]/> ...</model>
//               <attach path= to= [joint=] [mode=rigid|skin] [pos=] [rot=] [scale=] [line=]/> ...
//               <hide path=/> ...</extensions>
// and inside a <model>: <pin .../> <aim .../> <offset .../> (parse_cns).
// A chain's collision_model overrides its model's; a col without any collision model is ignored (collision needs both).
void reload() {
    namespace fs = std::filesystem;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    despawn_of(0);                                   // the records index the old lists
    detach_of(0);
    remove_cns(0, true);
    show_hidden();                                   // the lists still listed hide theirs again next frame
    g_entries.clear();
    g_attach_entries.clear();
    g_hide_entries.clear();
    g_cns_entries.clear();
    g_link_entries.clear();
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
                for (const pugi::xml_node c : m.children()) {
                    const std::string tag = c.name();
                    if (tag == "pin" || tag == "aim" || tag == "offset") parse_cns(c, model, name);
                    else if (tag == "link") parse_link(c, model, name);
                }
            }
            int n = 0;
            for (const pugi::xml_node a : root.children("attach")) parse_attach(a, name, ++n);
            for (const pugi::xml_node h : root.children("hide")) {
                const std::string path = normalize(h.attribute("path").as_string());
                if (path.empty()) { spdlog::warn("ModelExtension: {}: a <hide> without path", name); continue; }
                g_hide_entries.push_back({ path, name });
            }
        }
        spdlog::info("ModelExtension: {} chain(s), {} attachment(s), {} constraint(s), {} link(s), {} hidden model(s) listed in {}",
                     g_entries.size(), g_attach_entries.size(), g_cns_entries.size(), g_link_entries.size(), g_hide_entries.size(), folder);
    } else {
        spdlog::info("ModelExtension: no {} folder, nothing listed", folder);
    }
    for (const auto& [m, p] : g_models)              // the models already out get the new lists next frame (not our old
        if (!model_attach::is_attachment(m)) g_pending.push_back(m);   // attachments: they're dying, new ones queue themselves)
}

// A starter list in the lists folder (example.xml, never overwritten): every entry commented out, so it changes nothing
// until a line is copied out of its comment.
void write_example() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(folder, ec);
    const fs::path p = fs::path(folder) / "example.xml";
    if (fs::exists(p)) { spdlog::info("ModelExtension: {} already exists", p.string()); return; }
    std::ofstream out(p);
    out << "<!-- Model Extension list (guide: docs\\model_extension.html in the dmc4_hook repo).\n"
           "     Game paths have no extension; copy a model's path from the Models list in the panel.\n"
           "     Chain files must be named .clt on disk (Albam / extraction tools call them .phs).\n"
           "     Copy a line out of this comment, change the paths, save, then press Reload lists. -->\n"
           "<extensions>\n"
           "  <!-- a hat riding joint 4 of Nero's body, 5 cm up, turned 90 degrees:\n"
           "  <attach path=\"model\\game\\pl000\\my_hat\" to=\"model\\game\\pl000\\pl000\" joint=\"4\" pos=\"0 5 0\" rot=\"0 90 0\"/>\n"
           "  -->\n"
           "  <!-- clothing made on Nero's skeleton, following his body joint for joint:\n"
           "  <attach path=\"model\\game\\pl000\\my_jacket\" to=\"model\\game\\pl000\\pl000\"/>\n"
           "  -->\n"
           "  <!-- a swinging chain on Nero's coat, colliding with the coat's shapes on his body:\n"
           "  <model path=\"model\\game\\pl000\\pl000_03\" collision_model=\"model\\game\\pl000\\pl000\">\n"
           "    <chain clt=\"model\\game\\pl000\\my_tail\" col=\"model\\game\\pl000\\pl000_03\"/>\n"
           "  </model>\n"
           "  -->\n"
           "  <!-- Nero's own face, hair and coat not drawn, for a body that has its own (Dante: pl006\\pl006_01, _02, _03, and\n"
           "       pl006\\pl009_01 for his Devil Trigger coat). They still load and animate:\n"
           "  <hide path=\"model\\game\\pl000\\pl000_01\"/>\n"
           "  <hide path=\"model\\game\\pl000\\pl000_02\"/>\n"
           "  <hide path=\"model\\game\\pl000\\pl000_03\"/>\n"
           "  -->\n"
           "  <!-- joint constraints on a model: joint 40 follows joint 9 of its owner half way, joint 41 points its +z at the\n"
           "       owner's joint 4 (limit: degrees either way), joint 42 is scaled up 1.5 times:\n"
           "  <model path=\"model\\game\\pl000\\my_jacket\">\n"
           "    <pin joint=\"40\" to=\"9\" mode=\"rot\" blend=\"0.5\"/>\n"
           "    <aim joint=\"41\" to=\"4\" axis=\"+z\" up=\"+y\" limit=\"30\"/>\n"
           "    <offset joint=\"42\" scale=\"1.5\"/>\n"
           "  </model>\n"
           "  -->\n"
           "</extensions>\n";
    spdlog::info("ModelExtension: wrote {}", p.string());
}

} // namespace model_extension

// ---------------------------------------------------------------------------------------------------------------
using namespace model_extension;

std::optional<std::string> ModelExtension::on_initialize() {
    if (!install_hook_absolute(0x9E6D70, hook_set_model, &detour_set_model, &jmp_ret_set_model, 6)) {
        spdlog::error("ModelExtension: failed to hook uModel::setModel (0x9E6D70)");
        return "Failed to init ModelExtension (setModel hook)";
    }
    // the joint table waits for the joint array (detour_set_model's comment); the early store goes only once the late one is in
    if (install_hook_absolute(0x9E6F75, hook_joint_alloc, &detour_joint_alloc, &jmp_ret_joint_alloc, 6))
        install_patch_absolute(0x9E6EDB, patch_table_store, "\x90\x90\x90\x90\x90\x90", 6);   // mov [ebx+2E8h], edi
    else
        spdlog::error("ModelExtension: failed to hook setModel's joint allocation (0x9E6F75): joint reads stay racy");
    if (!install_hook_absolute(0x9E6AB0, hook_model_dtor, &detour_model_dtor, &jmp_ret_model_dtor, 6)) {
        spdlog::error("ModelExtension: failed to hook the uModel destructor (0x9E6AB0)");
        return "Failed to init ModelExtension (destructor hook)";
    }
    if (!install_hook_absolute(0x42C7A0, hook_chain_dtor, &detour_chain_dtor, &jmp_ret_chain_dtor, 6)) {
        spdlog::error("ModelExtension: failed to hook ~cCnsChain (0x42C7A0)");
        return "Failed to init ModelExtension (chain destructor hook)";
    }
    // the links' pass after a model's world pass (detour_world_matrix); without it the rest still works
    if (!install_hook_absolute(0x9EA8B0, hook_world_matrix, &detour_world_matrix, &orig_world_matrix))
        spdlog::error("ModelExtension: failed to hook uModel::updateWorldMatrix (0x9EA8B0): <link> entries do nothing");
    reload();
    return Mod::on_initialize();
}

// The present hook runs on the game's main thread between frames: the units added here first move next frame.
void ModelExtension::on_frame(fmilliseconds& dt) {
    frame();
    if (mod_enabled && show_collision) {
        std::lock_guard<std::recursive_mutex> lock(g_lock);
        draw_collision();
    }
}

void ModelExtension::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (ImGui::Checkbox(_("Model extension"), &mod_enabled)) {
        std::lock_guard<std::recursive_mutex> lock(g_lock);
        if (mod_enabled) { for (const auto& [m, p] : g_models) g_pending.push_back(m); }   // the models already out get theirs
        else { despawn_of(0); detach_of(0); remove_cns(0, true); show_hidden(); }
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
                  "  <hide path=\"model\\game\\pl000\\pl000_03\"/>\n"
                  "</extensions>\n"
                  "<hide>: the model isn't drawn. It stays loaded and keeps animating, so the game's coat motions and chains go on "
                  "as usual (for a body that has its own face, hair or coat).\n"
                  "Inside a <model>, per joint: <pin joint= to=> makes the joint follow joint `to` (mode=\"rot\": its rotation only; "
                  "blend: how much, 0-1; pos / rot / scale: an offset), <aim joint= to=> points an axis of the joint at joint `to` "
                  "(axis, up: x, y, z with + or -; limit: degrees either way), <offset joint=> moves, turns or scales the joint "
                  "(pos / rot / scale). `to` is a joint of the model's owner unless of= names another model (of=\"self\": this one). "
                  "Children of the joint follow it.\n"
                  "<link a= b= max=> (inside a <model>): two joints of the model's chains, at most `max` cm apart (neighbouring "
                  "tendrils of a skirt, so the cloth between them can't open); stiffness 0-1 (default 1). A tendril's first "
                  "joint keeps its place, so link the joints below it.\n"
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
    ImGui::TextDisabled(_("Put .xml lists in this folder, then Reload lists. Guide: docs\\model_extension.html in the hook's repo."));
    if (ImGui::Button(_("Reload lists"))) reload();   // also takes every chain off: they come back next frame from the new lists
    ImGui::SameLine();
    if (ImGui::Button(_("Open folder"))) {            // made when missing, so a first-time user has somewhere to put a list
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        ShellExecuteA(nullptr, "open", std::filesystem::absolute(folder, ec).string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    if (ImGui::Button(_("Create example"))) write_example();
    ImGui::SameLine();
    help_marker(_("Reload lists reads the .xml files again and spawns every chain and attachment again, which also loads edited "
                  ".clt / .col / .mod files (the game keeps a loaded file until nothing uses it). Open folder opens the lists "
                  "folder in Explorer. Create example writes example.xml there (everything in it is commented out: copy a "
                  "line out of its comment and change the paths)."));
    ImGui::Checkbox(_("Show chain collision"), &show_collision);
    ImGui::SameLine();
    help_marker(_("Draws the listed chains' collision as the game's chain solver tests it: each chain's segments in yellow "
                  "(grey when the chain mutes its collision; thickness = its collision size), and the shapes of its .col on "
                  "the collision model in blue, red while a segment touches one. Only the spheres and capsules count; a "
                  "shape on a joint the collision model lacks isn't drawn (the game skips it too)."));
    ImGui::Text(_("%d attachment(s) listed, %d out"), (int)g_attach_entries.size(), (int)g_attached.size());
    for (size_t e = 0; e < g_attach_entries.size(); ++e) {
        const AttachEntry& ae = g_attach_entries[e];
        const int out = (int)std::count_if(g_attached.begin(), g_attached.end(), [&](const Attached& a) { return a.entry == e; });
        if (ae.mode == model_attach::SKIN) ImGui::BulletText("%s on %s, skinned: %d  (%s)", ae.path.c_str(), ae.to.c_str(), out, ae.source.c_str());
        else ImGui::BulletText("%s on %s, joint %d: %d  (%s)", ae.path.c_str(), ae.to.c_str(), ae.joint, out, ae.source.c_str());
    }
    ImGui::Text(_("%d constraint(s) listed, %d on a joint"), (int)g_cns_entries.size(),
                (int)std::count_if(g_cns.begin(), g_cns.end(), [](const CnsLive& c) { return c.installed; }));
    for (size_t e = 0; e < g_cns_entries.size(); ++e) {
        const CnsEntry& ce = g_cns_entries[e];
        const int out = (int)std::count_if(g_cns.begin(), g_cns.end(), [&](const CnsLive& c) { return c.entry == e && c.installed; });
        ImGui::BulletText("%s: %d  (%s)", ce.model.c_str(), out, ce.source.c_str());
    }
    ImGui::Text(_("%d link(s) listed, %d live"), (int)g_link_entries.size(), g_links_live);
    for (const LinkEntry& le : g_link_entries)
        ImGui::BulletText("%s: %d - %d, at most %.1f cm  (%s)", le.model.c_str(), le.a, le.b, le.max, le.source.c_str());
    ImGui::Text(_("%d model(s) hidden"), (int)g_hide_entries.size());
    for (const HideEntry& h : g_hide_entries) {
        const int out = (int)std::count_if(g_models.begin(), g_models.end(), [&](const auto& m) { return m.second == h.path; });
        ImGui::BulletText("%s: %d  (%s)", h.path.c_str(), out, h.source.c_str());
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
    show_collision = cfg.get<bool>("model_extension_show_collision").value_or(false);
    const std::string f = cfg.get("model_extension_folder").value_or(std::string(folder));
    strncpy_s(folder, f.c_str(), sizeof(folder) - 1);
    reload();
}

void ModelExtension::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("model_extension", mod_enabled);
    cfg.set<bool>("model_extension_show_collision", show_collision);
    cfg.set("model_extension_folder", std::string(folder));
}
