#include "../mods.h"
#include "modInfCalibur.hpp"

bool InfCalibur::modEnabled{ false };

std::optional<std::string> InfCalibur::onInitialize()
{
    return Mod::onInitialize();
}

void InfCalibur::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03FDE98, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else
    {
        patch.revert();
    }
}

void InfCalibur::onGUIframe()
{
    if (ImGui::Checkbox("Infinite Calibur", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfCalibur::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_calibur").value_or(false);
    toggle(modEnabled);
};

void InfCalibur::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_calibur", modEnabled);
};

