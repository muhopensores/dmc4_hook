#include "WallRun.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/uActor.hpp"
#include "../sdk/InputCommand.hpp"   // the pre-check and post-action ticks
#include "../sdk/Enums_Internal.hpp" // CANCEL_NO
#include "MoveTable.hpp"             // HookNeroKADTbl / HookDanteKADTbl: the gun rows the run matches itself
#include "SpineAim.hpp"              // the torso lean while shooting
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

using namespace wallrun;

bool  WallRun::mod_enabled  = false;
bool  WallRun::replace_wall_jump = true;
uintptr_t WallRun::jmp_ret1 = NULL;
bool  WallRun::replace_wall_jump_check = true;
uintptr_t WallRun::jmp_ret2 = 0;
bool  WallRun::require_wall_jumps = false;
float WallRun::check_reach  = 150.0f;
float WallRun::check_height = 10.0f;
uint32_t WallRun::button_mask = 0x4;
bool  WallRun::gun_while_running = true;
float WallRun::shoot_lean_deg     = -45.0f;   // away from the wall
float WallRun::lean_blend_frames  = 8.0f;
float WallRun::shot_end_ticks = 0.0f;
float WallRun::upper_blend_min = 6.0f;
float WallRun::run_blend_frames = 8.0f;
float WallRun::gun_hold_frames = 10.0f;
bool  WallRun::gun_readout = false;
float WallRun::run_speed    = 12.0f;
float WallRun::run_frames   = 0.0f;     // 0 = as long as jump is held
float WallRun::stick_speed  = 2.0f;
float WallRun::wall_gap     = 30.0f;
float WallRun::lose_grace   = 4.0f;
float WallRun::cast_length  = 120.0f;
float WallRun::cast_height  = 80.0f;
float WallRun::cast_angle   = 45.0f;
float WallRun::corner_dot   = 0.5f;    // normals more than 60 degrees apart = corner
uint32_t WallRun::wall_filter = WallRun::COLL_WALL;
bool  WallRun::run_on_invisible_walls = false;
bool  WallRun::run_on_camera_walls    = false;
float WallRun::climb_angle  = 35.0f;
float WallRun::climb_speed  = 10.0f;
float WallRun::away_deadzone = 0.3f;
int   WallRun::jumps_after_run = 1;
int   WallRun::motion_left  = 23;      // placeholder: the wall-jump side kick-offs (L 23 / R 22, bank 0)
int   WallRun::motion_right = 22;
int   WallRun::motion_up    = 20;      // placeholder: the front kick-off (F, facing the wall)
bool  WallRun::use_custom_lmt  = false;
bool  WallRun::load_coat_lmt   = false;
int   WallRun::lmt_bank        = 14;
int   WallRun::run_slot_left   = 1;      // the custom LMT: 0 right, 1 left, 2 vertical, all attribute 0 (looping)
int   WallRun::run_slot_right  = 0;
int   WallRun::run_slot_up     = 2;
int   WallRun::run_slot_jump_right = 3;
int   WallRun::run_slot_jump_left  = 4;
float WallRun::jumpout_launch_frame = 0.0f;
bool  WallRun::jumpout_custom_physics = true;
float WallRun::jumpout_up       = 22.0f;
float WallRun::jumpout_out      = 14.0f;
float WallRun::jumpout_along    = 6.0f;
float WallRun::jumpout_gravity  = -1.1f;
float WallRun::jumpout_max_fall = 30.0f;
char  WallRun::lmt_body[128]       = "motion\\common\\wallrun";
char  WallRun::lmt_coat_nero[128]  = "motion\\common\\wallrun_pl000_03";
char  WallRun::lmt_coat_dante[128] = "motion\\common\\wallrun_pl006_03";

// ---------------------------------------------------------------------------------------------------------------
// Game functions. All __usercall; each thunk saves the registers and the stack pointer around the call so the
// __userpurge ones (callee pops) are fine too. Addresses are DX9.
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_setAction        = 0x007AA030; // uPlayer::setAction(this@ecx, int act)          vtbl slot 101
static uintptr_t fn_initJumpBegin     = 0x007AC9E0; // uPlayer::initJumpBegin(this@ecx, int)          vtbl slot 109
static uintptr_t fn_setActionLand     = 0x007AA6A0; // uPlayer::setActionLand(this@ecx)
static uintptr_t fn_setActionAtck     = 0x007AAC80; // uPlayer::setActionAtck(this@eax, atckId@ecx, body on the stack) userpurge: body 0 = the upper half only
static uintptr_t fn_setPoseWeapon     = 0x007AADE0; // uPlayer::setPoseWeapon(this@edi) -> al: mPose.wpnId = the long weapon when it can pose, else -1
constexpr int JUMP_INIT_WALL_JUMP     = 3;          // initJumpBegin arg: --mWallJumpCnt, ++mWJumpCnt
static uintptr_t fn_setJumpRate       = 0x007ACC70; // uPlayer::setJumpRate(this@eax)
static uintptr_t fn_adjustJumpSpeed   = 0x007ACCE0; // uPlayer::adjustJumpSpeed(this@esi)
static uintptr_t fn_calcSpeedXZ       = 0x007ACAB0; // uPlayer::calcSpeedXZ(this@esi, float yaw)      userpurge
static uintptr_t fn_setPadVibration   = 0x007AC130; // uPlayer::setPadVibration(this@edx, id@eax)
static uintptr_t fn_getEfctMtrlFlg    = 0x0045F780; // cUtil::getEfctMtrlFlg(result@eax)
static uintptr_t fn_setEffectStayDir  = 0x004806A0; // sDevil4Effect::setEffectStayDir(list@eax, idx@ecx, inst, dir*, pos*, group, material, line, owner) userpurge
static uintptr_t fn_setEffect         = 0x0081FD60; // uPlayer::setEffect(effect@eax, this@ecx, a, b, c, d)  userpurge
static uintptr_t fn_rotateDifference  = 0x004581A0; // cRotateCtrl::getDifference(a@xmm0, b@xmm1) -> xmm0
static uintptr_t fn_rotateRequest     = 0x00458210; // cRotateCtrl::request(this@eax, target@xmm3, float max) userpurge
static uintptr_t sDevil4Effect_ptr    = 0x00E559C8; // sDevil4Effect::mpInstance

using collision::find_intersection;   // the line casts (sdk/Collision.hpp)
using collision::tri_normal;

void set_action(uintptr_t pl, int act)        { ((void(__fastcall*)(uintptr_t, int, int))fn_setAction)(pl, 0, act); }
void init_jump_begin(uintptr_t pl, int arg)   { ((void(__fastcall*)(uintptr_t, int, int))fn_initJumpBegin)(pl, 0, arg); }
void set_action_land(uintptr_t pl)            { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

void set_jump_rate(uintptr_t pl) {
    _asm {
        pushad
        mov eax, pl
        call fn_setJumpRate
        popad
    }
}

void adjust_jump_speed(uintptr_t pl) {
    _asm {
        pushad
        mov esi, pl
        call fn_adjustJumpSpeed
        popad
    }
}

void calc_speed_xz(uintptr_t pl, float yaw) {
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov esi, pl
        push yaw
        call fn_calcSpeedXZ
        popad
        mov esp, ebx
        pop ebx
    }
}

void set_pad_vibration(uintptr_t pl, int id) {
    _asm {
        pushad
        mov edx, pl
        mov eax, id
        call fn_setPadVibration
        popad
    }
}

int efct_mtrl_flag(const LineResult* poly) {
    int r;
    _asm {
        pushad
        mov eax, poly
        call fn_getEfctMtrlFlg
        mov r, eax
        popad
    }
    return r;
}

// The game's wall-kick dust: effect list 0 index 0x18 at `pos`, oriented along `dir`.
void wall_dust_effect(uintptr_t pl, const MtVector3& dir, const MtVector3& pos, int material) {
    const MtVector3* pdir = &dir;
    const MtVector3* ppos = &pos;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push pl                 // owner
        push 0x14               // unit line
        push material
        push 0xFFFFFFFF         // group mask
        push ppos
        push pdir
        mov edi, sDevil4Effect_ptr
        push dword ptr [edi]
        mov ecx, 0x18
        xor eax, eax
        call fn_setEffectStayDir
        mov ecx, pl             // uPlayer::setEffect(effect, this, 1, -1, 1, 1)
        push 1
        push 1
        push 0xFFFFFFFF
        push 1
        call fn_setEffect
        popad
        mov esp, ebx
        pop ebx
    }
}

float rotate_difference(float a, float b) {
    float r;
    _asm {
        movss xmm0, a
        movss xmm1, b
        call fn_rotateDifference
        movss r, xmm0
    }
    return r;
}

void rotate_request(uintptr_t pl, float target, float max_step) {
    uintptr_t ctrl = pl + 0x1704;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov eax, ctrl
        movss xmm3, target
        push max_step
        call fn_rotateRequest
        popad
        mov esp, ebx
        pop ebx
    }
}

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

// ---- small helpers over the player ---------------------------------------------------------------------------

glm::vec3 wmat_row(uintptr_t pl, int row) {           // rows: 0 right, 1 up, 2 forward, 3 position
    const float* r = &at<float>(pl, pl::mWmat + 0x10 * row);
    return glm::vec3(r[0], r[1], r[2]);
}

CollParam wall_param() { return collision::line_param(WallRun::filter()); }   // walls, plus the invisible channels when asked

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }   // the game's yaw: dir = (sin, 0, cos)

constexpr uint32_t ATTR_KICK = 5;      // what the wall jump uses for its kick-off motions
constexpr uint32_t ATTR_FALL = 1;
constexpr uint32_t ATTR_LOOP = 0x15;
constexpr int MOT_FALL       = 5;      // the wall jump's fall motion (4 for a vertical jump, 9 back-facing)
constexpr int ACT_WALL_JUMP  = 0x18;   // mBdyAction[1].no of moveActWallJump

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Wall detection: two casts, +-cast_angle about the up axis, from cast_height above the feet.
// ---------------------------------------------------------------------------------------------------------------
uint32_t WallRun::filter() {
    uint32_t f = wall_filter;
    if (run_on_invisible_walls) f |= COLL_FALLSTOP;
    if (run_on_camera_walls)    f |= COLL_CAMERASTOP;
    return f;
}

glm::vec3 WallRun::heading(uintptr_t player, const Wall* running) {
    if (at<int>(player, pl::mAnlgPow) != 0) {                       // stick deflected: its world yaw
        const float yaw = at<float>(player, pl::mAnlgRadian);
        const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
        if (!running) return stick;
        const glm::vec3 normal = running->normal;
        if (glm::dot(stick, normal) <= 0.0f) return stick;          // into the wall or along it: as is
        // Leaning away from the wall has no effect by itself: only its part along the wall steers, and when
        // that part is small (mostly straight away) the run just keeps going.
        const glm::vec3 along = stick - normal * glm::dot(stick, normal);
        if (glm::length(along) >= away_deadzone) return glm::normalize(along);
    }
    if (running)                                                    // centred (or pushed away): keep going as we were
        return running->vertical ? -glm::vec3(running->normal) : glm::vec3(running->run_dir);
    return wmat_row(player, 2);                                     // no run yet: the facing, once
}

