#include "AerialDrive.hpp"
#include "misc/kAtckDefTbl.cpp"
#include "SDK/ReClass.hpp"
#include "MoveTable.hpp"

#if 1
bool AerialDrive::mod_enabled{ false };

uintptr_t AerialDrive::jmp_ret1{NULL};
    bool aerialDrive = false;
uintptr_t AerialDrive::jmp_ret2{NULL};
    float ADgrav = -0.5f; 
    float ADbounce = 8.0f; 
uintptr_t AerialDrive::jmp_ret3{NULL};
uintptr_t AerialDrive::jmp_ret4{NULL};
    constexpr uintptr_t detour4_jb_addr = 0x007D0BB1;
    constexpr uintptr_t detour4_jmp = 0x007D0A7B;
    float adTimer = 7.0f;
    float orgTimer = 18.0f;
uintptr_t AerialDrive::jmp_ret5{NULL};
    float detour5_float1 = 20.0f;
    float detour5_float2 = 5.0f;
    float detour5_float3 = 0.98f;
    float detour5_float4 = 1000000.0f;
    constexpr uintptr_t detour5_call = 0x7AA410;
    constexpr uintptr_t detour5_jmp = 0x7D0BB1;
uintptr_t AerialDrive::jmp_ret6{NULL};
    constexpr uintptr_t detour6_jmp = 0x007CE861;
uintptr_t AerialDrive::jmp_ret7{NULL};
uintptr_t AerialDrive::jmp_ret8{NULL};
    float ADRecoveryGrav = -1.0f;
    float AirDriveBufferF = 20.0f;
    float detour8_float1 = 52.0f;
    float detour8_float2 = 3.0f;
uintptr_t AerialDrive::jmp_ret9{NULL};
    constexpr uintptr_t landing_cancel_call = 0x007AA6A0;


void AerialDrive::toggle(bool enable) {
    kAtckDefTbl* DanteAtkTbl = (kAtckDefTbl*)HookDanteKADTbl;
    kAtckDefTbl* DriveEntry  = &DanteAtkTbl[5];
    if (enable) {
        install_patch_absolute(0x00C3FFA0, patch1, "\x03", 2);//Move class aerial lock
        install_patch_offset(0x3D0E32, patch2, "\x90\x90\x90\x90\x90\x90",6);//unlock aerial permission
        install_patch_offset(0x3D0B5D, patch3, "\x90\x90\x90\x90\x90\x90",6);//extendable ground qd
        DriveEntry->atckAs = 3;
    }
    else {
        DriveEntry->atckAs = 1;
        patch1.reset();
        patch2.reset();
        patch3.reset(); // do we not reset patch 3??????
    }
}
//Check
naked void detour1(void) {
    _asm {
            mov byte ptr [aerialDrive],0
            push eax
            mov eax,[ebp+0x1E8C]
            cmp byte ptr [eax+0x1C],0 //if not grounded
            pop eax
            jne handler
            mov byte ptr [aerialDrive],1
        originalcode:
            jmp [AerialDrive::jmp_ret1]
        handler:
            or [ebp+01550],ebx
            jmp originalcode
    }
}

//PoM skip
naked void detour2 (void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne handler

            add dword ptr [ebp+0x1504],03 //skip wind-up PoM
            fld [ADbounce]
            fstp [ebp+0xEC4]
            fld [ADgrav]
            fstp [ebp+0xED4]
        originalcode:
            jmp [AerialDrive::jmp_ret2]
        handler:
            add dword ptr [ebp+0x1504], 02
            jmp originalcode
    }
}

