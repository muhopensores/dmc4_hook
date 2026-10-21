#include "Gerb.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include <cmath>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace gerb {


bool  mod_enabled   = false;
int   input_cmd     = LOCKON_ANYDIRECTION_STYLE;   // lock-on + stick in any direction + style; the stick's direction picks the jump's
int   input_cmd_vertical = cmd_bf_style;     // lock-on + back, forward + style (Gerb's own command)
uintptr_t jmp_ret_cmd = 0;
int   lmt_bank      = 13;
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
static uintptr_t fn_commandCheckSub = 0x00805A10; // uPlayer::commandCheckSub(this, kSPlCmd*, kCmdHead*, kCmdData*) stdcall, result in al
static uintptr_t fn_SetEffectStay = 0x004805D0;   // sDevil4Effect::SetEffectStay(list@eax, idx@ecx, quat*, pos*, group, material, line, owner) -> effect@eax, retn 18h
static uintptr_t fn_setEffect     = 0x0081FD60;   // uPlayer::setEffect(effect@eax, this@ecx, EFCT_TYPE, id, bool, bool) userpurge
static uintptr_t fn_requestSe     = 0x00A47AA0;   // sDevil4Sound::requestSe(inst, rsrc, id, owner, pos*, 1, 1, cb) stdcall
static uintptr_t fn_sePositionCb  = 0x00486640;   // sSoundN::soundPositionUpdateCb
static uintptr_t sSoundN_ptr      = 0x00E559D8;   // sSoundN::mpInstance

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

// .ctn borrow (MoveDef D15): the next motion request reads the cancel script of ctn_bank:ctn_slot for both halves.
void borrow_ctn(uintptr_t pl, uint32_t bank, uint32_t idx) {
    for (int half = 0; half < 2; ++half) {
        uintptr_t p = pl + pl::mMotionSetCharTblParam + 12 * half;
        at<bool>(p, 0)     = true;
        at<uint32_t>(p, 4) = bank;
        at<uint32_t>(p, 8) = idx;
    }
}

// The cancel window the game's own way (MoveDef D11): a .ctn script synthesised in memory, `wait(frame)` then one
// contact command opening every cancel category, and both cCharTblMgr cursors pointed at it. cCharTblMgr::update
// then runs it like any vanilla script and updateContactInfoSub opens the windows, which stay open as a vanilla
// cancel does. One script per player slot; the bytes must outlive the motion, so they're static.
//   00 mode pad16 f32 frame          wait until mFrame >= frame (mode 0: go on)
//   01 01 b0 b1 b2 b3 b4 b5          contact set 1: byte k's low nibble picks cancelNoTbl[k] (0 = C_NORM0, C_COMMAND,
//                                    C_JUMP, C_CHOUHATSU, C_GUN, C_ALL for k = 0..5), high nibble 0x10 = open
//   FF                               end
#pragma pack(push, 1)
struct CancelScript {
    uint8_t  wait_op = 0, wait_mode = 0; uint16_t wait_pad = 0; float wait_frame = 0.0f;
    uint8_t  contact_op = 1, contact_idx = 1, info[6] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
    uint8_t  end_op = 0xFF, pad[3] = { 0, 0, 0 };
};
#pragma pack(pop)
static_assert(sizeof(CancelScript) == 20, "ctn script layout");
constexpr int kPlayerSlots = 4;        // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)
static CancelScript g_cancel_scripts[kPlayerSlots];

// Point both halves' cursors at the script. Call after updateMotion has installed the borrowed script for the new
// motion (the tick after the request), else updateMotion overwrites the cursors again.
void install_cancel_script(uintptr_t pl, int slot, float frame) {
    CancelScript& sc = g_cancel_scripts[slot % kPlayerSlots];
    sc.wait_frame = frame;
    for (int half = 0; half < 2; ++half) {
        uintptr_t mgr = pl + pl::mCharTblMgr + 0x24 * half;
        at<uint8_t*>(mgr, 0x0C) = (uint8_t*)&sc;   // mpDataTop
        at<uint8_t*>(mgr, 0x10) = (uint8_t*)&sc;   // mpDataCrnt
        at<bool>(mgr, 0x14)     = false;           // mIsDone
    }
}

