#include "CutsceneSkip.hpp"

bool CutsceneSkip::mod_enabled_intro = false;
bool CutsceneSkip::mod_enabled_outro = false;

std::optional<std::string> CutsceneSkip::on_initialize() {
    return Mod::on_initialize();
}

void CutsceneSkip::toggle_intro(bool enable) {
    if (enable) {
        install_patch_offset(0x0006DF1, patch1, "\x80\xb8\x88\x00\x00\x00\x01", 7);
    }
    else {
        patch1.reset();
    }
}
void CutsceneSkip::toggle_outro(bool enable) {
    if (enable) {
        install_patch_offset(0x0007E6D, patch2, "\x80\xb8\x88\x00\x00\x00\x01", 7);
    }
    else {
        patch2.reset();
    }
}

void CutsceneSkip::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Skip Mission Intros"), &mod_enabled_intro)) {
        toggle_intro(mod_enabled_intro);
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Skip Mission Outros"), &mod_enabled_outro)) {
        toggle_outro(mod_enabled_outro);
    }
}

void CutsceneSkip::on_config_load(const utility::Config& cfg) {
    mod_enabled_intro = cfg.get<bool>("auto_skip_mission_intro").value_or(false);
    toggle_intro(mod_enabled_intro);
    mod_enabled_outro = cfg.get<bool>("auto_skip_mission_outro").value_or(false);
    toggle_outro(mod_enabled_outro);
}

void CutsceneSkip::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("auto_skip_mission_intro", mod_enabled_intro);
    cfg.set<bool>("auto_skip_mission_outro", mod_enabled_outro);
}
