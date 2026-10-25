#include "ThirdPersonAim.hpp"
#include "WallRun.hpp"               // wallrun::at, the collision structs (CollParam, LineResult); shares no state with WallRun
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"   // the pre-check and post-action ticks
#include "../sdk/Math.hpp"
#include "SpineAim.hpp"              // the chest turns toward the aim before the body does
#include "../sdk/Cam.hpp"            // sCamera: viewport 0's camera
#include <Windows.h>
#include <cmath>
#include <cstring>

#undef naked                         // mod.hpp's `naked` is `static __declspec(naked)`; detour_lock() is declared in the header
#define naked __declspec(naked)

using namespace wallrun;

namespace tps {

bool  mod_enabled    = false;
float sensitivity    = 0.0025f;
bool  invert_y       = false;
bool  invert_x       = false;
float distance       = 220.0f;
float height         = 165.0f;
float shoulder       = 55.0f;
float range          = 6000.0f;
float pitch_up_deg   = 60.0f;
float pitch_down_deg = 60.0f;
bool  crosshair      = true;
bool  spine_twist    = true;
float body_turn_deg  = 45.0f;
float turn_speed_deg = 24.0f;
bool  toggle_aim     = false;
bool  charge_shots   = true;
bool  cam_collision  = true;
float collision_margin = 20.0f;
float strafe_speed   = 1.6f;
bool  gun_pose       = true;
bool  use_mouse      = true;
bool  use_pad        = true;
uint32_t pad_combo   = 0;        // pad bits (mPadBtnOn) that, all held, aim; 0 = none set
float pad_speed_deg  = 3.0f;
float pad_deadzone   = 24.0f;
bool  pad_invert_x   = false;
bool  pad_invert_y   = false;
bool  readout        = false;
uintptr_t jmp_ret_lock = 0;
uintptr_t jmp_ret_turn = 0;
uintptr_t jmp_ret_sync = 0;

namespace {

// uCamera position / up / target, in that order (sdk Cam.hpp; uPlayer::setCameraAngle 0x7ABCB0 takes the look
// direction as +0x50 - +0x30; the IDB's uCamera labels +0x40 / +0x50 the other way round)
constexpr uintptr_t kCamPos = 0x30, kCamUp = 0x40, kCamTarget = 0x50;
constexpr uintptr_t kpLockTargetParent = 0x1FFC, kpLockTarget = 0x2000; // uPlayer (updatePtr clears the target with its parent)
constexpr uintptr_t kTargetPos = 0x10;         // cLockOnTarget::mPos
constexpr uint32_t  BTN_MELEE = 0x1, BTN_GUN = 0x2;
constexpr uint32_t  COLL_SHELL = 0x40400;      // the channel the bullets collide in
constexpr uint32_t  COLL_CAMERA = 0x404000, COLL_CAMERASTOP = 0x20000000;   // what the camera stops at (RayTest's names)
constexpr uintptr_t fn_findIntersection  = 0x0095B320;   // sDevil4Collision::findIntersection(query@eax, inst, seg*, bothSides, hitOut*)
constexpr uintptr_t sDevil4Collision_ptr = 0x00E559D0;
constexpr uintptr_t sMain_ptr = 0x00E5574C, kMouse = 0x10384, kMouseDx = 0x2C, kMouseDy = 0x30;   // DebugCam's mouse reading
constexpr uintptr_t fn_rotateRequest = 0x00458210;   // cRotateCtrl::request(this@eax, target@xmm3, float max step) userpurge
constexpr uintptr_t kRotateCtrl = 0x1704;            // uPlayer::mRotate (cRotateCtrl)
constexpr uintptr_t kBdyUpperNo = 0x14F8;            // mBdyAction[0].no: 12 the shot, 13 / 16 the gun pose
constexpr uintptr_t kPoseStat = 0x16D0, kPoseHoldTimer = 0x16D4;   // mPose.stat (int), mPose.holdTimer (float)
inline bool pose_walk(uintptr_t pl) { const int l = at<int>(pl, pl::mBdyActionLowerNo); return l >= 15 && l <= 17; }

// Strafe speed: the pose walk is the game's slow lock-on walk. Its velocity (mVel +0xEC0 x / +0xEC8 z) is scaled in the
// post-action tick (after the walk set it, before moveMotion moves the player by it) and divided back in the next
// pre-check tick, so the walk's own logic never sees a scaled value (no compounding); the legs' motion (slot 0 of the
// body and the coat: uModel::MotionArray[0].mSpeed, +0x354) plays at the same factor, so the steps keep up.
constexpr uintptr_t kModelSlot0Speed = 0x330 + 0x24;
float g_vel_scale = 1.0f;                       // the factor in mVel right now (1 = untouched)
bool  g_legs_scaled = false;
void set_leg_speed(uintptr_t pl, float k) {
    at<float>(pl, kModelSlot0Speed) = k;
    if (const uintptr_t coat = at<uintptr_t>(pl, pl::mpCoatMdl)) at<float>(coat, kModelSlot0Speed) = k;
}
void unscale_walk(uintptr_t pl) {
    if (g_vel_scale != 1.0f) {
        at<float>(pl, pl::mVel + 0) /= g_vel_scale;
        at<float>(pl, pl::mVel + 8) /= g_vel_scale;
        g_vel_scale = 1.0f;
    }
}
void scale_walk(uintptr_t pl, bool on) {
    if (on && strafe_speed != 1.0f) {
        at<float>(pl, pl::mVel + 0) *= strafe_speed;
        at<float>(pl, pl::mVel + 8) *= strafe_speed;
        g_vel_scale = strafe_speed;
        set_leg_speed(pl, strafe_speed);
        g_legs_scaled = true;
    } else if (g_legs_scaled) {                 // left the pose walk / stopped aiming: the legs at their own speed again
        set_leg_speed(pl, 1.0f);
        g_legs_scaled = false;
    }
}

alignas(16) uint8_t g_fake[0x60] = {};   // the fake cLockOnTarget: +0x10 the aim point; +8 (its actor) stays null
alignas(16) uint8_t g_turn[0x60] = {};   // what setLockOnTargetAngle_2 turns the body toward instead (+0x10 only)
bool      g_was_aiming = false;
ULONGLONG g_cam_ms = 0;                  // when aim_camera last aimed (a stale value means no gameplay camera is running: a cutscene)
uint32_t  g_frame = 0, g_delta_frame = ~0u;   // rendered frames (on_frame), the frame the mouse delta was last applied
float     g_cam_frac = 1.0f;             // how far out the camera is (1 = the full distance; less = pulled in by a wall)
float     g_yaw = 0.0f, g_pitch = 0.0f;  // the aim, the game's yaw convention (dir = (sin, 0, cos)), pitch up positive
bool      g_hit = false;
glm::vec3 g_aim{};
bool      g_lmb_last = false;

bool find_intersection(CollParam* param, MtLineSegment* segment, bool both_sides, LineResult* out) {
    bool hit = false;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov eax, param
        push out
        movzx ecx, byte ptr both_sides
        push ecx
        push segment
        mov edi, sDevil4Collision_ptr
        push dword ptr [edi]
        call fn_findIntersection
        mov byte ptr hit, al
        popad
        mov esp, ebx
        pop ebx
    }
    return hit;
}

CollParam shell_param() {
    CollParam p{};
    p.type             = 127;
    p.filter           = COLL_SHELL;
    p.handle_exclude_0 = -1;
    p.handle_exclude_1 = -1;
    p.correctionNum    = 10;
    p.bAdjMvRelSet     = 1;
    p.bAdjAntiStop     = 1;
    p.capsuleFwdLimit  = 3.4028235e38f;
    p.capsuleBwdLimit  = 3.4028235e38f;
    return p;
}

// the mouse counts with the game in front, unpaused and the dmc4_hook menu closed
bool aim_allowed() {
    if (!mod_enabled || devil4_sdk::is_paused() || g_framework->m_draw_ui) return false;
    return GetForegroundWindow() == g_framework->get_window_handle();
}

// The gamepad, from the game's own pad state (mPeripheral +0x1370, any pad the game reads): the buttons held
// (mPadBtnOn +0x1374, the raw pad bits, before the button config maps them) for the aim combination, and the right
// stick (mAnlgR +0x13D4: x, y int16, len float +0x13D8) for the camera.
constexpr uintptr_t kPadBtnOn = 0x1374, kStickRX = 0x13D4, kStickRY = 0x13D6, kStickRLen = 0x13D8;
constexpr float     kStickMax = 128.0f;         // the stick length's full deflection (setInterfaceAnlgInfo: walk >= 80, run >= 120)
bool pad_combo_held() {
    const uintptr_t pl = (uintptr_t)devil4_sdk::get_local_player();
    return pl && pad_combo && (at<uint32_t>(pl, kPadBtnOn) & pad_combo) == pad_combo;
}

// once per camera frame: held = the right button (or the pad combination) down; toggle = each press flips it (kept
// through a pause / the menu)
bool g_rmb_last = false, g_toggled = false;
bool aim_button() {
    const bool allowed = aim_allowed();
    const bool down = allowed && ((use_mouse && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) || (use_pad && pad_combo_held()));
    const bool pressed = down && !g_rmb_last;
    g_rmb_last = down;
    if (!toggle_aim) return down;
    if (pressed) g_toggled = !g_toggled;
    return allowed && g_toggled;
}

// aiming right now: a gameplay camera aimed within the last 100 ms (a cutscene camera isn't one of them, and then nothing
// aims) and the inputs still count
bool aiming() { return g_was_aiming && aim_allowed() && GetTickCount64() - g_cam_ms < 100; }

uintptr_t local_player() { return (uintptr_t)devil4_sdk::get_local_player(); }

void mouse_delta(int& dx, int& dy) {
    dx = dy = 0;
    const uintptr_t main = *(uintptr_t*)sMain_ptr;
    if (!main) return;
    const uintptr_t mouse = *(uintptr_t*)(main + kMouse);
    if (!mouse) return;
    dx = *(int*)(mouse + kMouseDx);
    dy = *(int*)(mouse + kMouseDy);
}

void hand_target(uintptr_t pl) {                // the bullets read it; updatePtr drops it (no live parent) between ticks
    at<uintptr_t>(pl, kpLockTarget) = (uintptr_t)g_fake;
}

void release(uintptr_t pl) {                    // letting go: no fake left behind for melee homing to find
    if (pl && at<uintptr_t>(pl, kpLockTarget) == (uintptr_t)g_fake) {
        at<uintptr_t>(pl, kpLockTarget) = 0;
        at<uintptr_t>(pl, kpLockTargetParent) = 0;
    }
}

// Free aim: aiming without the lock-on button. Holding it, the game's lock-on target and the code built on it (the
// lock-on camera, the HUD, the lock-on update) take over: they expect an actor behind the target, and handing them the
// fake (no actor) crashed. The guns then aim at the locked enemy as usual; the camera stays the shoulder camera.
constexpr uint32_t BTN_LOCKON = 0x10;
bool free_aim(uintptr_t pl) { return aiming() && !(at<uint32_t>(pl, pl::mBtnOn) & BTN_LOCKON); }

constexpr float kPi = 3.14159265f;
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
inline float wrap_pi(float a) { while (a > kPi) a -= 2.0f * kPi; while (a < -kPi) a += 2.0f * kPi; return a; }

// the yaw from the player to the aim point (the camera sits off to the side, so not the camera's own yaw)
float aim_yaw(uintptr_t pl) {
    const glm::vec3 p = at<MtVector3>(pl, pl::mPos);
    const float dx = g_aim.x - p.x, dz = g_aim.z - p.z;
    return (dx * dx + dz * dz) > 1.0f ? std::atan2(dx, dz) : g_yaw;
}

// Chest first: the body only turns by what the spine can't take. The facing that keeps the aim within body_turn_deg of
// it, from `now`: `now` itself while inside the limit, else the aim yaw minus the limit on the aim's side.
float body_yaw_for(uintptr_t pl, float now) {
    const float aim = aim_yaw(pl);
    const float d = wrap_pi(aim - now), lim = body_turn_deg * kPi / 180.0f;
    if (std::fabs(d) <= lim) return now;
    return wrap_pi(aim - (d > 0.0f ? lim : -lim));
}

// What the body would face before the chest-first limit: where the stick points while it's pushed (the run moves along
// the facing, so a body held at its old facing walked up to body_turn_deg off the crosshair while the gun pose / a shot
// re-ran the turn every tick), else the facing it has.
float base_yaw(uintptr_t pl) {
    return at<int>(pl, pl::mAnlgPow) != 0 ? at<float>(pl, pl::mAnlgRadian) : at<float>(pl, pl::mRotateNow);
}

void rotate_request(uintptr_t pl, float target, float max_step) {
    const uintptr_t ctrl = pl + kRotateCtrl;
    uintptr_t saved = 0;
    _asm {
        pushad
        mov saved, esp
        mov eax, ctrl
        movss xmm3, target
        push max_step
        call fn_rotateRequest
        mov esp, saved
        popad
    }
}

// Charge shots. The gun charge is three input commands (uPlayer::commandTbl 0xC3E328): 0x21 / 0x24 / 0x26 = a gun
// press, then a hold (CMD_TYPE_TAME) of 60 / 240 / 420 frames that fires on release. Their state is the kSPlCmd entries
// of mPlCmd[85] (+0x2190, 20 bytes: step +0, status +1, cmdFlag +0xD, tame +0x10 = the frames held); the charge effects
// (uPlayerNero::updateChargeShotEffect 0x7E8180, Dante's likewise, from movePlayerBefore at the top of the tick) read
// the three cmdFlags' bit 0 as the level and 0x21's tame as the time. The mouse isn't the pad, so executeCommandCheck
// sees no hold and clears them every tick; the pre-check tick (after it) writes the held time back with step 0 (the
// game's check then only clears it, it can never fire from a pad that isn't held) and, on release, sets the reached
// level's mCmdFlag bit, so the game's own charge row starts.
constexpr uintptr_t kPlCmd = 0x2190, kPlCmdSize = 20, kCmdStep = 0x0, kCmdStatus = 0x1, kCmdCmdFlag = 0xD, kCmdTame = 0x10;
constexpr uintptr_t kCmdFlagWords = 0x2184;     // mCmdFlag[3]: bit = CMD_ID, read by checkAtck this tick
struct ChargeLevel { uint32_t cmd; float frames; };
constexpr ChargeLevel kCharge[3] = { { 0x21, 60.0f }, { 0x24, 240.0f }, { 0x26, 420.0f } };
float g_charge = 0.0f;                          // frames the left button has been held while aiming

void write_charge(uintptr_t pl, float frames) {
    for (const ChargeLevel& c : kCharge) {
        const uintptr_t s = pl + kPlCmd + kPlCmdSize * c.cmd;
        at<uint8_t>(s, kCmdStep) = 0;
        at<uint8_t>(s, kCmdStatus) = 0;
        at<float>(s, kCmdTame) = frames;
        at<uint8_t>(s, kCmdCmdFlag) = frames >= c.frames ? 1 : 0;
    }
}

// The stick follows the shoulder camera. The stick's world yaw (mInterface.mAnlg.radian +0x1420) is built by
// uPlayer::setInterfaceAnlgInfo 0x7A9550 as mCamAngl.now + the stick's angle; uPlayer::setCameraAngle 0x7ABCB0 (top of
// main) takes now from the camera, except while mCamAngl.isHold (+0x16FC: then holdAng +0x1700), and the stick angle is
// the held one while mPeripheral.mIsHold (+0x1404): the game keeps running the same way through a camera cut. Aiming,
// the camera is the mouse's, so a held direction ran on the old heading after a camera whip; it is rebuilt here from
// the shoulder camera's yaw and the live stick (mPeripheral.mAnlgL.radian +0x13C4) and the holds dropped.
constexpr uintptr_t kAnlgRadian = 0x1420, kCamAnglNow = 0x16F4, kCamAnglIsHold = 0x16FC, kPeriIsHold = 0x1404, kStickRadian = 0x13C4;
void live_stick(uintptr_t pl) {
    at<float>(pl, kCamAnglNow) = g_yaw;
    at<bool>(pl, kCamAnglIsHold) = false;
    at<bool>(pl, kPeriIsHold) = false;
    if (at<int>(pl, pl::mAnlgPow) != 0) at<float>(pl, kAnlgRadian) = wrap_pi(g_yaw + at<float>(pl, kStickRadian));
}

// Pre-check tick (after executeCommandCheck, before checkAction): the stick on the shoulder camera; the fake target
// back in mpLockTarget (updatePtr dropped it); the left button is the gun button (press, hold), melee waits; the charge
// as above.
bool     g_capturing = false;                   // the GUI's Capture: the pad buttons pressed together until all are let go
uint32_t g_capture_bits = 0;

void precheck_tick(uintptr_t pl) {
    if (pl != local_player()) return;
    unscale_walk(pl);                           // last tick's strafe speed out of mVel before anything reads it
    if (g_capturing) {
        const uint32_t bits = at<uint32_t>(pl, kPadBtnOn);
        if (bits) g_capture_bits |= bits;
        else if (g_capture_bits) { pad_combo = g_capture_bits; g_capture_bits = 0; g_capturing = false; }
    }
    if (!aiming()) { g_lmb_last = false; g_charge = 0.0f; return; }
    live_stick(pl);
    if (free_aim(pl)) hand_target(pl);
    else release(pl);
    // The gun up while aiming: the game's own gun pose. checkPoseBegin 0x803990 starts it (setActionPose: action 13)
    // while mPose.stat (+0x16D0) is 1, the weapon can pose and mActLimit allows (0x10); checkPoseRunBegin 0x803A40 turns
    // it into the pose walk (15 -> 16 / 17: the strafing walk) when moving; checkPoseEnd 0x8039F0 ends it once stat is 0,
    // which uPlayer::main sets when the hold timer (+0x16D4) runs out without lock-on held. So stat and the timer are
    // held up while aiming, and the gun counter kept at 1 (the pose aims up / down only "in battle": mAtckLongCnt or
    // enemies around; checkPoseBegin zeroes it). Not on the wall run (its own pose hooks keep the run's legs).
    if (gun_pose && at<int>(pl, pl::mBdyActionLowerNo) != 0x18) {
        at<int>(pl, kPoseStat) = 1;
        if (at<float>(pl, kPoseHoldTimer) < 30.0f) at<float>(pl, kPoseHoldTimer) = 30.0f;
        const int upper = at<int>(pl, kBdyUpperNo);
        if (upper >= 13 && upper <= 17 && at<uint8_t>(pl, pl::mAtckLongCnt) == 0) at<uint8_t>(pl, pl::mAtckLongCnt) = 1;
    }
    // the left mouse button is the gun button; the pad's own gun button needs nothing (its hold charges natively)
    const bool lmb = use_mouse && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    if (lmb) at<uint32_t>(pl, pl::mBtnOn) |= BTN_GUN;
    if (lmb && !g_lmb_last) { at<uint32_t>(pl, pl::mBtnTrg) |= BTN_GUN; g_charge = 0.0f; }
    if (charge_shots) {
        if (lmb) {
            g_charge += at<float>(pl, pl::m_delta_time);
            write_charge(pl, g_charge);
        } else if (g_lmb_last) {                // released: the highest level reached fires
            for (int i = 2; i >= 0; --i)
                if (g_charge >= kCharge[i].frames) {
                    at<uint32_t>(pl, kCmdFlagWords + 4 * (kCharge[i].cmd >> 5)) |= 1u << (kCharge[i].cmd & 31);
                    break;
                }
            g_charge = 0.0f;
            write_charge(pl, 0.0f);
        }
    }
    g_lmb_last = lmb;
    at<uint32_t>(pl, pl::mBtnTrg) &= ~BTN_MELEE;
    at<uint32_t>(pl, pl::mBtnOn)  &= ~BTN_MELEE;
}

// After the state functions (cRotateCtrl::update runs after this, 0x7A862F): a bullet made this tick sets up from the
// target, and the facing is held to the aim. Whatever this tick asked the body to turn to (mRotate.mTarget while
// mIsMove: a shot, the gun pose, Dante's setNearEmAngl toward the nearest enemy, any move's own turn), or the facing
// itself, is cut back to within body_turn_deg of the aim, so nothing swings the body (and the chest on it) off the
// crosshair; inside the limit the chest covers the rest. Walking outside a gun action is left to the game, whose run
// moves along the facing.
void post_action_tick(uintptr_t pl) {
    if (pl != local_player()) return;
    scale_walk(pl, aiming() && pose_walk(pl));  // the strafe speed (also off again once not aiming / not walking)
    if (!aiming()) return;
    if (!free_aim(pl)) { release(pl); return; }  // lock-on held: the game's target and turns
    hand_target(pl);
    if (pose_walk(pl)) return;                  // the pose walk faces the aim and strafes: the game's own turn
    const int upper = at<int>(pl, kBdyUpperNo);
    const bool gun = upper == 12 || upper == 13 || upper == 16;
    const bool stick = at<int>(pl, pl::mAnlgPow) != 0;
    if (stick && !gun) return;
    const float goal = stick ? base_yaw(pl)
                     : at<bool>(pl, pl::mRotateIsMove) ? at<float>(pl, pl::mRotateTarget) : at<float>(pl, pl::mRotateNow);
    const float want = body_yaw_for(pl, goal);
    const float from = stick ? at<float>(pl, pl::mRotateNow) : goal;   // pushed: turn toward the stick ourselves (the gun
    if (want != from) rotate_request(pl, want, turn_speed_deg * kPi / 180.0f);   // pose's own turn faces its target)
}

// SpineAim asks: while aiming, the spine twists toward the aim, all of it on spine joints 1 / 2 (the arms hang off 2;
// a share on the neck only turned the head and the gun fell short), within their limits and slew, measured against the
// body's facing, which is what the shot and pose motions aim along (against the chest, the shots' side-on stances threw
// the gun off); the body follows only past body_turn_deg.
bool spine_provider(uintptr_t pl, spine_aim::Request& out) {
    if (!spine_twist || pl != local_player() || !free_aim(pl)) return false;
    out.aim = true;
    out.wanted_set = true;
    out.wanted_yaw = aim_yaw(pl);
    out.ref_facing = true;
    out.aim_arms = true;
    return true;
}

} // namespace

