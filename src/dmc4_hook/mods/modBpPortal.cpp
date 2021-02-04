#include "modBpPortal.hpp"

bool BpPortal::modEnabled{ false };

std::optional<std::string> BpPortal::onInitialize()
{
    return Mod::onInitialize();
}

void BpPortal::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x004E1E0, patch, "\x75\x07", 2);
    }
    else
    {
        patch.revert();
    }
}

void BpPortal::onGUIframe()
{
    if (ImGui::Checkbox("Instant BP Portal", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Sealed doors and portals open instantly");
}

void BpPortal::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("instant_bp_portal").value_or(false);
    toggle(modEnabled);
};

void BpPortal::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("instant_bp_portal", modEnabled);
};
