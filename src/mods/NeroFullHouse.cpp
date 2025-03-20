#include "NeroFullHouse.hpp"
#include "misc/kAtckDefTbl.cpp"
#include "SDK/ReClass.hpp"
#include "MoveTable.hpp"
//#include "MoveIDsNero.hpp"

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
uintptr_t NeroFullHouse::jmp_ret6{ NULL };
uintptr_t NeroFullHouse::jmp_ret7{ NULL };
uintptr_t NeroFullHouse::jmp_ret8{ NULL };
uintptr_t NeroFullHouse::jmp_ret9{ NULL };
uintptr_t NeroFullHouse::jmp_ret10{ NULL };

static float payline_loop_frame = 8.0f; // landing
static float payline_bounce = 10.0f; // how much bounce
//static float payline_startup_frame = 10.0f; // using loop frame
static float payline_buffer_frame = 25.0f;
static float payline_recovery_frame = 35.0f;
static float payline_can_walk_frame = 55.0f;
static float detour6_float = 20.0f;
constexpr uintptr_t effect_call = 0x480480;
// DevilMayCry4_DX9.exe + 3B0300 - movss[esi+08] // xmm0 left + right stick rotation
// DevilMayCry4_DX9.exe + 5B8C - mov[esp+10], DevilMayCry4_DX9.exe + 790C18 // "PlayerID"
// player + 30C4 = buffers (from DevilMayCry4_DX9.exe+3AB8C8 - mov [ecx+esi+000030C4], 00000001)
// 
// DevilMayCry4_DX9.exe + 2B963 - movss xmm1, [edx+18] references frame of animation for cancel frames
// DevilMayCry4_DX9.exe + 5FD40A - movss xmm0, [esi+18] references frame of animation for hitbox timing
// DevilMayCry4_DX9.exe+5FCF53 - movss xmm1,[esi+18] references frame of animation to know when to restart loop anims
// DevilMayCry4_DX9.exe + 5FBA28 - movss xmm5, [eax+18] possible twitch mode, disables animations but motion functions normally

// DevilMayCry4_DX9.exe+3AC911 accesses move ids, call after it writes player+1564 for switch
// DevilMayCry4_DX9.exe+403C2D bans new move ids


// DevilMayCry4_DX9.exe+3AC911 accesses moveids for directionals
// DevilMayCry4_DX9.exe+403CAA might be better

// void NeroFullHouse::toggle(bool enable) {
//     if (enable) {
//         install_patch_offset(0x3D33D9, patch1, "\x090\x90\x90\x090\x90\x90\x90\x90", 8);//Move class aerial lock
//     }
//     else {
//         patch1.reset();
//     }
// }

naked void detour1(void) { // redirect streak 1 to full house function // player in eax
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            je fullhousecheck
        originalcode:
            jmp dword ptr [NeroFullHouse::nero_streak_1]
        fullhousecheck:
            cmp byte ptr [eax+0x1564], 28 // was streak 1 pushed?
            jne originalcode
			jmp dword ptr [NeroFullHouse::full_house]
    }
}

naked void detour2(void) { // select full house start animation // player in esi
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp byte ptr [esi+0x1494], 1 // nero
            jne code

        // cheatcode:
            // movss [backup],xmm0 // turns out this was not an issue :)
            // movss xmm0,[payline_bounce]
            // movss [esi+0xEC4],xmm0
            // movss xmm0,[backup]
            fld [payline_bounce]
            fstp [esi+0xEC4]
            push 0x00000352 // payline
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
            cmp byte ptr [esi+0x1494], 1 // nero
            jne code
            mov byte ptr [esi+0x1D7F],1 // exceedable
        // cheatcode:
            push 0x00000351 // 20 is shmove right, 351/2/3 is payline
            jmp dword ptr [NeroFullHouse::jmp_ret3]
        code:
            push 0x0000030D // full house landing
			jmp dword ptr [NeroFullHouse::jmp_ret3]
    }
}

