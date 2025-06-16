#include "VisualizeHitbox.hpp"
#include "./sdk/World2Screen.hpp"
#include "../sdk/uActor.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#include "EnemyTracker.hpp"

bool VisualizeHitbox::mod_enabled = false; // Visualize Hitboxes
constexpr uintptr_t sMainAddr = 0x00E5574C;
static bool enemyStepSphereDebug;

bool VisualizeHitbox::mod_enabled2 = false; // Visualize Pushboxes
// uintptr_t VisualizeHitbox::jmp_ret_pushboxes = NULL;
// static uintptr_t pushboxAddr = NULL;

bool VisualizeHitbox::mod_enabled3 = false; // Visualize JC Spheres

struct kCollPrim {
    int mType;
    int Bone0;
    int Bone1;
    float radius;
    uActorMain::MtVector3 Pos0;
    uActorMain::MtVector3 Pos1;
    uint32_t ukn0;
    uint32_t mId;
    float mShrink;
    uint32_t ukn1;
};

struct cCollision {
    uint32_t ukn1;
    kCollPrim* mpCollPrim; /* Created by retype action */
    float scale;
    int mAttackUniqueID;
    uActorMain::MtVector3 UknVec1;
    uActorMain::MtVector3 UknVec2;
    uint32_t padding0;
    struct cCollisionGroup* mpCollisionGroup;
    uint32_t padding1[2];
    MtMatrix mMat0;
    MtMatrix mMat1;
    MtMatrix mOldMat0;
    MtMatrix* mpRefMat0;
    MtMatrix* mpRefMat1;
    byte mUseOld;
};

struct CollisionGroup {
    char pad_0[0x94];
    uint32_t collisionCount;     // 0x94
    char pad_98[4];
    cCollision** collisionArray; // 0x9C
};

struct CollisionGroupsContainer {
    struct GroupEntry {
        CollisionGroup* groupPointer;
        char pad_04[0xC];
    }; // 0x10
    
    char pad_0x0[0x20];
    GroupEntry groups[8];
};

struct sMain {
    char pad_0[0x10340];
    CollisionGroupsContainer* pCollisionGroupsContainer;
};

