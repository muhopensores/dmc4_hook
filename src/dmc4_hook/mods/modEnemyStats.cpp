#include "modEnemyStats.hpp"
#include "modEnemySpawn.hpp"
#if 1

constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
int EnemyStats::hotkey1{ NULL };
int EnemyStats::hotkey2{ NULL };

bool displayEnemyStats = false;
int whichEnemy = 1;
bool freezeMoveID = false;
bool hotkeyEnabled = false;

int8_t savedEnemyMoveID = 0;
int savedEnemyMoveID2 = 0;
int8_t savedEnemyMovePart = 0;
float savedEnemyPosXYZ[3]{ 0.0f, 0.0f, 0.0f };
float savedEnemyAnimationFrame = 0.0f;
float savedEnemyVelocityXYZ[3]{ 0.0f, 0.0f, 0.0f };
int savedEnemyGrounded = 1;

std::optional<std::string> EnemyStats::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyStats::onGUIframe() {
    ImGui::Checkbox("Display Enemy Stats", &displayEnemyStats);
    if (displayEnemyStats) {
        sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
        if (sMedPtr) {
            int8_t& enemyCount = *(int8_t*)((uintptr_t)sMedPtr + 0x1E0);
            ImGui::SliderInt("Enemy Count", (int*)&enemyCount, 0, 0);
            ImGui::SliderInt("Enemy Select", &whichEnemy, 1, enemyCount);
            uintptr_t* enemyPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x164 + whichEnemy * 4);
            uintptr_t enemyBase = *enemyPtr;
            if (enemyBase) {
                // get stats
                uint8_t& enemyMoveID = *(uint8_t*)(enemyBase + 0x14);
                int& enemyMoveID2 = *(int*)(enemyBase + 0x334);
                uint8_t& enemyMovePart = *(uint8_t*)(enemyBase + 0x15);
                float* enemyPosXYZ[3];
                enemyPosXYZ[0] = (float*)(enemyBase + 0x30);
                enemyPosXYZ[1] = (float*)(enemyBase + 0x34);
                enemyPosXYZ[2] = (float*)(enemyBase + 0x38);
                float* enemyScaleXYZ[3];
                enemyScaleXYZ[0] = (float*)(enemyBase + 0x50);
                enemyScaleXYZ[1] = (float*)(enemyBase + 0x54);
                enemyScaleXYZ[2] = (float*)(enemyBase + 0x58);
                float& enemyFrame = *(float*)(enemyBase + 0x348);
                float& enemyHP = *(float*)(enemyBase + 0x1544);
                float& enemyMaxHP = *(float*)(enemyBase + 0x1548);
                int8_t& enemyGrounded = *(int8_t*)(enemyBase + 0x161C);
                float* enemyVelocityXYZ[3];
                enemyVelocityXYZ[0] = (float*)(enemyBase + 0x1B40);
                enemyVelocityXYZ[1] = (float*)(enemyBase + 0x1B44);
                enemyVelocityXYZ[2] = (float*)(enemyBase + 0x1B48);

                //int& launchThing = *(int*)(enemyBase + 0xEA0); // written by DevilMayCry4_DX9.exe+1503B7 
                //int& launchThing2 = *(int*)(enemyBase + 0xEA8);

                // imgui
                ImGui::InputFloat3("XYZ Position", *enemyPosXYZ);
                ImGui::InputFloat3("XYZ Velocity", *enemyVelocityXYZ);
                //ImGui::InputScalar("Grounded", ImGuiDataType_U8, &enemyGrounded);
                ImGui::InputScalar("Move ID", ImGuiDataType_U8, &enemyMoveID);
                ImGui::InputInt("Move ID 2", &enemyMoveID2);
                ImGui::InputScalar("Move Part", ImGuiDataType_U8, &enemyMovePart);
                ImGui::InputFloat("Animation Frame", &enemyFrame);
                ImGui::InputFloat3("XYZ Scale", *enemyScaleXYZ);
                ImGui::InputFloat("HP", &enemyHP);
                ImGui::InputFloat("Max HP", &enemyMaxHP);

                if (ImGui::Button("Save Selected Enemy Info")) {
                    savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                    savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                    savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                    savedEnemyVelocityXYZ[0] = *enemyVelocityXYZ[0];
                    savedEnemyVelocityXYZ[1] = *enemyVelocityXYZ[1];
                    savedEnemyVelocityXYZ[2] = *enemyVelocityXYZ[2];
                    //savedEnemyGrounded = enemyGrounded;
                    savedEnemyMoveID = enemyMoveID;
                    savedEnemyMoveID2 = enemyMoveID2;
                    savedEnemyMovePart = enemyMovePart;
                    savedEnemyAnimationFrame = enemyFrame;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is HOME by default");

                if (ImGui::Button("Replay Saved Move ID")) {
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyMovePart = (uint8_t)0;
                }

                if (ImGui::Button("Replay Saved Move ID & Position"))
                {
                    *enemyPosXYZ[0] = savedEnemyPosXYZ[0];
                    *enemyPosXYZ[1] = savedEnemyPosXYZ[1];
                    *enemyPosXYZ[2] = savedEnemyPosXYZ[2];
                    *enemyVelocityXYZ[0] = savedEnemyVelocityXYZ[0];
                    *enemyVelocityXYZ[1] = savedEnemyVelocityXYZ[1];
                    *enemyVelocityXYZ[2] = savedEnemyVelocityXYZ[2];
                    enemyMoveID = savedEnemyMoveID;
                    enemyMoveID2 = savedEnemyMoveID2;
                    enemyMovePart = (uint8_t)0;
                }
                ImGui::SameLine();
                HelpMarker("Hotkey is END by default");

                /*
                if (ImGui::Button("Restore Saved Move ID, Position & Frame")) {
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
                }
                */
                ImGui::Checkbox("Enable Save/Load hotkeys", &hotkeyEnabled);

                if (ImGui::CollapsingHeader("Saved Info"))
                {
                    ImGui::InputFloat3("Saved XYZ Position", savedEnemyPosXYZ);
                    ImGui::InputFloat3("Saved XYZ Velocity", savedEnemyVelocityXYZ);
                    ImGui::InputScalar("Saved Move ID", ImGuiDataType_U8, &savedEnemyMoveID);
                    ImGui::InputInt("Saved Move ID 2", &savedEnemyMoveID2);
                    ImGui::InputScalar("Saved Move Part", ImGuiDataType_U8, &savedEnemyMovePart);
                    ImGui::InputFloat("Saved Anim Frame", &savedEnemyAnimationFrame);
                }
            }
        }
    }
}

