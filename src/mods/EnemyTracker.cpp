#include "EnemyTracker.hpp"
#include "PlayerTracker.hpp"
#if 1
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

static bool display_enemy_stats = false;
static bool display_boss_stats = false;
static bool freeze_move_id = false;
static bool hotkey_enabled = false;
static int which_enemy = 0;
static bool useLockedOnEnemyInstead = 0;

static int8_t savedEnemyMoveID = 0;
static int8_t savedEnemyMoveID2 = 0;
static int8_t savedEnemyGrounded = 0;
static float savedEnemyHP = 0;
static int8_t savedEnemyStun = 0;
static float savedEnemyPosition[3]{ 0.0f, 0.0f, 0.0f };
static float savedEnemyVelocity[3]{ 0.0f, 0.0f, 0.0f };
static float savedEnemyRotation = 0.0f;
// static int8_t savedEnemyMovePart = 0;
// static float savedEnemyAnimationFrame = 0.0f;

static int8_t savedPlayerMoveID = 0;
static float savedPlayerPosition[3]{ 0.0f, 0.0f, 0.0f };
static float savedPlayerInertia = 0.0f;
static float savedPlayerRotation = 0.0f;
static int8_t savedPlayerGrounded = 0;
// static int8_t savedPlayerMovePart = 0;
// static float savedPlayerAnimationFrame = 0.0f;

enum HotkeyIndexes {
    SAVE_ENEMY_STATS_HOTKEY,
    APPLY_ENEMY_STATS_HOTKEY,
    SAVE_BOSS_STATS_HOTKEY,
    APPLY_BOSS_STATS_HOTKEY
};

std::optional<std::string> EnemyTracker::on_initialize() {

    utility::create_keyboard_hotkey(m_hotkeys, {VK_HOME},  "Save Enemy Stats",  "save_enemy_stats_key");
    utility::create_keyboard_hotkey(m_hotkeys, {VK_END},   "Apply Enemy Stats", "apply_enemy_stats_key");
    utility::create_keyboard_hotkey(m_hotkeys, {VK_PRIOR}, "Save Boss Stats",   "save_boss_stats_key");
    utility::create_keyboard_hotkey(m_hotkeys, {VK_NEXT},  "Apply Boss Stats",  "apply_boss_stats_key");
    
    return Mod::on_initialize();
}

int EnemyTracker::get_enemy_specific_damage_offset(int enemy_id) {
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
void save_load_enemy_info(bool isSave, uEnemy* enemy) {
    if (enemy) {
        if (isSave) {
            savedEnemyPosition[0] = enemy->position[0];
            savedEnemyPosition[1] = enemy->position[1];
            savedEnemyPosition[2] = enemy->position[2];
            savedEnemyRotation = enemy->rotation[1];
            savedEnemyVelocity[0] = enemy->velocity[0];
            savedEnemyVelocity[1] = enemy->velocity[1];
            savedEnemyVelocity[2] = enemy->velocity[2];
            savedEnemyMoveID = enemy->moveID;
            savedEnemyMoveID2 = enemy->moveID2;
            savedEnemyGrounded = enemy->grounded;
            uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
            savedEnemyHP = currentEnemyDamage->HP;
            savedEnemyStun = currentEnemyDamage->stun[0];
        }
        if (!isSave && savedEnemyPosition[0] != 0.0f) { // verify at least 1 save has happened
            enemy->position[0] = savedEnemyPosition[0]; // if the enemy has just landed, position is not being set correctly @siy
            enemy->position[1] = savedEnemyPosition[1];
            enemy->position[2] = savedEnemyPosition[2];
            enemy->rotation[1] = savedEnemyRotation;
            enemy->velocity[0] = savedEnemyVelocity[0];
            enemy->velocity[1] = savedEnemyVelocity[1];
            enemy->velocity[2] = savedEnemyVelocity[2];
            enemy->moveID = savedEnemyMoveID;
            enemy->moveID2 = savedEnemyMoveID2;
            uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
            currentEnemyDamage->HP = savedEnemyHP;
            currentEnemyDamage->stun[0] = savedEnemyStun;
            enemy->grounded = savedEnemyGrounded;
            enemy->movePart = 0;
        }
    }
}

// call with true to save, call with false to load
void save_load_boss_info(bool isSave) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    if (s_med_ptr->uBoss1) {
        if (isSave) {
            savedEnemyPosition[0] = s_med_ptr->uBoss1->position[0];
            savedEnemyPosition[1] = s_med_ptr->uBoss1->position[1];
            savedEnemyPosition[2] = s_med_ptr->uBoss1->position[2];
            savedEnemyRotation = s_med_ptr->uBoss1->rotation[1];
            savedEnemyMoveID = s_med_ptr->uBoss1->moveID;
            savedEnemyMoveID2 = s_med_ptr->uBoss1->moveID2;
        }
        else {
            s_med_ptr->uBoss1->position[0] = savedEnemyPosition[0];
            s_med_ptr->uBoss1->position[1] = savedEnemyPosition[1];
            s_med_ptr->uBoss1->position[2] = savedEnemyPosition[2];
            s_med_ptr->uBoss1->rotation[1] = savedEnemyRotation;
            s_med_ptr->uBoss1->moveID = savedEnemyMoveID;
            s_med_ptr->uBoss1->moveID2 = savedEnemyMoveID2;
            s_med_ptr->uBoss1->movePart = 0;
        }
    }
}

