#include "DanteJdc.hpp"

#if 1
bool DanteJdc::mod_enabled{false};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

uintptr_t DanteJdc::jmp_ret1{NULL};
    constexpr uintptr_t detour1_push1 = 0x00486640;
    constexpr uintptr_t detour1_mov1  = 0x00E559D8;
    constexpr uintptr_t detour1_call1 = 0x00A47AA0;
    bool jdcFlag    = 0;
    float jdcXCoord = 0.0f;
    float jdcYCoord = 0.0f;
    float jdcZCoord = 0.0f;
uintptr_t DanteJdc::jmp_ret2{NULL};
    constexpr uintptr_t detour2_call1 = 0x008DF530;
    constexpr uintptr_t detour2_jne   = 0x007E134D;
    float jdcTimer2 = 6.5f;
    float jdcTimer1 = 5.0f;
uintptr_t DanteJdc::jmp_ret3{NULL};
uintptr_t DanteJdc::jmp_ret4{NULL};
    constexpr uintptr_t detour4_mov1 = 0x00C3C6EC;
    float jdcDelay = 0.0f;
uintptr_t DanteJdc::jmp_ret5{NULL};
    float jdcOffset = 0.0f;
    float jdcOffsetAdd = 4.0f;
    int jdcHitcountDefault = 5;
    int jdcHitcount        = 5;
uintptr_t DanteJdc::jmp_ret6{NULL};
    constexpr uintptr_t detour6_call1 = 0x0050CB80;
uintptr_t DanteJdc::jmp_ret7{NULL};
uintptr_t DanteJdc::jmp_ret8{NULL};
uintptr_t DanteJdc::jmp_ret9{NULL};
    float jdcSDprojectileDelay = 40.0f;
    constexpr uintptr_t detour9_comiss1 = 0x00E163C8;
uintptr_t DanteJdc::jmp_ret10{NULL};
    float jdcBounce = 7.0f;
    float jdcEpsilon = 0.001f;
    constexpr uintptr_t detour10_call1 = 0x007BA210;
uintptr_t DanteJdc::jmp_ret11{NULL};

// set jdc vars
naked void detour1(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            pushad
            mov ebp, [static_mediator_ptr] // 0x00E558B8
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            mov eax, [ebp+0x16D0]
            test eax, eax
            je handler
            mov byte ptr [jdcFlag],1
            mov eax, [ebp+0x3080]
            test eax,eax
            je handler
            fld [eax+0x40]
            fstp [jdcXCoord]
            fld [eax+0x44]
            fstp [jdcYCoord]
            fld [eax+0x48]
            fstp [jdcZCoord]
            push detour1_push1
            mov ecx, [ebp+0x00001D64]
            mov edx, [detour1_mov1] // 00E559D8
            mov edx, [edx] // added this so we don't mov our addr
            push 01
            push 01
            lea eax, [jdcXCoord]
            push eax
            push ebp
            push 0x000000D3 // sound id
            push ecx
            push edx
            call detour1_call1 // spawn sound // 00A47AA0
            popad
            jmp originalcode

        handler:
            mov byte ptr [jdcFlag],0
            popad
		originalcode:
            mov ecx,esi
            fstp dword ptr [esp+0x04]
        // jmp_ret:
			jmp dword ptr [DanteJdc::jmp_ret1]
    }
}

// spawn jdc
naked void detour2(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            pushad
            mov ebp, [static_mediator_ptr]
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            mov al, [jdcFlag]
            test eax, eax
            je handler
            mov eax, [ebp+0x000028A4]
            movss xmm0, [eax+0x18]
            comiss xmm0, [jdcTimer1]
            jb handler
            comiss xmm0, [jdcTimer2]
            jnb handler
            mov eax, [ebp+0x15A68]
            call detour2_call1 // Spawn effect + hitbox // DevilMayCry4_DX9.exe+41F970
            popad
            jmp originalcode

        handler:
            popad
		originalcode:
            sub eax, 01
            cmp eax, 00 // complete guess of originalcode idk
            jne jmp_jne
        //jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret2]
        jmp_jne:
            jmp dword ptr [detour2_jne] // DevilMayCry4_DX9.exe+3E134D
    }
}