void DisplayEnemyStepSpheres(uEnemy* enemy) {
    while (enemy) {
        ImGui::PushID((void*)enemy);
        if (!enemy->enemyStepSphereArray || !enemy->joints) {
            ImGui::PopID();
            // enemyCount++;
            enemy = enemy->nextEnemy;
            continue;
        }
        for (int i = 0; i < 30; i++) {
            kEmJumpData* sphere = &enemy->enemyStepSphereArray->enemyStepSphere[i];
            if (!sphere) break;
            if (sphere->jointNo == -1) break;

            UModelJoint* joint = nullptr;
            for (int j = 0; j < enemy->m_joint_array_size; j++) {
                if (enemy->joints->joint[j].mNo == sphere->jointNo) {
                    joint = &enemy->joints->joint[j];
                    break;
                }
            }

            if (!joint) continue;

            float uniformScale = (joint->mScale.x + joint->mScale.y + joint->mScale.z) / 3.0f;
            Vector3f sphereOffset = glm::make_vec3((float*)&sphere->offset);
            Vector3f jointPos = Vector3f(joint->mWmat.m4.x, joint->mWmat.m4.y, joint->mWmat.m4.z);
            Vector3f finalPos = jointPos + (sphereOffset * uniformScale);
            float scaledRadius = sphere->radius * uniformScale;

            w2s::DrawWireframeCapsule(finalPos, scaledRadius, 0.0f, 0.0f, 0.0f, 0.0f, IM_COL32(0, 255, 0, 255), 16, 1.0f);
            if (enemyStepSphereDebug) {
                ImGui::PushID(i);
                // ImGui::Text("Enemy %d, Sphere %d/%d, Joint %d (index %d)", enemyCount, i, 30, sphere->jointNo, (int)(joint - enemy->joints->joint));
                ImGui::SliderFloat3("Enemy Pos", &enemy->position.x, -500.0f, 500.0f, "%.2f");
                ImGui::SliderInt("Joint No Desired", &sphere->jointNo, 0, enemy->m_joint_array_size - 1);
                ImGui::InputScalar("Joint No Got", ImGuiDataType_S8, &joint->mNo);
                ImGui::SliderFloat3("Joint Offset", &joint->mOffset.x, -500.0f, 500.0f, "%.2f");
                ImGui::SliderFloat3("Joint Scale", &joint->mScale.x, 0.1f, 5.0f, "%.2f");
                ImGui::InputFloat3("Joint Final Pos", &finalPos.x, "%.2f");
                ImGui::SliderFloat3("Sphere Offset", (float*)&sphere->offset, -500.0f, 500.0f, "%.2f");
                ImGui::SliderFloat("Sphere Radius", &sphere->radius, 0.0f, 500.0f, "%.2f");
                ImGui::InputFloat3("Final Pos", &finalPos.x, "%.2f");
                ImGui::InputFloat("Scaled Radius", &scaledRadius, 0.0f, 0.0f, "%.2f");
                ImGui::InputInt("Pad 08 (0-3)", (int*)&sphere->pad_08[0]);
                ImGui::InputInt("Pad 08 (4-7)", (int*)&sphere->pad_08[4]);
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        ImGui::PopID();
        // enemyCount++;
        enemy = enemy->nextEnemy;
    }
}

void VisualizeHitbox::on_frame(fmilliseconds& dt) {
    if (mod_enabled) { // hitboxes
        sMain* sMainPtr = *(sMain**)sMainAddr;
        if (!sMainPtr || !sMainPtr->pCollisionGroupsContainer) return;
        CollisionGroupsContainer* groupsContainer = sMainPtr->pCollisionGroupsContainer;
        for (int i = 0; i < 8; i++) {
            CollisionGroup* group = groupsContainer->groups[i].groupPointer;
            if (!group) continue;
            uint32_t collNum = group->collisionCount;
            cCollision** cColl = group->collisionArray;
            if (collNum == 0 || cColl == nullptr) continue;
            for (uint32_t j = 0; j < collNum; j++) {
                cCollision* collision = cColl[j];
                if (!collision || !collision->mpCollPrim) continue;
                kCollPrim* prim = collision->mpCollPrim;
                Matrix4x4 worldMatrix = glm::make_mat4((float*)&collision->mMat0);
                if (prim->mType == 3) {
                    Vector3f pos1 = glm::make_vec3((float*)&prim->Pos0);
                    Vector3f pos2 = glm::make_vec3((float*)&prim->Pos1);
                    float length = glm::length(pos2 - pos1);
                    length = (length > 0.00001f) ? length : 0.01f;
                    Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                    Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                    Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                    float rotationY = atan2(direction.x, direction.z);
                    float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                    w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), prim->radius, length, rotationX, rotationY, 0.0f, IM_COL32(255, 0, 0, 255), 16, 1.0f);
                }
                else {
                    Vector3f position = glm::make_vec3((float*)&prim->Pos0);
                    Matrix4x4 worldPos = glm::translate(worldMatrix, position);
                    w2s::DrawWireframeSphere(glm::vec3(worldPos[3]), prim->radius, 0.0f, IM_COL32(255, 0, 0, 255), 16, 1.0f);
                }
            }
        }
    }

    if (mod_enabled2) { // pushpills
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            uEnemy* enemy = devil4_sdk::get_uEnemies();
            int enemyCount = 0;
            while (enemy) {
                uCollisionMgr currentEnemyCollision = *(uCollisionMgr*)((char*)enemy + EnemyTracker::get_enemy_specific_uCollision_offset(enemy->ID));
                {
                    // mPushCap
                    Matrix4x4 worldMatrix = glm::make_mat4((float*)&currentEnemyCollision.field50_0x1c0);
                    Vector3f pos1 = glm::make_vec3((float*)&currentEnemyCollision.mPushCap.p0);
                    Vector3f pos2 = glm::make_vec3((float*)&currentEnemyCollision.mPushCap.p1);
                    float length = glm::length(pos2 - pos1);
                    length = (length > 0.00001f) ? length : 0.01f;
                    Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                    Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                    Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                    float rotationY = atan2(direction.x, direction.z);
                    float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                    w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), currentEnemyCollision.mPushCap.r, length, rotationX, rotationY, 0.0f, IM_COL32(0, 0, 255, 255), 16, 1.0f);
                }
                {
                    // mPushCap1
                    Matrix4x4 worldMatrix = glm::make_mat4((float*)&currentEnemyCollision.field51_0x200);
                    Vector3f pos1 = glm::make_vec3((float*)&currentEnemyCollision.mPushCap1.p0);
                    Vector3f pos2 = glm::make_vec3((float*)&currentEnemyCollision.mPushCap1.p1);
                    float length = glm::length(pos2 - pos1);
                    length = (length > 0.00001f) ? length : 0.01f;
                    Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                    Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                    Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                    float rotationY = atan2(direction.x, direction.z);
                    float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                    w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), currentEnemyCollision.mPushCap1.r, length, rotationX, rotationY, 0.0f, IM_COL32(0, 0, 255, 255), 16, 1.0f);
                }
                enemyCount++;
                enemy = enemy->nextEnemy;
            }
            // player
            {
                // mPushCap
                Matrix4x4 worldMatrix = glm::make_mat4((float*)&player->collisionSettings->field50_0x1c0);
                Vector3f pos1 = glm::make_vec3((float*)&player->collisionSettings->mPushCap.p0);
                Vector3f pos2 = glm::make_vec3((float*)&player->collisionSettings->mPushCap.p1);
                float length = glm::length(pos2 - pos1);
                length = (length > 0.00001f) ? length : 0.01f;
                Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                float rotationY = atan2(direction.x, direction.z);
                float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), player->collisionSettings->mPushCap.r, length, rotationX, rotationY, 0.0f, IM_COL32(0, 0, 255, 255), 16, 1.0f);
            }
            {
                // mPushCap1
                Matrix4x4 worldMatrix = glm::make_mat4((float*)&player->collisionSettings->field51_0x200);
                Vector3f pos1 = glm::make_vec3((float*)&player->collisionSettings->mPushCap1.p0);
                Vector3f pos2 = glm::make_vec3((float*)&player->collisionSettings->mPushCap1.p1);
                float length = glm::length(pos2 - pos1);
                length = (length > 0.00001f) ? length : 0.01f;
                Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                float rotationY = atan2(direction.x, direction.z);
                float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), player->collisionSettings->mPushCap1.r, length, rotationX, rotationY, 0.0f, IM_COL32(0, 0, 255, 255), 16, 1.0f);
            }
        }
    }

    if (mod_enabled3) { // enemy step
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            uEnemy* enemy = devil4_sdk::get_uEnemies();
            DisplayEnemyStepSpheres(enemy);
            uEnemy* object = devil4_sdk::get_objects();
            DisplayEnemyStepSpheres(object);
        // player
            Vector3f playerPos = glm::make_vec3((float*)&player->m_pos);
            Vector3f playerSphereOffset { 0.0f, 85.0f, 0.0f }; // from DevilMayCry4_DX9.exe+AB322
            Vector3f finalPos = playerPos + playerSphereOffset;
            w2s::DrawWireframeCapsule(finalPos, 2.0f, 0.0f, 0.0f, player->rotation2, 0.0f, IM_COL32(0, 255, 0, 255), 16, 1.0f);
        }
    }
}