naked void detour4(void) { // set frame of animation, player in ecx, this should not be here
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp byte ptr [ecx+0x1494], 1 // nero
            jne code

            push eax
            mov eax, [static_mediator_ptr]
            mov eax, [eax]
            mov eax, [eax+0x24]
            cmp ecx, eax
            pop eax
            jne code

        // payline loop - loop frame 10 while aerial
        // nerotest1:
            cmp dword ptr [ecx+0x2998], 0x352 // was 0x32C but we use payline anim now!
            jne nerotest2
            cmp byte ptr [ecx+0x2008], 1 // grounded
            je code
            comiss xmm0, [payline_loop_frame]
            jb code
            movss xmm0, [payline_loop_frame] 
            jmp code

        // payline ending, start at same frame as the loop for smooth transition
        nerotest2:
            cmp dword ptr [ecx+0x2998], 0x351
            jne code
            comiss xmm0, [payline_loop_frame]
            ja code
            movss xmm0, [payline_loop_frame]
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
            cmp byte ptr [esi+0x1494], 1 // nero
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
            cmp ecx, 0x0000FFFF
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

naked void detour6(void) { // how many frames to wait before starting loop
    _asm {
        cmp byte ptr [NeroFullHouse::mod_enabled], 1
        jne code
        cmp byte ptr [ebx+0x1494], 1 // nero
        jne code
        comiss xmm0, [payline_loop_frame]
        jmp [NeroFullHouse::jmp_ret6]
        // next instruction is jb so comiss loop frame is ok
    code:
        comiss xmm0, [detour6_float]
        jmp [NeroFullHouse::jmp_ret6]
    }
}

static void __stdcall exceed_handling(uPlayer& neroBase) {
    if (neroBase.animID == 0x351) {
        if ((neroBase.animFrame > payline_loop_frame + 2.0f) && (neroBase.animFrame < payline_loop_frame + 4.0f)) {
            neroBase.canExceed = 1;
        } else if (neroBase.animFrame > 24.0f) {
            neroBase.canExceed = 2;
        }
    }
}

naked void detour7(void) { // cancellable payline ending    
    _asm {
        cmp byte ptr [NeroFullHouse::mod_enabled], 1
        jne code
        cmp byte ptr [ebx+0x1494], 1 // nero
        jne code

        push ebx
        call exceed_handling

        movss xmm0, [ebx+0x348]
        comiss xmm0, [payline_buffer_frame]
        jb code

        mov byte ptr [ebx+0x3174], 1 // movement abilities cancel
        mov byte ptr [ebx+0x30C4], 1 // melee cancel
        mov byte ptr [ebx+0x31CC], 1 // gun cancel
        mov byte ptr [ebx+0x3148], 1 // directional melee cancel
        mov byte ptr [ebx+0x30F0], 1 // can melee cancel, again

        comiss xmm0, [payline_recovery_frame]
        jb code

        mov byte ptr [ebx+0x3174], 2 // movement abilities cancel
        mov byte ptr [ebx+0x30C4], 2 // melee cancel
        mov byte ptr [ebx+0x31CC], 2 // gun cancel
        mov byte ptr [ebx+0x3148], 2 // directional melee cancel
        mov byte ptr [ebx+0x30F0], 2 // can melee cancel, again

        comiss xmm0, [payline_can_walk_frame]
        jb code

        mov byte ptr [ebx+0x31F8],2 // can walk cancel
    code:
        cmp byte ptr [ebx+0x00002A54], 01
        jmp [NeroFullHouse::jmp_ret7]
    }
}

naked void detour8(void) { // cancellable payline ending 
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne code
            cmp byte ptr [ebx+0x1494], 1 // nero
            jne code

            jmp [NeroFullHouse::jmp_ret8]

        code:
            movss [ebx+0xEC4],xmm0
            jmp [NeroFullHouse::jmp_ret8]

    }
}

naked void detour9(void) { // looped effect
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne handler
            cmp byte ptr [ebx+0x1494], 1 // nero
            jne handler

            mov edx, 0x12
            mov eax, 0x01 // 1-Nero efx, 2-Dante efx

            cmp eax, [ebx+0xCCE8]
            ja originalcode

            mov edx,0x4D
        originalcode:
            call effect_call
            jmp [NeroFullHouse::jmp_ret9]
        handler:
            lea edx, [esi+0x3E]
            lea eax, [esi-0x02]
            jmp originalcode
    }
}

naked void detour10(void) { // landing effect
    _asm {
            cmp byte ptr [NeroFullHouse::mod_enabled], 1
            jne handler
            cmp byte ptr [ebx+0x1494], 1 // nero
            jne handler

            mov edx, 0x25
            mov eax, 0x01               // 1-Nero efx, 2-Dante efx

            cmp eax, [ebx+0xCCE8]
            ja originalcode

            mov edx,0x31
        originalcode:
            jmp [NeroFullHouse::jmp_ret10]
        handler:
            lea edx, [esi+0x3F]
            lea eax, [esi-0x02]
            jmp originalcode
    }
}

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
    if (!install_hook_offset(0x3D3404, hook6, &detour6, &jmp_ret6, 7)) { // how many frames to wait before starting loop
        spdlog::error("Failed to init NeroFullHouse6 mod\n");
        return "Failed to init NeroFullHouse6 mod";
    }
    if (!install_hook_offset(0x3D3665, hook7, &detour7, &jmp_ret7, 7)) { // cancellable payline ending 
        spdlog::error("Failed to init NeroFullHouse7 mod\n");
        return "Failed to init NeroFullHouse7 mod";
    }
    if (!install_hook_offset(0x3D33D9, hook8, &detour8, &jmp_ret8, 8)) { // cancellable payline ending 
        spdlog::error("Failed to init NeroFullHouse8 mod\n");
        return "Failed to init NeroFullHouse8 mod";
    }
    if (!install_hook_offset(0x3D3369, hook9, &detour9, &jmp_ret9, 8)) { // looped effect 
        spdlog::error("Failed to init NeroFullHouse9 mod\n");
        return "Failed to init NeroFullHouse9 mod";
    }
    if (!install_hook_offset(0x3D35C6, hook10, &detour10, &jmp_ret10, 6)) { // landing effect
        spdlog::error("Failed to init NeroFullHouse10 mod\n");
        return "Failed to init NeroFullHouse10 mod";
    }
    return Mod::on_initialize();
}

