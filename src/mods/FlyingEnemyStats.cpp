#include "FlyingEnemyStats.hpp"
#include "EnemyTracker.hpp" // for damage specific offsets
#include "sdk/World2Screen.hpp"

#if 1
bool FlyingEnemyStats::flyingEnemyStats = false;
bool FlyingEnemyStats::showFlyingEnemyHP = false;
bool FlyingEnemyStats::showFlyingEnemyDamageTaken = false;
bool FlyingEnemyStats::showFlyingEnemyDT = false;
bool FlyingEnemyStats::showFlyingEnemyStun = false;
bool FlyingEnemyStats::showFlyingEnemyDisplacement = false;
bool FlyingEnemyStats::showFlyingEnemyMoveID = false;
bool FlyingEnemyStats::showFlyingEnemyMechanics = false;
bool FlyingEnemyStats::showFlyingEnemyStunTimer = false;
bool FlyingEnemyStats::showFlyingEnemyDebug = false;
bool FlyingEnemyStats::showFlyingEnemyCollisionData = false;
int FlyingEnemyStats::collisionPage = 0;

class sUnit {
public:
    char pad_00[0x194];
    uEnemy* enemy;
};

sUnit* get_sUnit() {
		constexpr uintptr_t static_unit_ptr = 0xE552CC;
		sUnit* s_unit_ptr = (sUnit*)*(uintptr_t*)static_unit_ptr;
		return s_unit_ptr;
}

