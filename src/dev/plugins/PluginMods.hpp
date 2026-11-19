#pragma once

struct dmc4_mods_api;
struct dmc4_plugin;
namespace utility { class Config; }

// api->mods for plugins (dmc4_plugin.h, version 5): read a mod's settings and hold one for a while. A held setting is
// applied the way the debug server's `mods` tool does it (the mod's on_config_save into a Config, the key changed,
// on_config_load), and the user's value is kept: the config saves it, and release puts it back.
namespace plugins {
const dmc4_mods_api* mods_api();
void release_mods(dmc4_plugin* self);        // every setting the plugin holds (its stop)
void saved_mods(utility::Config& cfg);       // the config being saved: held keys get the user's values
}
