#pragma once

#include "../mod.hpp"

class CameraReset : public Mod
{
public:
    CameraReset();

    static bool modEnabled;
    static bool camRight;
    static uintptr_t _cameraResetContinue;
    static uintptr_t _cameraResetDefaultContinue;

    std::string getModName() override { return "CameraReset"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
};