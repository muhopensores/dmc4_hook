#pragma once

struct dmc4_game_api;

// api->game for plugins (dmc4_plugin.h, version 2): plain C wrappers over devil4_sdk (sdk/Devil4.hpp)
namespace plugins {
const dmc4_game_api* game_api();
}