// The shoulder camera, written into the active camera at the entry of uDevilCamera::sync 0x52EAC0, which builds the view
// matrix and the frustum from uCamera::mCameraPos / mTargetPos / mCameraUp (+0x30 / +0x40 / +0x50): there for the boss
// cameras only. The normal and the fixed / rail rooms are uCameraCtrl, whose controller (cCameraPlayer, or a
// cCameraRailBase: Pan / Rail / RailEm025 / RailEm026 / Normal) computes the view each frame from its own previous
// position (smoothing, wall checks); writing the camera object under it fought the controller (seen in-game), so for
// those the controller's output is replaced instead (its cCameraBase +0x10 / +0x20 / +0x30, after its Main ran): the
// next frame's smoothing then starts from the shoulder view, and letting go glides back.
constexpr uintptr_t kCameraCtrlVtbl = 0xBC6C20;    // uCameraCtrl::vftable
constexpr uintptr_t kCamMain        = 0x417A10;    // cCameraPlayer::Main (thiscall, no arguments), vtable slot 0xB986B0
constexpr uintptr_t kRailMain       = 0x428400;    // cCameraRailBase::Main, the fixed / rail controllers' slot 5
constexpr uintptr_t kRailSlots[6]   = { 0xB98B34, 0xB98B74, 0xB98BB4, 0xB98BF4, 0xB98C34, 0xB98C74 };   // RailBase, Pan, Rail, RailEm025, RailEm026, Normal
constexpr uintptr_t kCtlPos = 0x10, kCtlTarget = 0x20, kCtlUp = 0x30;    // cCameraBase::mCameraPos / mTargetPos / mCameraUp

