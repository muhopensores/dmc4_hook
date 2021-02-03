#include "modDisableCameraEvents.hpp"

bool DisableCameraEvents::modEnabled{ false };

std::optional<std::string> DisableCameraEvents::onInitialize()
{
    return Mod::onInitialize();
}

void DisableCameraEvents::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x000BEF1, patch1, "\x31\xC0\xB8\x06\x00\x00\x00", 7);
        install_patch_offset(0x000C6C1, patch2, "\xEB\x17", 2);
    }
    else
    {
        patch1.revert();
        patch2.revert();
    }
}

void DisableCameraEvents::onGUIframe()
{
    if (ImGui::Checkbox("Disable Camera Events", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Certain missions or parts will cause a black screen when this option is enabled. "
               "Press the start button then X/A to go into the item menu and the screen will return to normal.");
}

void DisableCameraEvents::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("disable_camera_events").value_or(false);
    toggle(modEnabled);
};

void DisableCameraEvents::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("disable_camera_events", modEnabled);
};
