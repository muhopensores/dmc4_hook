#include "modInfRevive.hpp"

bool InfRevive::modEnabled{ false };

std::optional<std::string> InfRevive::onInitialize() {
    return Mod::onInitialize();
}

void InfRevive::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x039FA7C, patch1, "\x88\x9e\xf6\x00\x00\x00", 6);
        install_patch_offset(0x000D270, patch2, "\x72\x0d", 2);
        install_patch_offset(0x000D49B, patch3, "\x83\x80\x3c\x01\x00\x00\x00", 7);
        install_patch_offset(0x000D25A, patch4, "\x76\x0d", 2);
    }
    else {
        patch1.revert();
        patch2.revert();
        patch3.revert();
        patch4.revert();
    }
}

void InfRevive::onGUIframe() {
    if (ImGui::Checkbox("Infinite Revives", &modEnabled)) {
        toggle(modEnabled);
    }
}

void InfRevive::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("infinite_revive").value_or(false);
    toggle(modEnabled);
}

void InfRevive::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("infinite_revive", modEnabled);
}
