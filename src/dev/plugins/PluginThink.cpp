#include "PluginThink.hpp"
#include "PluginHost.hpp"
#include "../../mods/EnemyAI.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MtDTI.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/sUnit.hpp"
#include <algorithm>
#include <vector>

namespace plugins {
namespace {

struct Listener {
    int id;
    dmc4_plugin* owner;
    dmc4_think_fn fn;
    void* user;
};
std::vector<Listener> g_listeners;   // game thread only (registration happens there or in init)
int g_next = 1;

constexpr int kEnemyLine = 15;

// The unit holding `mgr`: a think manager lies inside its enemy (base <= mgr < base + MtDTI::m_size), as EnemyAI's
// provoke code finds it. Enemies are on line 15.
uintptr_t owner_of(uintptr_t mgr) {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return 0;
    for (cUnit* c = (cUnit*)su->mMoveLine[kEnemyLine].mTop; c; c = c->mp_next_unit) {
        const uintptr_t u = (uintptr_t)c;
        if (mgr < u) continue;
        const MtDTI* dti = static_cast<const MtDTI*>(devil4_sdk::get_DTI(c));
        if (dti && mgr < u + dti->m_size) return u;
    }
    return 0;
}

struct Ctx {
    dmc4_think_fn fn;
    void* user;
    dmc4_think_call* call;
};
void trampoline(void* p) {
    Ctx* c = static_cast<Ctx*>(p);
    c->fn(c->user, c->call);
}

int filter(uintptr_t mgr, uintptr_t caller, int action) {
    if (g_listeners.empty()) return action;
    dmc4_think_call call{ (void*)owner_of(mgr), (void*)mgr, caller, action };
    const std::vector<Listener> now = g_listeners;   // a callback may remove itself
    for (const Listener& l : now) {
        Ctx ctx{ l.fn, l.user, &call };
        if (!run_guarded(l.owner, trampoline, &ctx, "an on_think callback")) {   // it crashed: stopped, its answer dropped
            call.action = action;
            continue;
        }
    }
    return call.action;
}

} // namespace

int __cdecl think_add(dmc4_plugin* self, dmc4_think_fn fn, void* user) {
    if (!self || !fn) return 0;
    enemy_think::set_filter(filter);
    g_listeners.push_back(Listener{ g_next, self, fn, user });
    return g_next++;
}

void __cdecl think_remove(dmc4_plugin* self, int id) {
    g_listeners.erase(std::remove_if(g_listeners.begin(), g_listeners.end(),
                                     [&](const Listener& l) { return l.owner == self && l.id == id; }),
                      g_listeners.end());
}

void release_think(dmc4_plugin* self) {
    g_listeners.erase(std::remove_if(g_listeners.begin(), g_listeners.end(), [&](const Listener& l) { return l.owner == self; }),
                      g_listeners.end());
}

} // namespace plugins
