#include "PluginPlayers.hpp"
#include "PluginHost.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../sdk/Players.hpp"
#include "../coop/CoopContext.hpp"

// player_info (sdk/Players.hpp) answers with and without co-op: without it there is one player, the game's own.
// run_as uses CoopContext's swap of sMediator::mpPlayer (+0x24), its camera, pad and lock-on values: the same one Coop
// does around a player 2-4's unit updates. That swap doesn't nest (swap_to refuses while one is active), so run_as
// leaves only a swap it made itself.

namespace plugins {
namespace {

constexpr uintptr_t kMediator = 0x00E558B8;   // sMediator::mpInstance
constexpr uintptr_t kMedPlayer = 0x24;        // sMediator::mpPlayer: the player most game code reads

int __cdecl count() { return player_info::count(); }
void* __cdecl get(int index) { return reinterpret_cast<void*>(player_info::player(index)); }
int __cdecl index_of(void* unit) { return player_info::index_of(reinterpret_cast<uintptr_t>(unit)); }
int __cdecl has_keyboard(void* player) { return player && player_info::has_keyboard(reinterpret_cast<uintptr_t>(player)); }
void* __cdecl keyboard_player() { return reinterpret_cast<void*>(player_info::keyboard_player()); }
int __cdecl view_rect(int index, float* rect) { return rect && player_info::view_rect(index, rect); }

void* __cdecl current() {
    const uintptr_t med = *reinterpret_cast<const uintptr_t*>(kMediator);
    return med ? *reinterpret_cast<void* const*>(med + kMedPlayer) : nullptr;
}

int __cdecl run_as(dmc4_plugin* self, void* player, dmc4_callback fn, void* user) {
    const uintptr_t p = reinterpret_cast<uintptr_t>(player);
    if (!self || !fn || player_info::index_of(p) < 0) return 0;
    if (current() == player) {   // already his context (player 1 outside any swap, or inside his own update)
        return run_guarded(self, fn, user, "a players->run_as callback") ? 1 : 0;
    }
    if (!coop_context::enter_player(p)) return 0;   // another player's context is active: fn isn't called
    const bool ok = run_guarded(self, fn, user, "a players->run_as callback");
    coop_context::leave();   // also after a crash: player 1 must get his place back
    return ok ? 1 : 0;
}

const dmc4_players_api g_players = {
    sizeof(dmc4_players_api),
    count,
    get,
    index_of,
    has_keyboard,
    keyboard_player,
    view_rect,
    current,
    run_as,
};

} // namespace

const dmc4_players_api* players_api() { return &g_players; }

} // namespace plugins
