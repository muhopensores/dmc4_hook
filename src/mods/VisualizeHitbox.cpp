#include "VisualizeHitbox.hpp"
#include "./sdk/World2Screen.hpp"
#include "../sdk/uActor.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#include "EnemyTracker.hpp"

bool VisualizeHitbox::mod_enabled = false;
constexpr uintptr_t sMainAddr = 0x00E5574C;

bool VisualizeHitbox::mod_enabled2 = false;
// uintptr_t VisualizeHitbox::jmp_ret_pushboxes = NULL;
// static uintptr_t pushboxAddr = NULL;

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

void VisualizeHitbox::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
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

    if (mod_enabled2) {
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
}

void VisualizeHitbox::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("visualize_hitbox").value_or(false);
    mod_enabled2 = cfg.get<bool>("visualize_pushbox").value_or(false);
};

void VisualizeHitbox::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("visualize_hitbox", mod_enabled);
    cfg.set<bool>("visualize_pushbox", mod_enabled2);
};
