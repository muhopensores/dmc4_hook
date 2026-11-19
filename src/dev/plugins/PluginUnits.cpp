#include "PluginUnits.hpp"
#include "PluginHost.hpp"
#include "PluginRooms.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../unit_packs/UnitPacks.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/sUnit.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>

// Ids: room index << 16 | unit index in UnitPacks' manifest (rooms sorted by name, units in file order; read once per
// session, so ids stay valid). A spawn is a one-unit .pla handed to a uPlacement on line 1; the uPlacement makes the
// unit on its move line during its first move and kills itself, so what a spawn made is read once the uPlacement is
// gone: the units on that line that weren't there before (and that no other spawn took).
namespace plugins {
namespace {

using namespace unit_packs;

constexpr int kPlacementLine = 1;
constexpr int kGiveUp = 3600;   // frames a uPlacement may linger (paused frames count too)

struct Spawn {
    int id = 0;
    dmc4_plugin* owner = nullptr;
    int unit = 0;
    uintptr_t placement = 0;
    void* room = nullptr;            // the room it was made in: a room change takes everything with it
    int line = 0, frames = 0;
    bool pending = true;
    std::vector<uintptr_t> before;   // the line's units at the spawn
    std::vector<uintptr_t> made;
};

std::vector<Spawn> g_spawns;
int g_next_spawn = 1;
std::map<int, std::set<dmc4_plugin*>> g_loads;   // unit id -> the plugins holding its files
std::string g_text;                               // name / class_name results

std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

const Unit* unit_of(int id, int* room = nullptr, int* index = nullptr) {
    if (id < 0) return nullptr;
    const int r = id >> 16, u = id & 0xFFFF;
    const std::vector<Room>& rs = rooms();
    if (r >= (int)rs.size() || u >= (int)rs[r].units.size()) return nullptr;
    if (room) *room = r;
    if (index) *index = u;
    return &rs[r].units[u];
}

void* current_room() { return rooms_api()->current(); }

std::vector<uintptr_t> line_units(int line) {
    std::vector<uintptr_t> out;
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su || line < 0 || line >= 32) return out;
    for (cUnit* c = (cUnit*)su->mMoveLine[line].mTop; c; c = c->mp_next_unit) out.push_back((uintptr_t)c);
    return out;
}

bool claimed(uintptr_t unit, const Spawn& except) {
    for (const Spawn& s : g_spawns)
        if (&s != &except && std::find(s.made.begin(), s.made.end(), unit) != s.made.end()) return true;
    return false;
}

void kill(Spawn& s) {
    for (uintptr_t u : s.made)
        if (devil4_sdk::check_exists_in_moveline((void*)u, s.line)) kill_unit(u);
    s.made.clear();
}

int __cdecl api_find(const char* room, const char* what, int nth) {
    if (!room || !what) return -1;
    ensure_manifest();
    const std::string r = lower(room), w = lower(what);
    const std::vector<Room>& rs = rooms();
    for (int i = 0; i < (int)rs.size(); ++i) {
        if (lower(rs[i].name) != r) continue;
        int seen = 0;
        for (int k = 0; k < (int)rs[i].units.size(); ++k) {
            const Unit& u = rs[i].units[k];
            if (lower(u.cls) != w && lower(u.name) != w) continue;
            if (seen++ == nth) return i << 16 | k;
        }
    }
    return -1;
}

const char* __cdecl api_name(int id) {
    const Unit* u = unit_of(id);
    if (!u) return nullptr;
    g_text = u->name;
    return g_text.c_str();
}

const char* __cdecl api_class_name(int id) {
    const Unit* u = unit_of(id);
    if (!u) return nullptr;
    g_text = u->cls;
    return g_text.c_str();
}

uint32_t __cdecl api_file_bytes(int id) {
    const Unit* u = unit_of(id);
    return u ? u->stored : 0;
}

int __cdecl api_load(dmc4_plugin* self, int id) {
    int r = 0, k = 0;
    const Unit* u = unit_of(id, &r, &k);
    if (!self || !u) return 0;
    if (!u->stored) return 1;
    if (!u->loaded.arc && !load_unit(r, k)) {
        plugin_log(self, DMC4_LOG_ERROR, "units->load " + u->name + ": " + u->loaded.error);
        return 0;
    }
    g_loads[id].insert(self);
    return 1;
}

void __cdecl api_release(dmc4_plugin* self, int id) {
    int r = 0, k = 0;
    if (!self || !unit_of(id, &r, &k)) return;
    auto it = g_loads.find(id);
    if (it == g_loads.end() || !it->second.erase(self)) return;
    if (it->second.empty()) {
        release_unit(r, k);
        g_loads.erase(it);
    }
}

int __cdecl api_spawn(dmc4_plugin* self, int id, const float* pos, float yaw) {
    int r = 0, k = 0;
    const Unit* u = unit_of(id, &r, &k);
    if (!self || !u) return 0;
    Spawn s;
    s.line = u->line;
    s.before = line_units(s.line);
    std::string error;
    s.placement = spawn_unit(r, k, pos, error, &yaw);
    if (!s.placement) {
        plugin_log(self, DMC4_LOG_ERROR, "units->spawn " + u->name + ": " + error);
        return 0;
    }
    s.id = g_next_spawn++;
    s.owner = self;
    s.unit = id;
    s.room = current_room();
    g_spawns.push_back(std::move(s));
    return g_spawns.back().id;
}

int __cdecl api_spawned(int id, void** out, int max) {
    for (const Spawn& s : g_spawns) {
        if (s.id != id) continue;
        if (s.pending) return -1;
        int n = 0;
        for (uintptr_t u : s.made) {
            if (!devil4_sdk::check_exists_in_moveline((void*)u, s.line)) continue;
            if (out && n < max) out[n] = (void*)u;
            ++n;
        }
        return n;
    }
    return 0;
}

void __cdecl api_despawn(dmc4_plugin* self, int id) {
    for (size_t i = 0; i < g_spawns.size(); ++i) {
        Spawn& s = g_spawns[i];
        if (s.id != id || s.owner != self) continue;
        if (s.pending) { s.owner = nullptr; return; }   // units_frame kills it once made
        kill(s);
        g_spawns.erase(g_spawns.begin() + i);
        return;
    }
}

void __cdecl api_despawn_all(dmc4_plugin* self) {
    std::vector<int> ids;
    for (const Spawn& s : g_spawns)
        if (s.owner == self) ids.push_back(s.id);
    for (int id : ids) api_despawn(self, id);
}

const dmc4_units_api g_units = {
    sizeof(dmc4_units_api),
    api_find,
    api_name,
    api_class_name,
    api_file_bytes,
    api_load,
    api_release,
    api_spawn,
    api_spawned,
    api_despawn,
    api_despawn_all,
};

} // namespace

