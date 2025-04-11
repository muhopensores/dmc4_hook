#include "VisualizeHitbox.hpp"
#include "./sdk/World2Screen.hpp"
#include "../sdk/uActor.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

bool VisualizeHitbox::mod_enabled{false};

constexpr uintptr_t sMain = 0x00E5574C;

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

void VisualizeHitbox::on_frame(fmilliseconds& dt) {
    if (mod_enabled)
        for (int i = 0; i < 8; i++) {
            uintptr_t sCollisionGame  = *(uintptr_t*)sMain + 0x10340;
            uintptr_t uCollisionGroup = *(uintptr_t*)(*(uintptr_t*)sCollisionGame + (i + 2) * 16);
            if (uCollisionGroup != 0) {
                uint32_t CollNum  = *(uint32_t*)(uCollisionGroup + 0x94);
                cCollision** cColl = *(cCollision***)(uCollisionGroup + 0x9C);
                if ((CollNum > 0) && (cColl != nullptr))
                    for (int j = 0; j < CollNum; j++) {
                        cCollision* coll = cColl[j];
                        kCollPrim* prim   = coll->mpCollPrim;
                        Matrix4x4 wMat    = glm::make_mat4((float*)&coll->mMat0);
                        if (prim->mType == 3) {
                            Vector3f Pos1 = glm::make_vec3((float*)&prim->Pos0);
                            Vector3f Pos2 = glm::make_vec3((float*)&prim->Pos1);
                            float length = glm::length(Pos2 - Pos1);
                            length = (length > 0.00001f) ? length : 0.01f;
                            Matrix4x4 wPos1    = glm::translate(wMat, Pos1);
                            Matrix4x4 wPos2  = glm::translate(wMat, Pos2);
                            Vector3f dir       = glm::vec3(wPos2[3]) - glm::vec3(wPos1[3]);
                            float rotationY = atan2(dir.x, dir.z);
                            float rotationX    = atan2(sqrt(dir.z * dir.z + dir.x * dir.x), dir.y);
                            float rotationZ    = 0.0f;
                            w2s::DrawWireframeCapsule(glm::vec3(wPos1[3]), prim->radius, length,
                                rotationX, rotationY, rotationZ,
                                IM_COL32(255, 0, 0, 100), 32, 3.0f);
                        } else {
                            Vector3f Pos  = glm::make_vec3((float*)&prim->Pos0);
                            Matrix4x4 wPos = glm::translate(wMat, Pos);
                            w2s::DrawWireframeSphere(glm::vec3(wPos[3]), prim->radius, 0.0f,
                            IM_COL32(255, 0, 0, 100), 32, 3.0f);
                        }
                    }
            }
        }
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