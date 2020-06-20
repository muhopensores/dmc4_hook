#include "../mods.h"
#include "modOneHitKill.hpp"

#if 1
bool OneHitKill::modEnabled{ false };

std::optional<std::string> OneHitKill::onInitialize()
{
    return Mod::onInitialize();
}

void OneHitKill::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x11C11B, patch, "\x74", 1); // TODO: Currently kills the player too. Should add esi+0x1C, 2,000f je code to fix
    }
    else
    {
        patch.revert();
    }
}

void OneHitKill::onGUIframe()
{
    if (ImGui::Checkbox("One Hit Kill", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void OneHitKill::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("one_hit_kill").value_or(false);
    toggle(modEnabled);
};

void OneHitKill::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("one_hit_kill", modEnabled);
};

#endif
