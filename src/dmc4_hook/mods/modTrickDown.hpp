#pragma once

#include "../mod.hpp"

class TrickDown : public Mod
{
public:
    TrickDown();

    static bool modEnabled;
    static uintptr_t trick_down_jmp_ret;
    static uintptr_t floor_touch_jmp_ret;

    std::string getModName() override { return "TrickDown"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker trickDownHook;
    hl::Hooker floorTouchHook;
};