void EnemyStats::onUpdateInput(hl::Input& input) {
    if (!input.isDown(EnemySpawn::hotkeySpawnModifier)) {
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
                    float* enemyPosXYZ[3];
                    enemyPosXYZ[0] = (float*)(enemyBase + 0x30);
                    enemyPosXYZ[1] = (float*)(enemyBase + 0x34);
                    enemyPosXYZ[2] = (float*)(enemyBase + 0x38);
                    float* enemyVelocityXYZ[3];
                    enemyVelocityXYZ[0] = (float*)(enemyBase + 0x1B40);
                    enemyVelocityXYZ[1] = (float*)(enemyBase + 0x1B44);
                    enemyVelocityXYZ[2] = (float*)(enemyBase + 0x1B48);

                    savedEnemyPosXYZ[0] = *enemyPosXYZ[0];
                    savedEnemyPosXYZ[1] = *enemyPosXYZ[1];
                    savedEnemyPosXYZ[2] = *enemyPosXYZ[2];
                    savedEnemyVelocityXYZ[0] = *enemyVelocityXYZ[0];
                    savedEnemyVelocityXYZ[1] = *enemyVelocityXYZ[1];
                    savedEnemyVelocityXYZ[2] = *enemyVelocityXYZ[2];
                    // savedEnemyGrounded = enemyGrounded;
                    savedEnemyMoveID = enemyMoveID;
                    savedEnemyMoveID2 = enemyMoveID2;
                    savedEnemyMovePart = enemyMovePart;
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
                }
            }
        }
    }
}

void EnemyStats::onConfigLoad(const utils::Config& cfg) {
    hotkeyEnabled = cfg.get<bool>("enable_enemy_stats_hotkeys").value_or(true);
    hotkey1 = cfg.get<int>("load_enemy_stats_hotkey").value_or(0x24); // HOME
    hotkey2 = cfg.get<int>("apply_enemy_stats_hotkey").value_or(0x23); // END
}

void EnemyStats::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("enable_enemy_stats_hotkeys", hotkeyEnabled);
    cfg.set<int>("load_enemy_stats_hotkey", hotkey1);
    cfg.set<int>("apply_enemy_stats_hotkey", hotkey2);
}

#endif
