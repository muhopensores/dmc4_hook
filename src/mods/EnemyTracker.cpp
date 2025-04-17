#include "EnemyTracker.hpp"
#include "PlayerTracker.hpp"
#include "sdk/World2Screen.hpp"

#include "utility/Dx9Utils.hpp"
#include "utility/Compressed.hpp"
#include "misc/CompressedDrawing.cpp"

#if 1
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
bool EnemyTracker::flyingAd = false;
bool EnemyTracker::flyingSpheres = false;
bool EnemyTracker::flyingEnemyStats = false;
static bool showFlyingEnemyHP = false;
static bool showFlyingEnemyDamageTaken = false;
static bool showFlyingEnemyDT = false;
static bool showFlyingEnemyStun = false;
static bool showFlyingEnemyDisplacement = false;
static bool showFlyingEnemyMoveID = false;
static bool showFlyingEnemyShield = false;
static bool showFlyingEnemyStunTimer = false;

static bool freeze_move_id = false;
static bool hotkey_enabled = false;
static int which_enemy = 0;
static bool useLockedOnEnemyInstead = 0;

static int8_t savedEnemyMoveID = 0;
static int8_t savedEnemyAnimID = 0;
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
    g_pd3dDevice = g_framework->get_d3d9_device();
    after_reset();
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
            savedEnemyAnimID = enemy->animID;
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
            enemy->animID = savedEnemyAnimID;
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
            savedEnemyAnimID = s_med_ptr->uBoss1->animID;
        }
        else {
            s_med_ptr->uBoss1->position[0] = savedEnemyPosition[0];
            s_med_ptr->uBoss1->position[1] = savedEnemyPosition[1];
            s_med_ptr->uBoss1->position[2] = savedEnemyPosition[2];
            s_med_ptr->uBoss1->rotation[1] = savedEnemyRotation;
            s_med_ptr->uBoss1->moveID = savedEnemyMoveID;
            s_med_ptr->uBoss1->animID = savedEnemyAnimID;
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
    if (auto enemy = GetDesiredEnemy(true))
        save_load_enemy_info(true, enemy);
    PlayerTracker::SavePlayerMove();
}
void LoadStateWithCurrentEnemy() {
    if (auto enemy = GetDesiredEnemy(true))
        save_load_enemy_info(false, enemy);
    PlayerTracker::LoadPlayerMove();
}

