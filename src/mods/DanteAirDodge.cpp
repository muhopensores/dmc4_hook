#include "DanteAirDodge.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MotionRegistry.hpp"
#include "../sdk/CtnScript.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace dante_air_dodge {

bool  mod_enabled     = false;
uintptr_t jmp_ret_wjump = 0;
int   lmt_set         = -1;
char  lmt_path[128]   = "motion\\pl006\\airdodge";
int   slot[DIR_COUNT] = { 1, 0 };        // right, left
int   ctn_bank        = 0;
int   ctn_slot        = 5;               // the fall motion's script: no cancels until the game's air rules say so
float launch_frame    = 0.0f;
float speed           = 16.0f;
float rise            = 16.0f;
float gravity         = -1.1f;
float max_fall        = 30.0f;
float cancel_frame    = 10.0f;
float side_angle      = 50.0f;
bool  face_direction  = false;
bool  airborne_status = true;
bool  enemy_step_from_start = true;   // the enemy step (jump cancel) can interrupt the dodge from frame 0, before the cancel frame
bool  cost_air_hike   = true;        // the dodge is the air jump: no air hike after it (uPlayer::checkWJumpBegin: mWJumpCnt >= wJumpCnt)
int   dodges_per_air  = 0;           // 0: the game's air-jump count is the only limit (2 in DT), since the dodge spends one
bool  effect_enabled  = true;
int   effect_list     = 2;
int   effect_index    = 0x14;
float effect_tilt     = 45.0f;
bool  se_enabled      = true;
int   se_id           = 40;

// ---------------------------------------------------------------------------------------------------------------
// Game functions and helpers (Gerb's, kept file-local the same way).
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_setActionLand = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed_) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed_, start, interp);
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
    if (effect_enabled) {
        float q[4];
        if (effect_tilt == 0.0f) { const float* g = &at<float>(pl, pl::mQuat); q[0] = g[0]; q[1] = g[1]; q[2] = g[2]; q[3] = g[3]; }
        else basis_quat(pl, dir, effect_tilt, q);
        devil4_sdk::effect_stay(effect_list, effect_index, q, &at<float>(pl, pl::mPos), (void*)pl);
    }
    if (se_enabled) devil4_sdk::play_player_se((void*)pl, se_id);
}

constexpr int PHASE_ENTRY    = 0;
constexpr int PHASE_WAIT     = 1;     // cycle playing, waiting for launch_frame
constexpr int PHASE_AIRBORNE = 2;

constexpr int kPlayerSlots = 4;       // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)
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

// First tick after the motion request: swap the borrowed script for the cancel script (if a cancel frame is set).
// (Opening C_JUMP from frame 0 was tried for the jump cancel and dropped: with an air hike left (DT) it let the
// dodge be cancelled into one at once. The enemy step needs no window: see check_enemy_step.)
void arm_cancel_script(DodgeState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    if (cancel_frame >= 0.0f) {
        ctn::Script& sc = g_cancel_scripts[slot_of(s) % kPlayerSlots];
        sc.reset().wait(cancel_frame).open_all().end();
        devil4_sdk::install_ctn_script((void*)pl, sc.data());
    }
}

// The cancel gate (mIsCancelCheckEnable): cleared by setActionAtck, set back at the cancel frame as the vanilla attacks do.
bool gate_open(uintptr_t pl) { return cancel_frame >= 0.0f && at<float>(pl, pl::mMotionFrame0) >= cancel_frame; }
void open_cancel_gate(uintptr_t pl) {
    if (gate_open(pl)) at<bool>(pl, pl::mIsCancelCheckEnable) = true;
}

// The press that started the dodge outlives the tick (the game ticks faster than the pad reports: the panel log showed
// mBtnTrg still carrying it one tick later, and uPlayer::updateMotCancel latching it as the C_JUMP reserve). Until the
// button has been released once, the dodge eats the trigger bit and that latch, so no jump check takes the same press.
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

// ---- the command: lock-on held + jump pressed, while the stick leans sideways (the predicate) ----------------
static input_commands::Command g_dodge_cmd;   // lock-on held (CND_POSE) + jump pressed; built in on_initialize

static int g_suppressed = 0;          // readout: air hikes the hook turned into dodge presses
static int g_bit_seen = 0;            // readout: ticks the command bit was set (counted from on_frame)
static int g_moves = 0;               // readout: entries into the move