static uintptr_t viewport0_camera() {
    const uintptr_t main = *(uintptr_t*)sMain_ptr;
    if (!main) return 0;
    const sCamera* cam = *(sCamera**)(main + 0x10358);
    return cam ? (uintptr_t)cam->viewports[0].mpCamera : 0;
}

// The shoulder camera written at `cam`'s position / target / up fields; the aim point from it.
static void aim_apply(uintptr_t cam, uintptr_t off_pos, uintptr_t off_target, uintptr_t off_up) {
    const uintptr_t pl = local_player();
    const bool button = aim_button();
    if (!pl || !button) {
        if (g_was_aiming) release(pl);
        g_was_aiming = false;
        return;
    }
    if (!g_was_aiming) {                        // start from where the game's camera looks
        const glm::vec3 d = glm::vec3(at<MtVector3>(cam, off_target)) - glm::vec3(at<MtVector3>(cam, off_pos));
        g_yaw   = std::atan2(d.x, d.z);
        g_pitch = 0.0f;
    }
    int dx = 0, dy = 0;                         // once per rendered frame: some boss cameras run sync several times a frame
    float pad_yaw = 0.0f, pad_pitch = 0.0f;
    if (g_frame != g_delta_frame) {
        g_delta_frame = g_frame;
        if (use_mouse) mouse_delta(dx, dy);
        const float len = at<float>(pl, kStickRLen);
        if (use_pad && len > pad_deadzone) {    // the right stick: degrees per frame, scaled past the dead zone
            const float m = clampf((len - pad_deadzone) / (kStickMax - pad_deadzone), 0.0f, 1.0f) * pad_speed_deg * kPi / 180.0f;
            const float h = at<int16_t>(pl, kStickRX) / len, v = -at<int16_t>(pl, kStickRY) / len;   // right +, up +
            pad_yaw   = (pad_invert_x ? h : -h) * m;   // right lowers the game's yaw
            pad_pitch = (pad_invert_y ? -v : v) * m;
        }
    }
    g_yaw   += (invert_x ? dx : -dx) * sensitivity + pad_yaw;   // the game's yaw grows to the left on screen
    g_pitch += (invert_y ? dy : -dy) * sensitivity + pad_pitch;
    g_pitch  = clampf(g_pitch, -pitch_down_deg * kPi / 180.0f, pitch_up_deg * kPi / 180.0f);
    if (g_yaw > kPi) g_yaw -= 2.0f * kPi;
    if (g_yaw < -kPi) g_yaw += 2.0f * kPi;

    const float cy = std::cos(g_yaw), sy = std::sin(g_yaw), cp = std::cos(g_pitch), sp = std::sin(g_pitch);
    const glm::vec3 fwd(sy * cp, sp, cy * cp);
    const glm::vec3 right(-cy, 0.0f, sy);       // screen right at this yaw (the game's yaw grows to the left: seen in-game)
    const glm::vec3 pivot = glm::vec3(at<MtVector3>(pl, pl::mPos)) + glm::vec3(0.0f, height, 0.0f);
    const glm::vec3 eye_free = pivot + right * shoulder - fwd * distance;   // where the camera sits with nothing in the way
    // camera collision: the line pivot -> camera in the camera channels (walls the game's own camera stops at, and the
    // camera-stop walls); on a hit the camera comes in to collision_margin before it, at once, and eases back out
    const bool first = !g_was_aiming;
    float want = 1.0f;
    if (cam_collision) {
        MtLineSegment cs;
        cs.p0 = pivot;
        cs.p1 = eye_free;
        CollParam cp = shell_param();
        cp.filter = COLL_CAMERA | COLL_CAMERASTOP;
        LineResult ch{};
        const float len = glm::length(eye_free - pivot);
        if (len > 1.0f && find_intersection(&cp, &cs, true, &ch))
            want = clampf((glm::length(glm::vec3(ch.hitPos) - pivot) - collision_margin) / len, 0.05f, 1.0f);
    }
    if (first || want < g_cam_frac) g_cam_frac = want;
    else g_cam_frac += (want - g_cam_frac) * 0.15f;
    const glm::vec3 eye = pivot + (eye_free - pivot) * g_cam_frac;
    at<MtVector3>(cam, off_pos)    = eye;
    at<MtVector3>(cam, off_target) = eye + fwd * 1000.0f;
    at<MtVector3>(cam, off_up)     = glm::vec3(0.0f, 1.0f, 0.0f);

    // the aim point: the crosshair ray from the pivot's depth on (not from the eye: a wall behind the player would
    // catch it), in the bullets' collision channel; the far end when it hits nothing. From the free camera position:
    // pulled in, the camera still looks along the same line
    MtLineSegment seg;
    seg.p0 = eye_free + fwd * distance;
    seg.p1 = eye_free + fwd * range;
    CollParam param = shell_param();
    LineResult hit{};
    g_hit = find_intersection(&param, &seg, false, &hit);
    g_aim = g_hit ? glm::vec3(hit.hitPos) : glm::vec3(seg.p1);
    at<MtVector3>((uintptr_t)g_fake, kTargetPos) = g_aim;

    g_was_aiming = true;
    g_cam_ms = GetTickCount64();
}

