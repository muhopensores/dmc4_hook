
#include "FreezeEnemies.hpp"

#if 1
bool FreezeEnemies::mod_enabled{ false };

std::optional<std::string> FreezeEnemies::on_initialize() {
    console->system().RegisterCommand("freeze", "Freeze enemies in their current position", [this]() {
        mod_enabled = !mod_enabled;
        toggle(mod_enabled);
    });

    return Mod::on_initialize();
}

void FreezeEnemies::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x14256D, scarecrow_patch, "\x90\x90\x90", 3);
        install_patch_offset(0x1A5AAD, frost_patch, "\xEB", 1);
        install_patch_offset(0x1649B6, armour_patch, "\xE9\xA6\x05\x00\x00\x90", 6);
        install_patch_offset(0x18311C, ghost_patch, "\x90\x90\x90", 3);
        install_patch_offset(0x20DA28, flying_sword_patch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1F8A6D, fish_sword_patch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1DE9C0, seed_patch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1B668D, assault_patch, "\xE9\x45\x03\x00\x00\x90", 6);
        install_patch_offset(0x21CE8D, dog_patch, "\x90\x90\x90", 3);
        install_patch_offset(0x1D3268, blitz_patch, "\xEB\x21", 2);
    }
    else {
        scarecrow_patch.reset();
        frost_patch.reset();
        armour_patch.reset();
        ghost_patch.reset();
        flying_sword_patch.reset();
        fish_sword_patch.reset();
        seed_patch.reset();
        assault_patch.reset();
        dog_patch.reset();
        blitz_patch.reset();
    }
}

void FreezeEnemies::on_gui_frame() {
    if (ImGui::Checkbox(_("Freeze Enemies"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Freezes enemies, even if they're midair. Best used with infinite enemy hp"));
}

void FreezeEnemies::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("freeze_enemies").value_or(false);
    toggle(mod_enabled);
}

void FreezeEnemies::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("freeze_enemies", mod_enabled);
}

#endif
