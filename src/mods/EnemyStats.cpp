#include "EnemyStats.hpp"
#if 1
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static bool display_enemy_stats = false;
static bool display_boss_stats = false;
static bool freeze_move_id = false;
static bool hotkey_enabled = false;
static int which_enemy = 0;
static int8_t saved_enemy_move_id = 0;
static int8_t saved_enemy_move_i_d2 = 0;
static int8_t saved_enemy_move_part = 0;
static int8_t saved_enemy_grounded = 0;
static float saved_enemy_pos_xyz[3]{ 0.0f, 0.0f, 0.0f };
static float saved_enemy_velocity_xyz[3]{ 0.0f, 0.0f, 0.0f };
// static float savedEnemyAnimationFrame = 0.0f;

enum HotkeyIndexes {
    SAVE_ENEMY_STATS_HOTKEY,
    APPLY_ENEMY_STATS_HOTKEY,
    SAVE_BOSS_STATS_HOTKEY,
    APPLY_BOSS_STATS_HOTKEY
};

std::optional<std::string> EnemyStats::on_initialize() {

    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_HOME, "Save Enemy Stats", "save_enemy_stats_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_END, "Apply Enemy Stats","apply_enemy_stats_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(VK_PRIOR, "Save Boss Stats", "save_boss_stats_key"));
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

// call with true to save, call with false to load
void save_load_enemy_info(bool isSave) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    if (isSave) {
        if (s_med_ptr->uEnemies[which_enemy]) {
            saved_enemy_pos_xyz[0]      = s_med_ptr->uEnemies[which_enemy]->position[0];
            saved_enemy_pos_xyz[1]      = s_med_ptr->uEnemies[which_enemy]->position[1];
            saved_enemy_pos_xyz[2]      = s_med_ptr->uEnemies[which_enemy]->position[2];
            saved_enemy_velocity_xyz[0] = s_med_ptr->uEnemies[which_enemy]->velocity[0];
            saved_enemy_velocity_xyz[1] = s_med_ptr->uEnemies[which_enemy]->velocity[1];
            saved_enemy_velocity_xyz[2] = s_med_ptr->uEnemies[which_enemy]->velocity[2];
            saved_enemy_move_id         = s_med_ptr->uEnemies[which_enemy]->moveID;
            saved_enemy_move_i_d2       = s_med_ptr->uEnemies[which_enemy]->moveID2;
            saved_enemy_grounded        = s_med_ptr->uEnemies[which_enemy]->grounded;
        }
    } else {
        if (s_med_ptr->uEnemies[which_enemy]) {
            s_med_ptr->uEnemies[which_enemy]->position[0] = saved_enemy_pos_xyz[0];
            s_med_ptr->uEnemies[which_enemy]->position[1] = saved_enemy_pos_xyz[1];
            s_med_ptr->uEnemies[which_enemy]->position[2] = saved_enemy_pos_xyz[2];
            s_med_ptr->uEnemies[which_enemy]->velocity[0] = saved_enemy_velocity_xyz[0];
            s_med_ptr->uEnemies[which_enemy]->velocity[1] = saved_enemy_velocity_xyz[1];
            s_med_ptr->uEnemies[which_enemy]->velocity[2] = saved_enemy_velocity_xyz[2];
            s_med_ptr->uEnemies[which_enemy]->moveID      = saved_enemy_move_id;
            s_med_ptr->uEnemies[which_enemy]->moveID2     = saved_enemy_move_i_d2;
            s_med_ptr->uEnemies[which_enemy]->grounded    = saved_enemy_grounded;
            s_med_ptr->uEnemies[which_enemy]->movePart    = (uint8_t)0;
        }
    }
}

