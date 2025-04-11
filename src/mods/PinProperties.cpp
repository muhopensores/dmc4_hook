#include "PinProperties.hpp"
#include "LuciAirThrow.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

bool PinProperties::pin_gs_passive_enabled  { false };
bool PinProperties::consistent_embed_enabled{ false };

uintptr_t PinProperties::jmp_ret1 { NULL };
uintptr_t PinProperties::jmp_ret2 { NULL };
    float PinSpeedMultiplier = 0.0f;
    float PinSlowDownSpeed = 0.3f;
uintptr_t PinProperties::jmp_ret3 { NULL };
uintptr_t PinProperties::jmp_ret4 { NULL };
uintptr_t PinProperties::jmp_ret5 { NULL };
uintptr_t PinProperties::jmp_ret6 { NULL };
uintptr_t PinProperties::jmp_ret7 { NULL };
uintptr_t PinProperties::jmp_ret8 { NULL };
uintptr_t PinProperties::jmp_ret9 { NULL };

naked void detour1(void) { // Explode on contact, 2nd bit
    _asm {
            cmp byte ptr [PinProperties::pin_gs_passive_enabled], 1
            je newcode
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            test [esi+0xEAC],2 // Unused uActor flags
            je originalcode

            cmp byte ptr [esi+0x17B4],1 // enemy contact flag
            je pinExplode

            cmp byte ptr [esi+0x17B5],1 // wall, ground contact flag
            je pinExplode

        originalcode:
            movss xmm2,[esi+0xEC0]
            jmp [PinProperties::jmp_ret1]
        pinExplode:
            pushad
            mov eax,[esi]
            fldz
            mov edx,[eax+0x150]
            push 01
            push ecx
            fstp dword ptr [esp]
            call edx
            popad
            pop esi
            pop ebx
            mov esp,ebp
            pop ebp
            ret
    }
}

naked void detour2(void) { // Keep pin moving, 1st bit
    _asm {
            cmp byte ptr [PinProperties::pin_gs_passive_enabled], 1
            je newcode
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            test [esi+0xEAC],1
            je originalcode
            mulss xmm2,[PinSpeedMultiplier]
        originalcode:
            mulss xmm2,xmm3
            subss xmm1,xmm2
            jmp [PinProperties::jmp_ret2]
    }
}

// commented out, idk what used this, surely something like this wasn't blanket enabled
naked void detour3(void) { // Slow down pinned enemies, 3rd bit
    _asm {
            test [ecx+0xEAC], 4
            je originalcode
            push eax
            mov eax, [ecx+0x18]
            fld [PinSlowDownSpeed]
            fstp [eax+0xCF4]
            pop eax
        originalcode:
            addss xmm0, [ecx+0x1790]
            jmp [PinProperties::jmp_ret3]
    }
}

// commented out, idk what used this
naked void detour4(void) { // Reset enemies' speed
    _asm {
            cmp [esi+0x18], 0
            je originalcode
            push eax
            mov eax, [esi+0x18]
            fld1
            fstp [eax+0xCF4]
            pop eax
        originalcode:
            movss [esi+0x1790], xmm0
            jmp [PinProperties::jmp_ret4]
    }
}

naked void detour5(void) { // Arcing explosion knockback (4), pull enemy towards player (5)
    _asm {
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            test [esi+0xEAC], 8
            je PullTest
            mov byte ptr [eax+0xA4+0x34], 8
            mov byte ptr [eax+0xA4+0x20], 2
            mov byte ptr [eax+0xA4+0x1C], 2
        PullTest:
            test [esi+0xEAC], 0x10
            je originalcode
            mov byte ptr [eax+0xA4+0x50], 2
        originalcode:
            movss [eax+0x14C], xmm0
            jmp [PinProperties::jmp_ret5]
    }
}

void __stdcall pin_conditional_properties(uintptr_t player, uintptr_t pin) {
    uint32_t* PinStat = (uint32_t*)(pin+0xEAC);
    float *PinSpeed = (float*)(pin+0x17D0);
    if ((uint8_t)*(uintptr_t*)(player + 0x156C) == 1) { // Style check: gunslinger
        *PinStat = *PinStat | 3;
    }
}

