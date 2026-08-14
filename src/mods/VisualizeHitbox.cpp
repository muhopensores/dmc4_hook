#include "VisualizeHitbox.hpp"
#include "./sdk/World2Screen.hpp"
#include "../sdk/uActor.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/uCollisionMgr.hpp"
#define _USE_MATH_DEFINES
#include <math.h>
#include "EnemyTracker.hpp"

// static std::unique_ptr<FunctionHook> hurtboxHook;
// static std::unique_ptr<FunctionHook> hitbox2hook;

bool VisualizeHitbox::mod_enabled = false; // Visualize Hitboxes
constexpr uintptr_t sMainAddr = 0x00E5574C;
static bool enemyStepSphereDebug;

bool VisualizeHitbox::mod_enabled2 = false; // Visualize Hurtboxes
uintptr_t VisualizeHitbox::jmp_ret_hurtboxes = NULL;
static std::vector<HurtboxSnapshot> hurtDataList;

static std::mutex hurtDataMutex;
static void AddHurtDataPtr(void* worldData, void* data) { 
    auto* wd = (hurtboxPos*)(worldData); // esi cCollision
    auto* d  = (hurtboxData*)(data); // ebx kCollPrim
    std::lock_guard<std::mutex> lock(hurtDataMutex);
    hurtDataList.push_back({wd->pos + d->offset, d->radius});
}

bool VisualizeHitbox::mod_enabled3 = false; // Visualize JC Spheres

bool VisualizeHitbox::mod_enabled4 = false; // Visualize Hitboxes 2

uintptr_t VisualizeHitbox::jmp_ret_hitboxes2 = NULL;
static std::vector<HitboxSnapshot> hitDataList;

static std::mutex hitDataMutex;
static void AddHitDataPtr(cCollision* collision /*, kCollPrim* prim*/) {
    if (!collision) return;
    kCollPrim* prim = collision->mpCollPrim;
    if (!prim) return;
    glm::mat4 worldMatrix = glm::make_mat4((float*)&collision->mMat0);
    glm::vec3 pos0 = glm::make_vec3((float*)&prim->mPos0);
    glm::vec3 pos1 = glm::make_vec3((float*)&prim->mPos1);
    glm::mat4 worldPos0 = glm::translate(worldMatrix, pos0);
    glm::mat4 worldPos1 = glm::translate(worldMatrix, pos1);
    glm::vec3 direction = glm::vec3(worldPos1[3]) - glm::vec3(worldPos0[3]);
    float length = glm::length(direction);

    std::lock_guard<std::mutex> lock(hitDataMutex);
    if (length > 0.001f) { // Capsule
        float rotationY = atan2(direction.x, direction.z);
        float rotationX = atan2(sqrt(direction.x*direction.x + direction.z*direction.z), direction.y);
        hitDataList.push_back(HitboxSnapshot(glm::vec3(worldPos0[3]), prim->mRadius, length, rotationX, rotationY));
    } else { // Sphere
        hitDataList.push_back(HitboxSnapshot(glm::vec3(worldPos0[3]), prim->mRadius));
    }
}

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

typedef int(__stdcall* calcEmJumpMask)(uEnemy_Old* enemy);
calcEmJumpMask calcEmJumpMask_ptr = (calcEmJumpMask)0x04AB3A0;
int __stdcall uActor__calcEmJumpMask(uEnemy_Old* enemy) {
    return calcEmJumpMask_ptr(enemy);
}

typedef float*(__thiscall* getJointWMat)(void* joint, float* matrixBuffer);
getJointWMat getJointWMat_ptr = (getJointWMat)0x42C680;
float* __fastcall uModel__getJointWMat(void* joint, float* matrixBuffer) {
    return getJointWMat_ptr(joint, matrixBuffer);
}

typedef char(__thiscall* isKindOf)(uEnemy_Old* enemy, uintptr_t MtDTI);
isKindOf isKindOf_ptr = (isKindOf)0x402BD0;
char __fastcall MtObject__isKindOf(uEnemy_Old* enemy, uintptr_t MtDTI) {
    return isKindOf_ptr(enemy, MtDTI);
}

