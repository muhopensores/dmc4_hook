#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

namespace debug_server {
void register_tools();   // the built-in tools (Tools.cpp); before start()
void register_queries(); // S1's query tools (Queries.cpp), called by register_tools
void register_actions(); // S2's tools that change the game (Actions.cpp), called by register_tools
void actions_frame();    // game thread, every frame: print_text's texts
void register_input();   // S3's input / pause / step / wait (Input.cpp), called by register_tools; hooks sDevil4Pad::move
void input_frame();      // game thread, every frame before the jobs: counts step frames down
nlohmann::json input_status();   // queued input frames, paused, stepping (for status)
void register_sdl();     // S5's Albam link v1: sdl_list / sdl_layout / sdl_patch / sdl_clock (Sdl.cpp); hooks uScheduler::move
void sdl_frame();        // game thread, every frame first: bumps load_gen on a room / area change
uint32_t load_gen();     // changes whenever the room or area layer changes (tools.md)
}
