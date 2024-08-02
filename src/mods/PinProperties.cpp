#include "PinProperties.hpp"

bool PinProperties::mod_enabled { false };

uintptr_t  PinProperties::jmp_ret1 { NULL };
uintptr_t  PinProperties::jmp_ret2 { NULL };
    float PinSpeedMultiplier = 0.0f;
    float PinSlowDownSpeed = 0.5f;
uintptr_t  PinProperties::jmp_ret3 { NULL };
uintptr_t  PinProperties::jmp_ret4 { NULL };


naked void detour1(void) {//Explode on contact
    _asm {
            cmp [esi+0xEA8],1 //Unused uActor flags
            jne originalcode

            cmp byte ptr [esi+0x17B4],1 //contact flag
            jne originalcode
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
        originalcode:
            movss xmm2,[esi+0xEC0]
            jmp [PinProperties::jmp_ret1]
    }
}

naked void detour2(void) {//Keep pin moving
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

naked void detour3(void) {//Slow down pinned enemies
    _asm {
            cmp [ecx+0xEA8],3
            jne originalcode
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
    return Mod::on_initialize();
}