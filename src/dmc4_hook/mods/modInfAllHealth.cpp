#include "modInfAllHealth.hpp"
//#include "hacklib/Input.h" // needed for input.wentDown
#include "../utils/MessageDisplay.hpp" // TODO(): DISPLAY_MESSAGE should probably be included in mod.hpp or something
#if 1
bool InfAllHealth::modEnabled{ false };
float inputLockoutTimer = 0.0f;
int infhphotkey;

std::optional<std::string> InfAllHealth::onInitialize() {

    return Mod::onInitialize();
}

void InfAllHealth::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x11BFD9, patchAllHealth, "\xF3\x0F\x5C\xC9", 4);
    }
    else
    {
        patchAllHealth.revert();
    }
}

void InfAllHealth::onGUIframe()
{
    if (ImGui::Checkbox("Infinite Health (All)", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfAllHealth::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("infinite_health_all").value_or(false);
    infhphotkey = cfg.get<int>("inf_hp_hotkey").value_or(0x70);
    toggle(modEnabled);
};

void InfAllHealth::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("infinite_health_all", modEnabled);
    cfg.set<int>("inf_hp_hotkey", infhphotkey);
};

void InfAllHealth::onUpdateInput(hl::Input& input)
{
    if (input.wentDown(VK_F1)) {
        DISPLAY_MESSAGE("Infinite HP toggle"); // lmao no format strings for almost a year or when i wrote that shit
        modEnabled = !modEnabled;
        toggle(modEnabled);
    }
}
#endif