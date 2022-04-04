#include "../mods.h"
#include "modFrostsDontJump.hpp"

#if 1
bool FrostsDontJump::modEnabled{ false };

std::optional<std::string> FrostsDontJump::onInitialize()
{
    return Mod::onInitialize();
}

// Enemies access a float on spawn, that float seems to control I assume something like aggression until they DT
void FrostsDontJump::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x01A46F3, patch1, "\x0F\x57\xC0\x90\x90\x90\x90\x90", 8);
        // xorps xmm0, xmm0
        // nop 5
    }
    else
    {
        patch1.revert();
        // movss xmm0,[eax+00000134]
    }
}

void FrostsDontJump::onGUIframe()
{

    if (ImGui::Checkbox("No DTless Frost Escape", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine();
    HelpMarker("Disable Frosts jumping out of combos until they DT");
}

void FrostsDontJump::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("disable_frost_jumpout").value_or(false);
    toggle(modEnabled);
};

void FrostsDontJump::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("disable_frost_jumpout", modEnabled);
};

#endif
