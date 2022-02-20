#include "../mods.h"
#include "modInfFaustCloak.hpp"

#if 1
bool InfFaustCloak::modEnabled{ false };

std::optional<std::string> InfFaustCloak::onInitialize()
{
    return Mod::onInitialize();
}

void InfFaustCloak::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x1932E5, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else
    {
        patch.revert();
    }
}

void InfFaustCloak::onGUIframe()
{
    if (ImGui::Checkbox("Infinite Faust Cloak", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfFaustCloak::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_faust_cloak").value_or(false);
    toggle(modEnabled);
};

void InfFaustCloak::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_faust_cloak", modEnabled);
};

#endif
