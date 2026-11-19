#pragma once

struct dmc4_units_api;
struct dmc4_plugin;

// api->units for plugins (dmc4_plugin.h, version 6): stage units from any room's files (UnitPacks' rooms manifest,
// dev/unit_packs): find, load their files, spawn them through the game's uPlacement, despawn.
namespace plugins {
const dmc4_units_api* units_api();
void units_frame();                      // game thread, every frame before the plugins' callbacks: spawns resolve
void release_units(dmc4_plugin* self);   // its stop: its spawns despawned, its loads released
}
