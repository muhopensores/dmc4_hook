#include "LongerJealousy.hpp"

bool LongerJealousy::mod_enabled = false;

void LongerJealousy::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x3DDF4B, patch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(0x3DE072, patch2, "\xEB", 1);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

// void on_frame(fmilliseconds& dt) {}

void LongerJealousy::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Longer Jealousy"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Jealousy doesn't stop shooting until you cancel or land"));
}

std::optional<std::string> LongerJealousy::on_initialize() {
    return Mod::on_initialize();
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void LongerJealousy::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("LongerJealousy").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void LongerJealousy::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("LongerJealousy", mod_enabled);
}
