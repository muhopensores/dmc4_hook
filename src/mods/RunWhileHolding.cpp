#include "RunWhileHolding.hpp"

bool RunWhileHolding::mod_enabled = false;

void RunWhileHolding::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x3E3E6E, patch1, "\xEB", 1);
    }
    else {
        patch1.reset();
    }
}

// void on_frame(fmilliseconds& dt) {}

void RunWhileHolding::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Run While Holding"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    // ImGui::SameLine();
    // help_marker(_("RunWhileHoldingHelpMarker"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> RunWhileHolding::on_initialize() {
    return Mod::on_initialize();
}

void RunWhileHolding::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("RunWhileHolding").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void RunWhileHolding::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("RunWhileHolding", mod_enabled);
}
