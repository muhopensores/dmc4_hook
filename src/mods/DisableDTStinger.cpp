#include "DisableDTStinger.hpp"

#if 1
bool DisableDTStinger::mod_enabled = false;

std::optional<std::string> DisableDTStinger::on_initialize() {
    return Mod::on_initialize();
}

void DisableDTStinger::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3CD20A, patch, "\xE8\x61\x12\x00\x00", 5);
    } else {
        patch.reset();
    }
}
void DisableDTStinger::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Disable DT Stinger"), &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void DisableDTStinger::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_dt_stinger").value_or(false);
    toggle(mod_enabled);
}

void DisableDTStinger::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_dt_stinger", mod_enabled);
}

#endif
