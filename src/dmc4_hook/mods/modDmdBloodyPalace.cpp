#include "../mods.h"
#include "modDmdBloodyPalace.hpp"

#if 1
bool DmdBloodyPalace::modEnabled{ false };

std::optional<std::string> DmdBloodyPalace::onInitialize()
{
    return Mod::onInitialize();
}

void DmdBloodyPalace::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x008FF7, patch, "\x03", 1);
    }
    else
    {
        patch.revert();
    }
}

void DmdBloodyPalace::onGUIframe()
{
    if (ImGui::Checkbox("DMD Bloody Palace", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void DmdBloodyPalace::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("dmd_bloody_palace").value_or(false);
    toggle(modEnabled);
};

void DmdBloodyPalace::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("dmd_bloody_palace", modEnabled);
};

#endif
