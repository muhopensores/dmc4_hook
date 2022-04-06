#include "modPlayerTracker.hpp"
#include "modWorkRate.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return{ NULL };
// uintptr_t* PlayerTracker::player_base_ptr{ (uintptr_t*)(0x00E558B8) }; // DevilMayCry4_DX9.exe+A558B8
uPlayer* PlayerTracker::player_ptr{ NULL };
bool PlayerTracker::lockOnAlloc{ false };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
bool displayPlayerStats = false;
bool potentiallyDumb = false;

void update_player_info(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    if (uLocalPlr) {
        PlayerTracker::lockOnAlloc = uLocalPlr->lockontoggle; // 16D0
    }
}

naked void detour() {
    _asm {
            movss [esi+0x30], xmm3 // originalcode
            push ecx
            push edx
            mov ecx, [staticMediatorPtr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            cmp esi, ecx
            pop edx
            pop ecx
            je manualplayer
        //jmpret:
		    jmp dword ptr [PlayerTracker::jmp_return]

            manualplayer:
            push eax
            call update_player_info
            pop eax
            jmp dword ptr [PlayerTracker::jmp_return]
    }
}

std::optional<std::string> PlayerTracker::onInitialize()
{
    if (!install_hook_offset(0x3A88A1, hook, &detour, &jmp_return, 5)) {
        HL_LOG_ERR("Failed to init PlayerTracker mod\n");
        return "Failed to init PlayerTracker mod";
    }

    return Mod::onInitialize();
}

void PlayerTracker::onGUIframe() {
    ImGui::Spacing();

    ImGui::Checkbox("Disable Game Pause When Opening The Trainer", &WorkRate::disableTrainerPause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Display Player Stats", &displayPlayerStats);
    if (displayPlayerStats) {
        sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
        uintptr_t* playerPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x24);
        uintptr_t playerBase = *playerPtr;
        if (playerBase)
        {
            float* playerXYZ[3];
            playerXYZ[0] = (float*)(playerBase + 0x30);
            playerXYZ[1] = (float*)(playerBase + 0x34);
            playerXYZ[2] = (float*)(playerBase + 0x38);

            float* playerRotation[4];
            playerRotation[0] = (float*)(playerBase + 0x40);
            playerRotation[1] = (float*)(playerBase + 0x44);
            playerRotation[2] = (float*)(playerBase + 0x48);
            playerRotation[3] = (float*)(playerBase + 0x4C);

            float* playerScale[3];
            playerScale[0] = (float*)(playerBase + 0x50);
            playerScale[1] = (float*)(playerBase + 0x54);
            playerScale[2] = (float*)(playerBase + 0x58);

            float& animationFrame = *(float*)(playerBase + 0x348);
            float& playerCurrentHP = *(float*)(playerBase + 0x15CC);
            float& playerMaxHP = *(float*)(playerBase + 0x15D0);
            int8_t& playerWeight = *(int8_t*)(playerBase + 0x1E7D);
            int& playerMoveID = *(int*)(playerBase + 0x2998);
            int8_t& playerMovePart = *(int8_t*)(playerBase + 0x1504);
            int8_t& playerLockOn = *(int8_t*)(playerBase + 0x16D0);

            float* playerVelocityXYZ[3];
            playerVelocityXYZ[0] = (float*)(playerBase + 0x1E50);
            playerVelocityXYZ[1] = (float*)(playerBase + 0x1E54);
            playerVelocityXYZ[2] = (float*)(playerBase + 0x1E58);
            float& playerMagnitude = *(float*)(playerBase + 0x1E60);

            ImGui::InputFloat("HP ##1", &playerCurrentHP);
            ImGui::InputFloat("Max HP ##1", &playerMaxHP);
            ImGui::InputFloat3("XYZ Position ##1", *playerXYZ);
            ImGui::InputFloat4("Rotation ##1", *playerRotation);
            ImGui::InputFloat3("XYZ Scale ##1", *playerScale);
            ImGui::InputFloat3("XYZ Velocity ##1", *playerVelocityXYZ);
            ImGui::InputFloat("Inertia ##1", &playerMagnitude);
            ImGui::InputInt("Move ID ##1", &playerMoveID, 0, 0);
            ImGui::InputScalar("Move Part ##1", ImGuiDataType_U8, &playerMovePart);
            ImGui::InputFloat("Animation Frame ##1", &animationFrame);
            ImGui::InputScalar("Weight ##1", ImGuiDataType_U8, &playerWeight);
            ImGui::InputScalar("Lock On ##1", ImGuiDataType_U8, &playerLockOn);
        }
    }
}

#endif
