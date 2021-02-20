#include "modInfTableHopper.hpp"

bool InfTableHopper::modEnabled{ false };

std::optional<std::string> InfTableHopper::onInitialize()
{
    return Mod::onInitialize();
}

void InfTableHopper::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03F873C, patch, "\x80\xbe\x14\xce\x00\x00\x00", 7);
    }
    else
    {
        patch.revert();
    }
}

void InfTableHopper::onGUIframe()
{
    if (ImGui::Checkbox("Infinite Table Hopper", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfTableHopper::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_table_hopper").value_or(false);
    toggle(modEnabled);
};

void InfTableHopper::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_table_hopper", modEnabled);
};
