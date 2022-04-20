#include "modEnemyStats.hpp"
#include "modEnemySpawn.hpp"
#if 1
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
int EnemyStats::hotkey1{ NULL };
int EnemyStats::hotkey2{ NULL };
int EnemyStats::hotkey3{ NULL };
int EnemyStats::hotkey4{ NULL };

static bool displayEnemyStats = false;
static bool displayBossStats = false;
static int whichEnemy = 1;
static bool freezeMoveID = false;
static bool hotkeyEnabled = false;

static int8_t savedEnemyMoveID = 0;
static int savedEnemyMoveID2 = 0;
static int8_t savedEnemyMovePart = 0;
static int8_t savedEnemyGrounded = 0;
static float savedEnemyPosXYZ[3]{ 0.0f, 0.0f, 0.0f };
static float savedEnemyVelocityXYZ[3]{ 0.0f, 0.0f, 0.0f };
// static float savedEnemyAnimationFrame = 0.0f;

std::optional<std::string> EnemyStats::onInitialize() {
    return Mod::onInitialize();
}

int EnemyStats::getEnemySpecificDamageOffset(int enemyID) {
    switch (enemyID) {
    // 0x152C
    case 0x0: // Scarecrow Leg
        return 0x152C;
    case 0x1: // Scarecrow Arm
        return 0x152C;
    case 0x3: // Scarecrow Mega
        return 0x152C;

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

    // 7FC4
    case 0x11: // Basilisk
        return 0x7FC4;
    }
    return NULL;
}

