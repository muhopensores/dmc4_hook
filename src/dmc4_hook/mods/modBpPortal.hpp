#pragma once

#include "../mod.hpp"

class BpPortal : public Mod
{
public:
    BpPortal() = default;

    static bool modEnabled;
    static int hotkey;
    void toggle(bool enable);
    // void onTimerCallback();

    std::string getModName() override { return "BpPortal"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    void onFrame(fmilliseconds& dt) override;
    void onUpdateInput(hl::Input& input) override;

private:
    utils::Timer* m_timer;
    hl::Patch patch;
};