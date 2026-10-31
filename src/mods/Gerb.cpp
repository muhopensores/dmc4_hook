#include "Gerb.hpp"
#include "NeroStyle.hpp"
#include "GerbShell.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MotionRegistry.hpp"
#include "../sdk/CtnScript.hpp"
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace gerb {

bool  mod_enabled   = false;
// the rows' inputs (Gerb.hpp): directional = lock-on + any direction + style, vertical = lock-on + stick centred + style
CustomInput custom_input[2] = {
    { SRC_GAME, 0x200, true, STICK_ANY },
    { SRC_GAME, 0x200, true, STICK_CENTRED },
};
int   lmt_set       = -1;
char  lmt_path[128] = "motion\\pl000\\gerb";          // Nero's
int   slot[DIR_COUNT] = { 0, 1, 2, 4, 3, 5 };   // forward, down, back, left, right, up
int   ctn_bank      = 0;
int   ctn_slot      = 5;           // the fall motion's script: no cancels until the game's air rules say so
float launch_frame  = 0.0f;
float speed         = 20.0f;
float rise          = 14.0f;
float up_speed      = 32.0f;
float down_speed    = 24.0f;
float gravity       = -1.1f;
float max_fall      = 30.0f;
float cancel_frame  = 10.0f;
float side_angle    = 50.0f;
bool  face_direction  = true;
bool  airborne_status = true;
bool  effect_enabled  = true;
int   effect_list     = 1;
int   effect_index    = 0x28;
float effect_tilt     = 45.0f;
bool  se_enabled      = true;
int   se_id           = 40;

// ---------------------------------------------------------------------------------------------------------------
// Game functions (the few the jump needs; WallRun.cpp keeps its own copies file-local).
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_setActionLand = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

constexpr int kPlayerSlots = 4;        // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)
// The cancel window the game's own way (MoveDef D11): one in-memory .ctn script per player slot (sdk/CtnScript.hpp):
// wait(cancel_frame), then every cancel category opened; the bytes must outlive the motion, so they're static.
static ctn::Script g_cancel_scripts[kPlayerSlots];

// The rows' command bits are set by custom_tick, not by a command of the game's checker; an empty command on each, never
// enabled, keeps them reserved (input_commands::next_free_bit, which MoveDef uses, skips registered bits).
static input_commands::Command g_reserved_cmd[2];
static const bool kNever = false;

void clear_add_speed(uintptr_t pl) {
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

// The player's orientation (mWmat rows right / up / forward) with its up leaned `tilt_deg` toward the level part of
// `dir`, as an MT quaternion (x, y, z, w) through mt_basis_quat (MtMath.hpp). A vertical `dir` gives no lean; with no
// tilt it reproduces mQuat (the debug readout shows both).
glm::vec3 wmat_row(uintptr_t pl, int row);   // defined below

// `roll_deg` then turns the basis about its own forward axis (180 = upside down, for the dive).
void basis_quat(uintptr_t pl, const glm::vec3& dir, float tilt_deg, float roll_deg, float out[4]) {
    glm::vec3 b[3] = { wmat_row(pl, 0), wmat_row(pl, 1), wmat_row(pl, 2) };
    const glm::vec3 h(dir.x, 0.0f, dir.z);
    if (tilt_deg != 0.0f && glm::length(h) > 1e-3f) {
        const glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(h)));
        for (glm::vec3& v : b) v = mt_rodrigues(v, axis, tilt_deg);
    }
    if (roll_deg != 0.0f) {
        const glm::vec3 axis = glm::normalize(b[2]);
        b[0] = mt_rodrigues(b[0], axis, roll_deg);
        b[1] = mt_rodrigues(b[1], axis, roll_deg);
    }
    mt_basis_quat(b[0], b[1], b[2], out);
}

// The air hike's effect and sound at the take-off: the effect leaned toward the jump, upside down for the dive.
void take_off_fx(uintptr_t pl, const glm::vec3& dir) {
    if (effect_enabled) {
        float q[4];
        const glm::vec3 h(dir.x, 0.0f, dir.z);
        const float roll = dir.y < -0.5f ? 180.0f : 0.0f;
        if (roll == 0.0f && (effect_tilt == 0.0f || glm::length(h) < 1e-3f)) { const float* g = &at<float>(pl, pl::mQuat); q[0] = g[0]; q[1] = g[1]; q[2] = g[2]; q[3] = g[3]; }
        else basis_quat(pl, dir, effect_tilt, roll, q);
        devil4_sdk::effect_stay(effect_list, effect_index, q, &at<float>(pl, pl::mPos), (void*)pl);   // the air hike's spawn + setEffect registration
    }
    if (se_enabled) devil4_sdk::play_player_se((void*)pl, se_id);   // the air hike's SE on the player's own sound resource
}

void face(uintptr_t pl, float yaw) {
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

glm::vec3 wmat_row(uintptr_t pl, int row) {           // 0 right, 1 up, 2 forward
    const float* r = &at<float>(pl, pl::mWmat + 0x10 * row);
    return glm::vec3(r[0], r[1], r[2]);
}

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }

bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

constexpr int PHASE_ENTRY   = 0;
constexpr int PHASE_WAIT    = 1;     // cycle playing, waiting for launch_frame
constexpr int PHASE_AIRBORNE = 2;

static int g_dbg_n0_at_move = -1, g_dbg_all_at_move = -1;   // mStat[0] / [7] as seen inside move (after updateCharTbl + the checks)

struct JumpState {                   // what the phase word can't hold
    uintptr_t player = 0;
    Dir dir = FORWARD;
    glm::vec3 dir_vec{0.0f};         // world direction of the jump (unit, level for the sideways ones)
    bool script_set = false;         // the cancel script has replaced the borrowed .ctn script
    bool vertical = false;           // entered through the vertical row (0x41)
    uint32_t last_seen = 0;          // state_for's tick when this player was last seen (eviction order)
    uint32_t shell_serial = 0;       // the jump's serial, counted up at each launch: the shell ends when it changes
};
JumpState g_jumps[kPlayerSlots];
int slot_of(const JumpState& s) { return (int)(&s - g_jumps); }
// One slot per uPlayer seen (the command hook asks for every uPlayer that ticks, not only the local player). Dead
// players are never reported, so a full table evicts the slot seen longest ago: a live player is seen every tick
// and keeps its slot. With a fixed "reset slot 0" two live players could evict each other every tick once two
// stale pointers held the table, wiping dir_vec mid-jump (the hop-in-place after a stage switch).
JumpState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (JumpState& s : g_jumps) if (s.player == player) { s.last_seen = tick; return s; }
    JumpState* victim = &g_jumps[0];
    for (JumpState& s : g_jumps) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = JumpState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

// First tick after the motion request: swap the borrowed script for the cancel script (if a cancel frame is set).
void arm_cancel_script(JumpState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    if (cancel_frame >= 0.0f) {
        ctn::Script& sc = g_cancel_scripts[slot_of(s) % kPlayerSlots];
        sc.reset().wait(cancel_frame).open_all().end();
        devil4_sdk::install_ctn_script((void*)pl, sc.data());
    }
}

