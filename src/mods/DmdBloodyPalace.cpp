#include "DmdBloodyPalace.hpp"

#if 1
bool DmdBloodyPalace::mod_enabled = false;

std::optional<std::string> DmdBloodyPalace::on_initialize() {
    return Mod::on_initialize();
}

void DmdBloodyPalace::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x008FF7, patch, "\x03", 1);
    }
    else {
        patch.reset();
    }
}

void DmdBloodyPalace::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("DMD Bloody Palace"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Forces the ingame version of DMD. Enemies will take longer to DT than with the 'Game Mode' option"));
}

void DmdBloodyPalace::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dmd_bloody_palace").value_or(false);
    toggle(mod_enabled);
}

void DmdBloodyPalace::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmd_bloody_palace", mod_enabled);
}

#endif
