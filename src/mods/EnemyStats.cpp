#include "EnemyStats.hpp"
#if 1
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static bool display_enemy_stats = false;
static bool display_boss_stats = false;
static int which_enemy = 1;
static bool freeze_move_id = false;
static bool hotkey_enabled = false;

static int8_t saved_enemy_move_id = 0;
static int saved_enemy_move_i_d2 = 0;
static int8_t saved_enemy_move_part = 0;
static int8_t saved_enemy_grounded = 0;
static float saved_enemy_pos_xyz[3]{ 0.0f, 0.0f, 0.0f };
static float saved_enemy_velocity_xyz[3]{ 0.0f, 0.0f, 0.0f };
// static float savedEnemyAnimationFrame = 0.0f;

enum HotkeyIndexes {
    LOAD_ENEMY_STATS_HOTKEY,
    APPLY_ENEMY_STATS_HOTKEY,
    LOAD_BOSS_STATS_HOTKEY,
    APPLY_BOSS_STATS_HOTKEY
};

std::optional<std::string> EnemyStats::on_initialize() {

    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_HOME, "Load Enemy Stats", "load_enemy_stats_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_END, "Apply Enemy Stats","apply_enemy_stats_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_PRIOR, "Load Boss Stats", "load_boss_stats_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_NEXT, "Apply Boss Stats", "apply_boss_stats_key"));
    
    return Mod::on_initialize();
}

int EnemyStats::get_enemy_specific_damage_offset(int enemy_id) {
    switch (enemy_id) {
    // 0x1500
    case 0x8: // mephisto
        return 0x1500;
    case 0x9: // faust
        return 0x1500;
    case 0xB: // assault
        return 0x1500;
    case 0x10: // gladius
        return 0x1500;

    // 0x1504
    case 0x5: // Alto
        return 0x1504;
    case 0x6: // Bianco
        return 0x1504;
    case 0xA: // Frost
        return 0x1504;
    case 0xC: // Blitz
        return 0x1504;

    // 1508
    case 0xF: // Cutlass
        return 0x1508;

    // 0x152C
    case 0x0: // Scarecrow Leg
        return 0x152C;
    case 0x1: // Scarecrow Arm
        return 0x152C;
    case 0x3: // Scarecrow Mega
        return 0x152C;

    // 7FC4
    case 0x11: // Basilisk
        return 0x7FC4;
    }
    return NULL;
}

