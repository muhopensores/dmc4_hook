// include your mod header file
#include "HeightRestrictionDante.hpp"

#if 1
// static variable defined in cpp file makes it local to current file
bool HeightRestrictionDante::mod_enabled{ false };

// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> HeightRestrictionDante::on_initialize() {
	return Mod::on_initialize();
}

void HeightRestrictionDante::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3B764E, patch, "\x0F\x84\x7B\x03\x00\x00", 6);
    }
    else {
        patch.reset();
    }
}

void HeightRestrictionDante::on_gui_frame() {
    if (ImGui::Checkbox("Remove Dante's Height Restriction", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void HeightRestrictionDante::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dante_height_restriction_removed").value_or(false);
    toggle(mod_enabled);
}

void HeightRestrictionDante::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_height_restriction_removed", mod_enabled);
}

#endif