// recreation of 0x4AB170 with sphere displays. This is called twice - one with sUnit+0x194 (enemies) and one with sUnit+0x1AC (extra enemy parts)
void DisplayEnemyStepSpheres(uEnemy_Old* enemy, uPlayer* player) {
    while (enemy) {
        ImGui::PushID((void*)enemy);

        // if ( !*((_BYTE *)this + 0x1320) ) return 0;
        if (!enemy->charAt1320) { // 0x1320
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        // cmp dword ptr [ebx+00001318],00
        if (!enemy->enemyStepSphereArray) { // 0x1318
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        // test byte ptr [ebx+00000EA8],-80
        if (enemy->launchStateThing2 & 0x80) { // 0xea8
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        // v1 = (*DWORD*)(a1 + 4); if ( (v1 & 7) != 2 ) return 0; if ( (v1 & 0x400) == 0 ) return 0;
        if ((enemy->flags & 7) != 2 || (enemy->flags & 0x400) == 0) { // 0x4
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        // v21 = sub_4AB3A0(a1); if ( !v21 ) return 0;
        uint32_t v21 = uActor__calcEmJumpMask(enemy);
        if (v21 == 0) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        // v2 = (*DWORD*)(a1 + 4888); v20 = 0; if ( *v2 == -1 ) return 0;
        uint32_t* v2 = (uint32_t*)enemy->enemyStepSphereArray;
        char v20 = 0;
        if (*v2 == -1) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        int v3 = *v2;
        Vector3f playerPos = player->mPos;

        // ( i = (float*)(v2 + 5); ; i += 8 )
        for (float* i = (float*)(v2 + 5);; i += 8) {
            // if ( ((1 << v20) & v21) != 0 )
            if (((1 << v20) & v21) != 0) {
                // find joint by the value at *v2 (current jointNo)
                uint8_t jointTableIdx = (uint8_t)v3;
                uint8_t jointArrayIdx = enemy->mJointTable[jointTableIdx];
                UModelJoint* joint    = (jointArrayIdx == 255) ? nullptr : (UModelJoint*)((char*)enemy->joints + jointArrayIdx * 0x90);

                if (!joint) {
                    goto next_sphere;
                }

                float v22[64]; // BYTE v22[64];
                float* v4 = uModel__getJointWMat(joint, v22); // // v4 = (float*)sub_42C680(v2

                if (!v4) {
                    goto next_sphere;
                }

                float v5 = i[1];     // v5 = i[1];
                float v6 = *(i - 1); // v6 = *(i - 1);

                float denominator = (v4[3] * v6 + v4[11] * v5) + v4[7] * (*i) + v4[15];

                // v7 = 1.0 / (float)((float)((float)((float)(v4[3] * v6) + (float)(v4[11] * v5)) + (float)(v4[7] * *i)) + v4[15]);
                float v7  = 1.0f / denominator;

                // v8 = (float)((float)((float)(v4[8] * v5) + (float)(v4[4] * *i)) + (float)(v6 * v4[0])) + v4[12];
                float v8  = ((v4[8] * v5 + v4[4] * (*i)) + v6 * v4[0]) + v4[12];

                // v9 = (float)((float)((float)(v4[1] * v6) + (float)(v4[9] * v5)) + (float)(v4[5] * *i)) + v4[13];
                float v9  = ((v4[1] * v6 + v4[9] * v5) + v4[5] * (*i)) + v4[13];

                // v10 = (float)((float)((float)(v4[2] * v6) + (float)(v4[10] * v5)) + (float)(v4[6] * *i)) + v4[14];
                float v10 = ((v4[2] * v6 + v4[10] * v5) + v4[6] * (*i)) + v4[14];

                float v12 = v8 * v7;
                float v13 = v9 * v7;
                float v14 = v10 * v7;

                Vector3f finalPos = Vector3f(v12, v13, v14);

                Vector3f adjustedPlayerPos = playerPos;
                adjustedPlayerPos.y += 85.0f;

                float dx              = v12 - adjustedPlayerPos.x;
                float dy              = v13 - adjustedPlayerPos.y;
                float dz              = v14 - adjustedPlayerPos.z;
                float distanceSquared = dx * dx + dy * dy + dz * dz;

                float radiusSquared = (*(i - 4)) * (*(i - 4));

                // if ( radiusSquared > distanceSquared ) break;
                // but we want to display all spheres
                ImColor inRange  = IM_COL32(255, 0, 0, 255); // Red when in range
                ImColor outRange = IM_COL32(0, 255, 0, 255); // Green when not
                ImColor color    = (radiusSquared > distanceSquared) ? inRange : outRange;

                w2s::DrawWireframeCapsule(finalPos, sqrt(radiusSquared), 0.0f, 0.0f, (enemy->rotation.y * (float)M_PI), 0.0f, color, 16, 1.0f);

                if (enemyStepSphereDebug) {
                    ImGui::PushID(v20);
                    ImGui::Text("Sphere %d - Active Mask: 0x%08X, Bit %d: %s", v20, v21, v20, ((1 << v20) & v21) ? "ACTIVE" : "INACTIVE");
                    ImGui::Text("Joint No: %d, Distance: %.2f, Radius: %.2f, In Range: %s", v3, distanceSquared, radiusSquared, (radiusSquared > distanceSquared) ? "YES" : "NO");
                    ImGui::Text("pseudocode vars: v5=%.2f, v6=%.2f, *i=%.2f", v5, v6, *i);
                    ImGui::Text("Transformed: v12=%.2f, v13=%.2f, v14=%.2f", v12, v13, v14);
                    ImGui::Text("Matrix v7 (w): %.6f", v7);
                    ImGui::PopID();
                }
            }

        next_sphere:
            v3 = *(v2 + 8);
            ++v20;
            v2 += 8;
            if (v3 == -1)
                break;
        }

        ImGui::PopID();
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
                    Vector3f pos1 = glm::make_vec3((float*)&prim->mPos0);
                    Vector3f pos2 = glm::make_vec3((float*)&prim->mPos1);
                    float length = glm::length(pos2 - pos1);
                    length = (length > 0.00001f) ? length : 0.01f;
                    Matrix4x4 worldPos1 = glm::translate(worldMatrix, pos1);
                    Matrix4x4 worldPos2 = glm::translate(worldMatrix, pos2);
                    Vector3f direction = glm::vec3(worldPos2[3]) - glm::vec3(worldPos1[3]);
                    float rotationY = atan2(direction.x, direction.z);
                    float rotationX = atan2(sqrt(direction.z * direction.z + direction.x * direction.x), direction.y);
                    w2s::DrawWireframeCapsule(glm::vec3(worldPos1[3]), prim->mRadius, length, rotationX, rotationY, 0.0f, IM_COL32(255, 0, 0, 255), 16, 1.0f);
                }
                else {
                    Vector3f position = glm::make_vec3((float*)&prim->mPos0);
                    Matrix4x4 worldPos = glm::translate(worldMatrix, position);
                    w2s::DrawWireframeSphere(glm::vec3(worldPos[3]), prim->mRadius, 0.0f, IM_COL32(255, 0, 0, 255), 16, 1.0f);
                }
            }
        }
    }

    if (mod_enabled2) { // hurtboxes
        for (const HurtboxSnapshot& snapshot : hurtDataList) {
                w2s::DrawWireframeSphere(snapshot.pos , snapshot.radius, 0.0f, IM_COL32(0, 0, 255, 255), 32, 1.0f);
        }
        hurtDataList.clear();
    }

    if (mod_enabled3) { // enemy step
        static constexpr uintptr_t uActor__MtDTI = 0xE58158;
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            uEnemy_Old* enemy = (uEnemy_Old*)devil4_sdk::get_moveline_top(15);
            if (enemy && MtObject__isKindOf(enemy, uActor__MtDTI))
                DisplayEnemyStepSpheres(enemy, player);

            uEnemy_Old* object = (uEnemy_Old*)devil4_sdk::get_moveline_top(16);
            if (object && MtObject__isKindOf(object, uActor__MtDTI))
                DisplayEnemyStepSpheres(object, player);

        // player
            Vector3f playerPos = glm::make_vec3((float*)&player->mPos);
            Vector3f playerSphereOffset { 0.0f, 85.0f, 0.0f }; // from DevilMayCry4_DX9.exe+AB322
            Vector3f finalPos = playerPos + playerSphereOffset;
            w2s::DrawWireframeCapsule(finalPos, 1.0f, 0.0f, 0.0f, player->rotation2, 0.0f, IM_COL32(0, 255, 0, 255), 16, 1.0f);
        }
    }
    if (mod_enabled4) { // hitboxes2
        for (const HitboxSnapshot& snapshot : hitDataList) {
            //if (snapshot.type == 3) {
                //w2s::DrawWireframeCapsule(glm::vec3(snapshot.pos), snapshot.radius, snapshot.length, snapshot.rotationX, snapshot.rotationY, 0.0f, IM_COL32(255, 0, 0, 255), 16, 1.0f);
            //} else {
                w2s::DrawWireframeSphere(snapshot.pos, snapshot.radius, 0.0f, IM_COL32(255, 0, 0, 255), 32, 1.0f);
            //}
        }
        hitDataList.clear();
    }
}

