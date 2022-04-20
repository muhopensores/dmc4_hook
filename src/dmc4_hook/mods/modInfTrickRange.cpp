#include "../mods.h"
#include "modInfTrickRange.hpp"

bool InfTrickRange::modEnabled{ false };

std::optional<std::string> InfTrickRange::onInitialize() {
	return Mod::onInitialize();
}

void InfTrickRange::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x003CB0A8, patch, "\x90\x90\x90", 3);
    }
    else {
        patch.revert();
    }
}

void InfTrickRange::onGUIframe() {
    if (ImGui::Checkbox("Infinite Trick Range", &modEnabled)) {
        toggle(modEnabled);
    }
}

void InfTrickRange::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("infinite_trick_range").value_or(false);
    toggle(modEnabled);
}

void InfTrickRange::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("infinite_trick_range", modEnabled);
}
