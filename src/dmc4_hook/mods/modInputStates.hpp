#pragma once

#include "../mod.hpp"

class InputStates : public Mod
{
public:
    InputStates();

    static uintptr_t jmp_return;
    static uintptr_t jmp_return2;
    static uint32_t inputpressed;
    static float inputTimer;
    static bool touchpadRoseEnabled;
    void PlayRose();

    std::string getModName() override { return "InputStates"; };

    std::optional<std::string> onInitialize() override;
    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    void onFrame(fmilliseconds& dt) override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
    utils::Timer* m_timer;
};