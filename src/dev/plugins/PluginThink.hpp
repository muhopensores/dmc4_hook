#pragma once

#include "../../../plugin_sdk/include/dmc4_plugin.h"

// api->game->on_think (dmc4_plugin.h, version 7): plugins see every enemy decision (EnemyAI's cThinkMgr::GetActionNo
// hook, its enemy_think filter) and may change the answer. Callbacks run with crash isolation, in the order added.
namespace plugins {
int __cdecl think_add(dmc4_plugin* self, dmc4_think_fn fn, void* user);
void __cdecl think_remove(dmc4_plugin* self, int id);
void release_think(dmc4_plugin* self);   // its stop: its callbacks go
}
