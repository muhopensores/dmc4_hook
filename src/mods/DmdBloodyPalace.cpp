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
    if (ImGui::Checkbox(_("BP DMD HP + DT"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Up enemy defence to DMD levels and enable enemy DT without adding DMD AI.\n"
        "Enemies will take longer to DT than with the SpoilerAL difficulty.\n"
        "Applied when starting BP"));
}

void DmdBloodyPalace::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dmd_bloody_palace").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
}

void DmdBloodyPalace::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmd_bloody_palace", mod_enabled);
}

#endif
