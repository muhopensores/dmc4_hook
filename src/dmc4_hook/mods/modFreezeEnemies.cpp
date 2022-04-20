#include "../mods.h"
#include "modFreezeEnemies.hpp"

#if 1
bool FreezeEnemies::modEnabled{ false };

std::optional<std::string> FreezeEnemies::onInitialize() {
    return Mod::onInitialize();
}

void FreezeEnemies::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x14256D, scarecrowPatch, "\x90\x90\x90", 3);
        install_patch_offset(0x1A5AAD, frostPatch, "\xEB", 1);
        install_patch_offset(0x1649B6, armourPatch, "\xE9\xA6\x05\x00\x00\x90", 6);
        install_patch_offset(0x18311C, ghostPatch, "\x90\x90\x90", 3);
        install_patch_offset(0x20DA28, flyingSwordPatch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1F8A6D, fishSwordPatch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1DE9C0, seedPatch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x1B668D, assaultPatch, "\xE9\x45\x03\x00\x00\x90", 6);
        install_patch_offset(0x21CE8D, dogPatch, "\x90\x90\x90", 3);
        install_patch_offset(0x1D3268, blitzPatch, "\x90\x90\x90\x90", 4);
    }
    else {
        scarecrowPatch.revert();
        frostPatch.revert();
        armourPatch.revert();
        ghostPatch.revert();
        flyingSwordPatch.revert();
        fishSwordPatch.revert();
        seedPatch.revert();
        assaultPatch.revert();
        dogPatch.revert();
        blitzPatch.revert();
    }
}

void FreezeEnemies::onGUIframe() {
    if (ImGui::Checkbox("Freeze Enemies", &modEnabled)) {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Freezes enemies, even if they're midair. Best used with infinite enemy hp");
}

void FreezeEnemies::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("freeze_enemies").value_or(false);
    toggle(modEnabled);
}

void FreezeEnemies::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("freeze_enemies", modEnabled);
}

#endif
