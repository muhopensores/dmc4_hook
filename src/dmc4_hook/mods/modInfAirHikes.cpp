#include "modInfAirHikes.hpp"

bool InfAirHikes::modEnabled{ false };

std::optional<std::string> InfAirHikes::onInitialize()
{
    return Mod::onInitialize();
}

void InfAirHikes::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03ACA3A, patch, "\x88\x91\x7E\x1E\x00\x00", 6);
    }
    else
    {
        patch.revert();
    }
}

void InfAirHikes::onGUIframe()
{
    if (ImGui::Checkbox("Inf Air Hikes", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfAirHikes::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_air_hikes").value_or(false);
    toggle(modEnabled);
};

void InfAirHikes::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_air_hikes", modEnabled);
};
