// include your mod header file
#include "HeightRestrictionDante.hpp"

#if 1
// static variable defined in cpp file makes it local to current file
bool HeightRestrictionDante::mod_enabled{ false };
bool HeightRestrictionDante::mod_airhike{ false };

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

void HeightRestrictionDante::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x4038F8, patch_airhike, "\x90\x90\x90\x90\x90\x90", 6);
    } else {
        patch_airhike.reset();
    }
}

void HeightRestrictionDante::on_gui_frame() {
    if (ImGui::Checkbox("Dante Height Restriction", &mod_enabled)) {
        toggle(mod_enabled);
    }
    if (ImGui::Checkbox("Air Hike Height Restriction", &mod_airhike)) {
        toggle2(mod_airhike);
    }
}

void HeightRestrictionDante::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dante_height_restriction_removed").value_or(false);
    toggle(mod_enabled);
    mod_airhike = cfg.get<bool>("airhike_height_restriction").value_or(false);
    toggle2(mod_airhike);
}

void HeightRestrictionDante::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_height_restriction_removed", mod_enabled);
    cfg.set<bool>("airhike_height_restriction", mod_airhike);
}

#endif
