
#include "NeroFullHouse.hpp"
#include "MoveIDsNero.hpp"

#if 1
bool NeroFullHouse::mod_enabled{ false };
bool NeroFullHouse::helm_splitter_remap{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
uintptr_t NeroFullHouse::full_house{ 0x007D3300 };
uintptr_t NeroFullHouse::nero_streak_1{ 0x007FDA80 };
uintptr_t NeroFullHouse::helm_splitter_directional{ 0x00C3F320 };
uintptr_t NeroFullHouse::double_down_directional{ 0x00C3F2E8 };

// jmp to double down is DevilMayCry4_DX9.exe+3F9DC7 jmp DevilMayCry4_DX9.exe+400810
// call to rave is DevilMayCry4_DX9.exe+3F9D62 call DevilMayCry4_DX9.exe+3FCF10
// move switch is DevilMayCry4_DX9.exe+3F9CE4 jmp dword ptr [ecx*4+007F9DE0]

uintptr_t NeroFullHouse::jmp_ret1{ NULL };
uintptr_t NeroFullHouse::jmp_ret2{ NULL };
uintptr_t NeroFullHouse::jmp_ret3{ NULL };
uintptr_t NeroFullHouse::jmp_ret4{ NULL };
uintptr_t NeroFullHouse::jmp_ret5{ NULL };
uintptr_t NeroFullHouse::jmp_jne5{ 0X0042BA72 };

static float current_frame = 0.0f;

// DevilMayCry4_DX9.exe + 3B0300 - movss[esi+08] // xmm0 left + right stick rotation
// DevilMayCry4_DX9.exe + 5B8C - mov[esp+10], DevilMayCry4_DX9.exe + 790C18 // "PlayerID"
// player + 30C4 = buffers (from DevilMayCry4_DX9.exe+3AB8C8 - mov [ecx+esi+000030C4],00000001)
// 
// DevilMayCry4_DX9.exe + 2B963 - movss xmm1, [edx+18] references frame of animation for cancel frames
// DevilMayCry4_DX9.exe + 5FD40A - movss xmm0, [esi+18] references frame of animation for hitbox timing
// DevilMayCry4_DX9.exe+5FCF53 - movss xmm1,[esi+18] references frame of animation to know when to restart loop anims
// DevilMayCry4_DX9.exe + 5FBA28 - movss xmm5, [eax+18] possible twitch mode, disables animations but motion functions normally

// DevilMayCry4_DX9.exe+3AC911 accesses move ids, call after it writes player+1564 for switch
// DevilMayCry4_DX9.exe+403C2D bans new move ids


// DevilMayCry4_DX9.exe+3AC911 accesses moveids for directionals
// DevilMayCry4_DX9.exe+403CAA might be better



naked void detour1(void) { // redirect streak 1 to full house function // player in eax
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            je fullhousecheck
        originalcode:
            jmp dword ptr [NeroFullHouse::nero_streak_1]
        fullhousecheck:
            cmp [eax+0x1564], 28 // was streak 1 pushed?
            jne originalcode
			jmp dword ptr [NeroFullHouse::full_house]
    }
}

naked void detour2(void) { // select full house start animation // player in esi
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
        //cheatcode:
            // push 0x00000351 // payline
            push 0x0000032C // double down
            jmp dword ptr [NeroFullHouse::jmp_ret2]
        code:
            push 0x0000030C // full house
			jmp dword ptr [NeroFullHouse::jmp_ret2]
    }
}

naked void detour3(void) { // select full house landing animation // player in esi
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
        //cheatcode:
            push 0x00000351 // 20 is shmove right, 351/2/3 is payline
            jmp dword ptr [NeroFullHouse::jmp_ret3]
        code:
            push 0x0000030D // full house landing
			jmp dword ptr [NeroFullHouse::jmp_ret3]
    }
}

float first_frame = 24.0f; // aerial
float first_frame2 = 10.0f; // landing
// 20 - 25 hits quickly
// 22 - 25 hits but has dumb leg
// 23 - 25 hits and has less dumb leg
// 24 - 25 seems good

