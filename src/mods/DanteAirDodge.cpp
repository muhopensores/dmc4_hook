#include "DanteAirDodge.hpp"
#include "../sdk/BossMoves.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MotionRegistry.hpp"
#include "../sdk/CtnScript.hpp"
#include "../sdk/Rollback.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace dante_air_dodge {

bool  mod_enabled     = false;
int   cmd_bit         = -1;
uintptr_t jmp_ret_wjump = 0;
int   lmt_set         = -1;

// ---------------------------------------------------------------------------------------------------------------
// Tuning (cm and frames at 60 fps).
// ---------------------------------------------------------------------------------------------------------------
namespace {

constexpr const char* lmt_path  = "motion\\pl006\\airdodge";   // game path under nativePC, without .lmt
constexpr int   slot[DIR_COUNT] = { 1, 0 };   // block per direction: right, left
constexpr int   ctn_bank        = 0;          // the vanilla .ctn script borrowed for the cycle (runtime banks have none):
constexpr int   ctn_slot        = 5;          // the fall motion's, so no cancels until the game's air rules say so
constexpr float launch_frame    = 0.0f;       // frame of the cycle at which the velocity is applied
constexpr float speed           = 16.0f;      // sideways
constexpr float rise            = 16.0f;      // vertical speed at the launch
constexpr float gravity         = -1.1f;      // added to the vertical speed every frame
constexpr float max_fall        = 30.0f;
constexpr float cancel_frame    = 10.0f;      // from this frame of the cycle every cancel window is open
constexpr float side_angle      = 50.0f;      // degrees off the right / left axis within which the stick counts as sideways
constexpr bool  face_direction  = false;      // keep facing the target
constexpr bool  airborne_status = true;       // mark the action as a jump (AST_JUMP) so the game's air rules apply
constexpr bool  enemy_step_from_start = true; // the enemy step (jump cancel) can interrupt the dodge from frame 0
constexpr bool  cost_air_hike   = true;       // the dodge is the air jump: no air hike after it (checkWJumpBegin: mWJumpCnt >= wJumpCnt)
constexpr int   dodges_per_air  = 0;          // 0: the game's air-jump count is the only limit (2 in DT), since the dodge spends one
constexpr int   effect_list     = 2;          // Dante's air hike effect (uPlayerDante::setWJumpEffect 0x7B9220): effect\efl\com\ec005_20v1
constexpr int   effect_index    = 0x14;
constexpr float effect_tilt     = 45.0f;      // degrees the effect leans from the player's up toward the dodge
constexpr int   se_id           = 40;         // the air hike's sound

// The end, Sky Star's (moveActTsAirDash): when the cycle ends, the fall motion (4, attr 0x15, 30-frame blend from frame
// 51) and a sideways speed that drains by param +0x130 a frame until it is under 1, then stops.
constexpr float end_inertia     = 7.5f;       // sideways speed the fall starts with (Sky Star's own is 1.7-8 by its frame)
constexpr float end_stop        = 1.0f;
constexpr float end_drain       = -0.39f;     // pl006_param.plp +0x130, used when the live table can't be read
constexpr uintptr_t kParamTbl   = 0x1EA8;     // uPlayer: the .plp parameter data Sky Star reads
constexpr uintptr_t kEndDrain   = 0x130;
constexpr int   kFallMotion     = 4;
constexpr uint32_t kFallAttr    = 0x15;
constexpr float kFallBlend      = 30.0f;
constexpr float kFallStart      = 51.0f;

// ---------------------------------------------------------------------------------------------------------------
// Game functions and helpers (Gerb's, kept file-local the same way).
// ---------------------------------------------------------------------------------------------------------------
static uintptr_t fn_setActionLand = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed_) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer_old*)pl, attr, speed_, start, interp);
}

