#include "InfTrickRange.hpp"

bool InfTrickRange::mod_enabled = false;

std::optional<std::string> InfTrickRange::on_initialize() {
	return Mod::on_initialize();
}

void InfTrickRange::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x003CB0A8, patch, "\x90\x90\x90", 3);
    }
    else {
        patch.reset();
    }
}

void InfTrickRange::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Trick Range"), &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfTrickRange::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_trick_range").value_or(false);
    toggle(mod_enabled);
}

void InfTrickRange::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_trick_range", mod_enabled);
}
