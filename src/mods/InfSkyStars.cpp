
#include "InfSkyStars.hpp"

bool InfSkyStars::mod_enabled{ false };

std::optional<std::string> InfSkyStars::on_initialize() {
    return Mod::on_initialize();
}

void InfSkyStars::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3B74AC, patch, "\x90\x90\x90", 3);
    }
    else {
        patch.reset();
    }
}

void InfSkyStars::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Sky Stars"), &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfSkyStars::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_sky_stars").value_or(false);
    toggle(mod_enabled);
};

void InfSkyStars::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_sky_stars", mod_enabled);
};
