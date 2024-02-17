#include "CharSwitcher.hpp"

bool CharSwitcher::mod_enabled{false};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

uintptr_t CharSwitcher::jmp_ret1{NULL};
    constexpr uintptr_t detour1_jmp1 = 0x00494B70;
uintptr_t CharSwitcher::jmp_ret2{NULL};
    constexpr uintptr_t detour2_call1 = 0x008DF530;
uintptr_t CharSwitcher::jmp_ret3{NULL};
uintptr_t CharSwitcher::jmp_ret4{NULL};
    constexpr uintptr_t detour4_call1 = 0x007B2150;
    constexpr uintptr_t detour4_call2 = 0x007E1B30;
    constexpr uintptr_t detour4_call3 = 0x008DC540;
    constexpr uintptr_t detour4_call4 = 0x004A5AA0;
uintptr_t CharSwitcher::jmp_ret5{NULL};
uintptr_t CharSwitcher::jmp_ret6{NULL};
    constexpr uintptr_t detour6_call1 = 0x007ACEE0;

uintptr_t primaryActor   = NULL;
uintptr_t secondaryActor = NULL;
float swapGrav           = -0.5f;
float ySpawn             = -3000.0f;
int16_t desiredInput1    = 0x40; // 0x40 default
int16_t desiredInput2    = 0x20; // 0x20 default
int16_t prevInput        = 0;

void CharSwitcher::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x005F3D, patch1, "\xEB\x1B", 2); // Load both Dante's & Nero's arc files 1
        install_patch_offset(0x005F2C, patch2, "\x90\x90", 2); // Load both Dante's & Nero's arc files 2
        install_patch_offset(0x3AA5D6, patch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 1
        install_patch_offset(0x3AA5E0, patch4, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 2
        install_patch_offset(0x3AA61E, patch5, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 3

    } else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
    }
}

// Always load Dante's save (enable out of mission to spawn Dante while playing as Nero, no effect on Nero spawn)
naked void detour1(void) {
    _asm {
			cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            jmp jnecode

		originalcode:
            test eax,eax
            jne jnecode
            lea esi, [eax+0x000001D8]
			jmp dword ptr [CharSwitcher::jmp_ret1]

        jnecode:
            jmp dword ptr [detour1_jmp1]
    }
}

// Load both Dante's & Nero's arc files
naked void detour2(void) {
    _asm {
			cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [0x00E552C8]
            mov ecx, [ecx+0x3834]
            mov ecx, [ecx+0x28]
            test cl,cl
            pop ecx
            je NeroArc
        // DanteArc:
            pushad
            push 0x00008002
            push 0x00B90CE8
            mov eax, [0x00E552D0]
            push 0x00EAD4A0
            call dword ptr [detour2_call1]
            popad
            jmp originalcode

        NeroArc:
            pushad
            push 0x00008002
            push 0x00B90CD0
            mov eax, [0x00E552D0]
            push 0x00EAD4A0
            call dword ptr [detour2_call1]
            popad
        originalcode:
            mov ecx, [0x00E14344]
            jmp dword ptr [CharSwitcher::jmp_ret2]
    }
}

// Get primary actor
naked void detour3(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            mov [primaryActor], eax

        originalcode:
            mov eax, [0x00E552CC]
            jmp dword ptr [CharSwitcher::jmp_ret3]
    }
}

// Spawn secondary actor
naked void detour4(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            pushad
            mov eax, [primaryActor]
            cmp eax, ecx
            jne handler
            mov ebp, [0x00E552C8]
            mov ebp, [ebp+0x3834]
            mov ecx, [ebp+0x28]
            test ecx, ecx
            je NeroInit
            jmp DanteInit

        DanteInit:
            mov ecx, [0x00E1434C]
            mov eax, [ecx]
            mov edx, [eax+0x14]
            push 0x10
            push 0x000152F0
            call edx
            call dword ptr [detour4_call1]
            jmp Spawn

        NeroInit:
            mov ecx, [0x00E1434C]
            mov eax, [ecx]
            mov edx, [eax+0x14]
            push 0x10
            push 0x0000D680
            call edx
            mov esi, eax
            call dword ptr [detour4_call2]
        Spawn:
            mov esi,eax
            mov eax, [0x00E552CC]
            push 0x0F
            call dword ptr [detour4_call3]
            mov [secondaryActor], esi
            popad
            jmp code

        handler:
            popad
        code:
            mov ecx, [primaryActor]
        originalcode:
            mov [eax+0x24], ecx
            call dword ptr [detour4_call4]
        // jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret4]
    }                   
}

