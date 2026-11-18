#include "Plugins.hpp"
#include "PluginHost.hpp"
#include "../../sdk/FrameHooks.hpp"
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <cstring>
#include <filesystem>

bool Plugins::mod_enabled = true;

namespace {

const char* const folder = "plugins";

void on_game_frame(void*) { plugins::frame(); }

} // namespace

// The frame hook is the shared one (sdk/FrameHooks, FRAME_END = sDevil4Main::move after checkAttackMain 0x8AF03F):
// the debug server runs its game-thread jobs from the same site.
std::optional<std::string> Plugins::on_initialize() {
    if (!frame_hooks::install() || !frame_hooks::add(frame_hooks::FRAME_END, on_game_frame, nullptr, "Plugins")) {
        spdlog::error("Plugins: no frame hook; plugins' frame callbacks won't run");
        return "Failed to init Plugins (frame hook)";
    }
    return Mod::on_initialize();
}

void Plugins::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("Plugins");
    ImGui::Checkbox(_("Plugin DLLs (Plugins)"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Loads the plugin DLLs in the game folder's plugins folder when the game starts (turning this off "
                  "takes effect at the next start). Plugins are code, like dmc4_hook itself: only use ones you trust. A plugin "
                  "that crashes is stopped and shows where; restart the game after fixing it. Making one: "
                  "plugin_sdk\\include\\dmc4_plugin.h and plugin_sdk\\example in the hook's repo."));
    ImGui::SameLine();
    if (ImGui::Button(_("Open folder"))) {            // made when missing, so a first-time user has somewhere to put a file
        std::error_code ec;
        std::filesystem::create_directories(folder, ec);
        ShellExecuteA(nullptr, "open", std::filesystem::absolute(folder, ec).string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    if (ImGui::TreeNode(_("Plugins"))) {
        plugins::draw();
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Plugins::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("plugins_enabled").value_or(true);
    if (mod_enabled) plugins::load_all(folder, cfg);
}

void Plugins::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("plugins_enabled", mod_enabled);
    plugins::save_config(cfg);
}
