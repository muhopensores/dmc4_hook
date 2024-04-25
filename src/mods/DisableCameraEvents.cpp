#include "DisableCameraEvents.hpp"

bool DisableCameraEvents::mod_enabled{ false };

std::optional<std::string> DisableCameraEvents::on_initialize() {
    return Mod::on_initialize();
}

void DisableCameraEvents::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x000BEF1, patch1, "\x31\xC0\xB8\x06\x00\x00\x00", 7);
        install_patch_offset(0x000C6C1, patch2, "\xEB\x17", 2);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

void DisableCameraEvents::on_gui_frame() {
    if (ImGui::Checkbox(_("Disable Camera Events"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Certain missions or parts will cause a black screen when this option is enabled. "
               "Press the pause button then confirm to go into the item menu and the screen will return to normal."));
}

void DisableCameraEvents::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_camera_events").value_or(false);
    toggle(mod_enabled);
}

void DisableCameraEvents::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_camera_events", mod_enabled);
}
