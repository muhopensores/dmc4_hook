#pragma once

// CoopDiag: the measurements the player-context step needs, and the serial context lines: on by the toggle, and forced
// on by Coop while players 2-4 are in the room (CoopContext swaps mpPlayer at the serial call sites only).
//
// sUnit runs its 32 move lines through the job threads when a line's mParallel bit is set (sUnit::sUnit 0x8DBF20 sets it
// on every line; RE note Vibed\RE\sunit_moveline_findings.md). Swapping sMediator's mpPlayer / mpCamera around each
// unit's move is only safe on lines that run serially, so the toggle clears mParallel on the context lines (players 13,
// stand 14, enemies 15 / 16 / 18, camera 23, HUD 25) and puts the saved bits back when turned off. The readout shows the
// job thread count (sMain mJobThreadNum +0x8C / mNextJobThreadNum +0x94, sDevil4Main 0xE5574C), every line's flags and
// unit count, and the frame time, to compare serial against parallel.

namespace coop_diag {

extern bool serial_lines;

void set_serial(bool serial);
void set_forced(bool forced);
bool serial_now();
void game_tick();   // applies the line flags; game thread only (the game ORs mPause into the same dword)
void on_frame(float ms);
void draw();

} // namespace coop_diag
