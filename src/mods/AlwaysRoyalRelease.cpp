#include "AlwaysRoyalRelease.hpp"

#if 1
bool AlwaysRoyalRelease::mod_enabled{false};

std::optional<std::string> AlwaysRoyalRelease::on_initialize() {
    return Mod::on_initialize();
}

void AlwaysRoyalRelease::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3CD47D, patch1, "\xE8\x7E\x2C", 3); // ground
        install_patch_offset(0x3CD49F, patch2, "\xE8\x2C\x34", 3); // air
    } else {
        patch1.reset();  // E8 FE 27 01 00
        patch2.reset(); // E8 2C 2F 01 00
    }
}
void AlwaysRoyalRelease::on_gui_frame() {
    if (ImGui::Checkbox("Always Royal Release", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void AlwaysRoyalRelease::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("always_royal_release").value_or(false);
    toggle(mod_enabled);
}

void AlwaysRoyalRelease::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("always_royal_release", mod_enabled);
}

#endif
