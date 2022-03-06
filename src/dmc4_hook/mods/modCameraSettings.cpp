#include "../mods.h"
#include "modCameraSettings.hpp"

bool cameraSensEnabled{ false };
bool cameraAutoCorrectTowardsCamEnabled{ false };

bool CameraSettings::modEnabled{ false };
float CameraSettings::cameraHeight{ 0 };
float CameraSettings::cameraDistance{ 0 };
float CameraSettings::cameraDistanceLockon{ 0 };
float CameraSettings::cameraAngle{ 0 };
float CameraSettings::cameraAngleLockon{ 0 };
float CameraSettings::cameraFovInBattle{ 0 };
float CameraSettings::cameraFov{ 0 };
bool  CameraSettings::freeCamEnabled{ false };
bool CameraSettings::cameraLookdownEnabled{ false };
bool CameraSettings::cameraResetEnabled{ false };
bool CameraSettings::camRight{ false };
bool CameraSettings::disableLastEnemyZoom{ false };

constexpr ptrdiff_t cameraTowardsAutoCorrect1 = 0x195A5;
constexpr ptrdiff_t cameraTowardsAutoCorrect2 = 0x195F7;
constexpr ptrdiff_t cameraTowardsAutoCorrect3 = 0x19822;

uintptr_t CameraSettings::cameraHeightContinue{ NULL };
uintptr_t CameraSettings::cameraDistanceContinue{ NULL };
uintptr_t CameraSettings::cameraDistanceLockonContinue{ NULL };
uintptr_t CameraSettings::cameraAngleContinue{ NULL };
uintptr_t CameraSettings::cameraAngleLockonContinue{ NULL };
uintptr_t CameraSettings::cameraFovInBattleContinue{ NULL };
uintptr_t CameraSettings::cameraFovContinue{ NULL };
uintptr_t CameraSettings::cameraResetKeyboardContinue{ NULL };
uintptr_t CameraSettings::cameraResetContinue{ NULL };
uintptr_t CameraSettings::cameraSensClockwiseContinue{ NULL };
uintptr_t CameraSettings::cameraSensAntiClockwiseContinue{ NULL };
uintptr_t CameraSettings::cameraSensBrakesContinue{ NULL };

float degrees{ 1.57f };
float doubleCameraSens{ 2.0f };

naked void cameraHeight_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm0,[edi+0x000000D0]
			addss xmm0,[CameraSettings::cameraHeight]
			jmp dword ptr [CameraSettings::cameraHeightContinue]

        code:
            movss xmm0,[edi+0x000000D0]
            jmp dword ptr [CameraSettings::cameraHeightContinue]
    }
}

naked void cameraDistance_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm0,[edi+0x000000E0]
			addss xmm0,[CameraSettings::cameraDistance]
			jmp dword ptr [CameraSettings::cameraDistanceContinue]

        code:
            movss xmm0,[edi+0x000000E0]
            jmp dword ptr [CameraSettings::cameraDistanceContinue]
    }
}

naked void cameraDistanceLockon_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm0,[ebx+0x000000DC]
			addss xmm0,[CameraSettings::cameraDistanceLockon]
			jmp dword ptr [CameraSettings::cameraDistanceLockonContinue]

        code:
            movss xmm0,[ebx+0x000000DC]
            jmp dword ptr [CameraSettings::cameraDistanceLockonContinue]
    }
}

naked void cameraAngle_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm2,[edi+0x000000D4]
			addss xmm2,[CameraSettings::cameraAngle]
            comiss xmm0,xmm1
			jmp dword ptr [CameraSettings::cameraAngleContinue]

        code:
            movss xmm2,[edi+0x000000D4]
            comiss xmm0,xmm1
            jmp dword ptr [CameraSettings::cameraAngleContinue]
    }
}

naked void cameraAngleLockon_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::cameraLookdownEnabled],0
            je code

			movss xmm0,[ebx+0x000000D4]
			addss xmm0,[CameraSettings::cameraAngleLockon]
			jmp dword ptr [CameraSettings::cameraAngleLockonContinue]

        code:
            movss xmm0,[ebx+0x000000D4]
            jmp dword ptr [CameraSettings::cameraAngleLockonContinue]
    }
}