// Tracking slash dimension
naked void detour3(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            push ebp
            mov ebp, [static_mediator_ptr]
            mov ebp, [ebp]
            mov ebp, [ebp+0x24]
            cmp dword ptr [ebp+0x00001564], 0x6B
            je handler
            cmp dword ptr [ebp+0x00001564], 0x6C
            je handler
            mov eax, [ebp+0x15A68]
            movss xmm0, [jdcXCoord]
            movss xmm1, [jdcYCoord]
            movss xmm2, [jdcZCoord]
            movss [esi+0x30], xmm0
            movss [esi+0x34], xmm1
            movss [esi+0x38], xmm2
        code:
            pop ebp
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret3]

        originalcode:
            fld dword ptr [edi+0x08]
            fstp dword ptr [esi+0x38]
            jmp jmp_return

        handler:
            fld dword ptr [edi+0x08]
            fstp dword ptr [esi+0x38]
            jmp code
    }
}

// Reduce JDC hitbox delay
naked void detour4(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            movss xmm0, [jdcDelay]
            jmp jmp_return

		originalcode:
            push eax // movss xmm0, [detour_mov1] movs our addr rather than the value
            mov eax, [detour4_mov1]
            movss xmm0, [eax] // DevilMayCry4_DX9.exe+83C6EC
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret4]
    }
}

// Multi-hit jdc
naked void detour5(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push ebp
            push eax
            mov ebp, esi
            movss xmm0, [ebp+0x10]
            addss xmm0, [ebp+0x1790]
            comiss xmm0, [jdcOffset]
            jb handler
            mov eax, [jdcHitcount]
            cmp eax, 1
            dec eax
            mov [jdcHitcount], eax
            je handler2
            movss xmm1, [jdcOffset]
            addss xmm1, [jdcOffsetAdd]
            movss [jdcOffset], xmm1
            mov byte ptr [esi+0x15], 00
            jmp handler

        handler2:
            xorps xmm1,xmm1
            movss [jdcOffset], xmm1
        handler:
            pop eax
            pop ebp
		originalcode:
            cmp byte ptr [esi+0x15], 00
            movss xmm0, [esi+0x10]
        // jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret5]
    }
}

// Stop jdc despawn timer
naked void detour6(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            mov eax,[jdcHitcount]
            test eax,eax
            je handler
            mov byte ptr [esi+0x14], 00
            pop eax
            jmp code

        handler:
            push ecx
            mov ecx,[jdcHitcountDefault]
            mov [jdcHitcount], ecx
            pop ecx
            mov byte ptr [esi+0x14], 02
            pop eax
        code:
            call detour6_call1
            jmp jmp_return

		originalcode:
            mov byte ptr [esi+0x14], 01
            call detour6_call1
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret6]
    }
}

// Change SD effect
naked void detour7(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            mov edx, 0x000000A2
            jmp jmp_return

		originalcode:
            mov edx, 0x0000008F
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret7]
    }
}

// Change SD anim
naked void detour8(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push 0x00000905
            jmp jmp_return

		originalcode:
            push 0x00000900
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret8]
    }
}

// Reduce SD projectile delay
naked void detour9(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            comiss xmm0, [jdcSDprojectileDelay]
            jmp jmp_return
		originalcode:
            push eax
            mov eax, [detour9_comiss1]
            comiss xmm0, [eax] // [DevilMayCry4_DX9.exe+A163C8]
            pop eax
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret9]
    }
}

