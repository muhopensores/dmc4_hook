#include "modInfAllHealth.hpp"

#if 1
bool InfAllHealth::modEnabled{ false };

std::optional<std::string> InfAllHealth::onInitialize() {

    return Mod::onInitialize();
}

void InfAllHealth::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x11BFD9, patchAllHealth, "\xF3\x0F\x5C\xC9", 4);
    }
    else
    {
        patchAllHealth.revert();
    }
}

void InfAllHealth::onGUIframe()
{
    if (ImGui::Checkbox("Infinite Health (All)", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfAllHealth::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("infinite_health_all").value_or(false);
    toggle(modEnabled);
};

void InfAllHealth::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("infinite_health_all", modEnabled);
};

#endif