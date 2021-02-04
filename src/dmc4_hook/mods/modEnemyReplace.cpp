#include "modEnemyReplace.hpp"

bool EnemyReplace::modEnabled{ false };

std::optional<std::string> EnemyReplace::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyReplace::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x0, patch1, "\x00", 1);
    }
    else
    {
        patch1.revert();
    }
}

void EnemyReplace::onGUIframe()
{
    if (ImGui::Checkbox("Enemy Replacements", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void EnemyReplace::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("enemy_replace").value_or(false);
    toggle(modEnabled);
};

void EnemyReplace::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("enemy_replace", modEnabled);
};