// The gate every cancel goes through: uPlayer::main calls checkBeforeAction / checkAction (checkAtck, the jump
// and gun checks, pose) only while mIsCancelCheckEnable is set. setActionAtck cleared it when this move started;
// the vanilla attack functions set it back at their cancel frame, and so does this. Before that frame nothing can
// interrupt the move whatever the .ctn windows say; after it the windows decide which rows may.
void open_cancel_gate(uintptr_t pl) {
    if (cancel_frame >= 0.0f && at<float>(pl, pl::mMotionFrame0) >= cancel_frame)
        at<bool>(pl, pl::mIsCancelCheckEnable) = true;
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Direction. The vertical row: up on the ground, down in the air (AST_JUMP). The directional row: the stick
// against the facing (it can't be centred, ANYDIRECTION needs a lean; forward if it somehow is); within
// side_angle of the facing = forward, of its opposite = back; otherwise left / right by the sign against the
// right axis.
// ---------------------------------------------------------------------------------------------------------------
Dir pick_direction(uintptr_t pl, bool vertical) {
    if (vertical) return (at<uint32_t>(pl, pl::mActStat) & 0x2) ? DOWN : UP;
    if (at<int>(pl, pl::mAnlgPow) == 0) return FORWARD;
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    const glm::vec3 fwd   = wmat_row(pl, 2);
    const glm::vec3 right = wmat_row(pl, 0);
    const float along = glm::dot(stick, fwd);
    const float cosA  = std::cos(glm::radians(side_angle));
    if (along >=  cosA) return FORWARD;
    if (along <= -cosA) return BACK;
    return glm::dot(stick, right) >= 0.0f ? RIGHT : LEFT;
}

// The LMT goes through the motion registry (sdk/MotionRegistry.hpp), which picks a free bank and merges the file with
// the other mods' (gerb.xml names the same file, so both get the same blocks). A missing file is logged there.
bool ready(uintptr_t pl) {
    return pl && lmt_set >= 0 && motion_registry::ready(lmt_set, pl);
}

void register_lmt() {
    motion_registry::SetDesc d;
    d.owner = "Gerb";
    d.body[motion_registry::NERO] = lmt_path;
    const int old = lmt_set;
    lmt_set = lmt_path[0] ? motion_registry::add(d) : -1;
    if (old >= 0) motion_registry::remove(old);
}

// ---------------------------------------------------------------------------------------------------------------
// The move.
// ---------------------------------------------------------------------------------------------------------------
void __stdcall move(uintptr_t pl, uint32_t atck_id) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    JumpState& s = state_for(pl);

    switch (phase) {
    case PHASE_ENTRY: {
        if (!ready(pl)) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // no LMT: bail out

        s.vertical = atck_id == move_id_vertical;
        s.dir = pick_direction(pl, s.vertical);
        const int motion = motion_registry::word(lmt_set, pl, slot[s.dir]);
        if (motion < 0) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // slot past the file's blocks
        const glm::vec3 fwd = wmat_row(pl, 2), right = wmat_row(pl, 0);
        switch (s.dir) {
        case FORWARD: s.dir_vec = fwd;    break;
        case BACK:    s.dir_vec = -fwd;   break;
        case LEFT:    s.dir_vec = -right; break;
        case RIGHT:   s.dir_vec = right;  break;
        case UP:      s.dir_vec = glm::vec3(0.0f,  1.0f, 0.0f); break;
        case DOWN:    s.dir_vec = glm::vec3(0.0f, -1.0f, 0.0f); break;
        default: break;
        }
        if (face_direction && (s.dir == FORWARD || s.dir == LEFT || s.dir == RIGHT))   // a back jump keeps facing
            face(pl, yaw_of(s.dir_vec));

        devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        // the cycle is the whole jump: loop as its block says, root translation off
        request_motion(pl, motion, (uint32_t)A_NULL_TRANS_OFF, 4.0f, 0.0f, 1.0f);

        s.script_set = false;                                           // the cancel script goes in next tick
        // momentum is left alone until the launch frame writes the velocity (zeroing it here froze the player
        // whenever the entry ran again before the launch)
        phase = PHASE_WAIT;
        return;
    }
    case PHASE_WAIT: {
        acc.x = acc.y = acc.z = 0.0f;
        arm_cancel_script(s, pl);
        open_cancel_gate(pl);
        if (at<float>(pl, pl::mMotionFrame0) < launch_frame) return;

        glm::vec3 v;
        switch (s.dir) {
        case UP:   v = glm::vec3(0.0f,  up_speed,   0.0f); break;
        case DOWN: v = glm::vec3(0.0f, -down_speed, 0.0f); break;
        default:   v = s.dir_vec * speed + glm::vec3(0.0f, rise, 0.0f); break;
        }
        vel.x = v.x; vel.y = v.y; vel.z = v.z;
        // keep the game's own XZ model in step (mMoveVel / mJump.moveAng), no table deceleration, no additive speed
        const glm::vec3 xz(v.x, 0.0f, v.z);
        at<float>(pl, pl::mMoveVel) = glm::length(xz);
        at<float>(pl, pl::mMoveAcc) = 0.0f;
        if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = yaw_of(xz);
        clear_add_speed(pl);
        // From the take-off the action counts as a jump (AST_JUMP) so the game's air rules and the air rows of the
        // attack table apply; until then it keeps the status it was entered with (stand on the ground), so ground
        // rows can still cancel the wind-up.
        if (airborne_status) at<uint32_t>(pl, pl::mActStat) = (at<uint32_t>(pl, pl::mActStat) & ~0x1u) | 0x2u;
        take_off_fx(pl, s.dir_vec);
        phase = PHASE_AIRBORNE;
        if (shell_enabled) spawn_shell(pl, s.dir_vec, ++s.shell_serial);   // after the phase: the shell checks it
        return;
    }
    case PHASE_AIRBORNE: {
        g_dbg_n0_at_move  = at<int>(pl, pl::mMotCncl + 4);
        g_dbg_all_at_move = at<int>(pl, pl::mMotCncl + 0x2C * 7 + 4);
        arm_cancel_script(s, pl);
        open_cancel_gate(pl);
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        vel.y += gravity * at<float>(pl, pl::m_delta_time);
        if (vel.y < -max_fall) vel.y = -max_fall;
        acc.x = acc.y = acc.z = 0.0f;
        return;
    }
    default:
        return;
    }
}

// The shell's lifetime: the jump `serial` of `pl` is still the running attack row and still airborne.
bool jump_active(uintptr_t pl, uint32_t serial) {
    if (!pl) return false;
    const JumpState& s = state_for(pl);
    if (s.shell_serial != serial) return false;
    const uint32_t id = at<uint32_t>(pl, pl::mAtckId);
    if (id != move_id && id != move_id_vertical) return false;
    return at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_AIRBORNE;
}

// The row MoveTable::updateKDATbl inserted (index from the front = extra_nero_moves - MoveTable::Gerb, the same
// arithmetic LuciAirThrow uses). Byte 0 of the command word = the ATTACK_DIRECTION_CMD, byte 1 = 1 makes the row
// unmatchable (the table's convention for a disabled custom move). The style word (atckConditionStyle, -1 = any) is
// NeroStyle's: the Gerbera style's value while that mod gates Gerb, so the game's own row filter keeps the jump to it.
void apply_row() {
    if (!HookNeroKADTbl || !MoveTable::Gerb || !MoveTable::GerbVertical) return;
    kAtckDefTbl* row = (kAtckDefTbl*)HookNeroKADTbl + (MoveTable::extra_nero_moves - MoveTable::Gerb);
    row->command.buffer = (uint32_t)cmd_row[ROW_DIRECTIONAL] | (mod_enabled ? 0u : 0x100u);
    row->atckConditionStyle = nero_style::gerb_condition();
    kAtckDefTbl* vrow = (kAtckDefTbl*)HookNeroKADTbl + (MoveTable::extra_nero_moves - MoveTable::GerbVertical);
    vrow->command.buffer = (uint32_t)cmd_row[ROW_VERTICAL] | (mod_enabled ? 0u : 0x100u);
    vrow->atckConditionStyle = nero_style::gerb_condition();
}