// Initial 2nd char conditions
naked void detour5(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [secondaryActor]
            cmp ecx, esi
            jne handler
            push esi
            push eax
            push ebp
            mov eax, esi
            mov ebp, [0x00E558B8]
            mov [eax+0x1509], 0 // input
            mov esi, [eax+0x1E8C]
            mov [esi+0xD4], 1 // collision
        // Position
            fld dword ptr [ebp+0x60]
            fstp dword ptr [eax+0x30]
            fld dword ptr [ySpawn]
            fstp dword ptr [eax+0x34]
            fld dword ptr [ebp+0x68]
            fstp dword ptr [eax+0x38]
            mov [eax+0x1550], 1
            mov [esi+0x1C], 0
            xorps xmm0, xmm0
            movss [eax+0x1E1C], xmm0
            movss [eax+0xEC4], xmm0
            movss [eax+0xED0], xmm0
            movss [eax+0xED4], xmm0
            movss [eax+0xED8], xmm0
            pop ebp
            pop eax
            pop esi
            pop ecx
            jmp originalcode

        handler:
            pop ecx
        originalcode:
            pop edi
            pop esi
            pop ebx
            mov esp, ebp
        // jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret5]
    }
}

// Suspend inactive actor
naked void detour6(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ebp
            mov ebp, [0x00E558B8]
            mov ebp, [ebp+0x24]
            cmp ebp, edi
            jne handler
            pop ebp
        originalcode:
            call dword ptr [detour6_call1]
            jmp jmp_ret

        handler:
            pop ebp
        jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret6]
    }
}

// Swap actor
naked void SwapActor(void) {
    _asm {
        pushad

            mov ebp, [0x00E558B8]
            mov ebp, [ebp+0x24]
            test ebp,ebp
            je loopend
            mov ecx, [ebp+0x1374]
            xor edx,edx
            mov dx, [desiredInput1]
            add dx, [desiredInput2]
            and ecx, edx
            cmp ecx, edx
            je loop2
            mov [prevInput], cx
            jmp loopend

        loop2:
            xor esi, esi
            mov si, [prevInput]
            xor esi, ecx
            test esi, esi
            je loopend
            mov [prevInput], dx
            call swapActor
            jmp loopend

        swapActor:
            pushad
            mov eax, [primaryActor]
            mov ecx, [secondaryActor]
            mov ebp, [0x00E552C8]
            mov ebp, [ebp+0x3834]
            xor [ebp+0x28], 1
            mov ebp, [0x00E558B8]
            mov [ebp+0x24], ecx
            // To-be main actor
            mov [primaryActor], ecx
            mov [ecx+1509], 1 // input
            // Position, rotation
            fld [eax+30] // X pos
            fstp [ecx+30]
            fld [eax+34] // Y pos
            fstp [ecx+34]
            fld [eax+38] // Z pos
            fstp [ecx+38]
            fld [ecx+30]
            fstp dword ptr [ecx+0x000014A0]
            fld dword ptr [ecx+0x34]
            fstp dword ptr [ecx+0x000014A4]
            fld dword ptr [ecx+0x38]
            fstp dword ptr [ecx+0x000014A8]
            fld dword ptr [ecx+0x30]
            fstp dword ptr [ecx+0x00002170]
            fld dword ptr [ecx+0x34]
            fstp dword ptr [ecx+0x00002174]
            fld dword ptr [ecx+0x38]
            fstp dword ptr [ecx+0x00002178]
            fld [eax+0x44] // facing direction
            fstp [ecx+0x44]
            fld [eax+0x4C]
            fstp [ecx+0x4C]
            fld [eax+0x1710]
            fstp [ecx+0x1710]
            // Collision
            mov [ecx+0x150C], 1
            mov [ecx+0x150D], 1
            mov [ecx+0x150E], 1
            mov [ecx+0x150F], 1
            mov edi, [eax+0x1E8C]
            mov esi, [ecx+0x1E8C]
            mov [esi+0xD4], 0
            mov [esi+0x1C], 0
            // Inertia, gravity
            xorps xmm0, xmm0
            movss [ecx+0xEC4], xmm0
            movss xmm0, [swapGrav]
            movss [ecx+0xED4], xmm0
            mov byte ptr [ecx+0x2A57], 00
            fld [eax+0x1E1C] // inertia
            fstp [ecx+0x1E1C]
            fld [eax+0x1E30] // X-axis inertia
            fstp [ecx+0x1E30]
            fld [eax+0x1E38] // Z-axis inertia
            fstp [ecx+0x1E38]
            fld [eax+0x1E50]
            fstp [ecx+0x1E50]
            fld [eax+0x1E54]
            fstp [ecx+0x1E54]
            fld [eax+0x1E58]
            fstp [ecx+0x1E58]
            fld [eax+0x1E60]
            fstp [ecx+0x1E60]
            fld [eax+0x1E64]
            fstp [ecx+0x1E64]
            // Motion
            mov [ecx+0x1550], 01
            // Previous main actor
            mov [secondaryActor], eax
            mov [eax+0x1509], 0 // input
            mov esi,[eax+0x1E8C]
            mov [esi+0xD4], 1 // collision
            // Position
            fld dword ptr [ebp+0x60]
            fstp dword ptr [eax+0x30]
            fld dword ptr [ySpawn]
            fstp dword ptr [eax+0x34]
            fld dword ptr [ebp+0x68]
            fstp dword ptr [eax+0x38]
            mov [eax+0x1550], 1
            mov [esi+0x1C], 0
            xorps xmm0, xmm0
            movss [eax+0x1E1C], xmm0
            movss [eax+0xEC4], xmm0
            movss [eax+0xED0], xmm0
            movss [eax+0xED4], xmm0
            movss [eax+0xED8], xmm0
            popad

        loopend:
            popad
            ret
    }
}

