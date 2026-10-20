#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"

// Wall run, a MoveTable move, modelled on the game's wall jump (uPlayer::moveActWallJump 0x827200), which is
// ported readably at the bottom of WallRun.cpp as a reference (`wall_jump_port`, not hooked).
//
// Player fields are read by offset through `wallrun::pl::*` + `at<T>()`, not through sdk/uPlayer.hpp: that struct
// is due for a revision, and the names here are the DX9 IDB's (the sdk name follows in brackets where one exists).

namespace wallrun {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

namespace pl {                                       // uPlayer offsets, DX9
    constexpr uintptr_t m_delta_time       = 0x0010; // cUnit: frames elapsed this tick
    constexpr uintptr_t mPos               = 0x0030; // uCoord
    constexpr uintptr_t mWmat              = 0x00A0; // uCoord: row 0 right, 1 up, 2 forward, 3 position (MtVector4 each)
    constexpr uintptr_t mVel               = 0x0EC0; // uActor (groundInertiaX / inertiaY / groundInertiaZ)
    constexpr uintptr_t mAcc               = 0x0ED0;
    constexpr uintptr_t mNowFramePos       = 0x1350; // what RayTest casts from
    constexpr uintptr_t mpMotionListArr    = 0x0A38; // uModel: rMotionList*[16], one per motion bank
    constexpr uintptr_t mMotionFrame0      = 0x0348; // uModel::MotionArray[0].mFrame: the lower-body motion's frame (animFrame)
    constexpr uintptr_t mBtnOn             = 0x140C; // cInterface::mBtnOn.bit (inputHold): 0x4 = jump
    constexpr uintptr_t mBtnTrg            = 0x1410; // (inputPress)
    constexpr uintptr_t mAnlgPow           = 0x141C; // stick deflection, 0 = centred (stickThreshold)
    constexpr uintptr_t mAnlgRadian        = 0x1420; // stick angle as a world yaw (stickFacingWorldAdjusted)
    constexpr uintptr_t charType           = 0x1494; // 1 = Nero (controllerID; Payline's `cmp [esi+0x1494], 1`)
    constexpr uintptr_t mBdyActionLowerNo  = 0x1500; // mBdyAction[1].no (moveBank): 0x18 = wall jump
    constexpr uintptr_t mBdyActionLowerPhase = 0x1504; // mBdyAction[1].phase (movePart)
    constexpr uintptr_t mActLimit          = 0x1550; // yActLimit (kActLimitBit): 1 fall (floor snap / may drop), 2 walk, 4 walkEnd, 8 jump, 0x10 pose, 0x20 poseEnd, 0x40 poseWalk, 0x80 poseWalkEnd, 0x100 poseWalkLo, 0x200 poseWalkEndLo, 0x400 atck, 0x800 guard, 0x1000 ik, 0x2000 atckVl, 0x4000 intr, 0x8000 pause. Loaded per action from uPlayer::actionTbl (action 24 = jump | atck | guard), then edited by the move
    constexpr uintptr_t mActStat           = 0x1554; // ACT_STAT: 1 AST_STAND, 2 AST_JUMP (airborne), 4 AST_ATCK, 8 AST_POSE, 0x10 AST_GUARD, 0x20 AST_RUN, 0x40 AST_FLY; from the same table (0 = keep the previous minus guard)
    constexpr uintptr_t mJumpType          = 0x16B8; // JUMP_TYPE
    constexpr uintptr_t mJumpMode          = 0x16BC; // JUMP_MODE
    constexpr uintptr_t mJumpMoveAng       = 0x16C0; // yaw the jump moves along (inertiaDirectionReal)
    constexpr uintptr_t mRotateIsMove      = 0x1708; // cRotateCtrl at 0x1704: mIsMove / mTarget / mNow
    constexpr uintptr_t mRotateTarget      = 0x170C;
    constexpr uintptr_t mRotateNow         = 0x1710; // facing yaw (rotation2)
    constexpr uintptr_t mActTimer0         = 0x177C;
    constexpr uintptr_t mMotContFree0      = 0x1D7E; // .ctn contact byte free0 (stage trigger)
    constexpr uintptr_t mMoveVel           = 0x1E1C; // horizontal move speed of a jump, loaded by setJumpRate from the jump table; calcSpeedXZ turns it into XZ velocity along mJump.moveAng every tick (sdk: inertia)
    constexpr uintptr_t mMoveAcc           = 0x1E20; // its per-frame change (deceleration), also from the table
    constexpr uintptr_t mMoveAngle         = 0x1E24; // stick yaw used for movement (rotation3)
    constexpr uintptr_t mAddSpd            = 0x1E30; // MtVector3: additive speed the game adds to the motion while mIsAddSpeed (carried momentum, moving floors)
    constexpr uintptr_t mIsAddSpeed        = 0x1E40; // bool
    constexpr uintptr_t mAtckLongCnt       = 0x1E7A;
    constexpr uintptr_t mJumpAtckCnt       = 0x1E7D;
    constexpr uintptr_t mWJumpCnt          = 0x1E7E; // air jumps used since leaving the ground (char)
    constexpr uintptr_t mWallJumpCnt       = 0x1E7F; // wall jumps left (char; canAirHike)
    constexpr uintptr_t mCollMgr           = 0x1E8C; // uCollisionMgr*: +0x1C mLand, +0x1F mWall
    constexpr uintptr_t mpActDataTbl       = 0x1DD0; // kActionData*: +0 wJumpCnt, +1 wallJumpCnt, +2 wallHikeCnt (the maxima)
    constexpr uintptr_t mParamTbl0Data     = 0x1E9C; // float* into the .plp (charactersettingsTwo)
    constexpr uintptr_t mGroundPoly        = 0x1F60; // sCollision::LineResult (0x90): the wall the jump reads
    constexpr uintptr_t mpCoatMdl          = 0x20C4; // uDevil4Model*: plays the same motion numbers (mMotionCtrl[4..6])
    constexpr uintptr_t mCharTblMgr        = 0x1DD4; // cCharTblMgr[2] (0x24 each: [0] upper, [1] lower at 0x1DF8): +0xC mpDataTop, +0x10 mpDataCrnt (the .ctn script cursor), +0x14 mIsDone
    constexpr uintptr_t mIsCancelCheckEnable = 0x1F50; // bool (sdk: fullHouseSomething1): uPlayer::main runs checkBeforeAction / checkAction (checkAtck, jump, gun, pose: every cancel) only while set. setActionUpperBdy sets it, setActionAtck / setActionDamage clear it, each attack function sets it again at its cancel frame
    constexpr uintptr_t isWallFront        = 0x1FF0;
    constexpr uintptr_t mDownMaxSpd        = 0x2164;
    constexpr uintptr_t mMotionCtrl0No     = 0x28A8; // cMotionCtrl[0].mMotionNo
    constexpr uintptr_t mMotionSetCharTblParam = 0x2A3C; // kMotCharTblSetParam[2] {bool isEnable; u32 bank; u32 idx}, 12 B each:
                                                         // [0] upper, [1] lower. Set before requestSetMotion: updateMotion
                                                         // points that half's cCharTblMgr at .ctn bank/idx instead of
                                                         // deriving them from the motion number, then clears it.
    constexpr uintptr_t mMotionFrameEnd0   = 0x2A54; // bool: lower-body motion reached its end
    constexpr uintptr_t mMotCncl           = 0x30C0; // cMotCancel[8], 0x2C each, .mStat at +4 (1 = buffer window, 2 = cancel open): C_NORM0..2, C_COMMAND, C_JUMP, C_CHOUHATSU, C_GUN, C_ALL
    constexpr uintptr_t mMotCnclJumpStat   = 0x3174; // mMotCncl[4] (the C_JUMP window, 0x30C0 + 4 * 0x2C) .mStat: 2 = open
    constexpr uintptr_t mMotCnclJumpReserve = 0x3178; // mMotCncl[4].mIsReserve: a jump press is buffered for it
}

enum JUMP_TYPE : int { JUMP_TYPE_F = 0, JUMP_TYPE_L = 1, JUMP_TYPE_B = 2, JUMP_TYPE_R = 3, JUMP_TYPE_V = 4 };
enum JUMP_MODE : int { JUMP_MODE_NORM_WALL_LO = 4, JUMP_MODE_NORM_WALL_HI = 5 };

// sCollision::Param, as RayTest fills it (sCollision::Param(127, 0x20000, 0, 0, 10) in checkWallJumpBegin).
#pragma pack(push, 2)
struct CollParam {
    int type;                 // 127
    int filter;               // 0x20000 = walls (RayTest COLL_WALL)
    int hitCallback_this, hitCallback;
    void* hitCallback_data;
    int _pad14;
    int handle_exclude_0, handle_exclude_1;   // -1
    int _pad20;
    int correctionNum;        // 10
    unsigned __int8 bAdjMvRelSet, _pad29, bAdjAntiStop, _pad2B;
    float capsuleFwdLimit, capsuleBwdLimit;   // FLT_MAX
    float sweepRadiusCap;     // 0
    int partsFilter;          // 0
    int _pad3C;
    char pad[0x10];
};
#pragma pack(pop)

// sCollision::LineResult, the hitOut of findIntersection (RayTest's LineInfo; uPlayer::mGroundPoly is one).
struct LineResult {
    void* pSbc;               // 0x00
    void* tag;                // 0x04
    void* pPartsInfo;         // 0x08
    uint8_t* pTriangle;       // 0x0C  28-byte .sbc triangle: +0x14 surface_attr (bit 0 = no wall jump)
    uint8_t* pVertex;         // 0x10  16-byte vertices
    char pad14[0xC];
    MtVector3 triPlaneNormal; // 0x20
    MtVector3 triEdge1;       // 0x30
    MtVector3 triEdge2;       // 0x40
    MtVector3 hitPlane;       // 0x50  (RayTest: hitNormal)
    MtVector3 hitPos;         // 0x60
    MtVector3 hitNormal;      // 0x70  (RayTest: sweepDirNeg; which one it is isn't settled, use getNormalToTri)
    float hitDistance;        // 0x80
    char pad84[0xC];
};
static_assert(sizeof(LineResult) == 0x90, "LineResult is the 0x90 PlayerLineResult");

struct Wall {
    MtVector3 normal;         // averaged triangle normal, unit, pointing at the player
    MtVector3 run_dir;        // sideways: cross(normal, up), flipped to agree with the facing; climb: up along the wall
    MtVector3 hit_pos;
    float yaw;                // facing: along run_dir sideways, into the wall for a climb (the game's atan2(x, z) yaw)
    bool on_left;             // the wall is on the player's left (sideways only)
    bool vertical;            // the stick (or the facing) points into the wall within climb_angle: run up it
    LineResult poly;          // the nearer hit, in mGroundPoly's layout (hand-off to the wall jump)
};

} // namespace wallrun