// From the uDevilCamera::sync hook: the boss cameras. sync builds the view with MtMatrix::setLookAtRH 0x8B9470
// (target@eax = +0x50, eye@ecx = +0x30, up@edi = +0x40) and then rolls it by uDevilCamera::mRoll (+0xC0). A boss camera
// computes each frame from its own previous position (locked onto the boss); written over for good, it carried on from
// the shoulder view and swung about a strange axis (seen in-game). So it only lends its fields for the build: they are
// saved here and given back when sync returns (detour_sync swaps sync's return address for sync_return_stub), with the
// roll zeroed meanwhile. The view and frustum stay the shoulder camera's.
constexpr uintptr_t kCamRoll = 0xC0;
static uintptr_t s_lent_cam = 0;
static uint8_t   s_lent_fields[0x30];          // +0x30 .. +0x5F: position, up, target
static float     s_lent_roll = 0.0f;
bool __stdcall aim_camera(uintptr_t cam) {         // true = fields lent, give them back on return
    if (!mod_enabled || !cam || cam != viewport0_camera() || *(uintptr_t*)cam == kCameraCtrlVtbl) return false;
    uint8_t saved[0x30];
    std::memcpy(saved, (void*)(cam + kCamPos), sizeof saved);
    const float roll = at<float>(cam, kCamRoll);
    aim_apply(cam, kCamPos, kCamTarget, kCamUp);
    if (!g_was_aiming || !aim_allowed()) return false;   // not aiming: nothing was written
    std::memcpy(s_lent_fields, saved, sizeof saved);
    s_lent_roll = roll;
    s_lent_cam = cam;
    at<float>(cam, kCamRoll) = 0.0f;
    return true;
}
void __stdcall give_back_camera() {
    if (!s_lent_cam) return;
    std::memcpy((void*)(s_lent_cam + kCamPos), s_lent_fields, sizeof s_lent_fields);
    at<float>(s_lent_cam, kCamRoll) = s_lent_roll;
    s_lent_cam = 0;
}

