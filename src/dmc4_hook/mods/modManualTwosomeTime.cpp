#include "../mods.h"
#include "modManualTwosomeTime.hpp"

#if 1
bool ManualTwosomeTime::modEnabled{ false };

std::optional<std::string> ManualTwosomeTime::onInitialize()
{
    return Mod::onInitialize();
}

void ManualTwosomeTime::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3B8B1F, patch, "\xE9\x48\x02\x00\x00\x90", 6);
    }
    else
    {
        patch.revert();
    }
}

void ManualTwosomeTime::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    if (ImGui::Checkbox("Manual Twosome Time", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void ManualTwosomeTime::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("manual_twosome_time").value_or(false);
    toggle(modEnabled);
};

void ManualTwosomeTime::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("manual_twosome_time", modEnabled);
};

#endif