// call with true to save, call with false to load
void save_load_boss_info(bool isSave) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    if (isSave) {
        if (s_med_ptr->uBoss1) {
            saved_enemy_pos_xyz[0] = s_med_ptr->uBoss1->position[0];
            saved_enemy_pos_xyz[1] = s_med_ptr->uBoss1->position[1];
            saved_enemy_pos_xyz[2] = s_med_ptr->uBoss1->position[2];
            saved_enemy_move_id    = s_med_ptr->uBoss1->moveID;
            saved_enemy_move_i_d2  = s_med_ptr->uBoss1->moveID2;
        }
    } else {
        if (s_med_ptr->uBoss1) {
            s_med_ptr->uBoss1->position[0] = saved_enemy_pos_xyz[0];
            s_med_ptr->uBoss1->position[1] = saved_enemy_pos_xyz[1];
            s_med_ptr->uBoss1->position[2] = saved_enemy_pos_xyz[2];
            s_med_ptr->uBoss1->moveID      = saved_enemy_move_id;
            s_med_ptr->uBoss1->moveID2     = saved_enemy_move_i_d2;
            s_med_ptr->uBoss1->movePart    = (uint8_t)0;
        }
    }
}

void EnemyStats::on_gui_frame() {
    ImGui::Checkbox(_("Display Enemy Stats"), &display_enemy_stats);
    if (display_enemy_stats) {
        ImGui::Indent(lineIndent);
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        if (s_med_ptr) {
            ImGui::SliderInt(_("Enemy Count"), (int*)&s_med_ptr->enemyCount[2], 0, 0);
            ImGui::SliderInt(_("Enemy Select"), &which_enemy, 0, s_med_ptr->enemyCount[2] - 1);
            if (s_med_ptr->uEnemies[0]) {
                ImGui::Spacing();
                uEnemy* currentEnemy      = s_med_ptr->uEnemies[which_enemy];
                int damage_info_offset    = get_enemy_specific_damage_offset(currentEnemy->ID);
                // i hate this, game accesses them from base ptr, e.g. [uEnemy+1544] for scarecrow hp
                uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)currentEnemy + damage_info_offset);
                if (currentEnemy != NULL) {
                    ImGui::InputFloat(_("HP ##2"), &currentEnemyDamage->HP);
                    ImGui::InputFloat(_("Max HP ##2"), &currentEnemyDamage->HPMax);
                    ImGui::InputFloat(_("HP Taken ##2"), &currentEnemyDamage->HPTaken);
                    ImGui::InputInt(_("Stun 1 ##2"), &currentEnemyDamage->stun[0]);
                    ImGui::InputInt(_("Stun 2 ##2"), &currentEnemyDamage->stun[1]);
                    ImGui::InputInt(_("Stun 3 ##2"), &currentEnemyDamage->stun[2]);
                    ImGui::InputInt(_("Stun 4 ##2"), &currentEnemyDamage->stun[3]);
                    ImGui::InputInt(_("Stun 5 ##2"), &currentEnemyDamage->stun[4]);
                    ImGui::InputInt(_("Displacement 1 ##2"), &currentEnemyDamage->displacement[0]);
                    ImGui::InputInt(_("Displacement 2 ##2"), &currentEnemyDamage->displacement[1]);
                    ImGui::InputInt(_("Displacement 3 ##2"), &currentEnemyDamage->displacement[2]);
                    ImGui::InputInt(_("Displacement 4 ##2"), &currentEnemyDamage->displacement[3]);
                    ImGui::InputInt(_("Displacement 5 ##2"), &currentEnemyDamage->displacement[4]);
                    ImGui::InputInt(_("Unknown 1 ##2"), &currentEnemyDamage->unknown[0]);
                    ImGui::InputInt(_("Unknown 2 ##2"), &currentEnemyDamage->unknown[1]);
                    ImGui::InputInt(_("Unknown 3 ##2"), &currentEnemyDamage->unknown[2]);
                    ImGui::InputInt(_("Unknown 4 ##2"), &currentEnemyDamage->unknown[3]);
                    ImGui::InputInt(_("Unknown 5 ##2"), &currentEnemyDamage->unknown[4]);
                    ImGui::InputInt(_("Unknown 6 ##2"), &currentEnemyDamage->unknown[5]);
                    ImGui::InputInt(_("Unknown 7 ##2"), &currentEnemyDamage->unknown[6]);
                    ImGui::InputInt(_("Unknown 8 ##2"), &currentEnemyDamage->unknown[7]);
                }
                ImGui::InputFloat3(_("XYZ Position ##2"), (float*)&currentEnemy->position);
                ImGui::InputFloat3(_("XYZ Velocity ##2"), (float*)&currentEnemy->velocity);
                ImGui::InputFloat3(_("XYZ Scale ##2"), (float*)&currentEnemy->scale);
                ImGui::InputScalar(_("Move ID ##2"), ImGuiDataType_U8, &currentEnemy->moveID);
                ImGui::InputScalar(_("Move ID 2 ##2"), ImGuiDataType_U8, &currentEnemy->moveID2, 0, 0);
                ImGui::InputScalar(_("Move Part ##2"), ImGuiDataType_U8, &currentEnemy->movePart);
                ImGui::InputScalar(_("Grounded ##2"), ImGuiDataType_U8, &currentEnemy->grounded);
                ImGui::InputFloat(_("Animation Frame ##2"), &currentEnemy->animFrame);

                if (ImGui::Button(_("Save Selected Enemy Info"))) {
                    save_load_enemy_info(true);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is HOME by default"));

                if (ImGui::Button(_("Replay Saved Move ID & Position"))) {
                    save_load_enemy_info(false);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is END by default"));
            }
        }
        ImGui::Unindent(lineIndent);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox(_("Display Boss Stats"), &display_boss_stats);
    if (display_boss_stats) {
        ImGui::Indent(lineIndent);
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        if (s_med_ptr) {
            uintptr_t* boss_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0xB0);
            uintptr_t boss_base = *boss_ptr;
            if (s_med_ptr->uBoss1) {
                ImGui::Spacing();
                ImGui::InputFloat3(_("XYZ Position ##3"), (float*)&s_med_ptr->uBoss1->position);
                ImGui::InputFloat3(_("XYZ Scale ##3"), (float*)&s_med_ptr->uBoss1->scale);
                ImGui::InputFloat(_("HP ##3"), &s_med_ptr->uBoss1->HP);
                ImGui::InputFloat(_("Max HP ##3"), &s_med_ptr->uBoss1->HPMax);
                ImGui::InputScalar(_("Move ID ##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->moveID);
                ImGui::InputScalar(_("Move ID 2 ##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->moveID2);
                ImGui::InputScalar(_("Move Part ##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->movePart);
                ImGui::InputFloat(_("Animation Frame ##3"), &s_med_ptr->uBoss1->animFrame);

                if (ImGui::Button(_("Save Boss Info ##3"))) {
                    save_load_boss_info(true);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is PAGE UP by default"));

                if (ImGui::Button(_("Replay Saved Move ID & Position ##3"))) {
                    save_load_boss_info(false);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is PAGE DOWN by default"));
            }
        }
        ImGui::Unindent(lineIndent);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox(_("Enable Save/Load hotkeys"), &hotkey_enabled);

    ImGui::Spacing();

    if (ImGui::CollapsingHeader(_("Saved Info"))) {
        ImGui::InputScalar(_("Saved Move ID"), ImGuiDataType_U8, &saved_enemy_move_id);
        ImGui::InputScalar(_("Saved Move ID 2"), ImGuiDataType_U8, &saved_enemy_move_i_d2, 0, 0);
        ImGui::InputFloat3(_("Saved XYZ Position"), saved_enemy_pos_xyz);
        ImGui::InputFloat3(_("Saved XYZ Velocity"), saved_enemy_velocity_xyz);
        ImGui::InputScalar(_("Saved Grounded"), ImGuiDataType_U8, &saved_enemy_grounded);
    }
}

void EnemyStats::on_update_input(utility::Input& input) {
    if (hotkey_enabled) {
        if (m_hotkeys[SAVE_ENEMY_STATS_HOTKEY]->check(input)) {
            save_load_enemy_info(true);
        }
        if (m_hotkeys[APPLY_ENEMY_STATS_HOTKEY]->check(input)) {
            save_load_enemy_info(false);
        }
        if (m_hotkeys[SAVE_BOSS_STATS_HOTKEY]->check(input)) {
            save_load_boss_info(true);
        }
        if (m_hotkeys[APPLY_BOSS_STATS_HOTKEY]->check(input)) {
            save_load_boss_info(false);
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