uEnemy* GetDesiredEnemy(bool useLockon) {
    SMediator* s_med_ptr = devil4_sdk::get_sMediator();
    uEnemy* enemy = NULL;
    if (useLockon) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            if (player->lockOnTargetPtr3) {
                if ((uintptr_t)player->lockOnTargetPtr3 != (uintptr_t)s_med_ptr->uBoss1) {
                    enemy = player->lockOnTargetPtr3;
                }
            }
        }
    }
    else {
        enemy = s_med_ptr->uEnemies[which_enemy];
    }
    return enemy;
}

void SaveStateWithCurrentEnemy() {
    if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
        save_load_enemy_info(true, enemy);
    PlayerTracker::SavePlayerMove();
}
void LoadStateWithCurrentEnemy() {
    if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
        save_load_enemy_info(false, enemy);
    PlayerTracker::LoadPlayerMove();
}

void EnemyTracker::on_gui_frame() {
    ImGui::Checkbox(_("Display Enemy Stats"), &display_enemy_stats);
    if (display_enemy_stats) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox("Use Locked On Enemy Instead Of Picking", &useLockedOnEnemyInstead);

        ImGui::Spacing();

        SMediator* s_med_ptr = devil4_sdk::get_sMediator();

        if (ImGui::Button(_("Save Selected Enemy Info"))) {
            if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                save_load_enemy_info(true, enemy);
        }

        if (ImGui::Button(_("Replay Saved Move ID & Position"))) {
            if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                save_load_enemy_info(false, enemy);
        }

        if (ImGui::Button(_("Save State"))) {
            SaveStateWithCurrentEnemy();
        }
        ImGui::SameLine();
        help_marker(_("Affects you and the enemy you have selected on this page\nHotkey is HOME by default"));

        if (ImGui::Button(_("Load State"))) {
            LoadStateWithCurrentEnemy();
        }
        ImGui::SameLine();
        help_marker(_("Affects you and the enemy you have selected on this page\nHotkey is END by default"));

        ImGui::Unindent(lineIndent);

        // get desired enemy and show stats
        if (auto currentEnemy = GetDesiredEnemy(useLockedOnEnemyInstead)) {
            ImGui::Indent(lineIndent);
            if (!useLockedOnEnemyInstead) {
                ImGui::SliderInt(_("Enemy Count"), (int*)&s_med_ptr->enemyCount[2], 0, 0);
                if (s_med_ptr->enemyCount[0] > 0) {
                    ImGui::SliderInt(_("Enemy Select"), &which_enemy, 0, s_med_ptr->enemyCount[2] - 1);
                    if (ImGui::Button(_("Find Locked On Enemy In List"))) {
                        if (uPlayer* player = devil4_sdk::get_local_player()) {
                            for (uint32_t i = 0; i < s_med_ptr->enemyCount[2]; ++i) {
                                if (s_med_ptr->uEnemies[i] && s_med_ptr->uEnemies[i] == player->lockOnTargetPtr3) {
                                    which_enemy = i;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // i hate this, game accesses them from base ptr, e.g. [uEnemy+1544] for scarecrow hp
            int damage_info_offset = get_enemy_specific_damage_offset(currentEnemy->ID);
            uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)currentEnemy + damage_info_offset);
            ImGui::InputFloat(_("HP ##2"), &currentEnemyDamage->HP);
            ImGui::InputFloat(_("Max HP ##2"), &currentEnemyDamage->HPMax);
            ImGui::InputFloat(_("Previous Hit Dealt"), &currentEnemyDamage->HPTaken);
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
                
            ImGui::InputFloat3(_("XYZ Position ##2"), (float*)&currentEnemy->position);
            ImGui::InputFloat3(_("XYZ Rotation ##2"), (float*)&currentEnemy->rotation);
            ImGui::InputFloat3(_("XYZ Velocity ##2"), (float*)&currentEnemy->velocity);
            ImGui::InputFloat3(_("XYZ Scale ##2"), (float*)&currentEnemy->scale);
            ImGui::InputScalar(_("Move ID ##2"), ImGuiDataType_U8, &currentEnemy->moveID);
            ImGui::InputScalar(_("Move ID 2 ##2"), ImGuiDataType_U8, &currentEnemy->moveID2, 0, 0);
            ImGui::InputScalar(_("Move Part ##2"), ImGuiDataType_U8, &currentEnemy->movePart);
            ImGui::InputScalar(_("Grounded ##2"), ImGuiDataType_U8, &currentEnemy->grounded);
            ImGui::InputFloat(_("Animation Frame ##2"), &currentEnemy->animFrame);
            ImGui::Unindent(lineIndent);
        }
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
                ImGui::InputFloat4(_("XYZ Rotation ##3"), (float*)&s_med_ptr->uBoss1->rotation);
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
    ImGui::SameLine();
    help_marker("Assuming default hotkeys,\nHome+End will save and load enemy attacks\nPage Up+Page Down will save and load boss attacks");

    ImGui::Spacing();

    if (ImGui::CollapsingHeader(_("Saved Info"))) {
        ImGui::InputScalar(_("Enemy Move ID"), ImGuiDataType_U8, &savedEnemyMoveID);
        ImGui::InputScalar(_("Enemy Move ID 2"), ImGuiDataType_U8, &savedEnemyMoveID2, 0, 0);
        ImGui::InputFloat3(_("Enemy Position"), savedEnemyPosition);
        ImGui::InputFloat(_("Enemy Rotation"), &savedEnemyRotation);
        ImGui::InputFloat3(_("Enemy Velocity"), savedEnemyVelocity);
        ImGui::InputScalar(_("Enemy Grounded"), ImGuiDataType_U8, &savedEnemyGrounded);
    }
}

void EnemyTracker::on_update_input(utility::Input& input) {
    if (hotkey_enabled) {
        if (m_hotkeys[SAVE_ENEMY_STATS_HOTKEY]->check(input)) {
            //if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                //save_load_enemy_info(true, enemy);
            SaveStateWithCurrentEnemy();
        }
        if (m_hotkeys[APPLY_ENEMY_STATS_HOTKEY]->check(input)) {
            //if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                //save_load_enemy_info(false, enemy);
            LoadStateWithCurrentEnemy();
        }
        if (m_hotkeys[SAVE_BOSS_STATS_HOTKEY]->check(input)) {
            save_load_boss_info(true);
        }
        if (m_hotkeys[APPLY_BOSS_STATS_HOTKEY]->check(input)) {
            save_load_boss_info(false);
        }
    }
}

void EnemyTracker::on_config_load(const utility::Config& cfg) {
    hotkey_enabled = cfg.get<bool>("enable_enemy_stats_hotkeys").value_or(true);
    useLockedOnEnemyInstead = cfg.get<bool>("enable_enemy_stats_lockon").value_or(false);
}

void EnemyTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enable_enemy_stats_hotkeys", hotkey_enabled);
    cfg.set<bool>("enable_enemy_stats_lockon", useLockedOnEnemyInstead);
}

#endif