// ---- Gerb's own input command ------------------------------------------------------------------------------------
// uPlayer::executeCommandCheck 0x805920 runs the 85 commands of uPlayer::commandTbl (0xC3E328, 16-byte heads) each
// tick, each through commandCheckSub with its own kSPlCmd state (mPlCmd[85] +0x2190), and sets bit CMD_ID of
// mCmdFlag[3] (+0x2184) for the ones that fire; a COMMAND row then needs its bit. The table and the loop are fixed,
// but the flag words hold 96 bits, so a command of our own gets bit 85: the same check, run from a hook right after
// executeCommandCheck, with our own state. Steps (kCmdData: type, button, param {mode, lever}, timer):
//   LEVER mode 1 (relative to the target / facing) lever 0 = stick centred, no time limit
//   LEVER mode 1 lever 4 = back within 45 deg, within 15 frames
//   LEVER mode 1 lever 1 = forward, within 15 frames
//   BTN   mode 1 (mBtnTrg) 0x200 = style
// = cmd 0x52 CMD_LOCK_BF_LEVER_SHORT with the style button in place of melee. Condition CND_POSE = lock-on held
// (mBtnOn 0x10), else the state resets, as executeCommandCheck does.
struct kCmdData { uint32_t type; uint32_t btn; uint16_t mode; uint16_t lever; int16_t timer; uint16_t pad; };
struct kCmdHead { uint16_t num; uint16_t pad0; uint32_t condition; int16_t active; uint16_t pad1; const kCmdData* data; };
struct kSPlCmd  { uint8_t step, status, pad0[2]; float activeF, timerF; uint8_t cnt, cmdFlag, pad1[2]; float tame; };
static_assert(sizeof(kCmdData) == 16 && sizeof(kCmdHead) == 16 && sizeof(kSPlCmd) == 20, "command layouts");

static const kCmdData g_bf_style_steps[4] = {
    { 2, 0,     1, 0, -1, 0 },
    { 2, 0,     1, 4, 15, 0 },
    { 2, 0,     1, 1, 15, 0 },
    { 1, 0x200, 1, 0,  0, 0 },
};
static const kCmdHead g_bf_style_head = { 4, 0, 1 /* CND_POSE */, 1, 0, g_bf_style_steps };
static kSPlCmd g_bf_style_state[kPlayerSlots];     // per player slot (state_for's order)

// commandCheckSub is stdcall, but its helpers read the player from esi, so it is called with esi = this too.
int command_check_sub(uintptr_t pl, kSPlCmd* pstate, const kCmdHead* phead, const kCmdData* psteps) {   // (`st` is a MASM register name)
    int r;
    _asm {
        pushad
        mov esi, pl
        push psteps
        push phead
        push pstate
        push esi
        call fn_commandCheckSub
        movzx eax, al
        mov r, eax
        popad
    }
    return r;
}

int player_slot(uintptr_t pl);   // below (JumpState)

void __stdcall extra_commands_from_hook(uintptr_t pl) { extra_commands(pl); }

// 0x7A83B5 in uPlayer::main, right after `call uPlayer::executeCommandCheck` (edi = this): the 8 bytes replaced are
// `mov edx, [edi]; mov eax, [edx+248h]` (the next virtual call's load).
naked void detour_extra_commands() {
    _asm {
        pushad
        push edi
        call extra_commands_from_hook
        popad
        mov edx, [edi]
        mov eax, [edx + 0x248]
        jmp dword ptr [jmp_ret_cmd]
    }
}

