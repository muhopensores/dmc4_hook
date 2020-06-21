#include "../mods.h"
#include "modDisableLastEnemyZoom.hpp"

#if 1
bool DisableLastEnemyZoom::modEnabled{ false };

std::optional<std::string> DisableLastEnemyZoom::onInitialize()
{
    return Mod::onInitialize();
}

void DisableLastEnemyZoom::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x1A4C1, patch, "\xEB", 1);

    }
    else
    {
        patch.revert();
    }
}

void DisableLastEnemyZoom::onGUIframe()
{
    if (ImGui::Checkbox("Disable Last Enemy Zoom", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void DisableLastEnemyZoom::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("disable_last_enemy_zoom").value_or(false);
    toggle(modEnabled);
};

void DisableLastEnemyZoom::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("disable_last_enemy_zoom", modEnabled);
};

#endif
