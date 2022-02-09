#include "../mods.h"
#include "modNoDeath.hpp"

#if 1
bool NoDeath::modEnabled{ false };
bool NoDeath::oneHitKill{ false };

std::optional<std::string> NoDeath::onInitialize()
{
    return Mod::onInitialize();
}

void NoDeath::toggle(bool enable){ // no death 
    if (enable)
    {
        install_patch_offset(0x11C11B, patchhp, "\xEB", 1);
        install_patch_offset(0x11C694, patchomen, "\xEB", 1);
    }
    else
    {
        patchhp.revert();
        patchomen.revert();
    }
}
void NoDeath::toggle2(bool enable) { // one hit kill
    if (enable)
    {
        install_patch_offset(0x11C11B, patchhp, "\x74", 1); // Currently kills the player too. Should add esi+0x1C, 2,000f je code to fix
    }
    else
    {
        patchhp.revert(); // 72
    }
}

void NoDeath::onGUIframe()
{
    if (ImGui::Checkbox("No Death", &modEnabled))
    {
        oneHitKill = false;
        toggle2(oneHitKill);
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Disables dying while still allowing hp to drop");
    ImGui::SameLine(205);
    if (ImGui::Checkbox("One Hit Kill", &oneHitKill))
    {
        modEnabled = false;
        toggle(modEnabled);
        toggle2(oneHitKill);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Be careful");
}

void NoDeath::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("no_death").value_or(false);
    oneHitKill = cfg.get<bool>("one_hit_kill").value_or(false);
    toggle(modEnabled);
};

void NoDeath::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("no_death", modEnabled);
    cfg.set<bool>("one_hit_kill", oneHitKill);
};

#endif