void clear_add_speed(uintptr_t pl) {
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
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

float end_deceleration(uintptr_t pl) {
    const uintptr_t tbl = at<uintptr_t>(pl, kParamTbl);
    const float a = tbl ? at<float>(tbl, kEndDrain) : 0.0f;
    return a < 0.0f ? a : end_drain;
}

// The player's orientation with its up leaned `tilt_deg` toward the level part of `dir` (mt_basis_quat, MtMath.hpp).
void basis_quat(uintptr_t pl, const glm::vec3& dir, float tilt_deg, float out[4]) {
    glm::vec3 b[3] = { wmat_row(pl, 0), wmat_row(pl, 1), wmat_row(pl, 2) };
    const glm::vec3 h(dir.x, 0.0f, dir.z);
    if (tilt_deg != 0.0f && glm::length(h) > 1e-3f) {
        const glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(h)));
        for (glm::vec3& v : b) v = mt_rodrigues(v, axis, tilt_deg);
    }
    mt_basis_quat(b[0], b[1], b[2], out);
}

// Dante's air hike effect and sound at the take-off, the effect leaned toward the dodge.
void take_off_fx(uintptr_t pl, const glm::vec3& dir) {
    float q[4];
    basis_quat(pl, dir, effect_tilt, q);
    devil4_sdk::effect_stay(effect_list, effect_index, q, &at<float>(pl, pl::mPos), (void*)pl);
    devil4_sdk::play_player_se((void*)pl, se_id);
}

constexpr int PHASE_ENTRY    = 0;
constexpr int PHASE_WAIT     = 1;     // cycle playing, waiting for launch_frame
constexpr int PHASE_AIRBORNE = 2;
constexpr int PHASE_FALL     = 3;     // the cycle ended: the fall motion, the sideways speed draining

constexpr int kPlayerSlots = 8;       // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)
static ctn::Script g_cancel_scripts[kPlayerSlots];   // the in-memory .ctn script per player slot (MoveDef D11, sdk/CtnScript.hpp)

struct DodgeState {                   // what the phase word can't hold
    uintptr_t player = 0;
    glm::vec3 dir_vec{0.0f};          // world direction of the dodge (unit, level)
    bool script_set = false;          // the cancel script has replaced the borrowed .ctn script
    bool jump_held = false;           // the jump button hasn't been released since the press that started the dodge
    int  wait_ticks = 0;              // ticks spent in PHASE_WAIT (watchdog: a motion that never advances)
    int  stuck_ticks = 0;             // airborne ticks with the motion frame not moving (same watchdog)
    float last_frame = -1.0f;
    Dir  dir = RIGHT;                 // the LMT slot in play
    int  used = 0;                    // dodges since the last landing
    uint32_t last_seen = 0;           // state_for's tick when this player was last seen (eviction order)
};
DodgeState g_dodges[kPlayerSlots];
int slot_of(const DodgeState& s) { return (int)(&s - g_dodges); }

// One slot per uPlayer seen; a full table evicts the one seen longest ago (dead players are never reported, a live
// one is seen every tick and keeps its slot).
DodgeState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (DodgeState& s : g_dodges) if (s.player == player) { s.last_seen = tick; return s; }
    DodgeState* victim = &g_dodges[0];
    for (DodgeState& s : g_dodges) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = DodgeState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

// First tick after the motion request: swap the borrowed script for the cancel script. (Opening C_JUMP from frame 0
// would let the dodge be cancelled into an air hike at once when one is left (DT); the enemy step needs no window:
// see check_enemy_step.)
void arm_cancel_script(DodgeState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    ctn::Script& sc = g_cancel_scripts[slot_of(s) % kPlayerSlots];
    sc.reset().wait(cancel_frame).open_all().end();
    devil4_sdk::install_ctn_script((void*)pl, sc.data());
}

// The cancel gate (mIsCancelCheckEnable): cleared by setActionAtck, set back at the cancel frame as the vanilla attacks do.
bool gate_open(uintptr_t pl) { return at<float>(pl, pl::mMotionFrame0) >= cancel_frame; }
void open_cancel_gate(uintptr_t pl) {
    if (gate_open(pl)) at<bool>(pl, pl::mIsCancelCheckEnable) = true;
}