// the gameplay controllers' Main, then the shoulder camera over their output
void __fastcall camera_player(uintptr_t ctl, uintptr_t) {
    ((void(__thiscall*)(uintptr_t))kCamMain)(ctl);
    if (mod_enabled) aim_apply(ctl, kCtlPos, kCtlTarget, kCtlUp);
}
void __fastcall camera_rail(uintptr_t ctl, uintptr_t) {
    ((void(__thiscall*)(uintptr_t))kRailMain)(ctl);
    if (mod_enabled) aim_apply(ctl, kCtlPos, kCtlTarget, kCtlUp);
}

// uDevilCamera::sync 0x52EAC0 (this@ecx, no stack arguments, plain ret): the shoulder camera first. When aim_camera lent
// the boss camera's fields, sync's return address is swapped for sync_return_stub, which gives them back and then
// returns where sync would have (sync isn't re-entered: the boss syncs call it one after another). The 6 bytes replaced
// are `push ebp; mov ebp, esp; and esp, -16`.
static uintptr_t s_sync_ret = 0;
naked void sync_return_stub() {
    _asm {
        push dword ptr [s_sync_ret] // where sync was returning to
        pushad
        call give_back_camera
        popad
        ret
    }
}
naked void detour_sync() {
    _asm {
        pushad
        push ecx
        call aim_camera             // __stdcall: pops its argument; al = lent
        test al, al
        popad                       // flags kept
        jz go
        push eax
        mov eax, [esp + 4]          // sync's return address
        mov [s_sync_ret], eax
        mov eax, offset sync_return_stub
        mov [esp + 4], eax
        pop eax
    go:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [jmp_ret_sync]
    }
}

