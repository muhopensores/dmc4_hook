#pragma once

#include <cstdint>

// Game-thread hook points shared by several features (plan Vibed/GameMCP/README.md, D4): one address takes one hook,
// so the plugins, the debug server and later online co-op register callbacks here instead of hooking the same site.
//
// FRAME_END: sDevil4Main::move after sCollisionGame::checkAttackMain (0x8AF03F, mov ecx, [ebp+0x10364]): the game
// thread, once per game frame, every unit ticked and the frame's hits resolved. (TICK_START, 0x8AEFA1, joins when
// online co-op moves its tick work here.)
//
// Callbacks run in the order they were added, on the game thread. Adding / removing is safe from any thread (also from
// inside a callback: the change applies from the next run).
namespace frame_hooks {

enum Point : int { FRAME_END = 0, POINT_COUNT };

using Fn = void (*)(void* user);

bool install();                                         // installs the hooks once (idempotent); false if a site differs
int add(Point point, Fn fn, void* user, const char* owner);   // > 0 id, 0 on failure
void remove(int id);
uint64_t frames();                                      // FRAME_END runs so far

} // namespace frame_hooks
