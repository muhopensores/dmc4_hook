#include "modEnemyAttackOffScreen.hpp"

bool EnemyAttackOffScreen::modEnabled{ false };

std::optional<std::string> EnemyAttackOffScreen::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyAttackOffScreen::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x00A8CE9, patch, "\x73\x1e", 2);
    }
    else
    {
        patch.revert();
    }
}

void EnemyAttackOffScreen::onGUIframe()
{
    if (ImGui::Checkbox("Enemies Attack Off Screen", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void EnemyAttackOffScreen::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("enemy_attack_off_screen").value_or(false);
    toggle(modEnabled);
};

void EnemyAttackOffScreen::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("enemy_attack_off_screen", modEnabled);
};
