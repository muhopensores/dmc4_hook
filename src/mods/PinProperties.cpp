#include "PinProperties.hpp"

bool PinProperties::mod_enabled { false };

uintptr_t  PinProperties::jmp_ret1 { NULL };
uintptr_t  PinProperties::jmp_ret2 { NULL };
    float PinSpeedMultiplier = 0.0f;
    float PinSlowDownSpeed = 0.3f;
uintptr_t  PinProperties::jmp_ret3 { NULL };
uintptr_t  PinProperties::jmp_ret4 { NULL };
uintptr_t  PinProperties::jmp_ret5  {NULL};


naked void detour1(void) {//Explode on contact, 2nd bit
    _asm {
            test [esi+0xEA8],2 //Unused uActor flags
            je originalcode

            cmp byte ptr [esi+0x17B4],1 //enemy contact flag
            je pinExplode

            cmp byte ptr [esi+0x17B5],1 //wall, ground contact flag
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

naked void detour2(void) {//Keep pin moving, 1st bit
    _asm {
            test [esi+0xEA8],1
            je originalcode
            mulss xmm2,[PinSpeedMultiplier]
        originalcode:
            mulss xmm2,xmm3
            subss xmm1,xmm2
            jmp [PinProperties::jmp_ret2]
    }
}

naked void detour3(void) {//Slow down pinned enemies, 3rd bit
    _asm {
            test [ecx+0xEA8],4
            je originalcode
            push eax
            mov eax,[ecx+0x18]
            fld [PinSlowDownSpeed]
            fstp [eax+0xCF4]
            pop eax
        originalcode:
            addss xmm0,[ecx+0x1790]
            jmp [PinProperties::jmp_ret3]
    }
}

naked void detour4(void) {//Reset enemies' speed
    _asm {
            cmp [esi+0x18],0
            je originalcode
            push eax
            mov eax,[esi+0x18]
            fld1
            fstp [eax+0xCF4]
            pop eax
        originalcode:
            movss [esi+0x1790],xmm0
            jmp [PinProperties::jmp_ret4]
    }
}

naked void detour5(void) { // Arcing explision knockback
    _asm {
            test [esi+0xEA8],8
            je originalcode
            mov byte ptr [eax+0xA4+0x34],8
            mov byte ptr [eax+0xA4+0x20],2
            mov byte ptr [eax+0xA4+0x1C],2
        originalcode:
            movss [eax+0x14C],xmm0
            jmp [PinProperties::jmp_ret5]
    }
}


std::optional<std::string> PinProperties::on_initialize() {
    if (!install_hook_offset(0x4131B0, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init PinProperties mod1\n");
		return "Failed to init PinProperties mod1";
	}
    if (!install_hook_offset(0x413503, hook2, &detour2, &jmp_ret2, 8)) {
		spdlog::error("Failed to init PinProperties mod2\n");
		return "Failed to init PinProperties mod2";
	}
    if (!install_hook_offset(0x413615, hook3, &detour3, &jmp_ret3, 8)) {
		spdlog::error("Failed to init PinProperties mod3\n");
		return "Failed to init PinProperties mod3";
	}
    if (!install_hook_offset(0x4158F0, hook4, &detour4, &jmp_ret4, 8)) {
		spdlog::error("Failed to init PinProperties mod4\n");
		return "Failed to init PinProperties mod4";
	}
    if (!install_hook_offset(0x4136F9, hook5, &detour5, &jmp_ret5, 8)) {
        spdlog::error("Failed to init PinProperties mod5\n");
        return "Failed to init PinProperties mod5";
    }
    return Mod::on_initialize();
}