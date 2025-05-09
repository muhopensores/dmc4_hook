#include "DmdLevelAi.hpp"

#if 1
bool DmdLevelAi::mod_enabled = false;

std::optional<std::string> DmdLevelAi::on_initialize() {
    return Mod::on_initialize();
}

// called on enemy spawn, gets enemy difficulty from sMediator+0x134 and applies a different value depending on it
// this change gets xmm0 get written 999.0f instead of 100.0f on BP 1
void DmdLevelAi::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x05D598, patch, "\x74", 1);
    }
    else {
        patch.reset();
    }
}

void DmdLevelAi::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("DMD AI"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Apply DMD AI to enemies without changing their defence or giving them DT.\n"
        "Applied when an enemy spawns"));
}

void DmdLevelAi::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dmd_level_ai").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
}

void DmdLevelAi::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmd_level_ai", mod_enabled);
}

#endif
