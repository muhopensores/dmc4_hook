#pragma once

namespace debug_server {
void register_tools();   // the built-in tools (Tools.cpp); before start()
void register_queries(); // S1's query tools (Queries.cpp), called by register_tools
}