bool WallRun::detect(uintptr_t player, Wall& out, const glm::vec3& heading) {
    const glm::vec3 up    = wmat_row(player, 1);
    glm::vec3 fwd = heading - up * glm::dot(heading, up);            // the casts fan out around the heading, level
    if (glm::length(fwd) < 1e-4f) fwd = wmat_row(player, 2);
    fwd = glm::normalize(fwd);
    glm::vec3 right = glm::cross(up, fwd);
    if (glm::dot(right, wmat_row(player, 0)) < 0.0f) right = -right; // same handedness as the model's right axis
    const glm::vec3 feet  = at<MtVector3>(player, pl::mPos);
    const glm::vec3 origin = feet + up * cast_height;

    const float a = glm::radians(cast_angle);
    const glm::vec3 dirs[2] = {
        glm::normalize(fwd * std::cos(a) - right * std::sin(a)),   // left cast
        glm::normalize(fwd * std::cos(a) + right * std::sin(a)),   // right cast
    };

    CollParam param = wall_param();
    LineResult hits[2]{};
    glm::vec3 normals[2]{};
    int n = 0, nearest = -1;
    for (int i = 0; i < 2; ++i) {
        MtLineSegment seg;
        seg.p0 = origin;
        seg.p1 = origin + dirs[i] * cast_length;
        if (!find_intersection(&param, &seg, false, &hits[i])) continue;
        if (*(uint32_t*)(hits[i].pTriangle + 0x14) & 1) continue;   // surface_attr bit 0: the wall jump refuses it too
        normals[n] = tri_normal(&hits[i]);
        if (nearest < 0 || hits[i].hitDistance < hits[nearest].hitDistance) nearest = i;
        ++n;
    }
    if (n == 0) return false;
    if (n == 2 && glm::dot(normals[0], normals[1]) < corner_dot) return false;   // inside corner: two walls

    glm::vec3 normal = normals[0];
    if (n == 2) normal += normals[1];
    normal = glm::normalize(normal);

    if (glm::length(glm::cross(normal, up)) < 1e-4f) return false;   // floor or ceiling, not a wall

    out.normal  = normal;
    out.hit_pos = hits[nearest].hitPos;
    out.poly    = hits[nearest];
    return true;
}

void WallRun::steer(uintptr_t player, Wall& wall, const glm::vec3& heading) {
    const glm::vec3 right  = wmat_row(player, 0);
    const glm::vec3 up     = wmat_row(player, 1);
    const glm::vec3 normal = wall.normal;
    const glm::vec3 want   = heading;        // the stick, or the run's own direction while it's centred

    // Into the wall within climb_angle: climb. Otherwise sideways, the way `want` leans along the wall.
    const bool vertical = glm::dot(want, -normal) > std::cos(glm::radians(climb_angle));
    glm::vec3 along = glm::normalize(glm::cross(normal, up));   // parallel to the wall, level with the ground
    if (glm::dot(along, want) < 0.0f) along = -along;
    if (vertical) {
        wall.run_dir = glm::normalize(up - normal * glm::dot(up, normal));   // up, projected onto the wall plane
        wall.yaw     = yaw_of(-normal);                                       // face into the wall
    } else {
        wall.run_dir = along;
        wall.yaw     = yaw_of(along);
    }
    wall.vertical = vertical;
    wall.on_left  = glm::dot(normal, right) > 0.0f;             // normal points at us from the left
}

