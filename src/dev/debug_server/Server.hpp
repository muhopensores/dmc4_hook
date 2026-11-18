#pragma once

#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// The game's MCP / debug server (plan Vibed/GameMCP/README.md): HTTP on 127.0.0.1, POST /mcp, JSON-RPC 2.0. MCP
// clients use initialize / tools/list / tools/call; other tools may call a tool by its name as the method. One tool
// registry; a tool runs on the server thread or, queued, on the game thread (the shared frame hook, sdk/FrameHooks).
namespace debug_server {

using json = nlohmann::json;

struct Tool {
    std::string name;
    std::string description;
    json schema = json{{"type", "object"}, {"properties", json::object()}};   // MCP inputSchema
    bool game_thread = false;                    // run from the frame hook (game state), else on the server thread
    std::function<json(const json& args)> run;   // throw std::runtime_error for a tool error
};

void add_tool(Tool tool);
std::vector<std::string> tool_names();

bool start(int port, std::string& error);       // localhost only
void stop();
bool running();
int port();

void install_log_sink();                        // keeps the hook's log lines for the `log` tool (call once)
void on_game_frame();                           // drains the game-thread jobs (registered on FRAME_END)

struct CallRecord {
    std::string tool;
    bool ok;
    double ms;
};
std::vector<CallRecord> recent_calls();         // the last calls, newest last (for the panel)

// The `log` tool's reads: lines with seq > since (since < 0: the last -since lines), at most max.
json log_lines(long long since, int max);

} // namespace debug_server
