#include "WallRun.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/uActor.hpp"
#include "../sdk/InputCommand.hpp"   // the pre-check and post-action ticks
#include "../sdk/MotionRegistry.hpp"  // the custom LMT's bank
#include "../sdk/Enums_Internal.hpp" // CANCEL_NO
#include "MoveTable.hpp"             // HookNeroKADTbl / HookDanteKADTbl: the gun rows the run matches itself
#include "../sdk/SpineAim.hpp"       // the torso lean while shooting
#include "UpperGun.hpp"              // the gun during the run: the run is one of its hosts
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

using namespace wallrun;

bool  WallRun::mod_enabled  = false;
uintptr_t WallRun::jmp_ret1 = NULL;
uintptr_t WallRun::jmp_ret2 = 0;
uint32_t WallRun::button_mask = 0x4;
bool  WallRun::gun_while_running = true;
bool  WallRun::run_on_invisible_walls = false;
int   WallRun::lmt_set         = -1;

// ---------------------------------------------------------------------------------------------------------------
// Fixed values
// ---------------------------------------------------------------------------------------------------------------
namespace {

// The entry check (wall_jump_check_port): a line through the hip, check_reach back to check_reach forward.
constexpr float check_reach  = 150.0f;  // cm (the game: 80)
constexpr float check_height = 10.0f;   // cm above the feet (the game: 10)

// The run. It lasts while jump is held; releasing it (or losing the wall) kicks off with the game's wall jump.
constexpr float run_speed     = 12.0f;  // cm per frame along the wall
constexpr float stick_speed   = 2.0f;   // cm per frame pushed into the wall so the capsule stays on it
constexpr float wall_gap      = 30.0f;  // cm the position is kept away from the wall plane each tick: the root-motion backstop
constexpr float lose_grace    = 4.0f;   // frames the run keeps going on the last wall after the casts stop hitting, before jumping off
constexpr float climb_angle   = 35.0f;  // degrees between the heading and the wall normal below which the run goes up, not sideways
constexpr float climb_speed   = 10.0f;  // cm per frame up the wall
constexpr float away_deadzone = 0.3f;   // a stick leaning away from the wall: its along-wall part below this (0-1) has no effect

// Wall detection: two casts from the hip, fanned around the heading.
constexpr float cast_length = 120.0f;   // cm, each
constexpr float cast_height = 80.0f;    // cm above the feet
constexpr float cast_angle  = 45.0f;    // degrees off the heading, per side
constexpr float corner_dot  = 0.5f;     // two hits whose normals' dot is below this (60 degrees apart) are a corner, not a wall
// The collision channel the casts and the entry check accept (sCollision::Param filter): walls, as the game's own wall
// checks, plus invisible walls (the fall-stop channel, what stops Stinger at a ledge) with run_on_invisible_walls.
// Camera-stop walls (0x20000000) are not run on.
constexpr uint32_t COLL_WALL     = 0x20000;
constexpr uint32_t COLL_FALLSTOP = 0x10000000;

// Placeholder motions (bank 0, the wall jump's kick-offs) while the custom LMT isn't on the player; the custom cycles
// borrow their .ctn scripts.
constexpr int motion_left  = 23;        // L
constexpr int motion_right = 22;        // R
constexpr int motion_up    = 20;        // F (facing the wall)

// The custom LMT (one motion_registry set: the body file for both characters; the coat holds its pose during the run).
constexpr const char* lmt_body = "motion\\common\\wallrun";   // game path under nativePC, without .lmt
constexpr int run_slot_right      = 0;  // the run cycles, attribute 0 (looping)
constexpr int run_slot_left       = 1;
constexpr int run_slot_up         = 2;
constexpr int run_slot_jump_right = 3;  // jump-out cycles: the kick-off after a run on a right / left wall
constexpr int run_slot_jump_left  = 4;

// Jump-out physics (cm per frame) after a jump-out cycle, in place of the jump table's; the take-off is the cycle's first frame.
constexpr float jumpout_up       = 22.0f;   // vertical speed at take-off
constexpr float jumpout_out      = 14.0f;   // speed away from the wall (along its normal)
constexpr float jumpout_along    = 6.0f;    // speed along the run's direction
constexpr float jumpout_gravity  = -1.1f;   // added to the vertical speed every frame
constexpr float jumpout_max_fall = 30.0f;   // fastest fall speed

// The gun during the run: UpperGun's tunables for this host (read through pointers) and SpineAim's torso lean.
constexpr float shoot_lean_deg    = -45.0f; // torso lean while shooting (joint 1): negative = away from the wall
constexpr float lean_blend_frames = 8.0f;   // frames the lean takes to come in when the gun comes up, and to go
constexpr float shot_end_ticks    = 0.0f;   // safety hand-off of a shot still running this many ticks after its start (0 = never)
constexpr float upper_blend_min   = 6.0f;   // frames: the least blend a shot's upper / aim request gets during the run (the shot
                                            // plays at full strength at once, only the gap from the running arms fades)
constexpr float run_blend_frames  = 8.0f;   // frames the arms take to blend back into the run cycle after a shot
constexpr float gun_hold_frames   = 10.0f;  // frames the gun stays up after the last shot before the arms go back to the run

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Game functions. All __usercall; each thunk saves the registers and the stack pointer around the call so the
// __userpurge ones (callee pops) are fine too. Addresses are DX9.
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_setAction        = 0x007AA030; // uPlayer::setAction(this@ecx, int act)          vtbl slot 101
static uintptr_t fn_initJumpBegin     = 0x007AC9E0; // uPlayer::initJumpBegin(this@ecx, int)          vtbl slot 109
static uintptr_t fn_setActionLand     = 0x007AA6A0; // uPlayer::setActionLand(this@ecx)
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
    devil4_sdk::indexed_anim_call(motion, (uPlayer_old*)pl, attr, speed, start, interp);
}