std::optional<std::string> CharSwitcher::on_initialize() {
    if (!install_hook_offset(0x094B55, hook1, &detour1, &jmp_ret1, 8)) { // Always load Dante's save
        spdlog::error("Failed to init CharSwitcher1 mod\n");
        return "Failed to init CharSwitcher1 mod";
    }
    if (!install_hook_offset(0x007320, hook2, &detour2, &jmp_ret2, 6)) { // Load both Dante's & Nero's arc files // AAAAAAA
        spdlog::error("Failed to init CharSwitcher2 mod\n");
        return "Failed to init CharSwitcher2 mod";
    }
    if (!install_hook_offset(0x00D90E, hook3, &detour3, &jmp_ret3, 5)) { // Get primary actor
        spdlog::error("Failed to init CharSwitcher3 mod\n");
        return "Failed to init CharSwitcher3 mod";
    }
    if (!install_hook_offset(0x3A91CC, hook4, &detour4, &jmp_ret4, 5)) { // Spawn secondary actor
        spdlog::error("Failed to init CharSwitcher4 mod\n");
        return "Failed to init CharSwitcher4 mod";
    }
    if (!install_hook_offset(0x3A7C12, hook5, &detour5, &jmp_ret5, 8)) { // Initial 2nd char conditions
        spdlog::error("Failed to init CharSwitcher5 mod\n");
        return "Failed to init CharSwitcher5 mod";
    }
    if (!install_hook_offset(0x3A853F, hook6, &detour6, &jmp_ret6, 5)) { // Suspend inactive actor
        spdlog::error("Failed to init CharSwitcher6 mod\n");
        return "Failed to init CharSwitcher6 mod";
    }
    return Mod::on_initialize();
}

void CharSwitcher::on_frame(fmilliseconds& dt) {
    /*
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            if (player->buttonInputRaw != prevInput && player->buttonInputRaw & desiredInput) {
                SwapActor();
            }
            prevInput = player->buttonInputRaw;
        }
    }
    */
    if (mod_enabled) {
        //SwapActor();
    }
}

void CharSwitcher::on_gui_frame() { 
    if (ImGui::Checkbox("Char Switcher", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Enable before loading into a stage");
    if (ImGui::Button("swap actor")) {
        SwapActor();
    }
    ImGui::PushItemWidth(sameLineItemWidth);
    if (ImGui::BeginCombo("Char Swap Input 1", devil4_sdk::getButtonInfo(desiredInput1).second)) {
        for (const auto& buttonPair : buttonPairs) {
            bool is_selected = (desiredInput1 == buttonPair.first);
            if (ImGui::Selectable(buttonPair.second, is_selected)) {
                desiredInput1 = buttonPair.first;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Char Swap Input 2", devil4_sdk::getButtonInfo(desiredInput2).second)) {
        for (const auto& buttonPair : buttonPairs) {
            bool is_selected = (desiredInput2 == buttonPair.first);
            if (ImGui::Selectable(buttonPair.second, is_selected)) {
                desiredInput2 = buttonPair.first;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

void CharSwitcher::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("char_switcher").value_or(false);
    toggle(mod_enabled);
    desiredInput1 = cfg.get<int16_t>("char_swap_input1").value_or(0x40);
    desiredInput2 = cfg.get<int16_t>("char_swap_input2").value_or(0x20);
}

void CharSwitcher::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("char_switcher", mod_enabled);
    cfg.set<int16_t>("char_swap_input1", desiredInput1);
    cfg.set<int16_t>("char_swap_input2", desiredInput1);
}
