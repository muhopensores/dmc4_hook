#include "FlyingStats.hpp"
#include "EnemyTracker.hpp" // for enemy specific offsets
#include "sdk/World2Screen.hpp"

bool FlyingStats::showFlyingStats = false;
bool FlyingStats::showFlyingEnemyStats = false;
bool FlyingStats::showFlyingPlayerStats = false;
bool FlyingStats::showFlyingHP = false;
bool FlyingStats::showFlyingDamageTaken = false;
bool FlyingStats::showFlyingDamageResist = false;
bool FlyingStats::showFlyingDT = false;
bool FlyingStats::showFlyingStun = false;
bool FlyingStats::showFlyingDisplacement = false;
bool FlyingStats::showFlyingMoveID = false;
bool FlyingStats::showFlyingMechanics = false;
bool FlyingStats::showFlyingStunTimer = false;
bool FlyingStats::showFlyingDebug = false;
bool FlyingStats::showFlyingCollisionData = false;
int FlyingStats::collisionPage = 0;

static void DisplayCollisionData(uCollisionMgr* currentEnemyCollision, float currentItemWidth) {
    uintptr_t collisionSettingsAddress = *(uintptr_t*)&currentEnemyCollision;
    ImGui::SetNextItemWidth(currentItemWidth);
    if (ImGui::InputInt("Page", &FlyingStats::collisionPage)) {
        if (FlyingStats::collisionPage < 0) FlyingStats::collisionPage = 0;
        if (FlyingStats::collisionPage > 10) FlyingStats::collisionPage = 10;
    }
    switch (FlyingStats::collisionPage) {
    case 0:
        ImGui::InputScalar(_("Start Addr"), ImGuiDataType_U32, &collisionSettingsAddress, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputScalar(_("base.uknFlag1"), ImGuiDataType_U8, &currentEnemyCollision->base.uknFlag1, NULL, NULL, "%02X");
        ImGui::InputScalar(_("base.uknFlag2"), ImGuiDataType_U8, &currentEnemyCollision->base.uknFlag2, NULL, NULL, "%02X");
        ImGui::InputScalar(_("base.mTransMode"), ImGuiDataType_U8, &currentEnemyCollision->base.mTransMode, NULL, NULL, "%02X");
        ImGui::InputScalar(_("base.mTransView"), ImGuiDataType_U8, &currentEnemyCollision->base.mTransView, NULL, NULL, "%02X");
        ImGui::InputScalar(_("base.mp_next_unit"), ImGuiDataType_U32, &currentEnemyCollision->base.mp_next_unit, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputScalar(_("base.mp_prev_unit"), ImGuiDataType_U32, &currentEnemyCollision->base.mp_prev_unit, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat(_("base.m_delta_time"), &currentEnemyCollision->base.m_delta_time);
        ImGui::InputScalarN(_("base.reserved_state_flags"), ImGuiDataType_U8, &currentEnemyCollision->base.reserved_state_flags, 4, NULL, NULL, "%02X");
        break;
    case 1:
        ImGui::InputScalar(_("mHit"), ImGuiDataType_U8, &currentEnemyCollision->mHit, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mGrab"), ImGuiDataType_U8, &currentEnemyCollision->mGrab, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mDamage"), ImGuiDataType_U8, &currentEnemyCollision->mDamage, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mPsh"), ImGuiDataType_U8, &currentEnemyCollision->mPsh, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mLand"), ImGuiDataType_U8, &currentEnemyCollision->mLand, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mTimedForceLand"), ImGuiDataType_U8, &currentEnemyCollision->mTimedForceLand, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mSetLand"), ImGuiDataType_U8, &currentEnemyCollision->mSetLand, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mWall"), ImGuiDataType_U8, &currentEnemyCollision->mWall, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mCeilling"), ImGuiDataType_U8, &currentEnemyCollision->mCeilling, NULL, NULL, "%02X");
        break;
    case 2:
        ImGui::InputScalar(_("mNoLandTimerEnable"), ImGuiDataType_U8, &currentEnemyCollision->mNoLandTimerEnable, NULL, NULL, "%02X");
        ImGui::InputFloat(_("mNoLandTimer"), &currentEnemyCollision->mNoLandTimer);
        ImGui::InputFloat(_("mNoLandTime"), &currentEnemyCollision->mNoLandTime);
        ImGui::InputScalar(_("ForceLandFlag"), ImGuiDataType_U8, &currentEnemyCollision->ForceLandFlag, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mCheckWallEnable"), ImGuiDataType_U8, &currentEnemyCollision->mCheckWallEnable, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mCheckGroundEnable"), ImGuiDataType_U8, &currentEnemyCollision->mCheckGroundEnable, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mFall"), ImGuiDataType_U8, &currentEnemyCollision->mFall, NULL, NULL, "%02X");
        ImGui::InputScalar(_("NoLandTimerType"), ImGuiDataType_U8, &currentEnemyCollision->NoLandTimerType, NULL, NULL, "%02X");
        ImGui::InputScalar(_("NoLandDTSwitch"), ImGuiDataType_U8, &currentEnemyCollision->NoLandDTSwitch, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field25_0x3f"), ImGuiDataType_U8, &currentEnemyCollision->field25_0x3f, NULL, NULL, "%02X");
        ImGui::InputScalar(_("NoLandTimerType2"), ImGuiDataType_U32, &currentEnemyCollision->NoLandTimerType2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("DamageValueCorrect"), ImGuiDataType_U8, &currentEnemyCollision->DamageValueCorrect, NULL, NULL, "%02X");
        ImGui::InputScalar(_("EnemyCollisionToggle"), ImGuiDataType_U32, &currentEnemyCollision->EnemyCollisionToggle, NULL, NULL, "%08X");
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("LockonTargetPos"), &currentEnemyCollision->LockonTargetPos.x);
        ImGui::InputScalar(_("LockOnTarget"), ImGuiDataType_U32, &currentEnemyCollision->LockonTarget, NULL, NULL, "%08X");
        break;
    case 3:
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("uknFixedVec1"), &currentEnemyCollision->uknFixedVec1.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m1"), &currentEnemyCollision->GroundMat.m1.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m2"), &currentEnemyCollision->GroundMat.m2.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m3"), &currentEnemyCollision->GroundMat.m3.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("GroundMat m4"), &currentEnemyCollision->GroundMat.m4.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("uknFixedVec2"), &currentEnemyCollision->uknFixedVec2.x);
        ImGui::InputScalar(_("mWeightType"), ImGuiDataType_U32, &currentEnemyCollision->mWeightType, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mModelID"), ImGuiDataType_U32, &currentEnemyCollision->mModelID, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mpReportActor"), ImGuiDataType_U32, &currentEnemyCollision->mpReportActor, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mpDstModel"), ImGuiDataType_U32, &currentEnemyCollision->mpDstModel, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mpSrcModel"), ImGuiDataType_U32, &currentEnemyCollision->mpSrcModel, NULL, NULL, "%08X");
        ImGui::InputScalar(_("uknSrcModelPtr"), ImGuiDataType_U32, &currentEnemyCollision->uknSrcModelPtr, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mPushType"), ImGuiDataType_U32, &currentEnemyCollision->mPushType, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mpPushModel"), ImGuiDataType_U32, &currentEnemyCollision->mpPushModel, NULL, NULL, "%08X");
        break;
    case 4:
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p0"), &currentEnemyCollision->mPushCap.p0.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p1"), &currentEnemyCollision->mPushCap.p1.x);
        ImGui::SetNextItemWidth(currentItemWidth); ImGui::InputFloat(_("mPushCap r"), &currentEnemyCollision->mPushCap.r);
        ImGui::InputScalar(_("field3_0x24"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field3_0x24, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field4_0x25"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field4_0x25, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field5_0x26"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field5_0x26, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field6_0x27"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field6_0x27, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field7_0x28"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field7_0x28, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field8_0x29"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field8_0x29, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field9_0x2a"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field9_0x2a, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field10_0x2b"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field10_0x2b, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field11_0x2c"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field11_0x2c, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field12_0x2d"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field12_0x2d, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field13_0x2e"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field13_0x2e, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field14_0x2f"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field14_0x2f, NULL, NULL, "%02X");
        break;
    case 5:
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p0"), &currentEnemyCollision->mPushCap1.p0.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushCap p1"), &currentEnemyCollision->mPushCap1.p1.x);
        ImGui::SetNextItemWidth(currentItemWidth); ImGui::InputFloat(_("mPushCap r"), &currentEnemyCollision->mPushCap1.r);
        ImGui::InputScalar(_("field3_0x24"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field3_0x24, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field4_0x25"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field4_0x25, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field5_0x26"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field5_0x26, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field6_0x27"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field6_0x27, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field7_0x28"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field7_0x28, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field8_0x29"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field8_0x29, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field9_0x2a"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field9_0x2a, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field10_0x2b"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field10_0x2b, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field11_0x2c"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field11_0x2c, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field12_0x2d"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field12_0x2d, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field13_0x2e"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field13_0x2e, NULL, NULL, "%02X");
        ImGui::InputScalar(_("field14_0x2f"), ImGuiDataType_U8, &currentEnemyCollision->mPushCap1.field14_0x2f, NULL, NULL, "%02X");
        break;
    case 6:
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushPos"), &currentEnemyCollision->mPushPos.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat3(_("mPushPosOld"), &currentEnemyCollision->mPushPosOld.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m1"), &currentEnemyCollision->field50_0x1c0.m1.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m2"), &currentEnemyCollision->field50_0x1c0.m2.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m3"), &currentEnemyCollision->field50_0x1c0.m3.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field50_0x1c0 m4"), &currentEnemyCollision->field50_0x1c0.m4.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m1"), &currentEnemyCollision->field51_0x200.m1.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m2"), &currentEnemyCollision->field51_0x200.m2.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m3"), &currentEnemyCollision->field51_0x200.m3.x);
        ImGui::SetNextItemWidth(currentItemWidth * 2.0f); ImGui::InputFloat4(_("field51_0x200 m4"), &currentEnemyCollision->field51_0x200.m4.x);
        ImGui::InputScalar(_("uknToggle"), ImGuiDataType_U32, &currentEnemyCollision->uknToggle, NULL, NULL, "%08X");
        ImGui::InputScalar(_("currentMotBuffer"), ImGuiDataType_U32, &currentEnemyCollision->currentMotBuffer, NULL, NULL, "%08X");
        ImGui::InputScalar(_("MotSeq1"), ImGuiDataType_U32, &currentEnemyCollision->MotSeq1, NULL, NULL, "%08X");
        ImGui::InputScalar(_("MotSeq2"), ImGuiDataType_U32, &currentEnemyCollision->MotSeq2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("HitCheckFlag"), ImGuiDataType_U8, &currentEnemyCollision->HitCheckFlag, NULL, NULL, "%02X");
        ImGui::InputScalar(_("HitConfirm"), ImGuiDataType_U8, &currentEnemyCollision->HitConfirm, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mSelfCollision"), ImGuiDataType_U8, &currentEnemyCollision->mSelfCollision, NULL, NULL, "%02X");
        ImGui::InputScalar(_("mMode"), ImGuiDataType_U32, &currentEnemyCollision->mMode, NULL, NULL, "%08X");
        break;
    case 7:
        ImGui::InputScalar(_("mVsAttrPlAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlAtk, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPlDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPlPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPlGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlGrb, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPsAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsAtk, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPsDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPsPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrPsGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsGrb, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEmAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmAtk, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEmDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEmPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEmGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmGrb, NULL, NULL, "%08X");
        break;
    case 8:
        ImGui::InputScalar(_("mVsAttrEsAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsAtk, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEsDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEsPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrEsGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsGrb, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrSetAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetAtk, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrSetDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrSetPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrSetGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetGrb, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrStgAtk"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgAtk, NULL, NULL, "%08X");
        break;
    case 9:
        ImGui::InputScalar(_("mVsAttrStgDmg"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgDmg, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrStgPsh"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgPsh, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mVsAttrStgGrb"), ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgGrb, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrAtk1"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrAtk1, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrDmg1"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrDmg1, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrPsh1"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrPsh1, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrGrb1"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrGrb1, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrAtk2"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrAtk2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrDmg2"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrDmg2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrPsh2"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrPsh2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrGrb2"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrGrb2, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrAtk3"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrAtk3, NULL, NULL, "%08X");
        break;
    case 10:
        ImGui::InputScalar(_("UknVsAttrDmg3"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrDmg3, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrPsh3"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrPsh3, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrGrb3"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrGrb3, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrAtk4"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrAtk4, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrDmg4"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrDmg4, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrPsh4"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrPsh4, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknVsAttrGrb4"), ImGuiDataType_U32, &currentEnemyCollision->UknVsAttrGrb4, NULL, NULL, "%08X");
        ImGui::InputScalar(_("UknCollisionToggle"), ImGuiDataType_U32, &currentEnemyCollision->UknCollisionToggle, NULL, NULL, "%08X");
        ImGui::InputScalar(_("mCollisionGroupNum"), ImGuiDataType_U32, &currentEnemyCollision->mCollisionGroupNum, NULL, NULL, "%08X");
        break;
    default:
        break;
    }
}

void FlyingStats::on_frame(fmilliseconds& dt) {
    if (SMediator* sMedPtr = devil4_sdk::get_sMediator()) {
        if (!showFlyingStats) { return; }
        uPlayer* player = sMedPtr->player_ptr;
        if (!player) { return; }
        if (showFlyingEnemyStats) {
            int enemyCount = 0;
            uEnemy* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                glm::vec3 objectPosition = enemy->position;
                float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                std::string windowName = "EnemyStats##" + std::to_string((uintptr_t)enemy);
                float currentFontScale = 0.8f * guiFriendlyDistance;
                float currentItemWidth = (sameLineItemWidth / 2.0f) * guiFriendlyDistance;
                if (w2s::IsVisibleOnScreen(objectPosition)) {
                    ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove);
                    ImGui::PushID((uintptr_t)enemy);
                    ImGui::SetWindowPos(screenPos);
                    ImGui::SetWindowFontScale(currentFontScale);
                    ImGui::PushItemWidth(currentItemWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                    uDamage* currentEnemyDamage = (uDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                    if (showFlyingHP) ImGui::SliderFloat("HP##EnemyFly", &currentEnemyDamage->HP, 0.0f, currentEnemyDamage->HPMax, "%.1f");
                    if (showFlyingDamageTaken) ImGui::InputFloat("PrevDamage##EnemyFly", &currentEnemyDamage->HPTaken, NULL, NULL, "%.1f");
                    if (showFlyingDamageResist) ImGui::InputFloat("PrevDamageResist##EnemyFly", &currentEnemyDamage->prevDamageResist, NULL, NULL, "%.1f");
                    if (showFlyingDT) ImGui::InputFloat("DT Timer##EnemyFly", &enemy->DTTimer, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                    if (showFlyingStun) ImGui::InputInt("Stun##EnemyFly", &currentEnemyDamage->stun[0], NULL, NULL);
                    if (showFlyingDisplacement) ImGui::InputInt("Displacement##EnemyFly", &currentEnemyDamage->displacement[0], NULL, NULL);
                    // if (showFlyingStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &currentEnemyDamage->stunResetTimer, 0.0f, 180.0f, "%.0f");
                    if (showFlyingMoveID) ImGui::InputScalar("MoveID##EnemyFly", ImGuiDataType_U8, &enemy->moveID);
                    if (showFlyingMechanics) {
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
                        if (enemy->ID == FROST) {
                            ImGui::SliderInt("Heal Count", &enemy->frostHealCount, 0, 5);
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
                    if (showFlyingDebug) {
                        ImGui::PushItemWidth(currentItemWidth * 2.0f);
                        ImGui::InputScalar(_("Base Addr"), ImGuiDataType_U32, &enemy, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
                        ImGui::InputFloat3(_("Position##EnemyFly"), (float*)&enemy->position);
                        ImGui::InputFloat3(_("Rotation##EnemyFly"), (float*)&enemy->rotation);
                        ImGui::InputFloat3(_("Velocity##EnemyFly"), (float*)&enemy->velocity);
                        ImGui::InputFloat3(_("Scale##EnemyFly"), (float*)&enemy->scale);
                        ImGui::PopItemWidth();
                        ImGui::InputScalar(_("Anim ID##EnemyFly"), ImGuiDataType_U16, &enemy->animID);
                        ImGui::InputScalar(_("Move Part##EnemyFly"), ImGuiDataType_U8, &enemy->movePart);
                        ImGui::SliderFloat(_("Animation Frame##EnemyFly"), &enemy->animFrame, 0.0f, enemy->animFrameMax);
                        ImGui::InputScalar(_("Team##EnemyFly"), ImGuiDataType_U8, &enemy->team);
                        ImGui::InputScalar(_("ID##EnemyFly"), ImGuiDataType_U8, &enemy->ID);
                        ImGui::InputInt(_("Number##EnemyFly"), &enemyCount, 0, 0);
                    }
                    if (showFlyingCollisionData) {
                        uCollisionMgr* currentEnemyCollision = (uCollisionMgr*)((char*)enemy + EnemyTracker::get_enemy_specific_uCollision_offset(enemy->ID));
                        DisplayCollisionData(currentEnemyCollision, currentItemWidth);
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
        if (showFlyingPlayerStats) {
            glm::vec3 objectPosition = player->m_pos;
            float objectDistance = w2s::GetDistanceFromCam(objectPosition);
            float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
            glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
            std::string windowName = "PlayerStats##" + std::to_string((uintptr_t)player);
            float currentFontScale = 0.8f * guiFriendlyDistance;
            float currentItemWidth = (sameLineItemWidth / 2.0f) * guiFriendlyDistance;
            if (w2s::IsVisibleOnScreen(objectPosition)) {
                ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove);
                ImGui::PushID((uintptr_t)player);
                ImGui::SetWindowPos(screenPos);
                ImGui::SetWindowFontScale(currentFontScale);
                ImGui::PushItemWidth(currentItemWidth);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                if (showFlyingHP) ImGui::SliderFloat("HP##EnemyFly", &player->damageStruct.HP, 0.0f, player->damageStruct.HPMax, "%.1f");
                if (showFlyingDamageTaken) ImGui::InputFloat("PrevDamage##EnemyFly", &player->damageStruct.HPTaken, NULL, NULL, "%.1f");
                if (showFlyingDamageResist) ImGui::InputFloat("PrevDamageResist##EnemyFly", &player->damageStruct.prevDamageResist, NULL, NULL, "%.1f");
                if (showFlyingDT) ImGui::InputFloat("DT##EnemyFly", &player->DT, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                if (showFlyingStun) ImGui::InputInt("Stun##EnemyFly", &player->damageStruct.stun[4], NULL, NULL);
                // if (showFlyingDisplacement) ImGui::InputInt("Displacement##EnemyFly", &player->damageStruct.displacement[0], NULL, NULL);
                // if (showFlyingStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &player->damageStruct.stunResetTimer, 0.0f, 180.0f, "%.0f");
                if (showFlyingMoveID) ImGui::InputScalar("MoveID##EnemyFly", ImGuiDataType_U8, &player->moveID2);
                if (showFlyingDebug) {
                    ImGui::PushItemWidth(currentItemWidth * 2.0f);
                    ImGui::InputScalar(_("Base Addr"), ImGuiDataType_U32, &player, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputFloat3(_("Position##EnemyFly"), (float*)&player->m_pos);
                    ImGui::InputFloat(_("InertiaXZ##EnemyFly"), (float*)&player->inertia);
                    ImGui::InputFloat(_("InertiaY##EnemyFly"), (float*)&player->inertiaY);
                    ImGui::InputFloat3(_("Velocity##EnemyFly"), (float*)&player->m_d_velocity);
                    ImGui::InputFloat3(_("Scale##EnemyFly"), (float*)&player->m_scale);
                    ImGui::PopItemWidth();
                    ImGui::InputFloat(_("Rotation##EnemyFly"), (float*)&player->rotation2);
                    ImGui::InputScalar(_("Anim ID##EnemyFly"), ImGuiDataType_U16, &player->animID);
                    ImGui::InputScalar(_("Move Part##EnemyFly"), ImGuiDataType_U8, &player->movePart);
                    ImGui::SliderFloat(_("Animation Frame##EnemyFly"), &player->animFrame, 0.0f, player->animFrameMax);
                    ImGui::InputScalar(_("ID##EnemyFly"), ImGuiDataType_U8, &player->controllerID);
                }
                if (showFlyingCollisionData) DisplayCollisionData(player->collisionSettings, currentItemWidth);
                ImGui::PopStyleVar(2);
                ImGui::PopItemWidth();
                ImGui::PopID();
                ImGui::End();
            }
        }
    }
}

std::optional<std::string> FlyingStats::on_initialize() {

    return Mod::on_initialize();
}

void FlyingStats::on_gui_frame(int display) {
    ImGui::Checkbox(_("Flying Stats"), &showFlyingStats);
    ImGui::SameLine();
    help_marker(_("Render ImGui stats on entities"));
    if (showFlyingStats) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Display Enemy Stats"), &showFlyingEnemyStats);
        ImGui::Checkbox(_("Display Player Stats"), &showFlyingPlayerStats);
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Display HP"), &showFlyingHP);
        ImGui::Checkbox(_("Display Damage Taken"), &showFlyingDamageTaken);
        ImGui::Checkbox(_("Display Damage Resist"), &showFlyingDamageResist);
        ImGui::SameLine();
        help_marker("Current difficulty * armour");
        ImGui::Checkbox(_("Display DT Timer"), &showFlyingDT);
        ImGui::Checkbox(_("Display Stun"), &showFlyingStun);
        ImGui::Checkbox(_("Display Displacement"), &showFlyingDisplacement);
        ImGui::Checkbox(_("Display Move ID"), &showFlyingMoveID);
        ImGui::Checkbox(_("Display Mechanics"), &showFlyingMechanics);
        ImGui::Checkbox(_("Display Debug Info"), &showFlyingDebug);
        ImGui::Checkbox(_("Display Collision Info"), &showFlyingCollisionData);
        ImGui::Unindent(lineIndent);
        ImGui::Unindent(lineIndent);
    }
}

void FlyingStats::on_config_load(const utility::Config& cfg) {
    showFlyingStats = cfg.get<bool>("showFlyingStats").value_or(false);
    showFlyingEnemyStats = cfg.get<bool>("showFlyingEnemyStats").value_or(false);
    showFlyingPlayerStats = cfg.get<bool>("showFlyingPlayerStats").value_or(false);
    showFlyingHP = cfg.get<bool>("showFlyingHP").value_or(true);
    showFlyingDT = cfg.get<bool>("showFlyingDT").value_or(false);
    showFlyingDamageTaken = cfg.get<bool>("showFlyingDamageTaken").value_or(false);
    showFlyingDamageResist = cfg.get<bool>("showFlyingDamageResist").value_or(false);
    showFlyingMoveID = cfg.get<bool>("showFlyingMoveID").value_or(false);
    showFlyingStun = cfg.get<bool>("showFlyingStun").value_or(false);
    showFlyingDisplacement = cfg.get<bool>("showFlyingDisplacement").value_or(false);
    showFlyingStunTimer = cfg.get<bool>("showFlyingStunTimer").value_or(false);
    showFlyingMechanics = cfg.get<bool>("showFlyingMechanics").value_or(false);
    showFlyingDebug = cfg.get<bool>("showFlyingDebug").value_or(false);
    showFlyingCollisionData = cfg.get<bool>("showFlyingCollisionData").value_or(false);
}

void FlyingStats::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("showFlyingStats", showFlyingStats);
    cfg.set<bool>("showFlyingEnemyStats", showFlyingEnemyStats);
    cfg.set<bool>("showFlyingPlayerStats", showFlyingPlayerStats);
    cfg.set<bool>("showFlyingHP", showFlyingHP);
    cfg.set<bool>("showFlyingDT", showFlyingDT);
    cfg.set<bool>("showFlyingDamageTaken", showFlyingDamageTaken);
    cfg.set<bool>("showFlyingDamageResist", showFlyingDamageResist);
    cfg.set<bool>("showFlyingMoveID", showFlyingMoveID);
    cfg.set<bool>("showFlyingStun", showFlyingStun);
    cfg.set<bool>("showFlyingMechanics", showFlyingMechanics);
    cfg.set<bool>("showFlyingStunTimer", showFlyingStunTimer);
    cfg.set<bool>("showFlyingDebug", showFlyingDebug);
    cfg.set<bool>("showFlyingCollisionData", showFlyingCollisionData);
}
