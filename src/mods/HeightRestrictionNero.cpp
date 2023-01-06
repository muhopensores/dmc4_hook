#include "HeightRestrictionNero.hpp"

#if 1
bool HeightRestrictionNero::mod_enabled{ false };

std::optional<std::string> HeightRestrictionNero::on_initialize() {
    return Mod::on_initialize();
}

void HeightRestrictionNero::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3E614B, patch_buster, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5F8D, patch_split, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E6248, patch_calibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E62B6, patch_ex_calibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E60E4, patch_snatch, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E603F, patch_rave, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5FE1, patch_double_down, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E4B12, patch_rev, "\x90\x90", 2);
    }
    else {
        patch_buster.reset();
        patch_split.reset();
        patch_calibur.reset();
        patch_ex_calibur.reset();
        patch_snatch.reset();
        patch_rave.reset();
        patch_double_down.reset();
        patch_rev.reset();
    }
}

void HeightRestrictionNero::on_gui_frame() {
    if (ImGui::Checkbox("Nero Height Restriction", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void HeightRestrictionNero::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("nero_height_restriction_removed").value_or(false);
    toggle(mod_enabled);
}

void HeightRestrictionNero::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_height_restriction_removed", mod_enabled);
}

#endif
