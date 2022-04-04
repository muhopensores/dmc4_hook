#include "../mods.h"
#include "modFreeJc.hpp"
#include "modEasyJc.hpp"

#if 1
bool FreeJc::modEnabled{ false };

std::optional<std::string> FreeJc::onInitialize()
{
    return Mod::onInitialize();
}

void FreeJc::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x404A26, patch1, "\x90\x90", 2);
        install_patch_offset(0x427999, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else
    {
        patch1.revert();
        install_patch_offset(0x427999, patch2, "\xBF\x02\x00\x00\x00", 5);
    }
}

void FreeJc::onGUIframe()
{
    if (ImGui::Checkbox("Free Enemy Step", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Enemy Step anywhere in the room with an enemy");
}

void FreeJc::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("free_jc").value_or(false);
    toggle(modEnabled);
};

void FreeJc::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("free_jc", modEnabled);
};

#endif