class WallRun : public Mod {
public:
    WallRun() = default;
    std::string get_mod_name() override { return "WallRun"; };

    static bool mod_enabled;
    static bool replace_wall_jump;   // run wall_jump_port in place of uPlayer::moveActWallJump (hook at its entry 0x827200)
    static uintptr_t jmp_ret1;       // 0x827206, after the 6 prologue bytes the hook replaces
    static bool replace_wall_jump_check; // run wall_jump_check_port in place of uPlayer::checkWallJumpBegin (entry 0x804650)
    static uintptr_t jmp_ret2;       // 0x804656
    static bool  require_wall_jumps; // the check's own rule: no entry when mWallJumpCnt is 0 (off = unlimited runs)
    static float check_reach;        // cm the entry check looks forward and back for a wall (the game: 80)
    static float check_height;       // cm above the feet it looks (the game: 10)

    // tunables (ImGui)
    static float run_speed;    // cm per frame along the wall
    static float run_frames;   // longest run in frames, 0 = as long as jump is held
    static float stick_speed;  // cm per frame pushed into the wall so the capsule stays on it
    static float wall_gap;     // cm the position is kept away from the wall plane each tick (0 = off): the root-motion backstop
    static float lose_grace;   // frames the run keeps going on the last wall after the casts stop hitting, before jumping off
    static float cast_length;  // cm, each of the two casts
    static float cast_height;  // cm above the feet the casts start from
    static float cast_angle;   // degrees off the forward axis, per side (45)
    static float corner_dot;   // two hits whose normals' dot is below this are a corner, not a wall
    // Collision channels the casts and the entry check accept (sCollision::Param filter). The game's wall checks use
    // COLL_WALL alone; invisible walls live in the fall-stop channel (what stops Stinger at a ledge).
    enum : uint32_t { COLL_WALL = 0x20000, COLL_FALLSTOP = 0x10000000, COLL_CAMERASTOP = 0x20000000 };
    static uint32_t wall_filter;
    static bool run_on_invisible_walls;   // adds COLL_FALLSTOP
    static bool run_on_camera_walls;      // adds COLL_CAMERASTOP
    static uint32_t filter();             // wall_filter with the two options folded in
    static float climb_angle;  // degrees between the facing and the wall normal below which the run goes up, not sideways
    static float climb_speed;  // cm per frame up the wall
    static float away_deadzone; // a stick leaning away from the wall: its along-wall part below this (0-1) has no effect
    static int   jumps_after_run; // what the run costs: 0 one wall jump (entry's initJumpBegin(3) stands), 1 nothing (refund), 2 reset both counters to the table maxima
    static int motion_left;    // placeholder motions (vanilla wall-jump kick-offs) while the run cycles aren't made
    static int motion_right;
    static int motion_up;

