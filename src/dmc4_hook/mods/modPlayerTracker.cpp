#include "modPlayerTracker.hpp"
#if 1

uintptr_t PlayerTracker::jmp_return{ NULL };
// uintptr_t* PlayerTracker::player_base_ptr{ (uintptr_t*)(0x00E558B8) }; // DevilMayCry4_DX9.exe+A558B8
uPlayer* PlayerTracker::player_ptr{ NULL };
bool PlayerTracker::lockOnAlloc{ false };
float* playerXYZ[3]{ NULL, NULL, NULL };
float* playerRotation[4]{ NULL, NULL, NULL, NULL };
float* playerScale[3]{ NULL, NULL, NULL };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

PlayerTracker::PlayerTracker()
{
    // onInitialize();
}

void update_player_info(void) {
    // sMediator* b = (sMediator*)*PlayerTracker::player_base_ptr;

    sMediator* sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;

    PlayerTracker::player_ptr = uLocalPlr; // used for valid check
    playerXYZ[0] = &uLocalPlr->mPos[0];
    playerXYZ[1] = &uLocalPlr->mPos[1];
    playerXYZ[2] = &uLocalPlr->mPos[2];

    playerRotation[0] = &uLocalPlr->mQuat[0];
    playerRotation[1] = &uLocalPlr->mQuat[1];
    playerRotation[2] = &uLocalPlr->mQuat[2];
    playerRotation[3] = &uLocalPlr->mQuat[3];

    playerScale[0] = &uLocalPlr->mScale[0];
    playerScale[1] = &uLocalPlr->mScale[1];
    playerScale[2] = &uLocalPlr->mScale[2];

    PlayerTracker::lockOnAlloc = uLocalPlr->lockontoggle;
}

naked void detour()
{
    _asm {
            movss [esi+0x30], xmm3 // originalcode
            push ecx
            push edx
            mov ecx, [staticMediatorPtr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            cmp esi, ecx
            //je manualplayer
            pop edx
            pop ecx
            je manualplayer
            jmp jmpret

        manualplayer:
            //pushad
            call update_player_info
            //popad
            //pop edx
            //pop ecx
        jmpret:
		    jmp dword ptr [PlayerTracker::jmp_return]
    }
}

std::optional<std::string> PlayerTracker::onInitialize()
{
    if (!install_hook_offset(0x3A88A1, hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init PlayerTracker mod\n");
        return "Failed to init PlayerTracker mod";
    }

    return Mod::onInitialize();
}


void PlayerTracker::onGUIframe()
{
    if (PlayerTracker::player_ptr != NULL)
    {
        ImGui::InputFloat3("Player Position", *playerXYZ);
        ImGui::InputFloat4("Player Rotation", *playerRotation);
        ImGui::InputFloat3("Player Scale", *playerScale);
        ImGui::Checkbox("Lock on", &PlayerTracker::lockOnAlloc);
    }
    else
    {
        ImGui::Text("Load into an area and debug info might pop up.");
    }
}
#endif
/*
            // sub esp, 0x10
            // movss [esp], xmm3
            // movss [esp+0x4], xmm0
            // movss [esp+0x8], xmm1
            // etc
            // movss xmm3, [esp]
            // movss xmm0, [esp+0x4]
            // movss xmm1, [esp+0x8]
            // add esp, 0x10
*/