naked void cameraFovInBattle_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm0,[esi+0x000000E8]
			addss xmm0,[CameraSettings::cameraFovInBattle]
			jmp dword ptr [CameraSettings::cameraFovInBattleContinue]

        code:
            movss xmm0,[esi+0x000000E8]
            jmp dword ptr [CameraSettings::cameraFovInBattleContinue]
    }
}

naked void cameraFov_proc(void)
{
    _asm {
            cmp byte ptr [CameraSettings::modEnabled],0
            je code

			movss xmm0,[esi+0x000000E4]
			addss xmm0,[CameraSettings::cameraFov]
			jmp dword ptr [CameraSettings::cameraFovContinue]

        code:
            movss xmm0,[esi+0x000000E4]
            jmp dword ptr [CameraSettings::cameraFovContinue]
    }
}

naked void cameraResetKeyboard_proc(void)
{
    _asm {
		    cmp byte ptr [CameraSettings::cameraResetEnabled], 1
		    je camleft
            cmp byte ptr [CameraSettings::camRight], 1
            je camright
            jmp originalcode

        camleft:
            movss xmm0, [edx+00001210h]
            subss xmm0, [degrees] // 90 degrees left
            jmp [CameraSettings::cameraResetKeyboardContinue]

        camright:
            movss xmm0, [edx+00001210h]
            addss xmm0, [degrees] // 90 degrees right
            jmp [CameraSettings::cameraResetKeyboardContinue]

        originalcode:
            movss xmm0, [edx+00001210h]
            jmp [CameraSettings::cameraResetKeyboardContinue]
    }
}

naked void cameraReset_proc(void)
{
    _asm {
		    cmp byte ptr [CameraSettings::cameraResetEnabled], 1 
		    je camleft
            cmp byte ptr [CameraSettings::camRight], 1
            je camright
            jmp originalcode

        camleft:
            movss xmm0, [edx+00001210h]
            subss xmm0, [degrees] // 90 degrees left
            jmp [CameraSettings::cameraResetContinue]

        camright:
            movss xmm0, [edx+00001210h]
            addss xmm0, [degrees] // 90 degrees right
            jmp [CameraSettings::cameraResetContinue]

        originalcode:
            movss xmm0,[edx+00001210h]
            jmp [CameraSettings::cameraResetContinue]
    }
}

naked void cameraSensClockwise_proc(void)
{
    _asm {
            cmp byte ptr [cameraSensEnabled], 0
            je code

            mulss xmm4, [doubleCameraSens]
        code:
            movss [esi+00000268h], xmm4
            jmp dword ptr [CameraSettings::cameraSensClockwiseContinue]
    }
}

naked void cameraSensAntiClockwise_proc(void)
{
    _asm {
            cmp byte ptr [cameraSensEnabled], 0
            je code

            mulss xmm6, [doubleCameraSens]
        code:
            movss [esi+00000268h], xmm6
            jmp dword ptr [CameraSettings::cameraSensAntiClockwiseContinue]
    }
}

naked void cameraSensBrakes_proc(void)
{
    _asm {
            cmp byte ptr [cameraSensEnabled], 0
            je code

            mulss xmm6, [doubleCameraSens]
        code:
            subss xmm0, xmm6
            movss [esi+00000268h], xmm0
            jmp dword ptr [CameraSettings::cameraSensBrakesContinue]
    }
}

