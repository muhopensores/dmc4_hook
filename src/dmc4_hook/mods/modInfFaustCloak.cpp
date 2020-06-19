#include "../mods.h"
#include "modInfFaustCloak.hpp"

#if 1
bool InfFaustCloak::modEnabled{ false };

std::optional<std::string> InfFaustCloak::onInitialize() /*
 {
     if (!install_hook_offset(0x3BDB76, hook, &infDreadnought_proc, &InfDreadnought::_infDreadnoughtContinue, 6))
     {
         HL_LOG_ERR("Failed to init InfDreadnought mod\n");
         return "Failed to init InfDreadnought mod";
     }
 */
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
    // from main.cpp
    // line 905 -> main->getMods()->onDrawUI("InfDreadnought"_hash);
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
