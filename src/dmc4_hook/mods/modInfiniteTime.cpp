#include "modInfiniteTime.hpp"

#if 1
bool InfiniteTime::modEnabled{ false };

std::optional<std::string> InfiniteTime::onInitialize() {

    return Mod::onInitialize();
}

void InfiniteTime::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x94D6E, patchTime, "\x77\x54", 2);
    }
    else
    {
        patchTime.revert();
    }
}

void InfiniteTime::onGUIframe()
{
    if (ImGui::Checkbox("Disable Timer", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfiniteTime::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("infinite_time").value_or(false);
    toggle(modEnabled);
};

void InfiniteTime::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("infinite_time", modEnabled);
};

#endif