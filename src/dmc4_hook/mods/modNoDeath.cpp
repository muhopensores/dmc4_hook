#include "../mods.h"
#include "modNoDeath.hpp"

#if 1
bool NoDeath::modEnabled{ false };

std::optional<std::string> NoDeath::onInitialize()
{
    return Mod::onInitialize();
}

void NoDeath::toggle(bool enable)
{
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

void NoDeath::onGUIframe()
{
    if (ImGui::Checkbox("No Death", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Disables dying while still allowing hp to drop");
}

void NoDeath::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("no_death").value_or(false);
    toggle(modEnabled);
};

void NoDeath::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("no_death", modEnabled);
};

#endif
