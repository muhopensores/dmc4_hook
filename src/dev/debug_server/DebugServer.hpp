#pragma once

#include "../../mod.hpp"

// DebugServer: the game's MCP / debug server (plan Vibed\GameMCP\README.md). Off by default; when on, it listens on
// http://127.0.0.1:<port>/mcp (JSON-RPC 2.0; MCP's tools/list and tools/call). Its game-thread tools run from the
// shared frame hook (sdk/FrameHooks).
class DebugServer : public Mod {
public:
    DebugServer() = default;
    std::string get_mod_name() override { return "DebugServer"; };
    std::vector<std::string> get_search_terms() override { return {"debug server", "mcp", "server", "debug", "api"}; }

    static bool mod_enabled;
    static int server_port;
    static bool dev_tools_at_start;   // cfg debug_server_dev_tools: the Dev tools switch starts on (test runs)

    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