// The press that started the dodge outlives the tick (the game ticks faster than the pad reports: mBtnTrg still
// carries it one tick later, and uPlayer::updateMotCancel latches it as the C_JUMP reserve). Until the button has been
// released once, the dodge eats the trigger bit and that latch, so no jump check takes the same press.
void eat_held_jump(DodgeState& s, uintptr_t pl) {
    if (!s.jump_held) return;
    if (!(at<uint32_t>(pl, pl::mBtnOn) & 0x4)) { s.jump_held = false; return; }
    at<uint32_t>(pl, pl::mBtnTrg) &= ~0x4u;
    at<bool>(pl, pl::mMotCnclJumpReserve) = false;
}

// The jump cancel (enemy step) from frame 0: uPlayer::checkEmJumpBegin 0x804970 reads no cancel window, only the gate
// (it is called from uPlayer::checkAction, which main runs while mIsCancelCheckEnable is set), a jump press, the
// enemy-step upgrade and an enemy to step on; on success it starts action 25 itself. While the gate is still off the
// move runs that one check, so the enemy step is available from the first frame and nothing else is.
static uintptr_t fn_checkEmJumpBegin = 0x00804970;    // __usercall, this@edi, result@al
bool check_enemy_step(uintptr_t pl) {
    uint8_t r = 0;
    uintptr_t fn = fn_checkEmJumpBegin;
    __asm {
        pushad
        mov edi, pl
        mov eax, fn
        call eax
        mov byte ptr [r], al
        popad
    }
    return r != 0;
}

// The cycle's end, as Sky Star's: the fall motion, the sideways speed (at most end_inertia) moved into mMoveVel along
// the dodge, draining by the game's Sky Star rate. mMoveVel / mJumpMoveAng stay in step, so a cancel into a move that
// carries the game's inertia carries this.
void start_fall(uintptr_t pl, DodgeState& s) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    const float now = std::sqrt(vel.x * vel.x + vel.z * vel.z);
    request_motion(pl, kFallMotion, kFallAttr, kFallBlend, kFallStart, 1.0f);
    at<float>(pl, pl::mMoveVel) = std::min(end_inertia, now);
    at<float>(pl, pl::mMoveAcc) = end_deceleration(pl);
    at<float>(pl, pl::mJumpMoveAng) = yaw_of(s.dir_vec);
}

// ---- the command: lock-on held + jump pressed, while the stick leans sideways (the predicate) ----------------
static input_commands::Command g_dodge_cmd;   // lock-on held (CND_POSE) + jump pressed; built in on_initialize

static int g_suppressed = 0;          // readout: air hikes the hook turned into dodge presses
static int g_bit_seen = 0;            // readout: ticks the command bit was set (counted from on_frame)
static int g_moves = 0;               // readout: entries into the move

// The air hike's check, suppressed when this press is the dodge's: the hook returns 0 from
// uPlayerDante::checkWJumpBegin so checkAtck, which runs after it, can match the row instead.
bool __stdcall suppress_air_hike(uintptr_t pl) {
    if (!mod_enabled || !pl || cmd_bit < 0) return false;          // no command: the row can't match, leave the air hike
    if (!(at<uint32_t>(pl, pl::mBtnOn) & 0x10)) return false;     // lock-on
    if (!(at<uint32_t>(pl, pl::mBtnTrg) & 0x4)) return false;     // jump pressed this tick
    const bool yes = can_dodge(pl);
    if (yes) {
        ++g_suppressed;
        // uPlayer::updateMotCancel (top of main) has already latched this press as a jump-cancel buffer
        // (mMotCncl[C_JUMP].mIsReserve) when the window was non-zero; the jump checks take that latch on later ticks
        // without a press, which this hook can't see. The press is the dodge's, so drop the latch too. (A new motion
        // clears every window anyway: uPlayer::updateMotion -> initMotCancel.)
        at<bool>(pl, pl::mMotCnclJumpReserve) = false;
    }
    return yes;
}