// ---------------------------------------------------------------------------------------------------------------
// The move. It is the body of lower-body action 0x18 (the hook), so the phase is the game's mBdyAction[1].phase:
// 0 = just entered, PHASE_RUN = running, 1-4 = handed over to wall_jump_port (the game's own wall jump).
// ---------------------------------------------------------------------------------------------------------------
namespace {

constexpr int PHASE_RUN = 100;      // out of the port's 0-4

// the upper body during the run (see "the gun during the run" below)
constexpr uintptr_t kBdyUpperNo = 0x14F8;   // mBdyAction[0].no: 12 = attack (the shot), 13 / 16 = the gun pose
inline bool upper_busy(uintptr_t pl) { const int u = at<int>(pl, kBdyUpperNo); return u == 12 || u == 13 || u == 16; }
inline bool running(uintptr_t pl) {
    return at<int>(pl, pl::mBdyActionLowerNo) == ACT_WALL_JUMP && at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_RUN;
}
inline float wrap_pi(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }

struct RunState {                   // what the game's phase word can't hold
    uintptr_t player = 0;
    float timer = 0.0f;             // frames left when run_frames caps the run
    int motion = -1;                // the motion playing, to re-request when the direction mode changes
    int kickoff = -1;               // custom jump-out motion for the port's kick-off (-1 = the vanilla one)
    bool custom_kickoff = false;    // a jump-out cycle is playing: it is the whole jump, the port adds no fall motion
    bool from_run = false;          // the port's next phase 0 follows a sideways run (set by hand_to_wall_jump, read once)
    float lost = 0.0f;              // frames since the casts last hit (the wall ended, or bent too sharply)
    uint32_t run_limit = 0, run_stat = 0;   // mActLimit / mActStat as the wall jump's entry left them: re-asserted while the upper body shoots
    float shot_age = -1.0f;         // ticks since the shot action started (-1 = none)
    float gun_hold = -1.0f;         // frames left with the gun up after a shot ended (-1 = none): the arms keep the shot's last frame
    uint32_t motion_attr = 0;      // the attribute the run cycle was requested with (back_to_run copies it)
    float lean_w = 0.0f;            // 0-1: how much of shoot_lean_deg applies (ramped by post_action_tick, read by spine_provider)
    bool  gun_buffered = false;     // a shot press waiting for the current shot's cancel window (the .ctn buffer window, mStat 1)
    Wall wall{};
    uint32_t last_seen = 0;         // state_for's tick when this player was last seen (eviction order)
};

RunState g_runs[4];                 // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)

// The hooks run for every uPlayer that ticks, and dead players are never reported, so a full table evicts the slot
// seen longest ago (a live player is seen every tick and keeps its slot). Resetting slot 0 instead let two live
// players evict each other every tick once stale pointers filled the table (Gerb's hop-in-place bug, 2026-10-21).
RunState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (RunState& s : g_runs) if (s.player == player) { s.last_seen = tick; return s; }
    RunState* victim = &g_runs[0];
    for (RunState& s : g_runs) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = RunState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

// Backstop against clipping through the wall (root motion, push collision off for a frame, ...): keep the
// position at least wall_gap from the wall plane, on the side the normal points to. Runs before moveMotion
// integrates the velocity, so it corrects last tick's result.
void keep_off_wall(uintptr_t pl, const Wall& wall) {
    if (WallRun::wall_gap <= 0.0f) return;
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    const glm::vec3 n = wall.normal;
    const float d = glm::dot(glm::vec3(pos) - glm::vec3(wall.hit_pos), n);   // signed distance to the plane
    if (d < WallRun::wall_gap) {
        const glm::vec3 fixed = glm::vec3(pos) + n * (WallRun::wall_gap - d);
        pos = fixed;
    }
}

// Drop any additive speed (mAddSpd while mIsAddSpeed): the run and the jump-out set the velocity themselves,
// and a carried momentum would push the capsule through the wall.
void clear_add_speed(uintptr_t pl) {
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

void face(uintptr_t pl, float yaw) {                 // snap, the way the wall jump does
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

// .ctn borrow (D15): make the next motion request read the cancel script of `ctn_bank:ctn_slot` for both body
// halves, as uPlayerNero::setShortGrabMotionSet does for the Buster motions. updateMotion consumes and clears it.
void borrow_ctn(uintptr_t pl, uint32_t ctn_bank, uint32_t ctn_slot, bool lower_only = false) {
    for (int half = lower_only ? 1 : 0; half < 2; ++half) {
        uintptr_t p = pl + pl::mMotionSetCharTblParam + 12 * half;
        at<bool>(p, 0)     = true;
        at<uint32_t>(p, 4) = ctn_bank;
        at<uint32_t>(p, 8) = ctn_slot;
    }
}

void play_run_motion(RunState& s, uintptr_t pl) {
    const int motion = WallRun::motion_for(pl, s.wall);
    if (motion == s.motion) return;
    const bool custom = WallRun::custom_ready(pl);
    // With the gun up (a direction change mid-shot), the legs only: the whole-body request clears the upper and aim
    // controllers, which cut the shot's motion, so its end (phase 4 at mMotionFrameEnd[1]) never came and no shot
    // could start again.
    const bool lower_only = upper_busy(pl) || s.gun_hold >= 0.0f;   // (held after a shot: the arms stay up too)
    if (custom)                                 // a runtime bank has no .ctn scripts: borrow the placeholder's
        borrow_ctn(pl, 0, WallRun::placeholder_motion(s.wall), lower_only);
    // The custom cycles loop, with their root translation ignored (A_NULL_TRANS_OFF: the run's velocity moves the
    // player, the LMT's root track would walk the capsule into the wall); the placeholder kick-offs use the wall
    // jump's 5 (A_NULL_TRANS_OFF | A_LOOP_OFF).
    const uint32_t attr = custom ? (uint32_t)A_NULL_TRANS_OFF : ATTR_KICK;
    if (lower_only) devil4_sdk::request_motion_body((void*)pl, MC_LOWER, motion, attr, 6.0f, 0.0f, 1.0f);
    else            request_motion(pl, motion, attr, 6.0f, 0.0f, 1.0f);
    s.motion = motion;
    s.motion_attr = attr;
}

// Entering action 0x18 cost a wall jump and counted an air jump (checkWallJumpBegin -> initJumpBegin(3):
// --mWallJumpCnt, ++mWJumpCnt). Give them back when the run ends so the player can keep jumping.
void give_jumps_back(uintptr_t pl) {
    char& wall_left = at<char>(pl, pl::mWallJumpCnt);
    char& air_used  = at<char>(pl, pl::mWJumpCnt);
    switch (WallRun::jumps_after_run) {
    case 1:                                                 // undo the entry's cost
        ++wall_left;
        if (air_used > 0) --air_used;
        break;
    case 2: {                                               // as if freshly airborne
        const uint8_t* tbl = at<uint8_t*>(pl, pl::mpActDataTbl);
        if (tbl) wall_left = (char)tbl[1];
        air_used = 0;
        break;
    }
    default: break;                                         // 0: the game's own accounting
    }
}

// Hand the action over to the game's wall jump: it reads the wall from mGroundPoly and runs its phases 0-4 in
// the same phase word from here on (checkWallJumpBegin already set mJump.mode and the jump counters on entry).
// ---- the gun during the run (RE/player_move_pipeline.md "Upper-body gun shots") ----------------------------------
// The game's own gun rows would take the whole body in the air (the air shots), so while the run is active the
// PRE-CHECK tick (before checkAction) matches the character's GROUND plain-shot row itself and starts it on the UPPER
// half only (setActionAtck(id, 0), the way setActionAtckGun starts Nero's Blue Rose while walking), then eats the
// press, the gun reserves and the charge-release command bits so checkAtck sees nothing gun-shaped (charge shots are
// consumed, not fired). Melee rows are untouched. It applies checkAtck's own row tests (unlocked bit, the row's extra
// condition through checkAtckEtcSub, and mid-shot the cancel windows the shot's .ctn opens), so mashing chains shots
// as on the ground, Dante's rapid follow-up rows included; Pandora is left out. The shot then runs exactly as the
// game's walking shot: its upper motion (the pose table's column 3 of the front walking row, setGunShotMotion[_2] on
// controller 1) and the elevation layer (the column by elevation on controller 2), upper-body blocks over the running
// legs; the Blue Rose's own recoil (setShot: motion 1 of the gun's LMT) and the Coyote-A's cartridges come from the shot
// functions on their own frames. When it ends, the arms go back to the run cycle (back_to_run), not to the gun pose,
// and a torso lean away from the wall comes through SpineAim while the gun is up. What keeps it on the wall:
//  - setActionUpperBdy writes the whole-player words (mActionNo = 12: checkAction switches on it, and case 12 runs
//    checkWallJumpBegin and the air hike; mActLimit from action 12's row; the lock-on turn: setLockOnTargetAngle
//    0x820CE0 writes mRotate.mTarget AND mNow): post_action_tick (after both halves' state functions, before anything
//    reads the words) puts the run's own back every tick;
//  - mPose.dir is pinned to 1 (front) while running: the shot setters pick their row by it, and dir 0 (stick centred)
//    is the full-body standing row, whose leg tracks froze the run cycle;
//  - setActionAtckGun (the Automatic mode's auto-repeat) and the shot functions' last phase call setActionPoseLowerBdy
//    (lower -> the pose): hooked at 0x7AA8C0 to return at once while the run is active;
//  - the shot functions' hand-off to the gun pose, setActionPoseUpperBdy 0x7AA860, is hooked: the upper action ends and,
//    after gun_hold_frames with the arms on the shot's last frame, the run cycle goes on the upper / aim controllers at
//    the legs' frame;
//  - a run-cycle change while the gun is up (a direction change) is requested on the lower body only: the whole-body
//    request would clear the shot's upper motion, and the shot would never end;
//  - a lower-body motion request that isn't the run's (the charge shot's own) is dropped, with its A_STOP;
//  - upper / aim requests get at least upper_blend_min frames (the game cuts in with 0 once the gun counts as held);
//  - the shot action ends on its own (phase 4 at its upper motion's end); shot_end_ticks and the stuck-shot check are
//    safety nets.
constexpr uintptr_t kActionNo       = 0x14F0;   // the whole-player action number (checkAction's switch, setActionStatus)
constexpr uintptr_t kPoseDir        = 0x16C4;   // mPose.dir: 1 front, 2 / 8 sides, 6 / 12 back diagonals, 0 none (the standing row)
constexpr uintptr_t kBdyUpperPhase  = 0x14FC;
constexpr uintptr_t kWpGripId       = 0x1DBC;   // mWeaponCtrl.mGripId
constexpr uintptr_t kWpEnableLongId = 0x1DC4;   // mWeaponCtrl.mEnableLongId (Blue Rose = 1, the rows' atckConditionWp)
constexpr uintptr_t kIsWpChangeCancel = 0x20C9; // mIsWpChangeCancel
constexpr uintptr_t kCmdFlag        = 0x2184;   // uint32[3]: bit = CMD_ID, set by executeCommandCheck this tick
constexpr uintptr_t kMotCncl        = 0x30C0, kMotCnclSize = 0x2C, kMotCnclReserve = 0x08;   // mMotCncl[k].mIsReserve
constexpr uintptr_t kMotionCtrl     = 0x2894, kMotionCtrlSize = 0x3C;                       // cMotionCtrl[7]
constexpr uintptr_t kMcMotionSetReq = 0x04, kMcMotionNo = 0x14, kMcInterp = 0x1C, kMcAddAttribute = 0x2A;   // byte, uint, float, s16
constexpr int16_t   A_STOP_BIT      = 0x10;
constexpr uintptr_t kModelMotion0Frame = 0x330 + 0x18;   // uModel::MotionArray[0].mFrame (0xE0 per slot, slot = controller type)
constexpr uintptr_t kModelMotionNo = 0x330 + 0x04, kModelMotionSize = 0xE0;   // MotionArray[i].mMotionNo (u16)
static int  g_shots = 0;                        // readout
static char g_last[96] = "";
uintptr_t jmp_ret_pose = 0;                     // 0x7AA8C8

inline uintptr_t gun_table(uintptr_t pl) { return at<int>(pl, pl::mPlayerID) == 1 ? HookNeroKADTbl : HookDanteKADTbl; }
inline void clear_cmd_bit(uintptr_t pl, uint32_t bit) { at<uint32_t>(pl, kCmdFlag + 4 * (bit >> 5)) &= ~(1u << (bit & 31)); }

void set_action_atck(uintptr_t pl, uint32_t id, int body) {
    _asm {
        pushad
        mov eax, pl
        mov ecx, id
        push body
        call fn_setActionAtck
        popad
    }
}

bool set_pose_weapon(uintptr_t pl) {
    uint8_t r = 0;
    _asm {
        pushad
        mov edi, pl
        call fn_setPoseWeapon
        mov byte ptr [r], al
        popad
    }
    return r != 0;
}

// The upper-body shot, the way setActionAtckGun starts it, minus the lower-body pose.
void start_upper_shot(uintptr_t pl, uint32_t id) {
    set_pose_weapon(pl);                                 // mPose.wpnId = the weapon when it can pose (Blue Rose, E&I); the Coyote-A can't and shoots anyway
    if (!at<bool>(pl, kIsWpChangeCancel)) at<uint32_t>(pl, kWpGripId) = at<uint32_t>(pl, kWpEnableLongId);
    set_action_atck(pl, id, 0);
    ++g_shots;
    snprintf(g_last, sizeof g_last, "shot id %u", id);
}

// checkAtck's own row tests, for the rows the pre-check tick matches itself (RE player_move_pipeline.md "Rapid shots"):
// the unlocked-moves bitset (vtbl 120 checkAtckBit(this, buf) -> a struct with the bits at +0x18), the row's extra
// condition (vtbl 108 checkAtckEtcSub(this, etc): Dante 10 / 11 = the E&I / Coyote-A follow-up window, the rapid timer
// at +0x14DA4 still running; 14 = Pandora free) and its cancel rule against the .ctn windows (checkAtckCancel 0x7AC4A0).
bool atck_unlocked(uintptr_t pl, uint32_t id) {
    static uint8_t buf[64];
    uint8_t* b = buf;
    const uintptr_t fn = (*(uintptr_t**)pl)[120];
    uintptr_t r = 0, saved = 0;
    _asm {
        pushad
        mov saved, esp              // whoever cleans the argument, esp comes back before popad (locals are ebp-relative)
        push b
        mov ecx, pl
        call fn
        mov r, eax
        mov esp, saved
        popad
    }
    return r && (at<uint32_t>(r, 0x18 + 4 * (id >> 5)) & (1u << (id & 31))) != 0;
}

bool atck_etc_ok(uintptr_t pl, uint32_t etc) {
    if (!etc) return true;
    const uintptr_t fn = (*(uintptr_t**)pl)[108];
    uint8_t r = 0;
    uintptr_t saved = 0;
    _asm {
        pushad
        mov saved, esp
        push etc
        mov ecx, pl
        call fn
        mov byte ptr [r], al
        mov esp, saved
        popad
    }
    return r == 1;
}

enum CancelVerdict { CANCEL_SHUT, CANCEL_BUFFER, CANCEL_OPEN };
// checkAtckCancel's rule: each cmdNo entry (bit 0) needs mMotCncl[val].mStat 1 or 2; every one without cmdOr (bit 2)
// must pass, one passing cmdOr entry is enough on its own; cmdId entries (bit 1) need one equal to mAtckId. mStat 1 (the
// .ctn high nibble 0x30) only buffers the input, 2 (0x10) starts the move.
CancelVerdict atck_cancel(uintptr_t pl, const kAtckDefTbl* row) {
    bool or_open = false, or_buffer = false, and_ok = true, and_any = false, and_buffer = false, id_any = false, id_hit = false;
    for (uint32_t e : row->cancelId) {
        if (!e) continue;
        const int val = (int8_t)(e >> 24);
        if (e & 1) {
            const int st = (val >= 0 && val < CANCEL_NO_NUM) ? at<int>(pl, kMotCncl + kMotCnclSize * val + 4) : 0;
            const bool ok = st == 1 || st == 2;
            if (e & 4) { if (st == 2) or_open = true; else if (st == 1) or_buffer = true; }
            else { and_any = true; if (!ok) and_ok = false; else if (st == 1) and_buffer = true; }
        } else if (e & 2) {
            id_any = true;
            if (val == at<int>(pl, pl::mAtckId)) id_hit = true;
        }
    }
    if (or_open) return CANCEL_OPEN;
    if (and_any && and_ok && (!id_any || id_hit)) return and_buffer ? CANCEL_BUFFER : CANCEL_OPEN;
    return or_buffer ? CANCEL_BUFFER : CANCEL_SHUT;
}

// Pre-check tick: the character's ground plain-shot row, matched here and started upper-only; checkAtck then sees no
// press, no gun reserve and no charge command.
void precheck_tick(uintptr_t pl) {
    if (!WallRun::mod_enabled || !WallRun::gun_while_running || !running(pl)) return;
    // Pandora (Dante's long weapon 8) has no place on the wall (and its row started upper-only, after a switch mid-shot,
    // crashed the game): no shot, nothing touched, only the press eaten so checkAtck doesn't start its air row
    if (at<int>(pl, pl::mPlayerID) != 1 && at<uint32_t>(pl, kWpEnableLongId) == 8) {
        at<uint32_t>(pl, pl::mBtnTrg) &= ~0x2u;
        at<bool>(pl, kMotCncl + kMotCnclSize * C_NORM0 + kMotCnclReserve) = false;
        at<bool>(pl, kMotCncl + kMotCnclSize * C_GUN   + kMotCnclReserve) = false;
        state_for(pl).gun_buffered = false;
        return;
    }
    // The pose table's STANDING row (mPose.dir 0, what setPoseDir gives with the stick centred: blocks 0502-0505) is
    // full-body on both characters, legs and root included; on the upper controller it froze the legs over the run
    // cycle. The walking rows are upper-body blocks. Pin the direction to front (1) every running tick, before a shot
    // starts: the shot setters pick their row from it (a footwork leaves 0 behind).
    at<int>(pl, kPoseDir) = 1;
    const kAtckDefTbl* row = (const kAtckDefTbl*)gun_table(pl);
    if (!row) return;
    const uint32_t wp = at<uint32_t>(pl, kWpEnableLongId);
    RunState& s = state_for(pl);
    if (at<uint32_t>(pl, pl::mBtnTrg) & 0x2) s.gun_buffered = true;    // a press waits for the shot's cancel window (the
    const bool mid_shot = at<int>(pl, kBdyUpperNo) == 12;              // game's buffer: mStat 1 holds it, 2 fires it)
    if (!mid_shot && !(at<uint32_t>(pl, pl::mBtnTrg) & 0x2)) s.gun_buffered = false;   // between shots only a fresh press fires
    bool started = false;
    for (; row->atckAttr != 3; ++row) {
        if (row->atckAttr != 1 || row->atckAs != 1) continue;             // a gun row, its ground version
        if (row->command.atckCommandNo) continue;                          // disabled row (MoveTable's convention)
        if (row->atckConditionWp != 0xFFFFFFFFu && row->atckConditionWp != wp) continue;
        const uint32_t cmd = row->command.atckCommand;
        if (cmd) { clear_cmd_bit(pl, cmd); continue; }                     // a charge release: consumed, never fired on the wall
        if (started || !s.gun_buffered) continue;
        // checkAtck's order: unlocked, the extra condition (the E&I / Coyote-A follow-up rows 61-63 / 72-74 come first
        // and pass only inside their rapid window), then mid-shot the string rules: AI_REPEAT / AI_CANCEL (atckLevel
        // here is the RE note's atckInfo: 7 on every gun row) and the cancel windows the shot's .ctn opens
        if (!atck_unlocked(pl, row->atckId) || !atck_etc_ok(pl, row->ukn)) continue;
        if (mid_shot) {
            if (!(row->atckLevel & 2)) continue;                                                // AI_CANCEL
            if ((int)row->atckId == at<int>(pl, pl::mAtckId) && !(row->atckLevel & 4)) continue;  // AI_REPEAT
            const CancelVerdict v = atck_cancel(pl, row);
            if (v == CANCEL_SHUT) { s.gun_buffered = false; break; }      // as checkAtck: a press outside the windows is lost
            if (v == CANCEL_BUFFER) break;                                 // held until the window opens (mStat 2)
        }
        start_upper_shot(pl, row->atckId);
        started = true;
        s.gun_buffered = false;
        s.shot_age = 0.0f;
        s.gun_hold = -1.0f;                                               // the next shot comes in from the held gun
    }
    at<uint32_t>(pl, pl::mBtnTrg) &= ~0x2u;
    at<bool>(pl, kMotCncl + kMotCnclSize * C_NORM0 + kMotCnclReserve) = false;
    at<bool>(pl, kMotCncl + kMotCnclSize * C_GUN   + kMotCnclReserve) = false;
}

// The run ends (kick-off, landing): the upper half must not stay in the shot / pose, or moveActPoseU keeps asking for
// the gun stance over the next whole-body motion; and the words go back to the run's.
void end_upper_shot(RunState& s, uintptr_t pl) {
    s.shot_age = -1.0f;
    if (!upper_busy(pl)) return;
    at<int>(pl, kBdyUpperNo)    = at<int>(pl, pl::mBdyActionLowerNo);
    at<int>(pl, kBdyUpperPhase) = 0;
    at<int>(pl, kActionNo)      = at<int>(pl, pl::mBdyActionLowerNo);
    at<uint32_t>(pl, pl::mActLimit) = s.run_limit;
    at<uint32_t>(pl, pl::mActStat)  = s.run_stat;
}

// The shot is over (its last phase, through the setActionPoseUpperBdy hook below, or a safety net): the upper action
// ends and the run cycle itself goes on the upper and aim controllers, at the legs' frame. A slot given a new motion
// crossfades from a snapshot of the pose on screen (uModel::moveMotion copies the joints into the slot when mState & 2),
// so the arms blend back into the run over run_blend_frames, and the next shot blends in from there (upper_blend_min).
// The same motion at the same frame as the legs looks exactly like the run; the next whole-body run request (a
// direction change, the kick-off) clears both controllers.
void back_to_run(RunState& s, uintptr_t pl) {
    end_upper_shot(s, pl);
    s.gun_hold = -1.0f;
    if (s.motion < 0) return;
    const float frame = at<float>(pl, kModelMotion0Frame);
    const float interp = WallRun::run_blend_frames > 1.0f ? WallRun::run_blend_frames : 1.0f;
    for (int body : { MC_UPPER, MC_AIM })   // mirrored to the coat's 5 / 6 by requestSetMotion
        devil4_sdk::request_motion_body((void*)pl, body, s.motion, s.motion_attr, interp, frame, 1.0f);
    if (spine_aim::inertialize(pl, interp))  // the body's controllers cut to the run, the fade takes the frames (the coat keeps the crossfade)
        for (int body : { MC_UPPER, MC_AIM }) at<float>(pl + kMotionCtrl + kMotionCtrlSize * body, kMcInterp) = 0.0f;
}

void post_action_tick(uintptr_t pl) {
    if (!WallRun::mod_enabled || !WallRun::gun_while_running || !running(pl)) return;
    RunState& s = state_for(pl);
    if (upper_busy(pl)) {
        at<int>(pl, kActionNo) = ACT_WALL_JUMP;             // checkAction: the wall jump's case, not 12 / 13 / 16
        at<uint32_t>(pl, pl::mActLimit) = s.run_limit;
        at<uint32_t>(pl, pl::mActStat)  = s.run_stat;
        face(pl, s.wall.yaw);                               // the shot's setLockOnTargetAngle turned the facing toward the target
        // the shot's upper and aim requests stand, but with at least upper_blend_min frames of interpolation: the shot
        // cuts in with the draw timer as its blend, 0 once the gun counts as held, a snap from the running arms. With
        // SpineAim's inertial blends, a new motion on the body's controllers cuts in (0) and the fade takes the frames
        // instead; the coat's (5 / 6) keep the crossfade (its tails hang on the body and swing anyway)
        if (WallRun::upper_blend_min > 0.0f) {
            float frames = 0.0f;
            for (int i : { 1, 2, 5, 6 }) {
                const uintptr_t c = pl + kMotionCtrl + kMotionCtrlSize * i;
                if (!at<bool>(c, kMcMotionSetReq)) continue;
                if (at<float>(c, kMcInterp) < WallRun::upper_blend_min) at<float>(c, kMcInterp) = WallRun::upper_blend_min;
                if ((i == MC_UPPER || i == MC_AIM) && at<uint32_t>(c, kMcMotionNo) != at<uint16_t>(pl, kModelMotionNo + kModelMotionSize * i))
                    frames = std::max(frames, at<float>(c, kMcInterp));   // a new motion, not the same one asked again
            }
            if (frames > 0.0f && spine_aim::inertialize(pl, frames))
                for (int i : { MC_UPPER, MC_AIM }) {
                    const uintptr_t c = pl + kMotionCtrl + kMotionCtrlSize * i;
                    if (at<bool>(c, kMcMotionSetReq)) at<float>(c, kMcInterp) = 0.0f;
                }
        }
        // a foreign lower request (the charge shot's own) and its A_STOP go
        const uintptr_t c0 = pl + kMotionCtrl;
        if (at<bool>(c0, kMcMotionSetReq) && (int)at<uint32_t>(c0, kMcMotionNo) != s.motion)
            for (int i : { 0, 3, 4 }) at<bool>(pl + kMotionCtrl + kMotionCtrlSize * i, kMcMotionSetReq) = false;   // lower, face, coat lower
        for (int i : { 0, 3, 4 }) at<int16_t>(pl + kMotionCtrl + kMotionCtrlSize * i, kMcAddAttribute) &= ~A_STOP_BIT;
        // safety: a shot action that never reaches its end (phase 4 needs its upper motion's end) is ended here
        if (WallRun::shot_end_ticks > 0.0f && at<int>(pl, kBdyUpperNo) == 12) {
            if (s.shot_age >= 0.0f) s.shot_age += at<float>(pl, pl::m_delta_time);
            if (s.shot_age >= WallRun::shot_end_ticks) back_to_run(s, pl);
        }
        // a shot whose upper motion is gone (cleared by a whole-body request: nothing on the run does that any more, but a
        // stuck shot blocks every later press) never reaches its end: end it here
        const uintptr_t c1 = pl + kMotionCtrl + kMotionCtrlSize * MC_UPPER;
        if (at<int>(pl, kBdyUpperNo) == 12 && at<uint32_t>(c1, kMcMotionNo) == 0xFFFF && !at<bool>(c1, kMcMotionSetReq))
            back_to_run(s, pl);
    } else if (s.gun_hold >= 0.0f) {                       // the gun held after a shot: back to the run when it runs out
        s.gun_hold -= at<float>(pl, pl::m_delta_time);
        if (s.gun_hold <= 0.0f) back_to_run(s, pl);
    }
    // the lean comes in and goes out over lean_blend_frames instead of popping with the upper action
    {
        const float target = (upper_busy(pl) || s.gun_hold >= 0.0f) ? 1.0f : 0.0f;
        const float step = WallRun::lean_blend_frames > 0.0f ? at<float>(pl, pl::m_delta_time) / WallRun::lean_blend_frames : 1.0f;
        s.lean_w = target > s.lean_w ? std::min(target, s.lean_w + step) : std::max(target, s.lean_w - step);
    }
}

// uPlayer::setActionPoseLowerBdy 0x7AA8C0 (this@esi, no stack arguments): the lower body into the gun pose. While
// the run is active it returns at once. The 8 bytes replaced are `push ecx; cmp dword ptr [esi+141Ch], 0`.
bool __stdcall pose_lower_blocked(uintptr_t pl) { return WallRun::mod_enabled && WallRun::gun_while_running && running(pl); }
naked void detour_pose_lower() {
    _asm {
        pushad
        push esi
        call pose_lower_blocked     // __stdcall: pops its argument; al = blocked
        test al, al
        popad                       // registers back, flags kept
        jnz blocked
        push ecx
        cmp dword ptr [esi + 0x141C], 0
        jmp dword ptr [jmp_ret_pose]
    blocked:
        xor eax, eax
        ret
    }
}

// uPlayer::setActionPoseUpperBdy 0x7AA860 (this@esi; called only by the shots' last phases: the E&I, Coyote-A, Blue Rose
// and Blue Rose charge shots): the upper body into the gun pose. While the run is active it goes back to the run cycle
// instead (back_to_run), after gun_hold_frames with the gun up: the shot action ends at once (the next press fires as
// between shots), but nothing is requested on the upper / aim controllers, so the arms keep the shot's last frame and a
// burst doesn't dip into the run between shots. The same 8 bytes as setActionPoseLowerBdy's are replaced.
bool __stdcall pose_upper_redirect(uintptr_t pl) {
    if (!pose_lower_blocked(pl)) return false;  // the same condition: the run is active
    RunState& s = state_for(pl);
    if (WallRun::gun_hold_frames > 0.0f) { end_upper_shot(s, pl); s.gun_hold = WallRun::gun_hold_frames; }
    else back_to_run(s, pl);
    return true;
}
uintptr_t jmp_ret_pose_upper = 0;               // 0x7AA868
naked void detour_pose_upper() {
    _asm {
        pushad
        push esi
        call pose_upper_redirect    // __stdcall: pops its argument; al = handled
        test al, al
        popad
        jnz handled
        push ecx
        cmp dword ptr [esi + 0x141C], 0
        jmp dword ptr [jmp_ret_pose_upper]
    handled:
        xor eax, eax
        ret
    }
}

void hand_to_wall_jump(RunState& s, uintptr_t pl, bool from_run) {
    s.kickoff = -1;
    s.custom_kickoff = false;
    s.from_run = from_run && !s.wall.vertical;               // a climb faces the wall: the port's own choice is clear
    if (from_run) {
        at<LineResult>(pl, pl::mGroundPoly) = s.wall.poly;   // else keep checkWallJumpBegin's wall
        give_jumps_back(pl);                                 // the kick-off itself doesn't charge again
        if (WallRun::custom_ready(pl) && !s.wall.vertical)   // the jump-out cycles (none for a climb yet)
            s.kickoff = (WallRun::lmt_bank << 8) |
                        ((s.wall.on_left ? WallRun::run_slot_jump_left : WallRun::run_slot_jump_right) & 0xFF);
    }
    at<int>(pl, pl::mBdyActionLowerPhase) = 0;
    end_upper_shot(s, pl);
    s.motion = -1;
    WallRun::wall_jump_port(pl);
}

} // namespace

void __stdcall WallRun::move(uintptr_t pl) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    if (phase != 0 && phase != PHASE_RUN) { wall_jump_port(pl); return; }   // handed over earlier

    RunState& s = state_for(pl);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);

