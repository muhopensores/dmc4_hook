#include "../mods.h"
#include "modNoClip.hpp"

#if 1
bool NoClip::modEnabled{ false };

std::optional<std::string> NoClip::onInitialize()
{
    return Mod::onInitialize();
}

void NoClip::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x115BC4, patch1, "\x90\x90\x90", 3);
        install_patch_offset(0x10FA06, patch2, "\x01", 1);
    }
    else
    {
        patch1.revert();
        patch2.revert();
    }
}

void NoClip::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    if (ImGui::Checkbox("Noclip", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void NoClip::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("noclip").value_or(false);
    toggle(modEnabled);
};

void NoClip::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("noclip", modEnabled);
};

#endif
