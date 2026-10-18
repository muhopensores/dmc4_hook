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
    constexpr uintptr_t mBtnOn             = 0x140C; // cInterface::mBtnOn.bit (inputHold): 0x4 = jump
    constexpr uintptr_t mBtnTrg            = 0x1410; // (inputPress)
    constexpr uintptr_t mAnlgPow           = 0x141C; // stick deflection, 0 = centred (stickThreshold)
    constexpr uintptr_t mAnlgRadian        = 0x1420; // stick angle as a world yaw (stickFacingWorldAdjusted)
    constexpr uintptr_t mBdyActionLowerNo  = 0x1500; // mBdyAction[1].no (moveBank): 0x18 = wall jump
    constexpr uintptr_t mBdyActionLowerPhase = 0x1504; // mBdyAction[1].phase (movePart)
    constexpr uintptr_t mActStat           = 0x1554; // bit 0x2 = in a jump
    constexpr uintptr_t mJumpType          = 0x16B8; // JUMP_TYPE
    constexpr uintptr_t mJumpMode          = 0x16BC; // JUMP_MODE
    constexpr uintptr_t mJumpMoveAng       = 0x16C0; // yaw the jump moves along (inertiaDirectionReal)
    constexpr uintptr_t mRotateIsMove      = 0x1708; // cRotateCtrl at 0x1704: mIsMove / mTarget / mNow
    constexpr uintptr_t mRotateTarget      = 0x170C;
    constexpr uintptr_t mRotateNow         = 0x1710; // facing yaw (rotation2)
    constexpr uintptr_t mActTimer0         = 0x177C;
    constexpr uintptr_t mMotContFree0      = 0x1D7E; // .ctn contact byte free0 (stage trigger)
    constexpr uintptr_t mMoveAngle         = 0x1E24; // stick yaw used for movement (rotation3)
    constexpr uintptr_t mAtckLongCnt       = 0x1E7A;
    constexpr uintptr_t mJumpAtckCnt       = 0x1E7D;
    constexpr uintptr_t mWallJumpCnt       = 0x1E7F; // wall jumps left (canAirHike)
    constexpr uintptr_t mCollMgr           = 0x1E8C; // uCollisionMgr*: +0x1C mLand, +0x1F mWall
    constexpr uintptr_t mParamTbl0Data     = 0x1E9C; // float* into the .plp (charactersettingsTwo)
    constexpr uintptr_t mGroundPoly        = 0x1F60; // sCollision::LineResult (0x90): the wall the jump reads
    constexpr uintptr_t isWallFront        = 0x1FF0;
    constexpr uintptr_t mDownMaxSpd        = 0x2164;
    constexpr uintptr_t mMotionCtrl0No     = 0x28A8; // cMotionCtrl[0].mMotionNo
    constexpr uintptr_t mMotionFrameEnd0   = 0x2A54; // bool: lower-body motion reached its end
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
    MtVector3 run_dir;        // cross(normal, up), flipped to agree with the facing
    MtVector3 hit_pos;
    float yaw;                // atan2(run_dir.x, run_dir.z), the game's yaw convention
    bool on_left;             // the wall is on the player's left
    LineResult poly;          // the nearer hit, in mGroundPoly's layout (hand-off to the wall jump)
};

} // namespace wallrun

class WallRun : public Mod {
public:
    WallRun() = default;
    std::string get_mod_name() override { return "WallRun"; };

    static bool mod_enabled;

    // tunables (ImGui)
    static float run_speed;    // cm per frame along the wall
    static float run_frames;   // how long one run lasts
    static float stick_speed;  // cm per frame pushed into the wall so the capsule stays on it
    static float cast_length;  // cm, each of the two casts
    static float cast_height;  // cm above the feet the casts start from
    static float cast_angle;   // degrees off the forward axis, per side (45)
    static float corner_dot;   // two hits whose normals' dot is below this are a corner, not a wall
    static int motion_left;    // LMT motion numbers; placeholders until the LMT has run cycles
    static int motion_right;

    // Two casts at +-cast_angle about the up axis from the hip. Both must agree (or one must miss) for a wall.
    static bool detect(uintptr_t player, wallrun::Wall& out);

    // The per-tick state function. Call it from MoveTable's dispatch while moveID2 is the wall run's row
    // (dante_move_switch: `case <id + 1>: WallRun::move(actor); break;`). Keeps its own phase per player.
    static void __stdcall move(uintptr_t player);

    // Reference: uPlayer::moveActWallJump 0x827200 line for line, over the offsets above. Not hooked.
    static void wall_jump_port(uintptr_t player);

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1;
};
