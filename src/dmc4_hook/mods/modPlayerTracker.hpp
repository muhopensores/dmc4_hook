#pragma once

#include "../mod.hpp"

class PlayerTracker : public Mod
{
public:
    PlayerTracker();

    static bool modEnabled;
    static uintptr_t lock_on_jmp_ret;
    static uintptr_t lock_off_jmp_ret;
    static int lockOnAlloc;

    std::string getModName() override { return "PlayerTracker"; };

    std::optional<std::string> onInitialize() override;

    // void onConfigLoad(const utils::Config& cfg) override;
    // void onConfigSave(utils::Config& cfg) override;

    // void onGUIframe() override;

private:
    hl::Hooker lockOnHook;
    hl::Hooker lockOffHook;
};