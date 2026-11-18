#include "DebugServer.hpp"
#include "Server.hpp"
#include "Tools.hpp"
#include "../../sdk/FrameHooks.hpp"
#include <spdlog/spdlog.h>

bool DebugServer::mod_enabled = false;
int DebugServer::server_port = 13350;
bool DebugServer::dev_tools_at_start = false;

namespace {

std::string g_error;

void drain_jobs(void*) {
    debug_server::sdl_frame();       // load_gen first, so this frame's jobs see the new one
    debug_server::input_frame();     // a step a job starts this frame runs from the next one
    debug_server::on_game_frame();
    debug_server::actions_frame();
}

void apply(bool on) {
    if (on) {
        if (!debug_server::start(DebugServer::server_port, g_error)) {
            spdlog::error("DebugServer: {}", g_error);
            DebugServer::mod_enabled = false;
        } else {
            g_error.clear();
        }
    } else {
        debug_server::stop();
    }
}

} // namespace

std::optional<std::string> DebugServer::on_initialize() {
    debug_server::install_log_sink();
    debug_server::register_tools();
    if (!frame_hooks::install() || !frame_hooks::add(frame_hooks::FRAME_END, drain_jobs, nullptr, "DebugServer"))
        spdlog::error("DebugServer: no frame hook; game-thread tools will time out");
    return Mod::on_initialize();
}

void DebugServer::on_frame(fmilliseconds&) { debug_server::screenshot_present(); }

void DebugServer::on_config_load(const utility::Config& cfg) {
    server_port = cfg.get<int>("debug_server_port").value_or(13350);
    mod_enabled = cfg.get<bool>("debug_server_enabled").value_or(false);
    dev_tools_at_start = cfg.get<bool>("debug_server_dev_tools").value_or(false);
    debug_server::set_dev_tools(dev_tools_at_start);
    if (mod_enabled) apply(true);
}

void DebugServer::on_config_save(utility::Config& cfg) {
    cfg.set<int>("debug_server_port", server_port);
    cfg.set<bool>("debug_server_enabled", mod_enabled);
    cfg.set<bool>("debug_server_dev_tools", dev_tools_at_start);   // the cfg's choice, not this session's checkbox
}

void DebugServer::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("DebugServer");
    if (ImGui::Checkbox(_("Debug server (DebugServer)"), &mod_enabled)) apply(mod_enabled);
    ImGui::SameLine();
    help_marker(_("Lets tools talk to the running game: Claude through MCP, Albam and scripts through the same calls. "
                  "Listens on this PC only (127.0.0.1), at the port below, path /mcp. Off by default."));
    if (ImGui::TreeNode(_("Debug server details"))) {
        ImGui::SetNextItemWidth(sameLineItemWidth);
        ImGui::InputInt(_("Port"), &server_port);
        ImGui::SameLine();
        help_marker(_("Takes effect the next time the server starts (switch it off and on)."));
        if (debug_server::running())
            ImGui::Text(_("Listening on http://127.0.0.1:%d/mcp"), debug_server::port());
        else if (!g_error.empty())
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", g_error.c_str());
        else
            ImGui::TextDisabled("%s", _("Not running"));
        bool dev = debug_server::dev_tools();
        if (ImGui::Checkbox(_("Dev tools"), &dev)) debug_server::set_dev_tools(dev);
        ImGui::SameLine();
        help_marker(_("Lets the server's tools change the game: write memory, call functions, enter rooms, show text, "
                      "spawn and delete units, change mod settings. For this session only; it starts on only when "
                      "dmc4_hook.cfg has debug_server_dev_tools = true (for test runs)."));
        for (const auto& c : debug_server::recent_calls())
            ImGui::BulletText("%s %s %.1f ms", c.tool.c_str(), c.ok ? "ok" : "error", c.ms);
        ImGui::TreePop();
    }
    ImGui::PopID();
}
