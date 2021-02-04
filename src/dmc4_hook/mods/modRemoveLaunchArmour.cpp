#include "modRemoveLaunchArmour.hpp"

bool RemoveLaunchArmour::modEnabled{ false };

std::optional<std::string> RemoveLaunchArmour::onInitialize()
{
    return Mod::onInitialize();
}

void RemoveLaunchArmour::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x011C358, patch, "\x90\x90\x90\x7c\x05", 5);
    }
    else
    {
        patch.revert();
    }
}

void RemoveLaunchArmour::onGUIframe()
{
    if (ImGui::Checkbox("Remove Launch Armour", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void RemoveLaunchArmour::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("remove_launch_armour").value_or(false);
    toggle(modEnabled);
};

void RemoveLaunchArmour::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("remove_launch_armour", modEnabled);
};