void FlyingEnemyStats::on_frame(fmilliseconds& dt) {
    if (SMediator* sMedPtr = devil4_sdk::get_sMediator()) {
        if (sMedPtr->player_ptr) {
            if (!flyingEnemyStats) { return; }
            sUnit* sUnit = get_sUnit();
            if (!sUnit) { return; }
            uEnemy* enemy = sUnit->enemy;
            int enemyCount = 0;
            while (enemy) {
                glm::vec3 objectPosition = enemy->position;
                float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                std::string windowName = "EnemyStats##" + std::to_string((uintptr_t)enemy);
                if (w2s::IsVisibleOnScreen(objectPosition)) {
                    ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove);
                    ImGui::PushID((uintptr_t)enemy);
                    ImGui::SetWindowPos(screenPos);
                    float currentFontScale = 0.8f * guiFriendlyDistance;
                    ImGui::SetWindowFontScale(currentFontScale);
                    float currentItemWidth = (sameLineItemWidth / 2.0f) * guiFriendlyDistance;
                    ImGui::PushItemWidth(currentItemWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                    uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                    if (showFlyingEnemyHP) ImGui::SliderFloat("HP##EnemyFly", &currentEnemyDamage->HP, 0.0f, currentEnemyDamage->HPMax, "%.0f");
                    if (showFlyingEnemyDamageTaken) ImGui::InputFloat("Damage##EnemyFly", &currentEnemyDamage->HPTaken, NULL, NULL, "%.0f");
                    if (showFlyingEnemyDT) ImGui::InputFloat("DT Timer##EnemyFly", &enemy->DTTimer, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                    if (showFlyingEnemyStun) ImGui::InputInt("Stun##EnemyFly", &currentEnemyDamage->stun[0], NULL, NULL);
                    if (showFlyingEnemyDisplacement) ImGui::InputInt("Displacement##EnemyFly", &currentEnemyDamage->displacement[0], NULL, NULL);
                    if (showFlyingEnemyStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &currentEnemyDamage->stunResetTimer, 0.0f, 180.0f, "%.0f");
                    if (showFlyingEnemyMoveID) ImGui::InputScalar("MoveID##EnemyFly", ImGuiDataType_U8, &enemy->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                    if (showFlyingEnemyMechanics) {
                        if (enemy->ID == ANGELO_BIANCO || enemy->ID == ANGELO_ALTO) {
                            ImGui::SliderFloat("Shield##EnemyFly", &enemy->angeloShield, 0.0f, enemy->angeloShieldMax, "%.0f");
                        }
                        if (enemy->ID == MEPHISTO || enemy->ID == FAUST) {
                            if (enemy->faustCloak > 0.0f) {
                                ImGui::SliderFloat("Cloak##EnemyFly", &enemy->faustCloak, 0.0f, enemy->faustCloakMax, "%.0f");
                            }
                            else {
                                ImGui::InputFloat("Cloak Timer##EnemyFly", &enemy->faustCloakTimer, NULL, NULL, "%.0f");
                            }
                        }
                        if (enemy->ID == BLITZ) {
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
                        if (enemy->ID == GLADIUS) {
                            if (enemy->gladiusBuried) {
                                ImGui::SliderFloat("Buried Timer##EnemyFly", &enemy->gladiusTimer, 0.0f, 300.0f, "%.0f");
                            }
                        }
                        if (enemy->ID == BERIAL) {
                            if (enemy->berialFireTimer > 0.0f) {
                                ImGui::InputFloat("Fire Timer##EnemyFly", &enemy->berialFireTimer, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat("Fire Damage##EnemyFly", &enemy->berialFire, 0.0f, enemy->berialFireMax, "%.0f");
                            }
                        }
                        if (enemy->ID == CREDO) {
                            if (enemy->credoShield > 0.0f) {
                                ImGui::SliderFloat("Shield##EnemyFly", &enemy->credoShield, 0.0f, 4000.0f, "%.0f");
                            }
                            else {
                                ImGui::InputFloat("Shield Timer##EnemyFly", &enemy->credoShieldTimer, NULL, NULL, "%.0f");
                            }
                        }
                        if (enemy->ID == SANCTUS_M11) {
                            if (enemy->sanctusShieldTimerM11 > 0.0f) {
                                ImGui::InputFloat("Shield Timer##EnemyFly", &enemy->sanctusShieldTimerM11, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat("Shield Damage##EnemyFly", &enemy->sanctusShieldM11, 0.0f, 720.0f, "%.0f");
                            }
                        }
                        if (enemy->ID == SANCTUS_M20) {
                            if (enemy->sanctusShieldTimerM20 > 0.0f) {
                                ImGui::InputFloat("Shield Timer##EnemyFly", &enemy->sanctusShieldTimerM20, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat("Shield Damage##EnemyFly", &enemy->sanctusShieldM20, 0.0f, 600.0f, "%.0f");
                            }
                        }
                    }
                    if (showFlyingEnemyDebug) {
                        ImGui::PushItemWidth(currentItemWidth * 2.0f);
                        ImGui::InputScalar(_("Base Addr"), ImGuiDataType_U32, &enemy, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
                        ImGui::InputFloat3(_("Position##EnemyFly"), (float*)&enemy->position);
                        ImGui::InputFloat3(_("Rotation##EnemyFly"), (float*)&enemy->rotation);
                        ImGui::InputFloat3(_("Velocity##EnemyFly"), (float*)&enemy->velocity);
                        ImGui::InputFloat3(_("Scale##EnemyFly"), (float*)&enemy->scale);
                        ImGui::PopItemWidth();
                        ImGui::InputScalar(_("Anim ID##EnemyFly"), ImGuiDataType_U16, &enemy->animID, NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
                        ImGui::InputScalar(_("Move Part##EnemyFly"), ImGuiDataType_U8, &enemy->movePart);
                        ImGui::SliderFloat(_("Animation Frame##EnemyFly"), &enemy->animFrame, 0.0f, enemy->animFrameMax);
                        ImGui::InputScalar(_("Team##EnemyFly"), ImGuiDataType_U8, &enemy->team);
                        ImGui::InputScalar(_("ID##EnemyFly"), ImGuiDataType_U8, &enemy->ID);
                        ImGui::InputInt(_("Number##EnemyFly"), &enemyCount, 0, 0);
                    }
                    if (showFlyingEnemyCollisionData) {
                        uintptr_t collisionSettingsAddress = (uintptr_t)&enemy->collisionSettings;
                        ImGui::SetNextItemWidth(currentItemWidth);
                        if (ImGui::InputInt("Page", &collisionPage)) {
                            if (collisionPage < 0) collisionPage = 0;
                            if (collisionPage > 8) collisionPage = 8;
                        }
                        switch (collisionPage) {
                        case 0:
                            ImGui::InputScalar(_("Start Addr"), ImGuiDataType_U64, &collisionSettingsAddress, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
                            ImGui::InputScalar(_("mHit"), ImGuiDataType_U8, &enemy->collisionSettings.mHit, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mGrab"), ImGuiDataType_U8, &enemy->collisionSettings.mGrab, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mDamage"), ImGuiDataType_U8, &enemy->collisionSettings.mDamage, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mPsh"), ImGuiDataType_U8, &enemy->collisionSettings.mPsh, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mLand"), ImGuiDataType_U8, &enemy->collisionSettings.mLand, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mTimedForceLand"), ImGuiDataType_U8, &enemy->collisionSettings.mTimedForceLand, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mSetLand"), ImGuiDataType_U8, &enemy->collisionSettings.mSetLand, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mWall"), ImGuiDataType_U8, &enemy->collisionSettings.mWall, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mCeilling"), ImGuiDataType_U8, &enemy->collisionSettings.mCeilling, NULL, NULL, "%02X");
                            break;
                        case 1:
                            ImGui::InputScalar(_("mNoLandTimerEnable"), ImGuiDataType_U8, &enemy->collisionSettings.mNoLandTimerEnable, NULL, NULL, "%02X");
                            ImGui::InputFloat(_("mNoLandTimer"), &enemy->collisionSettings.mNoLandTimer);
                            ImGui::InputFloat(_("mNoLandTime"), &enemy->collisionSettings.mNoLandTime);
                            ImGui::InputScalar(_("ForceLandFlag"), ImGuiDataType_U8, &enemy->collisionSettings.ForceLandFlag, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mCheckWallEnable"), ImGuiDataType_U8, &enemy->collisionSettings.mCheckWallEnable, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mCheckGroundEnable"), ImGuiDataType_U8, &enemy->collisionSettings.mCheckGroundEnable, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mFall"), ImGuiDataType_U8, &enemy->collisionSettings.mFall, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("NoLandTimerType"), ImGuiDataType_U8, &enemy->collisionSettings.NoLandTimerType, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("NoLandDTSwitch"), ImGuiDataType_U8, &enemy->collisionSettings.NoLandDTSwitch, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("field25_0x3f"), ImGuiDataType_U8, &enemy->collisionSettings.field25_0x3f, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("NoLandTimerType2"), ImGuiDataType_U32, &enemy->collisionSettings.NoLandTimerType2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("DamageValueCorrect"), ImGuiDataType_U8, &enemy->collisionSettings.DamageValueCorrect, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("EnemyCollisionToggle"), ImGuiDataType_U32, &enemy->collisionSettings.EnemyCollisionToggle, NULL, NULL, "%08X");
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("LockonTargetPos"), &enemy->collisionSettings.LockonTargetPos.x);
                            ImGui::InputScalar(_("LockOnTarget"), ImGuiDataType_U32, &enemy->collisionSettings.LockonTarget, NULL, NULL, "%08X");
                            break;
                        case 2:
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("uknFixedVec1"), &enemy->collisionSettings.uknFixedVec1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m1"), &enemy->collisionSettings.GroundMat.m1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m2"), &enemy->collisionSettings.GroundMat.m2.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m3"), &enemy->collisionSettings.GroundMat.m3.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m4"), &enemy->collisionSettings.GroundMat.m4.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("uknFixedVec2"), &enemy->collisionSettings.uknFixedVec2.x);
                            ImGui::InputScalar(_("mWeightType"), ImGuiDataType_U32, &enemy->collisionSettings.mWeightType, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mModelID"), ImGuiDataType_U32, &enemy->collisionSettings.mModelID, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mpReportActor"), ImGuiDataType_U32, &enemy->collisionSettings.mpReportActor, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mpDstModel"), ImGuiDataType_U32, &enemy->collisionSettings.mpDstModel, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mpSrcModel"), ImGuiDataType_U32, &enemy->collisionSettings.mpSrcModel, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("uknSrcModelPtr"), ImGuiDataType_U32, &enemy->collisionSettings.uknSrcModelPtr, NULL, NULL, "%08X");
                            break;
                        case 3:
                            ImGui::InputScalar(_("mPushType"), ImGuiDataType_U32, &enemy->collisionSettings.mPushType, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mpPushModel"), ImGuiDataType_U32, &enemy->collisionSettings.mpPushModel, NULL, NULL, "%08X");
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p0"), &enemy->collisionSettings.mPushCap.p0.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p1"), &enemy->collisionSettings.mPushCap.p1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap1 p0"), &enemy->collisionSettings.mPushCap1.p0.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap1 p1"), &enemy->collisionSettings.mPushCap1.p1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushPos"), &enemy->collisionSettings.mPushPos.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushPosOld"), &enemy->collisionSettings.mPushPosOld.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m1"), &enemy->collisionSettings.field50_0x1c0.m1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m2"), &enemy->collisionSettings.field50_0x1c0.m2.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m3"), &enemy->collisionSettings.field50_0x1c0.m3.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m4"), &enemy->collisionSettings.field50_0x1c0.m4.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m1"), &enemy->collisionSettings.field51_0x200.m1.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m2"), &enemy->collisionSettings.field51_0x200.m2.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m3"), &enemy->collisionSettings.field51_0x200.m3.x);
                            ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m4"), &enemy->collisionSettings.field51_0x200.m4.x);
                            break;
                        case 4:
                            ImGui::InputScalar(_("uknToggle"), ImGuiDataType_U32, &enemy->collisionSettings.uknToggle, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("currentMotBuffer"), ImGuiDataType_U32, &enemy->collisionSettings.currentMotBuffer, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("MotSeq1"), ImGuiDataType_U32, &enemy->collisionSettings.MotSeq1, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("MotSeq2"), ImGuiDataType_U32, &enemy->collisionSettings.MotSeq2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("HitCheckFlag"), ImGuiDataType_U8, &enemy->collisionSettings.HitCheckFlag, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("HitConfirm"), ImGuiDataType_U8, &enemy->collisionSettings.HitConfirm, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mSelfCollision"), ImGuiDataType_U8, &enemy->collisionSettings.mSelfCollision, NULL, NULL, "%02X");
                            ImGui::InputScalar(_("mMode"), ImGuiDataType_U32, &enemy->collisionSettings.mMode, NULL, NULL, "%08X");
                            break;
                        case 5:
                            ImGui::InputScalar(_("mVsAttrPlAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPlAtk, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPlDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPlDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPlPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPlPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPlGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPlGrb, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPsAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPsAtk, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPsDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPsDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPsPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPsPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrPsGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrPsGrb, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEmAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEmAtk, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEmDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEmDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEmPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEmPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEmGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEmGrb, NULL, NULL, "%08X");
                            break;
                        case 6:
                            ImGui::InputScalar(_("mVsAttrEsAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEsAtk, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEsDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEsDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEsPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEsPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrEsGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrEsGrb, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrSetAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrSetAtk, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrSetDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrSetDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrSetPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrSetPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrSetGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrSetGrb, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrStgAtk"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrStgAtk, NULL, NULL, "%08X");
                            break;
                        case 7:
                            ImGui::InputScalar(_("mVsAttrStgDmg"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrStgDmg, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrStgPsh"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrStgPsh, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mVsAttrStgGrb"), ImGuiDataType_U32, &enemy->collisionSettings.mVsAttrStgGrb, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrAtk1"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrAtk1, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrDmg1"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrDmg1, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrPsh1"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrPsh1, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrGrb1"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrGrb1, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrAtk2"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrAtk2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrDmg2"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrDmg2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrPsh2"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrPsh2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrGrb2"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrGrb2, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrAtk3"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrAtk3, NULL, NULL, "%08X");
                            break;
                        case 8:
                            ImGui::InputScalar(_("UknVsAttrDmg3"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrDmg3, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrPsh3"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrPsh3, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrGrb3"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrGrb3, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrAtk4"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrAtk4, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrDmg4"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrDmg4, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrPsh4"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrPsh4, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknVsAttrGrb4"), ImGuiDataType_U32, &enemy->collisionSettings.UknVsAttrGrb4, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("UknCollisionToggle"), ImGuiDataType_U32, &enemy->collisionSettings.UknCollisionToggle, NULL, NULL, "%08X");
                            ImGui::InputScalar(_("mCollisionGroupNum"), ImGuiDataType_U32, &enemy->collisionSettings.mCollisionGroupNum, NULL, NULL, "%08X");
                            break;
                        default:
                            break;
                        }
                    }
                    ImGui::PopStyleVar(2);
                    ImGui::PopItemWidth();    
                    ImGui::PopID();           
                    ImGui::End();             
                }
                enemyCount++;
                enemy = enemy->nextEnemy;     
            }
        }
    }
}

