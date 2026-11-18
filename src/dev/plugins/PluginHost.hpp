#pragma once

#include <string>

namespace utility { class Config; }
struct dmc4_plugin;
typedef void (*dmc4_callback)(void* user);

// The plugin loader behind the Plugins mod: loads plugins\*.dll, gives each the dmc4_api table
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

// For api functions that call back into a plugin (players->run_as): fn(user) with crash isolation. A crash stops `self`
// (its callbacks go, the reason is logged with `what`) and returns false.
bool run_guarded(dmc4_plugin* self, dmc4_callback fn, void* user, const char* what);

// For the api tables in other files: a line in the plugin's log (as api->log), and its DLL's file name.
void plugin_log(dmc4_plugin* self, int level, const std::string& text);
std::string plugin_file(const dmc4_plugin* self);

} // namespace plugins
