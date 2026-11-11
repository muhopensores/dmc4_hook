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
bool __stdcall is_player(uintptr_t unit);   // index_of(unit) >= 0
// For asm hooks, in place of comparing with sMediator::mpPlayer: the unit in eax; returns ZF clear (jnz) for a player,
// set (jz) for anything else. Keeps every register, eax included, and xmm0-xmm7.
void asm_is_player();
bool has_keyboard(uintptr_t player);   // the keyboard's keys are this player's (co-op's Keyboard setting)
uintptr_t keyboard_player();           // the player who has the keyboard (0 = nobody, or not in a stage)
int  count();                          // players in the stage (1 without co-op, 0 outside a stage)
uintptr_t player(int index);           // that player's uPlayer, 0 = none
// Where that player's view is on the game's screen, as fractions of it (left, top, right, bottom). False when he has
// none there: without a split only player 1's view shows (full screen); ViewportWindow's window holds a view of its own.
bool view_rect(int index, float rect[4]);

} // namespace player_info
