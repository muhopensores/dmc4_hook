#include "../mods.h"
#include "modFreeCam.hpp"

#if 1
bool FreeCam::modEnabled{ false };

std::optional<std::string> FreeCam::onInitialize()
{
    return Mod::onInitialize();
}

void FreeCam::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0xF9318, patch, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0xF9334, patch2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x180C1, patch3, "\x90\x90\x90\x90\x90", 5);
    }
    else
    {
        patch.revert();
        patch2.revert();
        patch3.revert();
    }
}

void FreeCam::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    if (ImGui::Checkbox("Free Cam", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void FreeCam::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("free_cam").value_or(false);
    toggle(modEnabled);
};

void FreeCam::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("free_cam", modEnabled);
};

#endif
