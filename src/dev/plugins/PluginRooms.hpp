#pragma once

struct dmc4_rooms_api;

// api->rooms for plugins (dmc4_plugin.h, version 4): room classes derived from the game's, over sdk/RoomClasses (plan
// Vibed\Rooms\README.md R3)
namespace plugins {
const dmc4_rooms_api* rooms_api();
}
