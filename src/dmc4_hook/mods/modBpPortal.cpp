#include "modBpPortal.hpp"
#include "../utils/MessageDisplay.hpp" // TODO(): DISPLAY_MESSAGE should probably be included in mod.hpp or something
#include "modDeltaTime.hpp"

bool BpPortal::modEnabled{ false };
int BpPortal::hotkey{ NULL };
BpPortal* g_mod{ nullptr };

static void onTimerCallback()
{
    BpPortal::modEnabled = !BpPortal::modEnabled;
    g_mod->toggle(BpPortal::modEnabled);
    // DISPLAY_MESSAGE("BP Portal / Red Seals Closed"); // debug
    BpPortal::modEnabled = false;
    g_mod->toggle(BpPortal::modEnabled);
}

std::optional<std::string> BpPortal::onInitialize()
{
    g_mod = this;
    m_timer = new utils::Timer(1.0f, onTimerCallback);
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
        //install_patch_offset(0x004E1E0, patch, "\x74\x07", 2);
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
    hotkey = cfg.get<int>("red_seal_and_bp_portal_hotkey").value_or(0x73); //F4
    toggle(modEnabled);
};

void BpPortal::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("instant_bp_portal", modEnabled);
    cfg.set<int>("red_seal_and_bp_portal_hotkey", hotkey);
};

void BpPortal::onFrame(fmilliseconds& dt)
{
    m_timer->tick(dt);
};

void BpPortal::onUpdateInput(hl::Input& input)
{
    if (input.wentDown(hotkey))
    {
        if (!modEnabled)
        {
            DISPLAY_MESSAGE("BP Portal / Red Seals Opened");
            modEnabled = true;
            toggle(modEnabled);
            m_timer->start();
        }
    }
}