void EnemyStats::onGUIframe() {
    ImGui::Checkbox("Display Enemy Stats", &displayEnemyStats);
    if (displayEnemyStats) {
        sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
        if (sMedPtr) {
            int8_t& enemyCount = *(int8_t*)((uintptr_t)sMedPtr + 0x1E8);
            uintptr_t* enemyPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x1B4 + whichEnemy * 4);
            uintptr_t enemyBase = *enemyPtr;
            if (enemyBase) {
                // get stats
                uint8_t& enemyMoveID = *(uint8_t*)(enemyBase + 0x14);
                int& enemyMoveID2 = *(int*)(enemyBase + 0x334);
                uint8_t& enemyMovePart = *(uint8_t*)(enemyBase + 0x15);
                int8_t& enemyGrounded = *(int8_t*)(enemyBase + 0x161C);
                float& enemyFrame = *(float*)(enemyBase + 0x348);
                float* enemyPosXYZ[3];
                    enemyPosXYZ[0] = (float*)(enemyBase + 0x30);
                    enemyPosXYZ[1] = (float*)(enemyBase + 0x34);
                    enemyPosXYZ[2] = (float*)(enemyBase + 0x38);
                float* enemyScaleXYZ[3];
                    enemyScaleXYZ[0] = (float*)(enemyBase + 0x50);
                    enemyScaleXYZ[1] = (float*)(enemyBase + 0x54);
                    enemyScaleXYZ[2] = (float*)(enemyBase + 0x58);
                float* enemyVelocityXYZ[3];
                    enemyVelocityXYZ[0] = (float*)(enemyBase + 0x1B40);
                    enemyVelocityXYZ[1] = (float*)(enemyBase + 0x1B44);
                    enemyVelocityXYZ[2] = (float*)(enemyBase + 0x1B48);

                // imgui
                ImGui::SliderInt("Enemy Count", (int*)&enemyCount, 0, 0);
                ImGui::SliderInt("Enemy Select", &whichEnemy, 1, enemyCount);

                ImGui::Spacing();

                int& enemyID = *(int*)(enemyBase + 0x1410);
                int damageInfoOffset = getEnemySpecificDamageOffset(enemyID);
                if (damageInfoOffset != NULL) {
                    float& enemyHP = *(float*)(enemyBase + damageInfoOffset + 0x18);
                    float& enemyMaxHP = *(float*)(enemyBase + damageInfoOffset + 0x1C);
                    int& enemyStun1 = *(int*)(enemyBase + damageInfoOffset + 0x3C);
                    int& enemyStun2 = *(int*)(enemyBase + damageInfoOffset + 0x40);
                    int& enemyStun3 = *(int*)(enemyBase + damageInfoOffset + 0x44);
                    int& enemyStun4 = *(int*)(enemyBase + damageInfoOffset + 0x48);
                    int& enemyStun5 = *(int*)(enemyBase + damageInfoOffset + 0x4C);
                    int& enemyDisplacement1 = *(int*)(enemyBase + damageInfoOffset + 0x50);
                    int& enemyDisplacement2 = *(int*)(enemyBase + damageInfoOffset + 0x54);
                    int& enemyDisplacement3 = *(int*)(enemyBase + damageInfoOffset + 0x58);
                    int& enemyDisplacement4 = *(int*)(enemyBase + damageInfoOffset + 0x5C);
                    int& enemyDisplacement5 = *(int*)(enemyBase + damageInfoOffset + 0x60);
                    int& Unknown1 = *(int*)(enemyBase + damageInfoOffset + 0x64);
                    int& Unknown2 = *(int*)(enemyBase + damageInfoOffset + 0x68);
                    int& Unknown3 = *(int*)(enemyBase + damageInfoOffset + 0x6C);
                    int& Unknown4 = *(int*)(enemyBase + damageInfoOffset + 0x70);
                    int& Unknown5 = *(int*)(enemyBase + damageInfoOffset + 0x74);
                    int& Unknown6 = *(int*)(enemyBase + damageInfoOffset + 0x78);
                    int& Unknown7 = *(int*)(enemyBase + damageInfoOffset + 0x7C);
                    int& Unknown8 = *(int*)(enemyBase + damageInfoOffset + 0x80);

                    ImGui::InputFloat("HP ##2", &enemyHP);
                    ImGui::InputFloat("Max HP ##2", &enemyMaxHP);
                    ImGui::InputInt("Stun 1 ##2", &enemyStun1, 0, 0);
                    ImGui::InputInt("Stun 2 ##2", &enemyStun2, 0, 0);
                    ImGui::InputInt("Stun 3 ##2", &enemyStun3, 0, 0);
                    ImGui::InputInt("Stun 4 ##2", &enemyStun4, 0, 0);
                    ImGui::InputInt("Stun 5 ##2", &enemyStun5, 0, 0);
                    ImGui::InputInt("Displacement 1 ##2", &enemyDisplacement1, 0, 0);
                    ImGui::InputInt("Displacement 2 ##2", &enemyDisplacement2, 0, 0);
                    ImGui::InputInt("Displacement 3 ##2", &enemyDisplacement3, 0, 0);
                    ImGui::InputInt("Displacement 4 ##2", &enemyDisplacement4, 0, 0);
                    ImGui::InputInt("Displacement 5 ##2", &enemyDisplacement5, 0, 0);
                    ImGui::InputInt("Unknown 1 ##2", &Unknown1, 0, 0);
                    ImGui::InputInt("Unknown 2 ##2", &Unknown2, 0, 0);
                    ImGui::InputInt("Unknown 3 ##2", &Unknown3, 0, 0);
                    ImGui::InputInt("Unknown 4 ##2", &Unknown4, 0, 0);
                    ImGui::InputInt("Unknown 5 ##2", &Unknown5, 0, 0);
                    ImGui::InputInt("Unknown 6 ##2", &Unknown6, 0, 0);
                    ImGui::InputInt("Unknown 7 ##2", &Unknown7, 0, 0);
                    ImGui::InputInt("Unknown 8 ##2", &Unknown8, 0, 0);
                }
                ImGui::InputFloat3("XYZ Position ##2", *enemyPosXYZ);
                ImGui::InputFloat3("XYZ Velocity ##2", *enemyVelocityXYZ);
                ImGui::InputFloat3("XYZ Scale ##2", *enemyScaleXYZ);
                ImGui::InputScalar("Move ID ##2", ImGuiDataType_U8, &enemyMoveID);
                ImGui::InputInt("Move ID 2 ##2", &enemyMoveID2, 0, 0);
                ImGui::InputScalar("Move Part ##2", ImGuiDataType_U8, &enemyMovePart);
                ImGui::InputScalar("Grounded ##2", ImGuiDataType_U8, &enemyGrounded);
                ImGui::InputFloat("Animation Frame ##2", &enemyFrame);

                if (ImGui::Button("Save Selected Enemy Info")) {
                    savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                    savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                    savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                    savedEnemyVelocityXYZ[0] = *enemyVelocityXYZ[0];
                    savedEnemyVelocityXYZ[1] = *enemyVelocityXYZ[1];
                    savedEnemyVelocityXYZ[2] = *enemyVelocityXYZ[2];
                    savedEnemyMoveID = enemyMoveID;
                    savedEnemyMoveID2 = enemyMoveID2;
                    savedEnemyGrounded = enemyGrounded;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is HOME by default");

                if (ImGui::Button("Replay Saved Move ID")) {
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyGrounded = savedEnemyGrounded;
                    enemyMovePart = (uint8_t)0;
                }

                if (ImGui::Button("Replay Saved Move ID & Position")) {
                    *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                    *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                    *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                    *enemyVelocityXYZ[0] = savedEnemyVelocityXYZ[0];
                    *enemyVelocityXYZ[1] = savedEnemyVelocityXYZ[1];
                    *enemyVelocityXYZ[2] = savedEnemyVelocityXYZ[2];
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyGrounded = savedEnemyGrounded;
                    enemyMovePart = (uint8_t)0;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is END by default");

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

    ImGui::Checkbox("Display Boss Stats", &displayBossStats);
    if (displayBossStats) {
        sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
        if (sMedPtr) {
            uintptr_t* bossPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0xB0);
            uintptr_t bossBase = *bossPtr;
            if (bossBase) {
                // get stats
                float& enemyHP = *(float*)(bossBase + 0x151C);
                float& enemyMaxHP = *(float*)(bossBase + 0x1520);
                uint8_t& enemyMoveID = *(uint8_t*)(bossBase + 0x14);
                int& enemyMoveID2 = *(int*)(bossBase + 0x334);
                uint8_t& enemyMovePart = *(uint8_t*)(bossBase + 0x15);
                float& enemyFrame = *(float*)(bossBase + 0x348);
                float* enemyPosXYZ[3];
                    enemyPosXYZ[0] = (float*)(bossBase + 0x30);
                    enemyPosXYZ[1] = (float*)(bossBase + 0x34);
                    enemyPosXYZ[2] = (float*)(bossBase + 0x38);
                float* enemyScaleXYZ[3];
                    enemyScaleXYZ[0] = (float*)(bossBase + 0x50);
                    enemyScaleXYZ[1] = (float*)(bossBase + 0x54);
                    enemyScaleXYZ[2] = (float*)(bossBase + 0x58);

                // imgui
                ImGui::InputFloat3("XYZ Position ##3", *enemyPosXYZ);
                ImGui::InputFloat3("XYZ Scale ##3", *enemyScaleXYZ);
                ImGui::InputFloat("HP ##3", &enemyHP);
                ImGui::InputFloat("Max HP ##3", &enemyMaxHP);
                ImGui::InputScalar("Move ID ##3", ImGuiDataType_U8, &enemyMoveID);
                ImGui::InputInt("Move ID 2 ##3", &enemyMoveID2, 0, 0);
                ImGui::InputScalar("Move Part ##3", ImGuiDataType_U8, &enemyMovePart);
                ImGui::InputFloat("Animation Frame ##3", &enemyFrame);

                if (ImGui::Button("Save Selected Enemy Info ##3")) {
                    savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                    savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                    savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                    savedEnemyMoveID = enemyMoveID;
                    savedEnemyMoveID2 = enemyMoveID2;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is PAGE UP by default");

                if (ImGui::Button("Replay Saved Move ID ##3")) {
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyMovePart = (uint8_t)0;
                }

                if (ImGui::Button("Replay Saved Move ID & Position ##3")) {
                    *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                    *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                    *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyMovePart = (uint8_t)0;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is PAGE DOWN by default");
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Enable Save/Load hotkeys", &hotkeyEnabled);

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Saved Info")) {
        ImGui::InputScalar("Saved Move ID", ImGuiDataType_U8, &savedEnemyMoveID);
        ImGui::InputInt("Saved Move ID 2", &savedEnemyMoveID2, 0, 0);
        ImGui::InputFloat3("Saved XYZ Position", savedEnemyPosXYZ);
        ImGui::InputFloat3("Saved XYZ Velocity", savedEnemyVelocityXYZ);
        ImGui::InputScalar("Saved Grounded", ImGuiDataType_U8, &savedEnemyGrounded);
    }
}

void EnemyStats::onUpdateInput(hl::Input& input) {
    if (!input.isDown(EnemySpawn::hotkeySpawnModifier)) {
        if (hotkeyEnabled) {
            if (input.wentDown(hotkey1)) {
                sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
                if (sMedPtr) {
                    uintptr_t* enemyPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x164 + whichEnemy * 4);
                    uintptr_t enemyBase = *enemyPtr;
                    if (enemyBase) {
                        // get stats
                        uint8_t& enemyMoveID = *(uint8_t*)(enemyBase + 0x14);
                        int& enemyMoveID2 = *(int*)(enemyBase + 0x334);
                        uint8_t& enemyMovePart = *(uint8_t*)(enemyBase + 0x15);
                        int8_t& enemyGrounded = *(int8_t*)(enemyBase + 0x161C);
                        float* enemyPosXYZ[3];
                            enemyPosXYZ[0] = (float*)(enemyBase + 0x30);
                            enemyPosXYZ[1] = (float*)(enemyBase + 0x34);
                            enemyPosXYZ[2] = (float*)(enemyBase + 0x38);
                        float* enemyVelocityXYZ[3];
                            enemyVelocityXYZ[0] = (float*)(enemyBase + 0x1B40);
                            enemyVelocityXYZ[1] = (float*)(enemyBase + 0x1B44);
                            enemyVelocityXYZ[2] = (float*)(enemyBase + 0x1B48);
                        // save stats
                        savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                        savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                        savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                        savedEnemyVelocityXYZ[0] = *enemyVelocityXYZ[0];
                        savedEnemyVelocityXYZ[1] = *enemyVelocityXYZ[1];
                        savedEnemyVelocityXYZ[2] = *enemyVelocityXYZ[2];
                        savedEnemyMoveID = enemyMoveID;
                        savedEnemyMoveID2 = enemyMoveID2;
                        savedEnemyGrounded = enemyGrounded;
                    }
                }
            }
            if (input.wentDown(hotkey2)) {
                sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
                if (sMedPtr) {
                    uintptr_t* enemyPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x164 + whichEnemy * 4);
                    uintptr_t enemyBase = *enemyPtr;
                    if (enemyBase) {
                        // get stats
                        uint8_t& enemyMoveID = *(uint8_t*)(enemyBase + 0x14);
                        int& enemyMoveID2 = *(int*)(enemyBase + 0x334);
                        uint8_t& enemyMovePart = *(uint8_t*)(enemyBase + 0x15);
                        int8_t& enemyGrounded = *(int8_t*)(enemyBase + 0x161C);
                        float* enemyPosXYZ[3];
                            enemyPosXYZ[0] = (float*)(enemyBase + 0x30);
                            enemyPosXYZ[1] = (float*)(enemyBase + 0x34);
                            enemyPosXYZ[2] = (float*)(enemyBase + 0x38);
                        float* enemyVelocityXYZ[3];
                            enemyVelocityXYZ[0] = (float*)(enemyBase + 0x1B40);
                            enemyVelocityXYZ[1] = (float*)(enemyBase + 0x1B44);
                            enemyVelocityXYZ[2] = (float*)(enemyBase + 0x1B48);
                        // apply stats
                        *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                        *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                        *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                        *enemyVelocityXYZ[0] = savedEnemyVelocityXYZ[0];
                        *enemyVelocityXYZ[1] = savedEnemyVelocityXYZ[1];
                        *enemyVelocityXYZ[2] = savedEnemyVelocityXYZ[2];
                        enemyMoveID = savedEnemyMoveID;
                        enemyMoveID2 = savedEnemyMoveID2;
                        enemyMovePart = (uint8_t)0;
                        enemyGrounded = savedEnemyGrounded;
                    }
                }
            }
            if (input.wentDown(hotkey3)) {
                sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
                if (sMedPtr) {
                    uintptr_t* bossPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0xB0);
                    uintptr_t bossBase = *bossPtr;
                    if (bossBase) {
                        // get stats
                        uint8_t& enemyMoveID = *(uint8_t*)(bossBase + 0x14);
                        int& enemyMoveID2 = *(int*)(bossBase + 0x334);
                        uint8_t& enemyMovePart = *(uint8_t*)(bossBase + 0x15);
                        float* enemyPosXYZ[3];
                            enemyPosXYZ[0] = (float*)(bossBase + 0x30);
                            enemyPosXYZ[1] = (float*)(bossBase + 0x34);
                            enemyPosXYZ[2] = (float*)(bossBase + 0x38);
                        // save stats
                        savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                        savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                        savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                        savedEnemyMoveID = enemyMoveID;
                        savedEnemyMoveID2 = enemyMoveID2;
                    }
                }
            }
            if (input.wentDown(hotkey4)) {
                sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
                if (sMedPtr) {
                    uintptr_t* bossPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0xB0);
                    uintptr_t bossBase = *bossPtr;
                    if (bossBase) {
                        // get stats
                        uint8_t& enemyMoveID = *(uint8_t*)(bossBase + 0x14);
                        int& enemyMoveID2 = *(int*)(bossBase + 0x334);
                        uint8_t& enemyMovePart = *(uint8_t*)(bossBase + 0x15);
                        float* enemyPosXYZ[3];
                            enemyPosXYZ[0] = (float*)(bossBase + 0x30);
                            enemyPosXYZ[1] = (float*)(bossBase + 0x34);
                            enemyPosXYZ[2] = (float*)(bossBase + 0x38);
                        // apply stats
                        *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                        *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                        *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                        enemyMoveID = savedEnemyMoveID;
                        enemyMoveID2 = savedEnemyMoveID2;
                        enemyMovePart = (uint8_t)0;
                    }
                }
            }
        }
    }
}

void EnemyStats::onConfigLoad(const utils::Config& cfg) {
    hotkeyEnabled = cfg.get<bool>("enable_enemy_stats_hotkeys").value_or(true);
    hotkey1 = cfg.get<int>("load_enemy_stats_hotkey").value_or(0x24); // HOME
    hotkey2 = cfg.get<int>("apply_enemy_stats_hotkey").value_or(0x23); // END
    hotkey3 = cfg.get<int>("load_boss_stats_hotkey").value_or(0x21);  // PAGE UP
    hotkey4 = cfg.get<int>("apply_boss_stats_hotkey").value_or(0x22); // PAGE DOWN
}

void EnemyStats::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("enable_enemy_stats_hotkeys", hotkeyEnabled);
    cfg.set<int>("load_enemy_stats_hotkey", hotkey1);
    cfg.set<int>("apply_enemy_stats_hotkey", hotkey2);
    cfg.set<int>("load_boss_stats_hotkey", hotkey3);
    cfg.set<int>("apply_boss_stats_hotkey", hotkey4);
}

#endif
