#pragma once

#include "../mod.hpp"

class TimerMem : public Mod
{
public:
    TimerMem();

    static bool TimerMem::instantHoneycombEnabled;
    static uintptr_t timer_jmp_ret;
    static uintptr_t back_forward_jmp_ret;
    static float timerMem;

    std::string getModName() override { return "TimerMem"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker timerHook;
    hl::Hooker backForwardHook;
};