// ---- The rows' inputs (Gerb.hpp): checked in the pre-check tick (after executeCommandCheck, before checkAtck reads
// mCmdFlag), the local player only (the pad / keyboard are its). A press with its conditions met sets the row's bit.
constexpr uintptr_t kPadBtnOn = 0x1374;          // mPeripheral.mPadBtnOn: the raw pad bits, before the button config maps them
constexpr uintptr_t kCmdFlagWords = 0x2184;      // mCmdFlag[3]
constexpr float     kBackForwardFrames = 15.0f;  // STICK_BACK_FORWARD: back -> forward, and forward -> the trigger, each within this
static bool     g_custom_held[2] = {};           // last tick's trigger state (the press edge of pad / key triggers)
static uint32_t g_custom_fired[2] = {};          // readout: times each row's input fired
// STICK_BACK_FORWARD runs like the game's command 0x52, Calibur's input (uPlayer::commandTbl 0xC3E328 + 0x52 * 16, run by
// uPlayer::commandCheckSub 0x805A10): stick centred (no time limit after it), back, forward within 15 frames of reaching
// back, the trigger within 15 frames of reaching forward. Each window starts when its step is reached and a window that
// runs out starts the input over (back held too long, too slow to the trigger); firing starts it over too. Frames are
// game frames (m_delta_time is subtracted every tick), so the windows don't depend on the tick rate.
enum BackForwardStep : int { BF_CENTRED = 0, BF_BACK = 1, BF_FORWARD = 2, BF_READY = 3 };
static int   g_bf_step = BF_CENTRED;             // the step being waited for (BF_READY: the trigger may fire)
static float g_bf_timer = 0.0f;                  // frames left in the current window, 0 = none

// The game's window when a step is reached: timer - 1 + this tick's delta for timers above 1 (commandCheckSub 0x805B96).
static float bf_window(float dt) { return kBackForwardFrames > 1.0f ? kBackForwardFrames - 1.0f + dt : kBackForwardFrames; }

static void back_forward_tick(uintptr_t pl, int now) {
    const float dt = at<float>(pl, pl::m_delta_time);
    if (g_bf_timer > 0.0f) {
        g_bf_timer -= dt;
        if (g_bf_timer <= 0.0f) { g_bf_step = BF_CENTRED; g_bf_timer = 0.0f; }   // too slow: start over
    }
    // steps reached this tick cascade, as the game checks the next step in the same call
    if (g_bf_step == BF_CENTRED && now == STICK_CENTRED) g_bf_step = BF_BACK;
    if (g_bf_step == BF_BACK && now == STICK_BACK)       { g_bf_step = BF_FORWARD; g_bf_timer = bf_window(dt); }
    if (g_bf_step == BF_FORWARD && now == STICK_FORWARD) { g_bf_step = BF_READY;   g_bf_timer = bf_window(dt); }
}
static int      g_capture = -1;                  // the row whose trigger is being captured (-1 none)
static uint32_t g_capture_bits = 0;              // pad capture: the buttons pressed together so far

static bool keyboard_allowed() { return !g_framework->m_draw_ui && GetForegroundWindow() == g_framework->get_window_handle(); }

// getLockOnTarget (vtbl 170, thiscall (this, a2, a3, exclude), retn 0Ch) as the game's lever check asks it: (0, 0, 0)
static uintptr_t lock_target(uintptr_t pl) {
    const uintptr_t fn = (*(uintptr_t**)pl)[170];
    return ((uintptr_t(__fastcall*)(uintptr_t, int, int, int, uintptr_t))fn)(pl, 0, 0, 0, 0);
}

// Where the stick leans: centred, or forward / back (within 45 deg) / left / right against the lock-on target's direction
// (TPS aim's crosshair point when it aims), else the facing.
static int stick_class(uintptr_t pl) {
    if (at<int>(pl, pl::mAnlgPow) == 0) return STICK_CENTRED;
    float ref = at<float>(pl, pl::mRotateNow);
    if (const uintptr_t t = lock_target(pl)) {
        const MtVector3& tp = at<MtVector3>(t, 0x10);
        const MtVector3& pp = at<MtVector3>(pl, pl::mPos);
        const float dx = tp.x - pp.x, dz = tp.z - pp.z;
        if (dx * dx + dz * dz > 1.0f) ref = std::atan2(dx, dz);
    }
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    const glm::vec3 fwd(std::sin(ref), 0.0f, std::cos(ref));
    const glm::vec3 right(-std::cos(ref), 0.0f, std::sin(ref));   // the game's yaw grows to the left
    const float along = glm::dot(stick, fwd), c45 = 0.70710678f;
    if (along >= c45)  return STICK_FORWARD;
    if (along <= -c45) return STICK_BACK;
    return glm::dot(stick, right) >= 0.0f ? STICK_RIGHT : STICK_LEFT;
}

