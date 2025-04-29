#include "RemoveLaunchArmour.hpp"

bool RemoveLaunchArmour::mod_enabled = false;

std::optional<std::string> RemoveLaunchArmour::on_initialize() {
    return Mod::on_initialize();
}

void RemoveLaunchArmour::toggle(bool enable) {
    if (enable)
    {
        install_patch_offset(0x011C358, patch, "\x90\x90\x90\x7c\x05", 5);
    }
    else
    {
        patch.reset();
    }
}

void RemoveLaunchArmour::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Remove Launch Armour"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Removes the armour enemies gain when downed, allowing you to relaunch for practice instantly"));
}

void RemoveLaunchArmour::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("remove_launch_armour").value_or(false);
    toggle(mod_enabled);
}

void RemoveLaunchArmour::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("remove_launch_armour", mod_enabled);
}
