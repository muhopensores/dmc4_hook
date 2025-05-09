#include "TricksterDashDT.hpp"

bool TricksterDashDT::mod_enabled = false;

void TricksterDashDT::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x3B744D, patch1, "\xEB", 1);
    }
    else {
        patch1.reset();
    }
}

std::optional<std::string> TricksterDashDT::on_initialize() {
    return Mod::on_initialize();
}

void TricksterDashDT::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Always DT Trickster Dash"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
}

// void on_frame(fmilliseconds& dt) {}

// void on_game_pause(bool toggle) {}

// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void TricksterDashDT::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("TricksterDashDT").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void TricksterDashDT::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("TricksterDashDT", mod_enabled);
}