void EnemyTracker::on_gui_frame(int display) {
    if (ImGui::CollapsingHeader(_("Display Enemy Stats"))) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("[DEBUG] Flying Spheres"), &flyingSpheres);
        ImGui::SameLine();
        help_marker(_("Sphere in the middle is clickable!\nThese are random sizes for proof of concept"));
        ImGui::Checkbox(_("[DEBUG] Flying Ad"), &flyingAd);
        ImGui::SameLine();
        help_marker(_("Makes the UI flicker :("));
        // ImGui::Checkbox(_("Use Locked On Enemy Instead Of Picking"), &useLockedOnEnemyInstead);

        ImGui::Spacing();

        SMediator* s_med_ptr = devil4_sdk::get_sMediator();

        /*if (ImGui::Button(_("Save Selected Enemy Info"))) {
            if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                save_load_enemy_info(true, enemy);
        }

        if (ImGui::Button(_("Replay Saved Move ID & Position"))) {
            if (auto enemy = GetDesiredEnemy(useLockedOnEnemyInstead))
                save_load_enemy_info(false, enemy);
        }*/

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
        if (auto currentEnemy = GetDesiredEnemy(false)) {
            ImGui::Indent(lineIndent);
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

            // i hate this, game accesses them from base ptr, e.g. [uEnemy+1544] for scarecrow hp
            int damage_info_offset = get_enemy_specific_damage_offset(currentEnemy->ID);
            uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)currentEnemy + damage_info_offset);
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::InputFloat(_("HP##2"), &currentEnemyDamage->HP);
            ImGui::InputFloat(_("Max HP##2"), &currentEnemyDamage->HPMax);
            ImGui::InputFloat(_("Previous Hit Dealt"), &currentEnemyDamage->HPTaken);
            ImGui::InputInt(_("Stun 1##2"), &currentEnemyDamage->stun[0]);
            ImGui::InputInt(_("Stun 2##2"), &currentEnemyDamage->stun[1]);
            ImGui::InputInt(_("Stun 3##2"), &currentEnemyDamage->stun[2]);
            ImGui::InputInt(_("Stun 4##2"), &currentEnemyDamage->stun[3]);
            ImGui::InputInt(_("Stun 5##2"), &currentEnemyDamage->stun[4]);
            ImGui::InputInt(_("Displacement 1##2"), &currentEnemyDamage->displacement[0]);
            ImGui::InputInt(_("Displacement 2##2"), &currentEnemyDamage->displacement[1]);
            ImGui::InputInt(_("Displacement 3##2"), &currentEnemyDamage->displacement[2]);
            ImGui::InputInt(_("Displacement 4##2"), &currentEnemyDamage->displacement[3]);
            ImGui::InputInt(_("Displacement 5##2"), &currentEnemyDamage->displacement[4]);
            ImGui::InputInt(_("Unknown 1##2"), &currentEnemyDamage->unknown[0]);
            ImGui::InputInt(_("Unknown 2##2"), &currentEnemyDamage->unknown[1]);
            ImGui::InputInt(_("Unknown 3##2"), &currentEnemyDamage->unknown[2]);
            ImGui::InputInt(_("Unknown 4##2"), &currentEnemyDamage->unknown[3]);
            ImGui::InputInt(_("Unknown 5##2"), &currentEnemyDamage->unknown[4]);
            ImGui::InputInt(_("Unknown 6##2"), &currentEnemyDamage->unknown[5]);
            ImGui::InputInt(_("Unknown 7##2"), &currentEnemyDamage->unknown[6]);
            ImGui::InputInt(_("Unknown 8##2"), &currentEnemyDamage->unknown[7]);

            ImGui::InputFloat3(_("XYZ Position##2"), (float*)&currentEnemy->position);
            ImGui::InputFloat3(_("XYZ Rotation##2"), (float*)&currentEnemy->rotation);
            ImGui::InputFloat3(_("XYZ Velocity##2"), (float*)&currentEnemy->velocity);
            ImGui::InputFloat3(_("XYZ Scale##2"), (float*)&currentEnemy->scale);
            ImGui::InputScalar(_("Move ID##2"), ImGuiDataType_U8, &currentEnemy->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputScalar(_("Move ID 2##2"), ImGuiDataType_U8, &currentEnemy->animID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputScalar(_("Move Part##2"), ImGuiDataType_U8, &currentEnemy->movePart);
            ImGui::InputScalar(_("Grounded##2"), ImGuiDataType_U8, &currentEnemy->grounded);
            ImGui::InputFloat(_("Animation Frame##2"), &currentEnemy->animFrame);
            if (ImGui::CollapsingHeader(_("Saved Info"))) {
                ImGui::InputScalar(_("Enemy Move ID"), ImGuiDataType_U8, &savedEnemyMoveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::InputScalar(_("Enemy Move ID 2"), ImGuiDataType_U8, &savedEnemyAnimID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::InputFloat3(_("Enemy Position"), savedEnemyPosition);
                ImGui::InputFloat(_("Enemy Rotation"), &savedEnemyRotation);
                ImGui::InputFloat3(_("Enemy Velocity"), savedEnemyVelocity);
                ImGui::InputScalar(_("Enemy Grounded"), ImGuiDataType_U8, &savedEnemyGrounded);
            }
            ImGui::PopItemWidth();
            ImGui::Unindent(lineIndent);
        }
    }
    if (ImGui::CollapsingHeader(_("Display Boss Stats"))) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
        if (s_med_ptr) {
            if (s_med_ptr->uBoss1) {
                ImGui::Spacing();
                ImGui::InputFloat3(_("XYZ Position##3"), (float*)&s_med_ptr->uBoss1->position);
                ImGui::InputFloat4(_("XYZ Rotation##3"), (float*)&s_med_ptr->uBoss1->rotation);
                ImGui::InputFloat3(_("XYZ Scale##3"), (float*)&s_med_ptr->uBoss1->scale);
                ImGui::InputFloat(_("HP##3"), &s_med_ptr->uBoss1->HP);
                ImGui::InputFloat(_("Max HP##3"), &s_med_ptr->uBoss1->HPMax);
                ImGui::InputScalar(_("Move ID##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::InputScalar(_("Move Part##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->movePart, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::InputScalar(_("Move ID 2##3"), ImGuiDataType_U8, &s_med_ptr->uBoss1->animID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                ImGui::InputFloat(_("Animation Frame##3"), &s_med_ptr->uBoss1->animFrame);

                if (ImGui::Button(_("Save State##3"))) {
                    save_load_boss_info(true);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is PAGE UP by default"));

                if (ImGui::Button(_("Load State##3"))) {
                    save_load_boss_info(false);
                }
                ImGui::SameLine();
                help_marker(_("Hotkey is PAGE DOWN by default"));
            }
        }
        if (ImGui::CollapsingHeader(_("Saved Info##Boss"))) {
            ImGui::InputScalar(_("Enemy Move ID##Boss"), ImGuiDataType_U8, &savedEnemyMoveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputScalar(_("Enemy Move ID 2##Boss"), ImGuiDataType_U8, &savedEnemyAnimID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
            ImGui::InputFloat3(_("Enemy Position##Boss"), savedEnemyPosition);
            ImGui::InputFloat(_("Enemy Rotation##Boss"), &savedEnemyRotation);
            ImGui::InputFloat3(_("Enemy Velocity##Boss"), savedEnemyVelocity);
            ImGui::InputScalar(_("Enemy Grounded##Boss"), ImGuiDataType_U8, &savedEnemyGrounded);
        }
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }

    ImGui::Checkbox(_("Enable Save/Load hotkeys"), &hotkey_enabled);
    ImGui::SameLine();
    help_marker(_("Assuming default hotkeys,\nHome+End will save and load enemy attacks\nPage Up+Page Down will save and load boss attacks"));

    ImGui::Checkbox(_("Flying Enemy Stats"), &flyingEnemyStats);
    ImGui::SameLine();
    help_marker(_("Render ImGui stats on enemies"));
    if (flyingEnemyStats) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox("Display HP", &showFlyingEnemyHP);
        ImGui::Checkbox("Display Damage Taken", &showFlyingEnemyDamageTaken);
        ImGui::Checkbox("Display DT Timer", &showFlyingEnemyDT);
        ImGui::Checkbox("Display Stun", &showFlyingEnemyStun);
        ImGui::Checkbox("Display Displacement", &showFlyingEnemyDisplacement);
        // ImGui::Checkbox("Stun Timer", &showFlyingEnemyStunTimer);
        ImGui::Checkbox("Display Move ID", &showFlyingEnemyMoveID);
        ImGui::Checkbox("Display Shield", &showFlyingEnemyShield);
        ImGui::Unindent(lineIndent);
    }
}

class InteractiveModel {
private:
    glm::vec3 position;
    float size;
    float rotation;
    ImU32 color;
    bool isSelected;
    float hoverHeight;
    float bobSpeed;
    float initialY;
    std::chrono::time_point<std::chrono::steady_clock> lastClickTime;
    int colorCycleIndex;
    
    const std::vector<ImU32> colorPalette = {
        IM_COL32(255, 0, 0, 255),   // Red
        IM_COL32(0, 255, 0, 255),   // Green
        IM_COL32(0, 0, 255, 255),   // Blue
        IM_COL32(255, 255, 0, 255), // Yellow
        IM_COL32(255, 0, 255, 255), // Magenta
        IM_COL32(0, 255, 255, 255)  // Siyan
    };

public:
    InteractiveModel(const glm::vec3& pos, float modelSize) 
        : position(pos), 
          size(modelSize), 
          rotation(0.0f), 
          color(IM_COL32(255, 255, 255, 255)), 
          isSelected(false), 
          hoverHeight(20.0f),
          bobSpeed(1.0f),
          initialY(pos.y),
          lastClickTime(std::chrono::steady_clock::now()),
          colorCycleIndex(0) {}

    void Update() {
        rotation += 0.01f;
        if (rotation > 2.0f * glm::pi<float>()) {
            rotation -= 2.0f * glm::pi<float>();
        }
        auto currentTime = std::chrono::steady_clock::now();
        float timeElapsed = std::chrono::duration<float>(currentTime - lastClickTime).count();
        position.y = initialY + sin(timeElapsed * bobSpeed) * hoverHeight;
    }
    
    bool CheckClick(const glm::vec2& mousePos) {
        glm::vec2 screenPos = w2s::WorldToScreen(position);
        float distance = glm::distance(mousePos, screenPos);
        float distFromCamera = w2s::GetDistanceFromCam(position);
        float screenRadius = size * 1000.0f / distFromCamera;
        
        if (distance < screenRadius) {
            colorCycleIndex = (colorCycleIndex + 1) % colorPalette.size();
            color = colorPalette[colorCycleIndex];
            lastClickTime = std::chrono::steady_clock::now();
            bobSpeed = 2.0f;
            isSelected = !isSelected;
            return true;
        }
        
        bobSpeed = glm::mix(bobSpeed, 1.0f, 0.01f);
        
        return false;
    }
    
    void Draw() {
        if (!w2s::IsVisibleOnScreen(position, size)) {
            return;
        }
        w2s::DrawWireframeSphere(position, size, rotation, color, 16, isSelected ? 2.0f : 1.0f);
        
        if (isSelected) {
            ImU32 highlightColor = IM_COL32(255, 255, 255, 100);
            w2s::DrawWireframeCube(position, size * 2.0f, rotation, highlightColor, 0.5f);
        }
    }
};

void RenderInteractiveModel() {
    static bool initialized = false;
    static InteractiveModel* model = nullptr;
    
    if (!initialized) {
        model = new InteractiveModel(glm::vec3(0.0f, 0.0f, 10.0f), 100.0f);
        initialized = true;
    }

    ImGuiIO& io = ImGui::GetIO();
    glm::vec2 mousePos(io.MousePos.x, io.MousePos.y);
    static bool wasPressed = false;
    bool isPressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (isPressed && !wasPressed) {
        model->CheckClick(mousePos);

    }
    wasPressed = isPressed;
    model->Update();
    model->Draw();
}

void EnemyTracker::RenderExample() {
    if (!g_pd3dDevice || !m_texture_handle)
        return;
    if (SUCCEEDED(g_pd3dDevice->BeginScene())) {
        // Get texture dimensions
        D3DSURFACE_DESC desc;
        if (SUCCEEDED(m_texture_handle->GetLevelDesc(0, &desc))) {
            glm::vec3 texturePos(1200.0f, 400.0f, 1200.0f);
            SMediator* sMed = devil4_sdk::get_sMediator();
            uCameraCtrl* camera = sMed->camera1;
            glm::mat4 viewMatrix = glm::lookAt(camera->pos, camera->lookat, camera->up);
            float width = static_cast<float>(desc.Width) * 2.0f;
            float height = static_cast<float>(desc.Height) * 2.0f;
            float yRotationDegrees = 220.0f;
            w2s::DrawTextureWithPerspective(g_pd3dDevice, m_texture_handle, texturePos, width, height, viewMatrix, yRotationDegrees);
        }
        g_pd3dDevice->EndScene();
    }
    g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

void EnemyTracker::on_frame(fmilliseconds& dt) {
    if (SMediator* sMedPtr = devil4_sdk::get_sMediator()) {
        if (sMedPtr->player_ptr) {
            if (flyingEnemyStats) {
                if (sMedPtr->uEnemies[0]) {
                    for (uint32_t i = 0; i < sMedPtr->enemyCount[2]; ++i) {
                        if (uEnemy* enemy = sMedPtr->uEnemies[i]) {
                            glm::vec3 objectPosition = enemy->position;
                            float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                            float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                            glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                            std::string windowName = "EnemyStats##" + std::to_string(i);
                            if (w2s::IsVisibleOnScreen(objectPosition)) {
                                ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                                ImGui::SetWindowPos(screenPos);
                                ImGui::SetWindowFontScale(1.0f * guiFriendlyDistance);
                                uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + get_enemy_specific_damage_offset(enemy->ID));
                                ImGui::PushItemWidth((sameLineItemWidth / 2.5f) * guiFriendlyDistance);
                                if (showFlyingEnemyHP) ImGui::SliderFloat("HP##EnemyFly", &currentEnemyDamage->HP, 0.0f, currentEnemyDamage->HPMax, "%.0f");
                                if (showFlyingEnemyDamageTaken) ImGui::InputFloat("Damage##EnemyFly", &currentEnemyDamage->HPTaken, NULL, NULL, "%.0f");
                                if (showFlyingEnemyDT) ImGui::InputFloat("DT Timer##EnemyFly", &enemy->DTTimer, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                                if (showFlyingEnemyStun) ImGui::InputInt("Stun##EnemyFly", &currentEnemyDamage->stun[0], NULL, NULL);
                                if (showFlyingEnemyDisplacement) ImGui::InputInt("Displacement##EnemyFly", &currentEnemyDamage->displacement[0], NULL, NULL);
                                if (showFlyingEnemyStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &currentEnemyDamage->stunResetTimer, 0.0f, 180.0f, "%.0f");
                                if (showFlyingEnemyMoveID) ImGui::InputScalar("MoveID##EnemyFly", ImGuiDataType_U8, &enemy->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                                if (showFlyingEnemyShield) {
                                    if (enemy->ID == 5 || enemy->ID == 6) { // Bianco, Alto
                                        ImGui::SliderFloat("Shield##EnemyFly", &enemy->angeloShield, 0.0f, enemy->angeloShieldMax, "%.0f");
                                    }
                                    if (enemy->ID == 8 || enemy->ID == 9) { // Mephisto, Faust
                                        if (enemy->faustCloak > 0.0f) {
                                            ImGui::SliderFloat("Cloak##EnemyFly", &enemy->faustCloak, 0.0f, enemy->faustCloakMax, "%.0f");
                                        }
                                        else {
                                            ImGui::InputFloat("Cloak Timer##EnemyFly", &enemy->faustCloakTimer, NULL, NULL, "%.0f");
                                        }
                                    }
                                    if (enemy->ID == 12) { // Blitz
                                        if (enemy->blitzElectric > 0.0f) {
                                            ImGui::SliderFloat("Electric##EnemyFly", &enemy->blitzElectric, 0.0f, 1000.0f, "%.0f");
                                        }
                                        else {
                                            if (currentEnemyDamage->HP < enemy->blitzElectricSuicideHPRequirement && enemy->blitzElectricSuicideTimer > 0.0f) {
                                                ImGui::SliderFloat("Suicide Timer##EnemyFly", &enemy->blitzElectricSuicideTimer, 0.0f, 1800.0f, "%.0f");
                                            }
                                            else {
                                                ImGui::SliderFloat("Electric Timer##EnemyFly", &enemy->blitzElectricTimer, 0.0f, 900.0f, "%.0f");
                                            }
                                        }
                                    }
                                }
                                ImGui::PopItemWidth();
                                ImGui::End();
                            }
                        }
                    }
                }
                if (sMedPtr->uBoss1) {
                    glm::vec3 objectPosition = sMedPtr->uBoss1->position;
                    float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                    float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                    glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                    if (w2s::IsVisibleOnScreen(objectPosition)) {
                        ImGui::Begin("BossStats##1", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                        ImGui::SetWindowPos(screenPos);
                        ImGui::SetWindowFontScale(1.0f * guiFriendlyDistance);
                        ImGui::PushItemWidth((sameLineItemWidth / 2.5f) * guiFriendlyDistance);
                        if (sMedPtr->uBoss1->ID == 29 || sMedPtr->uBoss1->ID == 30) { // Sanctus M19/M20
                            if (showFlyingEnemyHP) ImGui::SliderFloat("HP##BossFly", &sMedPtr->uBoss1->sanctusHP, 0.0f, sMedPtr->uBoss1->sanctusHPMax, "%.0f");
                        }
                        else {
                            if (showFlyingEnemyHP) ImGui::SliderFloat("HP##BossFly", &sMedPtr->uBoss1->HP, 0.0f, sMedPtr->uBoss1->HPMax, "%.0f");
                        }
                        if (showFlyingEnemyDamageTaken) ImGui::InputFloat("Damage##BossFly", &sMedPtr->uBoss1->HPTaken, NULL, NULL, "%.0f");
                        if (showFlyingEnemyDT) ImGui::InputFloat("DT Timer##BossFly", &sMedPtr->uBoss1->DTTimer, NULL, NULL, "%.0f");
                        if (showFlyingEnemyMoveID) ImGui::InputScalar("MoveID##BossFly", ImGuiDataType_U8, &sMedPtr->uBoss1->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                        if (showFlyingEnemyShield) {
                            if (sMedPtr->uBoss1->ID == 18) { // Berial
                                if (sMedPtr->uBoss1->berialFireTimer > 0.0f) {
                                    ImGui::InputFloat("Fire Timer##BossFly", &sMedPtr->uBoss1->berialFireTimer, NULL, NULL, "%.0f");
                                }
                                else {
                                    ImGui::SliderFloat("Fire Damage##BossFly", &sMedPtr->uBoss1->berialFire, 0.0f, sMedPtr->uBoss1->berialFireMax, "%.0f");
                                }
                            }
                            if (sMedPtr->uBoss1->ID == 22) { // Credo
                                if (sMedPtr->uBoss1->credoShield > 0.0f) {
                                    ImGui::SliderFloat("Shield##BossFly", &sMedPtr->uBoss1->credoShield, 0.0f, 4000.0f, "%.0f");
                                }
                                else {
                                    ImGui::InputFloat("Shield Timer##BossFly", &sMedPtr->uBoss1->credoShieldTimer, NULL, NULL, "%.0f");
                                }
                            }
                            if (sMedPtr->uBoss1->ID == 29) { // Sanctus M11
                                if (sMedPtr->uBoss1->sanctusShieldTimerM11 > 0.0f) {
                                    ImGui::InputFloat("Shield Timer##BossFly", &sMedPtr->uBoss1->sanctusShieldTimerM11, NULL, NULL, "%.0f");
                                }
                                else {
                                    ImGui::SliderFloat("Shield Damage##BossFly", &sMedPtr->uBoss1->sanctusShieldM11, 0.0f, 720.0f, "%.0f");
                                }
                            }
                            if (sMedPtr->uBoss1->ID == 30) { // Sanctus M20
                                if (sMedPtr->uBoss1->sanctusShieldTimerM20 > 0.0f) {
                                    ImGui::InputFloat("Shield Timer##BossFly", &sMedPtr->uBoss1->sanctusShieldTimerM20, NULL, NULL, "%.0f");
                                }
                                else {
                                    ImGui::SliderFloat("Shield Damage##BossFly", &sMedPtr->uBoss1->sanctusShieldM20, 0.0f, 600.0f, "%.0f");
                                }
                            }
                        }
                        ImGui::PopItemWidth();
                        ImGui::End();
                    }
                }
            }
            if (flyingSpheres) {
                glm::vec3 playerPos = glm::vec3(sMedPtr->player_ptr->m_pos.x, sMedPtr->player_ptr->m_pos.y, sMedPtr->player_ptr->m_pos.z);
                glm::vec3 playerRot = glm::vec3(0.0f, sMedPtr->player_ptr->rotation2, 0.0f);
                glm::mat4 rotMatrix = w2s::CreateRotationMatrix(playerRot.x, playerRot.y, playerRot.z);
                glm::vec3 forward = glm::vec3(rotMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
                glm::vec3 right = glm::vec3(rotMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
                glm::vec3 up = glm::vec3(rotMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

                glm::vec3 objectPos = playerPos + forward * 0.0f + up * 0.0f;

                w2s::DrawWireframeCube(objectPos, 50.0f, playerRot.y, IM_COL32(255, 0, 0, 255), 1.5f);
                w2s::DrawWireframeSphere(objectPos, 50.0f, playerRot.y, IM_COL32(255, 255, 000, 255), 16, 1.0f);
                w2s::DrawLine3D(playerPos, playerPos + forward * 50.0f, IM_COL32(000, 000, 255, 255), 2.0f);
                w2s::DrawLine3D(playerPos, playerPos + right   * 50.0f, IM_COL32(255, 000, 000, 255), 2.0f);
                w2s::DrawLine3D(playerPos, playerPos + up      * 50.0f, IM_COL32(000, 255, 000, 255), 2.0f);

                w2s::DrawWireframeCapsule(objectPos, 25.0f, 100.0f, 1.57f, playerRot.y, 0.0f, IM_COL32(255, 255, 000, 255), 16, 1.0f);

                RenderInteractiveModel();

                std::vector<glm::vec3> vertices = {
                    {0, 0, 0},      // base 1
                    {100, 0, 0},    // base 2
                    {100, 0, 100},  // base 3
                    {0, 0, 100},    // base 4
                    {50, 100, 50}   // apex
                };
                std::vector<std::vector<int>> triangles = {
                    {0, 1, 4}, {1, 2, 4}, {2, 3, 4}, {3, 0, 4}, // sides
                    {0, 2, 1}, {0, 3, 2}  // base
                };

                w2s::DrawWireframeMesh(vertices, triangles, IM_COL32(255, 255, 0, 255), 1.0f);
                
                /*std::vector<int> indices = {
                    0, 1, 4,   // first triangle
                    1, 2, 4,   // second triangle
                    2, 3, 4,   // third triangle
                    3, 0, 4,   // fourth triangle
                    0, 2, 1,   // fifth triangle (base)
                    0, 3, 2    // sixth triangle (base)
                };

                w2s::DrawWireframeMeshIndices(vertices, indices, IM_COL32(255, 255, 0, 255), 1.0f);*/
            }
            if (flyingAd) {
                RenderExample();
            }
        }
    }
}

void EnemyTracker::on_reset() {
    if(m_texture_handle != nullptr) {
        m_texture_handle->Release();
        m_texture_handle = nullptr;
    }
}

void EnemyTracker::after_reset() {
    auto [data, size] = utility::decompress_file_from_memory_with_size(drawing_compressed_data, drawing_compressed_size);

    if (!utility::dx9::load_texture_from_file(data, size, &m_texture_handle, &m_texture_width, &m_texture_height)) {
        spdlog::error("Failed to unpack and load compressed texture");
    }
    free(data);
}

void EnemyTracker::on_update_input(utility::Input& input) {
    if (hotkey_enabled) {
        if (m_hotkeys[SAVE_ENEMY_STATS_HOTKEY]->check(input)) {
            SaveStateWithCurrentEnemy();
        }
        if (m_hotkeys[APPLY_ENEMY_STATS_HOTKEY]->check(input)) {
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
    // useLockedOnEnemyInstead = cfg.get<bool>("enable_enemy_stats_lockon").value_or(false);
    flyingSpheres = cfg.get<bool>("flyingSpheres").value_or(false);
    flyingAd = cfg.get<bool>("flyingAd").value_or(false);
    flyingEnemyStats = cfg.get<bool>("flyingEnemyStats").value_or(false);

    showFlyingEnemyHP = cfg.get<bool>("showFlyingEnemyHP").value_or(true);
    showFlyingEnemyDT = cfg.get<bool>("showFlyingEnemyDT").value_or(false);
    showFlyingEnemyDamageTaken = cfg.get<bool>("showFlyingEnemyDamageTaken").value_or(false);
    showFlyingEnemyMoveID = cfg.get<bool>("showFlyingEnemyMoveID").value_or(false);
    showFlyingEnemyStun = cfg.get<bool>("showFlyingEnemyStun").value_or(false);
    showFlyingEnemyDisplacement = cfg.get<bool>("showFlyingEnemyDisplacement").value_or(false);
    showFlyingEnemyStunTimer = cfg.get<bool>("showFlyingEnemyStunTimer").value_or(false);
    showFlyingEnemyShield = cfg.get<bool>("showFlyingEnemyShield").value_or(false);
}

void EnemyTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enable_enemy_stats_hotkeys", hotkey_enabled);
    // cfg.set<bool>("enable_enemy_stats_lockon", useLockedOnEnemyInstead);
    cfg.set<bool>("flyingSpheres", flyingSpheres);
    cfg.set<bool>("flyingAd", flyingAd);
    cfg.set<bool>("flyingEnemyStats", flyingEnemyStats);
    cfg.set<bool>("showFlyingEnemyHP", showFlyingEnemyHP);
    cfg.set<bool>("showFlyingEnemyDT", showFlyingEnemyDT);
    cfg.set<bool>("showFlyingEnemyDamageTaken", showFlyingEnemyDamageTaken);
    cfg.set<bool>("showFlyingEnemyMoveID", showFlyingEnemyMoveID);
    cfg.set<bool>("showFlyingEnemyStun", showFlyingEnemyStun);
    cfg.set<bool>("showFlyingEnemyShield", showFlyingEnemyShield);
    cfg.set<bool>("showFlyingEnemyStunTimer", showFlyingEnemyStunTimer);
}
#endif
