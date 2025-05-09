#include "FasterFastDrive.hpp"

bool FasterFastDrive::mod_enabled = false;
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
bool FasterFastDrive::easy_fast_drive_enabled = false;

std::optional<std::string> FasterFastDrive::on_initialize() {
    return Mod::on_initialize();
}

void FasterFastDrive::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3D0DFB, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch.reset();
    }
}

void FasterFastDrive::on_gui_frame(int display) {
    if (display == 1) {
        if (ImGui::Checkbox(_("Fast Quick Drive"), &mod_enabled)) {
            toggle(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Quick Drive skips the initial slash"));
    }
    if (display == 2) {
        ImGui::Checkbox(_("Easy Quick Drive"), &easy_fast_drive_enabled);
        ImGui::SameLine();
        help_marker(_("Prop is cancellable into quick drive until frame 7"));
    }
}

void FasterFastDrive::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("faster_fast_drive").value_or(false);
    easy_fast_drive_enabled = cfg.get<bool>("easy_fast_drive").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
};

void FasterFastDrive::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("faster_fast_drive", mod_enabled);
    cfg.set<bool>("easy_fast_drive", easy_fast_drive_enabled);
};
