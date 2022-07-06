
#include "DmdLevelAi.hpp"

#if 1
bool DmdLevelAi::mod_enabled{ false };

std::optional<std::string> DmdLevelAi::on_initialize() {
    return Mod::on_initialize();
}

void DmdLevelAi::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x05D598, patch, "\x74", 1);
    }
    else {
        patch.reset();
    }
}

void DmdLevelAi::on_gui_frame() {
    if (ImGui::Checkbox("DMD Level AI", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Forces enemies to use the ingame DMD AI, even on non DMD difficulties");
}

void DmdLevelAi::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dmd_level_ai").value_or(false);
    toggle(mod_enabled);
}

void DmdLevelAi::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmd_level_ai", mod_enabled);
}

#endif
