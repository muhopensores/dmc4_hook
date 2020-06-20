#include "../mods.h"
#include "modPassiveEnemies.hpp"

#if 1
bool PassiveEnemies::modEnabled{ false };

std::optional<std::string> PassiveEnemies::onInitialize()
{
    return Mod::onInitialize();
}

void PassiveEnemies::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x055854, patch, "\xE9\xA9\x00\x00\x00\x90", 6);
    }
    else
    {
        patch.revert();
    }
}

void PassiveEnemies::onGUIframe()
{
    // from main.cpp
    // line 905 -> main->getMods()->onDrawUI("InfDreadnought"_hash);
    if (ImGui::Checkbox("Passive Enemies", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void PassiveEnemies::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("passive_enemies").value_or(false);
    toggle(modEnabled);
};

void PassiveEnemies::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("passive_enemies", modEnabled);
};

#endif
