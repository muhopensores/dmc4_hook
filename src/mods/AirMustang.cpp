#include "AirMustang.hpp"

#if 1
bool AirMustang::mod_enabled{ false };

uintptr_t AirMustang::jmp_ret1{ NULL };
    long long WriteMem = 0;
    bool flag = false;
uintptr_t AirMustang::jmp_ret2{ NULL };
    float MustangBounce = 0.0f;
    float MustangInertia = 15.0f;
    float MustangGrav = -2.0f;
uintptr_t AirMustang::jmp_ret3{ NULL };
    float InertiaMultiplier = 0.8f;

naked void detour1() {
    _asm {
            cmp byte ptr [AirMustang::mod_enabled],1
            jne originalcode

            cmp byte ptr [esi+0x14D95],1
            jne originalcode

            cmp byte ptr [esi+0x1504],2 //Part of move
            jb originalcode

            //mov eax,[esi+0x1E8C]
            //cmp byte ptr [eax+0x1A],1 //Enemy contact
            //jne originalcode

            mov edx,[esi]
            mov edx,[edx+0x2D4]
            lea eax,[WriteMem]
            push eax
            mov ecx,esi
            call edx
            cmp byte ptr [eax+0x01],01
            jne originalcode


            //test byte ptr [esi+0x1415], 0x2 //input release
            test byte ptr [esi+0x140D],0x2 //input held
            //test byte ptr [esi+0x1411], 0x2 //input press
            je originalcode

            mov eax,[esi]
            mov edx,[eax+0x194]
            mov byte ptr [flag],1
            push 0x28
            mov ecx,esi
            call edx
            // pop edi
            // pop esi
            // mov esp,ebp
            // pop ebp
            // ret
        originalcode:
            pop edi
            pop esi
            mov esp,ebp
            pop ebp
            jmp [AirMustang::jmp_ret1]
    }
}

naked void detour2() {
    _asm {
            cmp byte ptr [AirMustang::mod_enabled],1
            jne originalcode

            cmp byte ptr [flag],1
            jne originalcode

            fld [MustangBounce]
            fstp [ebp+0xEC4]
            fld [MustangGrav]
            fstp [ebp+0xED4]
            fld [MustangInertia]
            fstp [ebp+0x1E1C]
            mov byte ptr [flag],0
        
        originalcode:
            add dword ptr [ebp+0x00001504],01
            jmp [AirMustang::jmp_ret2]
    }
}

naked void detour3() {
    float buffer;
    _asm {
            cmp byte ptr [AirMustang::mod_enabled],1
            jne originalcode

            //cmp byte ptr [flag],1
            //jne originalcode
            movss [buffer],xmm0
            movss xmm0,[ebp+0x1E1C]
            mulss xmm0, [InertiaMultiplier]
            movss [ebp+0x1E1C],xmm0
            movss xmm0,[buffer]
            mov byte ptr [ebp+0x3174],2//movement abilities cancel
            mov byte ptr [ebp+0x30C4],2//melee cancel
            mov byte ptr [ebp+0x31CC],2//gun cancel
            mov byte ptr [ebp+0x3148],2//directional melee cancel
            mov byte ptr [ebp+0x30F0],2

        originalcode:
            cmp byte ptr [ebp+0x1D7E],02
            jmp [AirMustang::jmp_ret3]
    }
}

std::optional<std::string> AirMustang::on_initialize() {
    if (!install_hook_offset(0x3CAE77, hook1, &detour1, &jmp_ret1, 5)) {
		spdlog::error("Failed to init AirMustang mod\n");
		return "Failed to init AirMustang mod";
	}
    if (!install_hook_offset(0x3CB562, hook2, &detour2, &jmp_ret2, 7)) {
		spdlog::error("Failed to init AirMustang mod2\n");
		return "Failed to init AirMustang mod2";
	}
    if (!install_hook_offset(0x3CB56E, hook3, &detour3, &jmp_ret3, 7)) {
		spdlog::error("Failed to init AirMustang mod3\n");
		return "Failed to init AirMustang mod3";
	}
    return Mod::on_initialize();
}


void AirMustang::on_gui_frame() {
    ImGui::Checkbox(_("Air Mustang"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Release the style button as you make contact with the enemy during Sky Star to quickly descend"));
}

void AirMustang::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("air_mustang").value_or(false);
};

void AirMustang::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("air_mustang", mod_enabled);
};

#endif