void NeroFullHouse::on_gui_frame() {
    kAtckDefTbl* NeroAtkTbl     = (kAtckDefTbl*)HookNeroKADTbl;
    kAtckDefTbl* payline_params = &NeroAtkTbl[6];
    kAtckDefTbl* hb_params      = &NeroAtkTbl[21];
    kAtckDefTbl* dd_params      = &NeroAtkTbl[22];
    if (ImGui::Checkbox(_("Payline"), &mod_enabled)) {
       // toggle(mod_enabled);
        if (mod_enabled) {
            helm_splitter_remap = true;
            payline_params->atckAs = 2;
            // *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can be used in air
            hb_params->command.buffer = LOCKON_BACK_MELEE;
            dd_params->command.buffer = LOCKON_BACK_MELEE;
            // *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE; // back
            // *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;   // back
        }
        else {
            helm_splitter_remap = false;
            // *(uintptr_t*)0xC3EFB0 = 1; // streak 1 can only be used when grounded
            // *(uintptr_t*)helm_splitter_directional = LOCKON_FORWARD_MELEE2;
            // *(uintptr_t*)double_down_directional   = LOCKON_FORWARD_MELEE2;
            payline_params->atckAs                    = 1;
            hb_params->command.buffer              = LOCKON_FORWARD_MELEE2;
            dd_params->command.buffer              = LOCKON_FORWARD_MELEE2;
        }
    }
    ImGui::SameLine();
    help_marker(_("Gives Nero a divekick action like in DMC5"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Remap Helm Splitter"), &helm_splitter_remap)) {
        if (helm_splitter_remap) {
            // *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE;
            // *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;
            hb_params->command.buffer              = LOCKON_BACK_MELEE;
            dd_params->command.buffer              = LOCKON_BACK_MELEE;
        }
        else {
            // *(uintptr_t*)helm_splitter_directional = LOCKON_FORWARD_MELEE2;
            // *(uintptr_t*)double_down_directional = LOCKON_FORWARD_MELEE2;
            hb_params->command.buffer = LOCKON_FORWARD_MELEE2;
            dd_params->command.buffer = LOCKON_FORWARD_MELEE2;
        }
    }
    ImGui::SameLine();
    help_marker(_("Remap Helm Splitter and Double Down to lockon+back+melee\nRequires \"Move Table\" at the bottom of the Debug page, which applies at the start of a level"));
}

void NeroFullHouse::on_config_load(const utility::Config& cfg) {
    kAtckDefTbl* NeroAtkTbl     = (kAtckDefTbl*)HookNeroKADTbl;
    kAtckDefTbl* payline_params = &NeroAtkTbl[6];
    kAtckDefTbl* hb_params      = &NeroAtkTbl[21];
    kAtckDefTbl* dd_params      = &NeroAtkTbl[22];
    mod_enabled = cfg.get<bool>("nero_full_house").value_or(false);
    // toggle(mod_enabled);
    helm_splitter_remap = cfg.get<bool>("helm_splitter_remap").value_or(false);
    if (mod_enabled) {
        helm_splitter_remap = true;
        // *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can only be used when aerial
        payline_params->atckAs = 2;
    }
    if (helm_splitter_remap) {
        // *(uintptr_t*)helm_splitter_directional = LOCKON_BACK_MELEE;
        // *(uintptr_t*)double_down_directional = LOCKON_BACK_MELEE;

        hb_params->command.buffer = LOCKON_BACK_MELEE;
        dd_params->command.buffer = LOCKON_BACK_MELEE;
    }
};

void NeroFullHouse::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_full_house", mod_enabled);
    cfg.set<bool>("helm_splitter_remap", helm_splitter_remap);
};

#endif
