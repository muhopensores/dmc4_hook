#pragma once

struct dmc4_menus_api;
struct dmc4_plugin;

// api->menus for plugins (dmc4_plugin.h, version 5): a pause-menu row opening a list menu (sdk/PauseMenu), the plugin's
// callbacks run with crash isolation.
namespace plugins {
const dmc4_menus_api* menus_api();
void release_menus(dmc4_plugin* self);   // its stop: the row goes
}
