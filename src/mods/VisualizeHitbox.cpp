#include "VisualizeHitbox.hpp"
#include "./sdk/World2Screen.hpp"
#include "../sdk/uActor.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

bool VisualizeHitbox::mod_enabled{false};
constexpr uintptr_t sMainAddr = 0x00E5574C;

struct kCollPrim {
    int mType;
    int Bone0;
    int Bone1;
    float radius;
    struct uActorMain::MtVector3 Pos0;
    struct uActorMain::MtVector3 Pos1;
    uint32_t ukn0;
    uint32_t mId;
    float mShrink;
    uint32_t ukn1;
};

struct cCollision {
    uint32_t ukn1;
    struct kCollPrim* mpCollPrim; /* Created by retype action */
    float scale;
    int mAttackUniqueID;
    struct uActorMain::MtVector3 UknVec1;
    struct uActorMain::MtVector3 UknVec2;
    uint32_t padding0;
    struct cCollisionGroup* mpCollisionGroup;
    uint32_t padding1[2];
    struct MtMatrix mMat0;
    struct MtMatrix mMat1;
    struct MtMatrix mOldMat0;
    struct MtMatrix* mpRefMat0;
    struct MtMatrix* mpRefMat1;
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
    if (!mod_enabled) return;
    sMain* sMainPtr = *(sMain**)sMainAddr;
    if (!sMainPtr || !sMainPtr->pCollisionGroupsContainer) return;
    CollisionGroupsContainer* groupsContainer = sMainPtr->pCollisionGroupsContainer;
    for (int i = 0; i < 8; i++) {
        CollisionGroup* group = groupsContainer->groups[i].groupPointer;
        if (!group) continue;
        uint32_t collNum  = group->collisionCount;
        cCollision** cColl = group->collisionArray;
        if (collNum == 0 || cColl == nullptr) continue;
        for (int j = 0; j < collNum; j++) {
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
                w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), prim->radius, length, rotationX, rotationY, 0.0f, IM_COL32(255, 0, 0, 100), 32, 3.0f);
            } else {
                Vector3f position = glm::make_vec3((float*)&prim->Pos0);
                Matrix4x4 worldPos = glm::translate(worldMatrix, position);
                w2s::DrawWireframeSphere(glm::vec3(worldPos[3]), prim->radius, 0.0f, IM_COL32(255, 0, 0, 100), 32, 3.0f);
            }
        }
    }
}

std::optional<std::string> VisualizeHitbox::on_initialize() {
    return Mod::on_initialize();
}

void VisualizeHitbox::on_gui_frame() {
    ImGui::Checkbox(_("Visualize Hitboxes"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Draw hitbox outlines"));
}

void VisualizeHitbox::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("visualize_hitbox").value_or(false);
};

void VisualizeHitbox::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("visualize_hitbox", mod_enabled);
};