naked void detour4(void) { // set frame of animation, player in ecx, this should not be here
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp [ecx+0x1494], 1 // nero
            jne code

            push eax
            mov eax, [static_mediator_ptr]
            mov eax, [eax]
            mov eax, [eax+0x24]
            cmp ecx, eax
            pop eax
            jne code

        // payline loop - loop frames 24-25 while aerial
        // nerotest1:
            cmp dword ptr [ecx+0x2998], 0x32C
            jne nerotest2
            cmp byte ptr [ecx+0x2008], 1 // grounded
            je code
            movss [current_frame], xmm0
            cmp [current_frame], 0x41c80000 // 25.0f
            jl code
            movss xmm0, [first_frame] // 24.0f
            jmp code

        // payline ending, start at frame 10
        nerotest2:
            cmp dword ptr [ecx+0x2998], 0x351
            jne code
            movss [current_frame], xmm0
            cmp [current_frame], 0x41200000 // 10.0f
            ja code
            movss xmm0, [first_frame2] // 10.0f
            jmp code

        code:
            movss [esi+0x18], xmm0
			jmp dword ptr [NeroFullHouse::jmp_ret4]
    }
}

naked void detour5(void) { // cancellable payline ending // player in esi
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
            push eax
            mov eax, [static_mediator_ptr]
            mov eax, [eax]
            mov eax, [eax+0x24]
            cmp esi, eax
            pop eax
            jne code

            cmp ecx, 0x351
            je retcode

        code:
            cmp ecx,0x0000FFFF
            jne jnecode
			jmp dword ptr [NeroFullHouse::jmp_ret5]
        jnecode:
            jmp dword ptr [NeroFullHouse::jmp_jne5]
        retcode:
            ret
    }
}

// sounds accessed by DevilMayCry4_DX9.exe+5FD50D:
// 1024 = sound effect for part 1
// 320 = sound effect for part 2


std::optional<std::string> NeroFullHouse::on_initialize() {
    if (!install_hook_offset(0x03F9D82, hook1, &detour1, &jmp_ret1, 5)) { // streak
        spdlog::error("Failed to init NeroFullHouse1 mod\n");
        return "Failed to init NeroFullHouse1 mod";
    }

    if (!install_hook_offset(0x03D3341, hook2, &detour2, &jmp_ret2, 5)) { // full house start animation
        spdlog::error("Failed to init NeroFullHouse2 mod\n");
        return "Failed to init NeroFullHouse2 mod";
    }
    
    if (!install_hook_offset(0x03D358B, hook3, &detour3, &jmp_ret3, 5)) { // full house end animation
        spdlog::error("Failed to init NeroFullHouse3 mod\n");
        return "Failed to init NeroFullHouse3 mod";
    }
    
    if (!install_hook_offset(0x05FCF43, hook4, &detour4, &jmp_ret4, 5)) { // set frame of animation
        spdlog::error("Failed to init NeroFullHouse4 mod\n");
        return "Failed to init NeroFullHouse4 mod";
    }

    if (!install_hook_offset(0x002BA57, hook5, &detour5, &jmp_ret5, 6)) { // cancellable payline ending 
        spdlog::error("Failed to init NeroFullHouse5 mod\n");
        return "Failed to init NeroFullHouse5 mod";
    }

    return Mod::on_initialize();
}

void NeroFullHouse::on_gui_frame() {
    if (ImGui::Checkbox("Payline", &mod_enabled)) {
        if (mod_enabled) {
            helm_splitter_remap = true;
            *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can be used in air
            *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE; // back
            *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;   // back
        }
        else {
            helm_splitter_remap = false;
            *(uintptr_t*)0xC3EFB0 = 1; // streak 1 can only be used when grounded
        }
    }
    ImGui::SameLine();
    help_marker("Very work in progress");
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Remap Helm Splitter", &helm_splitter_remap)) {
        if (helm_splitter_remap) {
            *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE;
            *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;
        }
        else {
            *(uintptr_t*)helm_splitter_directional = LOCKON_FORWARD_MELEE2;
            *(uintptr_t*)double_down_directional = LOCKON_FORWARD_MELEE2;
        }
    }
    ImGui::SameLine();
    help_marker("Remap Helm Splitter and Double Down to lockon+back+melee");
}

void NeroFullHouse::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("nero_full_house").value_or(false);
    helm_splitter_remap = cfg.get<bool>("helm_splitter_remap").value_or(false);
    if (mod_enabled) {
        helm_splitter_remap = true;
        *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can only be used when aerial
    }
    if (helm_splitter_remap) {
        *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE;
        *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;
    }
};

void NeroFullHouse::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_full_house", mod_enabled);
    cfg.set<bool>("helm_splitter_remap", helm_splitter_remap);
};

#endif