// uPlayer::getLockOnTarget 0x820AE0 (thiscall: this@ecx, 3 stack arguments, retn 0Ch): the fake target while the local
// player aims. The 6 bytes replaced are `mov eax, [ecx+140Ch]`.
static bool __stdcall lock_fake(uintptr_t pl) { return pl == local_player() && free_aim(pl); }
naked void detour_lock() {
    _asm {
        pushad
        push ecx
        call lock_fake              // __stdcall: pops its argument; al = hand out the fake
        test al, al
        popad
        jnz fake
        mov eax, [ecx + 0x140C]
        jmp dword ptr [jmp_ret_lock]
    fake:
        lea eax, g_fake
        ret 0x0C
    }
}

// uPlayer::setLockOnTargetAngle_2 0x820CE0 (target@eax, this@esi, 3 stack arguments): turns the actor toward the
// target's +0x10 (snap or cRotateCtrl::request). For the fake target it gets a turn point instead, along the facing
// that keeps the aim within body_turn_deg (chest first). The 6 bytes replaced are `push ebp; mov ebp, esp;
// and esp, -16`.
static uintptr_t __stdcall turn_point(uintptr_t target, uintptr_t pl) {
    if (target != (uintptr_t)g_fake || pl != local_player()) return target;
    if (pose_walk(pl)) return target;           // the pose walk: the body faces the aim, the legs strafe
    const float yaw = body_yaw_for(pl, base_yaw(pl));
    const glm::vec3 p = at<MtVector3>(pl, pl::mPos);
    at<MtVector3>((uintptr_t)g_turn, kTargetPos) = p + glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw)) * 100.0f;
    return (uintptr_t)g_turn;
}
static uintptr_t s_turn_target = 0;
naked void detour_turn() {
    _asm {
        pushad
        push esi
        push eax
        call turn_point             // __stdcall: pops both; eax = the target to turn toward
        mov [s_turn_target], eax
        popad
        mov eax, [s_turn_target]
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [jmp_ret_turn]
    }
}

} // namespace tps

// ---------------------------------------------------------------------------------------------------------------
using namespace tps;