void EnemyStats::on_gui_frame() {
    ImGui::Checkbox("Display Enemy Stats", &display_enemy_stats);
    if (display_enemy_stats) {
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        if (s_med_ptr) {
            int8_t& enemy_count = *(int8_t*)((uintptr_t)s_med_ptr + 0x1E8);
            uintptr_t* enemy_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x1B4 + which_enemy * 4);
            ImGui::SliderInt("Enemy Count", (int*)&enemy_count, 0, 0);
            ImGui::SliderInt("Enemy Select", &which_enemy, 1, enemy_count);
            uintptr_t enemy_base = *enemy_ptr;
            if (enemy_base) {
                // get stats
                uint8_t& enemy_move_id = *(uint8_t*)(enemy_base + 0x14);
                int& enemy_move_i_d2 = *(int*)(enemy_base + 0x334);
                uint8_t& enemy_move_part = *(uint8_t*)(enemy_base + 0x15);
                int8_t& enemy_grounded = *(int8_t*)(enemy_base + 0x161C);
                float& enemy_frame = *(float*)(enemy_base + 0x348);
                float* enemy_pos_xyz[3];
                    enemy_pos_xyz[0] = (float*)(enemy_base + 0x30);
                    enemy_pos_xyz[1] = (float*)(enemy_base + 0x34);
                    enemy_pos_xyz[2] = (float*)(enemy_base + 0x38);
                float* enemy_scale_xyz[3];
                    enemy_scale_xyz[0] = (float*)(enemy_base + 0x50);
                    enemy_scale_xyz[1] = (float*)(enemy_base + 0x54);
                    enemy_scale_xyz[2] = (float*)(enemy_base + 0x58);
                float* enemy_velocity_xyz[3];
                    enemy_velocity_xyz[0] = (float*)(enemy_base + 0x1B40);
                    enemy_velocity_xyz[1] = (float*)(enemy_base + 0x1B44);
                    enemy_velocity_xyz[2] = (float*)(enemy_base + 0x1B48);

                // imgui
                ImGui::Spacing();

                int& enemy_id = *(int*)(enemy_base + 0x1410);
                int damage_info_offset = get_enemy_specific_damage_offset(enemy_id);
                if (damage_info_offset != NULL) {
                    float& enemy_hp = *(float*)(enemy_base + damage_info_offset + 0x18);
                    float& enemy_max_hp = *(float*)(enemy_base + damage_info_offset + 0x1C);
                    int& enemy_stun1 = *(int*)(enemy_base + damage_info_offset + 0x3C);
                    int& enemy_stun2 = *(int*)(enemy_base + damage_info_offset + 0x40);
                    int& enemy_stun3 = *(int*)(enemy_base + damage_info_offset + 0x44);
                    int& enemy_stun4 = *(int*)(enemy_base + damage_info_offset + 0x48);
                    int& enemy_stun5 = *(int*)(enemy_base + damage_info_offset + 0x4C);
                    int& enemy_displacement1 = *(int*)(enemy_base + damage_info_offset + 0x50);
                    int& enemy_displacement2 = *(int*)(enemy_base + damage_info_offset + 0x54);
                    int& enemy_displacement3 = *(int*)(enemy_base + damage_info_offset + 0x58);
                    int& enemy_displacement4 = *(int*)(enemy_base + damage_info_offset + 0x5C);
                    int& enemy_displacement5 = *(int*)(enemy_base + damage_info_offset + 0x60);
                    int& unknown1 = *(int*)(enemy_base + damage_info_offset + 0x64);
                    int& unknown2 = *(int*)(enemy_base + damage_info_offset + 0x68);
                    int& unknown3 = *(int*)(enemy_base + damage_info_offset + 0x6C);
                    int& unknown4 = *(int*)(enemy_base + damage_info_offset + 0x70);
                    int& unknown5 = *(int*)(enemy_base + damage_info_offset + 0x74);
                    int& unknown6 = *(int*)(enemy_base + damage_info_offset + 0x78);
                    int& unknown7 = *(int*)(enemy_base + damage_info_offset + 0x7C);
                    int& unknown8 = *(int*)(enemy_base + damage_info_offset + 0x80);

                    ImGui::InputFloat("HP ##2", &enemy_hp);
                    ImGui::InputFloat("Max HP ##2", &enemy_max_hp);
                    ImGui::InputInt("Stun 1 ##2", &enemy_stun1, 0, 0);
                    ImGui::InputInt("Stun 2 ##2", &enemy_stun2, 0, 0);
                    ImGui::InputInt("Stun 3 ##2", &enemy_stun3, 0, 0);
                    ImGui::InputInt("Stun 4 ##2", &enemy_stun4, 0, 0);
                    ImGui::InputInt("Stun 5 ##2", &enemy_stun5, 0, 0);
                    ImGui::InputInt("Displacement 1 ##2", &enemy_displacement1, 0, 0);
                    ImGui::InputInt("Displacement 2 ##2", &enemy_displacement2, 0, 0);
                    ImGui::InputInt("Displacement 3 ##2", &enemy_displacement3, 0, 0);
                    ImGui::InputInt("Displacement 4 ##2", &enemy_displacement4, 0, 0);
                    ImGui::InputInt("Displacement 5 ##2", &enemy_displacement5, 0, 0);
                    ImGui::InputInt("Unknown 1 ##2", &unknown1, 0, 0);
                    ImGui::InputInt("Unknown 2 ##2", &unknown2, 0, 0);
                    ImGui::InputInt("Unknown 3 ##2", &unknown3, 0, 0);
                    ImGui::InputInt("Unknown 4 ##2", &unknown4, 0, 0);
                    ImGui::InputInt("Unknown 5 ##2", &unknown5, 0, 0);
                    ImGui::InputInt("Unknown 6 ##2", &unknown6, 0, 0);
                    ImGui::InputInt("Unknown 7 ##2", &unknown7, 0, 0);
                    ImGui::InputInt("Unknown 8 ##2", &unknown8, 0, 0);
                }
                ImGui::InputFloat3("XYZ Position ##2", *enemy_pos_xyz);
                ImGui::InputFloat3("XYZ Velocity ##2", *enemy_velocity_xyz);
                ImGui::InputFloat3("XYZ Scale ##2", *enemy_scale_xyz);
                ImGui::InputScalar("Move ID ##2", ImGuiDataType_U8, &enemy_move_id);
                ImGui::InputInt("Move ID 2 ##2", &enemy_move_i_d2, 0, 0);
                ImGui::InputScalar("Move Part ##2", ImGuiDataType_U8, &enemy_move_part);
                ImGui::InputScalar("Grounded ##2", ImGuiDataType_U8, &enemy_grounded);
                ImGui::InputFloat("Animation Frame ##2", &enemy_frame);

                if (ImGui::Button("Save Selected Enemy Info")) {
                    saved_enemy_pos_xyz[0] = *enemy_pos_xyz[0];
                    saved_enemy_pos_xyz[1] = *enemy_pos_xyz[1];
                    saved_enemy_pos_xyz[2] = *enemy_pos_xyz[2];
                    saved_enemy_velocity_xyz[0] = *enemy_velocity_xyz[0];
                    saved_enemy_velocity_xyz[1] = *enemy_velocity_xyz[1];
                    saved_enemy_velocity_xyz[2] = *enemy_velocity_xyz[2];
                    saved_enemy_move_id = enemy_move_id;
                    saved_enemy_move_i_d2 = enemy_move_i_d2;
                    saved_enemy_grounded = enemy_grounded;
                }
                ImGui::SameLine();
                help_marker("Hotkey is HOME by default");

                if (ImGui::Button("Replay Saved Move ID")) {
                    enemy_move_id = saved_enemy_move_id;
                    enemy_move_i_d2 = saved_enemy_move_i_d2;
                    enemy_grounded = saved_enemy_grounded;
                    enemy_move_part = (uint8_t)0;
                }

                if (ImGui::Button("Replay Saved Move ID & Position")) {
                    *enemy_pos_xyz[0] = saved_enemy_pos_xyz[0];
                    *enemy_pos_xyz[1] = saved_enemy_pos_xyz[1];
                    *enemy_pos_xyz[2] = saved_enemy_pos_xyz[2];
                    *enemy_velocity_xyz[0] = saved_enemy_velocity_xyz[0];
                    *enemy_velocity_xyz[1] = saved_enemy_velocity_xyz[1];
                    *enemy_velocity_xyz[2] = saved_enemy_velocity_xyz[2];
                    enemy_move_id = saved_enemy_move_id;
                    enemy_move_i_d2 = saved_enemy_move_i_d2;
                    enemy_grounded = saved_enemy_grounded;
                    enemy_move_part = (uint8_t)0;
                }
                ImGui::SameLine();
                help_marker("Hotkey is END by default");

                /*if (ImGui::Button("Restore Saved Move ID, Position & Frame")) {
                    *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                    *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                    *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                    *enemyVelocityXYZ[0] = savedEnemyVelocityXYZ[0];
                    *enemyVelocityXYZ[1] = savedEnemyVelocityXYZ[1];
                    *enemyVelocityXYZ[2] = savedEnemyVelocityXYZ[2];
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyMovePart = (uint8_t)0; // this writes animation frame 0 after I do so I can't set frame myself :(
                    enemyFrame = savedEnemyAnimationFrame;
                }*/
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Display Boss Stats", &display_boss_stats);
    if (display_boss_stats) {
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        if (s_med_ptr) {
            uintptr_t* boss_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0xB0);
            uintptr_t boss_base = *boss_ptr;
            if (boss_base) {
                // get stats
                float& enemy_hp = *(float*)(boss_base + 0x151C);
                float& enemy_max_hp = *(float*)(boss_base + 0x1520);
                uint8_t& enemy_move_id = *(uint8_t*)(boss_base + 0x14);
                int& enemy_move_i_d2 = *(int*)(boss_base + 0x334);
                uint8_t& enemy_move_part = *(uint8_t*)(boss_base + 0x15);
                float& enemy_frame = *(float*)(boss_base + 0x348);
                float* enemy_pos_xyz[3];
                    enemy_pos_xyz[0] = (float*)(boss_base + 0x30);
                    enemy_pos_xyz[1] = (float*)(boss_base + 0x34);
                    enemy_pos_xyz[2] = (float*)(boss_base + 0x38);
                float* enemy_scale_xyz[3];
                    enemy_scale_xyz[0] = (float*)(boss_base + 0x50);
                    enemy_scale_xyz[1] = (float*)(boss_base + 0x54);
                    enemy_scale_xyz[2] = (float*)(boss_base + 0x58);

                // imgui
                ImGui::InputFloat3("XYZ Position ##3", *enemy_pos_xyz);
                ImGui::InputFloat3("XYZ Scale ##3", *enemy_scale_xyz);
                ImGui::InputFloat("HP ##3", &enemy_hp);
                ImGui::InputFloat("Max HP ##3", &enemy_max_hp);
                ImGui::InputScalar("Move ID ##3", ImGuiDataType_U8, &enemy_move_id);
                ImGui::InputInt("Move ID 2 ##3", &enemy_move_i_d2, 0, 0);
                ImGui::InputScalar("Move Part ##3", ImGuiDataType_U8, &enemy_move_part);
                ImGui::InputFloat("Animation Frame ##3", &enemy_frame);

                if (ImGui::Button("Save Selected Enemy Info ##3")) {
                    saved_enemy_pos_xyz[0] = *enemy_pos_xyz[0];
                    saved_enemy_pos_xyz[1] = *enemy_pos_xyz[1];
                    saved_enemy_pos_xyz[2] = *enemy_pos_xyz[2];
                    saved_enemy_move_id = enemy_move_id;
                    saved_enemy_move_i_d2 = enemy_move_i_d2;
                }
                ImGui::SameLine();
                help_marker("Hotkey is PAGE UP by default");

                if (ImGui::Button("Replay Saved Move ID ##3")) {
                    enemy_move_id = saved_enemy_move_id;
                    enemy_move_i_d2 = saved_enemy_move_i_d2;
                    enemy_move_part = (uint8_t)0;
                }

                if (ImGui::Button("Replay Saved Move ID & Position ##3")) {
                    *enemy_pos_xyz[0] = saved_enemy_pos_xyz[0];
                    *enemy_pos_xyz[1] = saved_enemy_pos_xyz[1];
                    *enemy_pos_xyz[2] = saved_enemy_pos_xyz[2];
                    enemy_move_id = saved_enemy_move_id;
                    enemy_move_i_d2 = saved_enemy_move_i_d2;
                    enemy_move_part = (uint8_t)0;
                }
                ImGui::SameLine();
                help_marker("Hotkey is PAGE DOWN by default");
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Enable Save/Load hotkeys", &hotkey_enabled);

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Saved Info")) {
        ImGui::InputScalar("Saved Move ID", ImGuiDataType_U8, &saved_enemy_move_id);
        ImGui::InputInt("Saved Move ID 2", &saved_enemy_move_i_d2, 0, 0);
        ImGui::InputFloat3("Saved XYZ Position", saved_enemy_pos_xyz);
        ImGui::InputFloat3("Saved XYZ Velocity", saved_enemy_velocity_xyz);
        ImGui::InputScalar("Saved Grounded", ImGuiDataType_U8, &saved_enemy_grounded);
    }
}

void EnemyStats::on_update_input(utility::Input& input) {
        if (hotkey_enabled) {
            if (m_hotkeys[LOAD_ENEMY_STATS_HOTKEY]->check(input)) {
                SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
                if (s_med_ptr) {
                    uintptr_t* enemy_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x164 + which_enemy * 4);
                    uintptr_t enemy_base = *enemy_ptr;
                    if (enemy_base) {
                        // get stats
                        uint8_t& enemy_move_id = *(uint8_t*)(enemy_base + 0x14);
                        int& enemy_move_i_d2 = *(int*)(enemy_base + 0x334);
                        uint8_t& enemy_move_part = *(uint8_t*)(enemy_base + 0x15);
                        int8_t& enemy_grounded = *(int8_t*)(enemy_base + 0x161C);
                        float* enemy_pos_xyz[3];
                            enemy_pos_xyz[0] = (float*)(enemy_base + 0x30);
                            enemy_pos_xyz[1] = (float*)(enemy_base + 0x34);
                            enemy_pos_xyz[2] = (float*)(enemy_base + 0x38);
                        float* enemy_velocity_xyz[3];
                            enemy_velocity_xyz[0] = (float*)(enemy_base + 0x1B40);
                            enemy_velocity_xyz[1] = (float*)(enemy_base + 0x1B44);
                            enemy_velocity_xyz[2] = (float*)(enemy_base + 0x1B48);
                        // save stats
                        saved_enemy_pos_xyz[0] = *enemy_pos_xyz[0];
                        saved_enemy_pos_xyz[1] = *enemy_pos_xyz[1];
                        saved_enemy_pos_xyz[2] = *enemy_pos_xyz[2];
                        saved_enemy_velocity_xyz[0] = *enemy_velocity_xyz[0];
                        saved_enemy_velocity_xyz[1] = *enemy_velocity_xyz[1];
                        saved_enemy_velocity_xyz[2] = *enemy_velocity_xyz[2];
                        saved_enemy_move_id = enemy_move_id;
                        saved_enemy_move_i_d2 = enemy_move_i_d2;
                        saved_enemy_grounded = enemy_grounded;
                    }
                }
            }
            if (m_hotkeys[APPLY_ENEMY_STATS_HOTKEY]->check(input)) {
                SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
                if (s_med_ptr) {
                    uintptr_t* enemy_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x164 + which_enemy * 4);
                    uintptr_t enemy_base = *enemy_ptr;
                    if (enemy_base) {
                        // get stats
                        uint8_t& enemy_move_id = *(uint8_t*)(enemy_base + 0x14);
                        int& enemy_move_i_d2 = *(int*)(enemy_base + 0x334);
                        uint8_t& enemy_move_part = *(uint8_t*)(enemy_base + 0x15);
                        int8_t& enemy_grounded = *(int8_t*)(enemy_base + 0x161C);
                        float* enemy_pos_xyz[3];
                            enemy_pos_xyz[0] = (float*)(enemy_base + 0x30);
                            enemy_pos_xyz[1] = (float*)(enemy_base + 0x34);
                            enemy_pos_xyz[2] = (float*)(enemy_base + 0x38);
                        float* enemy_velocity_xyz[3];
                            enemy_velocity_xyz[0] = (float*)(enemy_base + 0x1B40);
                            enemy_velocity_xyz[1] = (float*)(enemy_base + 0x1B44);
                            enemy_velocity_xyz[2] = (float*)(enemy_base + 0x1B48);
                        // apply stats
                        *enemy_pos_xyz[0] = saved_enemy_pos_xyz[0];
                        *enemy_pos_xyz[1] = saved_enemy_pos_xyz[1];
                        *enemy_pos_xyz[2] = saved_enemy_pos_xyz[2];
                        *enemy_velocity_xyz[0] = saved_enemy_velocity_xyz[0];
                        *enemy_velocity_xyz[1] = saved_enemy_velocity_xyz[1];
                        *enemy_velocity_xyz[2] = saved_enemy_velocity_xyz[2];
                        enemy_move_id = saved_enemy_move_id;
                        enemy_move_i_d2 = saved_enemy_move_i_d2;
                        enemy_move_part = (uint8_t)0;
                        enemy_grounded = saved_enemy_grounded;
                    }
                }
            }
            if (m_hotkeys[LOAD_BOSS_STATS_HOTKEY]->check(input)) {
                SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
                if (s_med_ptr) {
                    uintptr_t* boss_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0xB0);
                    uintptr_t boss_base = *boss_ptr;
                    if (boss_base) {
                        // get stats
                        uint8_t& enemy_move_id = *(uint8_t*)(boss_base + 0x14);
                        int& enemy_move_i_d2 = *(int*)(boss_base + 0x334);
                        uint8_t& enemy_move_part = *(uint8_t*)(boss_base + 0x15);
                        float* enemy_pos_xyz[3];
                            enemy_pos_xyz[0] = (float*)(boss_base + 0x30);
                            enemy_pos_xyz[1] = (float*)(boss_base + 0x34);
                            enemy_pos_xyz[2] = (float*)(boss_base + 0x38);
                        // save stats
                        saved_enemy_pos_xyz[0] = *enemy_pos_xyz[0];
                        saved_enemy_pos_xyz[1] = *enemy_pos_xyz[1];
                        saved_enemy_pos_xyz[2] = *enemy_pos_xyz[2];
                        saved_enemy_move_id = enemy_move_id;
                        saved_enemy_move_i_d2 = enemy_move_i_d2;
                    }
                }
            }
            if (m_hotkeys[APPLY_BOSS_STATS_HOTKEY]->check(input)) {
                SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
                if (s_med_ptr) {
                    uintptr_t* boss_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0xB0);
                    uintptr_t boss_base = *boss_ptr;
                    if (boss_base) {
                        // get stats
                        uint8_t& enemy_move_id = *(uint8_t*)(boss_base + 0x14);
                        int& enemy_move_i_d2 = *(int*)(boss_base + 0x334);
                        uint8_t& enemy_move_part = *(uint8_t*)(boss_base + 0x15);
                        float* enemy_pos_xyz[3];
                            enemy_pos_xyz[0] = (float*)(boss_base + 0x30);
                            enemy_pos_xyz[1] = (float*)(boss_base + 0x34);
                            enemy_pos_xyz[2] = (float*)(boss_base + 0x38);
                        // apply stats
                        *enemy_pos_xyz[0] = saved_enemy_pos_xyz[0];
                        *enemy_pos_xyz[1] = saved_enemy_pos_xyz[1];
                        *enemy_pos_xyz[2] = saved_enemy_pos_xyz[2];
                        enemy_move_id = saved_enemy_move_id;
                        enemy_move_i_d2 = saved_enemy_move_i_d2;
                        enemy_move_part = (uint8_t)0;
                    }
                }
            }
        }
}

void EnemyStats::on_config_load(const utility::Config& cfg) {
    hotkey_enabled = cfg.get<bool>("enable_enemy_stats_hotkeys").value_or(true);
}

void EnemyStats::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enable_enemy_stats_hotkeys", hotkey_enabled);
}

#endif
