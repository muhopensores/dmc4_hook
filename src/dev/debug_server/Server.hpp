#pragma once

#include <functional>
#include <stdexcept>
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
    bool dev = false;                            // changes the game: refused unless the panel's Dev tools switch is on
};

// A tool error with a stable code for clients (tools.md "Errors"): plain calls get error.data = {code, ...data}.
// Other exceptions are code "failed".
struct ToolError : std::runtime_error {
    ToolError(std::string code, const std::string& message, json data = json::object())
        : std::runtime_error(message), code(std::move(code)), data(std::move(data)) {}
    std::string code;
    json data;
};

void add_tool(Tool tool);
std::vector<std::string> tool_names();

// The panel's "Dev tools" switch (plan D5): off at every start, never saved. Tools marked `dev` need it.
bool dev_tools();
void set_dev_tools(bool on);

// For server-thread tools: runs fn on the game thread (the next frame hook) and returns its result; throws if the game
// doesn't take it within 3 s.
json run_on_game_thread(std::function<json()> fn);

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
