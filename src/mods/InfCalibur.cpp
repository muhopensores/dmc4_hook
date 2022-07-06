
#include "InfCalibur.hpp"

bool InfCalibur::mod_enabled{ false };

std::optional<std::string> InfCalibur::on_initialize() {
    return Mod::on_initialize();
}

void InfCalibur::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03FDE98, patch, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x04003AB, patch2, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch.reset();
        patch2.reset();
    }
}

void InfCalibur::on_gui_frame() {
    if (ImGui::Checkbox("Infinite Calibur", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfCalibur::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_calibur").value_or(false);
    toggle(mod_enabled);
}

void InfCalibur::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_calibur", mod_enabled);
}
