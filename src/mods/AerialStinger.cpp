#include "AerialStinger.hpp"

#if 1
bool AerialStinger::mod_enabled{ false };

uintptr_t AerialStinger::jmp_ret1{NULL};
    bool aerialFlag = false;
uintptr_t AerialStinger::jmp_ret2{NULL};
    float multiplier = 0.3f;
    float gravity = -0.7f; 
uintptr_t AerialStinger::jmp_ret3{NULL};
uintptr_t AerialStinger::jmp_ret4{NULL};
uintptr_t AerialStinger::jmp_ret5{NULL};

void AerialStinger::toggle(bool enable) {
    if (enable) {
        install_patch_absolute(0x00C3FEF8, patch1, "\x03", 2);
        install_patch_offset(0x3CE6FA, patch2, "\x90\x90\x90\x90\x90\x90\x90\x90",8);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

naked void detour1() {
    _asm {
        push eax
        mov eax,[ebp+0x1E8C]
        cmp byte ptr [eax+0x1C],0
        jne handler
    originalcode:
        pop eax
        jmp [AerialStinger::jmp_ret1]
    handler:
        or [ebp+0x1550],ebx
        jmp originalcode
    }
}

naked void detour2() {
    _asm {
        cmp byte ptr [ebp+0x1550], 1
        je originalcode
        movss xmm0, [ebp+0xEC0]
        mulss xmm0, [multiplier]
    originalcode:
        movss [ebp+0xEC0],xmm0
        xorps xmm0,xmm0
        jmp [AerialStinger::jmp_ret2]
    }
}

naked void detour3() {
    _asm {
        cmp byte ptr [ebp+0x1550], 1
        je originalcode
        movss xmm0, [ebp+0xEC8]
        mulss xmm0, [multiplier]
        fld [gravity]
        fstp [ebp+0xED4]
    originalcode:
        movss [ebp+0xEC8],xmm0
        xorps xmm0,xmm0
        jmp [AerialStinger::jmp_ret3]
    }
}

naked void detour4() {
    _asm {
        cmp byte ptr [ebp+0x1550],1
        je originalcode
        push eax
        //landing cancel
        mov eax,[ebp+0x1E8C]
        cmp byte ptr [eax+0x1C],1
        jne handler
        or [ebp+0x02A54],1
        jmp handler
    originalcode:
        mov al,[ebp+0x1D7E]
        jmp [AerialStinger::jmp_ret4]
    handler:
        pop eax
        jmp originalcode
    }
}

naked void detour5() {
    _asm {
        cmp [ebp+0x30C4],01
        jb originalcode
        fldz
        fstp [ebp+0x1E1C]
    originalcode:
        cmp [ebp+0x2A54],bl
        jmp [AerialStinger::jmp_ret5]
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
    return Mod::on_initialize();
}

void AerialStinger::on_gui_frame() {
    if (ImGui::Checkbox(_("Aerial Stinger"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allow Dante to use stinger in the air"));
}

void AerialStinger::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("aerial_stinger").value_or(false);
	toggle(mod_enabled);
};

void AerialStinger::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("aerial_stinger", mod_enabled);
};

#endif
