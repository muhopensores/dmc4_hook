#pragma once

struct dmc4_players_api;

// api->players for plugins (dmc4_plugin.h, version 3): which player is which in co-op (sdk/Players.hpp) and running
// code in a player's context (dev/coop/CoopContext)
namespace plugins {
const dmc4_players_api* players_api();
}