std::optional<std::string> CameraSettings::onInitialize()
{
    if (!install_hook_offset(0x0191C5, hook, &cameraHeight_proc, &CameraSettings::cameraHeightContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01946C, hook, &cameraDistance_proc, &CameraSettings::cameraDistanceContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01A140, hook, &cameraDistanceLockon_proc, &CameraSettings::cameraDistanceLockonContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01914C, hook, &cameraAngle_proc, &CameraSettings::cameraAngleContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x0198E6, hook, &cameraAngleLockon_proc, &CameraSettings::cameraAngleLockonContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x0180EA, hook, &cameraFovInBattle_proc, &CameraSettings::cameraFovInBattleContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x018193, hook, &cameraFov_proc, &CameraSettings::cameraFovContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x02261A, cameraResetHook, &cameraReset_proc, &CameraSettings::cameraResetContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraReset mod\n");
        return "Failed to init CameraReset mod";
    }
    
    if (!install_hook_offset(0x022681, cameraResetKeyboardHook, &cameraResetKeyboard_proc, &CameraSettings::cameraResetKeyboardContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraReset2 mod\n");
        return "Failed to init CameraReset2 mod";
    }

    if (!install_hook_offset(0x0225A4, cameraSensClockwiseHook, &cameraSensClockwise_proc, &CameraSettings::cameraSensClockwiseContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSens mod\n");
        return "Failed to init CameraSens mod";
    }

    if (!install_hook_offset(0x0225BB, cameraSensAntiClockwiseHook, &cameraSensAntiClockwise_proc, &CameraSettings::cameraSensAntiClockwiseContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraSens2 mod\n");
        return "Failed to init CameraSens2 mod";
    }

    if (!install_hook_offset(0x022575, cameraSensBrakesHook, &cameraSensBrakes_proc, &CameraSettings::cameraSensBrakesContinue, 12))
    {
        HL_LOG_ERR("Failed to init CameraSens3 mod\n");
        return "Failed to init CameraSens3 mod";
    }

    return Mod::onInitialize();
}

void CameraSettings::toggleAttackTowardsCam(bool toggle)
{
    if (toggle)
    {
        install_patch_offset(cameraTowardsAutoCorrect1, attackTowardsCamPatch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(cameraTowardsAutoCorrect2, attackTowardsCamPatch2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(cameraTowardsAutoCorrect3, attackTowardsCamPatch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else
    {
        attackTowardsCamPatch1.revert();
        attackTowardsCamPatch2.revert();
        attackTowardsCamPatch3.revert();
    }
}

void CameraSettings::toggleFreeCam(bool toggle)
{
    if (toggle)
    {
        install_patch_offset(0xF9318, patchFreeCam1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0xF9334, patchFreeCam2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x180C1, patchFreeCam3, "\x90\x90\x90\x90\x90", 5);
    }
    else
    {
        patchFreeCam1.revert();
        patchFreeCam2.revert();
        patchFreeCam3.revert();
    }
}

void CameraSettings::toggleCameraLookdown(bool toggle)
{
    if (toggle)
    {
        install_patch_offset(0x132483, patchCameraLookdown, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else
    {
        patchCameraLookdown.revert();
    }
}

void CameraSettings::toggleDisableLastEnemyZoom(bool toggle)
{
    if (toggle)
    {
        install_patch_offset(0x01A4C1, cameraDisableLastEnemyZoomPatch, "\xEB", 1);
    }
    else
    {
        cameraDisableLastEnemyZoomPatch.revert();
    }
}

void CameraSettings::onGUIframe()
{
    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Checkbox("Custom Camera Variables", &modEnabled);
        ImGui::PushItemWidth(217);
        ImGui::InputFloat("Camera Height", &CameraSettings::cameraHeight, 1.0f, 1.0f, "%.0f");

        ImGui::Spacing();

        ImGui::InputFloat("Camera Distance", &CameraSettings::cameraDistance, 1.0f, 10.0f, "%.0f%");

        ImGui::Spacing();

        ImGui::InputFloat("Camera Distance (Lockon)", &CameraSettings::cameraDistanceLockon, 1.0f, 10.0f, "%.0f%");

        ImGui::Spacing();

        ImGui::InputFloat("Camera Angle", &CameraSettings::cameraAngle, 0.1f, 0.5f, "%.1f%");

        ImGui::Spacing();

        ImGui::InputFloat("Camera Angle (Lockon)", &CameraSettings::cameraAngleLockon, 0.1f, 0.5f, "%.1f%");

        ImGui::Spacing();

        ImGui::InputFloat("Camera FOV", &CameraSettings::cameraFov, 1.0f, 10.0f, "%.0f%");

        ImGui::Spacing();

        ImGui::InputFloat("Camera FOV (In Battle)", &CameraSettings::cameraFovInBattle, 1.0f, 10.0f, "%.0f%");
        ImGui::PopItemWidth();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Checkbox("Disable Autocorrect When Attacking Camera Direction", &cameraAutoCorrectTowardsCamEnabled))
        {
            toggleAttackTowardsCam(cameraAutoCorrectTowardsCamEnabled);
        }

        ImGui::Checkbox("Increased Sensitivity", &cameraSensEnabled);
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Disable Last Enemy Zoom", &disableLastEnemyZoom))
        {
            toggleDisableLastEnemyZoom(disableLastEnemyZoom);
        }

        if (ImGui::Checkbox("Free Cam", &freeCamEnabled))
        {
            toggleFreeCam(freeCamEnabled);
        }
        ImGui::SameLine(0, 1);
        HelpMarker("Activate this before starting a level! Forces Free Cam and allows it to pass through walls");
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Camera Lookdown", &cameraLookdownEnabled))
        {
            toggleCameraLookdown(cameraLookdownEnabled);
        }
        ImGui::SameLine(0, 1);
        HelpMarker("When above the locked on enemy the camera will look down");

        if (ImGui::Checkbox("Left Side Reset", &cameraResetEnabled))
        {
            camRight = 0;
        }
        ImGui::SameLine(0, 1);
        HelpMarker("When pressing the button that resets the camera behind Dante, the camera will instead be set to Dante's left");
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Right Side Reset", &camRight))
        {
            cameraResetEnabled = 0;
        }
        ImGui::SameLine(0, 1);
        HelpMarker("Set the camera to the right instead");
    }
}

void CameraSettings::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("camera_settings").value_or(false);
    cameraHeight = cfg.get<float>("camera_height").value_or(0.0f);
    cameraDistance = cfg.get<float>("camera_distance").value_or(0.0f);
    cameraDistanceLockon = cfg.get<float>("camera_distance_lockon").value_or(0.0f);
    cameraAngle = cfg.get<float>("camera_angle").value_or(0.0f);
    cameraAngleLockon = cfg.get<float>("camera_angle_lockon").value_or(0.0f);
    cameraFov = cfg.get<float>("camera_fov_battle").value_or(0.0f);
    cameraSensEnabled = cfg.get<bool>("increased_camera_sensitivity").value_or(false);
    cameraAutoCorrectTowardsCamEnabled = cfg.get<bool>("disable_camera_autocorrect_towards_camera").value_or(false);
    toggleAttackTowardsCam(cameraAutoCorrectTowardsCamEnabled);
    freeCamEnabled = cfg.get<bool>("free_cam").value_or(false);
    toggleFreeCam(freeCamEnabled);
    cameraLookdownEnabled = cfg.get<bool>("camera_lookdown").value_or(false);
    toggleCameraLookdown(cameraLookdownEnabled);
    cameraResetEnabled = cfg.get<bool>("camera_reset").value_or(false);
    camRight = cfg.get<bool>("right_side_reset").value_or(false);
    disableLastEnemyZoom = cfg.get<bool>("disable_last_enemy_zoom").value_or(false);
    toggleDisableLastEnemyZoom(disableLastEnemyZoom);
};

void CameraSettings::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("camera_settings", modEnabled);
    cfg.set<float>("camera_height", cameraHeight);
    cfg.set<float>("camera_distance", cameraDistance);
    cfg.set<float>("camera_distance_lockon", cameraDistanceLockon);
    cfg.set<float>("camera_angle", cameraAngle);
    cfg.set<float>("camera_angle_lockon", cameraAngleLockon);
    cfg.set<float>("camera_fov_battle", cameraFov);
    cfg.set<bool>("increased_camera_sensitivity", cameraSensEnabled);
    cfg.set<bool>("disable_camera_autocorrect_towards_camera", cameraAutoCorrectTowardsCamEnabled);
    cfg.set<bool>("free_cam", freeCamEnabled);
    cfg.set<bool>("camera_lookdown", cameraLookdownEnabled);
    cfg.set<bool>("camera_reset", cameraResetEnabled);
    cfg.set<bool>("right_side_reset", camRight);
    cfg.set<bool>("disable_last_enemy_zoom", disableLastEnemyZoom);
};
