#pragma once

#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets
#include <cstdint>

// UpperGun: the game's own gun shot on the UPPER half of the body while a custom lower-body move goes on (the wall run's
// "gun during the run", 2026-10-24, made shareable 2026-10-30 for FreeRide). RE: Vibed\RE\player_move_pipeline.md
// "Upper-body gun shots".
//
// The game's own gun rows would take the whole body in the air (the air shots), so while a host move is active the
// PRE-CHECK tick (before checkAction) matches the character's GROUND plain-shot row itself and starts it on the upper
// half only (setActionAtck(id, 0), the way setActionAtckGun starts Nero's Blue Rose while walking), then eats the press,
// the gun reserves and the charge-release command bits so checkAtck sees nothing gun-shaped (charge shots are consumed,
// not fired). Melee rows are untouched. It applies checkAtck's own row tests (unlocked bit, the row's extra condition
// through checkAtckEtcSub, and mid-shot the cancel windows the shot's .ctn opens), so mashing chains shots as on the
// ground, Dante's rapid follow-up rows included; Pandora is left out. The shot then runs exactly as the game's walking
// shot: its upper motion and the elevation layer, upper-body blocks over the host's legs; the Blue Rose's recoil and the
// Coyote-A's cartridges come from the shot functions on their own frames. When it ends, the arms go back to the host's
// lower motion (back_to_lower), not to the gun pose. What keeps the host's move in charge:
//  - setActionUpperBdy writes the whole-player words (mActionNo = 12: checkAction switches on it; mActLimit from action
//    12's row; the shot's own phase 0 adds the fall bit): the words as they were before the first shot are saved and
//    put back every post-action tick (after both halves' state functions, before anything reads them), with the host's
//    facing (the shot's setLockOnTargetAngle 0x820CE0 writes mRotate.mTarget AND mNow);
//  - mPose.dir is pinned to 1 (front) while the host is active: the shot setters pick their row by it, and dir 0 is the
//    full-body standing row, whose leg tracks froze the run cycle;
//  - setActionAtckGun (the Automatic mode's auto-repeat) and the shot functions' last phase call setActionPoseLowerBdy
//    (lower -> the pose): hooked at 0x7AA8C0 to return at once while a host is active;
//  - the shot functions' hand-off to the gun pose, setActionPoseUpperBdy 0x7AA860, is hooked: the upper action ends and,
//    after gun_hold_frames with the arms on the shot's last frame, the host's lower motion goes on the upper / aim
//    controllers at the legs' frame;
//  - a lower-body motion request that isn't the host's (the charge shot's own) is dropped, with its A_STOP;
//  - upper / aim requests get at least upper_blend_min frames (the game cuts in with 0 once the gun counts as held);
//  - the shot action ends on its own (phase 4 at its upper motion's end); shot_end_ticks and the stuck-shot check are
//    safety nets.
// A host whose lower body is itself ATTACK 12 (FreeRide: a MoveTable row, mAtckId = its id) shares the one mAtckId word
// with the shot: setActionAtck writes the gun id into it, and both halves dispatch by it (moveActAtckL / moveActAtckU).
// Such a host gives `lower_atck_id` and calls lower_pass_done() at the end of its state function: the word holds the
// host's id through the pre-check tick and the lower pass (the dispatch reaches the host), the gun id through the upper
// pass (the shot runs), and the post-action tick puts the host's id back. `clear_cancel_gate` keeps
// mIsCancelCehckEnable off while the host is active (the shot's phase 0 turns it on; with it on, checkAction's case 12
// runs the enemy-step / wall-jump / air-hike checks and checkAtck on an attack move that handles its own exits).
// Hosts register from on_initialize; the first registration installs the hooks (the shared pre-check / post-action
// ticks of sdk/InputCommand.hpp, the two pose setters). Per-player state is a 4-slot least-recently-seen table.

