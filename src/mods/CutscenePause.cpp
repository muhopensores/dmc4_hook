#include "CutscenePause.hpp"

bool  CutscenePause::mod_enabled{ false };

void CutscenePause::toggle(bool toggle) {
    if (toggle) {
        install_patch_offset(0x50A8AB, patch1, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.reset();
    }
}

void CutscenePause::on_gui_frame() {
    if (ImGui::Checkbox(_("Pause Cutscene"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("This is hotkeyed to PAUSE BREAK by default"));
}

std::optional<std::string> CutscenePause::on_initialize() {
    utility::create_keyboard_hotkey(m_hotkeys, {VK_PAUSE}, "Pause Cutscene", "pause_cutscene");
    return Mod::on_initialize();
}

void CutscenePause::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        CutscenePause::mod_enabled = !CutscenePause::mod_enabled;
        toggle(mod_enabled);
    }
}

void CutscenePause::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("cutscene_pause").value_or(false);
}

void CutscenePause::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("cutscene_pause", mod_enabled);
}
