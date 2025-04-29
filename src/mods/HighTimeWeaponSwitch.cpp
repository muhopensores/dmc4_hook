#include "HighTimeWeaponSwitch.hpp"

#if 1
bool HighTimeWeaponSwitch::mod_enabled = false;

std::optional<std::string> HighTimeWeaponSwitch::on_initialize() {
    return Mod::on_initialize();
}

void HighTimeWeaponSwitch::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3CEEE3, patch, "\xEB\x08", 2); // jmp DevilMayCry4_DX9.exe+3CEEED
    } else {
        patch.reset(); // je DevilMayCry4_DX9.exe+3CEEED
    }
}
void HighTimeWeaponSwitch::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("High Time Weapon Switch"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Changing weapons early while holding High Time will still allow you to do the held variant"));
}

void HighTimeWeaponSwitch::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("high_time_weapon_switch").value_or(false);
    toggle(mod_enabled);
}

void HighTimeWeaponSwitch::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("high_time_weapon_switch", mod_enabled);
}

#endif
