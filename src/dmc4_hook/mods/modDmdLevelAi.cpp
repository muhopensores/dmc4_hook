#include "../mods.h"
#include "modDmdLevelAi.hpp"

#if 1
bool DmdLevelAi::modEnabled{ false };

std::optional<std::string> DmdLevelAi::onInitialize()
{
    return Mod::onInitialize();
}

void DmdLevelAi::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x05D598, patch, "\x74", 1);
    }
    else
    {
        patch.revert();
    }
}

void DmdLevelAi::onGUIframe()
{
    if (ImGui::Checkbox("DMD Level AI", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void DmdLevelAi::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("dmd_level_ai").value_or(false);
    toggle(modEnabled);
};

void DmdLevelAi::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("dmd_level_ai", modEnabled);
};

#endif
