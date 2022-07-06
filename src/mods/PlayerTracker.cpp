#include "PlayerTracker.hpp"
#include "WorkRate.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return{ NULL };
uPlayer* PlayerTracker::player_ptr{ NULL };
bool PlayerTracker::lock_on_alloc{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8; // DevilMayCry4_DX9.exe+A558B8
static bool display_player_stats = false;

/*void update_player_info(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    if (uLocalPlr) {
        PlayerTracker::lockOnAlloc = uLocalPlr->lockontoggle; // 16D0
    }
}*/

/*naked void detour() {
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
}*/

std::optional<std::string> PlayerTracker::on_initialize() {
    /*if (!install_hook_offset(0x3A88A1, hook, &detour, &jmp_return, 5)) {
        spdlog::error("Failed to init PlayerTracker mod\n");
        return "Failed to init PlayerTracker mod";
    }*/

    return Mod::on_initialize();
}

void PlayerTracker::on_gui_frame() {
    ImGui::Spacing();

    ImGui::Checkbox("Disable Game Pause When Opening The Trainer", &WorkRate::disable_trainer_pause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Display Player Stats", &display_player_stats);
    if (display_player_stats) {
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        uintptr_t* player_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x24);
        uintptr_t player_base = *player_ptr;
        if (player_base)
        {
            float* player_xyz[3];
                player_xyz[0] = (float*)(player_base + 0x30);
                player_xyz[1] = (float*)(player_base + 0x34);
                player_xyz[2] = (float*)(player_base + 0x38);

            float* player_rotation[4];
                player_rotation[0] = (float*)(player_base + 0x40);
                player_rotation[1] = (float*)(player_base + 0x44);
                player_rotation[2] = (float*)(player_base + 0x48);
                player_rotation[3] = (float*)(player_base + 0x4C);

            float* player_scale[3];
                player_scale[0] = (float*)(player_base + 0x50);
                player_scale[1] = (float*)(player_base + 0x54);
                player_scale[2] = (float*)(player_base + 0x58);

            float& animation_frame = *(float*)(player_base + 0x348);
            float& player_current_hp = *(float*)(player_base + 0x15CC);
            float& player_max_hp = *(float*)(player_base + 0x15D0);
            int8_t& player_weight = *(int8_t*)(player_base + 0x1E7D);
            int8_t& player_move_bank = *(int8_t*)(player_base + 0x1500);
            int8_t& player_move_id = *(int8_t*)(player_base + 0x1564);
            int8_t& player_move_part = *(int8_t*)(player_base + 0x1504);
            int8_t& player_lock_on = *(int8_t*)(player_base + 0x16D0);

            float* player_velocity_xyz[3];
                player_velocity_xyz[0] = (float*)(player_base + 0x1E50);
                player_velocity_xyz[1] = (float*)(player_base + 0x1E54);
                player_velocity_xyz[2] = (float*)(player_base + 0x1E58);
            float& player_speed = *(float*)(player_base + 0x1E60);
            float& player_inertia = *(float*)(player_base + 0x1E1C);

            ImGui::InputFloat("HP ##1", &player_current_hp);
            ImGui::InputFloat("Max HP ##1", &player_max_hp);
            ImGui::InputFloat3("XYZ Position ##1", *player_xyz);
            ImGui::InputFloat4("Rotation ##1", *player_rotation);
            ImGui::InputFloat3("XYZ Scale ##1", *player_scale);
            ImGui::InputFloat3("XYZ Velocity ##1", *player_velocity_xyz);
            ImGui::InputFloat("Movement Speed ##1", &player_speed);
            ImGui::InputFloat("Inertia ##1", &player_inertia);
            ImGui::InputScalar("Move Bank ##1", ImGuiDataType_U8, &player_move_bank);
            ImGui::InputScalar("Move ID ##1", ImGuiDataType_U8, &player_move_id);
            ImGui::InputScalar("Move Part ##1", ImGuiDataType_U8, &player_move_part);
            ImGui::InputFloat("Animation Frame ##1", &animation_frame);
            if (ImGui::Button("Replay Current Move")) {
                player_move_part = 0;
            }
            ImGui::InputScalar("Weight ##1", ImGuiDataType_U8, &player_weight);
            ImGui::InputScalar("Lock On ##1", ImGuiDataType_U8, &player_lock_on);
        }
    }
}

#endif