// they just stutter between different entities with this,
// and while it does find enemies like berial, pills are still not displayed
/*naked void detour_pushboxes() {
    _asm {
            cmp byte ptr [VisualizeHitbox::mod_enabled2], 1
            jne originalcode

            mov dword ptr [pushboxAddr], edi

        originalcode:
            lea ebx,[edi+0x00000358]
            jmp dword ptr [VisualizeHitbox::jmp_ret_pushboxes]
    }
}*/

std::optional<std::string> VisualizeHitbox::on_initialize() {
    /*if (!install_hook_offset(0x10CE33, hook1, &detour_pushboxes, &jmp_ret_pushboxes, 6)) {
        spdlog::error("Failed to init detour_pushboxes\n");
        return "Failed to init detour_pushboxes";
	}*/

    return Mod::on_initialize();
}

void VisualizeHitbox::on_gui_frame(int display) {
    ImGui::Checkbox(_("Visualize Hitboxes"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Draw hitbox outlines in red"));

    ImGui::Checkbox(_("Visualize Pushboxes"), &mod_enabled2);
    ImGui::SameLine();
    help_marker(_("Draw pushbox outlines in blue"));

    ImGui::Checkbox(_("Visualize Enemy Step Spheres"), &mod_enabled3);
    ImGui::SameLine();
    help_marker(_("Draw enemy step sphere outlines in green"));

    ImGui::Indent(lineIndent);
    ImGui::Checkbox(_("Debug Stats##EnemyStepSpheres"), &enemyStepSphereDebug);
    ImGui::Unindent();
}

void VisualizeHitbox::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("visualize_hitbox").value_or(false);
    mod_enabled2 = cfg.get<bool>("visualize_pushbox").value_or(false);
    mod_enabled3 = cfg.get<bool>("visualize_enemystep").value_or(false);
};

void VisualizeHitbox::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("visualize_hitbox", mod_enabled);
    cfg.set<bool>("visualize_pushbox", mod_enabled2);
    cfg.set<bool>("visualize_enemystep", mod_enabled3);
};
