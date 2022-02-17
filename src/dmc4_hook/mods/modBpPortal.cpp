#include "modBpPortal.hpp"
#include "../utils/MessageDisplay.hpp" // TODO(): DISPLAY_MESSAGE should probably be included in mod.hpp or something
#include "modDeltaTime.hpp"

bool BpPortal::modEnabled{ false };
int redSealAndBPPortalHotkey;

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
        //patch.revert();
        install_patch_offset(0x004E1E0, patch, "\x74\x07", 2);
    }
}

void BpPortal::onGUIframe()
{
    if (ImGui::Checkbox("Ignore Red Seal kill requirements", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Sealed doors and portals open instantly");
}

void BpPortal::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("instant_bp_portal").value_or(false);
    redSealAndBPPortalHotkey = cfg.get<int>("red_seal_and_bp_portal_hotkey").value_or(0x72);
    toggle(modEnabled);
};

void BpPortal::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("instant_bp_portal", modEnabled);
    cfg.set<int>("red_seal_and_bp_portal_hotkey", redSealAndBPPortalHotkey);
};

void BpPortal::toggleOffTimer(void) { // meant to toggle the cheat back off after the portal has opened but it never does
    float portaltimer = 0.0f;
    while (portaltimer < 10.0f)
    {
        portaltimer + 1.0f; // * DeltaTime::currentDeltaTime;
    }
    modEnabled = false;
    toggle(modEnabled);
}

void BpPortal::onUpdateInput(hl::Input& input)
{
    if (input.wentDown(redSealAndBPPortalHotkey))
    {
        if (!modEnabled)
        {
            DISPLAY_MESSAGE("BP Portal / Red Seals Opened");
            modEnabled = true;
            toggle(modEnabled);
            toggleOffTimer(); // save me
        }
    }
}
