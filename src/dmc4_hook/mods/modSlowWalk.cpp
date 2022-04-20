#include "modSlowWalk.hpp"

bool SlowWalk::modEnabled{ false };

std::optional<std::string> SlowWalk::onInitialize()
{
    return Mod::onInitialize();
}

void SlowWalk::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x0421C83, patch1, "\x0c\x14\x00\x00\x04\x0f\x83", 7);
        install_patch_offset(0x0421D85, patch2, "\x0c\x14\x00\x00\x39\xc8", 6);
    }
    else {
        patch1.revert();
        patch2.revert();
    }
}

void SlowWalk::onGUIframe() {
    if (ImGui::Checkbox("Slow Walk", &modEnabled)) {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Walk slowly while holding the jump button");
}

void SlowWalk::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("slow_walk").value_or(false);
    toggle(modEnabled);
}

void SlowWalk::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("slow_walk", modEnabled);
}