const dmc4_units_api* units_api() { return &g_units; }

void units_frame() {
    void* room = g_spawns.empty() ? nullptr : current_room();
    for (size_t i = 0; i < g_spawns.size();) {
        Spawn& s = g_spawns[i];
        if (s.room != room) { g_spawns.erase(g_spawns.begin() + i); continue; }   // the room's exit took them
        if (s.pending) {
            if (devil4_sdk::check_exists_in_moveline((void*)s.placement, kPlacementLine) && ++s.frames < kGiveUp) { ++i; continue; }
            s.pending = false;
            for (uintptr_t c : line_units(s.line))
                if (std::find(s.before.begin(), s.before.end(), c) == s.before.end() && !claimed(c, s)) s.made.push_back(c);
            s.before.clear();
            if (s.made.empty()) {
                if (s.owner) plugin_log(s.owner, DMC4_LOG_WARN, "units: a spawn made nothing (see the log)");
                g_spawns.erase(g_spawns.begin() + i);
                continue;
            }
            if (!s.owner) { kill(s); g_spawns.erase(g_spawns.begin() + i); continue; }   // despawned while pending
        }
        ++i;
    }
}

void release_units(dmc4_plugin* self) {
    api_despawn_all(self);
    std::vector<int> ids;
    for (const auto& [id, owners] : g_loads)
        if (owners.count(self)) ids.push_back(id);
    for (int id : ids) api_release(self, id);
}

} // namespace plugins
