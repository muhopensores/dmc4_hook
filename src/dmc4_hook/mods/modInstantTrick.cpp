#include "../mods.h"
#include "modInstantTrick.hpp"

#if 1
bool InstantTrick::modEnabled{ false };

std::optional<std::string> InstantTrick::onInitialize() {
    return Mod::onInitialize();
}

void InstantTrick::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03CB005, patch, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
    }
    else {
        patch.revert();
    }
}

void InstantTrick::onGUIframe()
{
    if (ImGui::Checkbox("Instant Trick", &modEnabled)) {
        toggle(modEnabled);
    }
}

void InstantTrick::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("instant_trick").value_or(false);
    toggle(modEnabled);
};

void InstantTrick::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("instant_trick", modEnabled);
};

#endif