// ---- small helpers over the player ---------------------------------------------------------------------------

glm::vec3 wmat_row(uintptr_t pl, int row) {           // rows: 0 right, 1 up, 2 forward, 3 position
    const float* r = &at<float>(pl, pl::mWmat + 0x10 * row);
    return glm::vec3(r[0], r[1], r[2]);
}

CollParam wall_param() { return collision::line_param(COLL_WALL | (WallRun::run_on_invisible_walls ? COLL_FALLSTOP : 0)); }

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

// the upper body during the run: UpperGun (the run is one of its hosts, see "the gun during the run" below)
inline bool running(uintptr_t pl) {
    return at<int>(pl, pl::mBdyActionLowerNo) == ACT_WALL_JUMP && at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_RUN;
}
inline float wrap_pi(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }

struct RunState {                   // what the game's phase word can't hold
    uintptr_t player = 0;
    int motion = -1;                // the motion playing, to re-request when the direction mode changes
    int kickoff = -1;               // custom jump-out motion for the port's kick-off (-1 = the vanilla one)
    bool custom_kickoff = false;    // a jump-out cycle is playing: it is the whole jump, the port adds no fall motion
    bool from_run = false;          // the port's next phase 0 follows a sideways run (set by hand_to_wall_jump, read once)
    float lost = 0.0f;              // frames since the casts last hit (the wall ended, or bent too sharply)
    uint32_t motion_attr = 0;      // the attribute the run cycle was requested with (the arms go back to it after a shot)
    Wall wall{};
    uint32_t last_seen = 0;         // state_for's tick when this player was last seen (eviction order)
};

