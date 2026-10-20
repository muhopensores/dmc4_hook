#pragma once
#include <cstdint>

// uPlayer's per-action table: what each lower-body action allows and which state category it is in.
// Traced 2026-10-20 from the DX9 exe (names from the SE PDB types the IDB carries); see
// E:\DMC mod stuffs\Vibed\RE\player_move_pipeline.md, "Action limits and status".
//
//   uPlayer::setAction(act) 0x7AA030: setActionUpperBdy / setActionLowerBdy (mBdyAction[n] = {act, phase 0}),
//   setActionStatus 0x7ACD70 (mActStat from the row; a 0 row keeps the previous status minus AST_GUARD), then
//   mActLimit = row.actLimit. Rows 0-34 come from uPlayer::actionTbl, 35+ from the character's table
//   (mpActionTbl[1]). The move functions then OR / AND bits of mActLimit as the action progresses.
//
// uPlayer offsets (DX9): mActionNo +0x14F0, mActLimit +0x1550 (sdk/uPlayer.hpp: cancels1), mActStat +0x1554
// (cancels2), mActStatOld +0x1558, mpActionTbl[2] +0x155C.

namespace uplayer_action {

// kActLimitBit: "what may start from this action now" (mActLimit)
enum ACT_LIMIT : uint32_t {
    AL_FALL          = 1 << 0,   // the floor check applies: ground snap (uPlayer::main), may drop off an edge (checkFloor)
    AL_WALK          = 1 << 1,   // checkRunBegin
    AL_WALK_END      = 1 << 2,
    AL_JUMP          = 1 << 3,   // checkWJumpBegin, checkWallJumpBegin, Dante checkBlockActSet
    AL_POSE          = 1 << 4,   // checkPoseBegin (the gun-ready stance)
    AL_POSE_END      = 1 << 5,   // checkPoseEnd
    AL_POSE_WALK     = 1 << 6,
    AL_POSE_WALK_END = 1 << 7,
    AL_POSE_WALK_LO  = 1 << 8,   // lower body walks while the upper body shoots (gun moves OR in 0x301)
    AL_POSE_WALK_END_LO = 1 << 9,// checkPoseRunEnd
    AL_ATCK          = 1 << 10,  // checkAtck: attacks may start (cancels still go through checkAtckCancel)
    AL_GUARD         = 1 << 11,
    AL_IK            = 1 << 12,  // leg IK (no reader found in the player code)
    AL_ATCK_VL       = 1 << 13,  // unknown reader
    AL_INTR          = 1 << 14,  // Damage / Dead / Revive carry intr | pause (event / message side)
    AL_PAUSE         = 1 << 15,
};

// ACT_STAT: the category the current action is in (mActStat)
enum ACT_STAT : uint32_t {
    AST_NONE  = 0,
    AST_STAND = 1 << 0,
    AST_JUMP  = 1 << 1,   // airborne: what every "in the air" test reads
    AST_ATCK  = 1 << 2,   // ORed in by checkAtckSub when an attack starts
    AST_POSE  = 1 << 3,
    AST_GUARD = 1 << 4,
    AST_RUN   = 1 << 5,
    AST_FLY   = 1 << 6,
};

struct kActionTbl {          // one row per action, 8 bytes
    uint32_t actLimit;       // ACT_LIMIT bits
    uint32_t actStatus;      // ACT_STAT bits, 0 = inherit the previous status minus AST_GUARD
};
static_assert(sizeof(kActionTbl) == 8, "kActionTbl is 8 bytes");

// Lower-body action numbers (mBdyAction[1].no / mActionNo), from uPlayer::moveActionLower 0x8215E0 and
// uPlayerDante::moveActionLower 0x7C9670. Nero's 35-43 are dispatched elsewhere and unnamed here.
enum ACTION : int {
    ACT_NONE = 0, ACT_FOOTWORK = 1, ACT_180TURN = 2, ACT_RUN_BEGIN = 3, ACT_RUN = 4, ACT_RUN_END = 5,
    ACT_JUMP_BEGIN = 6, ACT_JUMP = 7, ACT_WJUMP = 8, ACT_STINGER_JUMP = 9, ACT_LAND = 10, ACT_FALL = 11,
    ACT_POSE = 13, ACT_POSE_RUN_BEGIN = 15, ACT_POSE_RUN = 16, ACT_POSE_JUMP_BEGIN = 18,
    ACT_DAMAGE = 20, ACT_DEAD = 21, ACT_REVIVE = 22, ACT_WALL_JUMP = 24, ACT_EM_JUMP = 25, ACT_CATAPULT = 26,
    ACT_HOLY_WATER = 27,
    ACT_CHARACTER_BASE = 35,  // rows of mpActionTbl[1] start here
    // Dante
    ACT_DANTE_STYLE_CHANGE = 35, ACT_DANTE_TS_DASH = 36, ACT_DANTE_TS_AIR_DASH = 37, ACT_DANTE_TS_AIR_TRICK = 38,
    ACT_DANTE_TS_AIR_ESC = 40, ACT_DANTE_TS_AIR_ENEMY_HIKE = 41, ACT_DANTE_GILGAMESH_ROLLING_BEGIN = 42,
    ACT_DANTE_GILGAMESH_ROLLING_B = 43, ACT_DANTE_GILGAMESH_ROLLING_W = 44, ACT_DANTE_RG_BLOCK = 45,
    ACT_DANTE_RG_JUST_BLOCK = 47, ACT_DANTE_RG_GUARD_CRUSH = 48, ACT_DANTE_RG_AIR_BLOCK = 49,
    ACT_DANTE_RG_AIR_JUST_BLOCK = 51, ACT_DANTE_RG_AIR_GUARD_CRUSH = 52, ACT_DANTE_RG_INVINCIBLE = 53,
};

// The game's tables (DX9 .data): the live rows, editable in place.
static kActionTbl* const game_uPlayer_actionTbl      = (kActionTbl*)0x00C776F0;  // 35 rows, actions 0-34
static kActionTbl* const game_uPlayerNero_actionTbl  = (kActionTbl*)0x00C3F908;  //  9 rows, actions 35-43
static kActionTbl* const game_uPlayerDante_actionTbl = (kActionTbl*)0x00C418A0;  // 23 rows, actions 35-57

constexpr uintptr_t uPlayer_mActionNo   = 0x14F0;
constexpr uintptr_t uPlayer_mActLimit   = 0x1550;
constexpr uintptr_t uPlayer_mActStat    = 0x1554;
constexpr uintptr_t uPlayer_mActStatOld = 0x1558;
constexpr uintptr_t uPlayer_mpActionTbl = 0x155C;  // kActionTbl*[2]

inline const kActionTbl& row_for(uintptr_t player, int action) {
    kActionTbl* const* tbl = (kActionTbl* const*)(player + uPlayer_mpActionTbl);
    return action < ACT_CHARACTER_BASE ? tbl[0][action] : tbl[1][action - ACT_CHARACTER_BASE];
}

// Vanilla copies, dumped 2026-10-20 (restore from these after editing the live tables).
static const kActionTbl vanilla_uPlayer_actionTbl[35] = {
    { 0x100B, 0x01 }, //  0 none                   fall | walk | jump | ik / stand
    { 0x1C1B, 0x01 }, //  1 Footwork               fall | walk | jump | pose | atck | guard | ik / stand
    { 0x1C1B, 0x01 }, //  2 180Turn                fall | walk | jump | pose | atck | guard | ik / stand
    { 0x1C19, 0x21 }, //  3 RunBegin               fall | jump | pose | atck | guard | ik / stand | run
    { 0x1C19, 0x21 }, //  4 Run                    fall | jump | pose | atck | guard | ik / stand | run
    { 0x1C19, 0x21 }, //  5 RunEnd                 fall | jump | pose | atck | guard | ik / stand | run
    { 0x1001, 0x01 }, //  6 JumpBegin              fall | ik / stand
    { 0x0808, 0x02 }, //  7 Jump                   jump | guard / jump
    { 0x0C08, 0x02 }, //  8 WJump                  jump | atck | guard / jump
    { 0x0C08, 0x02 }, //  9 StingerJump            jump | atck | guard / jump
    { 0x1C01, 0x01 }, // 10 Land                   fall | atck | guard | ik / stand
    { 0x0C08, 0x02 }, // 11 Fall                   jump | atck | guard / jump
    { 0x0000, 0x00 }, // 12 -                      - / -
    { 0x1C2B, 0x09 }, // 13 PoseL / PoseU          fall | walk | jump | poseEnd | atck | guard | ik / stand | pose
    { 0x1C2B, 0x01 }, // 14 -                      fall | walk | jump | poseEnd | atck | guard | ik / stand
    { 0x1C29, 0x29 }, // 15 PoseRunBeginL / U      fall | jump | poseEnd | atck | guard | ik / stand | pose | run
    { 0x1C29, 0x29 }, // 16 PoseRunL / U           fall | jump | poseEnd | atck | guard | ik / stand | pose | run
    { 0x1C09, 0x21 }, // 17 -                      fall | jump | atck | guard | ik / stand | run
    { 0x3801, 0x09 }, // 18 PoseJumpBegin          fall | guard | ik | atckVl / stand | pose
    { 0x3801, 0x09 }, // 19 -                      fall | guard | ik | atckVl / stand | pose
    { 0xC000, 0x00 }, // 20 Damage                 intr | pause / -
    { 0xC001, 0x01 }, // 21 Dead                   fall | intr | pause / stand
    { 0xC000, 0x01 }, // 22 Revive                 intr | pause / stand
    { 0x5001, 0x01 }, // 23 -                      fall | ik | intr / stand
    { 0x0C08, 0x02 }, // 24 WallJump               jump | atck | guard / jump
    { 0x0C08, 0x02 }, // 25 EmJump                 jump | atck | guard / jump
    { 0x0008, 0x02 }, // 26 Catapult               jump / jump
    { 0x5001, 0x01 }, // 27 HolyWater              fall | ik | intr / stand
    { 0x3801, 0x09 }, // 28 -                      fall | guard | ik | atckVl / stand | pose
    { 0x3801, 0x09 }, // 29 -                      fall | guard | ik | atckVl / stand | pose
    { 0x0808, 0x02 }, // 30 -                      jump | guard / jump
    { 0x3801, 0x09 }, // 31 -                      fall | guard | ik | atckVl / stand | pose
    { 0x3801, 0x01 }, // 32 -                      fall | guard | ik | atckVl / stand
    { 0x0808, 0x02 }, // 33 -                      jump | guard / jump
    { 0x3801, 0x01 }, // 34 -                      fall | guard | ik | atckVl / stand
};

static const kActionTbl vanilla_uPlayerNero_actionTbl[9] = {
    { 0x3801, 0x09 }, // 35 -                      fall | guard | ik | atckVl / stand | pose
    { 0x3801, 0x09 }, // 36 -                      fall | guard | ik | atckVl / stand | pose
    { 0x1C2B, 0x09 }, // 37 -                      fall | walk | jump | poseEnd | atck | guard | ik / stand | pose
    { 0x1C2B, 0x29 }, // 38 -                      fall | walk | jump | poseEnd | atck | guard | ik / stand | pose | run
    { 0x1C08, 0x0A }, // 39 -                      jump | atck | guard | ik / jump | pose
    { 0x0000, 0x0A }, // 40 -                      - / jump | pose
    { 0x0001, 0x00 }, // 41 -                      fall / -
    { 0x000B, 0x01 }, // 42 -                      fall | walk | jump / stand
    { 0x0000, 0x0C }, // 43 -                      - / atck | pose
};

static const kActionTbl vanilla_uPlayerDante_actionTbl[23] = {
    { 0x1809, 0x01 }, // 35 StyleChange            fall | jump | guard | ik / stand
    { 0x5001, 0x01 }, // 36 TsDash                 fall | ik | intr / stand
    { 0x4000, 0x02 }, // 37 TsAirDash              intr / jump
    { 0x4000, 0x00 }, // 38 TsAirTrick             intr / -
    { 0x3801, 0x09 }, // 39 -                      fall | guard | ik | atckVl / stand | pose
    { 0x4000, 0x02 }, // 40 TsAirEsc               intr / jump
    { 0x4000, 0x02 }, // 41 TsAirEnemyHike         intr / jump
    { 0x1001, 0x01 }, // 42 GilgameshRollingBegin  fall | ik / stand
    { 0x0C08, 0x02 }, // 43 GilgameshRollingB      jump | atck | guard / jump
    { 0x0C08, 0x02 }, // 44 GilgameshRollingW      jump | atck | guard / jump
    { 0x5001, 0x01 }, // 45 RgBlock                fall | ik | intr / stand
    { 0x5001, 0x01 }, // 46 -                      fall | ik | intr / stand
    { 0x5001, 0x01 }, // 47 RgJustBlock            fall | ik | intr / stand
    { 0x5001, 0x01 }, // 48 RgGuardCrush           fall | ik | intr / stand
    { 0x4000, 0x02 }, // 49 RgAirBlock             intr / jump
    { 0x4000, 0x02 }, // 50 -                      intr / jump
    { 0x4000, 0x02 }, // 51 RgAirJustBlock         intr / jump
    { 0x4000, 0x02 }, // 52 RgAirGuardCrush        intr / jump
    { 0x5001, 0x01 }, // 53 RgInvincible           fall | ik | intr / stand
    { 0x5001, 0x01 }, // 54 -                      fall | ik | intr / stand
    { 0x0000, 0x00 }, // 55 -                      - / -
    { 0x0000, 0x00 }, // 56 -                      - / -
    { 0x0000, 0x00 }, // 57 -                      - / -
};

} // namespace uplayer_action
