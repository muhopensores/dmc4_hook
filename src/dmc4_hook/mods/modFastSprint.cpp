#include "../mods.h"
#include "modFastSprint.hpp"

#if 1
bool FastSprint::modEnabled{ false };

std::optional<std::string> FastSprint::onInitialize()
{
    return Mod::onInitialize();
}

void FastSprint::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x040456C, patch, "\x77\x33", 2);
    }
    else
    {
        patch.revert();
    }
}

void FastSprint::onGUIframe()
{
    if (ImGui::Checkbox("Fast Sprint", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void FastSprint::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("fast_sprint").value_or(false);
    toggle(modEnabled);
};

void FastSprint::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("fast_sprint", modEnabled);
};

#endif
