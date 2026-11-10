#include "Players.hpp"
#include "Devil4.hpp"
#include "../dev/coop/Coop.hpp"
#include "../dev/coop/CoopPads.hpp"

namespace players {

int index_of(uintptr_t player) {
    if (!player) {
        return -1;
    }
    if (Coop::mod_enabled) {
        for (int i = 0; i < coop::kMaxPlayers; i++) {
            if (coop::players[i].unit == player) {
                return i;
            }
        }
        if (coop::players[0].unit) {
            return -1;
        }
    }
    // co-op off, or turned on mid-stage before player 1 was registered: the game's player
    return player == (uintptr_t)devil4_sdk::get_local_player() ? 0 : -1;
}

bool has_keyboard(uintptr_t player) {
    const int i = index_of(player);
    if (i < 0) {
        return false;
    }
    return Coop::mod_enabled ? i == coop_pads::keyboard_owner : i == 0;
}

} // namespace players
