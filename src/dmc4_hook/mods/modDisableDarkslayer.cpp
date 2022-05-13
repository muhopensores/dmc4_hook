#include "modDisableDarkslayer.hpp"

bool DisableDarkslayer::modEnabledUp{ false };
bool DisableDarkslayer::modEnabledDown{ false };
bool DisableDarkslayer::modEnabledLeft{ false };
bool DisableDarkslayer::modEnabledRight{ false };

std::optional<std::string> DisableDarkslayer::onInitialize() {
    return Mod::onInitialize();
}

void DisableDarkslayer::toggleUp(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6C84, patch1, "\x90\x90\x90", 3);
    }
    else {
        patch1.revert();
    }
}
void DisableDarkslayer::toggleDown(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6CE3, patch2, "\x90\x90\x90", 3);
    }
    else {
        patch2.revert();
    }
}
void DisableDarkslayer::toggleLeft(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6D42, patch3, "\x90\x90\x90", 3);
    }
    else {
        patch3.revert();
    }
}
void DisableDarkslayer::toggleRight(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6D99, patch4, "\x90\x90\x90", 3);
    }
    else {
        patch4.revert();
    }
}

void DisableDarkslayer::onGUIframe() {
    ImGui::Text("Disable Darkslayer Inputs");

    ImGui::Spacing();
    ImGui::Text("                         ");
    ImGui::SameLine();
    if (ImGui::Checkbox("Trickster", &modEnabledUp)) {
        toggleUp(modEnabledUp);
    }

    ImGui::Text("                  ");
    ImGui::SameLine();
    if (ImGui::Checkbox("Gunslinger", &modEnabledLeft)) {
        toggleLeft(modEnabledLeft);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Sword Master", &modEnabledRight)) {
        toggleRight(modEnabledRight);
    }

    ImGui::Text("                         ");
    ImGui::SameLine();
    if (ImGui::Checkbox("Royal Guard", &modEnabledDown)) {
        toggleDown(modEnabledDown);
    }
}

void DisableDarkslayer::onConfigLoad(const utils::Config& cfg) {
    modEnabledUp = cfg.get<bool>("disable_darkslayer_up").value_or(false);
    toggleUp(modEnabledUp);
    modEnabledDown = cfg.get<bool>("disable_darkslayer_down").value_or(false);
    toggleDown(modEnabledDown);
    modEnabledLeft = cfg.get<bool>("disable_darkslayer_left").value_or(false);
    toggleLeft(modEnabledLeft);
    modEnabledRight = cfg.get<bool>("disable_darkslayer_right").value_or(false);
    toggleRight(modEnabledRight);
}

void DisableDarkslayer::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("disable_darkslayer_up", modEnabledUp);
    cfg.set<bool>("disable_darkslayer_down", modEnabledDown);
    cfg.set<bool>("disable_darkslayer_left", modEnabledLeft);
    cfg.set<bool>("disable_darkslayer_right", modEnabledRight);
}
