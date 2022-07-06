#include "InfTableHopper.hpp"

bool InfTableHopper::mod_enabled{ false };

std::optional<std::string> InfTableHopper::on_initialize() {
    return Mod::on_initialize();
}

void InfTableHopper::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03F873C, patch, "\x80\xbe\x14\xce\x00\x00\x00", 7);
    }
    else {
        patch.reset();
    }
}

void InfTableHopper::on_gui_frame() {
    if (ImGui::Checkbox("Infinite Table Hopper", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfTableHopper::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_table_hopper").value_or(false);
    toggle(mod_enabled);
}

void InfTableHopper::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("inf_table_hopper", mod_enabled);
}