static bool stick_ok(int now, int want) {
    switch (want) {
    case STICK_NONE:         return true;
    case STICK_ANY:          return now != STICK_CENTRED;
    case STICK_BACK_FORWARD: return g_bf_step == BF_READY;
    default:                 return now == want;
    }
}

void custom_tick(uintptr_t pl) {
    if (!mod_enabled || !pl || pl != (uintptr_t)devil4_sdk::get_local_player()) return;
    if (g_capture >= 0 && custom_input[g_capture].source == SRC_PAD) {   // capture: every button down together, done on release
        const uint32_t bits = at<uint32_t>(pl, kPadBtnOn);
        if (bits) g_capture_bits |= bits;
        else if (g_capture_bits) { custom_input[g_capture].bits = g_capture_bits; g_capture_bits = 0; g_capture = -1; }
        return;                                                          // nothing fires while capturing
    }
    const int now = stick_class(pl);
    back_forward_tick(pl, now);                                          // the back-forward motion
    for (int i = 0; i < 2; ++i) {
        const CustomInput& ci = custom_input[i];
        bool held = false, pressed = false;
        if (ci.bits) switch (ci.source) {
        case SRC_GAME:
            held = (at<uint32_t>(pl, pl::mBtnOn) & ci.bits) == ci.bits;
            pressed = held && (at<uint32_t>(pl, pl::mBtnTrg) & ci.bits) != 0;
            break;
        case SRC_PAD:
            held = (at<uint32_t>(pl, kPadBtnOn) & ci.bits) == ci.bits;
            pressed = held && !g_custom_held[i];
            break;
        case SRC_KEY:
            held = keyboard_allowed() && (GetAsyncKeyState((int)ci.bits) & 0x8000) != 0;
            pressed = held && !g_custom_held[i];
            break;
        default: break;
        }
        g_custom_held[i] = held;
        if (!pressed) continue;
        if (ci.lockon && !(at<uint32_t>(pl, pl::mBtnOn) & 0x10)) continue;
        if (!stick_ok(now, ci.stick)) continue;
        at<uint32_t>(pl, kCmdFlagWords + 4 * (cmd_row[i] >> 5)) |= 1u << (cmd_row[i] & 31);
        ++g_custom_fired[i];
        if (ci.stick == STICK_BACK_FORWARD) { g_bf_step = BF_CENTRED; g_bf_timer = 0.0f; }   // used up, as the game's
    }
}

static void key_name(uint32_t vk, char* out, int size) {
    const UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    if (!sc || !GetKeyNameTextA((LONG)(sc << 16), out, size)) snprintf(out, size, "VK 0x%02X", vk);
}