naked void detour6(void) { // pin speed
    _asm {
            cmp byte ptr [PinProperties::pin_gs_passive_enabled], 1
            je newcode
            jmp originalcode

        newcode:
            push esi
            push edi
            call pin_conditional_properties
        originalcode:
            mov [edi+ebx*4+0x14DBC],esi
            jmp [PinProperties::jmp_ret6]
    }
}

void __stdcall getCenterPos(void* Target, float* reserved) {
    _asm {
            mov eax,[Target]
            mov eax,[eax]
            mov edx,[eax+0x84]
            push reserved
            mov ecx,[Target]
            call edx
    }
}

void __stdcall Quat2Euler(void* pin, float* reserved) {
    uintptr_t convert2Euler = 0x00A32CC0;
    _asm {
            mov ecx,[pin]
            push reserved
            call convert2Euler
    }
}

void __stdcall set_incremental_rotation(float* pinPos, float* eulerAngle, float* TargetPos, float* reserved) {
    float turnRate = 0.5f;
    uintptr_t turnFunc = 0x004A99A0;
    _asm {
            mov ecx,[pinPos]
            mov eax,[TargetPos]
            mov edi,[reserved]
            push eax
            push eax
            fld [turnRate]
            fstp [esp+4]
            fld [turnRate]
            fstp [esp]
            push eulerAngle
            call turnFunc
    }
}

void __stdcall rotate(void* pin, float* angle) {
    uintptr_t rotateCall = 0x00A32930;
    _asm {
            mov eax,angle
            push pin
            call rotateCall
    }
}

void __stdcall rotate_to_target(void* pin, void* target) {
    float TargetCenterPos[3];
    float EulerAngle[3];
    float EulerSetAngle[3];
    getCenterPos(target, TargetCenterPos);
    Quat2Euler(pin, EulerAngle);
    set_incremental_rotation((float*)((uintptr_t)pin + 0x30), EulerAngle, TargetCenterPos, EulerSetAngle);
    rotate(pin, EulerSetAngle);
}

// commented out, idk what used this
naked void detour7(void) { // discipline track target
    _asm {
            pushad
            test [esi+0xEAC],0x20
            je handler
            mov edx,[esi+0x3080]
            test edx,edx
            je handler
            mov edx,[edx+8]
            push edx
            push ebx
            call rotate_to_target
            popad
        originalcode:
            movss xmm0,[esi+0x30]
            jmp [PinProperties::jmp_ret7]
        handler:
            popad
            jmp originalcode
    }
}

void __stdcall set_rotation(float* DispVal, float* RotBuffer) {
    
    RotBuffer[1] = atan2(DispVal[0], DispVal[2]);
    RotBuffer[0] = -atan2(DispVal[1], sqrt(DispVal[0] * DispVal[0] + DispVal[2] * DispVal[2]));
    RotBuffer[2] = 0;
}

void __stdcall set_relative_pos(float* Wmat, float* TargetWmat, float* res) {
    Matrix4x4 TargetMat = glm::make_mat4x4(TargetWmat);
    Matrix4x4 ObjMat = glm::make_mat4x4(Wmat);
    ObjMat = glm::translate(ObjMat,glm::vec3(0.0f, -0.0f, -20.0f));
    //tempMat           = glm::translate(tempMat, glm::vec3(Wpos[0] - tempMat[3].x, Wpos[1] - tempMat[3].y, Wpos[2] - tempMat[3].z));
    Vector3f resVec = glm::inverse(TargetMat) * glm::vec4(ObjMat[3].x, ObjMat[3].y, ObjMat[3].z, 1.0f);
    res[0] = resVec.x;
    res[1] = resVec.y;
    res[2] = resVec.z;
    if (sqrt(res[0] * res[0] + res[1] * res[1] + res[2] * res[2]) > 170.0f) {
        res[0] *= 0.6f;
        res[1] *= 0.6f;
        res[2] *= 0.6f;
    }
}