namespace upper_gun {

struct Motion {                      // what the arms go back to after a shot
    int      motion = -1;            // the motion word on the lower controller (-1 = nothing to request)
    uint32_t attr   = 0;             // its request attribute
    float    speed  = 1.0f;          // its speed (a stand-in held still: 0)
};

struct Host {
    const char* name = "";
    const bool* enabled = nullptr;                          // the host's "shoot while ..." option (null = always)
    bool (*active)(uintptr_t player) = nullptr;             // the host's lower-body move is running (also from the pose hooks, mid upper pass)
    bool (*facing)(uintptr_t player, float* yaw) = nullptr; // the yaw the body keeps (false / null = leave the facing alone)
    bool (*lower_motion)(uintptr_t player, Motion* out) = nullptr;   // the legs' motion, for the arms after a shot (false = nothing)
    void (*before_back)(uintptr_t player) = nullptr;        // right before that request (a host arms the .ctn borrow for a registry word here)
    uint32_t (*lower_atck_id)(uintptr_t player) = nullptr;  // the host's mAtckId when its lower body is attack 12 (null / 0 = not an attack)
    bool clear_cancel_gate = false;                         // keep mIsCancelCehckEnable off while active
    // tuning, read live (pointers so each host keeps its own tunables and config keys; null = 0)
    const float* shot_end_ticks = nullptr;                  // safety: a shot action still running this many ticks after its start is ended (0 = never)
    const float* upper_blend_min = nullptr;                 // frames: the least interpolation a shot's upper / aim request gets
    const float* back_blend_frames = nullptr;               // frames the arms take to blend back into the lower motion after a shot
    const float* gun_hold_frames = nullptr;                 // frames the gun stays up after the last shot before the arms go back (0 = at once)
    const float* lean_blend_frames = nullptr;               // frames the lean weight takes to come in / go (0 = at once)
};

constexpr int kMaxHosts = 4;
bool add_host(const Host& host);     // from on_initialize; installs the hooks on the first call. false = full, or a hook failed (logged)

bool  busy(uintptr_t player);        // the upper body is in the shot / gun pose, or held up after a shot
float lean_weight(uintptr_t player); // 0-1: how much of a host's torso lean applies (ramped while the gun is up)
void  end(uintptr_t player);         // the host's move ends: the upper action and the saved words go back (call before the exit's setAction)
void  lower_pass_done(uintptr_t player);        // hosts with lower_atck_id: the last statement of the state function
uint32_t effective_atck_id(uintptr_t player);   // mAtckId as the host sees it: its own id while the gun id is swapped in

// checkAtck's row tests for a host that matches a row of its own: the unlocked-moves bitset (vtbl 120 checkAtckBit) and the
// row's extra condition (vtbl 108 checkAtckEtcSub with the row's `ukn` field; 0 = none)
bool row_allowed(uintptr_t player, uint32_t atck_id, uint32_t etc);

// readout
extern int  shots;                   // shots started by the pre-check tick
extern char last[96];
bool buffered(uintptr_t player);     // a shot press waits for the current shot's cancel window
uint32_t shot_id(uintptr_t player);  // the gun row started (0 = none)
bool  shooting(uintptr_t player);    // a shot is running or the gun is held up after one. Not busy(): a host whose own action is
                                     // attack 12 on both halves (FreeRide) reads as upper-busy all the time

// uPlayer offsets the hosts' readouts share (DX9)
constexpr uintptr_t kActionNo       = 0x14F0;   // the whole-player action number (checkAction's switch, setActionStatus)
constexpr uintptr_t kBdyUpperNo     = 0x14F8;   // mBdyAction[0].no: 12 = attack (the shot), 13 / 16 = the gun pose
constexpr uintptr_t kBdyUpperPhase  = 0x14FC;
constexpr uintptr_t kPoseDir        = 0x16C4;   // mPose.dir: 1 front, 2 / 8 sides, 6 / 12 back diagonals, 0 none (the standing row)
constexpr uintptr_t kWpGripId       = 0x1DBC;   // mWeaponCtrl.mGripId
constexpr uintptr_t kWpLongId       = 0x1DB8;   // mWeaponCtrl.mLongId: the CHOSEN long weapon (L2 / R2 advance it at once; what the rows'
                                                 // atckConditionWp is checked against, uPlayer::checkAtckWeapon 0x7E5CB0 (vtbl 111))
constexpr uintptr_t kWpEnableLongId = 0x1DC4;   // mWeaponCtrl.mEnableLongId: the HELD long weapon (Blue Rose = 1), swapped to mLongId by
                                                 // cWeaponCtrl::update once mIsChangeEnable is set (setActionUpperBdy sets it)
constexpr uintptr_t kPoseIsHoldGun  = 0x16F1;   // mPose.isHoldGun
constexpr uintptr_t kVtSetupLongWeaponChange = 140 * 4;   // uPlayer vtbl 140: the gun tables for mLongId (uPlayerDante::setupLongWeaponChange 0x7B8750)
constexpr uintptr_t kIsWpChangeCancel = 0x20C9; // mIsWpChangeCancel
constexpr uintptr_t kCmdFlag        = 0x2184;   // uint32[3]: bit = CMD_ID, set by executeCommandCheck this tick
constexpr uintptr_t kMotCncl        = 0x30C0, kMotCnclSize = 0x2C, kMotCnclReserve = 0x08;   // mMotCncl[k].mIsReserve
constexpr uintptr_t kMotionCtrl     = 0x2894, kMotionCtrlSize = 0x3C;                       // cMotionCtrl[7]
constexpr uintptr_t kMcMotionSetReq = 0x04, kMcMotionNo = 0x14, kMcInterp = 0x1C, kMcAddAttribute = 0x2A;   // byte, uint, float, s16
constexpr int16_t   A_STOP_BIT      = 0x10;
constexpr uintptr_t kModelMotion0Frame = 0x330 + 0x18;   // uModel::MotionArray[0].mFrame (0xE0 per slot, slot = controller type)
constexpr uintptr_t kModelMotionNo  = 0x330 + 0x04, kModelMotionSize = 0xE0;   // MotionArray[i].mMotionNo (u16)
constexpr uintptr_t kIsCancelCheckEnable = 0x1F50;   // bool: uPlayer::main runs checkBeforeAction / checkAction only while set

} // namespace upper_gun
