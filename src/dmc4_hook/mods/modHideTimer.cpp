#include "../mods.h"
#include "modHideTimer.hpp"

#if 1
bool HideTimer::modEnabled{ false };

std::optional<std::string> HideTimer::onInitialize()
{
    return Mod::onInitialize();
}

void HideTimer::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x0FDE2A, patchbp, "\x00", 1);
        install_patch_offset(0x0FE298, patchm12, "\x00", 1);
        install_patch_offset(0x50A8E6, patchm12flash, "\x90\x90\x90\x90\x90", 5);
        install_patch_offset(0x0FE088, patchbpplus, "\x90\x90\x90\x90", 4);
    }
    else
    {
        patchbp.revert();
        patchm12.revert();
        patchm12flash.revert();
        patchbpplus.revert();
    }
}

void HideTimer::onGUIframe()
{
    if (ImGui::Checkbox("Hide Timer", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void HideTimer::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("hide_timer").value_or(false);
    toggle(modEnabled);
};

void HideTimer::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("hide_timer", modEnabled);
};

#endif
