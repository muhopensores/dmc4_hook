#include "InfTableHopper.hpp"

bool InfTableHopper::mod_enabled = false;

std::optional<std::string> InfTableHopper::on_initialize() {
    return Mod::on_initialize();
}

void InfTableHopper::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3F8743, patch, "\x90\x90", 2);
    }
    else {
        patch.reset();
    }
}

void InfTableHopper::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Always Table Hopper"), &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfTableHopper::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_table_hopper").value_or(false);
    toggle(mod_enabled);
}

void InfTableHopper::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_table_hopper", mod_enabled);
}