    if (phase == 0) {                                   // entry: this tick's jump press near a wall
        s.motion = -1;
        s.timer  = run_frames;
        s.lost   = 0.0f;
        s.custom_kickoff = false;
        s.run_limit = at<uint32_t>(pl, pl::mActLimit);  // the wall jump's row (jump | atck | guard), as its entry set them
        s.run_stat  = at<uint32_t>(pl, pl::mActStat);
        s.shot_age = -1.0f;
        s.gun_hold = -1.0f;
        s.lean_w = 0.0f;
        s.gun_buffered = false;
        const glm::vec3 h = heading(pl, nullptr);
        if (!detect(pl, s.wall, h)) { hand_to_wall_jump(s, pl, false); return; }   // nothing runnable that way: plain wall jump
        steer(pl, s.wall, h);
        face(pl, s.wall.yaw);
        acc.x = acc.y = acc.z = 0.0f;
        vel.x = vel.y = vel.z = 0.0f;
        clear_add_speed(pl);
        play_run_motion(s, pl);
        phase = PHASE_RUN;
        return;                                         // the jump button is down on this tick by definition
    }

    // running
    if (!(at<uint32_t>(pl, pl::mBtnOn) & 0x4)) { hand_to_wall_jump(s, pl, true); return; }   // jump released: kick off
    // The wall ended, or bent more than corner_dot allows: keep running on the last wall for lose_grace frames (a
    // seam, a one-tick miss), then end the way a release does, with the kick-off off that wall. Never an abrupt
    // fall mid-cycle.
    const glm::vec3 h = heading(pl, &s.wall);
    if (!detect(pl, s.wall, h)) {                       // detect leaves s.wall as it was on a miss
        s.lost += at<float>(pl, pl::m_delta_time);
        if (s.lost > lose_grace) { hand_to_wall_jump(s, pl, true); return; }
    } else {
        s.lost = 0.0f;
    }
    steer(pl, s.wall, h);
    play_run_motion(s, pl);                             // only when left / right / up changed; the override rides on top
    face(pl, s.wall.yaw);

