#include "../mods.h"
#include "modInfSkyStars.hpp"

bool InfSkyStars::modEnabled{ false };

std::optional<std::string> InfSkyStars::onInitialize()
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
    if (ImGui::Checkbox("Infinite Sky Stars", &modEnabled))
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
