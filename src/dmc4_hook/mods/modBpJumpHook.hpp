#pragma once

#include "../mod.hpp"

class BpJumpHook : public Mod
{
public:
    BpJumpHook();

    static bool modEnabled;
    static uintptr_t _bpJumpHookContinue;
    static uintptr_t _bpJumpHook2Continue;
    static uintptr_t _bpJumpHook3Continue;
    void toggle(bool enable);

    std::string getModName() override { return "BpJumpHook"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
    hl::Hooker hook3;
    hl::Patch patch;
};