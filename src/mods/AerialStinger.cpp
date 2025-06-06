#include "../sdk/Devil4.hpp"
#include "AerialStinger.hpp"
#include "MoveTable.hpp"
#if 1
bool AerialStinger::mod_enabled = false;

uintptr_t AerialStinger::jmp_ret1 = NULL;
    bool aerialFlag = false;
uintptr_t AerialStinger::jmp_ret2 = NULL;
    float multiplier = 0.4f;
    float gravity = -0.7f; 
    float ASBounce = 5.0f; 
uintptr_t AerialStinger::jmp_ret3 = NULL;
uintptr_t AerialStinger::jmp_ret4 = NULL;
uintptr_t AerialStinger::jmp_ret5 = NULL;
uintptr_t AerialStinger::jmp_ret6 = NULL;
    constexpr uintptr_t detour6_jmp = 0x007CE861;

void AerialStinger::toggle(bool enable) {
    kAtckDefTbl* StingerEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 1;
    if (enable) {
        install_patch_absolute(0x00C3FEF8, patch1, "\x03", 2);//Move class aerial lock
        install_patch_offset(0x3CE6FA, patch2, "\x90\x90\x90\x90\x90\x90\x90\x90",8);//stinger inertia
        StingerEntry->atckAs = 3; // air condition
    }
    else {
        patch1.reset();
        patch2.reset();
        StingerEntry->atckAs = 1; // air condition
    }
}

//Aerial Stinger init
naked void detour1() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je actualoriginalcode // ogcode is `or [ebp+00001550],ebx`

        push eax
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je handler

        mov eax,[ebp+0x1E8C]
        cmp byte ptr [eax+0x1C],0 // grounded check
        jne handler
        fld [ebp+0x1710]
        fstp [ebp+0x16C0]
    originalcode:
        pop eax
    retcode:
        jmp [AerialStinger::jmp_ret1]
    handler:
        or [ebp+0x1550],ebx
        jmp originalcode

    actualoriginalcode:
        or [ebp+0x1550],ebx
        jmp retcode
    }
}
//X-axis velocity
naked void detour2() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je originalcode

        cmp byte ptr [ebp+0x1550], 1
        je originalcode
        movss xmm0, [ebp+0xEC0]
        mulss xmm0, [multiplier]
    originalcode:
        movss [ebp+0xEC0],xmm0
        // xorps xmm0,xmm0 // you return to an address that does movss xmm0,0
        jmp [AerialStinger::jmp_ret2]
    }
}
//Z-axis velocity + grav
naked void detour3() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je originalcode

        cmp byte ptr [ebp+0x1550], 1
        je originalcode
        movss xmm0, [ebp+0xEC8]
        mulss xmm0, [multiplier]
        fld [ASBounce]
        fstp [ebp+0xEC4]
        fld [gravity]
        fstp [ebp+0xED4]
    originalcode:
        movss [ebp+0xEC8],xmm0
        xorps xmm0,xmm0 // xmm0 was 0 so this is restoring it
        jmp [AerialStinger::jmp_ret3]
    }
}

void __stdcall stinger_recovery_anim_call(uintptr_t actor) {
    float currFrame = *(float*)(actor+0x348);
    uint16_t motionID = (uint16_t)*(uint16_t*)(actor+0x334);
    if (motionID == 0x210)
        if (currFrame > 25.0f) 
            devil4_sdk::indexed_anim_call(0x235, (uPlayer*)actor, 0, 0.3f, 35.0f, 10.0f);
}

//Landing cancel
naked void detour4() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je originalcode

        cmp byte ptr [ebp+0x1550],1
        je originalcode

        push eax
        push ecx

        push ebp
        call stinger_recovery_anim_call // fucks eax, ecx

        pop ecx
        pop eax

        push eax
        mov eax,[ebp+0x1E8C]
        cmp byte ptr [eax+0x1C],1
        jne handler
        or [ebp+0x02A54],1 // byte ptr?
        jmp handler
    originalcode:
        mov al,[ebp+0x1D7E]
        jmp [AerialStinger::jmp_ret4]
    handler:
        pop eax
        jmp originalcode
    }
}
//Inertia brake
naked void detour5() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je originalcode

        cmp [ebp+0x30C4],01
        jb originalcode
        fldz
        fstp [ebp+0x1E1C]

        cmp [ebp+0x2A54],bl
        jne originalcode

        cmp byte ptr [ebp+0x1550],1
        je originalcode

        push eax
        push ecx
        push edx

        push ebp
        call devil4_sdk::neutral_air_transition // fucks eax, ecx, edx
        mov bl,1

        pop edx
        pop ecx
        pop eax

    originalcode:
        cmp [ebp+0x2A54],bl
        jmp [AerialStinger::jmp_ret5]
    }
}
//Suppress MS input in the air
naked void detour6() {
    _asm {
        cmp byte ptr [AerialStinger::mod_enabled], 0
        je originalcode

        cmp byte ptr [ebp+0x1550],1
        je originalcode
        jmp [detour6_jmp]
    originalcode:
        cmp dword ptr [ebp+0x0000141C],00
        jmp [AerialStinger::jmp_ret6]
    }
}

std::optional<std::string> AerialStinger::on_initialize() {
    if (!install_hook_offset(0x3CE499, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod";
	}
    if (!install_hook_offset(0x3CE711, hook2, &detour2, &jmp_ret2, 8)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod2";
	}
    if (!install_hook_offset(0x3CE73C, hook3, &detour3, &jmp_ret3, 8)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod3";
	}
    if (!install_hook_offset(0x3CE7D8, hook4, &detour4, &jmp_ret4, 6)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod4";
	}
    if (!install_hook_offset(0x3CE861, hook5, &detour5, &jmp_ret5, 6)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod5";
	}
    if (!install_hook_offset(0x3CE7F6, hook6, &detour6, &jmp_ret6, 7)) {
		spdlog::error("Failed to init AerialStinger mod\n");
		return "Failed to init AerialStinger mod6";
	}
    return Mod::on_initialize();
}

void AerialStinger::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Aerial Stinger"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allow Dante to use Stinger in the air"));
}

void AerialStinger::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("aerial_stinger").value_or(false);
	if (mod_enabled) toggle(mod_enabled);
};

void AerialStinger::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("aerial_stinger", mod_enabled);
};

#endif