naked void detour8(void) { // Set pin attachment point
    _asm {
            cmp byte ptr [PinProperties::consistent_embed_enabled], 1
            jne originalcode

            push edi
            push eax
            push edx

            mov [esp+0x18], 1 // hip joint index
            lea edi, [esp+0x1C]
            push edi
            
            mov eax, [ebx]
            mov edx, [eax+0x3C] // Get joint wmat from index
            push 01
            mov ecx,ebx
            call edx
            push eax

            lea edi, [esi+0xF0] // Pin PrevWmat
            push edi
            call set_relative_pos
            pop edx
            pop eax
            pop edi
        originalcode:
            movss xmm4, [esp+0x18]
            jmp [PinProperties::jmp_ret8]
    }
}

void __stdcall negrad_to_2rad(float* v) {

    v[0] += (float)M_PI;
    //v[1] -= (float)M_PI;
}

//naked void detour9(void) {
//    _asm {
//            movss [esi+0x1818],xmm0
//            push edi
//            lea edi,[esi+0x1810]
//            push edi
//            call negrad_to_2rad
//            pop edi
//            jmp [PinProperties::jmp_ret9]
//    }
//}

std::optional<std::string> PinProperties::on_initialize() {
    if (!install_hook_offset(0x4131B0, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init PinProperties mod1\n");
		return "Failed to init PinProperties mod1";
	}
    if (!install_hook_offset(0x413503, hook2, &detour2, &jmp_ret2, 8)) {
		spdlog::error("Failed to init PinProperties mod2\n");
		return "Failed to init PinProperties mod2";
	}
    /*if (!install_hook_offset(0x413615, hook3, &detour3, &jmp_ret3, 8)) {
		spdlog::error("Failed to init PinProperties mod3\n");
		return "Failed to init PinProperties mod3";
	}*/
    /*if (!install_hook_offset(0x4158F0, hook4, &detour4, &jmp_ret4, 8)) {
		spdlog::error("Failed to init PinProperties mod4\n");
		return "Failed to init PinProperties mod4";
	}*/
    if (!install_hook_offset(0x4136F9, hook5, &detour5, &jmp_ret5, 8)) {
        spdlog::error("Failed to init PinProperties mod5\n");
        return "Failed to init PinProperties mod5";
    }
    if (!install_hook_offset(0x3BC5CD, hook6, &detour6, &jmp_ret6, 7)) {
        spdlog::error("Failed to init PinProperties mod6\n");
        return "Failed to init PinProperties mod6";
    }
    /*if (!install_hook_offset(0x4141D9, hook7, &detour7, &jmp_ret7, 5)) {
        spdlog::error("Failed to init PinProperties mod6\n");
        return "Failed to init PinProperties mod6";
    }*/
    if (!install_hook_offset(0x4144A7, hook8, &detour8, &jmp_ret8, 6)) {
        spdlog::error("Failed to init PinProperties mod8\n");
        return "Failed to init PinProperties mod8";
    }
    //if (!install_hook_offset(0x414596, hook9, &detour9, &jmp_ret9, 8)) {
    //    spdlog::error("Failed to init PinProperties mod8\n");
    //    return "Failed to init PinProperties mod8";
    //}
    return Mod::on_initialize();
}

void PinProperties::on_gui_frame() {
    ImGui::Checkbox(_("Gunslinger passive"), &pin_gs_passive_enabled);
    ImGui::SameLine();
    help_marker(_("Thrown lucifer pins continue moving and explode on contact in Gunslinger style."));
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Consistent embedding"), &consistent_embed_enabled);
    ImGui::SameLine();
    help_marker(_("Remove random pin placement. Embedded pin accurately attaches to hit location."));
}

void PinProperties::on_config_load(const utility::Config& cfg) {
    pin_gs_passive_enabled = cfg.get<bool>("pin_gs_passive").value_or(false);
    consistent_embed_enabled = cfg.get<bool>("consistent_embed").value_or(false);
};

void PinProperties::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pin_gs_passive", pin_gs_passive_enabled);
    cfg.set<bool>("consistent_embed", consistent_embed_enabled);
};