RunState g_runs[8];                 // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)

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
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    const glm::vec3 n = wall.normal;
    const float d = glm::dot(glm::vec3(pos) - glm::vec3(wall.hit_pos), n);   // signed distance to the plane
    if (d < wall_gap) {
        const glm::vec3 fixed = glm::vec3(pos) + n * (wall_gap - d);
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
    const bool lower_only = upper_gun::busy(pl);   // (held after a shot: the arms stay up too)
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
// --mWallJumpCnt, ++mWJumpCnt). Give them back when the run ends, so the run costs nothing and the player can keep jumping.
void give_jumps_back(uintptr_t pl) {
    char& wall_left = at<char>(pl, pl::mWallJumpCnt);
    char& air_used  = at<char>(pl, pl::mWJumpCnt);
    ++wall_left;
    if (air_used > 0) --air_used;
}

// Hand the action over to the game's wall jump: it reads the wall from mGroundPoly and runs its phases 0-4 in
// the same phase word from here on (checkWallJumpBegin already set mJump.mode and the jump counters on entry).
void hand_to_wall_jump(RunState& s, uintptr_t pl, bool from_run) {
    s.kickoff = -1;
    s.custom_kickoff = false;
    s.from_run = from_run && !s.wall.vertical;               // a climb faces the wall: the port's own choice is clear
    if (from_run) {
        at<LineResult>(pl, pl::mGroundPoly) = s.wall.poly;   // else keep checkWallJumpBegin's wall
        give_jumps_back(pl);                                 // the kick-off itself doesn't charge again
        if (WallRun::custom_ready(pl) && !s.wall.vertical)   // the jump-out cycles (none for a climb yet; -1 = none)
            s.kickoff = motion_registry::word(WallRun::lmt_set, pl,
                                              s.wall.on_left ? run_slot_jump_left : run_slot_jump_right);
    }
    at<int>(pl, pl::mBdyActionLowerPhase) = 0;
    upper_gun::end(pl);
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
        s.lost   = 0.0f;
        s.custom_kickoff = false;
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

    uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
    if (coll && at<bool>(coll, 0x1C)) { upper_gun::end(pl); set_action_land(pl); }   // mLand: back on the ground
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

bool WallRun::custom_ready(uintptr_t player) {     // installs the player's banks when needed (call from its tick)
    return player && lmt_set >= 0 && motion_registry::ready(lmt_set, player);
}

int WallRun::motion_for(uintptr_t player, const Wall& wall) {
    if (!custom_ready(player)) return placeholder_motion(wall);   // no file for this player: the stand-ins
    const int slot = wall.vertical ? run_slot_up : wall.on_left ? run_slot_left : run_slot_right;
    const int word = motion_registry::word(lmt_set, player, slot);
    return word >= 0 ? word : placeholder_motion(wall);           // a slot past the file's blocks: the stand-in
}

// SpineAim asks this during its pass: the torso leans away from the wall while the gun is up (the walking shot has none
// of the run cycle's lean), faded by lean_w. The axis turns "up" toward the wall, so a negative angle leans away.
bool spine_provider(uintptr_t pl, spine_aim::Request& out) {
    if (!WallRun::mod_enabled || !WallRun::gun_while_running || !running(pl)) return false;
    RunState& s = state_for(pl);
    const float w = upper_gun::lean_weight(pl);
    if (s.wall.vertical || w <= 0.0f) return false;
    const glm::vec3 axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -glm::vec3(s.wall.normal));
    if (glm::length(axis) <= 1e-3f) return false;
    out.lean_axis = glm::normalize(axis);
    out.lean_deg = shoot_lean_deg * w;
    return true;
}

// ---- the gun during the run: UpperGun's host (RE/player_move_pipeline.md "Upper-body gun shots"; the code is UpperGun.cpp) ----
// The run is lower-body action 0x18, so the shot's mAtckId is its own (no id swap). The words the shot overwrites
// (mActionNo = 12, the limits) are saved before the first shot and put back every post-action tick, with the run's
// facing; after a shot the arms go back to the run cycle on the upper / aim controllers.
bool gun_active(uintptr_t pl) { return WallRun::mod_enabled && running(pl); }
bool gun_facing(uintptr_t pl, float* yaw) { *yaw = state_for(pl).wall.yaw; return true; }
bool gun_lower_motion(uintptr_t pl, upper_gun::Motion* m) {
    const RunState& s = state_for(pl);
    if (s.motion < 0) return false;
    m->motion = s.motion; m->attr = s.motion_attr; m->speed = 1.0f;
    return true;
}

// The custom LMT as one motion registry set: the body file for both characters (no coat file: the coat holds its pose
// during the run). A missing file is logged by the registry, and the run keeps its placeholder motions.
void WallRun::register_lmt() {
    const int old = lmt_set;
    motion_registry::SetDesc d;
    d.owner = "WallRun";
    d.body[motion_registry::NERO] = d.body[motion_registry::DANTE] = lmt_body;
    lmt_set = motion_registry::add(d);
    if (old >= 0) motion_registry::remove(old);
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
    // No wall-jump count rule, unlike the game's check: a run starts with no wall jumps left too.

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
        set_jump_rate(pl);                                  // the table's values (inertia, speed, gravity) ...
        const glm::vec3 n = tri_normal(&poly);              // ... with the jump-out physics in place of its speeds
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
        // A jump-out cycle takes off on its first frame: launch on this tick (not the next: one tick standing still),
        // keep the cycle playing, skip the fall motion and the phase-3 loop re-request (straight to the airborne phase).
        if (rs.custom_kickoff) launch_jumpout(rs);
        break;
    }
    case 2: {
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
        if (state_for(pl).custom_kickoff) {
            // Ballistic on the jump-out physics: the horizontal speed set at take-off holds, gravity pulls, capped.
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
    // the gun during the run: UpperGun (the shared pre-check / post-action ticks and the two pose-setter hooks, installed once
    // for every host); the run's own tunables are read through the pointers
    {
        upper_gun::Host h;
        h.name = "WallRun";
        h.enabled = &gun_while_running;
        h.active = &gun_active;
        h.facing = &gun_facing;
        h.lower_motion = &gun_lower_motion;
        h.shot_end_ticks = &shot_end_ticks;
        h.upper_blend_min = &upper_blend_min;
        h.back_blend_frames = &run_blend_frames;
        h.gun_hold_frames = &gun_hold_frames;
        h.lean_blend_frames = &lean_blend_frames;
        if (!upper_gun::add_host(h)) {
            spdlog::error("WallRun: UpperGun refused the host (hooks)");
            return "Failed to init WallRun (UpperGun hooks)";
        }
    }
    spine_aim::add_provider(&spine_provider);           // the torso lean while shooting
    return Mod::on_initialize();
}

void WallRun::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;       // debug page: it hooks the wall jump of both characters
    ImGui::Checkbox(_("Wall Run"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Run along a wall found by two 45-degree casts. Jump kicks off into the game's wall jump."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::InputScalar(_("Button (mBtnTrg bits)"), ImGuiDataType_U32, &button_mask, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("Hex mask of cInterface::mBtnTrg (+0x1410): a fresh press of any of these buttons while airborne starts the run. "
                  "4 = jump. No jump-window or act-limit condition applies."));
    ImGui::Checkbox(_("Shoot while running"), &gun_while_running);
    ImGui::SameLine();
    help_marker(_("The gun fires from the upper half while the run goes on, exactly as the game's walking shot (its shot motion and "
                  "the elevation layer, on the upper body over the running legs, so the steps stay in sync); after each shot the arms "
                  "blend back into the run. A torso lean away from the wall while the gun is up. "
                  "Bullets, effects, sound, mashing for rapid fire and the guns' own animations are the game's. No charge shots, no "
                  "Pandora. Melee still cancels."));
    ImGui::Checkbox(_("Run on invisible walls"), &run_on_invisible_walls);
    ImGui::SameLine();
    help_marker(_("Also run on the invisible walls that stop Stinger at ledges (the fall-stop collision channel), in the run's "
                  "casts and the entry check. Off: only real walls, as the game's own wall jump."));
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | in use: %s"), lmt_file_exists(lmt_body) ? "found" : "missing",
                    motion_registry::placed(lmt_set, (uintptr_t)pl) ? "custom cycles" : "placeholders");
        ImGui::Text("%s", motion_registry::describe(lmt_set, (uintptr_t)pl).c_str());
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();   // read again from disk on the next run
        ImGui::SameLine();
        help_marker(_("Reads the run's LMT (nativePC\\motion\\common\\wallrun.lmt) again from disk on the next run. "
                      "Without the file the run plays the wall jump's kick-off motions instead."));
    }
    ImGui::Unindent(lineIndent);
}

void WallRun::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("wall_run").value_or(false);
    button_mask = (uint32_t)cfg.get<int>("wall_run_button").value_or((int)button_mask);
    gun_while_running = cfg.get<bool>("wall_run_gun").value_or(gun_while_running);
    run_on_invisible_walls = cfg.get<bool>("wall_run_invisible_walls").value_or(run_on_invisible_walls);
    register_lmt();
}

void WallRun::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("wall_run", mod_enabled);
    cfg.set<int>("wall_run_button", (int)button_mask);
    cfg.set<bool>("wall_run_gun", gun_while_running);
    cfg.set<bool>("wall_run_invisible_walls", run_on_invisible_walls);
}
