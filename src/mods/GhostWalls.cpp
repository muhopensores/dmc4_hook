#include "GhostWalls.hpp"

bool GhostWalls::mod_enabled = false;

void GhostWalls::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x183E58, patch1, "\x90\x90", 2);
    }
    else {
        patch1.reset();
    }
}

void GhostWalls::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Mephisto/Faust Collision"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Mephisto and Faust get caught on stage collision"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GhostWalls::on_initialize() {
    return Mod::on_initialize();
}

void GhostWalls::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("GhostWalls").value_or(false);
    toggle1(mod_enabled);
}

void GhostWalls::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("GhostWalls", mod_enabled);
}