//Motion
naked void detour3(void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne handler

            push 0x900 //custom motion (roulette spin)
        originalcode:
            jmp [AerialDrive::jmp_ret3]
        handler:
            push 0x23B
            jmp originalcode
    }
}
//Timer
naked void detour4(void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne originalcode

            comiss xmm0,[adTimer]
            jb detour4_jb
            jmp detour4_jmp
        originalcode:
            comiss xmm0,[orgTimer]
            jmp [AerialDrive::jmp_ret4]
        detour4_jb:
            jmp detour4_jb_addr
    }
}
//Recovery
naked void detour5(void) {
    uintptr_t esp_addr;
    //char buffer[1000];
    _asm {
            cmp byte ptr [aerialDrive],1
            jne originalcode

            mov [esp_addr],esp

            fld dword ptr [detour5_float3]
            movss xmm1,[detour5_float4]
            sub esp,30
            mov [esp+0xC],ebx
            fld dword ptr [ebp+0xEC4]
            fstp dword ptr [esp+0x20]
            fstp dword ptr [esp+0x8]
            mov edi, 8
            fld dword ptr [esp+0x20]
            mov eax,ebp
            fstp dword ptr[esp+0x4]
            fld dword ptr [detour5_float1]
            fstp dword ptr [esp]
            call detour5_call
            mov esp,[esp_addr]
            jmp detour5_jmp
        originalcode:
            fld dword ptr [detour5_float2]
            jmp [AerialDrive::jmp_ret5]
    }
}
//Projectile
naked void detour6(void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne handler

            mov eax,1
        originalcode:
            push edi
            jmp [AerialDrive::jmp_ret6]
        handler:
            mov eax,[esp+0x4]
            jmp originalcode
    }
}
//Cancel
naked void detour7(void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne originalcode

            fld [ADRecoveryGrav]
            fstp [ebp+0xED4]

            mov byte ptr [ebp+0x30C4],2
            mov byte ptr [ebp+0x3174],2//movement abilities cancel
            mov byte ptr [ebp+0x30C4],2//melee cancel
            mov byte ptr [ebp+0x31CC],2//gun cancel
            mov byte ptr [ebp+0x3148],2//directional melee cancel
            mov byte ptr [ebp+0x30F0],2//can melee cancel, again
        originalcode:
            push ebp
            push 65
            mov ecx,ebx
            jmp [AerialDrive::jmp_ret7]
    }
}
//Buffer
naked void detour8(void) {
    //uintptr_t esp_addr;
    _asm {
            movss xmm0,[ecx+0x18]
            cmp byte ptr [aerialDrive],1
            jne originalcode

            comiss xmm0,[AirDriveBufferF]
            jb originalcode

            mov byte ptr [ebp+0x30C4],1
            mov byte ptr [ebp+0x3174],1//movement abilities cancel
            mov byte ptr [ebp+0x30C4],1//melee cancel
            mov byte ptr [ebp+0x31CC],1//gun cancel
            mov byte ptr [ebp+0x3148],1//directional melee cancel
            mov byte ptr [ebp+0x30F0],1//can melee cancel, again
        originalcode:
            jmp [AerialDrive::jmp_ret8]
    }
}
//Landing cancel, wew
naked void detour9(void) {
    _asm {
            cmp byte ptr [aerialDrive],1
            jne originalcode

            push eax
            mov eax,[ebp+0x1E8C]
            cmp byte ptr [eax+0x1C],1
            pop eax
            jne originalcode
            or byte ptr [ebp+0x1550],1
            mov [ebp+0x1564],0
            mov byte ptr [aerialDrive],0
            mov ecx,ebp
            call landing_cancel_call
            mov [ebp+0x1504],6
        originalcode:
            lea ecx,[ebp+0x00001D90]
            jmp [AerialDrive::jmp_ret9]
    }
}

std::optional<std::string> AerialDrive::on_initialize() {
    if (!install_hook_offset(0x3D0742, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod";
	}
    if (!install_hook_offset(0x3D0779, hook2, &detour2, &jmp_ret2, 7)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod2";
	}
    if (!install_hook_offset(0x3D0A08, hook3, &detour3, &jmp_ret3, 5)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod3";
	}
    if (!install_hook_offset(0x3D0A6E, hook4, &detour4, &jmp_ret4, 7)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod4";
	}
    if (!install_hook_offset(0x3D0B9F, hook5, &detour5, &jmp_ret5, 6)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod5";
	}
    if (!install_hook_offset(0x3BC260, hook6, &detour6, &jmp_ret6, 5)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod6";
	}
    if (!install_hook_offset(0x3D0B4A, hook7, &detour7, &jmp_ret7, 5)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod7";
	}
    if (!install_hook_offset(0x3D0B32, hook8, &detour8, &jmp_ret8, 5)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod8";
	}
    if (!install_hook_offset(0x3D06FA, hook9, &detour9, &jmp_ret9, 6)) {
		spdlog::error("Failed to init AerialDrive mod\n");
		return "Failed to init AerialDrive mod9";
	}
    return Mod::on_initialize();
}

void AerialDrive::on_gui_frame() {
    if (ImGui::Checkbox(_("Aerial Drive"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allow Dante to use Drive in the air"));
}

void AerialDrive::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("aerial_drive").value_or(false);
	toggle(mod_enabled);
};

void AerialDrive::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("aerial_drive", mod_enabled);
};

#endif