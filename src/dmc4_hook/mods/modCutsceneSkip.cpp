#include "modCutsceneSkip.hpp"

bool CutsceneSkip::modEnabledIntro{ false };
bool CutsceneSkip::modEnabledOutro{ false };

std::optional<std::string> CutsceneSkip::onInitialize() {
    return Mod::onInitialize();
}

void CutsceneSkip::toggleIntro(bool enable) {
    if (enable) {
        install_patch_offset(0x0006DF1, patch1, "\x80\xb8\x88\x00\x00\x00\x01", 7);
    }
    else {
        patch1.revert();
    }
}
void CutsceneSkip::toggleOutro(bool enable) {
    if (enable) {
        install_patch_offset(0x0007E6D, patch2, "\x80\xb8\x88\x00\x00\x00\x01", 7);
    }
    else {
        patch2.revert();
    }
}

void CutsceneSkip::onGUIframe() {
    if (ImGui::Checkbox("Skip Mission Intros", &modEnabledIntro)) {
        toggleIntro(modEnabledIntro);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Skip Mission Outros", &modEnabledOutro)) {
        toggleOutro(modEnabledOutro);
    }
}

void CutsceneSkip::onConfigLoad(const utils::Config& cfg) {
    modEnabledIntro = cfg.get<bool>("auto_skip_mission_intro").value_or(false);
    toggleIntro(modEnabledIntro);
    modEnabledOutro = cfg.get<bool>("auto_skip_mission_outro").value_or(false);
    toggleOutro(modEnabledOutro);
}

void CutsceneSkip::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("auto_skip_mission_intro", modEnabledIntro);
    cfg.set<bool>("auto_skip_mission_outro", modEnabledOutro);
}