static bool s_suppress = false;

// 0x7B8E30 uPlayerDante::checkWJumpBegin (thiscall, this in ecx, no stack arguments): 6 prologue bytes
// `push ebp; mov ebp, esp; and esp, -8` replaced; the result is a char in al.
naked void detour_wjump_check() {
    _asm {
        pushad
        push ecx                        // this
        call suppress_air_hike          // __stdcall: pops its argument, result in al
        mov byte ptr [s_suppress], al
        popad
        cmp byte ptr [s_suppress], 0
        je original
        xor al, al                      // no air hike this tick
        ret
    original:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF8
        jmp dword ptr [jmp_ret_wjump]
    }
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// The predicate, the LMT and the move.
// ---------------------------------------------------------------------------------------------------------------
bool stick_sideways(uintptr_t pl) {
    if (at<int>(pl, pl::mAnlgPow) == 0) return false;
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    const float across = glm::dot(stick, wmat_row(pl, 0));
    return std::fabs(across) >= std::cos(glm::radians(side_angle));
}

// The count resets whenever the player stands (AST_STAND); with dodges_per_air set, a spent dodge leaves the next lock-on
// jump press to the air hike instead of chaining another dodge.
bool can_dodge(uintptr_t pl) {
    DodgeState& s = state_for(pl);
    if (at<uint32_t>(pl, pl::mActStat) & 0x1) s.used = 0;
    if (dodges_per_air > 0 && s.used >= dodges_per_air) return false;
    if (cost_air_hike) {              // the dodge and the air hike share the air jump: none left = no dodge either
        const uintptr_t tbl = at<uintptr_t>(pl, pl::mpActDataTbl);   // uPlayer::kActionData: +0 wJumpCnt, +1 wallJumpCnt
        if (tbl && at<char>(pl, pl::mWJumpCnt) >= at<char>(tbl, 0)) return false;
    }
    return stick_sideways(pl);
}

// The LMT goes through the motion registry (sdk/MotionRegistry.hpp), which picks a free bank (dante_air_dodge.xml
// names the same file, so both get the same blocks). A missing file is logged there.
bool ready(uintptr_t pl) {
    return pl && lmt_set >= 0 && motion_registry::ready(lmt_set, pl);
}

void register_lmt() {
    motion_registry::SetDesc d;
    d.owner = "DanteAirDodge";
    d.body[motion_registry::DANTE] = lmt_path;
    const int old = lmt_set;
    lmt_set = motion_registry::add(d);
    if (old >= 0) motion_registry::remove(old);
}

void __stdcall move(uintptr_t pl) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    DodgeState& s = state_for(pl);

    switch (phase) {
    case PHASE_ENTRY: {
        ++g_moves;
        ++s.used;
        if (!ready(pl)) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }   // no LMT: bail out

        const float yaw = at<float>(pl, pl::mAnlgRadian);
        const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
        const glm::vec3 right = wmat_row(pl, 0);
        const Dir dir = glm::dot(stick, right) >= 0.0f ? RIGHT : LEFT;
        s.dir_vec = dir == RIGHT ? right : -right;
        s.dir = dir;
        const int motion = motion_registry::word(lmt_set, pl, slot[dir]);
        if (motion < 0) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }   // slot past the file's blocks
        if (face_direction) face(pl, yaw_of(s.dir_vec));
        if (cost_air_hike) ++at<char>(pl, pl::mWJumpCnt);              // one air jump; Devil Trigger keeps its second

        devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        // the cycle is the whole dodge: loop as its block says, root translation off
        request_motion(pl, motion, (uint32_t)A_NULL_TRANS_OFF, 4.0f, 0.0f, 1.0f);
        s.script_set = false;                                           // the cancel script goes in next tick
        s.jump_held = true;                                             // this press is spent: see eat_held_jump
        s.wait_ticks = 0; s.stuck_ticks = 0; s.last_frame = -1.0f;
        spdlog::info("DanteAirDodge: start, motion {:04X} (bank {} slot {}), player {:#x}, frame {:.1f} of motion {:04X}",
                     motion, motion >> 8, slot[dir], pl, at<float>(pl, pl::mMotionFrame0), at<uint32_t>(pl, pl::mMotionCtrl0No));
        phase = PHASE_WAIT;                                             // momentum is left alone until the launch
        return;
    }
    case PHASE_WAIT: {
        acc.x = acc.y = acc.z = 0.0f;
        eat_held_jump(s, pl);
        arm_cancel_script(s, pl);
        open_cancel_gate(pl);
        if (enemy_step_from_start && !gate_open(pl) && check_enemy_step(pl)) return;   // stepped: action 25 took over
        if (at<float>(pl, pl::mMotionFrame0) < launch_frame) {
            // Watchdog: a block that is missing / empty, or a bank something else overwrote, never reaches the launch
            // frame, and the player would hang here with no animation. Report it and leave the move.
            if (++s.wait_ticks > 60) {
                spdlog::warn("DanteAirDodge: motion {:04X} did not reach the launch frame {} in 60 ticks (frame {:.1f}, controller plays {:04X}): "
                             "is {}.lmt block {} present and not empty ({})?",
                             motion_registry::word(lmt_set, pl, slot[s.dir]), launch_frame,
                             at<float>(pl, pl::mMotionFrame0), at<uint32_t>(pl, pl::mMotionCtrl0No), lmt_path, slot[s.dir],
                             motion_registry::describe(lmt_set, pl));
                devil4_sdk::neutral_air_transition((uPlayer_old*)pl);
            }
            return;
        }

        const glm::vec3 v = s.dir_vec * speed + glm::vec3(0.0f, rise, 0.0f);
        vel.x = v.x; vel.y = v.y; vel.z = v.z;
        // keep the game's own XZ model in step (mMoveVel / mJump.moveAng), no table deceleration, no additive speed
        const glm::vec3 xz(v.x, 0.0f, v.z);
        at<float>(pl, pl::mMoveVel) = glm::length(xz);
        at<float>(pl, pl::mMoveAcc) = 0.0f;
        if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = yaw_of(xz);
        clear_add_speed(pl);
        if (airborne_status) at<uint32_t>(pl, pl::mActStat) = (at<uint32_t>(pl, pl::mActStat) & ~0x1u) | 0x2u;   // AST_JUMP
        take_off_fx(pl, s.dir_vec);
        phase = PHASE_AIRBORNE;
        return;
    }
    case PHASE_AIRBORNE: {
        eat_held_jump(s, pl);
        arm_cancel_script(s, pl);
        open_cancel_gate(pl);
        if (enemy_step_from_start && !gate_open(pl) && check_enemy_step(pl)) return;
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        const float frame = at<float>(pl, pl::mMotionFrame0);
        if (frame >= 2.0f && at<bool>(pl, pl::mMotionFrameEnd0)) {      // the cycle is over (a fresh request still shows the old end)
            start_fall(pl, s);
            phase = PHASE_FALL;
            return;
        }
        // Watchdog: the exits need the motion to play (its end, the landing, the cancels after cancel_frame). A frame
        // that doesn't move for 60 ticks means it isn't: report and leave, instead of a soft lock.
        s.stuck_ticks = (frame == s.last_frame) ? s.stuck_ticks + 1 : 0;
        s.last_frame = frame;
        if (s.stuck_ticks > 60) {
            spdlog::warn("DanteAirDodge: motion frame stuck at {:.1f} for 60 ticks airborne (controller plays {:04X}, wanted {:04X}): "
                         "is {}.lmt block {} present and not empty ({})?",
                         frame, at<uint32_t>(pl, pl::mMotionCtrl0No), motion_registry::word(lmt_set, pl, slot[s.dir]), lmt_path, slot[s.dir],
                         motion_registry::describe(lmt_set, pl));
            devil4_sdk::neutral_air_transition((uPlayer_old*)pl);
            return;
        }
        vel.y += gravity * at<float>(pl, pl::m_delta_time);
        if (vel.y < -max_fall) vel.y = -max_fall;
        acc.x = acc.y = acc.z = 0.0f;
        return;
    }
    case PHASE_FALL: {                                                  // Sky Star's phase 4
        eat_held_jump(s, pl);
        open_cancel_gate(pl);
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        float& inertia = at<float>(pl, pl::mMoveVel);
        float& drain = at<float>(pl, pl::mMoveAcc);
        if (inertia < end_stop) { inertia = 0.0f; drain = 0.0f; }
        const float dt = at<float>(pl, pl::m_delta_time);
        vel.x = s.dir_vec.x * inertia;
        vel.z = s.dir_vec.z * inertia;
        inertia += drain * dt;
        vel.y += gravity * dt;
        if (vel.y < -max_fall) vel.y = -max_fall;
        acc.x = acc.y = acc.z = 0.0f;
        return;
    }
    default:
        return;
    }
}

