
#include "NoDtCooldown.hpp"

#if 1
bool NoDtCooldown::mod_enabled{ false };

std::optional<std::string> NoDtCooldown::on_initialize() {
    return Mod::on_initialize();
}

void NoDtCooldown::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x404B84, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else {
        patch.reset();
    }
}

void NoDtCooldown::on_gui_frame() {
    if (ImGui::Checkbox(_("No DT Cooldown"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Disables the cooldown on leaving DT upon entering"));
}

void NoDtCooldown::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("no_dt_cooldown").value_or(false);
    toggle(mod_enabled);
}

void NoDtCooldown::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_dt_cooldown", mod_enabled);
}

#endif