void clear_add_speed(uintptr_t pl) {
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

// The air hike's effect the way uPlayerNero::setWJumpEffect spawns it: SetEffectStay at `pos` oriented by `quat`
// (group / material masks -1, unit line 0x14, the player as owner), then uPlayer::setEffect(effect, EFCT_TYPE_ERASE_RESET,
// -1, true, true) so the player's effect list owns it (erased on a reset; released at once when effects are off).
void stay_effect(uintptr_t pl, int list, int idx, const float* quat, const MtVector3* pos) {
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push pl                 // owner
        push 0x14               // unit line
        push 0xFFFFFFFF         // material mask
        push 0xFFFFFFFF         // group mask
        push pos
        push quat
        mov eax, list
        mov ecx, idx
        call fn_SetEffectStay
        test eax, eax
        jz stay_effect_done
        mov ecx, pl             // uPlayer::setEffect(effect, this, 1, -1, 1, 1)
        push 1
        push 1
        push 0xFFFFFFFF
        push 1
        call fn_setEffect
    stay_effect_done:
        popad
        mov esp, ebx
        pop ebx
    }
}

// The air hike's sound: SE `id` of the player's own sound resource, positioned on the player.
void play_se(uintptr_t pl, int id) {
    const uintptr_t inst = *(uintptr_t*)sSoundN_ptr;
    const uintptr_t rsrc = at<uintptr_t>(pl, pl::mSeRsrc);
    if (!inst || !rsrc) return;
    ((void(__stdcall*)(uintptr_t, uintptr_t, int, uintptr_t, const MtVector3*, int, int, uintptr_t))fn_requestSe)(
        inst, rsrc, id, pl, &at<MtVector3>(pl, pl::mPos), 1, 1, fn_sePositionCb);
}

// The player's orientation (mWmat rows right / up / forward) with its up leaned `tilt_deg` toward the level part of
// `dir`, as an MT quaternion (x, y, z, w). A vertical `dir` gives no lean. The conversion takes the rows as the columns
// of a column-vector rotation (MT's (x, y, z, w) is Blender's (w, x, y, z) for the transposed matrix: RE notes,
// uEffectVFR::setQuatParentOfs), so with no tilt it reproduces mQuat; the debug readout shows both to check that.
glm::vec3 wmat_row(uintptr_t pl, int row);   // defined below
glm::vec3 rodrigues(const glm::vec3& v, const glm::vec3& axis, float deg) {
    const float a = deg * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
    return v * c + glm::cross(axis, v) * s + axis * (glm::dot(axis, v) * (1.0f - c));
}

// `roll_deg` then turns the basis about its own forward axis (180 = upside down, for the dive).
void basis_quat(uintptr_t pl, const glm::vec3& dir, float tilt_deg, float roll_deg, float out[4]) {
    glm::vec3 b[3] = { wmat_row(pl, 0), wmat_row(pl, 1), wmat_row(pl, 2) };
    const glm::vec3 h(dir.x, 0.0f, dir.z);
    if (tilt_deg != 0.0f && glm::length(h) > 1e-3f) {
        const glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(h)));
        for (glm::vec3& v : b) v = rodrigues(v, axis, tilt_deg);
    }
    if (roll_deg != 0.0f) {
        const glm::vec3 axis = glm::normalize(b[2]);
        b[0] = rodrigues(b[0], axis, roll_deg);
        b[1] = rodrigues(b[1], axis, roll_deg);
    }
    const float m00 = b[0].x, m10 = b[0].y, m20 = b[0].z;
    const float m01 = b[1].x, m11 = b[1].y, m21 = b[1].z;
    const float m02 = b[2].x, m12 = b[2].y, m22 = b[2].z;
    const float tr = m00 + m11 + m22;
    float x, y, z, w, S;
    if (tr > 0.0f)                       { S = std::sqrt(tr + 1.0f) * 2.0f;             w = 0.25f * S;       x = (m21 - m12) / S; y = (m02 - m20) / S; z = (m10 - m01) / S; }
    else if (m00 > m11 && m00 > m22)     { S = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f; w = (m21 - m12) / S; x = 0.25f * S;       y = (m01 + m10) / S; z = (m02 + m20) / S; }
    else if (m11 > m22)                  { S = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f; w = (m02 - m20) / S; x = (m01 + m10) / S; y = 0.25f * S;       z = (m12 + m21) / S; }
    else                                 { S = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f; w = (m10 - m01) / S; x = (m02 + m20) / S; y = (m12 + m21) / S; z = 0.25f * S; }
    out[0] = x; out[1] = y; out[2] = z; out[3] = w;
}

