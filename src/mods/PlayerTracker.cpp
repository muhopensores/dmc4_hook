#include "PlayerTracker.hpp"
#include "WorkRate.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return{ NULL };
uPlayer* PlayerTracker::player_ptr{ NULL };
bool PlayerTracker::lock_on_alloc{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8; // DevilMayCry4_DX9.exe+A558B8
bool PlayerTracker::pin_imgui_enabled = false;
static bool display_player_stats = false;

std::optional<std::string> PlayerTracker::on_initialize() {
    MutatorRegistry::define("PocketKing").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(0.5f, 0.5f, 0.5f);
        }
    }).set_timer(60.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });

    MutatorRegistry::define("BigGuy").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.35f, 1.35f, 1.35f);
        }
    }).set_timer(60.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });
    return Mod::on_initialize();
}

void PlayerTracker::on_gui_frame() {
    ImGui::Spacing();

    ImGui::Checkbox("Disable Game Pause When Opening The Trainer", &WorkRate::disable_trainer_pause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Show Pin Timers", &PlayerTracker::pin_imgui_enabled);
    ImGui::Checkbox("Display Player Stats", &display_player_stats);
    if (display_player_stats) {
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        uintptr_t* player_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x24);
        uintptr_t player_base = *player_ptr;
        if (player_base)
        {
            int& controller_id = *(int*)(player_base + 0x1494);
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
            uint8_t& weapon_change_disable = *(uint8_t*)(player_base + 0x14F0);
            int8_t& player_lock_on = *(int8_t*)(player_base + 0x16D0);
            float& exceed_timer = *(float*)(player_base + 0xCDBC);

            float* player_velocity_xyz[3];
                player_velocity_xyz[0] = (float*)(player_base + 0x1E50);
                player_velocity_xyz[1] = (float*)(player_base + 0x1E54);
                player_velocity_xyz[2] = (float*)(player_base + 0x1E58);
            float& player_speed = *(float*)(player_base + 0x1E60);
            float& player_inertia = *(float*)(player_base + 0x1E1C);
            float& player_disaster = *(float*)(player_base + 0x151F4);

            ImGui::InputFloat("HP ##1", &player_current_hp);
            ImGui::InputFloat("Max HP ##1", &player_max_hp);
            ImGui::InputFloat3("XYZ Position ##1", *player_xyz);
            ImGui::InputFloat4("Rotation ##1", *player_rotation);
            ImGui::InputFloat3("XYZ Scale ##1", *player_scale);
            ImGui::InputFloat3("XYZ Velocity ##1", *player_velocity_xyz);
            ImGui::InputFloat("Movement Speed ##1", &player_speed);
            ImGui::InputFloat("Inertia ##1", &player_inertia);
            ImGui::SameLine();
            help_marker("Uhm, ehm, akshually, internia isn't a thing, it's a property of a thing, the shit you're showing is velocity, the "
                        "measure of inertia is mass, resistance to acceleration (slowing down is also acceleration just in the opposite "
                        "direction), if the thing that got yeeted doesn't like burn off it's layers in flight due to air friction like a "
                        "fucking meteor or some shit then it's inertia isn't changing. Get it right NERD");
            ImGui::InputScalar("Weight ##1", ImGuiDataType_U8, &player_weight);
            ImGui::InputScalar("Lock On ##1", ImGuiDataType_U8, &player_lock_on);
            if (controller_id == 0) { // dante
                ImGui::InputFloat("Disaster Gauge ##1", &player_disaster);
            } else { // nero
                ImGui::InputFloat("Exceed Timer ##1", &exceed_timer);
                ImGui::SameLine();
                help_marker("If you press exceed while this timer is between 0 and 1, you'll get MAX-Act.");
            }
            ImGui::InputFloat("Animation Frame ##1", &animation_frame);
            ImGui::InputScalar("Move Part ##1", ImGuiDataType_U8, &player_move_part);
            ImGui::InputScalar("Move Bank ##1", ImGuiDataType_U8, &player_move_bank);
            ImGui::InputScalar("Move ID ##1", ImGuiDataType_U8, &player_move_id);
            static int8_t tempMoveBank = 0;
            static int8_t tempMoveID   = 0;
            if (ImGui::Button("Save Current Move")) {
                tempMoveBank = player_move_bank;
                tempMoveID   = player_move_id;
            }
            ImGui::InputScalar("Saved Move Bank ##1", ImGuiDataType_U8, &tempMoveBank);
            ImGui::InputScalar("Saved Move ID ##1", ImGuiDataType_U8, &tempMoveID);
            if (ImGui::Button("Play Saved Move")) {
                player_move_bank = tempMoveBank;
                player_move_id   = tempMoveID;
                player_move_part = 0;
                weapon_change_disable = 12;
            }
        }
    }
}

void PlayerTracker::custom_imgui_window() {
    if (pin_imgui_enabled) {
        SMediator* s_med_ptr  = *(SMediator**)static_mediator_ptr;
        uintptr_t* player_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x24);
        uintptr_t player_base = *player_ptr;
        if (player_base) {
            int& controller_id = *(int*)(player_base + 0x1494);
            if (controller_id == 0) { // 0 = dante, some were crashing with nero and this might:tm: fix it
                uintptr_t* pin_ptr = (uintptr_t*)((uintptr_t)player_base + 0x14DBC); // 0x14DBC
                uintptr_t pin_base = *pin_ptr;
                if (pin_base) {
                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags window_flags =
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;
                    ImVec2 window_size = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.35f);
                    ImVec2 window_pos  = ImVec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                    ImGui::Begin("Lucifer Pin Timers", NULL, window_flags);

                    for (int i = 0; i < 15; i++) {
                        uintptr_t* pin_ptr = (uintptr_t*)((uintptr_t)player_base + 0x14DBC + i * 4); // 0x14DBC
                        uintptr_t pin_base = *pin_ptr;
                        if (pin_base) {
                            float& pin_timer     = *(float*)(pin_base + 0x1790);
                            float& pin_timer_max = *(float*)(pin_base + 0x1794);
                            bool& pin_penetrated = *(bool*)(pin_base + 0x17B4);
                            if (pin_penetrated) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                            }
                            ImGui::Text("Pin %i = %.0f", i + 1, pin_timer);
                            ImGui::SameLine();
                            ImGui::Text("/ %.0f", pin_timer_max);
                            ImGui::PopStyleColor();
                        }
                    }
                    ImGui::End();
                }
            }
        }
    }
}

void PlayerTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pin_imgui_enabled", pin_imgui_enabled);
}

void PlayerTracker::on_config_load(const utility::Config& cfg) {
    pin_imgui_enabled = cfg.get<bool>("pin_imgui_enabled").value_or(false);
}
#endif