// This is missing some, likely those in the non enemies moveline
naked void detour_hurtboxes(void) {
    _asm {
            pushfd
            cmp byte ptr [VisualizeHitbox::mod_enabled2], 1
            jne originalcode

            pushad // 0x20
            push ebx // radius and offset
            push esi // hitbox location
			call AddHurtDataPtr
			add esp,0x8
			popad

        originalcode:
            popfd
            movss xmm0,[edi+0x78]
            jmp dword ptr [VisualizeHitbox::jmp_ret_hurtboxes]
    }
}

// This also gets non active hitboxes, e.g. echidna has some on her all the time
naked void detour_hitboxes2(void) {
    _asm {
            pushfd
            cmp byte ptr [VisualizeHitbox::mod_enabled4], 1
            jne originalcode

            sub esp, 0x80
            movups [esp+0x0*0x10], xmm0 // only xmm0 has a vec3 but only backing this up would mean i have to do more simple addition
            movups [esp+0x1*0x10], xmm1
            movups [esp+0x2*0x10], xmm2
            movups [esp+0x3*0x10], xmm3
            movups [esp+0x4*0x10], xmm4
            movups [esp+0x5*0x10], xmm5
            movups [esp+0x6*0x10], xmm6
            movups [esp+0x7*0x10], xmm7
            pushad // 0x20
            //push ebx // kCollPrim
            push esi // cCollision
			call AddHitDataPtr // this screws like every xmm and breaks col
			add esp,0x4
			popad
            movups xmm7, [esp+0x7*0x10]
            movups xmm6, [esp+0x6*0x10]
            movups xmm5, [esp+0x5*0x10]
            movups xmm4, [esp+0x4*0x10]
            movups xmm3, [esp+0x3*0x10]
            movups xmm2, [esp+0x2*0x10]
            movups xmm1, [esp+0x1*0x10]
            movups xmm0, [esp+0x0*0x10]
            add esp, 0x80

        originalcode:
            popfd
            movss xmm0,[ebx+0x0C]
            jmp dword ptr [VisualizeHitbox::jmp_ret_hitboxes2]
    }
}

