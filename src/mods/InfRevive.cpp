#include "InfRevive.hpp"

bool InfRevive::mod_enabled{ false };

std::optional<std::string> InfRevive::on_initialize() {
    return Mod::on_initialize();
}

void InfRevive::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x039FA7C, patch1, "\x88\x9e\xf6\x00\x00\x00", 6);
        install_patch_offset(0x000D270, patch2, "\x72\x0d", 2);
        install_patch_offset(0x000D49B, patch3, "\x83\x80\x3c\x01\x00\x00\x00", 7);
        install_patch_offset(0x000D25A, patch4, "\x76\x0d", 2);
    }
    else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
    }
}

void InfRevive::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Revives"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Revive on the spot whenever you die for free"));
}

void InfRevive::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_revive").value_or(false);
    toggle(mod_enabled);
}

void InfRevive::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_revive", mod_enabled);
}