    // Custom LMT (D14 / D15 in MoveDef): loaded into a free bank of the body and coat models at runtime.
    static bool use_custom_lmt;   // off = the placeholder motions above, on = lmt_bank:run_slot_* when the file is there
    static int  lmt_bank;         // 10-14 are free on both characters; 15 is Nero's Buster bank
    static int  run_slot_left;    // block indices inside the custom LMT
    static int  run_slot_right;
    static int  run_slot_up;
    static int  run_slot_jump_right; // jump-out cycles: the kick-off after a run on a right / left wall (3 / 4)
    static int  run_slot_jump_left;
    static float jumpout_launch_frame; // frame of the jump-out cycle at which the jump's velocity is applied (the take-off)
    // Jump-out physics (cm per frame), used instead of the jump table's when `jumpout_custom_physics` is on.
    static bool  jumpout_custom_physics;
    static float jumpout_up;        // vertical speed at take-off
    static float jumpout_out;       // speed away from the wall (along its normal)
    static float jumpout_along;     // speed along the run's direction
    static float jumpout_gravity;   // added to the vertical speed every frame (negative)
    static float jumpout_max_fall;  // fastest fall speed
    static char lmt_body[128];    // game paths without extension, like effect paths
    static bool load_coat_lmt;    // off for now: only the body gets the custom bank; the coat keeps its pose on the run
    static char lmt_coat_nero[128];
    static char lmt_coat_dante[128];