// The air hike's check, suppressed when this press is the dodge's: the hook returns 0 from
// uPlayerDante::checkWJumpBegin so checkAtck, which runs after it, can match the row instead.
bool __stdcall suppress_air_hike(uintptr_t pl) {
    if (!mod_enabled || !pl) return false;
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

// Once per airtime by default: the count resets whenever the player stands (AST_STAND), so a spent dodge leaves the
// next lock-on jump press to the air hike instead of chaining another dodge.
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
    lmt_set = lmt_path[0] ? motion_registry::add(d) : -1;
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
        if (!ready(pl)) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // no LMT: bail out

        const float yaw = at<float>(pl, pl::mAnlgRadian);
        const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
        const glm::vec3 right = wmat_row(pl, 0);
        const Dir dir = glm::dot(stick, right) >= 0.0f ? RIGHT : LEFT;
        s.dir_vec = dir == RIGHT ? right : -right;
        s.dir = dir;
        const int motion = motion_registry::word(lmt_set, pl, slot[dir]);
        if (motion < 0) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // slot past the file's blocks
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
                devil4_sdk::neutral_air_transition((uPlayer*)pl);
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
        // Watchdog: the only exits are the landing and the cancels after cancel_frame, both of which need the motion to
        // play. A frame that doesn't move for 60 ticks means it isn't: report and leave, instead of a soft lock.
        const float frame = at<float>(pl, pl::mMotionFrame0);
        s.stuck_ticks = (frame == s.last_frame) ? s.stuck_ticks + 1 : 0;
        s.last_frame = frame;
        if (s.stuck_ticks > 60) {
            spdlog::warn("DanteAirDodge: motion frame stuck at {:.1f} for 60 ticks airborne (controller plays {:04X}, wanted {:04X}): "
                         "is {}.lmt block {} present and not empty ({})?",
                         frame, at<uint32_t>(pl, pl::mMotionCtrl0No), motion_registry::word(lmt_set, pl, slot[s.dir]), lmt_path, slot[s.dir],
                         motion_registry::describe(lmt_set, pl));
            devil4_sdk::neutral_air_transition((uPlayer*)pl);
            return;
        }
        vel.y += gravity * at<float>(pl, pl::m_delta_time);
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
    row->command.buffer = (uint32_t)(cmd_bit & 0xFF) | (mod_enabled ? 0u : 0x100u);
}

} // namespace dante_air_dodge

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only.
// ---------------------------------------------------------------------------------------------------------------
using namespace dante_air_dodge;

// the registry's state-function shape (player, atckId): the dodge has one row
static void __stdcall dodge_move(uintptr_t player, uint32_t) { dante_air_dodge::move(player); }

std::optional<std::string> DanteAirDodge::on_initialize() {
    using namespace input_commands;
    move_registry::register_move(move_registry::DANTE, move_id, &mod_enabled, &dodge_move);   // the ID gate + Dante dispatch
    g_dodge_cmd.condition(CND_POSE).button(BTN_JUMP);
    add(g_dodge_cmd.head(), cmd_bit, &mod_enabled, &can_dodge);   // bit 86: lock-on + jump while the stick leans sideways and a dodge is left
    if (!install_hook_absolute(0x7B8E30, hook_wjump, &detour_wjump_check, &jmp_ret_wjump, 6)) {
        spdlog::error("DanteAirDodge: failed to hook uPlayerDante::checkWJumpBegin (0x7B8E30)");
        return "Failed to init DanteAirDodge (air hike hook)";
    }
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
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        if ((at<uint32_t>((uintptr_t)pl, 0x2184 + 8) >> (cmd_bit - 64)) & 1) ++g_bit_seen;
        log_transitions((uintptr_t)pl);
    }
}

