#include "FlyingStats.hpp"
#include "EnemyTracker.hpp" // for enemy specific offsets
#include "sdk/World2Screen.hpp"
#include "sdk/StringData.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/sMediator.hpp"
#include <corecrt_math_defines.h>
#include "imgui_internal.h"

#include "sdk/uCollisionMgr.hpp"

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
bool FlyingStats::showFlyingCancelBools = false;
bool FlyingStats::showFlyingLuciferPinTimers = false;


static void DisplayCollisionData(uCollisionMgr* currentEnemyCollision, float currentItemWidth) {
    uintptr_t collisionSettingsAddress = *(uintptr_t*)&currentEnemyCollision;
    ImGui::SetNextItemWidth(currentItemWidth);
    if (ImGui::InputInt(_("Page##EnemyFly"), &FlyingStats::collisionPage)) {
        if (FlyingStats::collisionPage < 0) FlyingStats::collisionPage = 0;
        if (FlyingStats::collisionPage > 21) FlyingStats::collisionPage = 21;
    }
    switch (FlyingStats::collisionPage) {
    case 0:
        ImGui::InputScalar("Address", ImGuiDataType_U32, &collisionSettingsAddress, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputScalar(
            "vtable_ptr", ImGuiDataType_U32, &currentEnemyCollision->vtable_ptr, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputScalar("flags.raw", ImGuiDataType_U32, &currentEnemyCollision->flags.raw, NULL, NULL, "%08X");
        ImGui::InputScalar("mp_next_unit", ImGuiDataType_U32, &currentEnemyCollision->mp_next_unit, NULL, NULL, "%08X");
        ImGui::InputScalar("mp_prev_unit", ImGuiDataType_U32, &currentEnemyCollision->mp_prev_unit, NULL, NULL, "%08X");
        ImGui::InputFloat("m_delta_time", &currentEnemyCollision->m_delta_time);
        ImGui::InputScalarN("reserved_state_flags", ImGuiDataType_U8, currentEnemyCollision->reserved_state_flags, 4, NULL, NULL, "%02X");
        break;
    case 1:
        ImGui::Checkbox("mEnable", &currentEnemyCollision->mEnable);
        ImGui::Checkbox("mDamage", &currentEnemyCollision->mDamage);
        ImGui::Checkbox("mPsh", &currentEnemyCollision->mPsh);
        ImGui::Checkbox("mCapsuleLand", &currentEnemyCollision->mCapsuleLand);
        ImGui::Checkbox("mLand", &currentEnemyCollision->mLand);
        ImGui::Checkbox("mNoLand", &currentEnemyCollision->mNoLand);
        ImGui::Checkbox("mMoveLand", &currentEnemyCollision->mMoveLand);
        ImGui::Checkbox("mWall", &currentEnemyCollision->mWall);
        ImGui::Checkbox("mCeilling", &currentEnemyCollision->mCeilling);
        ImGui::Checkbox("mHitCap", &currentEnemyCollision->mHitCap);
        ImGui::Checkbox("mMoveWall", &currentEnemyCollision->mMoveWall);
        break;

    case 2:
        ImGui::Checkbox("mNoLandTimerEnable", &currentEnemyCollision->mNoLandTimerEnable);
        ImGui::InputFloat("mNoLandTimer", &currentEnemyCollision->mNoLandTimer);
        ImGui::InputFloat("mNoLandTime", &currentEnemyCollision->mNoLandTime);
        ImGui::Checkbox("mObjLand", &currentEnemyCollision->mObjLand);
        ImGui::Checkbox("mOldLand", &currentEnemyCollision->mOldLand);
        ImGui::Checkbox("mOldMoveLand", &currentEnemyCollision->mOldMoveLand);
        ImGui::Checkbox("mOldWall", &currentEnemyCollision->mOldWall);
        ImGui::Checkbox("mOldCeilling", &currentEnemyCollision->mOldCeilling);
        ImGui::Checkbox("mCheckWallEnable", &currentEnemyCollision->mCheckWallEnable);
        ImGui::Checkbox("mCheckGroundEnable", &currentEnemyCollision->mCheckGroundEnable);
        break;
    case 3:
        ImGui::Checkbox("mFall", &currentEnemyCollision->mFall);
        ImGui::Checkbox("mIsBoss", &currentEnemyCollision->mIsBoss);
        ImGui::Checkbox("mIsDevilTrigger", &currentEnemyCollision->mIsDevilTrigger);
        ImGui::InputScalar("field26_0x3f", ImGuiDataType_U8, &currentEnemyCollision->field26_0x3f, NULL, NULL, "%02X");
        ImGui::InputScalar("mDevilTriggerMode", ImGuiDataType_U32, &currentEnemyCollision->mDevilTriggerMode, NULL, NULL, "%08X");
        ImGui::InputFloat("mDamageValueCorrect", &currentEnemyCollision->mDamageValueCorrect);
        ImGui::Checkbox("mPushStraight", &currentEnemyCollision->mPushStraight);
        ImGui::InputScalar("field30_0x49", ImGuiDataType_U8, &currentEnemyCollision->field30_0x49, NULL, NULL, "%02X");
        ImGui::InputScalar("field31_0x4a", ImGuiDataType_U8, &currentEnemyCollision->field31_0x4a, NULL, NULL, "%02X");
        ImGui::InputScalar("field32_0x4b", ImGuiDataType_U8, &currentEnemyCollision->field32_0x4b, NULL, NULL, "%02X");
        ImGui::InputFloat3("mPushStraightDir", &currentEnemyCollision->mPushStraightDir.x);
        ImGui::InputScalar("mpPushStraightLockOn", ImGuiDataType_U32, &currentEnemyCollision->mpPushStraightLockOn, NULL, NULL, "%08X");
        break;
    case 4:
        ImGui::InputFloat3("mCorrectPos", &currentEnemyCollision->mCorrectPos.x);

        ImGui::InputFloat4("mCorrectMat.m1", &currentEnemyCollision->mCorrectMat.m1.x);
        ImGui::InputFloat4("mCorrectMat.m2", &currentEnemyCollision->mCorrectMat.m2.x);
        ImGui::InputFloat4("mCorrectMat.m3", &currentEnemyCollision->mCorrectMat.m3.x);
        ImGui::InputFloat4("mCorrectMat.m4", &currentEnemyCollision->mCorrectMat.m4.x);

        ImGui::InputFloat3("mAttackCorrectPos", &currentEnemyCollision->mAttackCorrectPos.x);

        ImGui::InputScalar("mWeightType", ImGuiDataType_U32, &currentEnemyCollision->mWeightType, NULL, NULL, "%08X");
        ImGui::InputScalar("mModelID", ImGuiDataType_U32, &currentEnemyCollision->mModelID, NULL, NULL, "%08X");

        ImGui::InputScalar("mpReportActor", ImGuiDataType_U32, &currentEnemyCollision->mpReportActor, NULL, NULL, "%08X");
        ImGui::InputScalar("mpDstModel", ImGuiDataType_U32, &currentEnemyCollision->mpDstModel, NULL, NULL, "%08X");
        ImGui::InputScalar("mpSrcModel", ImGuiDataType_U32, &currentEnemyCollision->mpSrcModel, NULL, NULL, "%08X");
        ImGui::InputScalar("mpModel", ImGuiDataType_U32, &currentEnemyCollision->mpModel, NULL, NULL, "%08X");
        break;
    case 5:
        ImGui::InputScalar("mppIgnoreModel[0]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[0], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[1]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[1], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[2]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[2], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[3]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[3], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[4]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[4], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[5]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[5], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[6]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[6], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[7]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[7], NULL, NULL, "%08X");
        break;
    case 6:
        ImGui::InputScalar("mppIgnoreModel[8]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[8], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[9]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[9], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[10]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[10], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[11]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[11], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[12]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[12], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[13]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[13], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[14]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[14], NULL, NULL, "%08X");
        ImGui::InputScalar("mppIgnoreModel[15]", ImGuiDataType_U32, &currentEnemyCollision->mppIgnoreModel[15], NULL, NULL, "%08X");
        break;
    case 7:
        ImGui::InputInt("mIgnoreModelNum", &currentEnemyCollision->mIgnoreModelNum);

        ImGui::InputScalar("mPushType", ImGuiDataType_U32, &currentEnemyCollision->mPushType, NULL, NULL, "%08X");
        ImGui::InputScalar("mpPushModel", ImGuiDataType_U32, &currentEnemyCollision->mpPushModel, NULL, NULL, "%08X");

        ImGui::InputFloat3("mPushCap.p0", &currentEnemyCollision->mPushCap.p0.x);
        ImGui::InputFloat3("mPushCap.p1", &currentEnemyCollision->mPushCap.p1.x);
        ImGui::InputFloat("mPushCap.r", &currentEnemyCollision->mPushCap.r);
        break;
    case 8:
        ImGui::InputScalar("mPushCap.field3_0x24", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field3_0x24, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field4_0x25", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field4_0x25, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field5_0x26", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field5_0x26, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field6_0x27", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field6_0x27, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field7_0x28", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field7_0x28, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field8_0x29", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field8_0x29, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field9_0x2a", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field9_0x2a, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field10_0x2b", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field10_0x2b, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field11_0x2c", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field11_0x2c, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field12_0x2d", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field12_0x2d, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field13_0x2e", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field13_0x2e, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushCap.field14_0x2f", ImGuiDataType_U8, &currentEnemyCollision->mPushCap.field14_0x2f, NULL, NULL, "%02X");
        ImGui::InputScalar("mPushPosMode", ImGuiDataType_U32, &currentEnemyCollision->mPushPosMode, NULL, NULL, "%08X");
        ImGui::InputFloat("mPushCapShrinkTimer", &currentEnemyCollision->mPushCapShrinkTimer);
        break;
    case 9:
        ImGui::InputScalar("field54_0x178", ImGuiDataType_U8, &currentEnemyCollision->field54_0x178, NULL, NULL, "%02X");
        ImGui::InputScalar("field55_0x179", ImGuiDataType_U8, &currentEnemyCollision->field55_0x179, NULL, NULL, "%02X");
        ImGui::InputScalar("field56_0x17a", ImGuiDataType_U8, &currentEnemyCollision->field56_0x17a, NULL, NULL, "%02X");
        ImGui::InputScalar("field57_0x17b", ImGuiDataType_U8, &currentEnemyCollision->field57_0x17b, NULL, NULL, "%02X");
        ImGui::InputScalar("field58_0x17c", ImGuiDataType_U8, &currentEnemyCollision->field58_0x17c, NULL, NULL, "%02X");
        ImGui::InputScalar("field59_0x17d", ImGuiDataType_U8, &currentEnemyCollision->field59_0x17d, NULL, NULL, "%02X");
        ImGui::InputScalar("field60_0x17e", ImGuiDataType_U8, &currentEnemyCollision->field60_0x17e, NULL, NULL, "%02X");
        ImGui::InputScalar("field61_0x17f", ImGuiDataType_U8, &currentEnemyCollision->field61_0x17f, NULL, NULL, "%02X");

        ImGui::InputFloat3("mPushLineSgXZ.p0", &currentEnemyCollision->mPushLineSgXZ.p0.x);
        ImGui::InputFloat3("mPushLineSgXZ.p1", &currentEnemyCollision->mPushLineSgXZ.p1.x);

        ImGui::InputFloat3("mPushLineSgY.p0", &currentEnemyCollision->mPushLineSgY.p0.x);
        ImGui::InputFloat3("mPushLineSgY.p1", &currentEnemyCollision->mPushLineSgY.p1.x);
        break;
    case 10:
        ImGui::InputFloat4("mPushNewMat.m1", &currentEnemyCollision->mPushNewMat.m1.x);
        ImGui::InputFloat4("mPushNewMat.m2", &currentEnemyCollision->mPushNewMat.m2.x);
        ImGui::InputFloat4("mPushNewMat.m3", &currentEnemyCollision->mPushNewMat.m3.x);
        ImGui::InputFloat4("mPushNewMat.m4", &currentEnemyCollision->mPushNewMat.m4.x);

        ImGui::InputFloat4("mPushOldMat.m1", &currentEnemyCollision->mPushOldMat.m1.x);
        ImGui::InputFloat4("mPushOldMat.m2", &currentEnemyCollision->mPushOldMat.m2.x);
        ImGui::InputFloat4("mPushOldMat.m3", &currentEnemyCollision->mPushOldMat.m3.x);
        ImGui::InputFloat4("mPushOldMat.m4", &currentEnemyCollision->mPushOldMat.m4.x);
        break;
    case 11:
        ImGui::InputFloat4("mPushNewMatThrough.m1", &currentEnemyCollision->mPushNewMatThrough.m1.x);
        ImGui::InputFloat4("mPushNewMatThrough.m2", &currentEnemyCollision->mPushNewMatThrough.m2.x);
        ImGui::InputFloat4("mPushNewMatThrough.m3", &currentEnemyCollision->mPushNewMatThrough.m3.x);
        ImGui::InputFloat4("mPushNewMatThrough.m4", &currentEnemyCollision->mPushNewMatThrough.m4.x);

        ImGui::Checkbox("mPushUseOld", &currentEnemyCollision->mPushUseOld);

        ImGui::InputScalar("mpCollisionIdxData", ImGuiDataType_U32, &currentEnemyCollision->mpCollisionIdxData, NULL, NULL, "%08X");
        ImGui::InputScalar("mpCollisionShape", ImGuiDataType_U32, &currentEnemyCollision->mpCollisionShape, NULL, NULL, "%08X");
        ImGui::InputScalar("mpAttackStatusData", ImGuiDataType_U32, &currentEnemyCollision->mpAttackStatusData, NULL, NULL, "%08X");
        ImGui::InputScalar("mpDefendStatusData", ImGuiDataType_U32, &currentEnemyCollision->mpDefendStatusData, NULL, NULL, "%08X");
        break;
    case 12:
        ImGui::Checkbox("mInDamageMessage", &currentEnemyCollision->mInDamageMessage);
        ImGui::InputScalar("mMotType", ImGuiDataType_U32, &currentEnemyCollision->mMotType, NULL, NULL, "%08X");
        ImGui::InputScalar("mSeqTrgOn", ImGuiDataType_U32, &currentEnemyCollision->mSeqTrgOn, NULL, NULL, "%08X");
        ImGui::InputScalar("mSeqTrgOff", ImGuiDataType_U32, &currentEnemyCollision->mSeqTrgOff, NULL, NULL, "%08X");

        ImGui::Checkbox("mClearReq", &currentEnemyCollision->mClearReq);
        ImGui::Checkbox("mClear", &currentEnemyCollision->mClear);
        ImGui::Checkbox("mPushMode", &currentEnemyCollision->mPushMode);

        ImGui::InputScalar("mCollisionMgrID", ImGuiDataType_U32, &currentEnemyCollision->mCollisionMgrID, NULL, NULL, "%08X");
        ImGui::InputInt("mUniqueID", &currentEnemyCollision->mUniqueID);
        break;
    case 13:
        ImGui::InputScalar("mVsAttrPlAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPlDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPlPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPlGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlGrb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPlFrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPlFrdAtk, NULL, NULL, "%08X");

        ImGui::InputScalar("mVsAttrPsAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPsDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPsPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPsGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsGrb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrPsFrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrPsFrdAtk, NULL, NULL, "%08X");
        break;
    case 14:
        ImGui::InputScalar("mVsAttrEmAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEmDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEmPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEmGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmGrb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEmFrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEmFrdAtk, NULL, NULL, "%08X");

        ImGui::InputScalar("mVsAttrEsAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEsDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEsPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEsGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsGrb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEsFrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEsFrdAtk, NULL, NULL, "%08X");
        break;
    case 15:
        ImGui::InputScalar("mVsAttrEm2Atk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEm2Atk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEm2Dmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEm2Dmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEm2Psh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEm2Psh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEm2Grb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEm2Grb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEm2FrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEm2FrdAtk, NULL, NULL, "%08X");

        ImGui::InputScalar("mVsAttrEs2Atk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEs2Atk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEs2Dmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEs2Dmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEs2Psh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEs2Psh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEs2Grb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEs2Grb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrEs2FrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrEs2FrdAtk, NULL, NULL, "%08X");
        break;
    case 16:
        ImGui::InputScalar("mVsAttrSetAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrSetDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrSetPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrSetGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetGrb, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrSetFrdAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrSetFrdAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrStgAtk", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgAtk, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrStgDmg", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgDmg, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrStgPsh", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgPsh, NULL, NULL, "%08X");
        ImGui::InputScalar("mVsAttrStgGrb", ImGuiDataType_U32, &currentEnemyCollision->mVsAttrStgGrb, NULL, NULL, "%08X");
        break;
    case 17:
        ImGui::Checkbox("mIsCheckPush", &currentEnemyCollision->mIsCheckPush);
        ImGui::Checkbox("mIsStingerStop", &currentEnemyCollision->mIsStingerStop);
        ImGui::Checkbox("mIsLuciferStick", &currentEnemyCollision->mIsLuciferStick);
        ImGui::InputScalar("mMode", ImGuiDataType_U32, &currentEnemyCollision->mMode, NULL, NULL, "%08X");
        ImGui::InputScalar("mCollisionGroupNum", ImGuiDataType_U32, &currentEnemyCollision->mCollisionGroupNum, NULL, NULL, "%08X");
        break;
    case 18:
        ImGui::InputScalar("mppCollisionGroup[0]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[0], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[1]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[1], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[2]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[2], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[3]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[3], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[4]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[4], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[5]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[5], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[6]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[6], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[7]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[7], NULL, NULL, "%08X");
        break;
    case 19:
        ImGui::InputScalar("mppCollisionGroup[8]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[8], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[9]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[9], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[10]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[10], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[11]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[11], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[12]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[12], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[13]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[13], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[14]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[14], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[15]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[15], NULL, NULL, "%08X");
        break;
    case 20:
        ImGui::InputScalar("mppCollisionGroup[16]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[16], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[17]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[17], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[18]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[18], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[19]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[19], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[20]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[20], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[21]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[21], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[22]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[22], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[23]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[23], NULL, NULL, "%08X");
        break;
    case 21:
        ImGui::InputScalar("mppCollisionGroup[24]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[24], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[25]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[25], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[26]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[26], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[27]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[27], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[28]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[28], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[29]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[29], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[30]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[30], NULL, NULL, "%08X");
        ImGui::InputScalar("mppCollisionGroup[31]", ImGuiDataType_U32, &currentEnemyCollision->mppCollisionGroup[31], NULL, NULL, "%08X");
        break;
    default:
        break;
    }
}

void FlyingStats::on_frame(fmilliseconds& dt) {
    // flawless enemy riding 
    /*uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        uEnemy* enemy = player->lockOnTargetPtr3;
        if (enemy) {
            player->m_pos = { enemy->position.x, enemy->position.y + 100.0f, enemy->position.z };
        }
    }*/

    // stupid funship
    /*uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    static bool previousButtonState = false;
    static float joltTimer = 0.0f;
    static float joltCooldown = 0.0f;
    bool currentButtonState = (player->inputPress[0] & 2);

    if (joltCooldown <= 0.0f) {
        if (currentButtonState && !previousButtonState) {
            player->groundInertiaX += sin(player->stickFacingWorldAdjusted) * 50.0f;
            player->groundInertiaZ += cos(player->stickFacingWorldAdjusted) * 50.0f;
            joltTimer = 1.0f;
            joltCooldown = 50.0f;
        }
    }

    if (joltCooldown > 0.0f) {
        joltCooldown -= player->m_delta_time;
    }

    if (joltTimer <= 0.0f) {
        previousButtonState = currentButtonState;
    }
    if (joltTimer > 0.0f) {
        joltTimer -= player->m_delta_time;
    }
    else {
        player->groundInertiaX *= 0.95f;
        player->groundInertiaZ *= 0.95f;
    }*/
    
    if (!showFlyingStats) { return; }
    if (sMediator* sMedPtr = devil4_sdk::get_sMediator()) {
        uPlayer* player = sMedPtr->player_ptr;
        if (!player) { return; }

        if (showFlyingEnemyStats) {
            int enemyCount = 0;
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                glm::vec3 objectPosition = enemy->position;
                float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                std::string windowName = "EnemyStats##" + std::to_string((uintptr_t)enemy);
                float currentFontScale = 0.8f /** guiFriendlyDistance*/; // malice didn't like it resizing
                float currentItemWidth = (sameLineItemWidth / 2.0f)/* * guiFriendlyDistance*/;
                if (w2s::IsVisibleOnScreen(objectPosition) && objectDistance < 10000.0f) { // distance made it crash for vieris
                    ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NoDecoration |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove);
                    ImGui::PushID((uintptr_t)enemy);
                    ImGui::SetWindowPos(screenPos);
                    ImGui::UpdateCurrentFontSize(currentFontScale * ImGui::GetStyle().FontSizeBase);
                    ImGui::PushItemWidth(currentItemWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                    uDamage_Old* currentEnemyDamage = (uDamage_Old*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                    if (showFlyingHP) ImGui::SliderFloat(_("HP##EnemyFly"), &currentEnemyDamage->HP, 0.0f, currentEnemyDamage->HPMax, "%.1f");
                    if (showFlyingDamageTaken) ImGui::InputFloat(_("PrevDamage##EnemyFly"), &currentEnemyDamage->HPTaken, NULL, NULL, "%.1f");
                    if (showFlyingDamageResist) ImGui::InputFloat(_("PrevDamageResist##EnemyFly"), &currentEnemyDamage->prevDamageResist, NULL, NULL, "%.1f");
                    if (showFlyingDT) ImGui::InputFloat(_("DT Timer##EnemyFly"), &enemy->DTTimer, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                    if (showFlyingStun) ImGui::InputInt(_("Stun##EnemyFly"), &currentEnemyDamage->stun[0], NULL, NULL);
                    if (showFlyingDisplacement) ImGui::InputInt(_("Displacement##EnemyFly"), &currentEnemyDamage->displacement[0], NULL, NULL);
                    // if (showFlyingStunTimer)ImGui::SliderFloat(_("Stun Reset Timer##EnemyFly"), &currentEnemyDamage->stunResetTimer, 0.0f, 180.0f, "%.0f");
                    if (showFlyingMoveID) ImGui::InputScalar(_("MoveID##EnemyFly"), ImGuiDataType_U8, &enemy->moveID);
                    if (showFlyingMechanics) {
                        if (enemy->ID == ANGELO_BIANCO || enemy->ID == ANGELO_ALTO) {
                            ImGui::SliderFloat(_("Shield##EnemyFly"), &enemy->angeloShield, 0.0f, enemy->angeloShieldMax, "%.0f");
                        }
                        if (enemy->ID == MEPHISTO || enemy->ID == FAUST) {
                            if (enemy->faustCloak > 0.0f) {
                                ImGui::SliderFloat(_("Cloak##EnemyFly"), &enemy->faustCloak, 0.0f, enemy->faustCloakMax, "%.0f");
                            }
                            else {
                                ImGui::InputFloat(_("Cloak Timer##EnemyFly"), &enemy->faustCloakTimer, NULL, NULL, "%.0f");
                            }
                        }
                        if (enemy->ID == FROST) {
                            ImGui::SliderInt(_("Heal Count##EnemyFly"), &enemy->frostHealCount, 0, 5);
                        }
                        if (enemy->ID == BLITZ) {
                            if (enemy->blitzElectric > 0.0f) {
                                ImGui::SliderFloat(_("Electric##EnemyFly"), &enemy->blitzElectric, 0.0f, 1000.0f, "%.0f");
                            }
                            else {
                                if (currentEnemyDamage->HP < enemy->blitzElectricSuicideHPRequirement && enemy->blitzElectricSuicideTimer > 0.0f) {
                                    ImGui::SliderFloat(_("Suicide Timer##EnemyFly"), &enemy->blitzElectricSuicideTimer, 0.0f, 1800.0f, "%.0f");
                                }
                                else {
                                    ImGui::SliderFloat(_("Electric Timer##EnemyFly"), &enemy->blitzElectricTimer, 0.0f, 900.0f, "%.0f");
                                }
                            }
                        }
                        if (enemy->ID == GLADIUS) {
                            if (enemy->gladiusBuried) {
                                ImGui::SliderFloat(_("Buried Timer##EnemyFly"), &enemy->gladiusTimer, 0.0f, 300.0f, "%.0f");
                            }
                        }
                        if (enemy->ID == BERIAL) {
                            if (enemy->berialFireTimer > 0.0f) {
                                ImGui::InputFloat(_("Fire Timer##EnemyFly"), &enemy->berialFireTimer, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat(_("Fire Damage##EnemyFly"), &enemy->berialFire, 0.0f, enemy->berialFireMax, "%.0f");
                            }
                        }
                        if (enemy->ID == CREDO) {
                            if (enemy->credoShield > 0.0f) {
                                ImGui::SliderFloat(_("Shield##EnemyFly"), &enemy->credoShield, 0.0f, 4000.0f, "%.0f");
                            }
                            else {
                                ImGui::InputFloat(_("Shield Timer##EnemyFly"), &enemy->credoShieldTimer, NULL, NULL, "%.0f");
                            }
                        }
                        if (enemy->ID == SANCTUS_M11) {
                            if (enemy->sanctusShieldTimerM11 > 0.0f) {
                                ImGui::InputFloat(_("Shield Timer##EnemyFly"), &enemy->sanctusShieldTimerM11, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat(_("Shield Damage##EnemyFly"), &enemy->sanctusShieldM11, 0.0f, 720.0f, "%.0f");
                            }
                        }
                        if (enemy->ID == SANCTUS_M20) {
                            if (enemy->sanctusShieldTimerM20 > 0.0f) {
                                ImGui::InputFloat(_("Shield Timer##EnemyFly"), &enemy->sanctusShieldTimerM20, NULL, NULL, "%.0f");
                            }
                            else {
                                ImGui::SliderFloat(_("Shield Damage##EnemyFly"), &enemy->sanctusShieldM20, 0.0f, 600.0f, "%.0f");
                            }
                        }
                    }
                    if (showFlyingDebug) {
                        ImGui::PushItemWidth(currentItemWidth * 2.0f);
                        ImGui::InputScalar(_("Base Addr##EnemyFly"), ImGuiDataType_U32, &enemy, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
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
            glm::vec3 objectPosition = player->mPos;
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
                ImGui::UpdateCurrentFontSize(currentFontScale * ImGui::GetStyle().FontSizeBase);
                ImGui::PushItemWidth(currentItemWidth);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                if (showFlyingHP) ImGui::SliderFloat(_("HP##EnemyFly"), &player->damageStruct.HP, 0.0f, player->damageStruct.HPMax, "%.1f");
                if (showFlyingDamageTaken) ImGui::InputFloat(_("PrevDamage##EnemyFly"), &player->damageStruct.HPTaken, NULL, NULL, "%.1f");
                if (showFlyingDamageResist) ImGui::InputFloat(_("PrevDamageResist##EnemyFly"), &player->damageStruct.prevDamageResist, NULL, NULL, "%.1f");
                if (showFlyingDT) ImGui::InputFloat(_("DT##EnemyFly"), &player->DT, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                if (showFlyingStun) ImGui::InputInt(_("Stun##EnemyFly"), &player->damageStruct.stun[4], NULL, NULL);
                // if (showFlyingDisplacement) ImGui::InputInt("Displacement##EnemyFly", &player->damageStruct.displacement[0], NULL, NULL);
                // if (showFlyingStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &player->damageStruct.stunResetTimer, 0.0f, 180.0f, "%.0f");
                if (showFlyingMoveID) {
                    ImGui::InputScalar(_("MoveID##PlayerFly"), ImGuiDataType_U8, &player->moveID2);
                    if (ImGui::IsItemHovered()) {
                        if (player->controllerID == 0) {
                            ImGui::BeginTooltip();
                            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                            ImGui::TextUnformatted(dante_attack_names[player->moveID2]);
                            ImGui::PopTextWrapPos();
                            ImGui::EndTooltip();
                        }
                    }
                }
                if (showFlyingDebug) {
                    ImGui::PushItemWidth(currentItemWidth * 2.0f);
                    ImGui::InputScalar(_("Base Addr##EnemyFly"), ImGuiDataType_U32, &player, NULL, NULL, "%08X", ImGuiInputTextFlags_ReadOnly);
                    ImGui::InputFloat3(_("Position##EnemyFly"), (float*)&player->mPos);
                    ImGui::InputFloat(_("InertiaXZ##EnemyFly"), (float*)&player->inertia);
                    ImGui::InputFloat(_("InertiaY##EnemyFly"), (float*)&player->inertiaY);
                    ImGui::InputFloat3(_("Velocity##EnemyFly"), (float*)&player->m_d_velocity);
                    ImGui::InputFloat3(_("Scale##EnemyFly"), (float*)&player->mScale);
                    ImGui::PopItemWidth();
                    ImGui::InputFloat(_("Rotation##EnemyFly"), (float*)&player->rotation2);
                    ImGui::InputScalar(_("Anim ID##EnemyFly"), ImGuiDataType_U16, &player->animID);
                    ImGui::InputScalar(_("Move Part##EnemyFly"), ImGuiDataType_U8, &player->movePart);
                    ImGui::SliderFloat(_("Animation Frame##EnemyFly"), &player->animFrame, 0.0f, player->animFrameMax);
                    ImGui::InputScalar(_("ID##EnemyFly"), ImGuiDataType_U8, &player->controllerID);
                }
                if (showFlyingCollisionData) DisplayCollisionData(player->collisionSettings, currentItemWidth);
                if (showFlyingCancelBools) {
                    ImGui::PushItemWidth(currentItemWidth / 3.0f);
                    ImGui::InputScalar(_("melee+gun"), ImGuiDataType_S8, (bool*)&player->bufferPermissions);               // C_NORM0 = 0x0,
                    ImGui::InputScalar(_("melee"), ImGuiDataType_S8, (bool*)&player->meleeCancelPermissions2);             // C_NORM1 = 0x1,
                    ImGui::InputScalar(_("norm2"), ImGuiDataType_S8, (bool*)&player->unknPermission1);                     // C_NORM2 = 0x2,
                    ImGui::InputScalar(_("directional"),  ImGuiDataType_S8, (bool*)&player->directionalMeleePermissions);  // C_COMMAND = 0x3,
                    ImGui::InputScalar(_("jump"),  ImGuiDataType_S8, (bool*)&player->jumpCancelPermissions2);              // C_JUMP = 0x4,
                    ImGui::InputScalar(_("taunt"), ImGuiDataType_S8, (bool*)&player->unknPermission2);                     // C_CHOUHATSU = 0x5,
                    ImGui::InputScalar(_("gun"),    ImGuiDataType_S8, (bool*)&player->gunCancelPermissions2);              // C_GUN = 0x6,
                    ImGui::InputScalar(_("all"), ImGuiDataType_S8, (bool*)&player->unknPermission3);                       // C_ALL = 0x7,
                    ImGui::SameLine();                                                                                     // CANCEL_NO_NUM = 0x8,
                    ImGui::PopItemWidth();
                }   
                ImGui::PopStyleVar(2);
                ImGui::PopItemWidth();
                ImGui::PopID();
                ImGui::End();

                if (showFlyingLuciferPinTimers && player->controllerID == 0) {
                    for (int i = 0; i < 15; i++) {
                        if (!player->luciferPins[i]) { continue; }
                        glm::vec3 objectPosition{0.0f, 0.0f, 0.0f};
                        objectPosition = player->luciferPins[i]->penetratedPos;
                        glm::vec2 screenPos       = w2s::WorldToScreen(objectPosition);
                        std::string windowName    = "PlayerStats##" + std::to_string((uintptr_t)player->luciferPins[i]);
                        float currentFontScale    = 1.0f;
                        float currentItemWidth    = (sameLineItemWidth);
                        if (w2s::IsVisibleOnScreen(objectPosition)) {
                            ImGui::Begin(windowName.c_str(), NULL,
                                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                            ImGui::PushID((uintptr_t)player->luciferPins[i]);
                            ImGui::SetWindowPos(screenPos);
                            ImGui::UpdateCurrentFontSize(currentFontScale * ImGui::GetStyle().FontSizeBase);
                            ImGui::PushItemWidth(currentItemWidth);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0, 1.0f));
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
                            ImGui::PushItemWidth(currentItemWidth / 3.0f);
                            float countDownTimer = player->luciferPins[i]->timerMax - player->luciferPins[i]->timer;
                            ImGui::SliderFloat("##PinTimer", &countDownTimer, (float)0, player->luciferPins[i]->timerMax, "%.0f");
                            ImGui::PopItemWidth();
                            ImGui::PopStyleVar(2);
                            ImGui::PopItemWidth();
                            ImGui::PopID();
                            ImGui::End();
                        }
                    }
                }
            }
        }
    }
}

std::optional<std::string> FlyingStats::on_initialize() {
    return Mod::on_initialize();
}

void FlyingStats::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
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
            help_marker(_("Current difficulty * armour"));
            ImGui::Checkbox(_("Display DT Timer"), &showFlyingDT);
            ImGui::Checkbox(_("Display Stun"), &showFlyingStun);
            ImGui::Checkbox(_("Display Displacement"), &showFlyingDisplacement);
            ImGui::Checkbox(_("Display Move ID"), &showFlyingMoveID);
            ImGui::Checkbox(_("Display Mechanics"), &showFlyingMechanics);
            ImGui::Checkbox(_("Display Debug Info"), &showFlyingDebug);
            ImGui::Checkbox(_("Display Collision Info"), &showFlyingCollisionData);
            ImGui::Checkbox(_("Display Cancels"), &showFlyingCancelBools);
            ImGui::Checkbox(_("Display Lucifer Pin Timers"), &showFlyingLuciferPinTimers);
            ImGui::Unindent(lineIndent);
            ImGui::Unindent(lineIndent);
        }
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
    showFlyingCancelBools = cfg.get<bool>("showFlyingCancelBools").value_or(false);
    showFlyingLuciferPinTimers = cfg.get<bool>("showFlyingLuciferPinTimers").value_or(false);
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
    cfg.set<bool>("showFlyingCancelBools", showFlyingCancelBools);
    cfg.set<bool>("showFlyingLuciferPinTimers", showFlyingLuciferPinTimers);
}