    const float speed = s.wall.vertical ? climb_speed : run_speed;
    const glm::vec3 v = glm::vec3(s.wall.run_dir) * speed - glm::vec3(s.wall.normal) * stick_speed;
    vel.x = v.x; vel.z = v.z;
    vel.y = s.wall.vertical ? v.y : 0.0f;               // climb: up the wall; sideways: level
    acc.x = acc.y = acc.z = 0.0f;
    clear_add_speed(pl);
    keep_off_wall(pl, s.wall);

    if (run_frames > 0.0f) {
        s.timer -= at<float>(pl, pl::m_delta_time);
        if (s.timer <= 0.0f) { hand_to_wall_jump(s, pl, true); return; }
    }

    uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
    if (coll && at<bool>(coll, 0x1C)) { end_upper_shot(s, pl); set_action_land(pl); }   // mLand: back on the ground
}

// ---------------------------------------------------------------------------------------------------------------
// Custom LMT: a free motion bank filled at runtime (MoveDef D14), body and coat. Without the file the run keeps
// the placeholder kick-offs, so the mod works for people who don't have the animation.
// ---------------------------------------------------------------------------------------------------------------
static bool lmt_file_exists(const char* game_path) {        // the loose-file root, as Payline checks its LMT
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

int WallRun::placeholder_motion(const Wall& wall) {
    if (wall.vertical) return motion_up;
    return wall.on_left ? motion_left : motion_right;
}

bool WallRun::custom_ready(uintptr_t player) {
    return use_custom_lmt && player && devil4_sdk::get_motion_bank((void*)player, lmt_bank) != 0;
}

int WallRun::motion_for(uintptr_t player, const Wall& wall) {
    if (!custom_ready(player)) return placeholder_motion(wall);   // no file for this player: the stand-ins
    const int slot = wall.vertical ? run_slot_up : wall.on_left ? run_slot_left : run_slot_right;
    return (lmt_bank << 8) | (slot & 0xFF);
}

// SpineAim asks this during its pass: the torso leans away from the wall while the gun is up (the walking shot has none
// of the run cycle's lean), faded by lean_w. The axis turns "up" toward the wall, so a negative angle leans away.
bool spine_provider(uintptr_t pl, spine_aim::Request& out) {
    if (!WallRun::mod_enabled || !WallRun::gun_while_running || WallRun::shoot_lean_deg == 0.0f || !running(pl)) return false;
    RunState& s = state_for(pl);
    if (s.wall.vertical || s.lean_w <= 0.0f) return false;
    const glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -glm::vec3(s.wall.normal));
    if (glm::length(axis) <= 1e-3f) return false;
    out.lean_axis = glm::normalize(axis);
    out.lean_deg = WallRun::shoot_lean_deg * s.lean_w;
    return true;
}

