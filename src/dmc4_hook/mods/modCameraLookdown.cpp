#include "../mods.h"
#include "modCameraLookdown.hpp"

#if 1
bool CameraLookdown::modEnabled{ false };

std::optional<std::string> CameraLookdown::onInitialize()
{
    return Mod::onInitialize();
}

void CameraLookdown::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x132483, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else
    {
        patch.revert();
    }
}

void CameraLookdown::onGUIframe()
{
    if (ImGui::Checkbox("Camera Lookdown", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void CameraLookdown::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("camera_lookdown").value_or(false);
    toggle(modEnabled);
};

void CameraLookdown::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("camera_lookdown", modEnabled);
};

#endif
