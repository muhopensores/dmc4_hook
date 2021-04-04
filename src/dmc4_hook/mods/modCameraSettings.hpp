#pragma once

#include "../mod.hpp"

class CameraSettings : public Mod
{
public:
    CameraSettings() = default;

    static bool modEnabled;
    static float cameraHeight;
    static float cameraDistance;
    static float cameraDistanceLockon;
    static float cameraAngle;
    static float cameraAngleLockon;
    static float cameraFovInBattle;
    static float cameraFov;
    static uintptr_t cameraHeightContinue;
    static uintptr_t cameraDistanceContinue;
    static uintptr_t cameraDistanceLockonContinue;
    static uintptr_t cameraAngleContinue;
    static uintptr_t cameraAngleLockonContinue;
    static uintptr_t cameraFovInBattleContinue;
    static uintptr_t cameraFovContinue;
    static uintptr_t cameraResetDefaultContinue;
    static uintptr_t cameraResetDefaultKeyboardContinue;
    static bool freeCamEnabled;
    static bool cameraLookdownEnabled;
    static bool cameraResetEnabled;
    static bool camRight;
    static bool disableLastEnemyZoom;

    std::string getModName() override { return "CameraSettings"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;
    void toggleAttackTowardsCam(bool toggle);
    void toggleFreeCam(bool toggle);
    void toggleCameraLookdown(bool toggle);
    void toggleDisableLastEnemyZoom(bool toggle);

private:
    hl::Hooker hook;
    // hl::Hooker cameraResetHook;
    hl::Hooker cameraResetDefaultHook;
    hl::Hooker cameraResetDefaultKeyboardHook;
    hl::Patch  attackTowardsCamPatch1;
    hl::Patch  attackTowardsCamPatch2;
    hl::Patch  attackTowardsCamPatch3;
    hl::Patch  patchFreeCam1;
    hl::Patch  patchFreeCam2;
    hl::Patch  patchFreeCam3;
    hl::Patch  patchCameraLookdown;
    hl::Patch  cameraDisableLastEnemyZoomPatch;
};