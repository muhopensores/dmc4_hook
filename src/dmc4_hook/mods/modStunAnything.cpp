#include "modStunAnything.hpp"

bool StunAnything::modEnabled{ false };

std::optional<std::string> StunAnything::onInitialize() {

    return Mod::onInitialize();
}

void StunAnything::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x0011C3B2, patch, "\x83\xf9\x01\x90\x90", 5);
    }
    else
    {
        patch.revert();
    }
}

void StunAnything::onGUIframe()
{
    if (ImGui::Checkbox("Stun Anything", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine();
    HelpMarker("Allows anything (even Lucifer pins) to stun");
}

void StunAnything::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("stun_anything").value_or(false);
    toggle(modEnabled);
};

void StunAnything::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("stun_anything", modEnabled);
};
