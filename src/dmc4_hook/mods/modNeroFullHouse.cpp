#include "../mods.h"
#include "modNeroFullHouse.hpp"
#include "modMoveIDsNero.hpp"

#if 1
bool NeroFullHouse::modEnabled{ false };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
uintptr_t NeroFullHouse::full_house{ 0x007D3300 };
uintptr_t NeroFullHouse::nero_streak_1{ 0x007FDA80 };
uintptr_t NeroFullHouse::helmSplitterDirectional{ 0x00C3F320 };
uintptr_t NeroFullHouse::doubleDownDirectional{ 0x00C3F2E8 };

// jmp to double down is DevilMayCry4_DX9.exe+3F9DC7 jmp DevilMayCry4_DX9.exe+400810
// call to rave is DevilMayCry4_DX9.exe+3F9D62 call DevilMayCry4_DX9.exe+3FCF10
// move switch is DevilMayCry4_DX9.exe+3F9CE4 jmp dword ptr [ecx*4+007F9DE0]

uintptr_t NeroFullHouse::jmp_ret1{ NULL };
uintptr_t NeroFullHouse::jmp_ret2{ NULL };
uintptr_t NeroFullHouse::jmp_ret3{ NULL };
uintptr_t NeroFullHouse::jmp_ret4{ NULL };
uintptr_t NeroFullHouse::jmp_ret5{ NULL };
uintptr_t NeroFullHouse::jmp_jne5{ 0X0042BA72 };

float currentFrame = 0.0f;

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


naked void detour1(void) { // redirect streak 1 to full house function
    _asm {
            cmp byte ptr [NeroFullHouse::modEnabled], 1
            je fullhousecheck
        originalcode:
            jmp dword ptr [NeroFullHouse::nero_streak_1]
        fullhousecheck:
            cmp [eax+0x1564], 28
            jne originalcode
			jmp dword ptr [NeroFullHouse::full_house]
    }
}

naked void detour2(void) { // select full house start animation
    _asm {
            cmp byte ptr [NeroFullHouse::modEnabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
        //cheatcode:
            // push 0x00000351
            push 0x0000032C
            jmp dword ptr [NeroFullHouse::jmp_ret2]
        code:
            push 0x0000030C
			jmp dword ptr [NeroFullHouse::jmp_ret2]
    }
}

naked void detour3(void) { // select full house landing animation
    _asm {
            cmp byte ptr [NeroFullHouse::modEnabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
        //cheatcode:
            push 0x00000351 // 20 is shmove right, 351/2/3 is payline
            mov byte ptr [esi+0x1E15], 0x10 // jump cancellable
            jmp dword ptr [NeroFullHouse::jmp_ret3]
        code:
            push 0x0000030D
			jmp dword ptr [NeroFullHouse::jmp_ret3]
    }
}

float firstFrame = 24.0f;
float firstFrame2 = 30.0f;
// 20 - 25 hits quickly
// 22 - 25 hits but has dumb leg
// 23 - 25 hits and has less dumb leg
// 24 - 25 seems good

naked void detour4(void) { // set frame of animation, player in ecx
    _asm {
            cmp byte ptr [NeroFullHouse::modEnabled], 1
            jne code
            cmp [ecx+0x1494], 1 // nero
            jne code

            push eax
            mov eax, [staticMediatorPtr]
            mov eax, [eax]
            mov eax, [eax+0x24]
            cmp ecx, eax
            pop eax
            jne code

        // payline loop
        // test1:
            cmp dword ptr [MoveIdsNero::moveIDNero], 0x32C
            jne test2
            cmp byte ptr [ecx+0x2008], 1 // grounded
            je code
            movss [currentFrame], xmm0
            cmp [currentFrame], 0x41c80000 // 25.0f
            jl nerocode
            movss xmm0, [firstFrame]
            jmp nerocode

        // payline ending
        test2:
            cmp dword ptr [MoveIdsNero::moveIDNero], 0x341
            jne nerocode
            movss [currentFrame], xmm0
            cmp [currentFrame], 0x41f00000         // 30.0f
            ja nerocode
            movss xmm0, [firstFrame2]
            jmp nerocode

        nerocode:
            movss [esi+0x18], xmm0
            jmp dword ptr [NeroFullHouse::jmp_ret4]
        code:
            movss [esi+0x18], xmm0
			jmp dword ptr [NeroFullHouse::jmp_ret4]
    }
}

naked void detour5(void) { // make payline ending cancellable
    _asm {
            cmp byte ptr [NeroFullHouse::modEnabled], 1
            jne code
            cmp [esi+0x1494], 1 // nero
            jne code
            push eax
            mov eax, [staticMediatorPtr]
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



std::optional<std::string> NeroFullHouse::onInitialize() {
    if (!install_hook_offset(0x03F9D82, hook1, &detour1, &jmp_ret1, 5)) { // streak
        HL_LOG_ERR("Failed to init NeroFullHouse1 mod\n");
        return "Failed to init NeroFullHouse1 mod";
    }

    if (!install_hook_offset(0x03D3341, hook2, &detour2, &jmp_ret2, 5)) {
        HL_LOG_ERR("Failed to init NeroFullHouse2 mod\n");
        return "Failed to init NeroFullHouse2 mod";
    }
    
    if (!install_hook_offset(0x03D358B, hook3, &detour3, &jmp_ret3, 5)) {
        HL_LOG_ERR("Failed to init NeroFullHouse3 mod\n");
        return "Failed to init NeroFullHouse3 mod";
    }
    
    if (!install_hook_offset(0x05FCF43, hook4, &detour4, &jmp_ret4, 5)) {
        HL_LOG_ERR("Failed to init NeroFullHouse4 mod\n");
        return "Failed to init NeroFullHouse4 mod";
    }

    if (!install_hook_offset(0x002BA57, hook5, &detour5, &jmp_ret5, 6)) {
        HL_LOG_ERR("Failed to init NeroFullHouse5 mod\n");
        return "Failed to init NeroFullHouse5 mod";
    }

    return Mod::onInitialize();
}

void NeroFullHouse::onGUIframe() {
    if (ImGui::Checkbox("Nero Full House", &modEnabled)) {
        if (modEnabled) {
            *(uintptr_t*)helmSplitterDirectional = 4;
            *(uintptr_t*)doubleDownDirectional = 4;
            *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can be used in air
        }
        else {
            *(uintptr_t*)helmSplitterDirectional = 9;
            *(uintptr_t*)doubleDownDirectional = 9;
            *(uintptr_t*)0xC3EFB0 = 1; // streak 1 can be used on ground
        }
    }
    ImGui::SameLine();
    HelpMarker("Very work in progress");
}

void NeroFullHouse::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("nero_full_house").value_or(false);
    if (modEnabled) {
        *(uintptr_t*)helmSplitterDirectional = 4;
        *(uintptr_t*)doubleDownDirectional = 4;
        *(uintptr_t*)0xC3EFB0 = 2; // streak 1 can be used in air 
    }
};

void NeroFullHouse::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("nero_full_house", modEnabled);
};

#endif
