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

typedef int(__stdcall *GetActiveSphereMask_t)(uEnemy_Old* enemy);
GetActiveSphereMask_t GetActiveSphereMask_ptr = (GetActiveSphereMask_t)0x04AB3A0;
int __stdcall CallSub4AB3A0(uEnemy_Old* enemy) {
    return GetActiveSphereMask_ptr(enemy);
}

typedef float*(__thiscall *CopyMatrix_t)(void* joint, float* matrixBuffer);
CopyMatrix_t CopyMatrix_ptr = (CopyMatrix_t)0x42C680;
float* __fastcall CallSub42C680(void* joint, float* matrixBuffer) {
    return CopyMatrix_ptr(joint, matrixBuffer);
}

// recreation of 0x4AB170 with sphere displays. This is called twice - one with sUnit+0x194 (enemies) and one with sUnit+0x1AC (extra enemy parts)
void DisplayEnemyStepSpheres(uEnemy_Old* enemy, uPlayer* player) {
    while (enemy) {
        ImGui::PushID((void*)enemy);

        // new checks, the enemy step func lacks these
        uDamage_Old* currentEnemyDamage = (uDamage_Old*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
        if (currentEnemyDamage->HP <= 0.0f) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }

        if ((uintptr_t)enemy->enemyStepSphereArray < 0x10000 || 
            (uintptr_t)enemy->enemyStepSphereArray & 0xF0000000) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // if ( !(BYTE*)(a1 + 4896 // + 0x1320) ) return 0;
        if (!enemy->intAt1320) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // if ( !(*DWORD*)(a1 + 4888 // + 0x1318) ) return 0;
        if (!enemy->enemyStepSphereArray) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // if ( (char*)(a1 + 3752 // + 0xEA8) < 0 ) return 0;
        if (enemy->launchStateThing2 < 0) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // v1 = (*DWORD*)(a1 + 4); if ( (v1 & 7) != 2 ) return 0; if ( (v1 & 0x400) == 0 ) return 0;
        if ((enemy->flags & 7) != 2 || (enemy->flags & 0x400) == 0) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // required fields for CallSub4AB3A0
        if (!enemy->m_joint_array_size || enemy->m_joint_array_size > 1000) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // v21 = sub_4AB3A0(a1); if ( !v21 ) return 0;
        uint32_t v21 = CallSub4AB3A0(enemy);
        if (v21 == 0) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        // v2 = (*DWORD*)(a1 + 4888); v20 = 0; if ( *v2 == -1 ) return 0;
        uintptr_t* v2 = (uintptr_t*)enemy->enemyStepSphereArray;
        char v20 = 0;
        if (*v2 == -1) {
            ImGui::PopID();
            enemy = enemy->nextEnemy;
            continue;
        }
        
        Vector3f playerPos = player->mPos;
        // ( i = (float*)(v2 + 5); ; i += 8 )
        for (float* i = (float*)(v2 + 5); ; i += 8) {
            // if ( ((1 << v20) & v21) != 0 )
            if (((1 << v20) & v21) != 0) {
                // find joint by the value at *v2 (current jointNo)
                int currentJointNo = *v2;
                UModelJoint* joint = nullptr;
                for (int j = 0; j < enemy->m_joint_array_size; j++) {
                    if (enemy->joints->joint[j].mNo == currentJointNo) {
                        joint = &enemy->joints->joint[j];
                        break;
                    }
                }
                
                if (joint) {
                    float v22[64];  // BYTE v22[64];
                    float* v4 = CallSub42C680(joint, v22);  // v4 = (float*)sub_42C680(v22);
                    
                    // Check if CallSub42C680 returned null
                    if (!v4) {
                        goto next_sphere;
                    }
                    
                    float v5 = i[1];      // v5 = i[1];
                    float v6 = *(i - 1);  // v6 = *(i - 1);

                    // Check for division by zero before calculating v7, was crashing
                    float denominator = (v4[3] * v6 + v4[11] * v5) + v4[7] * (*i) + v4[15];
                    if (denominator == 0.0f) {
                        goto next_sphere;
                    }
                    
                    // v7 = 1.0 / (float)((float)((float)((float)(v4[3] * v6) + (float)(v4[11] * v5)) + (float)(v4[7] * *i)) + v4[15]);
                    float v7 = 1.0f / denominator;
                    
                    // v8 = (float)((float)((float)(v4[8] * v5) + (float)(v4[4] * *i)) + (float)(v6 * v4[0])) + v4[12];
                    float v8 = ((v4[8] * v5 + v4[4] * (*i)) + v6 * v4[0]) + v4[12];
                    
                    // v9 = (float)((float)((float)(v4[1] * v6) + (float)(v4[9] * v5)) + (float)(v4[5] * *i)) + v4[13];
                    float v9 = ((v4[1] * v6 + v4[9] * v5) + v4[5] * (*i)) + v4[13];
                    
                    // v10 = (float)((float)((float)(v4[2] * v6) + (float)(v4[10] * v5)) + (float)(v4[6] * *i)) + v4[14];
                    float v10 = ((v4[2] * v6 + v4[10] * v5) + v4[6] * (*i)) + v4[14];
                    
                    // v12 = v8 * v7; v13 = v9 * v7; v14 = v10 * v7;
                    float v12 = v8 * v7;  // transformed X
                    float v13 = v9 * v7;  // transformed Y  
                    float v14 = v10 * v7;  // transformed Z
                    
                    Vector3f finalPos = Vector3f(v12, v13, v14);
                    
                    Vector3f adjustedPlayerPos = playerPos;
                    adjustedPlayerPos.y += 85.0f;
                    
                    // distance
                    float dx = v12 - adjustedPlayerPos.x;  // v12 - v16
                    float dy = v13 - adjustedPlayerPos.y;  // v13 - (v15 + 85.0)  
                    float dz = v14 - adjustedPlayerPos.z;  // v14 - v17
                    float distanceSquared = dx * dx + dy * dy + dz * dz;
                    
                    // *(i - 4) * *(i - 4)
                    float radiusSquared = (*(i - 4)) * (*(i - 4));
                    
                    // Validate radius is reasonable
                    if (radiusSquared < 0.0f || radiusSquared > 1000000.0f) {
                        goto next_sphere;
                    }
                    
                    // if ( radiusSquared > distanceSquared ) break;
                    // but we want to display all spheres
                    ImColor inRange = IM_COL32(255, 0, 0, 255);   // Red when in range
                    ImColor outRange = IM_COL32(0, 255, 0, 255);  // Green when not
                    ImColor color = (radiusSquared > distanceSquared) ? inRange : outRange;
                    
                    w2s::DrawWireframeCapsule(finalPos, sqrt(radiusSquared), 0.0f, 0.0f, (enemy->rotation.y * (float)M_PI)/*(atan2(joint->mWmat.m1[2], joint->mWmat.m3[2]))*/, 0.0f, color, 16, 1.0f);
                    
                    if (enemyStepSphereDebug) {
                        ImGui::PushID(v20);
                        ImGui::Text("Sphere %d - Active Mask: 0x%08X, Bit %d: %s", v20, v21, v20, 
                                  ((1 << v20) & v21) ? "ACTIVE" : "INACTIVE");
                        ImGui::Text("Joint No: %d, Distance²: %.2f, Radius²: %.2f, In Range: %s", 
                                  currentJointNo, distanceSquared, radiusSquared, (radiusSquared > distanceSquared) ? "YES" : "NO");
                        ImGui::Text("pseudocode vars: v5=%.2f, v6=%.2f, *i=%.2f", v5, v6, *i);
                        ImGui::Text("Transformed: v12=%.2f, v13=%.2f, v14=%.2f", v12, v13, v14);
                        ImGui::Text("Matrix v7 (w): %.6f", v7);
                        ImGui::PopID();
                    }
                }
            }
            
            next_sphere:
            // v18 = *v2[8]; ++v20; v2 += 8; if ( v18 == -1 ) return 0;
            int v18 = *(v2 + 8);    // v18 = v2[8]
            ++v20;                  // increment sphere counter
            v2 += 8;                // advance pointer to next sphere
            if (v18 == -1) break;   // exit condition
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
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            if (enemy) DisplayEnemyStepSpheres(enemy, player);
            uEnemy_Old* object = devil4_sdk::get_objects();
            if (object) DisplayEnemyStepSpheres(object, player);
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

naked void detour_hitboxes2(void) {
    _asm {
            pushfd
            cmp byte ptr [VisualizeHitbox::mod_enabled4], 1
            jne originalcode

            sub esp, 0x80
            movups [esp+0x0*10], xmm0 // only xmm0 has a vec3 but only backing this up would mean i have to do more simple addition
            movups [esp+0x1*10], xmm1
            movups [esp+0x2*10], xmm2
            movups [esp+0x3*10], xmm3
            movups [esp+0x4*10], xmm4
            movups [esp+0x5*10], xmm5
            movups [esp+0x6*10], xmm6
            movups [esp+0x7*10], xmm7
            pushad // 0x20
            //push ebx // kCollPrim
            push esi // cCollision
			call AddHitDataPtr // this screws like every xmm and breaks col
			add esp,0x4
			popad
            movups xmm7, [esp+0x7*10]
            movups xmm6, [esp+0x6*10]
            movups xmm5, [esp+0x5*10]
            movups xmm4, [esp+0x4*10]
            movups xmm3, [esp+0x3*10]
            movups xmm2, [esp+0x2*10]
            movups xmm1, [esp+0x1*10]
            movups xmm0, [esp+0x0*10]
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
        if (ImGui::Checkbox(_("Visualize Hitboxes"), &mod_enabled)) {
            mod_enabled4 = false;
        }
        ImGui::SameLine();
        help_marker(_("Draw hitbox outlines in red"));

        if (ImGui::Checkbox(_("Visualize Hitboxes 2"), &mod_enabled4)) {
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

        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Debug Stats##EnemyStepSpheres"), &enemyStepSphereDebug);
        ImGui::Unindent();
    }
}

void VisualizeHitbox::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("visualize_hitbox").value_or(false);
    mod_enabled2 = cfg.get<bool>("visualize_hurtbox").value_or(false);
    mod_enabled3 = cfg.get<bool>("visualize_enemystep").value_or(false);
    mod_enabled4  = cfg.get<bool>("visualize_hitbox2").value_or(false);
};

void VisualizeHitbox::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("visualize_hitbox", mod_enabled);
    cfg.set<bool>("visualize_hurtbox", mod_enabled2);
    cfg.set<bool>("visualize_enemystep", mod_enabled3);
    cfg.set<bool>("visualize_hitbox2", mod_enabled4);
};