// The row MoveTable::updateKDATbl inserted (index from the front = extra_dante_moves - MoveTable::DanteAirDodge).
// Byte 0 of the command word = the CMD_ID (our bit), byte 1 = 1 makes the row unmatchable (disabled).
void apply_row() {
    if (!HookDanteKADTbl || !MoveTable::DanteAirDodge) return;
    kAtckDefTbl* row = (kAtckDefTbl*)HookDanteKADTbl + (MoveTable::extra_dante_moves - MoveTable::DanteAirDodge);
    row->command.buffer = (uint32_t)(cmd_bit & 0xFF) | (mod_enabled && cmd_bit >= 0 ? 0u : 0x100u);
}

} // namespace dante_air_dodge

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only.
// ---------------------------------------------------------------------------------------------------------------
using namespace dante_air_dodge;

// the registry's state-function shape (player, atckId): the dodge has one row
static void __stdcall dodge_move(uintptr_t player, uint32_t) { dante_air_dodge::move(player); }

// The boss Dante's air dodge (sdk/BossMoves.hpp): sidesteps in the air. The command is lock-on held + jump with the stick
// within side_angle of his facing's right axis (he faces his target), an air jump left; one move per side.
static void register_boss_move() {
    using namespace boss_moves;
    Move m;
    m.name = "Air dodge (left)";
    m.enabled = &dante_air_dodge::mod_enabled;
    m.input(BTN_LOCK_ON, STICK_LEFT, 3).input(BTN_LOCK_ON | BTN_JUMP, STICK_LEFT, 1);
    m.atck_id = dante_air_dodge::move_id;
    m.start_timeout = 20.0f;
    m.where = WHERE_AIR;
    m.max_dist = 900.0f;
    m.stands_in = bit(ACTION_AIR_ESC) | bit(ACTION_W_JUMP);
    m.chance = 0.4f; m.cooldown = 180.0f;
    add(m);

    Move r = m;
    r.name = "Air dodge (right)";
    r.num_inputs = 0;
    r.input(BTN_LOCK_ON, STICK_RIGHT, 3).input(BTN_LOCK_ON | BTN_JUMP, STICK_RIGHT, 1);
    add(r);
}

