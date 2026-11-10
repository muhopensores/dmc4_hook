#pragma once

#include <cstdint>

// Which player a uPlayer is when several play at once (dmc4_hook's co-op, src/dev/coop). Without co-op there is one
// player, the game's own (sDevil4Main's player line top / sMediator::mpPlayer), who has the keyboard.
//
// Use these instead of comparing with devil4_sdk::get_local_player(): co-op swaps sMediator::mpPlayer while code runs on
// behalf of players 2-4 (CoopContext), and a per-player mod must run for every player, reading the keyboard only for the
// one who has it. A player's own pad is already his: mPeripheral (+0x1370) and mInterface (+0x1408) are filled from it.
namespace player_info {

int  index_of(uintptr_t player);       // 0 = player 1, 1-3 = co-op's players 2-4, -1 = not a player
bool has_keyboard(uintptr_t player);   // the keyboard's keys are this player's (co-op's Keyboard setting)

} // namespace player_info
