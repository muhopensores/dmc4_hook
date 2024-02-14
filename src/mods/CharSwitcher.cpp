#include "CharSwitcher.hpp"

bool CharSwitcher::mod_enabled{false};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

constexpr uintptr_t loadPlayerFiles = 0x00E552D0;

uintptr_t CharSwitcher::jmp_ret1{false};
constexpr uintptr_t detour1_jmp1 = 0x00494B70;

constexpr uintptr_t loadNeroArc_call1 = 0x008DF530;

constexpr uintptr_t loadDanteArc_call1 = 0x008DF530;

constexpr uintptr_t spawnNero_call1 = 0x007E1B30;
constexpr uintptr_t spawnNero_call2 = 0x008DC540;

constexpr uintptr_t spawnDante_call1 = 0x007B2150;
constexpr uintptr_t spawnDante_call2 = 0x008DC540; // same as nero

uintptr_t uPlAddr1    = 0;
uintptr_t uPlAddr2    = 0;
float swapGrav        = 0.0f;
uintptr_t tempAddr    = 0;
float ySpawn          = 0.0f;
uint16_t desiredInput = 0;
uint16_t prevInput    = 0;

constexpr uintptr_t detour2_mov1 = 0;

// 1. Always load Dante's save (enable out of mission to spawn Dante while playing as Nero, no effect on Nero spawn)
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

// 2. set uPlayer addresses
naked void SetUPlayerAddresses(void) {
    _asm {
            pushad
            mov ebp, [static_mediator_ptr]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x24]
            mov [uPlAddr2], ebp
            mov dword ptr [swapGrav], 0xbdcccccd // -0.1
            mov dword ptr [ySpawn], 0xc4fa0000 // -2000.0f
            popad
            ret
    }
}

// 3.a. Load Nero arc
naked void LoadNeroArc(void) {
    _asm {
        pushad
        mov eax, [loadPlayerFiles]
        mov eax, [eax] //
        push 0x00008002
        push 0x00B90CD0 // DevilMayCry4_DX9.exe+790CD0
        push 0x00EAD4A0 // DevilMayCry4_DX9.exe+AAD4A0
        call dword ptr [loadNeroArc_call1] // DevilMayCry4_DX9.exe+4DF530
        popad
        ret
    }
}

// 3.b. Load Dante arc
naked void LoadDanteArc(void) {
    _asm {
        pushad
        mov eax, [loadPlayerFiles]
        mov eax, [eax] //
        push 0x00008002     
        push 0x00B90CE8 // push DevilMayCry4_DX9.exe+790CD0
        push 0x00EAD4A0 // push DevilMayCry4_DX9.exe+AAD4A0
        call dword ptr [loadDanteArc_call1] // call DevilMayCry4_DX9.exe+4DF530
        popad
        ret
    }                   
}

// 4.a. Spawn Nero (load arc file first if not main actor)
naked void SpawnNero(void) {
    _asm {
        pushad
        mov ecx, [0x00E1434C]
        mov ecx, [ecx] //
        mov eax, [ecx]
        mov edx, [eax+0x14]
        push 0x10
        push 0x0000D680
        call edx // edx is 40003??
        mov [uPlAddr1], eax
        mov esi, eax
        call dword ptr [spawnNero_call1]
        mov esi, [uPlAddr1]
        mov eax, [0x00E552CC]
        mov eax, [eax] //
        push 0x0F
        call dword ptr [spawnNero_call2]
        popad
        ret
    }
}

// 4.b. Spawn Dante (load arc file first if not main actor)
naked void SpawnDante(void) {
    _asm {
        pushad
        mov ecx, [0x00E1434C]
        mov ecx, [ecx] //
        mov eax, [ecx]
        mov edx, [eax+0x14]
        push 0x10
        push 0x000152F0
        call edx
        mov [uPlAddr1], eax
        call dword ptr [spawnDante_call1]
        mov esi, [uPlAddr1]
        mov eax, [0x00E552CC]
        mov eax, [eax] //
        push 0x0F
        call dword ptr [spawnDante_call2]
        popad
        ret
    }
}