// A row's input: the trigger (with a capture for the pad / keyboard) on one line, the conditions on the next.
static void input_editor(int row, const char* title) {
    static const char* kSources[] = { "Game buttons", "Gamepad buttons", "Keyboard key" };
    static const char* kSticks[] = { "any stick", "stick centred", "forward", "back", "left", "right", "any direction", "back, forward" };
    static const struct { uint32_t bit; const char* name; } kButtons[] = { { 0x1, "melee" }, { 0x2, "gun" }, { 0x4, "jump" }, { 0x10, "lock-on" }, { 0x200, "style / Buster" } };
    CustomInput& ci = custom_input[row];
    ImGui::PushID(row);
    ImGui::TextUnformatted(title);
    ImGui::SameLine();
    ImGui::TextDisabled(ci.bits ? "(fired %u)" : "(no trigger: never fires)", g_custom_fired[row]);
    ImGui::SetNextItemWidth(Mod::sameLineItemWidth);
    if (ImGui::Combo("##source", &ci.source, kSources, 3)) { ci.bits = 0; if (g_capture == row) g_capture = -1; }
    ImGui::SameLine();
    if (ci.source == SRC_GAME) {
        ImGui::SetNextItemWidth(Mod::sameLineItemWidth);
        char preview[64] = "";
        for (const auto& b : kButtons) if (ci.bits & b.bit) { if (*preview) strcat_s(preview, " + "); strcat_s(preview, b.name); }
        if (ImGui::BeginCombo("##buttons", *preview ? preview : "(none)")) {   // tick the buttons; all held, one pressed
            for (const auto& b : kButtons) {
                bool on = (ci.bits & b.bit) != 0;
                if (ImGui::Checkbox(b.name, &on)) ci.bits = on ? (ci.bits | b.bit) : (ci.bits & ~b.bit);
            }
            ImGui::EndCombo();
        }
    } else if (ci.source == SRC_PAD) {
        if (g_capture == row) ImGui::TextUnformatted(_("press, then let go ..."));
        else ImGui::Text("0x%X", ci.bits);
        ImGui::SameLine();
        if (ImGui::Button(_("Capture"))) { g_capture = row; g_capture_bits = 0; }
    } else {
        if (g_capture == row) {   // the first key pressed (not the mouse buttons)
            ImGui::TextUnformatted(_("press a key ..."));
            for (int vk = 0x08; vk <= 0xFE; ++vk) {
                if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) continue;   // the left / right variants are reported
                if (GetAsyncKeyState(vk) & 0x8000) { ci.bits = (uint32_t)vk; g_capture = -1; break; }
            }
        } else {
            char name[64] = "(none)";
            if (ci.bits) key_name(ci.bits, name, sizeof(name));
            ImGui::TextUnformatted(name);
        }
        ImGui::SameLine();
        if (ImGui::Button(_("Capture"))) g_capture = row;
    }
    ImGui::Checkbox(_("Lock-on held"), &ci.lockon);
    ImGui::SameLine();
    int stick = ci.stick + 1;
    ImGui::SetNextItemWidth(Mod::sameLineItemWidth);
    if (ImGui::Combo("##stick", &stick, kSticks, 8)) ci.stick = stick - 1;
    ImGui::SameLine();
    Mod::help_marker(_("The trigger: game buttons (after your button config: the ticked ones held, one pressed), gamepad buttons "
                       "(the raw pad, before the button config: Capture, press the button or several together, let go) or a "
                       "keyboard key (Capture, press it; counts while the game is in front and this menu is closed). Optional: "
                       "lock-on held (TPS aim's counts) and where the stick leans, against the lock-on target (else the facing): "
                       "forward / back within 45 degrees, left / right between, or Calibur's back, forward (stick centred first, back, forward within 15 frames, the trigger within 15 more)."));
    ImGui::PopID();
}

} // namespace gerb

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only. Everything the move is lives in namespace gerb above.
// ---------------------------------------------------------------------------------------------------------------
using namespace gerb;

