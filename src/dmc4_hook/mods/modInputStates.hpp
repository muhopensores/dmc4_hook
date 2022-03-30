#pragma once

#include "../mod.hpp"

class InputStates : public Mod
{
public:
    InputStates() = default;

    static uintptr_t jmp_return;
    static uintptr_t jmp_return2;
    static uint32_t inputpressed;
    static float inputTimer;
    static float inputTimer2;
    static bool touchpadRoseEnabled;
    static bool roseTimerActive;

    void RoseInput();
    void RoseBuffer();
    void PlayRose();
    void onTimerCallback();
    void toggleDisableRoseDespawnOnHittingEnemy(bool enabled);
    void toggleDisableRoseDespawnOnHittingObject(bool enabled);
    void toggleDisableRoseDespawnOnChangingToLucifer(bool enabled);
    void toggleRoseInfiniteTimer(bool enabled);

    std::string getModName() override { return "InputStates"; };

    std::optional<std::string> onInitialize() override;
    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
    hl::Patch patch4;
};