std::optional<std::string> FlyingEnemyStats::on_initialize() {

    return Mod::on_initialize();
}

void FlyingEnemyStats::on_gui_frame(int display) {
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
        ImGui::Checkbox("Display Move ID", &showFlyingEnemyMoveID);
        ImGui::Checkbox("Display Mechanics", &showFlyingEnemyMechanics);
        ImGui::Checkbox("Display Debug Info", &showFlyingEnemyDebug);
        ImGui::Checkbox("Display Collision Info", &showFlyingEnemyCollisionData);
        ImGui::Unindent(lineIndent);
    }
}

void FlyingEnemyStats::on_config_load(const utility::Config& cfg) {
    flyingEnemyStats = cfg.get<bool>("flyingEnemyStats").value_or(false);
    showFlyingEnemyHP = cfg.get<bool>("showFlyingEnemyHP").value_or(true);
    showFlyingEnemyDT = cfg.get<bool>("showFlyingEnemyDT").value_or(false);
    showFlyingEnemyDamageTaken = cfg.get<bool>("showFlyingEnemyDamageTaken").value_or(false);
    showFlyingEnemyMoveID = cfg.get<bool>("showFlyingEnemyMoveID").value_or(false);
    showFlyingEnemyStun = cfg.get<bool>("showFlyingEnemyStun").value_or(false);
    showFlyingEnemyDisplacement = cfg.get<bool>("showFlyingEnemyDisplacement").value_or(false);
    showFlyingEnemyStunTimer = cfg.get<bool>("showFlyingEnemyStunTimer").value_or(false);
    showFlyingEnemyMechanics = cfg.get<bool>("showFlyingEnemyMechanics").value_or(false);
    showFlyingEnemyDebug = cfg.get<bool>("showFlyingEnemyDebug").value_or(false);
    showFlyingEnemyCollisionData = cfg.get<bool>("showFlyingEnemyCollisionData").value_or(false);
}

void FlyingEnemyStats::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("flyingEnemyStats", flyingEnemyStats);
    cfg.set<bool>("showFlyingEnemyHP", showFlyingEnemyHP);
    cfg.set<bool>("showFlyingEnemyDT", showFlyingEnemyDT);
    cfg.set<bool>("showFlyingEnemyDamageTaken", showFlyingEnemyDamageTaken);
    cfg.set<bool>("showFlyingEnemyMoveID", showFlyingEnemyMoveID);
    cfg.set<bool>("showFlyingEnemyStun", showFlyingEnemyStun);
    cfg.set<bool>("showFlyingEnemyMechanics", showFlyingEnemyMechanics);
    cfg.set<bool>("showFlyingEnemyStunTimer", showFlyingEnemyStunTimer);
    cfg.set<bool>("showFlyingEnemyDebug", showFlyingEnemyDebug);
    cfg.set<bool>("showFlyingEnemyCollisionData", showFlyingEnemyCollisionData);
}
#endif
