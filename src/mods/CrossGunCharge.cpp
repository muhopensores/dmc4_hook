#include "CrossGunCharge.hpp"

bool CrossGunCharge::mod_enabled = false;
uintptr_t CrossGunCharge::jmp_ret1 = NULL;

void CrossGunCharge::toggle1(bool enable) {
    if (enable) {
        // Don't kill charge when changing weapons
        install_patch_offset(0x3B87AA, patch1, "\xEB\x47\x0F\x1F", 4); // replaces call with a jump to near end of func
    }
    else {
        patch1.reset();
    }
}

std::optional<std::string> CrossGunCharge::on_initialize() {
    return Mod::on_initialize();
}

void CrossGunCharge::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Cross Gun Charge"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Changing gun doesn't empty your charge"));
}

// void on_frame(fmilliseconds& dt) {}

// void on_game_pause(bool toggle) {}

// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void CrossGunCharge::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("CrossGunCharge").value_or(false);
    toggle1(mod_enabled);
}

void CrossGunCharge::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("CrossGunCharge", mod_enabled);
}
