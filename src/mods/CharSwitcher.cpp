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

uintptr_t uPlAddr1 = 0;
uintptr_t uPlAddr2 = 0;
float swapGrav     = 0.0f;
uintptr_t tempAddr = 0;

constexpr uintptr_t detour2_mov1 = 0;

// 1. Always load Dante's save (enable out of mission to spawn Dante while playing as Nero, no effect on Nero spawn)
naked void detour1(void) {
    _asm {
			cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            jmp jnecode

		originalcode:
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
            mov [swapGrav], 0xbdcccccd // -0.1
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
        mov dword ptr [ecx+0x1509], 1

        // Collision
        mov esi, [ecx+0x1E8C]
        mov dword ptr [esi+0xD4], 0
        mov dword ptr [esi+0x1C], 1

        fld [eax+0x30] // X pos
        fstp [ecx+0x30]
        fld [eax+0x34] // Y pos
        fstp [ecx+0x34]
        fld [eax+0x38] // Z pos
        fstp [ecx+0x38]
        fld [eax+0x44] // facing direction
        fstp [ecx+0x44]

        // Inertia, gravity
        fld [eax+0xEC4]
        fstp [ecx+0xEC4]
        movss xmm0, [swapGrav]
        movss [ecx+0xED4], xmm0
        fld [eax+0x1E18]
        fstp [ecx+0x1E18]
        fld [eax+0x1E1C] // inertia
        fstp [ecx+0x1E1C]
        fld [eax+0x1E30] // X-axis inertia
        fstp [ecx+0x1E30]
        fld [eax+0x1E38] // Z-axis inertia
        fstp [ecx+0x1E38]

        // Previous main actor
        mov [uPlAddr2],eax
        mov dword ptr [eax+0x1509], 0 // input
        mov esi, [eax+0x1E8C]
        mov dword ptr [esi+0xD4], 1 // collision

        xorps xmm0,xmm0
        movss [eax+0xEC4], xmm0
        movss [eax+0xED0], xmm0
        movss [eax+0xED4], xmm0
        movss [eax+0xED8], xmm0
        movss [eax+0x1E1C], xmm0
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
    if (ImGui::Button("3.b. Load Dante arc")) {
        LoadDanteArc();
    }
    if (ImGui::Button("4.a. Spawn Nero (load arc file first if not main actor)")) {
        SpawnNero();
    }
    if (ImGui::Button("4.b. Spawn Dante(load arc file first if not main actor)")) {
        SpawnDante();
    }
    if (ImGui::Button("swap actor")) {
        SwapActor();
    }
}

void CharSwitcher::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("char_switcher").value_or(true);
}

void CharSwitcher::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("char_switcher", mod_enabled);
}
