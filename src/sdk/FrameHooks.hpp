#pragma once

#include <cstdint>

// Game-thread hook points shared by several features (plan Vibed/GameMCP/README.md, D4): one address takes one hook,
// so the plugins, the debug server and later online co-op register callbacks here instead of hooking the same site.
//
// FRAME_END: sDevil4Main::move after sCollisionGame::checkAttackMain (0x8AF03F, mov ecx, [ebp+0x10364]): the game
// thread, once per game frame, every unit ticked and the frame's hits resolved.
//
// TICK_START: the start of a game tick's simulation, before any unit moves. It has no hook site: it runs where
// tick_start() is called, which CoopOnline does for every tick it simulates, forward and re-simulated alike. While
// ticks_driven() is true (a netplay mode or the rollback probe runs the ticks), per-tick game work belongs here and not
// at a site in sDevil4Main::move's prelude, which runs once per presented frame and never for a re-simulated tick.
//
// Callbacks run in the order they were added, on the game thread. Adding / removing is safe from any thread (also from
// inside a callback: the change applies from the next run).
namespace frame_hooks {

enum Point : int { FRAME_END = 0, TICK_START, POINT_COUNT };

using Fn = void (*)(void* user);

bool install();                                         // installs the hooks once (idempotent); false if a site differs
int add(Point point, Fn fn, void* user, const char* owner);   // > 0 id, 0 on failure
void remove(int id);
uint64_t frames();                                      // FRAME_END runs so far
// Runs FRAME_END by hand, for code that replaces the block holding its site (CoopOnline's netplay tick runs its
// own copy of 0x8AEFD5..0x8AF04C and calls this once per forward tick, never for re-simulated ones).
void frame_end();
void tick_start(int frame);                              // runs TICK_START (CoopOnline, at the start of each simulated tick)
// The session frame the tick being simulated belongs to, the same on every machine (-1 outside a netplay tick: the
// rollback probe's ticks, or no tick driven). Valid from TICK_START until the next tick.
int tick_frame();
void set_ticks_driven(bool driven);                     // CoopOnline, once per presented frame before the tick
bool ticks_driven();

} // namespace frame_hooks
