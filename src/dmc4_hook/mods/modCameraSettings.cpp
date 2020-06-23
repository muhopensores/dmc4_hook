#include "../mods.h"
#include "modCameraSettings.hpp"

#if 1

bool CameraSettings::modEnabled{ false };
float CameraSettings::cameraHeight{ 0 };
float CameraSettings::cameraDistance{ 0 };
float CameraSettings::cameraDistanceLockon{ 0 };
float CameraSettings::cameraAngle{ 0 };
float CameraSettings::cameraAngleLockon{ 0 };
float CameraSettings::cameraFovInBattle{ 0 };
float CameraSettings::cameraFov{ 0 };


uintptr_t CameraSettings::cameraHeightContinue{ NULL };
uintptr_t CameraSettings::cameraDistanceContinue{ NULL };
uintptr_t CameraSettings::cameraDistanceLockonContinue{ NULL };
uintptr_t CameraSettings::cameraAngleContinue{ NULL };
uintptr_t CameraSettings::cameraAngleLockonContinue{ NULL };
uintptr_t CameraSettings::cameraFovInBattleContinue{ NULL };
uintptr_t CameraSettings::cameraFovContinue{ NULL };

CameraSettings::CameraSettings()
{
    // onInitialize();
}

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
            cmp byte ptr [CameraSettings::modEnabled],0
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

    return Mod::onInitialize();
}

void CameraSettings::onGUIframe()
{
    ImGui::Checkbox("Camera Settings", &modEnabled);
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
};

#endif