std::optional<std::string> VisualizeHitbox::on_initialize() {
    if (!install_hook_offset(0x10AF83, hurtboxHook, &detour_hurtboxes, &jmp_ret_hurtboxes, 5)) {
        spdlog::error("Failed to init detour_hurtboxes\n");
        return "Failed to init detour_hurtboxes";
    }

    if (!install_hook_offset(0x10A98C, hitbox2hook, &detour_hitboxes2, &jmp_ret_hitboxes2, 5)) {
        spdlog::error("Failed to init detour_hitboxes2\n");
        return "Failed to init detour_hitboxes2";
    }
    
    return Mod::on_initialize();
}

void VisualizeHitbox::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        /*if (ImGui::Checkbox(_("Visualize Hitboxes"), &mod_enabled)) {
            mod_enabled4 = false;
        }
        ImGui::SameLine();
        help_marker(_("Draw hitbox outlines in red"));*/

        if (ImGui::Checkbox(_("Visualize Hitboxes"), &mod_enabled4)) {
            mod_enabled = false;
        }
        ImGui::SameLine();
        help_marker(_("Draw hitbox outlines in red"));

        ImGui::Checkbox(_("Visualize Hurtboxes"), &mod_enabled2);
        ImGui::SameLine();
        help_marker(_("Draw hurtbox outlines in blue"));

        ImGui::Checkbox(_("Visualize Enemy Step Spheres"), &mod_enabled3);
        ImGui::SameLine();
        help_marker(_("Draw enemy step sphere outlines in green"));

        /*if (mod_enabled3) {
            ImGui::Indent(lineIndent);
            ImGui::Checkbox(_("Debug Stats##EnemyStepSpheres"), &enemyStepSphereDebug);
            ImGui::Unindent();
        }*/
    }
}

void VisualizeHitbox::on_config_load(const utility::Config& cfg) {
    //mod_enabled = cfg.get<bool>("visualize_hitbox").value_or(false);
    mod_enabled2 = cfg.get<bool>("visualize_hurtbox").value_or(false);
    mod_enabled3 = cfg.get<bool>("visualize_enemystep").value_or(false);
    mod_enabled4  = cfg.get<bool>("visualize_hitbox2").value_or(false);
};

void VisualizeHitbox::on_config_save(utility::Config& cfg) {
    //cfg.set<bool>("visualize_hitbox", mod_enabled);
    cfg.set<bool>("visualize_hurtbox", mod_enabled2);
    cfg.set<bool>("visualize_enemystep", mod_enabled3);
    cfg.set<bool>("visualize_hitbox2", mod_enabled4);
};
