#include "Plugins.hpp"
#include "PluginHost.hpp"
#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <cstring>
#include <filesystem>

bool Plugins::mod_enabled = true;

namespace {

constexpr uintptr_t kFrameSite = 0x8AF03F;   // sDevil4Main::move, after call sCollisionGame::checkAttackMain
constexpr uint8_t kFrameBytes[6] = {0x8B, 0x8D, 0x64, 0x03, 0x01, 0x00};   // mov ecx, [ebp+0x10364]
uintptr_t jmp_ret_frame = 0;
const char* const folder = "dmc4_hook_plugins";

void __stdcall on_game_frame() { plugins::frame(); }

naked void detour_frame() {
    _asm {
        pushad
        call on_game_frame
        popad
        mov ecx, [ebp+0x10364]
        jmp dword ptr [jmp_ret_frame]
    }
}

} // namespace

std::optional<std::string> Plugins::on_initialize() {
    if (std::memcmp(reinterpret_cast<const void*>(kFrameSite), kFrameBytes, sizeof kFrameBytes) != 0) {
        spdlog::error("Plugins: unexpected code at 0x{:X}; plugins' frame callbacks won't run", kFrameSite);
        return "Failed to init Plugins (frame hook site)";
    }
    if (!install_hook_absolute(kFrameSite, hook_frame, &detour_frame, &jmp_ret_frame, 6)) {
        spdlog::error("Plugins: failed to hook sDevil4Main::move (0x{:X})", kFrameSite);
        return "Failed to init Plugins (frame hook)";
    }
    return Mod::on_initialize();
}

void Plugins::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("Plugins");
    ImGui::Checkbox(_("Plugin DLLs (Plugins)"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Loads the plugin DLLs in the game folder's dmc4_hook_plugins folder when the game starts (turning this off "
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
