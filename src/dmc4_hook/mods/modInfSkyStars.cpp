#include "../mods.h"
#include "modInfSkyStars.hpp"

#if 1
bool InfSkyStars::modEnabled{ false };
uintptr_t InfSkyStars::_infSkyStarsContinue{ NULL };

std::optional<std::string> InfSkyStars::onInitialize()/*
{   
    if (!install_hook_offset(0x3BDB76, hook, &infDreadnought_proc, &InfDreadnought::_infDreadnoughtContinue, 6))
    {
        HL_LOG_ERR("Failed to init InfSkyStars mod\n");
        return "Failed to init InfSkyStars mod";
    }
*/
{
    return Mod::onInitialize();
}

void InfSkyStars::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3B74AC, patch, "\x90\x90\x90", 3);
    }
    else
    {
        patch.revert();
    }
}

void InfSkyStars::onGUIframe()
{
    // from main.cpp
    // line 905 -> main->getMods()->onDrawUI("InfDreadnought"_hash);
    if (ImGui::Checkbox("Inf Sky Stars", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfSkyStars::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_sky_stars").value_or(false);
    toggle(modEnabled);
};

void InfSkyStars::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_sky_stars", modEnabled);
};

#endif