std::optional<std::string> DanteAirDodge::on_initialize() {
    // rollback netplay: the dodges and their built cancel scripts go with the frames (a player's script manager points into
    // the scripts, and a later dodge rebuilds the bytes)
    rollback::add_range("DanteAirDodge dodges", dante_air_dodge::g_dodges, sizeof dante_air_dodge::g_dodges);
    rollback::add_range("DanteAirDodge cancel scripts", dante_air_dodge::g_cancel_scripts, sizeof dante_air_dodge::g_cancel_scripts);
    using namespace input_commands;
    move_registry::register_move(move_registry::DANTE, move_id, &mod_enabled, &dodge_move);   // the ID gate + Dante dispatch
    g_dodge_cmd.condition(CND_POSE).button(BTN_JUMP);
    // lock-on + jump while the stick leans sideways and a dodge is left, on a free bit of Dante's
    cmd_bit = next_free_bit(CHAR_DANTE);
    if (cmd_bit < 0 || !add_for(CHAR_DANTE, g_dodge_cmd.head(), cmd_bit, &mod_enabled, &can_dodge)) {
        spdlog::error("DanteAirDodge: no free command bit for the dodge (Dante's {}-{} are all taken)", FIRST_FREE_BIT, LAST_BIT);
        cmd_bit = -1;
    }
    if (!install_hook_absolute(0x7B8E30, hook_wjump, &detour_wjump_check, &jmp_ret_wjump, 6)) {
        spdlog::error("DanteAirDodge: failed to hook uPlayerDante::checkWJumpBegin (0x7B8E30)");
        return "Failed to init DanteAirDodge (air hike hook)";
    }
    register_boss_move();
    return Mod::on_initialize();
}

