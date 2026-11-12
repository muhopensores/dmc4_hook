#include "ModelExtension.hpp"
#include "../../sdk/CnsChain.hpp"
#include "../../sdk/ChainLink.hpp"
#include "../../sdk/ChainSolver.hpp"
#include "../../sdk/ModelAttach.hpp"
#include "../../sdk/ModelCns.hpp"
#include "../../sdk/ModelShape.hpp"
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
#include <unordered_set>

#undef naked                     // mod.hpp's `naked` is `static __declspec(naked)`; the detours are declared in the header
#define naked __declspec(naked)

namespace model_extension {

bool mod_enabled = false;
bool show_collision = false;
bool show_targets = false;      // the hook's chain solver's debug view (draw_targets, the Physics readout)
#define PHYSICS_DUMP_NAME "dmc4_hook_physics_dump.json"
constexpr const char* kPhysicsDumpPath = PHYSICS_DUMP_NAME;   // the state recording (chain_solver::request_dump), game folder
int g_record_frames = 600;      // frames the recording covers
char folder[128] = "nativePC\\model_ext";
uintptr_t jmp_ret_set_model = 0, jmp_ret_model_dtor = 0, jmp_ret_chain_dtor = 0, jmp_ret_joint_alloc = 0, orig_world_matrix = 0;
uintptr_t orig_chain_move = 0, orig_chain_joint = 0;

namespace {

constexpr uintptr_t kRModel = 0xA30;            // uModel::mpRModel (the .mod resource; cResource::path at +4)
constexpr uintptr_t kResPath = 0x04;
constexpr uintptr_t kFaceModel = 0x20C0, kCoatModel = 0x20C4;   // uPlayer::mpFaceMdl / mpCoatMdl (members, not units)
constexpr uintptr_t kTransparency = 0xE20;      // uDevil4Model::mTransparency: draw 0x522DC0 skips the model at <= 0
constexpr uintptr_t kCoordParent = 0x18;        // uCoord::mpParent (coats, attachments: their owner)
constexpr uintptr_t kChainModel = 0xFB4;        // cCnsChain: its model (the unit's +0xFD4)
constexpr uintptr_t kChainStep = 0x10B0;        // cCnsChain: the frame's step t, written by move, read by calcJoint
constexpr uintptr_t kChainFixedStep = 0x18;     // cConstraint: players' fixed step (calcJointImpl solves with 1.0, not t)
constexpr uintptr_t kWorkRate = 0xCF4, kWorkRateType = 0xCF8;   // uDevil4Model::mWorkRate (cWorkRate): rate, speed bits
constexpr uintptr_t kWorkRatePtr = 0xE558D0;    // sWorkRate*: +0x20 game, +0x28 global, +0x2C player, +0x30 enemy,
                                                // +0x34 room, +0x38 turbo
constexpr uintptr_t kMediatorPtr = 0xE558B8;    // sMediator*: +0x494 turbo mode, +0x495 turbo cancel
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

// <shape shp= [shw= speed= rate= frame= loop=]> [<weights shw= .../>]... </shape> inside a <model>: morph targets (.shp) and up
// to four weight lists (.shw) on every model with that path (sdk/ModelShape.hpp). A model has one shape: the first entry wins.
struct ShapeEntry {
    std::string model, shp;                     // normalized
    std::vector<model_shape::List> lists;
    std::string source;
};
std::vector<ShapeEntry> g_shape_entries;

// Only uDevil4Model::move runs a shape's update and the players' move doesn't: whether the game runs it for a model is
// measured (a list's frame moves on its own between two of our frames, or not), and we run it where the game doesn't.
struct ShapeLive {
    size_t entry;                               // index into g_shape_entries
    uintptr_t model;
    int runs = -1;                              // who runs the update: -1 not known yet, 0 the game, 1 us
    bool probed = false;                        // a frame left to the game, measured at the next
    float last[model_shape::kLists] = {};
};
std::vector<ShapeLive> g_shapes;

// <physics clt= [substeps=] ...> inside a <model>: our solver (sdk/ChainSolver.hpp; Vibed\ModelExt\chain_solver.md) in place
// of the game's for every chain with that .clt on a model with that path: the game's own (Nero's coat) or a <chain>'s.
struct PhysicsEntry {
    std::string model, clt;                     // normalized
    chain_solver::Settings settings;
    std::string source;
};
std::vector<PhysicsEntry> g_physics_entries;

// Each chain's entry, looked up by the solver hooks (line jobs too) and cached per chain; another .clt or model on it
// (setCltRes, setModel) looks it up again. Its own lock: the hooks never take g_lock.
struct PhysicsCache {
    uintptr_t res, model;
    int entry;                                  // -1: the game's solver
    chain_solver::Settings settings;
};
std::mutex g_physics_lock;
std::unordered_map<uintptr_t, PhysicsCache> g_physics_cache;
std::atomic<bool> g_any_physics{ false };
// <cloth roots= [closed=] [stretch=] [shear=] [bend=]/> inside a <model>: tendrils of the model's chains on our solver, in
// order across a sheet, solved together and joined point by point (chain_solver.md "Sheets")
struct ClothEntry {
    std::string model;                          // normalized
    std::vector<int> roots;                     // joint numbers: each a tendril's first joint
    bool closed;
    float stretch, shear, bend;
    std::string source;
    bool warned;
};
std::vector<ClothEntry> g_cloth_entries;
int g_cloths_live = 0;                          // for the readout

// cCnsChain (RE note chain_cnschain.md section 2): mpCltRes (the .clt, cResource path at +4) and mpModel
constexpr uintptr_t kCnsCltRes = 0xFB0, kCnsModel = 0xFB4;

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
std::unordered_set<uintptr_t> g_excluded;       // other mods' models the lists leave alone (exclude())
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

bool is_devil4_model(uintptr_t unit) {
    static std::mutex lock;
    static std::unordered_map<uintptr_t, bool> cache;
    std::lock_guard<std::mutex> guard(lock);
    return derives_from(unit, "uDevil4Model", cache);
}

// A model's speed from its cWorkRate, as the game reads it for things that follow an owner (uDevil4Effect::move 0x521920:
// sub_4A6800(mType) x mWorkRate), without the game speed (bit 1), which a chain's step already holds. Bits: 4 global
// (x turbo while turbo mode is on), 8 player, 0x10 enemy, 0x20 room.
float work_rate(uintptr_t model) {
    const uintptr_t wr = *(uintptr_t*)kWorkRatePtr;
    if (!wr) return 1.0f;
    const uint32_t bits = *(uint32_t*)(model + kWorkRateType);
    float r = *(float*)(model + kWorkRate);
    if (bits & 4) {
        const uintptr_t med = *(uintptr_t*)kMediatorPtr;
        const bool turbo = med && *(uint8_t*)(med + 0x494) && !*(uint8_t*)(med + 0x495);
        r *= turbo ? *(float*)(wr + 0x38) * *(float*)(wr + 0x28) : *(float*)(wr + 0x28);
    }
    if (bits & 8) r *= *(float*)(wr + 0x2C);
    if (bits & 0x10) r *= *(float*)(wr + 0x30);
    if (bits & 0x20) r *= *(float*)(wr + 0x34);
    return r;
}

// The speed a chain on `model` runs at, beyond the game speed: the product of the work rates of the model and the models
// above it (uCoord::mpParent: a coat's player, an attachment's owner), so the trainer's pause (global speed 0), slow
// motion and a frozen character carry over; 0 when any of their move lines is paused (MoveLine::mPause: sUnit::move
// 0x8DC290 skips its units). A chain unit runs on its own line (12, or the one before its model's) at the game speed
// alone, so without this it went on swinging while its character stood still.
float chain_rate(uintptr_t model) {
    sUnit* su = devil4_sdk::get_sUnit();
    float r = 1.0f;
    for (int depth = 0; model && depth < 8; ++depth) {
        const int line = su ? line_of(model) : -1;
        if (line >= 0 && su->mMoveLine[line].mPause) return 0.0f;
        if (is_devil4_model(model)) r *= work_rate(model);
        model = *(uintptr_t*)(model + kCoordParent);
    }
    return std::max(r, 0.0f);
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

// The listed shape `model` doesn't carry yet (one per model: the first entry with its path).
void shape_for(uintptr_t model, const std::string& path) {
    if (!mod_enabled || path.empty()) return;
    if (std::any_of(g_shapes.begin(), g_shapes.end(), [&](const ShapeLive& l) { return l.model == model; })) return;
    for (size_t e = 0; e < g_shape_entries.size(); ++e) {
        const ShapeEntry& se = g_shape_entries[e];
        if (se.model != path) continue;
        std::string error;
        if (!model_shape::set(model, se.shp.c_str(), se.lists.data(), (int)se.lists.size(), error)) {
            spdlog::warn("ModelExtension: {}: {}", se.source, error);
            return;
        }
        ShapeLive l;
        l.entry = e;
        l.model = model;
        g_shapes.push_back(l);
        spdlog::info("ModelExtension: {} on {} ({} target(s))", se.shp, path, model_shape::targets(model));
        for (int i = 0; i < (int)se.lists.size(); ++i)
            if (!model_shape::list_on(model, i))
                spdlog::warn("ModelExtension: {}: {} stayed off (a .shw needs a curve for every target of the .shp)", se.source, se.lists[i].shw);
        return;
    }
}

// Run every frame: the shapes' update where the game doesn't run it (shape_for's comment). All four lists count as they
// are now, not only the entry's: a move can start one on demand (model_shape::play, MoveDef's <shape>).
void update_shapes() {
    constexpr int n = model_shape::kLists;
    for (ShapeLive& l : g_shapes) {
        if (!model_shape::shape_of(l.model)) continue;   // cleared by someone else
        const ShapeEntry& se = g_shape_entries[l.entry];
        const float delta = model_shape::delta_of(l.model);
        bool moving = false;
        for (int i = 0; i < n; ++i) moving = moving || (model_shape::speed(l.model, i) != 0.0f && model_shape::list_on(l.model, i));
        if (!moving) {                              // still weights: running the update twice changes nothing
            model_shape::update(l.model, delta);
            model_shape::retire(l.model);
            continue;
        }
        if (l.runs < 0 && delta > 0.0f) {
            if (!l.probed) l.probed = true;
            else {
                bool advanced = false;
                for (int i = 0; i < n; ++i) advanced = advanced || model_shape::frame(l.model, i) != l.last[i];
                l.runs = advanced ? 0 : 1;
                spdlog::info("ModelExtension: {}: the weight update is run by {}", se.source, l.runs ? "ModelExtension" : "the game");
            }
        }
        if (l.runs == 1) model_shape::update(l.model, delta);
        model_shape::retire(l.model);              // one-shots started on demand that ended (List::hold)
        for (int i = 0; i < n; ++i) l.last[i] = model_shape::frame(l.model, i);
    }
}

// Takes `model`'s shape off (model 0: every one). `alive`: the model stays, so its cShape is cleared (a dying model's
// destructor releases the files itself).
void remove_shapes(uintptr_t model, bool alive) {
    for (size_t i = 0; i < g_shapes.size();) {
        if (model && g_shapes[i].model != model) { ++i; continue; }
        if (alive) model_shape::clear(g_shapes[i].model);
        else model_shape::forget(g_shapes[i].model);
        g_shapes.erase(g_shapes.begin() + i);
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

// The live links: for each listed link and each model out with its path, the two joints' slots in the model's chains: the
// <chain>s out on it, and the game's own chains on it that a <physics> entry puts on our solver (Nero's coat). ChainLink
// takes uCnsChain units (the cCnsChain at +0x20) and reads nothing outside the cCnsChain, so a bare cCnsChain is passed as
// "unit" = chain - 0x20, as the chain destructor hook's drop_links does.
void update_links() {
    std::unordered_map<uintptr_t, std::vector<chain_link::Link>> live;
    int count = 0;
    std::vector<std::pair<uintptr_t, uintptr_t>> solved;    // (model, unit) of the <physics> chains seen by the hooks
    if (mod_enabled && !g_link_entries.empty()) {
        std::lock_guard<std::mutex> plock(g_physics_lock);
        for (const auto& [c, pc] : g_physics_cache)
            if (pc.entry >= 0 && pc.model) solved.push_back({ pc.model, c - cns_chain::kChainInUnit });
    }
    if (mod_enabled)
        for (LinkEntry& le : g_link_entries)
            for (const auto& [m, p] : g_models) {
                if (p != le.model) continue;
                chain_link::Link l{ 0, 0, -1, -1, le.max, le.stiffness };
                auto take = [&](uintptr_t unit) {
                    if (l.slot_a < 0 && (l.slot_a = chain_link::slot_of(unit, le.a)) >= 0) l.chain_a = unit;
                    if (l.slot_b < 0 && (l.slot_b = chain_link::slot_of(unit, le.b)) >= 0) l.chain_b = unit;
                };
                for (const Spawned& s : g_spawned) {
                    if (s.model != m || !chain_exists(s) || cns_chain::model_of(s.chain) != m) continue;
                    take(s.chain);
                }
                for (const auto& [model, unit] : solved)
                    if (model == m && *(uintptr_t*)(unit + cns_chain::kChainInUnit + kCnsModel) == m) take(unit);
                const char* problem = nullptr;
                if (l.slot_a < 0 || l.slot_b < 0) problem = "isn't held by a chain listed on the model (a <chain>, or a <physics> one)";
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

// The live sheets: for each listed <cloth> and each model out with its path, its roots' tendrils among the model's chains
// on our solver (the hooks' cache: <physics> chains, the game's own and <chain>s alike), handed to the solver.
void update_cloths() {
    std::vector<chain_solver::Cloth> live;
    if (mod_enabled && !g_cloth_entries.empty()) {
        struct Solved { uintptr_t chain, model; chain_solver::Settings settings; };
        std::vector<Solved> solved;
        {
            std::lock_guard<std::mutex> plock(g_physics_lock);
            for (const auto& [c, pc] : g_physics_cache)
                if (pc.entry >= 0 && pc.model && *(uintptr_t*)(c + kCnsModel) == pc.model) solved.push_back({ c, pc.model, pc.settings });
        }
        for (ClothEntry& ce : g_cloth_entries)
            for (const auto& [m, p] : g_models) {
                if (p != ce.model) continue;
                chain_solver::Cloth cloth;
                cloth.model = m;
                cloth.closed = ce.closed;
                cloth.stretch = ce.stretch;
                cloth.shear = ce.shear;
                cloth.bend = ce.bend;
                int missing = -1;
                for (int root : ce.roots) {
                    bool found = false;
                    for (const Solved& sv : solved) {
                        if (sv.model != m) continue;
                        const uintptr_t unit = sv.chain - cns_chain::kChainInUnit;   // ChainLink's convention
                        const int slot = chain_link::slot_of(unit, root);
                        if (slot < 0 || !chain_link::is_root(unit, slot)) continue;
                        cloth.members.push_back({ sv.chain, slot, sv.settings });
                        found = true;
                        break;
                    }
                    if (!found) { missing = root; break; }
                }
                if (missing >= 0) {
                    if (!ce.warned) spdlog::warn("ModelExtension: {}: joint {} isn't the first joint of a tendril on the hook's solver (a <physics> chain on the model)", ce.source, missing);
                    ce.warned = true;
                    continue;
                }
                live.push_back(std::move(cloth));
            }
    }
    g_cloths_live = (int)live.size();
    chain_solver::set_cloths(std::move(live));
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
            remove_shapes(m, true);
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
        shape_for(m, it->second);
    }
    if (mod_enabled) { apply_hidden(); update_cns(); update_shapes(); }
    update_links();
    update_cloths();
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

// The hook's chain solver's debug view: each tendril's animated targets (cyan), where its substep targets started this
// frame (the carried previous pose, blue) and its solved points (orange), as the last solve left them.
void draw_targets() {
    auto pt = [](const std::vector<float>& v, size_t i) { return glm::vec3(v[i * 3], v[i * 3 + 1], v[i * 3 + 2]); };
    for (const chain_solver::DebugPath& d : chain_solver::debug_paths()) {
        const size_t n = d.points.size() / 3;
        for (size_t i = 0; i + 1 < n; ++i) {
            w2s::DrawLine3D(pt(d.prev_targets, i), pt(d.prev_targets, i + 1), IM_COL32(60, 90, 255, 255));
            w2s::DrawLine3D(pt(d.targets, i), pt(d.targets, i + 1), IM_COL32(0, 255, 255, 255), 2.0f);
            w2s::DrawLine3D(pt(d.points, i), pt(d.points, i + 1), IM_COL32(255, 140, 0, 255), 2.0f);
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
    if (g_excluded.count(model)) return;
    if (std::find(g_pending.begin(), g_pending.end(), model) == g_pending.end()) g_pending.push_back(model);
}

// The uModel destructor body 0x9E6AB0 (this@ecx), its entry (every derived destructor calls it; the joints are freed
// right after): the model's chains and attachments come off while the joints are still there, and the model is forgotten.
// One of our attachments the game deletes itself (a room change kills its line) is forgotten, and its owner, when still
// alive, is queued again so the next frame attaches a new one.
void __stdcall on_model_dtor(uintptr_t model) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    g_excluded.erase(model);
    g_pending.erase(std::remove(g_pending.begin(), g_pending.end(), model), g_pending.end());
    drop_cns_source(model);
    drop_links(model, 0);
    remove_cns(model, false);
    remove_shapes(model, false);
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

void exclude(uintptr_t model) {
    if (!model) return;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    g_excluded.insert(model);
    g_pending.erase(std::remove(g_pending.begin(), g_pending.end(), model), g_pending.end());
    auto it = g_models.find(model);
    if (it == g_models.end()) return;
    despawn_of(model);
    detach_of(model);
    remove_cns(model, true);
    remove_shapes(model, true);
    drop_links(model, 0);
    g_models.erase(it);
}

std::vector<uintptr_t> attachments_of(uintptr_t owner) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    std::vector<uintptr_t> out;
    for (const Attached& a : g_attached)
        if (a.owner == owner) out.push_back(a.model);
    return out;
}

bool hidden(uintptr_t model) {
    if (!mod_enabled || !model) return false;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    auto it = g_models.find(model);
    return it != g_models.end() && is_hidden(it->second);
}

// `chain`'s <physics> settings into `out`; false: the game solves it.
bool physics_for(uintptr_t chain, chain_solver::Settings& out) {
    if (!mod_enabled || !g_any_physics.load(std::memory_order_acquire)) return false;
    const uintptr_t res = *(uintptr_t*)(chain + kCnsCltRes), model = *(uintptr_t*)(chain + kCnsModel);
    std::lock_guard<std::mutex> lock(g_physics_lock);
    auto it = g_physics_cache.find(chain);
    if (it == g_physics_cache.end() || it->second.res != res || it->second.model != model) {
        PhysicsCache c{ res, model, -1, {} };
        if (res && model) {
            const char* p = (const char*)(res + kResPath);
            const std::string clt = normalize(std::string(p, strnlen(p, 64))), path = model_path(model);
            for (size_t e = 0; e < g_physics_entries.size(); ++e)
                if (g_physics_entries[e].clt == clt && g_physics_entries[e].model == path) {
                    c.entry = (int)e;
                    c.settings = g_physics_entries[e].settings;
                    break;
                }
        }
        if (it != g_physics_cache.end() && it->second.entry >= 0) chain_solver::forget(chain);   // its points were another file's
        it = g_physics_cache.insert_or_assign(chain, c).first;
    }
    if (it->second.entry < 0) return false;
    out = it->second.settings;
    return true;
}

// cCnsChain::move 0x42DB00 (this@ecx, retn) and cCnsChain::calcJoint 0x430A00 (this@ecx, joint, model; retn 8), function
// hooks: a chain a <physics> entry names still moves as the game moves it (wind, turbulence, the blend fade, the reset
// timer) and is solved by ours.
// Every chain runs at its character's speed (chain_rate): the step is scaled by it, and at 0 both solvers keep the stored
// result (the game's: RE note chain_cnschain.md 3.2b; ours: no pending time). The game's move still runs (it registers
// the chain on the model's joints).
void __fastcall detour_chain_move(uintptr_t chain, int edx) {
    ((void(__fastcall*)(uintptr_t, int))orig_chain_move)(chain, edx);
    const float rate = chain_rate(*(uintptr_t*)(chain + kChainModel));
    float& step = *(float*)(chain + kChainStep);
    step = rate > 0.0f ? std::min(step * rate, 2.0f) : 0.0f;
    chain_solver::Settings s;
    if (rate > 0.0f && physics_for(chain, s)) chain_solver::on_move(chain, rate);
}

void __fastcall detour_chain_joint(uintptr_t chain, int edx, uintptr_t joint, uintptr_t model) {
    chain_solver::Settings s;
    if (physics_for(chain, s)) { chain_solver::calc_joint(chain, joint, model, s); return; }
    // the game's solver: a fixed-step chain (a player's coat) solves with 1.0 whatever the step, so while it's stopped the
    // fixed step is lifted for the call, and the zero step keeps the stored result
    uint8_t& fixed = *(uint8_t*)(chain + kChainFixedStep);
    if (fixed && *(float*)(chain + kChainStep) <= 0.0f) {
        fixed = 0;
        ((void(__fastcall*)(uintptr_t, int, uintptr_t, uintptr_t))orig_chain_joint)(chain, edx, joint, model);
        fixed = 1;
        return;
    }
    ((void(__fastcall*)(uintptr_t, int, uintptr_t, uintptr_t))orig_chain_joint)(chain, edx, joint, model);
}

// ~cCnsChain 0x42C7A0 (esi = the cCnsChain), its entry: the destructor clears the constraint of EVERY joint of its model,
// which would drop constraints that claim their joint only once (Nero's coat joint 0 cCnsMatrix, installed by
// uPlayerNero::initModel: its move is a nullsub). The game deletes our chains itself when a room change kills their line,
// so for one of ours the chain lets go of its own joints and its model pointer is cleared first: the sweep then sees no
// model. The record stays: the next check finds the unit gone and spawns the chain again for a model still alive.
void __stdcall on_chain_dtor(uintptr_t cns) {
    {
        std::lock_guard<std::mutex> lock(g_physics_lock);
        g_physics_cache.erase(cns);
    }
    chain_solver::forget(cns);
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

model_shape::List parse_weights(const pugi::xml_node c) {
    model_shape::List l;
    l.shw = normalize(c.attribute("shw").as_string());
    l.speed = c.attribute("speed").as_float(1.0f);
    l.rate = c.attribute("rate").as_float(1.0f);
    l.frame = c.attribute("frame").as_float(0.0f);
    l.loop = c.attribute("loop").as_bool(true);
    return l;
}

// <shape shp= [shw= speed= rate= frame= loop=]> [<weights shw= [speed=] [rate=] [frame=] [loop=]/>]... </shape>
void parse_shape(const pugi::xml_node c, const std::string& model, const std::string& file) {
    ShapeEntry se;
    se.model = model;
    se.shp = normalize(c.attribute("shp").as_string());
    se.source = file + " <shape> " + std::filesystem::path(se.shp).filename().string();
    if (se.shp.empty()) { spdlog::warn("ModelExtension: {}: a <shape> of {} without shp", file, model); return; }
    if (c.attribute("shw")) se.lists.push_back(parse_weights(c));
    for (const pugi::xml_node w : c.children("weights")) {
        if ((int)se.lists.size() == model_shape::kLists) {
            spdlog::warn("ModelExtension: {}: more than {} weight lists, the rest are ignored", se.source, model_shape::kLists);
            break;
        }
        model_shape::List l = parse_weights(w);
        if (l.shw.empty()) { spdlog::warn("ModelExtension: {}: a <weights> without shw", se.source); continue; }
        se.lists.push_back(std::move(l));
    }
    if (std::any_of(g_shape_entries.begin(), g_shape_entries.end(), [&](const ShapeEntry& o) { return o.model == model; }))
        spdlog::warn("ModelExtension: {}: {} already has a <shape>; a model takes one, the first listed", se.source, model);
    g_shape_entries.push_back(std::move(se));
}

// A <physics> setting that may vary along the tendril: one value, or 2-8 separated by spaces or commas (root to tail),
// each clamped to [lo, hi]. False (with the warning) when it isn't numbers or holds more than 8.
bool parse_curve(const pugi::xml_node c, const char* name, chain_solver::Curve& out, float lo, float hi, const std::string& source) {
    const char* text = c.attribute(name).as_string(nullptr);
    if (!text) return true;
    chain_solver::Curve cv;
    cv.n = 0;
    const char* p = text;
    while (*p) {
        while (*p == ' ' || *p == ',' || *p == '	') ++p;
        if (!*p) break;
        char* end = nullptr;
        const float f = std::strtof(p, &end);
        if (end == p || cv.n == 8) {
            spdlog::warn("ModelExtension: {}: {}=\"{}\" (1 to 8 numbers, root to tail)", source, name, text);
            return false;
        }
        cv.v[cv.n++] = std::clamp(f, lo, hi);
        p = end;
    }
    if (cv.n == 0) return true;
    out = cv;
    return true;
}

// <physics clt= [substeps=] [iterations=] [stiffness=] [pin=] [inertia=] [inertia_move=] [inertia_turn=] [limit=]
// [friction=] [damping=] [damp_to=] [radius=] [gravity=] [ground=] [self=] [backstop=] [axis=]/>; stiffness, pin, limit,
// damping, radius and gravity may be lists (chain_solver.md V2.2); damp_to = animation (default) or world (V2.5)
void parse_physics(const pugi::xml_node c, const std::string& model, const std::string& file) {
    PhysicsEntry pe;
    pe.model = model;
    pe.clt = normalize(c.attribute("clt").as_string());
    pe.source = file + " <physics> " + std::filesystem::path(pe.clt).filename().string();
    if (pe.clt.empty()) { spdlog::warn("ModelExtension: {}: a <physics> of {} without clt", file, model); return; }
    chain_solver::Settings& s = pe.settings;
    s.substeps = std::clamp(c.attribute("substeps").as_int(s.substeps), 1, 16);
    s.iterations = std::clamp(c.attribute("iterations").as_int(s.iterations), 1, 8);
    if (!parse_curve(c, "stiffness", s.stiffness, 0.0f, 1.0f, pe.source) || !parse_curve(c, "pin", s.pin, 0.0f, 1.0f, pe.source)
        || !parse_curve(c, "limit", s.limit, 0.0f, 180.0f, pe.source) || !parse_curve(c, "damping", s.damping, 0.0f, 1.0f, pe.source)
        || !parse_curve(c, "radius", s.radius, 0.0f, 1000.0f, pe.source) || !parse_curve(c, "gravity", s.gravity, -100.0f, 100.0f, pe.source))
        return;
    if (c.attribute("inertia")) s.inertia = std::clamp(c.attribute("inertia").as_float(1.0f), 0.0f, 1.0f);
    if (c.attribute("inertia_move")) s.inertia_move = std::clamp(c.attribute("inertia_move").as_float(1.0f), 0.0f, 1.0f);
    if (c.attribute("inertia_turn")) s.inertia_turn = std::clamp(c.attribute("inertia_turn").as_float(1.0f), 0.0f, 1.0f);
    s.friction = std::clamp(c.attribute("friction").as_float(s.friction), 0.0f, 1.0f);
    s.ground = c.attribute("ground").as_bool(s.ground);
    s.self = c.attribute("self").as_bool(s.self);
    s.backstop = c.attribute("backstop").as_bool(s.backstop);
    if (const char* to = c.attribute("damp_to").as_string(nullptr)) {
        const std::string v = to;
        if (v == "world") s.damp_world = true;
        else if (v == "animation") s.damp_world = false;
        else { spdlog::warn("ModelExtension: {}: damp_to \"{}\" (animation or world)", pe.source, to); return; }
    }
    if (const char* axis = c.attribute("axis").as_string(nullptr)) {
        s.axis = parse_axis(axis);
        if (s.axis < 0) { spdlog::warn("ModelExtension: {}: axis \"{}\" (x, y, z with + or -)", pe.source, axis); return; }
    }
    if (std::any_of(g_physics_entries.begin(), g_physics_entries.end(), [&](const PhysicsEntry& o) { return o.model == model && o.clt == pe.clt; })) {
        spdlog::warn("ModelExtension: {}: {} on {} already has a <physics>; the first listed wins", pe.source, pe.clt, model);
        return;
    }
    g_physics_entries.push_back(std::move(pe));
}

// <cloth roots= [closed=] [stretch=] [shear=] [bend=]/>
void parse_cloth(const pugi::xml_node c, const std::string& model, const std::string& file) {
    ClothEntry ce;
    ce.model = model;
    ce.source = file + " <cloth> on " + std::filesystem::path(model).filename().string();
    float roots[32];
    const int n = parse_floats(c.attribute("roots").as_string(), roots, 32);
    for (int i = 0; i < n; ++i) ce.roots.push_back((int)roots[i]);
    ce.closed = c.attribute("closed").as_bool(false);
    ce.stretch = std::clamp(c.attribute("stretch").as_float(1.0f), 0.0f, 1.0f);
    ce.shear = std::clamp(c.attribute("shear").as_float(0.5f), 0.0f, 1.0f);
    ce.bend = std::clamp(c.attribute("bend").as_float(0.0f), 0.0f, 1.0f);
    ce.warned = false;
    if (ce.roots.size() < 2) { spdlog::warn("ModelExtension: {}: needs roots=, two or more joint numbers", ce.source); return; }
    for (size_t i = 0; i < ce.roots.size(); ++i) {
        if (ce.roots[i] < 0 || ce.roots[i] > 255) { spdlog::warn("ModelExtension: {}: root {} isn't a joint number 0-255", ce.source, ce.roots[i]); return; }
        for (size_t j = 0; j < i; ++j)
            if (ce.roots[j] == ce.roots[i]) { spdlog::warn("ModelExtension: {}: root {} is listed twice", ce.source, ce.roots[i]); return; }
    }
    for (const ClothEntry& o : g_cloth_entries)
        if (o.model == model)
            for (int r : o.roots)
                if (std::find(ce.roots.begin(), ce.roots.end(), r) != ce.roots.end()) {
                    spdlog::warn("ModelExtension: {}: root {} is already in another <cloth> ({}); a tendril takes one, the first listed", ce.source, r, o.source);
                    return;
                }
    g_cloth_entries.push_back(std::move(ce));
}
} // namespace

// The lists: nativePC\model_ext\*.xml (ModelExtension.hpp):
//   <extensions><model path= [collision_model=]><chain clt= [col=] [collision_model=]/> ...</model>
//               <attach path= to= [joint=] [mode=rigid|skin] [pos=] [rot=] [scale=] [line=]/> ...
//               <hide path=/> ...</extensions>
// and inside a <model>: <pin .../> <aim .../> <offset .../> (parse_cns), <link .../>, <shape ...> (parse_shape).
// A chain's collision_model overrides its model's; a col without any collision model is ignored (collision needs both).
void reload() {
    namespace fs = std::filesystem;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    despawn_of(0);                                   // the records index the old lists
    detach_of(0);
    remove_cns(0, true);
    remove_shapes(0, true);
    show_hidden();                                   // the lists still listed hide theirs again next frame
    g_entries.clear();
    g_attach_entries.clear();
    g_hide_entries.clear();
    g_cns_entries.clear();
    g_link_entries.clear();
    g_shape_entries.clear();
    {
        std::lock_guard<std::mutex> plock(g_physics_lock);   // the hooks read the entries
        g_any_physics.store(false, std::memory_order_release);
        g_physics_entries.clear();
        g_physics_cache.clear();
        chain_solver::forget_all();
    }
    g_cloth_entries.clear();
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
                    else if (tag == "shape") parse_shape(c, model, name);
                    else if (tag == "physics") parse_physics(c, model, name);
                    else if (tag == "cloth") parse_cloth(c, model, name);
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
        spdlog::info("ModelExtension: {} chain(s), {} attachment(s), {} constraint(s), {} link(s), {} shape(s), {} physics, {} cloth(s), {} hidden model(s) listed in {}",
                     g_entries.size(), g_attach_entries.size(), g_cns_entries.size(), g_link_entries.size(), g_shape_entries.size(),
                     g_physics_entries.size(), g_cloth_entries.size(), g_hide_entries.size(), folder);
    } else {
        spdlog::info("ModelExtension: no {} folder, nothing listed", folder);
    }
    g_any_physics.store(!g_physics_entries.empty(), std::memory_order_release);
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
           "  <!-- Nero's coat (the game's own six coat chains) on the hook's chain solver: substeps, every segment colliding,\n"
           "       the floor under it, a little pull toward the animated shape (one <physics> per chain file):\n"
           "  <model path=\"model\\game\\pl000\\pl000_03\">\n"
           "    <physics clt=\"model\\game\\pl000\\pl000_03_00\" ground=\"true\" stiffness=\"0.05\"/>\n"
           "  </model>\n"
           "  -->\n"
           "  <!-- the coat's six tails (all on the hook's solver: a <physics> per chain file) joined into one sheet; roots are the\n"
           "       first joint of each tail, in order across the coat:\n"
           "  <model path=\"model\\game\\pl000\\pl000_03\">\n"
           "    <cloth roots=\"1 8 14 20 26 32\" stretch=\"1\" shear=\"0.5\"/>\n"
           "  </model>\n"
           "  -->\n"
           "  <!-- morph targets (shape keys exported by Albam) on a model, their weights played from a .shw, looping:\n"
           "  <model path=\"model\\game\\pl000\\pl000\">\n"
           "    <shape shp=\"model\\game\\pl000\\pl000\" shw=\"model\\game\\pl000\\pl000_idle\" speed=\"1\"/>\n"
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
    // our chain solver for the chains a <physics> entry names (detour_chain_move / detour_chain_joint)
    if (!install_hook_absolute(0x42DB00, hook_chain_move, &detour_chain_move, &orig_chain_move)
        || !install_hook_absolute(0x430A00, hook_chain_joint, &detour_chain_joint, &orig_chain_joint))
        spdlog::error("ModelExtension: failed to hook cCnsChain::move / calcJoint (0x42DB00 / 0x430A00): <physics> entries do nothing");
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
    if (mod_enabled && show_targets) draw_targets();
}

void ModelExtension::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (ImGui::Checkbox(_("Model extension"), &mod_enabled)) {
        std::lock_guard<std::recursive_mutex> lock(g_lock);
        if (mod_enabled) { for (const auto& [m, p] : g_models) g_pending.push_back(m); }   // the models already out get theirs
        else { despawn_of(0); detach_of(0); remove_cns(0, true); remove_shapes(0, true); show_hidden(); }
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
                  "<shape shp= shw=> (inside a <model>): morph targets (.shp, shape keys exported by Albam) with their weights played "
                  "from a .shw (speed, loop, frame: where it starts); up to four <weights shw=> inside it play together, mixed by "
                  "rate. Only meshes exported for it move.\n"
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
    if (ImGui::Checkbox(_("Show physics targets"), &show_targets)) chain_solver::set_debug(show_targets);
    ImGui::SameLine();
    help_marker(_("Debug view of the hook's chain solver (<physics> chains): each tail's animated pose, the one it is pulled "
                  "toward, in cyan; where this frame's pull started (last frame's pose, carried along with the character) in "
                  "blue; the solved tail in orange. The Physics list then shows per tail the time and steps of its last solve "
                  "and how far the character's frame, the targets and the tail moved."));
    // readouts: one closed tree node per kind, the counts in its label (the str_id keeps it open while they change)
    if (ImGui::TreeNode("Attachments", _("%d attachment(s) listed, %d out"), (int)g_attach_entries.size(), (int)g_attached.size())) {
        for (size_t e = 0; e < g_attach_entries.size(); ++e) {
            const AttachEntry& ae = g_attach_entries[e];
            const int out = (int)std::count_if(g_attached.begin(), g_attached.end(), [&](const Attached& a) { return a.entry == e; });
            if (ae.mode == model_attach::SKIN) ImGui::BulletText("%s on %s, skinned: %d  (%s)", ae.path.c_str(), ae.to.c_str(), out, ae.source.c_str());
            else ImGui::BulletText("%s on %s, joint %d: %d  (%s)", ae.path.c_str(), ae.to.c_str(), ae.joint, out, ae.source.c_str());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Constraints", _("%d constraint(s) listed, %d on a joint"), (int)g_cns_entries.size(),
                        (int)std::count_if(g_cns.begin(), g_cns.end(), [](const CnsLive& c) { return c.installed; }))) {
        for (size_t e = 0; e < g_cns_entries.size(); ++e) {
            const CnsEntry& ce = g_cns_entries[e];
            const int out = (int)std::count_if(g_cns.begin(), g_cns.end(), [&](const CnsLive& c) { return c.entry == e && c.installed; });
            ImGui::BulletText("%s: %d  (%s)", ce.model.c_str(), out, ce.source.c_str());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Links", _("%d link(s) listed, %d live"), (int)g_link_entries.size(), g_links_live)) {
        for (const LinkEntry& le : g_link_entries)
            ImGui::BulletText("%s: %d - %d, at most %.1f cm  (%s)", le.model.c_str(), le.a, le.b, le.max, le.source.c_str());
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Shapes", _("%d shape(s) listed, %d on a model"), (int)g_shape_entries.size(), (int)g_shapes.size())) {
        for (const ShapeEntry& se : g_shape_entries)
            ImGui::BulletText("%s on %s, %d weight list(s)  (%s)", se.shp.c_str(), se.model.c_str(), (int)se.lists.size(), se.source.c_str());
        for (const ShapeLive& l : g_shapes) {
            const ShapeEntry& se = g_shape_entries[l.entry];
            std::string frames;
            char buf[48];
            for (int i = 0; i < model_shape::kLists; ++i) {
                if (!model_shape::list_on(l.model, i)) continue;
                std::snprintf(buf, sizeof(buf), "%s%d: %.0f", frames.empty() ? "" : ", ", i, model_shape::frame(l.model, i));
                frames += buf;
            }
            ImGui::BulletText(_("live: %s, %d target(s), lists playing %s, update by %s"), g_models.count(l.model) ? g_models[l.model].c_str() : "?",
                              model_shape::targets(l.model), frames.empty() ? "-" : frames.c_str(),
                              l.runs < 0 ? "(measuring)" : l.runs ? "ModelExtension" : "the game");
        }
        ImGui::TreePop();
    }
    {
        int solved = 0;
        {
            std::lock_guard<std::mutex> plock(g_physics_lock);
            for (const auto& [c, pc] : g_physics_cache) solved += pc.entry >= 0;
        }
        if (ImGui::TreeNode("Physics", _("%d physics entr(ies) listed, %d chain(s) on our solver"), (int)g_physics_entries.size(), solved)) {
            // a setting as listed: one value, or root..tip for a list; "auto" for the default of damping / radius
            auto curve = [](const chain_solver::Curve& cv, const char* fmt, char (&buf)[32]) -> const char* {
                if (cv.unset()) return "auto";
                char a[12], b[12];
                snprintf(a, sizeof(a), fmt, cv.v[0]);
                snprintf(b, sizeof(b), fmt, cv.v[cv.n - 1]);
                if (cv.n > 1) snprintf(buf, sizeof(buf), "%s..%s", a, b); else snprintf(buf, sizeof(buf), "%s", a);
                return buf;
            };
            for (const PhysicsEntry& pe : g_physics_entries) {
                const chain_solver::Settings& s = pe.settings;
                char st[32], pi[32], da[32], li[32];
                ImGui::BulletText("%s on %s: substeps %d, stiffness %s, pin %s, damping %s%s, limit %s, ground %s, self %s  (%s)",
                                  pe.clt.c_str(), pe.model.c_str(), s.substeps, curve(s.stiffness, "%.2f", st), curve(s.pin, "%.2f", pi),
                                  curve(s.damping, "%.2f", da), s.damp_world ? " to world" : "", curve(s.limit, "%.0f", li),
                                  s.ground ? "on" : "off",
                                  s.self ? "on" : "off", pe.source.c_str());
            }
            ImGui::BulletText(_("last tendril / sheet solve: %.3f ms"), chain_solver::stats().last_ms);
            if (ImGui::Button(_("Record physics state"))) chain_solver::request_dump(kPhysicsDumpPath, g_record_frames);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::InputInt(_("frames##physics_record"), &g_record_frames)) g_record_frames = std::clamp(g_record_frames, 1, 3600);
            ImGui::SameLine();
            help_marker(_("Records every <physics> tendril and sheet as the next frames solve them (settings, the chain's "
                          "values, targets, points and speeds before and after, the collision shapes) to " PHYSICS_DUMP_NAME
                          " in the game folder, for replaying them in Albam's preview. 1 frame is about 65 KB per sheet; the "
                          "game may stutter once while the file is written."));
            ImGui::TextWrapped(_("Last recording: %s"), chain_solver::dump_status().c_str());
            if (show_targets) {
                // per tendril: the last solve's time and steps, the reference frame's motion, how far the targets moved
                // since last frame (a - carried a_prev, the most of any node) and how far the solved points sit off them
                for (const chain_solver::DebugPath& d : chain_solver::debug_paths()) {
                    float moved = 0.0f, off = 0.0f;
                    for (size_t i = 0; i + 2 < d.points.size(); i += 3) {
                        const glm::vec3 a(d.targets[i], d.targets[i + 1], d.targets[i + 2]);
                        const glm::vec3 p(d.prev_targets[i], d.prev_targets[i + 1], d.prev_targets[i + 2]);
                        const glm::vec3 x(d.points[i], d.points[i + 1], d.points[i + 2]);
                        moved = std::max(moved, glm::length(a - p));
                        off = std::max(off, glm::length(x - a));
                    }
                    // wrapped: the panel can't be widened, and the end of the line is what matters
                    ImGui::Bullet();
                    ImGui::TextWrapped(_("chain %08X slot %d: T %.2f, %d steps%s, +%d passes; frame %.2f cm %.2f deg; targets moved %.2f cm; tail off targets %.1f cm"),
                                       (unsigned)d.chain, d.root, d.T, d.steps, d.reset ? _(" (reset)") : "", d.extra_passes,
                                       d.ref_move, d.ref_turn, moved, off);
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Cloth", _("%d cloth sheet(s) listed, %d live"), (int)g_cloth_entries.size(), g_cloths_live)) {
            for (const ClothEntry& ce : g_cloth_entries) {
                std::string roots;
                for (int r : ce.roots) roots += (roots.empty() ? "" : " ") + std::to_string(r);
                ImGui::BulletText("%s: roots %s%s, stretch %.2f, shear %.2f, bend %.2f  (%s)", ce.model.c_str(), roots.c_str(),
                                  ce.closed ? " (closed)" : "", ce.stretch, ce.shear, ce.bend, ce.source.c_str());
            }
            ImGui::TreePop();
        }
    }
    if (ImGui::TreeNode("Hidden", _("%d model(s) hidden"), (int)g_hide_entries.size())) {
        for (const HideEntry& h : g_hide_entries) {
            const int out = (int)std::count_if(g_models.begin(), g_models.end(), [&](const auto& m) { return m.second == h.path; });
            ImGui::BulletText("%s: %d  (%s)", h.path.c_str(), out, h.source.c_str());
        }
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Chains", _("%d chain(s) listed, %d out, %d model(s) known"), (int)g_entries.size(), (int)g_spawned.size(), (int)g_models.size())) {
        for (size_t e = 0; e < g_entries.size(); ++e) {
            const ListEntry& le = g_entries[e];
            const int out = (int)std::count_if(g_spawned.begin(), g_spawned.end(), [&](const Spawned& s) { return s.entry == e; });
            ImGui::BulletText("%s -> %s: %d  (%s)", le.chain.c_str(), le.model.c_str(), out, le.source.c_str());
        }
        ImGui::TreePop();
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
    show_targets = cfg.get<bool>("model_extension_show_targets").value_or(false);
    chain_solver::set_debug(show_targets);
    const std::string f = cfg.get("model_extension_folder").value_or(std::string(folder));
    strncpy_s(folder, f.c_str(), sizeof(folder) - 1);
    reload();
}

void ModelExtension::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("model_extension", mod_enabled);
    cfg.set<bool>("model_extension_show_collision", show_collision);
    cfg.set<bool>("model_extension_show_targets", show_targets);
    cfg.set("model_extension_folder", std::string(folder));
}
