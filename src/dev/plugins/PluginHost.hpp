#pragma once

#include <string>

namespace utility { class Config; }

// The plugin loader behind the Plugins mod: loads dmc4_hook_plugins\*.dll, gives each the dmc4_api table
// (plugin_sdk\include\dmc4_plugin.h) and runs their callbacks with crash isolation. Plan: Vibed\Plugins\README.md.
namespace plugins {

// startup (Plugins::on_config_load, the game's main thread before WinMain): loads every plugin once
void load_all(const char* folder, const utility::Config& cfg);

// game thread, once per game frame (the frame hook)
void frame();

// the menu: one section per plugin with its status, log and the plugin's own panel
void draw();

// the plugins' settings and the list of plugins switched off
void save_config(utility::Config& cfg);

} // namespace plugins
