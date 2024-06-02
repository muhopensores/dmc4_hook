#include "RisingSun.hpp"

#if 1
bool RisingSun::mod_enabled{ false };

uintptr_t RisingSun::jmp_ret1{NULL};
    constexpr uintptr_t detour1_call = 0x007D36A0;
    constexpr uintptr_t detour1_org_call = 0x007D4970;
    bool kickFlag = false;
uintptr_t RisingSun::jmp_ret2{NULL};
    float RS_start_frame = 135.0f;
uintptr_t RisingSun::jmp_ret3{NULL};
    constexpr uintptr_t detour3_jmp1 = 0x007D3733;
uintptr_t RisingSun::jmp_ret4{NULL};
    float kickGrav = -1.0f;
uintptr_t RisingSun::jmp_ret5{NULL};
    constexpr uintptr_t detour5_call = 0x7BA210;
    float epsilon = 0.001f;
    float bounce = 10.0f;
uintptr_t RisingSun::jmp_ret6{NULL};
uintptr_t RisingSun::jmp_ret7{NULL};
uintptr_t RisingSun::jmp_ret8{NULL};
    float speed = 1.1f;
// uintptr_t RisingSun::jmp_ret9{NULL};
//     constexpr uintptr_t detour9_call = 0x00820060;



void RisingSun::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3D37E6, patch1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3D37F3, patch2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3D3748, patch3, "\x90\x90\x90\x90\x90", 5); //remove grounded collision check for efx
    }
    else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
    }
}

naked void detour1() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne originalcode

            mov byte ptr [kickFlag], 1
            call [detour1_call]
            mov byte ptr [kickFlag], 0
            jmp [RisingSun::jmp_ret1]

            originalcode:
            call [detour1_org_call]
            jmp [RisingSun::jmp_ret1]
    }
}

naked void detour2() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne handler

            cmp byte ptr [kickFlag], 1
            jne handler
            fld dword ptr [RS_start_frame]

        originalcode:
            jmp [RisingSun::jmp_ret2]

        handler:
            fldz
            or [ebp+0x1550], ebx
            jmp originalcode
    }
}

naked void detour3() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne originalcode

            cmp byte ptr [kickFlag], 1
            jne originalcode
            jmp dword ptr [detour3_jmp1]

        originalcode:
            cmp dword ptr [ebp+0x14D34], 0
            jne jmp_jne
            push 0x30E
            jmp [RisingSun::jmp_ret3]
        jmp_jne:
            jmp [detour3_jmp1]
    }
}

naked void detour4() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne handler

            cmp byte ptr [kickFlag], 1
            jne handler
            fld dword ptr [kickGrav]
            
        originalcode:
            jmp [RisingSun::jmp_ret4]

        handler:
            fld dword ptr [edx+0x178]
            jmp originalcode
    }
}

naked void detour5() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne handler

            cmp byte ptr [kickFlag], 1
            jne handler

            movss xmm0, [ebp+0x1E1C]
            comiss xmm0, [epsilon]
            jb handler

            push eax
            mov eax, ebp
            call [detour5_call]
            pop eax

            fld [ebp+0x1E30]
            fstp [ebp+0xEC0]
            fld [ebp+0x1E38]
            fstp [ebp+0xEC8]
            fld dword ptr [bounce]

        originalcode:
            jmp [RisingSun::jmp_ret5]

        handler:
            fld dword ptr [eax+0x388]
            jmp originalcode
    }
}

naked void detour6() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne originalcode

            cmp byte ptr [kickFlag], 1
            jne originalcode

            movss [ebp+0xEC0], xmm0
            movss [ebp+0xEC8], xmm0

        originalcode:
            fld1
            sub esp, 0x0C
            jmp [RisingSun::jmp_ret6]
    }
}

naked void detour7() {
    _asm {
            cmp byte ptr [RisingSun::mod_enabled], 1
            jne originalcode

            cmp byte ptr [kickFlag], 1
            jne originalcode

            movss [ebp+0xEC0], xmm0
            movss [ebp+0xEC8], xmm0

        originalcode:
            movss [ebp+0xEC4], xmm0
            jmp [RisingSun::jmp_ret7]
    }
}
//Speed up animation
naked void detour8() {
    _asm {
        cmp byte ptr [RisingSun::mod_enabled], 1
        jne handler
        
        cmp byte ptr [kickFlag],1
        jne handler

        fld [speed]
    originalcode:
        sub esp,0x0C
        jmp [RisingSun::jmp_ret8]
    handler:
        fld1
        jmp originalcode
    }
}

// naked void detour9() {
//     _asm {
//         cmp byte ptr [RisingSun::mod_enabled], 1
//         jne handler

//         cmp byte ptr [kickFlag],1
//         jne handler

//     originalcode:
//         jmp [RisingSun::jmp_ret9]
//     handler:
//         call [detour9_call]
//         jmp originalcode
//     }
// }

std::optional<std::string> RisingSun::on_initialize() {
    if (!install_hook_offset(0x3CD359, hook1, &detour1, &jmp_ret1, 5)) {
		spdlog::error("Failed to init RisingSun mod\n");
		return "Failed to init RisingSun mod";
	}
    if (!install_hook_offset(0x3D36D7, hook2, &detour2, &jmp_ret2, 8)) {
		spdlog::error("Failed to init RisingSun mod2\n");
		return "Failed to init RisingSun mod2";
	}
    if (!install_hook_offset(0x3D36F9, hook3, &detour3, &jmp_ret3, 7)) {
		spdlog::error("Failed to init RisingSun mod3\n");
		return "Failed to init RisingSun mod3";
	}
    if (!install_hook_offset(0x3D390C, hook4, &detour4, &jmp_ret4, 6)) {
		spdlog::error("Failed to init RisingSun mod4\n");
		return "Failed to init RisingSun mod4";
	}
    if (!install_hook_offset(0x3D380B, hook5, &detour5, &jmp_ret5, 6)) {
		spdlog::error("Failed to init RisingSun mod5\n");
		return "Failed to init RisingSun mod5";
	}
    if (!install_hook_offset(0x3D3943, hook6, &detour6, &jmp_ret6, 5)) {
		spdlog::error("Failed to init RisingSun mod6\n");
		return "Failed to init RisingSun mod6";
	}
    if (!install_hook_offset(0x3D38E4, hook7, &detour7, &jmp_ret7, 8)) {
		spdlog::error("Failed to init RisingSun mod7\n");
		return "Failed to init RisingSun mod7";
	}
    if (!install_hook_offset(0x3D36C9, hook8, &detour8, &jmp_ret8, 5)) {
		spdlog::error("Failed to init RisingSun mod8\n");
		return "Failed to init RisingSun mod8";
	}
    // if (!install_hook_offset(0x3D3748, hook9, &detour9, &jmp_ret9, 5)) {
	// 	spdlog::error("Failed to init RisingSun mod8\n");
	// 	return "Failed to init RisingSun mod8";
	// }
    return Mod::on_initialize();
}

void RisingSun::on_gui_frame() {
    if (ImGui::Checkbox(_("Aerial Rising Sun"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Replace Dante's Shocking with the last 2 kicks of DT Kick 13"));
}

void RisingSun::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("rising_sun").value_or(false);
	toggle(mod_enabled);
};

void RisingSun::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("rising_sun", mod_enabled);
};

#endif