void WallRun::ensure_bank(uintptr_t player) {
    if (!use_custom_lmt || !player || lmt_bank < 10 || lmt_bank > 14) return;   // 0-9 are setup banks, 15 Buster
    static uintptr_t s_failed_for = 0;       // don't retry (and spam the log) every frame on a missing file
    if (s_failed_for == player) return;

    void* body = (void*)player;
    if (!devil4_sdk::get_motion_bank(body, lmt_bank)) {
        if (!lmt_file_exists(lmt_body)) {                    // no file: the placeholders stay in use, say so once
            spdlog::warn("WallRun: nativePC\\{}.lmt not found, using the placeholder motions", lmt_body);
            s_failed_for = player;
            return;
        }
        if (!devil4_sdk::load_motion_bank(body, lmt_body, lmt_bank)) { s_failed_for = player; return; }
        spdlog::info("WallRun: '{}' -> bank {} of the body", lmt_body, lmt_bank);
    }
    if (!load_coat_lmt) return;                              // coat (and face) motions stay off for now
    void* coat = (void*)at<uintptr_t>(player, pl::mpCoatMdl);   // requestSetMotion mirrors motion numbers to it
    if (coat && !devil4_sdk::get_motion_bank(coat, lmt_bank)) {
        const char* path = at<uint32_t>(player, pl::charType) == 1 ? lmt_coat_nero : lmt_coat_dante;
        if (!lmt_file_exists(path)) {                        // body runs the custom cycles, the coat holds its pose
            spdlog::warn("WallRun: nativePC\\{}.lmt not found, the coat won't animate on the run", path);
            s_failed_for = player;
            return;
        }
        if (!devil4_sdk::load_motion_bank(coat, path, lmt_bank)) { s_failed_for = player; return; }
        spdlog::info("WallRun: '{}' -> bank {} of the coat", path, lmt_bank);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// uPlayer::checkWallJumpBegin 0x804650 (SE 0xA059D0), readable: the entry test for action 0x18.
// ---------------------------------------------------------------------------------------------------------------
bool WallRun::wall_jump_check_port(uintptr_t pl) {
    if (!(at<uint32_t>(pl, pl::mActStat) & 0x2)) return false;             // only while airborne
    if (at<int>(pl, pl::mBdyActionLowerNo) == ACT_WALL_JUMP &&
        at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_RUN) return false;   // not from inside a run (a kick-off may chain)

    // A press of the chosen button(s): this tick, or, for the jump button, latched by the game as the C_JUMP
    // reserve (updateMotCancel keeps a press made while the window is open, which is how a press that started
    // an air hike still reaches the wall jump on a later tick). No mActLimit.jump condition.
    const bool pressed  = (at<uint32_t>(pl, pl::mBtnTrg) & button_mask) != 0;
    const bool latched  = (button_mask & 0x4) && at<int>(pl, pl::mMotCnclJumpStat) != 0 && at<bool>(pl, pl::mMotCnclJumpReserve);
    if (!pressed && !latched) return false;

    if (require_wall_jumps && at<char>(pl, pl::mWallJumpCnt) <= 0) return false;

    // A line through the player at hip height, check_reach back to check_reach forward, both sides, walls only.
    const glm::vec3 up  = wmat_row(pl, 1);
    const glm::vec3 fwd = wmat_row(pl, 2);
    const glm::vec3 hip = glm::vec3(at<MtVector3>(pl, pl::mPos)) + up * check_height;
    MtLineSegment segment;
    segment.p0 = hip - fwd * check_reach;
    segment.p1 = hip + fwd * check_reach;
    CollParam param = wall_param();
    LineResult& poly = at<LineResult>(pl, pl::mGroundPoly);              // the action reads the wall from here
    if (!find_intersection(&param, &segment, true, &poly)) return false;
    if (*(uint32_t*)(poly.pTriangle + 0x14) & 1) return false;           // surface_attr bit 0: no wall jump off it

    init_jump_begin(pl, JUMP_INIT_WALL_JUMP);
    at<int>(pl, pl::mJumpMode) = JUMP_MODE_NORM_WALL_LO;
    set_action(pl, ACT_WALL_JUMP);
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// Reference: uPlayer::moveActWallJump 0x827200 (SE 0xA2BCD0), readable. Phases in mBdyAction[1].phase.
// ---------------------------------------------------------------------------------------------------------------
void WallRun::wall_jump_port(uintptr_t pl) {
    int&   phase     = at<int>(pl, pl::mBdyActionLowerPhase);
    int&   jumpType  = at<int>(pl, pl::mJumpType);
    bool&  wallFront = at<bool>(pl, pl::isWallFront);
    MtVector3& vel   = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc   = at<MtVector3>(pl, pl::mAcc);
    const LineResult& poly = at<LineResult>(pl, pl::mGroundPoly);

    // The jump-out cycle's take-off: the jump's velocity, the dust, straight to the airborne phase.
    auto launch_jumpout = [&](RunState& rs) {
        if (at<uint32_t>(pl, pl::mBtnOn) & 0x4)
            at<int>(pl, pl::mJumpMode) = JUMP_MODE_NORM_WALL_HI;
        set_jump_rate(pl);                                  // the table's values (inertia, speed, gravity)
        if (jumpout_custom_physics) {                       // ... replaced by the tunables
            const glm::vec3 n = tri_normal(&poly);
            const glm::vec3 v = n * jumpout_out + glm::vec3(rs.wall.run_dir) * jumpout_along;
            vel.x = v.x; vel.z = v.z;
            vel.y = jumpout_up;
            acc.x = acc.y = acc.z = 0.0f;
            // keep the game's own XZ model in step: mMoveVel = the horizontal speed, no table deceleration,
            // moveAng = its yaw, and no additive speed on top
            const glm::vec3 xz(v.x, 0.0f, v.z);
            at<float>(pl, pl::mMoveVel) = glm::length(xz);
            at<float>(pl, pl::mMoveAcc) = 0.0f;
            if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = yaw_of(xz);
            clear_add_speed(pl);
        }
        wall_dust_effect(pl, tri_normal(&poly), poly.hitPos, efct_mtrl_flag(&poly));
        phase = 4;
    };

    switch (phase) {
    case 0: {
        at<float>(pl, pl::mDownMaxSpd) = at<float*>(pl, pl::mParamTbl0Data)[103];
        at<float>(pl, pl::mActTimer0)  = 10.0f;

        const glm::vec3 n = tri_normal(&poly);              // wall normal, points at the player
        const float wallYaw = yaw_of(n);

        // Stick direction against the wall normal picks the jump type.
        if (at<int>(pl, pl::mAnlgPow) != 0) {
            const float stick = at<float>(pl, pl::mAnlgRadian);
            const glm::vec3 s(std::sin(stick), 0.0f, std::cos(stick));
            const float dot = glm::dot(s, n);
            if (dot < -0.75f)      jumpType = JUMP_TYPE_F;              // pushing into the wall
            else if (dot > 0.75f)  jumpType = JUMP_TYPE_B;              // pulling away from it
            else                   jumpType = (s.x * n.z - s.z * n.x) <= 0.0f ? JUMP_TYPE_L : JUMP_TYPE_R;
            at<float>(pl, pl::mJumpMoveAng) = at<float>(pl, pl::mMoveAngle);
        } else {
            jumpType = JUMP_TYPE_V;                                     // no stick: straight up, facing-based
            at<float>(pl, pl::mJumpMoveAng) = at<float>(pl, pl::mRotateNow);
        }

        const float facing = at<float>(pl, pl::mRotateNow);
        const float facingDot = std::sin(facing) * n.x + std::cos(facing) * n.z;
        RunState& rs = state_for(pl);
        const bool from_run = rs.from_run;
        rs.from_run = false;

        acc.x = acc.y = acc.z = 0.0f;
        if (from_run) {                          // keep the run's speed along the wall until the launch (no dead stop),
            const float into = vel.x * n.x + vel.y * n.y + vel.z * n.z;   // only its push into the wall goes
            vel.x -= n.x * into; vel.y -= n.y * into; vel.z -= n.z * into;
        } else {
            vel.x = vel.y = vel.z = 0.0f;
        }
        at<int8_t>(pl, pl::mJumpAtckCnt) = 0;
        if ((at<uint32_t>(pl, pl::mActStat) & 2) && at<uint8_t>(pl, pl::mAtckLongCnt) > 2)
            at<uint8_t>(pl, pl::mAtckLongCnt) = 2;
        phase = 1;

        if (from_run) {
            // The run faces along the wall, so facingDot is about 0 and the test below was a coin flip between
            // two 90-degree snaps (which phase 1's turn then swung back: a twitch). Back to the wall, always; the
            // jump-out cycle is authored from the run's facing, so it keeps it; the placeholder kicks (back-to-wall
            // motions 25 / 26 / 27) still need the back to the wall.
            wallFront = false;
            if (rs.kickoff < 0) face(pl, wallYaw);
        } else if (facingDot >= 0.0f) {          // facing away from the wall
            wallFront = false;
            face(pl, wallYaw);
        } else {                                 // facing the wall
            wallFront = true;
            face(pl, wallYaw + 3.1415927f);
            if (jumpType == JUMP_TYPE_B) {       // back jump while facing it: turn first (motion 21), wait
                request_motion(pl, 21, ATTR_KICK, 0.0f, 0.0f, 1.0f);
                return;
            }
        }
        [[fallthrough]];
    }
    case 1: {
        // Kick-off motion by type and side. Front-facing / back-facing pairs: F 20 / 25, L 23 / 27, R 22 / 26.
        // After a wall run with the custom LMT, RunState.kickoff names a jump-out cycle that plays instead,
        // borrowing the vanilla kick-off's .ctn script; the physics and the rest of the phases are unchanged.
        int vanilla; float interp = 4.0f;
        switch (jumpType) {
        case JUMP_TYPE_F: vanilla = wallFront ? 20 : 25; break;
        case JUMP_TYPE_L: vanilla = wallFront ? 23 : 27; break;
        case JUMP_TYPE_R: vanilla = wallFront ? 22 : 26; break;
        case JUMP_TYPE_B:
            if (!wallFront) { vanilla = 25; break; }
            if (!at<bool>(pl, pl::mMotionFrameEnd0)) return;           // still turning
            vanilla = 28; interp = 0.0f;
            face(pl, yaw_of(tri_normal(&poly)));
            break;
        default:          vanilla = wallFront ? 20 : 25; break;        // V
        }
        RunState& rs = state_for(pl);
        if (rs.kickoff >= 0) {
            // The jump-out cycle is the whole jump: take-off, flight and whatever it loops into until landing.
            // Loop as its block says (A_LOOP_OFF not set), root translation off; phase 2 only launches.
            borrow_ctn(pl, 0, vanilla);
            request_motion(pl, rs.kickoff, (uint32_t)A_NULL_TRANS_OFF, interp, 0.0f, 1.0f);
            rs.kickoff = -1;
            rs.custom_kickoff = true;
        } else {
            request_motion(pl, vanilla, ATTR_KICK, interp, 0.0f, 1.0f);
        }
        // Turn toward the move direction, 20% of the remaining difference per request.
        if (jumpType != JUMP_TYPE_F || wallFront) {
            const float moveAng = at<float>(pl, pl::mJumpMoveAng);
            const float diff = rotate_difference(moveAng, at<float>(pl, pl::mRotateNow));
            rotate_request(pl, moveAng, std::fabs(diff * 0.2f));
        }
        ++phase;
        // A take-off on the cycle's first frame launches on this tick, not the next (one tick standing still).
        if (rs.custom_kickoff && jumpout_launch_frame <= 0.0f) launch_jumpout(rs);
        break;
    }
    case 2: {
        RunState& rs = state_for(pl);
        if (rs.custom_kickoff) {
            // A jump-out cycle: launch at its take-off frame, keep the cycle playing, skip the fall motion and
            // the phase-3 loop re-request (straight to the airborne phase).
            if (at<float>(pl, pl::mMotionFrame0) >= jumpout_launch_frame) launch_jumpout(rs);
            break;
        }
        if (at<bool>(pl, pl::mMotionFrameEnd0)) {
            const int fall = jumpType == JUMP_TYPE_V ? 4 : (jumpType != JUMP_TYPE_F || wallFront) ? 5 : 9;
            request_motion(pl, fall, ATTR_FALL, 10.0f, 7.0f, 1.0f);
            if (at<uint32_t>(pl, pl::mBtnOn) & 0x4)                         // jump still held: high variant
                at<int>(pl, pl::mJumpMode) = JUMP_MODE_NORM_WALL_HI;
            set_jump_rate(pl);
            wall_dust_effect(pl, tri_normal(&poly), poly.hitPos, efct_mtrl_flag(&poly));
            ++phase;
        }
        break;
    }
    case 3:
        if (at<uint8_t>(pl, pl::mMotContFree0) >= 2) {                      // .ctn trigger: loop the fall
            request_motion(pl, at<int>(pl, pl::mMotionCtrl0No), ATTR_LOOP, 0.0f, 51.0f, 1.0f);
            ++phase;
        }
        [[fallthrough]];
    case 4: {
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        if (state_for(pl).custom_kickoff && jumpout_custom_physics) {
            // Ballistic on the tunables: the horizontal speed set at take-off holds, gravity pulls, capped.
            vel.y += jumpout_gravity * at<float>(pl, pl::m_delta_time);
            if (vel.y < -jumpout_max_fall) vel.y = -jumpout_max_fall;
            acc.x = acc.y = acc.z = 0.0f;
            break;
        }
        adjust_jump_speed(pl);
        calc_speed_xz(pl, at<float>(pl, pl::mJumpMoveAng));
        break;
    }
    default:
        break;
    }

    // Every phase but a landing: the 10-frame timer from phase 0 ends in a pad vibration.
    float& t = at<float>(pl, pl::mActTimer0);
    if (t > 0.0f) {
        t -= at<float>(pl, pl::m_delta_time);
        if (t <= 0.0f) set_pad_vibration(pl, 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Hook: the entry of uPlayer::moveActWallJump 0x827200 (`this` in eax, no stack arguments, returns nothing). The
// first 6 bytes are `push ebp; mov ebp, esp; and esp, -16`; the detour runs our port and returns straight to the
// caller (moveActionLower), or replays those 6 bytes and continues the original at 0x827206.
// WallRun::move itself needs no hook: MoveTable's dispatch calls it.
// ---------------------------------------------------------------------------------------------------------------
static void __stdcall wall_jump_from_hook(uintptr_t player) { WallRun::move(player); }

naked void detour_wall_jump() {
    _asm {
        cmp byte ptr [WallRun::mod_enabled], 1
        jne original
        cmp byte ptr [WallRun::replace_wall_jump], 1
        jne original
        pushad
        push eax                        // this
        call wall_jump_from_hook        // __stdcall: pops its argument
        popad
        ret                             // back to moveActionLower, as the game's function would
    original:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [WallRun::jmp_ret1]
    }
}

// Hook: the entry of uPlayer::checkWallJumpBegin 0x804650 (`this` in esi, result in al, no stack arguments; the
// callers keep using esi afterwards). Same 6 prologue bytes as above; continues at 0x804656 when not replaced.
static bool s_check_result = false;

static bool __stdcall wall_jump_check_from_hook(uintptr_t player) { return WallRun::wall_jump_check_port(player); }

naked void detour_wall_jump_check() {
    _asm {
        cmp byte ptr [WallRun::mod_enabled], 1
        jne original
        cmp byte ptr [WallRun::replace_wall_jump_check], 1
        jne original
        pushad
        push esi                        // this
        call wall_jump_check_from_hook  // __stdcall: pops its argument, result in al
        mov byte ptr [s_check_result], al
        popad
        mov al, byte ptr [s_check_result]
        ret
    original:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [WallRun::jmp_ret2]
    }
}

std::optional<std::string> WallRun::on_initialize() {
    if (!install_hook_absolute(0x827200, hook1, &detour_wall_jump, &jmp_ret1, 6)) {
        spdlog::error("Failed to init WallRun hook (moveActWallJump entry)\n");
        return "Failed to init WallRun hook (moveActWallJump entry)";
    }
    if (!install_hook_absolute(0x804650, hook2, &detour_wall_jump_check, &jmp_ret2, 6)) {
        spdlog::error("Failed to init WallRun hook (checkWallJumpBegin entry)\n");
        return "Failed to init WallRun hook (checkWallJumpBegin entry)";
    }
    // the gun during the run: the shared pre-check and post-action ticks (sdk/InputCommand.hpp) and the two pose-setter hooks
    if (!input_commands::hook_installed) {
        if (!install_hook_absolute(0x7A83B5, hook_pre, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("WallRun: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init WallRun (pre-check hook)";
        }
        input_commands::hook_installed = true;
    }
    input_commands::add_tick(&precheck_tick);
    if (!input_commands::post_hook_installed) {
        if (!install_hook_absolute(0x7A8453, hook_post, &input_commands::detour_post, &input_commands::jmp_ret_post, 8)) {
            spdlog::error("WallRun: failed to hook uPlayer::main after moveAction (0x7A8453)");
            return "Failed to init WallRun (post-action hook)";
        }
        input_commands::post_hook_installed = true;
    }
    input_commands::add_post_tick(&post_action_tick);
    spine_aim::add_provider(&spine_provider);           // the torso lean while shooting
    if (!install_hook_absolute(0x7AA8C0, hook_pose, &detour_pose_lower, &jmp_ret_pose, 8)) {
        spdlog::error("WallRun: failed to hook uPlayer::setActionPoseLowerBdy (0x7AA8C0)");
        return "Failed to init WallRun (pose hook)";
    }
    if (!install_hook_absolute(0x7AA860, hook_pose_upper, &detour_pose_upper, &jmp_ret_pose_upper, 8)) {
        spdlog::error("WallRun: failed to hook uPlayer::setActionPoseUpperBdy (0x7AA860)");
        return "Failed to init WallRun (upper pose hook)";
    }
    return Mod::on_initialize();
}

void WallRun::on_frame(fmilliseconds& dt) {
    if (!mod_enabled || !use_custom_lmt) return;
    if (uPlayer* pl = devil4_sdk::get_local_player()) ensure_bank((uintptr_t)pl);
}

void WallRun::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;       // debug page: it hooks the wall jump of both characters
    ImGui::Checkbox(_("Wall Run"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Run along a wall found by two 45-degree casts. Jump kicks off into the game's wall jump."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::Checkbox(_("Replace the wall jump's entry check"), &replace_wall_jump_check);
    ImGui::SameLine();
    help_marker(_("Runs the C++ port of uPlayer::checkWallJumpBegin (the trigger: jump in the air near a wall) instead of the game's, "
                  "so its reach, height and the wall-jump count rule below apply."));
    if (replace_wall_jump_check) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Needs a wall jump left"), &require_wall_jumps);
        ImGui::SliderFloat(_("Check reach"), &check_reach, 10.0f, 300.0f);
        ImGui::SliderFloat(_("Check height"), &check_height, 0.0f, 150.0f);
        ImGui::InputScalar(_("Button (mBtnTrg bits)"), ImGuiDataType_U32, &button_mask, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::SameLine();
        help_marker(_("Hex mask of cInterface::mBtnTrg (+0x1410): a fresh press of any of these buttons while airborne starts the run. "
                      "4 = jump. No jump-window or act-limit condition applies."));
        ImGui::Unindent(lineIndent);
    }
    ImGui::Checkbox(_("Replace the game's wall jump"), &replace_wall_jump);
    ImGui::SameLine();
    help_marker(_("Runs the C++ port of uPlayer::moveActWallJump instead of the game's own (same behaviour; the port is "
                  "the base the wall run is built on, so this checks it)."));
    ImGui::Checkbox(_("Shoot while running"), &gun_while_running);
    ImGui::SameLine();
    help_marker(_("The gun fires from the upper half while the run goes on, exactly as the game's walking shot (its shot motion and "
                  "the elevation layer, on the upper body over the running legs, so the steps stay in sync); after each shot the arms "
                  "blend back into the run. A torso lean away from the wall while the gun is up (Spine aim must be on for the lean). "
                  "Bullets, effects, sound, mashing for rapid fire and the guns' own animations are the game's. No charge shots, no "
                  "Pandora. Melee still cancels."));
    ImGui::SliderFloat(_("Torso lean while shooting"), &shoot_lean_deg, -45.0f, 45.0f);
    ImGui::SameLine();
    help_marker(_("Degrees: negative leans away from the wall, positive into it. Needs Spine aim on."));
    ImGui::SliderFloat(_("Lean blend (frames)"), &lean_blend_frames, 0.0f, 30.0f);
    ImGui::SameLine();
    help_marker(_("How long the lean takes to come in when the gun comes up, and to go when it goes down. 0 = at once."));
    ImGui::SliderFloat(_("Shot hand-off (ticks, 0 = off)"), &shot_end_ticks, 0.0f, 120.0f);
    ImGui::SameLine();
    help_marker(_("Safety only: a shot action still running this many ticks after it started is ended (back to the run), so the "
                  "next press can shoot again. The shots end on their own (the Coyote-A fires at its motion's frame 36), so 0 is "
                  "right unless a weapon gets stuck."));
    ImGui::SliderFloat(_("Shot blend in (frames)"), &upper_blend_min, 0.0f, 20.0f);
    ImGui::SameLine();
    help_marker(_("The least blend a shot (and its aim layer) gets coming in during the run. The game cuts into the shot with no "
                  "blend once the gun counts as held, which snaps from the running arms. With Spine aim's inertial blends the shot "
                  "plays at full strength at once and only the gap from the running arms fades out over these frames, so 6-8 is "
                  "smooth and keeps the recoil; with the game's crossfade a long blend dilutes the short shots (3 is quick)."));
    ImGui::SliderFloat(_("Back to run blend (frames)"), &run_blend_frames, 1.0f, 30.0f);
    ImGui::SameLine();
    help_marker(_("How long the arms take to blend from the end of a shot back into the run cycle (with inertial blends: the fade "
                  "of the gap, the run cycle playing at once)."));
    ImGui::SliderFloat(_("Gun held after a shot (frames)"), &gun_hold_frames, 0.0f, 120.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("How long the gun stays up after the last shot before the arms go back to the run (60 = one second). A burst "
                  "then stays aimed instead of dipping into the run between shots. 0 = back to the run after every shot."));
    ImGui::Checkbox(_("Gun readout"), &gun_readout);
    ImGui::SameLine();
    help_marker(_("Debug lines for the local player: shots fired, the controllers' motions, the lower motion's state and the "
                  "cancel windows a follow-up shot needs."));
    if (gun_readout) {
        if (uPlayer* P = devil4_sdk::get_local_player()) {
            const uintptr_t p = (uintptr_t)P;
            ImGui::Text("gun: shots %d | %s | lower motion %04X upper %04X aim %04X | custom LMT %s | upper action %d",
                        g_shots, g_last, at<uint32_t>(p, pl::mMotionCtrl0No), at<uint32_t>(p, kMotionCtrl + kMotionCtrlSize * 1 + kMcMotionNo),
                        at<uint32_t>(p, kMotionCtrl + kMotionCtrlSize * 2 + kMcMotionNo), WallRun::custom_ready(p) ? "yes" : "no", at<int>(p, kBdyUpperNo));
            // the lower motion's life signs: its frame, the controller's speed and attribute word (0x10 = A_STOP), the actor's
            // motion work rate (mProcSpdCtrl.mpWorkRate[1], what the characters' setActionUpperBdy rewrite)
            const float* work = at<float*>(p, 0x145C);
            ImGui::Text("lower: frame %.1f | speed %.2f | attr %04X add %04X | work rate %.2f | act limit %04X stat %02X",
                        at<float>(p, pl::mMotionFrame0), at<float>(p, kMotionCtrl + 0x24), (uint32_t)at<uint16_t>(p, kMotionCtrl + 0x28),
                        (uint32_t)at<uint16_t>(p, kMotionCtrl + kMcAddAttribute), work ? work[1] : -1.0f,
                        at<uint32_t>(p, pl::mActLimit), at<uint32_t>(p, pl::mActStat));
            // the windows a follow-up shot needs (mStat: 0 shut, 1 buffer, 2 open), the attack id, the upper motion's frame
            ImGui::Text("rapid: C_NORM0 %d | C_GUN %d | C_ALL %d | atck id %d | buffered %s | upper frame %.1f",
                        at<int>(p, kMotCncl + kMotCnclSize * C_NORM0 + 4), at<int>(p, kMotCncl + kMotCnclSize * C_GUN + 4),
                        at<int>(p, kMotCncl + kMotCnclSize * C_ALL + 4), at<int>(p, pl::mAtckId),
                        state_for(p).gun_buffered ? "yes" : "no", at<float>(p, kModelMotion0Frame + 0xE0));
        } else ImGui::Text("gun: shots %d | %s", g_shots, g_last);
    }
    ImGui::SliderFloat(_("Run speed"), &run_speed, 0.0f, 40.0f);
    ImGui::SliderFloat(_("Max run frames"), &run_frames, 0.0f, 300.0f);
    ImGui::SameLine();
    help_marker(_("The run lasts while jump is held; this caps it (0 = no cap). Releasing jump kicks off with the game's wall jump."));
    ImGui::SliderFloat(_("Wall stick"), &stick_speed, 0.0f, 10.0f);
    ImGui::SliderFloat(_("Lose grace"), &lose_grace, 0.0f, 30.0f);
    ImGui::SameLine();
    help_marker(_("Frames the run keeps going on the last wall after the casts stop hitting (the wall ended, or bent too sharply) "
                  "before it ends with the jump-off from that wall."));
    ImGui::SliderFloat(_("Wall gap"), &wall_gap, 0.0f, 100.0f);
    ImGui::SameLine();
    help_marker(_("The position is pushed back out to this distance from the wall plane every tick of the run (0 = off). "
                  "Stops the animation's root motion or a missed push from clipping through."));
    ImGui::SliderFloat(_("Cast length"), &cast_length, 10.0f, 400.0f);
    ImGui::SliderFloat(_("Cast height"), &cast_height, 0.0f, 200.0f);
    ImGui::SliderFloat(_("Cast angle"), &cast_angle, 5.0f, 85.0f);
    ImGui::SliderFloat(_("Corner dot"), &corner_dot, -1.0f, 1.0f);
    ImGui::Checkbox(_("Run on invisible walls"), &run_on_invisible_walls);
    ImGui::SameLine();
    help_marker(_("Also accept the fall-stop channel (0x10000000: the invisible walls that stop Stinger at ledges) in the "
                  "run's casts and the entry check. The game's own wall checks only see 0x20000."));
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Camera-stop walls too"), &run_on_camera_walls);
    ImGui::InputScalar(_("Wall filter"), ImGuiDataType_U32, &wall_filter, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("The base collision filter of the casts (hex). 20000 = walls, the game's value; the two options above add their channels to it."));
    ImGui::SliderFloat(_("Climb angle"), &climb_angle, 0.0f, 90.0f);
    ImGui::SameLine();
    help_marker(_("Facing the wall within this many degrees of its normal runs up it; otherwise the run goes sideways."));
    ImGui::SliderFloat(_("Climb speed"), &climb_speed, 0.0f, 40.0f);
    ImGui::SliderFloat(_("Away deadzone"), &away_deadzone, 0.0f, 1.0f);
    ImGui::SameLine();
    help_marker(_("A stick leaning away from the wall only steers by its part along the wall; below this much it does nothing."));
    ImGui::Combo(_("Jumps after a run"), &jumps_after_run, "Costs one wall jump\0Costs nothing\0Reset to the maximum\0");
    ImGui::SameLine();
    help_marker(_("Entering the run spends a wall jump and counts an air jump, like the game's wall jump. "
                  "Costs nothing gives both back when the run ends; Reset fills them to the character's maxima."));
    ImGui::Checkbox(_("Custom wall run LMT"), &use_custom_lmt);
    ImGui::SameLine();
    help_marker(_("Off: the wall-jump kick-off motions stand in for the run cycles.\n"
                  "On: the LMT below is loaded into a free motion bank of the body and coat at runtime, and the run "
                  "plays bank:slot from it. The .ctn cancels of the placeholder motions are borrowed either way."));
    if (!use_custom_lmt) {
        ImGui::InputInt(_("Motion left"), &motion_left);
        ImGui::InputInt(_("Motion right"), &motion_right);
        ImGui::InputInt(_("Motion up"), &motion_up);
    } else {
        ImGui::SliderInt(_("Bank"), &lmt_bank, 10, 14);
        ImGui::InputInt(_("Slot left"), &run_slot_left);
        ImGui::InputInt(_("Slot right"), &run_slot_right);
        ImGui::InputInt(_("Slot up"), &run_slot_up);
        ImGui::InputInt(_("Slot jump-out right"), &run_slot_jump_right);
        ImGui::InputInt(_("Slot jump-out left"), &run_slot_jump_left);
        ImGui::SliderFloat(_("Jump-out launch frame"), &jumpout_launch_frame, 0.0f, 60.0f);
        ImGui::SameLine();
        help_marker(_("Frame of the jump-out cycle at which the jump's velocity starts (the take-off). The cycle keeps playing "
                      "through the flight; no fall motion is added."));
        ImGui::Checkbox(_("Jump-out physics"), &jumpout_custom_physics);
        ImGui::SameLine();
        help_marker(_("Off: the game's jump table drives the jump-out (set_jump_rate / adjustJumpSpeed). On: the speeds below, "
                      "in cm per frame, from the take-off frame until landing."));
        if (jumpout_custom_physics) {
            ImGui::Indent(lineIndent);
            ImGui::SliderFloat(_("Up"), &jumpout_up, 0.0f, 60.0f);
            ImGui::SliderFloat(_("Away from wall"), &jumpout_out, 0.0f, 40.0f);
            ImGui::SliderFloat(_("Along the run"), &jumpout_along, -20.0f, 40.0f);
            ImGui::SliderFloat(_("Gravity"), &jumpout_gravity, -5.0f, 0.0f);
            ImGui::SliderFloat(_("Max fall"), &jumpout_max_fall, 1.0f, 80.0f);
            ImGui::Unindent(lineIndent);
        }
        ImGui::InputText(_("Body LMT"), lmt_body, sizeof(lmt_body));
        ImGui::Checkbox(_("Load coat LMT too"), &load_coat_lmt);
        ImGui::SameLine();
        help_marker(_("Off: only the body plays the custom cycles and the coat holds its pose during the run."));
        if (load_coat_lmt) {
            ImGui::InputText(_("Nero coat LMT"), lmt_coat_nero, sizeof(lmt_coat_nero));
            ImGui::InputText(_("Dante coat LMT"), lmt_coat_dante, sizeof(lmt_coat_dante));
        }
        if (uPlayer* pl = devil4_sdk::get_local_player()) {
            ImGui::Text(_("File: %s | in use: %s"), lmt_file_exists(lmt_body) ? "found" : "missing",
                        custom_ready((uintptr_t)pl) ? "custom cycles" : "placeholders");
            ImGui::Text(_("Loaded: body %s, coat %s"),
                        devil4_sdk::get_motion_bank(pl, lmt_bank) ? "yes" : "no",
                        devil4_sdk::get_motion_bank((void*)at<uintptr_t>((uintptr_t)pl, pl::mpCoatMdl), lmt_bank) ? "yes" : "no");
            if (ImGui::Button(_("Reload LMT"))) {       // swap the slots out and let ensure_bank refill them
                devil4_sdk::unload_motion_bank(pl, lmt_bank);
                if (load_coat_lmt)
                    devil4_sdk::unload_motion_bank((void*)at<uintptr_t>((uintptr_t)pl, pl::mpCoatMdl), lmt_bank);
            }
        }
    }
    ImGui::Unindent(lineIndent);
}

void WallRun::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("wall_run").value_or(false);
    replace_wall_jump = cfg.get<bool>("wall_run_replace_wall_jump").value_or(true);
    replace_wall_jump_check = cfg.get<bool>("wall_run_replace_check").value_or(true);
    require_wall_jumps = cfg.get<bool>("wall_run_require_wall_jumps").value_or(false);
    check_reach  = cfg.get<float>("wall_run_check_reach").value_or(check_reach);
    check_height = cfg.get<float>("wall_run_check_height").value_or(check_height);
    button_mask  = (uint32_t)cfg.get<int>("wall_run_button").value_or((int)button_mask);
    run_speed   = cfg.get<float>("wall_run_speed").value_or(run_speed);
    gun_while_running = cfg.get<bool>("wall_run_gun").value_or(gun_while_running);
    shot_end_ticks    = cfg.get<float>("wall_run_shot_end").value_or(shot_end_ticks);
    upper_blend_min   = cfg.get<float>("wall_run_shot_blend").value_or(upper_blend_min);   // new key: the old one held 8
    run_blend_frames  = cfg.get<float>("wall_run_back_blend").value_or(run_blend_frames);
    gun_hold_frames   = cfg.get<float>("wall_run_gun_hold").value_or(gun_hold_frames);
    gun_readout       = cfg.get<bool>("wall_run_gun_readout").value_or(gun_readout);
    shoot_lean_deg     = cfg.get<float>("wall_run_shot_lean").value_or(shoot_lean_deg);
    lean_blend_frames  = cfg.get<float>("wall_run_lean_blend").value_or(lean_blend_frames);
    run_frames  = cfg.get<float>("wall_run_frames").value_or(run_frames);
    wall_gap    = cfg.get<float>("wall_run_wall_gap").value_or(wall_gap);
    lose_grace  = cfg.get<float>("wall_run_lose_grace").value_or(lose_grace);
    use_custom_lmt = cfg.get<bool>("wall_run_custom_lmt").value_or(false);
    lmt_bank       = cfg.get<int>("wall_run_bank").value_or(lmt_bank);
    run_slot_left  = cfg.get<int>("wall_run_slot_left").value_or(run_slot_left);
    run_slot_right = cfg.get<int>("wall_run_slot_right").value_or(run_slot_right);
    run_slot_up    = cfg.get<int>("wall_run_slot_up").value_or(run_slot_up);
    run_slot_jump_right = cfg.get<int>("wall_run_slot_jump_right").value_or(run_slot_jump_right);
    run_slot_jump_left  = cfg.get<int>("wall_run_slot_jump_left").value_or(run_slot_jump_left);
    jumpout_launch_frame = cfg.get<float>("wall_run_jumpout_launch").value_or(jumpout_launch_frame);
    jumpout_custom_physics = cfg.get<bool>("wall_run_jumpout_physics").value_or(true);
    jumpout_up       = cfg.get<float>("wall_run_jumpout_up").value_or(jumpout_up);
    jumpout_out      = cfg.get<float>("wall_run_jumpout_out").value_or(jumpout_out);
    jumpout_along    = cfg.get<float>("wall_run_jumpout_along").value_or(jumpout_along);
    jumpout_gravity  = cfg.get<float>("wall_run_jumpout_gravity").value_or(jumpout_gravity);
    jumpout_max_fall = cfg.get<float>("wall_run_jumpout_max_fall").value_or(jumpout_max_fall);
    climb_angle    = cfg.get<float>("wall_run_climb_angle").value_or(climb_angle);
    wall_filter    = (uint32_t)cfg.get<int>("wall_run_filter").value_or((int)COLL_WALL);
    run_on_invisible_walls = cfg.get<bool>("wall_run_invisible_walls").value_or(false);
    run_on_camera_walls    = cfg.get<bool>("wall_run_camera_walls").value_or(false);
    climb_speed    = cfg.get<float>("wall_run_climb_speed").value_or(climb_speed);
    jumps_after_run = cfg.get<int>("wall_run_jumps_after").value_or(jumps_after_run);
    load_coat_lmt   = cfg.get<bool>("wall_run_coat_lmt").value_or(false);
}

void WallRun::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("wall_run", mod_enabled);
    cfg.set<bool>("wall_run_replace_wall_jump", replace_wall_jump);
    cfg.set<bool>("wall_run_replace_check", replace_wall_jump_check);
    cfg.set<bool>("wall_run_require_wall_jumps", require_wall_jumps);
    cfg.set<float>("wall_run_check_reach", check_reach);
    cfg.set<float>("wall_run_check_height", check_height);
    cfg.set<int>("wall_run_button", (int)button_mask);
    cfg.set<float>("wall_run_speed", run_speed);
    cfg.set<bool>("wall_run_gun", gun_while_running);
    cfg.set<float>("wall_run_shot_end", shot_end_ticks);
    cfg.set<float>("wall_run_shot_blend", upper_blend_min);
    cfg.set<float>("wall_run_back_blend", run_blend_frames);
    cfg.set<float>("wall_run_gun_hold", gun_hold_frames);
    cfg.set<bool>("wall_run_gun_readout", gun_readout);
    cfg.set<float>("wall_run_shot_lean", shoot_lean_deg);
    cfg.set<float>("wall_run_lean_blend", lean_blend_frames);
    cfg.set<float>("wall_run_frames", run_frames);
    cfg.set<float>("wall_run_wall_gap", wall_gap);
    cfg.set<float>("wall_run_lose_grace", lose_grace);
    cfg.set<bool>("wall_run_custom_lmt", use_custom_lmt);
    cfg.set<int>("wall_run_bank", lmt_bank);
    cfg.set<int>("wall_run_slot_left", run_slot_left);
    cfg.set<int>("wall_run_slot_right", run_slot_right);
    cfg.set<int>("wall_run_slot_up", run_slot_up);
    cfg.set<int>("wall_run_slot_jump_right", run_slot_jump_right);
    cfg.set<int>("wall_run_slot_jump_left", run_slot_jump_left);
    cfg.set<float>("wall_run_jumpout_launch", jumpout_launch_frame);
    cfg.set<bool>("wall_run_jumpout_physics", jumpout_custom_physics);
    cfg.set<float>("wall_run_jumpout_up", jumpout_up);
    cfg.set<float>("wall_run_jumpout_out", jumpout_out);
    cfg.set<float>("wall_run_jumpout_along", jumpout_along);
    cfg.set<float>("wall_run_jumpout_gravity", jumpout_gravity);
    cfg.set<float>("wall_run_jumpout_max_fall", jumpout_max_fall);
    cfg.set<float>("wall_run_climb_angle", climb_angle);
    cfg.set<int>("wall_run_filter", (int)wall_filter);
    cfg.set<bool>("wall_run_invisible_walls", run_on_invisible_walls);
    cfg.set<bool>("wall_run_camera_walls", run_on_camera_walls);
    cfg.set<float>("wall_run_climb_speed", climb_speed);
    cfg.set<int>("wall_run_jumps_after", jumps_after_run);
    cfg.set<bool>("wall_run_coat_lmt", load_coat_lmt);
}