std::optional<std::string> ThirdPersonAim::on_initialize() {
    const uintptr_t fn_player = (uintptr_t)&camera_player, fn_rail = (uintptr_t)&camera_rail;
    install_patch_absolute(0xB986B0, patch_player, (const char*)&fn_player, 4);   // cCameraPlayer::vftable slot 5
    for (int i = 0; i < 6; ++i)                                                    // the fixed / rail controllers' slot 5
        install_patch_absolute(kRailSlots[i], patch_rail[i], (const char*)&fn_rail, 4);
    if (!install_hook_absolute(0x52EAC0, hook_sync, &detour_sync, &jmp_ret_sync, 6)) {   // the boss cameras
        spdlog::error("ThirdPersonAim: failed to hook uDevilCamera::sync (0x52EAC0)");
        return "Failed to init ThirdPersonAim (camera hook)";
    }
    if (!install_hook_absolute(0x820AE0, hook_lock, &detour_lock, &jmp_ret_lock, 6)) {
        spdlog::error("ThirdPersonAim: failed to hook uPlayer::getLockOnTarget (0x820AE0)");
        return "Failed to init ThirdPersonAim (lock-on hook)";
    }
    if (!install_hook_absolute(0x820CE0, hook_turn, &detour_turn, &jmp_ret_turn, 6)) {
        spdlog::error("ThirdPersonAim: failed to hook uPlayer::setLockOnTargetAngle_2 (0x820CE0)");
        return "Failed to init ThirdPersonAim (turn hook)";
    }
    spine_aim::add_provider(&spine_provider);           // the chest leads the aim
    if (!input_commands::hook_installed) {
        if (!install_hook_absolute(0x7A83B5, hook_pre, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("ThirdPersonAim: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init ThirdPersonAim (pre-check hook)";
        }
        input_commands::hook_installed = true;
    }
    input_commands::add_tick(&precheck_tick);
    if (!input_commands::post_hook_installed) {
        if (!install_hook_absolute(0x7A8453, hook_post, &input_commands::detour_post, &input_commands::jmp_ret_post, 8)) {
            spdlog::error("ThirdPersonAim: failed to hook uPlayer::main after moveAction (0x7A8453)");
            return "Failed to init ThirdPersonAim (post-action hook)";
        }
        input_commands::post_hook_installed = true;
    }
    input_commands::add_post_tick(&post_action_tick);
    return Mod::on_initialize();
}

void ThirdPersonAim::on_frame(fmilliseconds& dt) {
    ++g_frame;
    if (!mod_enabled || !crosshair || !aiming()) return;
    const ImVec2 size = ImGui::GetIO().DisplaySize;
    const ImVec2 c(size.x * 0.5f, size.y * 0.5f);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 white = IM_COL32(255, 255, 255, 230), black = IM_COL32(0, 0, 0, 160);
    const float gap = 4.0f, len = 9.0f;
    const ImVec2 dirs[4] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (const ImVec2& d : dirs) {
        const ImVec2 a(c.x + d.x * gap, c.y + d.y * gap), b(c.x + d.x * (gap + len), c.y + d.y * (gap + len));
        dl->AddLine(a, b, black, 4.0f);
        dl->AddLine(a, b, white, 2.0f);
    }
    dl->AddCircleFilled(c, 2.0f, g_hit ? IM_COL32(255, 80, 80, 230) : white);
}

void ThirdPersonAim::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Third-person aim"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Hold the right mouse button to aim over the shoulder with the mouse; the left mouse button fires the gun. "
                  "Guns only: melee is ignored while aiming. The bullets, the body turn and the up / down aim go to where the "
                  "crosshair points (a red dot: it is on a surface)."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::Checkbox(_("Toggle aim"), &toggle_aim);
    ImGui::SameLine();
    help_marker(_("Off: aim while the right mouse button (or the gamepad combination) is held. On: each press turns aiming on or off."));
    ImGui::Checkbox(_("Mouse"), &use_mouse);
    ImGui::SameLine();
    help_marker(_("Right mouse button aims, the mouse turns the camera, the left button fires (and charges while held)."));
    ImGui::SameLine();
    ImGui::Checkbox(_("Gamepad"), &use_pad);
    ImGui::SameLine();
    help_marker(_("The button combination below aims, the right stick turns the camera; the gun button fires and charges as usual."));
    if (use_pad) {
        ImGui::Text(_("Aim combination: %s (pad bits %08X)"), pad_combo ? _("set") : _("none"), pad_combo);
        ImGui::SameLine();
        if (g_capturing) {
            ImGui::Text(_("- press the buttons together, then let go (%08X)"), g_capture_bits);
            ImGui::SameLine();
            if (ImGui::Button(_("Cancel"))) { g_capturing = false; g_capture_bits = 0; }
        } else {
            if (ImGui::Button(_("Capture"))) { g_capturing = true; g_capture_bits = 0; }
            ImGui::SameLine();
            if (ImGui::Button(_("Clear"))) pad_combo = 0;
        }
        ImGui::SameLine();
        help_marker(_("Capture, then on the gamepad press every button of the combination together and let go: those buttons, all "
                      "held, aim. The buttons still do their own thing in the game, so pick ones that don't get in the way (the "
                      "stick clicks, say). Leave lock-on out: holding lock-on while aiming hands the target back to the game."));
        ImGui::SliderFloat(_("Right stick speed"), &pad_speed_deg, 0.5f, 10.0f, "%.1f");
        ImGui::SameLine();
        help_marker(_("Degrees per frame the camera turns at full deflection."));
        ImGui::SliderFloat(_("Right stick dead zone"), &pad_deadzone, 0.0f, 100.0f, "%.0f");
        ImGui::SameLine();
        help_marker(_("Stick deflection (0-128) below which the camera doesn't turn."));
        ImGui::Checkbox(_("Invert stick X"), &pad_invert_x);
        ImGui::SameLine();
        ImGui::Checkbox(_("Invert stick Y"), &pad_invert_y);
    }
    ImGui::SliderFloat(_("Mouse sensitivity"), &sensitivity, 0.0005f, 0.01f, "%.4f");
    ImGui::Checkbox(_("Invert X"), &invert_x);
    ImGui::SameLine();
    ImGui::Checkbox(_("Invert Y"), &invert_y);
    ImGui::SliderFloat(_("Camera distance"), &distance, 50.0f, 600.0f, "%.0f");
    ImGui::SliderFloat(_("Camera height"), &height, 50.0f, 300.0f, "%.0f");
    ImGui::SliderFloat(_("Shoulder offset"), &shoulder, -150.0f, 150.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Centimetres to the right of the character; negative puts the camera over the left shoulder."));
    ImGui::SliderFloat(_("Aim range"), &range, 1000.0f, 20000.0f, "%.0f");
    ImGui::SliderFloat(_("Look up limit"), &pitch_up_deg, 10.0f, 89.0f, "%.0f");
    ImGui::SliderFloat(_("Look down limit"), &pitch_down_deg, 10.0f, 89.0f, "%.0f");
    ImGui::Checkbox(_("Crosshair"), &crosshair);
    ImGui::Checkbox(_("Camera collision"), &cam_collision);
    ImGui::SameLine();
    help_marker(_("The camera comes in front of walls between it and the character instead of looking through them, and eases "
                  "back out when the way is clear."));
    if (cam_collision) ImGui::SliderFloat(_("Collision margin"), &collision_margin, 0.0f, 60.0f, "%.0f");
    ImGui::SliderFloat(_("Strafe speed"), &strafe_speed, 1.0f, 2.5f, "%.2fx");
    ImGui::SameLine();
    help_marker(_("How much faster than the game's gun-pose walk the strafing walk goes while aiming (its steps play faster to "
                  "match). 1 = the game's own pace."));
    ImGui::Checkbox(_("Gun up while aiming"), &gun_pose);
    ImGui::SameLine();
    help_marker(_("Aiming holds the game's gun pose: the gun stays raised between shots and, moving, the character walks in the "
                  "gun pose's strafing walk facing the crosshair. Guns that have no pose (Coyote-A, Pandora) keep their usual "
                  "stance. Not on the wall run."));
    ImGui::Checkbox(_("Charge shots"), &charge_shots);
    ImGui::SameLine();
    help_marker(_("Holding the left mouse button charges the gun as holding the gun button does (Blue Rose levels at 1, 4 and 7 "
                  "seconds; Ebony & Ivory's charge shot), with the game's charge effects; letting go fires the charge shot. "
                  "The levels you haven't unlocked don't fire. Not on the wall run, which has no charge shots."));
    ImGui::Checkbox(_("Chest leads the aim"), &spine_twist);
    ImGui::SameLine();
    help_marker(_("The spine twists toward the aim (SpineAim must be on; the twist goes on its lower and upper spine joints, within "
                  "their limits) and the body turns only when the aim is more than the body-turn angle off its facing."));
    ImGui::SliderFloat(_("Body turn angle"), &body_turn_deg, 0.0f, 90.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Degrees between the aim and the body's facing before the body turns. 0 = the body always faces the aim. Keep it "
                  "within what the spine can twist: SpineAim's limits for its first two joints (20 + 25 = 45 by default)."));
    ImGui::SliderFloat(_("Body turn speed"), &turn_speed_deg, 1.0f, 90.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Degrees per frame the body turns while standing still (the game's own gun turn is 24)."));
    ImGui::Checkbox(_("Readout"), &readout);
    ImGui::SameLine();
    help_marker(_("Debug lines: the aim, the aim point, the body's facing against the aim, the charge."));
    if (readout) {
        const uintptr_t pl = local_player();
        ImGui::Text("aiming %s | yaw %.1f pitch %.1f | aim (%.0f, %.0f, %.0f) %s | target handed %s | camera out %.2f | charge %.0f",
                    aiming() ? "yes" : "no", g_yaw * 180.0f / kPi, g_pitch * 180.0f / kPi, g_aim.x, g_aim.y, g_aim.z,
                    g_hit ? "hit" : "far", pl && at<uintptr_t>(pl, kpLockTarget) == (uintptr_t)g_fake ? "yes" : "no", g_cam_frac, g_charge);
        if (pl)
            ImGui::Text("facing %.0f | aim yaw %.0f | aim off the facing %.0f | upper %d lower %d",
                        at<float>(pl, pl::mRotateNow) * 180.0f / kPi, aim_yaw(pl) * 180.0f / kPi,
                        wrap_pi(aim_yaw(pl) - at<float>(pl, pl::mRotateNow)) * 180.0f / kPi, at<int>(pl, kBdyUpperNo),
                        at<int>(pl, pl::mBdyActionLowerNo));
    }
    ImGui::Unindent(lineIndent);
}

void ThirdPersonAim::on_config_load(const utility::Config& cfg) {
    mod_enabled    = cfg.get<bool>("tps_aim").value_or(mod_enabled);
    sensitivity    = cfg.get<float>("tps_sensitivity").value_or(sensitivity);
    invert_x       = cfg.get<bool>("tps_invert_x").value_or(invert_x);
    invert_y       = cfg.get<bool>("tps_invert_y").value_or(invert_y);
    distance       = cfg.get<float>("tps_distance").value_or(distance);
    height         = cfg.get<float>("tps_height").value_or(height);
    shoulder       = cfg.get<float>("tps_shoulder").value_or(shoulder);
    range          = cfg.get<float>("tps_range").value_or(range);
    pitch_up_deg   = cfg.get<float>("tps_pitch_up").value_or(pitch_up_deg);
    pitch_down_deg = cfg.get<float>("tps_pitch_down").value_or(pitch_down_deg);
    crosshair      = cfg.get<bool>("tps_crosshair").value_or(crosshair);
    toggle_aim     = cfg.get<bool>("tps_toggle").value_or(toggle_aim);
    cam_collision  = cfg.get<bool>("tps_cam_collision").value_or(cam_collision);
    collision_margin = cfg.get<float>("tps_collision_margin").value_or(collision_margin);
    gun_pose       = cfg.get<bool>("tps_gun_pose").value_or(gun_pose);
    strafe_speed   = cfg.get<float>("tps_strafe_speed").value_or(strafe_speed);
    use_mouse      = cfg.get<bool>("tps_mouse").value_or(use_mouse);
    use_pad        = cfg.get<bool>("tps_pad").value_or(use_pad);
    pad_combo      = (uint32_t)cfg.get<int>("tps_pad_combo").value_or((int)pad_combo);
    pad_speed_deg  = cfg.get<float>("tps_pad_speed").value_or(pad_speed_deg);
    pad_deadzone   = cfg.get<float>("tps_pad_deadzone").value_or(pad_deadzone);
    pad_invert_x   = cfg.get<bool>("tps_pad_invert_x").value_or(pad_invert_x);
    pad_invert_y   = cfg.get<bool>("tps_pad_invert_y").value_or(pad_invert_y);
    readout        = cfg.get<bool>("tps_readout").value_or(readout);
    charge_shots   = cfg.get<bool>("tps_charge").value_or(charge_shots);
    spine_twist    = cfg.get<bool>("tps_spine_twist").value_or(spine_twist);
    body_turn_deg  = cfg.get<float>("tps_body_turn").value_or(body_turn_deg);
    turn_speed_deg = cfg.get<float>("tps_turn_speed").value_or(turn_speed_deg);
}

void ThirdPersonAim::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("tps_aim", mod_enabled);
    cfg.set<float>("tps_sensitivity", sensitivity);
    cfg.set<bool>("tps_invert_x", invert_x);
    cfg.set<bool>("tps_invert_y", invert_y);
    cfg.set<float>("tps_distance", distance);
    cfg.set<float>("tps_height", height);
    cfg.set<float>("tps_shoulder", shoulder);
    cfg.set<float>("tps_range", range);
    cfg.set<float>("tps_pitch_up", pitch_up_deg);
    cfg.set<float>("tps_pitch_down", pitch_down_deg);
    cfg.set<bool>("tps_crosshair", crosshair);
    cfg.set<bool>("tps_toggle", toggle_aim);
    cfg.set<bool>("tps_cam_collision", cam_collision);
    cfg.set<float>("tps_collision_margin", collision_margin);
    cfg.set<bool>("tps_gun_pose", gun_pose);
    cfg.set<float>("tps_strafe_speed", strafe_speed);
    cfg.set<bool>("tps_mouse", use_mouse);
    cfg.set<bool>("tps_pad", use_pad);
    cfg.set<int>("tps_pad_combo", (int)pad_combo);
    cfg.set<float>("tps_pad_speed", pad_speed_deg);
    cfg.set<float>("tps_pad_deadzone", pad_deadzone);
    cfg.set<bool>("tps_pad_invert_x", pad_invert_x);
    cfg.set<bool>("tps_pad_invert_y", pad_invert_y);
    cfg.set<bool>("tps_readout", readout);
    cfg.set<bool>("tps_charge", charge_shots);
    cfg.set<bool>("tps_spine_twist", spine_twist);
    cfg.set<float>("tps_body_turn", body_turn_deg);
    cfg.set<float>("tps_turn_speed", turn_speed_deg);
}
