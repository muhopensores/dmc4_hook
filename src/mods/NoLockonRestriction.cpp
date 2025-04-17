#include "NoLockonRestriction.hpp"

bool NoLockonRestriction::mod_enabled { false };

void NoLockonRestriction::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x510C0, patch1, "\x90\x90", 2);
    } else {
        patch1.reset();
    }
}

void NoLockonRestriction::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Remove lock-on restriction"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Remove Dante's lock-on restrictions (grim grip, gyro, etc.)"));
}

void NoLockonRestriction::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("no_lockon_restriction").value_or(false);
};

void NoLockonRestriction::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_lockon_restriction", mod_enabled);
};