// The air hike's effect and sound at the take-off: the effect leaned toward the jump, upside down for the dive.
void take_off_fx(uintptr_t pl, const glm::vec3& dir) {
    if (effect_enabled) {
        float q[4];
        const glm::vec3 h(dir.x, 0.0f, dir.z);
        const float roll = dir.y < -0.5f ? 180.0f : 0.0f;
        if (roll == 0.0f && (effect_tilt == 0.0f || glm::length(h) < 1e-3f)) { const float* g = &at<float>(pl, pl::mQuat); q[0] = g[0]; q[1] = g[1]; q[2] = g[2]; q[3] = g[3]; }
        else basis_quat(pl, dir, effect_tilt, roll, q);
        stay_effect(pl, effect_list, effect_index, q, &at<MtVector3>(pl, pl::mPos));
    }
    if (se_enabled) play_se(pl, se_id);
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
    g_bf_style_state[slot_of(*victim)] = kSPlCmd{};   // the command state belongs to the evicted player
    return *victim;
}
int player_slot(uintptr_t pl) { return slot_of(state_for(pl)); }

// First tick after the motion request: swap the borrowed script for the cancel script (if a cancel frame is set).
void arm_cancel_script(JumpState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    if (cancel_frame >= 0.0f) install_cancel_script(pl, slot_of(s), cancel_frame);
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

// The command hook's body: the extra command through the game's own checker, its bit into mCmdFlag.
void __stdcall extra_commands(uintptr_t pl) {
    if (!mod_enabled) return;
    kSPlCmd& st = g_bf_style_state[player_slot(pl) % kPlayerSlots];
    if (g_bf_style_head.condition == 1 && !(at<uint32_t>(pl, pl::mBtnOn) & 0x10)) { st = kSPlCmd{}; return; }
    if (command_check_sub(pl, &st, &g_bf_style_head, g_bf_style_steps) == 1)
        at<uint32_t>(pl, 0x2184 + 4 * (cmd_bf_style / 32)) |= 1u << (cmd_bf_style % 32);   // mCmdFlag[3]
}

bool ready(uintptr_t pl) {
    return pl && devil4_sdk::get_motion_bank((void*)pl, lmt_bank) != 0;
}

void ensure_bank(uintptr_t pl) {
    if (!pl || lmt_bank < 10 || lmt_bank > 14) return;      // 0-9 are setup banks, 15 Nero's Buster bank
    static uintptr_t s_failed_for = 0;
    if (s_failed_for == pl || ready(pl)) return;
    if (!lmt_file_exists(lmt_path)) {
        spdlog::warn("Gerb: nativePC\\{}.lmt not found; the move is disabled until it is", lmt_path);
        s_failed_for = pl;
        return;
    }
    if (!devil4_sdk::load_motion_bank((void*)pl, lmt_path, lmt_bank)) { s_failed_for = pl; return; }
    spdlog::info("Gerb: '{}' -> bank {}", lmt_path, lmt_bank);
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
        ensure_bank(pl);
        if (!ready(pl)) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // no LMT: bail out

        s.vertical = atck_id == move_id_vertical;
        s.dir = pick_direction(pl, s.vertical);
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

        borrow_ctn(pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        // the cycle is the whole jump: loop as its block says, root translation off
        request_motion(pl, (lmt_bank << 8) | (slot[s.dir] & 0xFF), (uint32_t)A_NULL_TRANS_OFF, 4.0f, 0.0f, 1.0f);

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

// The row MoveTable::updateKDATbl inserted (index from the front = extra_nero_moves - MoveTable::Gerb, the same
// arithmetic LuciAirThrow uses). Byte 0 of the command word = the ATTACK_DIRECTION_CMD, byte 1 = 1 makes the row
// unmatchable (the table's convention for a disabled custom move).
void apply_row() {
    if (!HookNeroKADTbl || !MoveTable::Gerb || !MoveTable::GerbVertical) return;
    kAtckDefTbl* row = (kAtckDefTbl*)HookNeroKADTbl + (MoveTable::extra_nero_moves - MoveTable::Gerb);
    row->command.buffer = (uint32_t)(input_cmd & 0xFF) | (mod_enabled ? 0u : 0x100u);
    kAtckDefTbl* vrow = (kAtckDefTbl*)HookNeroKADTbl + (MoveTable::extra_nero_moves - MoveTable::GerbVertical);
    vrow->command.buffer = (uint32_t)(input_cmd_vertical & 0xFF) | (mod_enabled ? 0u : 0x100u);
}

static const struct { int id; const char* name; } kInputs[] = {
    { LOCKON_MELEE, "lock-on + melee" }, { LOCKON_GUN, "lock-on + gun" }, { LOCKON_STYLE, "lock-on + style" },
    { LOCKON_FORWARD_MELEE, "lock-on + forward + melee" }, { LOCKON_BACK_MELEE, "lock-on + back + melee" },
    { LOCKON_FORWARD_GUN, "lock-on + forward + gun" }, { LOCKON_BACK_GUN, "lock-on + back + gun" },
    { LOCKON_FORWARD_STYLE, "lock-on + forward + style" }, { LOCKON_BACK_STYLE, "lock-on + back + style" },
    { LOCKON_FORWARD_MELEE2, "lock-on + forward + melee (2)" }, { LOCKON_BACK_MELEE2, "lock-on + back + melee (2)" },
    { LOCKON_FORWARD_GUN2, "lock-on + forward + gun (2)" }, { LOCKON_BACK_GUN2, "lock-on + back + gun (2)" },
    { LOCKON_FORWARD_STYLE2, "lock-on + forward + style (2)" }, { LOCKON_BACK_STYLE2, "lock-on + back + style (2)" },
    { LOCKON_ANYDIRECTION_MELEE, "lock-on + any direction + melee" }, { LOCKON_ANYDIRECTION_GUN, "lock-on + any direction + gun" },
    { LOCKON_ANYDIRECTION_STYLE, "lock-on + any direction + style" }, { ANYDIRECTION_MELEE, "any direction + melee" },
    { ANYDIRECTION_GUN, "any direction + gun" }, { ANYDIRECTION_STYLE, "any direction + style" },
    { MELEE_STYLE, "melee + style" }, { LOCKON_BACKFORWARD_MELEE, "lock-on + back-forward + melee" },
    { cmd_bf_style, "lock-on + back-forward + style (Gerb's own)" },
};

static void input_combo(const char* label, int& cmd) {
    int cur = -1;
    for (int i = 0; i < (int)(sizeof(kInputs) / sizeof(kInputs[0])); ++i) if (kInputs[i].id == cmd) cur = i;
    if (ImGui::BeginCombo(label, cur >= 0 ? kInputs[cur].name : "(raw id)")) {
        for (int i = 0; i < (int)(sizeof(kInputs) / sizeof(kInputs[0])); ++i) {
            if (ImGui::Selectable(kInputs[i].name, i == cur)) { cmd = kInputs[i].id; apply_row(); }
        }
        ImGui::EndCombo();
    }
}

} // namespace gerb

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only. Everything the move is lives in namespace gerb above.
// ---------------------------------------------------------------------------------------------------------------
using namespace gerb;

std::optional<std::string> Gerb::on_initialize() {
    if (!install_hook_absolute(0x7A83B5, hook_cmd, &detour_extra_commands, &jmp_ret_cmd, 8)) {
        spdlog::error("Gerb: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
        return "Failed to init Gerb (command hook)";
    }
    return Mod::on_initialize();
}

void Gerb::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (uPlayer* pl = devil4_sdk::get_local_player()) ensure_bank((uintptr_t)pl);
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
    {   // the inputs: each row's CMD_ID
        input_combo(_("Input (directional)"), input_cmd);
        ImGui::SameLine();
        help_marker(_("The command (CMD_ID) the directional row triggers on; air only. Its own input, separate from the jump "
                      "button; pick one no vanilla Nero move uses in the air."));
        ImGui::InputInt(_("Directional id (raw)"), &input_cmd);
        if (ImGui::IsItemDeactivatedAfterEdit()) apply_row();
        input_combo(_("Input (vertical)"), input_cmd_vertical);
        ImGui::SameLine();
        help_marker(_("The command the vertical row triggers on: up on the ground, down in the air. The default, lock-on + "
                      "back-forward + style, is Gerb's own command (bit 85 of mCmdFlag, set by its hook after "
                      "executeCommandCheck: cmd 0x52's lever steps with the style button)."));
        ImGui::InputInt(_("Vertical id (raw)"), &input_cmd_vertical);
        if (ImGui::IsItemDeactivatedAfterEdit()) apply_row();
    }
    ImGui::SliderInt(_("Bank"), &lmt_bank, 10, 14);
    ImGui::InputText(_("LMT"), lmt_path, sizeof(lmt_path));
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | loaded: %s"), lmt_file_exists(lmt_path) ? "found" : "missing", ready((uintptr_t)pl) ? "yes" : "no");
        if (ImGui::Button(_("Reload LMT"))) devil4_sdk::unload_motion_bank(pl, lmt_bank);
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
        ImGui::Text("mStat N0 %d N1 %d N2 %d CMD %d JMP %d TNT %d GUN %d ALL %d",
                    at<int>(P, pl::mMotCncl + 0x2C * 0 + 4), at<int>(P, pl::mMotCncl + 0x2C * 1 + 4), at<int>(P, pl::mMotCncl + 0x2C * 2 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 3 + 4), at<int>(P, pl::mMotCncl + 0x2C * 4 + 4), at<int>(P, pl::mMotCncl + 0x2C * 5 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 6 + 4), at<int>(P, pl::mMotCncl + 0x2C * 7 + 4));
        for (int half = 0; half < 2; ++half) {
            const uintptr_t mgr = P + pl::mCharTblMgr + 0x24 * half;
            const uint8_t* top = at<uint8_t*>(mgr, 0x0C); const uint8_t* cur = at<uint8_t*>(mgr, 0x10);
            const uint8_t* c1 = &at<uint8_t>(mgr, 0x15 + 6);
            ImGui::Text("ctn[%d] top %p cur +%d done %d ours %d  contact1 %02X %02X %02X %02X %02X %02X", half, top, cur && top ? (int)(cur - top) : -1,
                        (int)at<bool>(mgr, 0x14), (int)(top >= (const uint8_t*)&g_cancel_scripts[0] && top < (const uint8_t*)&g_cancel_scripts[kPlayerSlots]),
                        c1[0], c1[1], c1[2], c1[3], c1[4], c1[5]);
        }
    }
    ImGui::Checkbox(_("Face the direction"), &face_direction);
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Count as airborne"), &airborne_status);
    ImGui::Unindent(lineIndent);
}

void Gerb::on_config_load(const utility::Config& cfg) {
    mod_enabled  = cfg.get<bool>("gerb").value_or(false);
    input_cmd    = cfg.get<int>("gerb_input").value_or(input_cmd);
    apply_row();
    lmt_bank     = cfg.get<int>("gerb_bank").value_or(lmt_bank);
    for (int i = 0; i < DIR_COUNT; ++i) slot[i] = cfg.get<int>("gerb_slot_" + std::to_string(i)).value_or(slot[i]);
    ctn_bank     = cfg.get<int>("gerb_ctn_bank").value_or(ctn_bank);
    ctn_slot     = cfg.get<int>("gerb_ctn_slot").value_or(ctn_slot);
    input_cmd_vertical = cfg.get<int>("gerb_input_vertical").value_or(input_cmd_vertical);
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
}

void Gerb::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gerb", mod_enabled);
    cfg.set<int>("gerb_input", input_cmd);
    cfg.set<int>("gerb_bank", lmt_bank);
    for (int i = 0; i < DIR_COUNT; ++i) cfg.set<int>("gerb_slot_" + std::to_string(i), slot[i]);
    cfg.set<int>("gerb_ctn_bank", ctn_bank);
    cfg.set<int>("gerb_ctn_slot", ctn_slot);
    cfg.set<int>("gerb_input_vertical", input_cmd_vertical);
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
}