std::optional<std::string> Gerb::on_initialize() {
    using namespace input_commands;
    move_registry::register_move(move_registry::NERO, move_id, &mod_enabled, &gerb::move);            // the ID gate + Nero dispatch
    move_registry::register_move(move_registry::NERO, move_id_vertical, &mod_enabled, &gerb::move);
    for (int i = 0; i < 2; ++i) add(g_reserved_cmd[i].head(), cmd_row[i], &kNever);   // bits 85 / 86 reserved
    add_tick(&custom_tick);                   // the rows' inputs set them
    if (!input_commands::hook_installed) {     // one installer: whichever of Gerb / WallRun / MoveDef initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("Gerb: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init Gerb (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void Gerb::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;        // debug page
    if (ImGui::Checkbox(_("Gerb"), &mod_enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("A free jump in any direction, no wall: forward / back / left / right by the stick (in the air only), "
                  "straight up from the ground or straight down in the air on the vertical input. Needs motion\\pl000\\gerb.lmt "
                  "(slots 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up) and the two MoveTable rows (0x40, 0x41)."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("Gerb");            // WallRun's panel shares this page and some labels (Bank, Gravity, Slot ...)
    input_editor(ROW_DIRECTIONAL, _("Directional input (air): forward / back / left / right by the stick"));
    input_editor(ROW_VERTICAL, _("Vertical input: up on the ground, down in the air"));
    if (ImGui::InputText(_("LMT"), lmt_path, sizeof(lmt_path), ImGuiInputTextFlags_EnterReturnsTrue)) register_lmt();
    ImGui::SameLine();
    help_marker(_("Game path of the LMT under nativePC, without .lmt; Enter applies it. The hook places it in a free motion "
                  "bank itself (shared with the other features that add animations)."));
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | %s"), lmt_file_exists(lmt_path) ? "found" : "missing",
                    motion_registry::describe(lmt_set, (uintptr_t)pl).c_str());
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();   // read again from disk on the next jump
    }
    ImGui::InputInt(_("Slot forward"), &slot[gerb::FORWARD]);
    ImGui::InputInt(_("Slot down"), &slot[gerb::DOWN]);
    ImGui::InputInt(_("Slot back"), &slot[gerb::BACK]);
    ImGui::InputInt(_("Slot left"), &slot[gerb::LEFT]);
    ImGui::InputInt(_("Slot right"), &slot[gerb::RIGHT]);
    ImGui::InputInt(_("Slot up"), &slot[gerb::UP]);
    ImGui::InputInt(_("Borrowed .ctn bank"), &ctn_bank);
    ImGui::InputInt(_("Borrowed .ctn slot"), &ctn_slot);
    ImGui::SliderFloat(_("Launch frame"), &launch_frame, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Speed"), &speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Rise"), &rise, -20.0f, 40.0f);
    ImGui::SliderFloat(_("Up speed"), &up_speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Down speed"), &down_speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Gravity"), &gravity, -5.0f, 0.0f);
    ImGui::SliderFloat(_("Max fall"), &max_fall, 1.0f, 80.0f);
    ImGui::SliderFloat(_("Cancel frame"), &cancel_frame, -1.0f, 60.0f);
    ImGui::SameLine();
    help_marker(_("From this frame of the cycle every cancel window (melee, command, jump, taunt, gun, all) is open, so attacks and "
                  "jumps can interrupt the jump: a .ctn script made in memory replaces the borrowed one. -1 = never (the borrowed "
                  "script's windows apply)."));
    ImGui::SliderFloat(_("Side angle"), &side_angle, 10.0f, 80.0f);
    ImGui::Checkbox(_("Take-off effect"), &effect_enabled);
    ImGui::SameLine();
    help_marker(_("The air hike's effect at the launch frame, spawned the way uPlayerNero::setWJumpEffect does (list / index of "
                  "the game's effect table, at the player's feet). Nero's air hike is list 1 index 0x28 (ec000_00v2); Dante's "
                  "is 2 / 0x14 (ec005_20v1)."));
    if (effect_enabled) {
        ImGui::InputInt(_("Effect list"), &effect_list);
        ImGui::InputInt(_("Effect index"), &effect_index);
        ImGui::SliderFloat(_("Effect tilt"), &effect_tilt, -60.0f, 60.0f);
        ImGui::SameLine();
        help_marker(_("Degrees the effect leans from the player's up toward the direction of the jump (its level part, so the "
                      "up / down jumps never lean). 0 = the player's own orientation, as the air hike has it."));
    }
    ImGui::Checkbox(_("Take-off sound"), &se_enabled);
    ImGui::SameLine();
    help_marker(_("The air hike's sound at the launch frame: an SE id of the player's own sound resource. The air hike plays 40."));
    if (se_enabled) ImGui::InputInt(_("SE id"), &se_id);
    if (uPlayer* pl = devil4_sdk::get_local_player()) {      // live readout for debugging the cancel window
        const uintptr_t P = (uintptr_t)pl;
        ImGui::Text("actLimit %04X  actStat %02X  frame %.1f  phase %d  motion ctrl0 %04X ctrl1 %04X (half used: %s)",
                    at<uint32_t>(P, pl::mActLimit), at<uint32_t>(P, pl::mActStat), at<float>(P, pl::mMotionFrame0),
                    at<int>(P, pl::mBdyActionLowerPhase), at<uint32_t>(P, pl::mMotionCtrl0No) & 0xFFFF,
                    at<uint32_t>(P, pl::mMotionCtrl0No + 0x3C) & 0xFFFF,
                    (at<uint32_t>(P, pl::mMotionCtrl0No + 0x3C) & 0xFFFF) == 0xFFFF ? "lower = ctn[1]" : "upper = ctn[0]");
        ImGui::Text("inside move(): N0 %d ALL %d | cancel checks enabled: %d", g_dbg_n0_at_move, g_dbg_all_at_move, (int)at<bool>(P, pl::mIsCancelCheckEnable));
        {   // the quaternion conversion check: both rows should match (tilt 0 = the player's basis as is)
            float q[4]; basis_quat(P, glm::vec3(0.0f), 0.0f, 0.0f, q);
            const float* g = &at<float>(P, pl::mQuat);
            ImGui::Text("quat from wmat %.3f %.3f %.3f %.3f | mQuat %.3f %.3f %.3f %.3f", q[0], q[1], q[2], q[3], g[0], g[1], g[2], g[3]);
        }
        shell_debug_readout(P);
        ImGui::Text("mStat N0 %d N1 %d N2 %d CMD %d JMP %d TNT %d GUN %d ALL %d",
                    at<int>(P, pl::mMotCncl + 0x2C * 0 + 4), at<int>(P, pl::mMotCncl + 0x2C * 1 + 4), at<int>(P, pl::mMotCncl + 0x2C * 2 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 3 + 4), at<int>(P, pl::mMotCncl + 0x2C * 4 + 4), at<int>(P, pl::mMotCncl + 0x2C * 5 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 6 + 4), at<int>(P, pl::mMotCncl + 0x2C * 7 + 4));
        for (int half = 0; half < 2; ++half) {
            const uintptr_t mgr = P + pl::mCharTblMgr + 0x24 * half;
            const uint8_t* top = at<uint8_t*>(mgr, 0x0C); const uint8_t* cur = at<uint8_t*>(mgr, 0x10);
            const uint8_t* c1 = &at<uint8_t>(mgr, 0x15 + 6);
            ImGui::Text("ctn[%d] top %p cur +%d done %d ours %d  contact1 %02X %02X %02X %02X %02X %02X", half, top, cur && top ? (int)(cur - top) : -1,
                        (int)at<bool>(mgr, 0x14), (int)(top >= g_cancel_scripts[0].bytes && top < g_cancel_scripts[kPlayerSlots - 1].bytes + ctn::Script::kCapacity),
                        c1[0], c1[1], c1[2], c1[3], c1[4], c1[5]);
        }
    }
    ImGui::Checkbox(_("Face the direction"), &face_direction);
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Count as airborne"), &airborne_status);
    ImGui::Separator();
    shell_gui();
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void Gerb::on_config_load(const utility::Config& cfg) {
    mod_enabled  = cfg.get<bool>("gerb").value_or(false);
    for (int i = 0; i < DIR_COUNT; ++i) slot[i] = cfg.get<int>("gerb_slot_" + std::to_string(i)).value_or(slot[i]);
    ctn_bank     = cfg.get<int>("gerb_ctn_bank").value_or(ctn_bank);
    ctn_slot     = cfg.get<int>("gerb_ctn_slot").value_or(ctn_slot);
    launch_frame = cfg.get<float>("gerb_launch_frame").value_or(launch_frame);
    speed        = cfg.get<float>("gerb_speed").value_or(speed);
    rise         = cfg.get<float>("gerb_rise").value_or(rise);
    up_speed     = cfg.get<float>("gerb_up_speed").value_or(up_speed);
    down_speed   = cfg.get<float>("gerb_down_speed").value_or(down_speed);
    gravity      = cfg.get<float>("gerb_gravity").value_or(gravity);
    max_fall     = cfg.get<float>("gerb_max_fall").value_or(max_fall);
    cancel_frame = cfg.get<float>("gerb_cancel_frame").value_or(cancel_frame);
    side_angle   = cfg.get<float>("gerb_side_angle").value_or(side_angle);
    face_direction  = cfg.get<bool>("gerb_face_direction").value_or(true);
    airborne_status = cfg.get<bool>("gerb_airborne_status").value_or(true);
    effect_enabled  = cfg.get<bool>("gerb_effect").value_or(true);
    effect_list     = cfg.get<int>("gerb_effect_list").value_or(effect_list);
    effect_index    = cfg.get<int>("gerb_effect_index").value_or(effect_index);
    effect_tilt     = cfg.get<float>("gerb_effect_tilt").value_or(effect_tilt);
    se_enabled      = cfg.get<bool>("gerb_se").value_or(true);
    se_id           = cfg.get<int>("gerb_se_id").value_or(se_id);
    for (int i = 0; i < 2; ++i) {           // the rows' inputs: gerb_in<row>_source / _bits / _lockon / _stick
        CustomInput& ci = custom_input[i];
        const std::string p = "gerb_in" + std::to_string(i) + "_";
        ci.source = cfg.get<int>(p + "source").value_or(ci.source);
        ci.bits   = (uint32_t)cfg.get<int>(p + "bits").value_or((int)ci.bits);
        ci.lockon = cfg.get<bool>(p + "lockon").value_or(ci.lockon);
        ci.stick  = cfg.get<int>(p + "stick").value_or(ci.stick);
    }
    apply_row();
    shell_config_load(cfg);
    register_lmt();
}

void Gerb::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gerb", mod_enabled);
    for (int i = 0; i < DIR_COUNT; ++i) cfg.set<int>("gerb_slot_" + std::to_string(i), slot[i]);
    cfg.set<int>("gerb_ctn_bank", ctn_bank);
    cfg.set<int>("gerb_ctn_slot", ctn_slot);
    cfg.set<float>("gerb_launch_frame", launch_frame);
    cfg.set<float>("gerb_speed", speed);
    cfg.set<float>("gerb_rise", rise);
    cfg.set<float>("gerb_up_speed", up_speed);
    cfg.set<float>("gerb_down_speed", down_speed);
    cfg.set<float>("gerb_gravity", gravity);
    cfg.set<float>("gerb_max_fall", max_fall);
    cfg.set<float>("gerb_cancel_frame", cancel_frame);
    cfg.set<float>("gerb_side_angle", side_angle);
    cfg.set<bool>("gerb_face_direction", face_direction);
    cfg.set<bool>("gerb_airborne_status", airborne_status);
    cfg.set<bool>("gerb_effect", effect_enabled);
    cfg.set<int>("gerb_effect_list", effect_list);
    cfg.set<int>("gerb_effect_index", effect_index);
    cfg.set<float>("gerb_effect_tilt", effect_tilt);
    cfg.set<bool>("gerb_se", se_enabled);
    cfg.set<int>("gerb_se_id", se_id);
    for (int i = 0; i < 2; ++i) {
        const CustomInput& ci = custom_input[i];
        const std::string p = "gerb_in" + std::to_string(i) + "_";
        cfg.set<int>(p + "source", ci.source);
        cfg.set<int>(p + "bits", (int)ci.bits);
        cfg.set<bool>(p + "lockon", ci.lockon);
        cfg.set<int>(p + "stick", ci.stick);
    }
    shell_config_save(cfg);
}