// Readout log: one line per change of the lower-body action, motion or phase, and one for any upward impulse
// (vel.y rising by more than `kImpulse` between two frames: a jump that keeps the action number still shows).
struct EventLine { char text[128]; };
constexpr int kLogLines = 14;
constexpr float kImpulse = 4.0f;
static EventLine g_log[kLogLines];
static int g_log_n = 0, g_log_head = 0;
static uint32_t g_last_action = ~0u, g_last_motion = ~0u;
static int g_last_phase = -1;
static float g_last_vel_y = 0.0f;

static void log_line(uintptr_t P, const char* tag) {
    EventLine& l = g_log[g_log_head];
    snprintf(l.text, sizeof l.text, "%s act %u atk %u mot %04X f %.1f ph %d | J %d/%d | trg %X on %X | wj %d st %02X vy %.1f",
             tag, at<uint32_t>(P, pl::mBdyActionLowerNo), at<uint32_t>(P, pl::mAtckId), at<uint32_t>(P, pl::mMotionCtrl0No),
             at<float>(P, pl::mMotionFrame0), at<int>(P, pl::mBdyActionLowerPhase),
             at<int>(P, pl::mMotCnclJumpStat), (int)at<bool>(P, pl::mMotCnclJumpReserve),
             at<uint32_t>(P, pl::mBtnTrg), at<uint32_t>(P, pl::mBtnOn), (int)at<char>(P, pl::mWJumpCnt), at<uint32_t>(P, pl::mActStat),
             at<MtVector3>(P, pl::mVel).y);
    g_log_head = (g_log_head + 1) % kLogLines;
    if (g_log_n < kLogLines) ++g_log_n;
}

static void log_transitions(uintptr_t P) {
    const uint32_t action = at<uint32_t>(P, pl::mBdyActionLowerNo);
    const uint32_t motion = at<uint32_t>(P, pl::mMotionCtrl0No);
    const int phase = at<int>(P, pl::mBdyActionLowerPhase);
    const float vy = at<MtVector3>(P, pl::mVel).y;
    if (action != g_last_action || motion != g_last_motion) log_line(P, "ACT");
    else if (phase != g_last_phase) log_line(P, "ph ");
    if (vy - g_last_vel_y > kImpulse) log_line(P, "UP ");
    g_last_action = action; g_last_motion = motion; g_last_phase = phase; g_last_vel_y = vy;
}

