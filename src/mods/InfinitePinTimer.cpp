#include "InfinitePinTimer.hpp"

bool InfinitePinTimer::mod_enabled = false;

void InfinitePinTimer::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x04135F6, patch1, "\xEB\x14", 2); // not in enemy
        install_patch_offset(0x041362C, patch2, "\xEB\x12", 2); // in enemy
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

std::optional<std::string> InfinitePinTimer::on_initialize() {
	return Mod::on_initialize();
}

void InfinitePinTimer::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Pin Timer"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
}

void InfinitePinTimer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_pin_timer").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void InfinitePinTimer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_pin_timer", mod_enabled);
}
