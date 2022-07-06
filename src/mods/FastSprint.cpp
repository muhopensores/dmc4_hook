
#include "FastSprint.hpp"

#if 1
bool FastSprint::mod_enabled{ false };

std::optional<std::string> FastSprint::on_initialize() {
    return Mod::on_initialize();
}

void FastSprint::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x040456C, patch, "\x77\x33", 2);
    }
    else {
        patch.reset();
    }
}

void FastSprint::on_gui_frame() {
    if (ImGui::Checkbox("Fast Sprint", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void FastSprint::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("fast_sprint").value_or(false);
    toggle(mod_enabled);
}

void FastSprint::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_sprint", mod_enabled);
}

#endif
