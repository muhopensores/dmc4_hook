#pragma once

namespace debug_server {
void register_tools();   // the built-in tools (Tools.cpp); before start()
void register_queries(); // S1's query tools (Queries.cpp), called by register_tools
void register_actions(); // S2's tools that change the game (Actions.cpp), called by register_tools
void actions_frame();    // game thread, every frame: print_text's texts
}