// swap actor
naked void SwapActor(void) {
    _asm {
        pushad
        mov eax, [uPlAddr1]
        mov ecx, [uPlAddr2]
        mov ebp, [static_mediator_ptr]
        mov ebp, [ebp]
        mov [ebp+0x24], ecx

        // To-be main actor
        mov [uPlAddr1], ecx
        mov dword ptr [ecx+0x1509], 1 //input

        // Position, rotation
        fld [eax+0x30] // X pos
        fstp [ecx+0x30]
        fld [eax+0x34] // Y pos
        fstp [ecx+0x34]
        fld [eax+0x38] // Z pos
        fstp [ecx+0x38]
        fld [ecx+0x30]
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
        //  mov edi,[eax+0x1E8C]
        mov esi,[ecx+0x1E8C]
        mov [esi+0xD4], 0
        mov [esi+0x1C], 0

        // Inertia, gravity
        xorps xmm0, xmm0
        movss [ecx+0xEC4], xmm0
        movss xmm0, [swapGrav]
        movss [ecx+0xED4], xmm0
        //  fld [eax+0xED4]
        //  fstp [ecx+0xED4]
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
        mov [ecx+0x1504], 0
        mov [ecx+0x1550], 01
        mov edx,[ecx+0x1554]
        or edx, 0x02
        mov [ecx+0x1554], edx

        // Previous main actor
        mov [uPlAddr2], eax
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
        //  mov [esi+0x1C], 0
        mov [eax+0x1504], 5
        //  mov [eax+0x150C], 0
        //  mov [eax+0x150D], 1
        xorps xmm0, xmm0
        movss [eax+0x1E1C], xmm0
        movss [eax+0xEC4], xmm0
        movss [eax+0xED0], xmm0
        movss [eax+0xED4], xmm0
        movss [eax+0xED8], xmm0
        popad
        ret
    }
}

std::optional<std::string> CharSwitcher::on_initialize() {
    if (!install_hook_offset(0x94B55, hook1, &detour1, &jmp_ret1, 8)) {
        spdlog::error("Failed to init CharSwitcher mod\n");
        return "Failed to init CharSwitcher mod";
    }
    return Mod::on_initialize();
}

void CharSwitcher::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            if (player->buttonInputRaw != prevInput && player->buttonInputRaw & desiredInput) {
                SwapActor();
            }
            prevInput = player->buttonInputRaw;
        }
    }
}

void CharSwitcher::on_gui_frame() { 
    ImGui::Checkbox("Char Switcher", &mod_enabled);
    ImGui::SameLine();
    help_marker("Enable before loading into a stage");

    if (ImGui::Button("2. set uPlayer addresses")) {
        SetUPlayerAddresses();
    }
    ImGui::SameLine();
    help_marker("Click after loading into a stage");

    if (ImGui::Button("3.a. Load Nero arc")) {
        LoadNeroArc();
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Button("3.b. Load Dante arc")) {
        LoadDanteArc();
    }
    if (ImGui::Button("4.a. Spawn Nero")) {
        SpawnNero();
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Button("4.b. Spawn Dante")) {
        SpawnDante();
    }
    if (ImGui::Button("swap actor")) {
        SwapActor();
    }
    ImGui::SameLine(sameLineWidth);
    ImGui::PushItemWidth(sameLineItemWidth);
    if (ImGui::BeginCombo("Char Swap Input", devil4_sdk::getButtonInfo(desiredInput).second)) {
        for (const auto& buttonPair : buttonPairs) {
            bool is_selected = (desiredInput == buttonPair.first);
            if (ImGui::Selectable(buttonPair.second, is_selected)) {
                desiredInput = buttonPair.first;
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
    mod_enabled = cfg.get<bool>("char_switcher").value_or(true);
    desiredInput = cfg.get<int16_t>("char_swap_input").value_or(0x1); // Select default
}

void CharSwitcher::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("char_switcher", mod_enabled);
    cfg.set<int16_t>("char_swap_input", desiredInput);
}