    // Loads the custom LMT into `lmt_bank` of the body and coat models if the slot is empty (a new player object
    // after a level load has empty slots again). Cheap when already loaded: one dword read per model.
    static void ensure_bank(uintptr_t player);
    static bool custom_ready(uintptr_t player);          // the custom LMT is in this player's bank (else placeholders)
    static int  motion_for(uintptr_t player, const wallrun::Wall& wall);   // the motion the run plays now
    static int  placeholder_motion(const wallrun::Wall& wall);   // the vanilla kick-off that stands in (also the .ctn borrowed)

    // The direction the player means: the stick as a level world vector; centred = the run's current direction
    // (into the wall for a climb), or the facing before a run exists. Never the facing during a run. A stick
    // leaning away from the wall counts only by its part along the wall (none below away_deadzone).
    static glm::vec3 heading(uintptr_t player, const wallrun::Wall* running);
    // Two casts at +-cast_angle about the up axis, fanned around `heading`, from the hip. Both must agree (or one
    // must miss) for a wall. Fills normal / hit_pos / poly only; `steer` picks the direction.
    static bool detect(uintptr_t player, wallrun::Wall& out, const glm::vec3& heading);
    // Direction along the wall from `heading`: into the wall within climb_angle = climb, else sideways the way it
    // leans. Fills run_dir / yaw / vertical / on_left.
    static void steer(uintptr_t player, wallrun::Wall& wall, const glm::vec3& heading);

    // The per-tick state function, run in place of uPlayer::moveActWallJump (lower-body action 0x18) by the hook
    // below, so it uses the game's own phase word mBdyAction[1].phase: 0 on entry, PHASE_RUN while running, and
    // the port's 1-4 once the run has handed the wall to the game's wall jump (jump released, or no wall ahead).
    static void __stdcall move(uintptr_t player);

    // uPlayer::checkWallJumpBegin 0x804650 line for line: the trigger of lower-body action 0x18. In a jump, with a
    // jump press (fresh and allowed, or buffered in the C_JUMP window) and a wall jump left, it casts 80 cm back
    // and forward at hip height; on a wall it charges initJumpBegin(3), sets the jump mode and starts the action.
    // Runs in place of the game's when `replace_wall_jump_check` is on. Returns what the game's returns (al).
    static bool wall_jump_check_port(uintptr_t player);

    // uPlayer::moveActWallJump 0x827200 line for line, over the offsets above. Runs in place of the game's when
    // `replace_wall_jump` is on (hook at the function entry 0x827200, `this` in eax).
    static void wall_jump_port(uintptr_t player);

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};
