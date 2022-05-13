#include "../mods.h"
#include "modDtEnemiesDontStun.hpp"

#if 1
bool DtEnemiesDontStun::modEnabled{ false };

std::optional<std::string> DtEnemiesDontStun::onInitialize() {
    return Mod::onInitialize();
}

void DtEnemiesDontStun::toggle(bool enable){
    if (enable){
        install_patch_offset(0x11B88E, patch, "\x90\x90", 2);
    }
    else {
        patch.revert();
    }
}

void DtEnemiesDontStun::onGUIframe() {
    if (ImGui::Checkbox("DT Enemies Don't Stun", &modEnabled)) {
        toggle(modEnabled);
    }
}

void DtEnemiesDontStun::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("dt_enemies_dont_stun").value_or(false);
    toggle(modEnabled);
}

void DtEnemiesDontStun::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("dt_enemies_dont_stun", modEnabled);
}

#endif
