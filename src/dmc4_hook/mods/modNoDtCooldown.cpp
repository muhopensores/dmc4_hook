#include "../mods.h"
#include "modNoDtCooldown.hpp"

#if 1
bool NoDtCooldown::modEnabled{ false };

std::optional<std::string> NoDtCooldown::onInitialize()
{
    return Mod::onInitialize();
}

void NoDtCooldown::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x404B84, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else
    {
        patch.revert();
    }
}

void NoDtCooldown::onGUIframe()
{
    // from main.cpp
    // line 905 -> main->getMods()->onDrawUI("InfDreadnought"_hash);
    if (ImGui::Checkbox("No DT Cooldown", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void NoDtCooldown::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("no_dt_cooldown").value_or(false);
    toggle(modEnabled);
};

void NoDtCooldown::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("no_dt_cooldown", modEnabled);
};

#endif