// JDC inertia carry
naked void detour10(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            mov al, [jdcFlag]
            test al, al
            je handler
            movss xmm6, [esi+0x1E1C]
            comiss xmm6, [jdcEpsilon]
            jb handler
            mov eax,esi
            call detour10_call1
            pop eax
            fld [esi+0x1E30]
            fstp [esi+0x00000EC0]
            fld [esi+0x1E38]
            fstp [esi+0xEC8]
            fld [jdcBounce]
            fstp [esi+0xEC4]
            jmp jmp_return

        handler:
            pop eax
		originalcode:
            movss [esi+0x00000EC4], xmm0
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret10]
    }
}

// JDC inertia enable
naked void detour11(void) {
    _asm {
			cmp byte ptr [DanteJdc::mod_enabled], 0
			je originalcode

            push eax
            mov al, [jdcFlag]
            test al,al
            je handler
            pop eax
            jmp jmp_return
        handler:
            pop eax
		originalcode:
            movss [esi+0x00001E1C], xmm0
            movss [esi+0x00001E20], xmm0
        jmp_return:
			jmp dword ptr [DanteJdc::jmp_ret11]
    }
}

std::optional<std::string> DanteJdc::on_initialize() {
    if (!install_hook_offset(0x3E1189, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init DanteJdc mod\n");
        return "Failed to init DanteJdc mod";
    }
    if (!install_hook_offset(0x3E110C, hook2, &detour2, &jmp_ret2, 9)) {
        spdlog::error("Failed to init DanteJdc mod2\n");
        return "Failed to init DanteJdc mod2";
    }
    if (!install_hook_offset(0x41F9C2, hook3, &detour3, &jmp_ret3, 6)) {
        spdlog::error("Failed to init DanteJdc mod3\n");
        return "Failed to init DanteJdc mod3";
    }
    if (!install_hook_offset(0x41F78B, hook4, &detour4, &jmp_ret4, 8)) {
        spdlog::error("Failed to init DanteJdc mod4\n");
        return "Failed to init DanteJdc mod4";
    }
    if (!install_hook_offset(0x41F8A3, hook5, &detour5, &jmp_ret5, 9)) {
        spdlog::error("Failed to init DanteJdc mod5\n");
        return "Failed to init DanteJdc mod5";
    }
    if (!install_hook_offset(0x41F95D, hook6, &detour6, &jmp_ret6, 9)) {
        spdlog::error("Failed to init DanteJdc mod6\n");
        return "Failed to init DanteJdc mod6";
    }
    if (!install_hook_offset(0x3E183B, hook7, &detour7, &jmp_ret7, 5)) {
        spdlog::error("Failed to init DanteJdc mod7\n");
        return "Failed to init DanteJdc mod7";
    }
    if (!install_hook_offset(0x3E17C3, hook8, &detour8, &jmp_ret8, 5)) {
        spdlog::error("Failed to init DanteJdc mod8\n");
        return "Failed to init DanteJdc mod8";
    }
    if (!install_hook_offset(0x3E15F9, hook9, &detour9, &jmp_ret9, 7)) {
        spdlog::error("Failed to init DanteJdc mod9\n");
        return "Failed to init DanteJdc mod9";
    }
    if (!install_hook_offset(0x3E12FE, hook10, &detour10, &jmp_ret10, 8)) {
        spdlog::error("Failed to init DanteJdc mod10\n");
        return "Failed to init DanteJdc mod10";
    }
    if (!install_hook_offset(0x3E12AD, hook11, &detour11, &jmp_ret11, 16)) {
        spdlog::error("Failed to init DanteJdc mod11\n");
        return "Failed to init DanteJdc mod11";
    }
    return Mod::on_initialize();
}

void DanteJdc::on_gui_frame() {
    ImGui::Checkbox("Judgement Cut", &mod_enabled);
    ImGui::SameLine();
    help_marker("Enable before entering a stage to load alternate animations");
}

void DanteJdc::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dante_jdc").value_or(false);
}

void DanteJdc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_jdc", mod_enabled);
}
#endif