void DanteAirDodge::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;        // debug page, like Gerb
    if (ImGui::Checkbox(_("Dante air dodge"), &mod_enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("Lock-on + stick left or right + jump in the air: a sideways dodge with Gerb's arc, from motion\\pl006\\airdodge.lmt "
                  "(slot 0 right, 1 left). The same press without a sideways stick stays an air hike (uPlayerDante::checkWJumpBegin "
                  "is hooked to step aside only for the dodge)."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("DanteAirDodge");            // the page's other panels share some labels
    if (ImGui::InputText(_("LMT"), lmt_path, sizeof(lmt_path), ImGuiInputTextFlags_EnterReturnsTrue)) register_lmt();
    ImGui::SameLine();
    help_marker(_("Game path of the LMT under nativePC, without .lmt; Enter applies it. The hook places it in a free motion "
                  "bank itself (shared with the other features that add animations)."));
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | %s"), lmt_file_exists(lmt_path) ? "found" : "missing",
                    motion_registry::describe(lmt_set, (uintptr_t)pl).c_str());
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();   // read again from disk on the next dodge
    }
    ImGui::InputInt(_("Slot right"), &slot[RIGHT]);
    ImGui::InputInt(_("Slot left"), &slot[LEFT]);
    ImGui::InputInt(_("Borrowed .ctn bank"), &ctn_bank);
    ImGui::InputInt(_("Borrowed .ctn slot"), &ctn_slot);
    ImGui::SliderFloat(_("Launch frame"), &launch_frame, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Speed"), &speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Rise"), &rise, -20.0f, 40.0f);
    ImGui::SliderFloat(_("Gravity"), &gravity, -5.0f, 0.0f);
    ImGui::SliderFloat(_("Max fall"), &max_fall, 1.0f, 80.0f);
    ImGui::SliderFloat(_("Cancel frame"), &cancel_frame, -1.0f, 60.0f);
    ImGui::Checkbox(_("Enemy step from frame 0"), &enemy_step_from_start);
    ImGui::SameLine();
    help_marker(_("Runs the game's enemy-step check (uPlayer::checkEmJumpBegin) during the dodge before the cancel frame, so a jump "
                  "press over an enemy cancels it from the first frame. Opens nothing else: the air hike and wall jump wait for the "
                  "cancel frame."));
    ImGui::SameLine();
    help_marker(_("From this frame of the cycle every cancel window is open (an in-memory .ctn script replaces the borrowed one). "
                  "-1 = never (the borrowed script's windows apply)."));
    ImGui::SliderFloat(_("Side angle"), &side_angle, 10.0f, 85.0f);
    ImGui::SameLine();
    help_marker(_("Degrees off the right / left axis of the facing within which the stick counts as sideways, for the input and the "
                  "air-hike suppression. 90 would take any lean."));
    ImGui::Checkbox(_("Face the direction"), &face_direction);
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Count as airborne"), &airborne_status);
    ImGui::InputInt(_("Dodges per airtime"), &dodges_per_air);
    ImGui::SameLine();
    help_marker(_("An extra cap on dodges between landings; 0 = none, so the air-jump count alone decides (1 normally, 2 in Devil Trigger, "
                  "shared with the air hike since the dodge spends one). A spent press is a normal air hike if any jump is left."));
    ImGui::Checkbox(_("Costs the air hike"), &cost_air_hike);
    ImGui::SameLine();
    help_marker(_("Counts the dodge as the air jump (mWJumpCnt), so no air hike is left after it until landing."));
    ImGui::Checkbox(_("Take-off effect"), &effect_enabled);
    ImGui::SameLine();
    help_marker(_("Dante's air hike effect at the launch frame: list 2 index 0x14 (ec005_20v1), spawned as uPlayerDante::setWJumpEffect does."));
    if (effect_enabled) {
        ImGui::InputInt(_("Effect list"), &effect_list);
        ImGui::InputInt(_("Effect index"), &effect_index);
        ImGui::SliderFloat(_("Effect tilt"), &effect_tilt, -60.0f, 60.0f);
    }
    ImGui::Checkbox(_("Take-off sound"), &se_enabled);
    if (se_enabled) ImGui::InputInt(_("SE id"), &se_id);
    if (uPlayer* pl = devil4_sdk::get_local_player()) {      // live readout
        const uintptr_t P = (uintptr_t)pl;
        const uint32_t* f = &at<uint32_t>(P, 0x2184);
        ImGui::Text("actStat %02X  frame %.1f  phase %d  atckId %u  stick sideways %d  bit86 %d  wjumpCnt %d  jump window stat %d reserve %d",
                    at<uint32_t>(P, pl::mActStat), at<float>(P, pl::mMotionFrame0), at<int>(P, pl::mBdyActionLowerPhase),
                    at<uint32_t>(P, pl::mAtckId), (int)stick_sideways(P), (int)((f[2] >> (cmd_bit - 64)) & 1), (int)at<char>(P, pl::mWJumpCnt),
                    at<int>(P, pl::mMotCnclJumpStat), (int)at<bool>(P, pl::mMotCnclJumpReserve));
        ImGui::Text("counters: air hikes suppressed %d | ticks with bit86 %d | move entries %d | used this airtime %d | row cmd %08X | hook %s",
                    g_suppressed, g_bit_seen, g_moves, state_for(P).used,
                    (HookDanteKADTbl && MoveTable::DanteAirDodge) ? ((kAtckDefTbl*)HookDanteKADTbl + (MoveTable::extra_dante_moves - MoveTable::DanteAirDodge))->command.buffer : 0,
                    hook_wjump ? "installed" : "MISSING");
        ImGui::TextWrapped("log (newest last). ACT = action / motion change, ph = phase change, UP = upward impulse. act 6 jump, 7 air, 8 air hike, "
                           "10 land, 12 attack, 0x18 wall jump, 25 enemy step; J = C_JUMP window stat / reserve; vy = vertical speed");
        for (int i = 0; i < g_log_n; ++i) ImGui::TextWrapped("%s", g_log[(g_log_head - g_log_n + i + kLogLines) % kLogLines].text);
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void DanteAirDodge::on_config_load(const utility::Config& cfg) {
    mod_enabled     = cfg.get<bool>("dante_air_dodge").value_or(false);
    apply_row();
    for (int i = 0; i < DIR_COUNT; ++i) slot[i] = cfg.get<int>("dante_air_dodge_slot_" + std::to_string(i)).value_or(slot[i]);
    ctn_bank        = cfg.get<int>("dante_air_dodge_ctn_bank").value_or(ctn_bank);
    ctn_slot        = cfg.get<int>("dante_air_dodge_ctn_slot").value_or(ctn_slot);
    launch_frame    = cfg.get<float>("dante_air_dodge_launch_frame").value_or(launch_frame);
    speed           = cfg.get<float>("dante_air_dodge_speed").value_or(speed);
    rise            = cfg.get<float>("dante_air_dodge_rise").value_or(rise);
    gravity         = cfg.get<float>("dante_air_dodge_gravity").value_or(gravity);
    max_fall        = cfg.get<float>("dante_air_dodge_max_fall").value_or(max_fall);
    cancel_frame    = cfg.get<float>("dante_air_dodge_cancel_frame").value_or(cancel_frame);
    enemy_step_from_start = cfg.get<bool>("dante_air_dodge_enemy_step_from_start").value_or(enemy_step_from_start);
    side_angle      = cfg.get<float>("dante_air_dodge_side_angle").value_or(side_angle);
    face_direction  = cfg.get<bool>("dante_air_dodge_face_direction").value_or(face_direction);
    airborne_status = cfg.get<bool>("dante_air_dodge_airborne_status").value_or(airborne_status);
    cost_air_hike   = cfg.get<bool>("dante_air_dodge_costs_air_hike").value_or(cost_air_hike);   // (key renamed so the old false default isn't reloaded)
    dodges_per_air  = cfg.get<int>("dante_air_dodge_max_per_air").value_or(dodges_per_air);   // (key renamed: the old default 1 shouldn't reload)
    effect_enabled  = cfg.get<bool>("dante_air_dodge_effect").value_or(effect_enabled);
    effect_list     = cfg.get<int>("dante_air_dodge_effect_list").value_or(effect_list);
    effect_index    = cfg.get<int>("dante_air_dodge_effect_index").value_or(effect_index);
    effect_tilt     = cfg.get<float>("dante_air_dodge_effect_tilt").value_or(effect_tilt);
    se_enabled      = cfg.get<bool>("dante_air_dodge_se").value_or(se_enabled);
    se_id           = cfg.get<int>("dante_air_dodge_se_id").value_or(se_id);
    register_lmt();
}

void DanteAirDodge::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dante_air_dodge", mod_enabled);
    for (int i = 0; i < DIR_COUNT; ++i) cfg.set<int>("dante_air_dodge_slot_" + std::to_string(i), slot[i]);
    cfg.set<int>("dante_air_dodge_ctn_bank", ctn_bank);
    cfg.set<int>("dante_air_dodge_ctn_slot", ctn_slot);
    cfg.set<float>("dante_air_dodge_launch_frame", launch_frame);
    cfg.set<float>("dante_air_dodge_speed", speed);
    cfg.set<float>("dante_air_dodge_rise", rise);
    cfg.set<float>("dante_air_dodge_gravity", gravity);
    cfg.set<float>("dante_air_dodge_max_fall", max_fall);
    cfg.set<float>("dante_air_dodge_cancel_frame", cancel_frame);
    cfg.set<bool>("dante_air_dodge_enemy_step_from_start", enemy_step_from_start);
    cfg.set<float>("dante_air_dodge_side_angle", side_angle);
    cfg.set<bool>("dante_air_dodge_face_direction", face_direction);
    cfg.set<bool>("dante_air_dodge_airborne_status", airborne_status);
    cfg.set<bool>("dante_air_dodge_costs_air_hike", cost_air_hike);
    cfg.set<int>("dante_air_dodge_max_per_air", dodges_per_air);
    cfg.set<bool>("dante_air_dodge_effect", effect_enabled);
    cfg.set<int>("dante_air_dodge_effect_list", effect_list);
    cfg.set<int>("dante_air_dodge_effect_index", effect_index);
    cfg.set<float>("dante_air_dodge_effect_tilt", effect_tilt);
    cfg.set<bool>("dante_air_dodge_se", se_enabled);
    cfg.set<int>("dante_air_dodge_se_id", se_id);
}
