#include "../mods.h"
#include "modCameraReset.hpp"

#if 1

bool CameraReset::modEnabled{ false };
bool CameraReset::camRight{ NULL };
uintptr_t CameraReset::_cameraResetContinue{ NULL };
uintptr_t CameraReset::_cameraResetDefaultContinue{ NULL };
float degrees{ 1.57f };

CameraReset::CameraReset()
{
    // onInitialize();
}

naked void cameraReset_proc(void)
{
    _asm {
		cmp byte ptr [CameraReset::modEnabled], 0
		je originalcode
        cmp byte ptr [CameraReset::camRight], 1
        je camright

        movss xmm0, [ecx+00001210h]
        subss xmm0, [degrees] // 90 degrees left
        jmp [CameraReset::_cameraResetContinue]

    camright:
        movss xmm0, [ecx+00001210h]
        addss xmm0, [degrees] // 90 degrees right
        jmp [CameraReset::_cameraResetContinue]

    originalcode:
        movss xmm0,[ecx+00001210h]
        jmp [CameraReset::_cameraResetContinue]
    }
}

naked void cameraResetDefault_proc(void)
{
    _asm {
		cmp byte ptr [CameraReset::modEnabled], 0
		je originalcode
        cmp byte ptr [CameraReset::camRight], 1
        je camright

        movss xmm0, [edx+00001210h]
        subss xmm0, [degrees] // 90 degrees left
        jmp [CameraReset::_cameraResetDefaultContinue]

    camright:
        movss xmm0, [edx+00001210h]
        addss xmm0, [degrees] // 90 degrees right
        jmp [CameraReset::_cameraResetDefaultContinue]

    originalcode:
        movss xmm0,[edx+00001210h]
        jmp [CameraReset::_cameraResetDefaultContinue]
    }
}

std::optional<std::string> CameraReset::onInitialize()
{
    if (!install_hook_offset(0x022D5F, hook, &cameraReset_proc, &CameraReset::_cameraResetContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraReset mod\n");
        return "Failed to init CameraReset mod";
    }

    if (!install_hook_offset(0x02261A, hook2, &cameraResetDefault_proc, &CameraReset::_cameraResetDefaultContinue, 8))
    {
        HL_LOG_ERR("Failed to init CameraReset mod\n");
        return "Failed to init CameraReset mod";
    }
    return Mod::onInitialize();
}

void CameraReset::onGUIframe()
{
    ImGui::Checkbox("Camera Reset", &modEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("When pressing the button that resets the camera behind Dante, the camera will instead be set to Dante's left");

    ImGui::SameLine(205);

    ImGui::Checkbox("Right Side Reset", &camRight);
    ImGui::SameLine(0, 1);
    HelpMarker("Set the camera to the right instead");
}

void CameraReset::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("camera_reset").value_or(false);
    camRight = cfg.get<bool>("right_side_reset").value_or(false);
};

void CameraReset::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("camera_reset", modEnabled);
    cfg.set<bool>("right_side_reset", camRight);
};

#endif
