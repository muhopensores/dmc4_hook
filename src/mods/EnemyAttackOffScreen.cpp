#include "EnemyAttackOffScreen.hpp"

bool EnemyAttackOffScreen::mod_enabled = false;

std::optional<std::string> EnemyAttackOffScreen::on_initialize() {
    return Mod::on_initialize();
}

void EnemyAttackOffScreen::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x00A8CE9, patch, "\x73\x1e", 2);
    }
    else {
        patch.reset();
    }
}

void EnemyAttackOffScreen::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Enemies Attack Offscreen"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allow enemies to attack even if they're not currently in camera view"));
}

void EnemyAttackOffScreen::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_attack_off_screen").value_or(false);
    toggle(mod_enabled);
}

void EnemyAttackOffScreen::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_attack_off_screen", mod_enabled);
}