void DanteAirDodge::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {
        if (cmd_bit >= 0 && at<int>((uintptr_t)pl, pl::mPlayerID) == 0       // Dante's bit: on Nero it is another command's
            && (at<uint32_t>((uintptr_t)pl, 0x2184 + 4 * (cmd_bit >> 5)) >> (cmd_bit & 31)) & 1) ++g_bit_seen;
        log_transitions((uintptr_t)pl);
    }
}

void DanteAirDodge::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) { debug_panel(); return; }   // the Debug tab: the readout
    if (display != DISPLAY_DANTE_A) return;
    if (ImGui::Checkbox(_("Air Dodge"), &mod_enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("Lock-on + stick left or right + jump in the air: a sideways dodge. It spends the air hike (Devil Trigger keeps "
                  "its second), and the same press without a sideways stick stays an air hike. When the dodge plays out, Dante "
                  "falls on drifting sideways, the drift dying away as after Sky Star. Needs nativePC\\motion\\pl006\\airdodge.lmt."));
}

void DanteAirDodge::debug_panel() {
    if (!mod_enabled) return;
    uPlayer_old* pl = devil4_sdk::get_local_player();
    if (!pl) return;
    const uintptr_t P = (uintptr_t)pl;
    ImGui::PushID("DanteAirDodge");
    ImGui::TextUnformatted(_("Dante Air Dodge"));
    ImGui::Indent(lineIndent);
    ImGui::Text(_("File: %s | %s"), lmt_file_exists(lmt_path) ? "found" : "missing", motion_registry::describe(lmt_set, P).c_str());
    if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();   // read again from disk on the next dodge
    ImGui::SameLine();
    help_marker(_("Reads the dodge's LMT (nativePC\\motion\\pl006\\airdodge.lmt, slot 0 right, 1 left) again from disk on the next dodge."));
    const uint32_t* f = &at<uint32_t>(P, 0x2184);
    ImGui::Text("actStat %02X  frame %.1f  phase %d  atckId %u  stick sideways %d  bit %d: %d  wjumpCnt %d  jump window stat %d reserve %d  inertia %.2f (%+.2f)",
                at<uint32_t>(P, pl::mActStat), at<float>(P, pl::mMotionFrame0), at<int>(P, pl::mBdyActionLowerPhase),
                at<uint32_t>(P, pl::mAtckId), (int)stick_sideways(P), cmd_bit,
                cmd_bit >= 0 ? (int)((f[cmd_bit >> 5] >> (cmd_bit & 31)) & 1) : 0, (int)at<char>(P, pl::mWJumpCnt),
                at<int>(P, pl::mMotCnclJumpStat), (int)at<bool>(P, pl::mMotCnclJumpReserve),
                at<float>(P, pl::mMoveVel), at<float>(P, pl::mMoveAcc));
    ImGui::Text("counters: air hikes suppressed %d | ticks with the bit %d | move entries %d | used this airtime %d | row cmd %08X | hook %s",
                g_suppressed, g_bit_seen, g_moves, state_for(P).used,
                (HookDanteKADTbl && MoveTable::DanteAirDodge) ? ((kAtckDefTbl*)HookDanteKADTbl + (MoveTable::extra_dante_moves - MoveTable::DanteAirDodge))->command.buffer : 0,
                hook_wjump ? "installed" : "MISSING");
    ImGui::TextWrapped("log (newest last). ACT = action / motion change, ph = phase change (3 = the fall after the cycle), UP = upward "
                       "impulse. act 6 jump, 7 air, 8 air hike, 10 land, 12 attack, 0x18 wall jump, 25 enemy step; J = C_JUMP window "
                       "stat / reserve; vy = vertical speed");
    for (int i = 0; i < g_log_n; ++i) ImGui::TextWrapped("%s", g_log[(g_log_head - g_log_n + i + kLogLines) % kLogLines].text);
    ImGui::Unindent(lineIndent);
    ImGui::PopID();
}

void DanteAirDodge::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dante_air_dodge").value_or(false);
    apply_row();
    register_lmt();
}

void DanteAirDodge::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_air_dodge", mod_enabled);
}
