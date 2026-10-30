#include "UpperGun.hpp"
#include "MoveTable.hpp"             // HookNeroKADTbl / HookDanteKADTbl: the gun rows matched here
#include "SpineAim.hpp"              // inertialize: the blends
#include "../sdk/Devil4.hpp"         // request_motion_body
#include "../sdk/uActor.hpp"
#include "../sdk/InputCommand.hpp"   // the pre-check and post-action ticks
#include "../sdk/Enums_Internal.hpp" // CANCEL_NO, MOTION_CTRL
#include "../utility/FunctionHook.hpp"
#include <algorithm>
#include <cstdio>
#include <memory>

using namespace wallrun;

namespace upper_gun {

int  shots = 0;
char last[96] = "";

namespace {

constexpr uintptr_t fn_setActionAtck = 0x007AAC80;   // uPlayer::setActionAtck(this@eax, atckId@ecx, body on the stack) userpurge: body 0 = the upper half only
constexpr uintptr_t fn_setPoseWeapon = 0x007AADE0;   // uPlayer::setPoseWeapon(this@edi) -> al: mPose.wpnId = the long weapon when it can pose, else -1

Host g_hosts[kMaxHosts];
int  g_num_hosts = 0;
bool g_installed = false;
std::unique_ptr<FunctionHook> g_hook_pre, g_hook_post, g_hook_pose, g_hook_pose_upper;
uintptr_t jmp_ret_pose = 0;          // 0x7AA8C8
uintptr_t jmp_ret_pose_upper = 0;    // 0x7AA868

inline float tune(const float* p) { return p ? *p : 0.0f; }

// What the game's phase words can't hold, per player.
struct GunState {
    uintptr_t player = 0;
    uint32_t  last_seen = 0;
    int       host = -1;            // the host whose move the shots ride on (the one active when the last shot started)
    float     shot_age = -1.0f;     // ticks since the shot action started (-1 = none)
    float     gun_hold = -1.0f;     // frames left with the gun up after a shot ended (-1 = none): the arms keep the shot's last frame
    float     lean_w = 0.0f;        // 0-1, ramped by the post-action tick (hosts read it for their SpineAim lean)
    bool      gun_buffered = false; // a shot press waiting for the current shot's cancel window (the .ctn buffer window, mStat 1)
    uint32_t  shot_id = 0;          // the gun row started (0 = none)
    bool      words_saved = false;  // the words below hold the host's values from before the first shot
    int       action_no = 0;        // mActionNo / mActLimit / mActStat as the host's move had them
    uint32_t  act_limit = 0, act_stat = 0;
    bool      swapped = false;      // mAtckId holds the gun id (the upper pass), not the host's
};

GunState g_states[4];               // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)

// The ticks run for every uPlayer that ticks, and dead players are never reported, so a full table evicts the slot seen
// longest ago (a live player is seen every tick and keeps its slot).
GunState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (GunState& s : g_states) if (s.player == player) { s.last_seen = tick; return s; }
    GunState* victim = &g_states[0];
    for (GunState& s : g_states) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = GunState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

GunState* find_state(uintptr_t player) {   // no eviction (the GUI thread, the readout)
    for (GunState& s : g_states) if (s.player == player) return &s;
    return nullptr;
}

// The host whose move is active for this player, or -1.
int host_of(uintptr_t pl) {
    for (int i = 0; i < g_num_hosts; ++i) {
        const Host& h = g_hosts[i];
        if (h.enabled && !*h.enabled) continue;
        if (h.active && h.active(pl)) return i;
    }
    return -1;
}

inline bool upper_busy(uintptr_t pl) { const int u = at<int>(pl, kBdyUpperNo); return u == 12 || u == 13 || u == 16; }
inline uintptr_t gun_table(uintptr_t pl) { return at<int>(pl, pl::mPlayerID) == 1 ? HookNeroKADTbl : HookDanteKADTbl; }
inline void clear_cmd_bit(uintptr_t pl, uint32_t bit) { at<uint32_t>(pl, kCmdFlag + 4 * (bit >> 5)) &= ~(1u << (bit & 31)); }
inline uint32_t host_atck_id(const Host& h, uintptr_t pl) { return h.lower_atck_id ? h.lower_atck_id(pl) : 0; }

void face(uintptr_t pl, float yaw) {                 // snap, the way the wall jump does
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

void set_action_atck(uintptr_t pl, uint32_t id, int body) {
    uintptr_t fn = fn_setActionAtck;
    _asm {
        pushad
        mov eax, pl
        mov ecx, id
        push body
        call fn
        popad
    }
}

bool set_pose_weapon(uintptr_t pl) {
    uint8_t r = 0;
    uintptr_t fn = fn_setPoseWeapon;
    _asm {
        pushad
        mov edi, pl
        call fn
        mov byte ptr [r], al
        popad
    }
    return r != 0;
}

// The words the shot's setActionUpperBdy overwrites, as the host's move has them, saved before the first shot of a burst.
void save_words(GunState& s, uintptr_t pl) {
    if (s.words_saved) return;
    s.action_no = at<int>(pl, kActionNo);
    s.act_limit = at<uint32_t>(pl, pl::mActLimit);
    s.act_stat  = at<uint32_t>(pl, pl::mActStat);
    s.words_saved = true;
}

void restore_words(const GunState& s, uintptr_t pl) {
    if (s.words_saved) {
        at<int>(pl, kActionNo) = s.action_no;
        at<uint32_t>(pl, pl::mActLimit) = s.act_limit;
        at<uint32_t>(pl, pl::mActStat)  = s.act_stat;
    } else {
        at<int>(pl, kActionNo) = at<int>(pl, pl::mBdyActionLowerNo);
    }
}

// mAtckId back to the host's id (hosts whose lower body is attack 12).
void unswap(GunState& s, const Host& h, uintptr_t pl) {
    const uint32_t id = host_atck_id(h, pl);
    if (id) at<uint32_t>(pl, pl::mAtckId) = id;
    s.swapped = false;
}

// The upper-body shot, the way setActionAtckGun 0x7AAE20 starts it, minus the lower-body pose. A weapon change chosen since
// the last shot (L2 / R2: mLongId != mEnableLongId) is taken over as the game does: the row is the chosen gun's, its tables
// are set up here (vtbl 140) and the models swap at the end of this tick (setActionUpperBdy enables cWeaponCtrl::update).
// Until 2026-10-30 the row was matched against the held gun, so a switch mid-shot started the old gun's row over the new
// weapon: a glitch or a crash (Pandora's).
void start_upper_shot(GunState& s, const Host& h, uintptr_t pl, uint32_t id) {
    if (!upper_busy(pl)) save_words(s, pl);
    if (!at<bool>(pl, kIsWpChangeCancel)) {
        at<uint32_t>(pl, kWpGripId) = at<uint32_t>(pl, kWpEnableLongId);
        if (at<uint32_t>(pl, kWpEnableLongId) != at<uint32_t>(pl, kWpLongId)) {
            at<bool>(pl, kPoseIsHoldGun) = false;
            using SetupFn = void(__thiscall*)(uintptr_t);
            (*(SetupFn*)(*(uintptr_t*)pl + kVtSetupLongWeaponChange))(pl);
        }
    }
    set_pose_weapon(pl);                                 // mPose.wpnId = the weapon when it can pose (Blue Rose, E&I); the Coyote-A can't and shoots anyway
    set_action_atck(pl, id, 0);
    s.shot_id = id;
    unswap(s, h, pl);                                    // the lower pass comes next: the host's dispatch needs its own id
    ++shots;
    snprintf(last, sizeof last, "%s: shot id %u", h.name, id);
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
// must pass, one passing cmdOr entry is enough on its own; cmdId entries (bit 1) need one equal to the current attack id.
// mStat 1 (the .ctn high nibble 0x30) only buffers the input, 2 (0x10) starts the move.
CancelVerdict atck_cancel(uintptr_t pl, const kAtckDefTbl* row, uint32_t current_id) {
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
            if ((uint32_t)val == current_id) id_hit = true;
        }
    }
    if (or_open) return CANCEL_OPEN;
    if (and_any && and_ok && (!id_any || id_hit)) return and_buffer ? CANCEL_BUFFER : CANCEL_OPEN;
    return or_buffer ? CANCEL_BUFFER : CANCEL_SHUT;
}

// Pre-check tick: the character's ground plain-shot row, matched here and started upper-only; checkAtck then sees no
// press, no gun reserve and no charge command.
void precheck_tick(uintptr_t pl) {
    const int hi = host_of(pl);
    if (hi < 0) return;
    const Host& h = g_hosts[hi];
    GunState& s = state_for(pl);
    s.host = hi;
    // Pandora (Dante's long weapon 8) has no place here (its row started upper-only crashed the game): no shot, nothing
    // touched, only the press eaten so checkAtck doesn't start its air row. The chosen gun is what the row would be.
    if (at<int>(pl, pl::mPlayerID) != 1 && (at<uint32_t>(pl, kWpLongId) == 8 || at<uint32_t>(pl, kWpEnableLongId) == 8)) {
        at<uint32_t>(pl, pl::mBtnTrg) &= ~0x2u;
        at<bool>(pl, kMotCncl + kMotCnclSize * C_NORM0 + kMotCnclReserve) = false;
        at<bool>(pl, kMotCncl + kMotCnclSize * C_GUN   + kMotCnclReserve) = false;
        s.gun_buffered = false;
        return;
    }
    // The pose table's STANDING row (mPose.dir 0, what setPoseDir gives with the stick centred: blocks 0502-0505) is
    // full-body on both characters, legs and root included; on the upper controller it froze the legs over the run
    // cycle. The walking rows are upper-body blocks. Pin the direction to front (1) every active tick, before a shot
    // starts: the shot setters pick their row from it (a footwork leaves 0 behind).
    at<int>(pl, kPoseDir) = 1;
    const kAtckDefTbl* row = (const kAtckDefTbl*)gun_table(pl);
    if (!row) return;
    const uint32_t wp = at<uint32_t>(pl, kWpLongId);                   // the chosen gun, as checkAtckWeapon tests the rows
    if (at<uint32_t>(pl, pl::mBtnTrg) & 0x2) s.gun_buffered = true;    // a press waits for the shot's cancel window (the
    const bool mid_shot = at<int>(pl, kBdyUpperNo) == 12;              // game's buffer: mStat 1 holds it, 2 fires it)
    if (!mid_shot && !(at<uint32_t>(pl, pl::mBtnTrg) & 0x2)) s.gun_buffered = false;   // between shots only a fresh press fires
    const uint32_t current_id = s.shot_id ? s.shot_id : at<uint32_t>(pl, pl::mAtckId);   // the shot's own id, whatever mAtckId holds now
    bool started = false;
    for (; row->atckAttr != 3; ++row) {
        if (row->atckAttr != 1 || row->atckAs != 1) continue;             // a gun row, its ground version
        if (row->command.atckCommandNo) continue;                          // disabled row (MoveTable's convention)
        if (row->atckConditionWp != 0xFFFFFFFFu && row->atckConditionWp != wp) continue;
        const uint32_t cmd = row->command.atckCommand;
        if (cmd) { clear_cmd_bit(pl, cmd); continue; }                     // a charge release: consumed, never fired here
        if (started || !s.gun_buffered) continue;
        // checkAtck's order: unlocked, the extra condition (the E&I / Coyote-A follow-up rows 61-63 / 72-74 come first
        // and pass only inside their rapid window), then mid-shot the string rules: AI_REPEAT / AI_CANCEL (atckLevel
        // here is the RE note's atckInfo: 7 on every gun row) and the cancel windows the shot's .ctn opens
        if (!atck_unlocked(pl, row->atckId) || !atck_etc_ok(pl, row->ukn)) continue;
        if (mid_shot) {
            if (!(row->atckLevel & 2)) continue;                                                // AI_CANCEL
            if (row->atckId == current_id && !(row->atckLevel & 4)) continue;                   // AI_REPEAT
            const CancelVerdict v = atck_cancel(pl, row, current_id);
            if (v == CANCEL_SHUT) { s.gun_buffered = false; break; }      // as checkAtck: a press outside the windows is lost
            if (v == CANCEL_BUFFER) break;                                 // held until the window opens (mStat 2)
        }
        start_upper_shot(s, h, pl, row->atckId);
        started = true;
        s.gun_buffered = false;
        s.shot_age = 0.0f;
        s.gun_hold = -1.0f;                                               // the next shot comes in from the held gun
    }
    at<uint32_t>(pl, pl::mBtnTrg) &= ~0x2u;
    at<bool>(pl, kMotCncl + kMotCnclSize * C_NORM0 + kMotCnclReserve) = false;
    at<bool>(pl, kMotCncl + kMotCnclSize * C_GUN   + kMotCnclReserve) = false;
}

// The host's move ends, or the shot is over: the upper half must not stay in the shot / pose, or moveActPoseU keeps
// asking for the gun stance over the next whole-body motion; and the words go back to the host's.
void end_upper_shot(GunState& s, const Host* h, uintptr_t pl) {
    s.shot_age = -1.0f;
    if (h && s.swapped) unswap(s, *h, pl);
    if (!upper_busy(pl)) return;
    at<int>(pl, kBdyUpperNo)    = at<int>(pl, pl::mBdyActionLowerNo);
    at<int>(pl, kBdyUpperPhase) = 0;
    restore_words(s, pl);
    s.words_saved = false;
    s.shot_id = 0;
}

// The shot is over (its last phase, through the setActionPoseUpperBdy hook below, or a safety net): the upper action
// ends and the host's lower motion itself goes on the upper and aim controllers, at the legs' frame. A slot given a new
// motion crossfades from a snapshot of the pose on screen (uModel::moveMotion copies the joints into the slot when
// mState & 2), so the arms blend back over back_blend_frames, and the next shot blends in from there (upper_blend_min).
// The same motion at the same frame as the legs looks exactly like the legs' motion; the next whole-body request clears
// both controllers.
void back_to_lower(GunState& s, const Host& h, uintptr_t pl) {
    end_upper_shot(s, &h, pl);
    s.gun_hold = -1.0f;
    Motion m;
    if (!h.lower_motion || !h.lower_motion(pl, &m) || m.motion < 0) return;
    if (h.before_back) h.before_back(pl);
    const float frame = at<float>(pl, kModelMotion0Frame);
    const float back = tune(h.back_blend_frames);
    const float interp = back > 1.0f ? back : 1.0f;
    for (int body : { MC_UPPER, MC_AIM })   // mirrored to the coat's 5 / 6 by requestSetMotion
        devil4_sdk::request_motion_body((void*)pl, body, m.motion, m.attr, interp, frame, m.speed);
    if (spine_aim::inertialize(pl, interp))  // the body's controllers cut to the motion, the fade takes the frames (the coat keeps the crossfade)
        for (int body : { MC_UPPER, MC_AIM }) at<float>(pl + kMotionCtrl + kMotionCtrlSize * body, kMcInterp) = 0.0f;
}

void post_action_tick(uintptr_t pl) {
    const int hi = host_of(pl);
    if (hi < 0) return;
    const Host& h = g_hosts[hi];
    GunState& s = state_for(pl);
    s.host = hi;
    if (s.swapped) unswap(s, h, pl);                       // the upper pass is over: the host's id until the next lower pass
    if (h.clear_cancel_gate) at<bool>(pl, kIsCancelCheckEnable) = false;
    // A shot is running while shot_id is set (start_upper_shot .. end_upper_shot), not while the upper half reads as busy:
    // a host whose own action is attack 12 on both halves (FreeRide) is upper-busy all the time, which kept this branch
    // and never reached the hold's countdown below, so the arms looped the shot for good (in game 2026-10-30).
    if (s.shot_id != 0) {
        restore_words(s, pl);                               // checkAction: the host's case, not 12 / 13 / 16; the limits as the host left them
        float yaw;
        if (h.facing && h.facing(pl, &yaw)) face(pl, yaw);  // the shot's setLockOnTargetAngle turned the facing toward the target
        // the shot's upper and aim requests stand, but with at least upper_blend_min frames of interpolation: the shot
        // cuts in with the draw timer as its blend, 0 once the gun counts as held, a snap from the legs' arms. With
        // SpineAim's inertial blends, a new motion on the body's controllers cuts in (0) and the fade takes the frames
        // instead; the coat's (5 / 6) keep the crossfade (its tails hang on the body and swing anyway)
        const float blend_min = tune(h.upper_blend_min);
        if (blend_min > 0.0f) {
            float frames = 0.0f;
            for (int i : { 1, 2, 5, 6 }) {
                const uintptr_t c = pl + kMotionCtrl + kMotionCtrlSize * i;
                if (!at<bool>(c, kMcMotionSetReq)) continue;
                if (at<float>(c, kMcInterp) < blend_min) at<float>(c, kMcInterp) = blend_min;
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
        Motion m;
        const bool known = h.lower_motion && h.lower_motion(pl, &m);
        if (at<bool>(c0, kMcMotionSetReq) && (!known || (int)at<uint32_t>(c0, kMcMotionNo) != m.motion))
            for (int i : { 0, 3, 4 }) at<bool>(pl + kMotionCtrl + kMotionCtrlSize * i, kMcMotionSetReq) = false;   // lower, face, coat lower
        for (int i : { 0, 3, 4 }) at<int16_t>(pl + kMotionCtrl + kMotionCtrlSize * i, kMcAddAttribute) &= ~A_STOP_BIT;
        // safety: a shot action that never reaches its end (phase 4 needs its upper motion's end) is ended here
        const float end_ticks = tune(h.shot_end_ticks);
        if (end_ticks > 0.0f && at<int>(pl, kBdyUpperNo) == 12) {
            if (s.shot_age >= 0.0f) s.shot_age += at<float>(pl, pl::m_delta_time);
            if (s.shot_age >= end_ticks) back_to_lower(s, h, pl);
        }
        // a shot whose upper motion is gone (cleared by a whole-body request; a stuck shot blocks every later press)
        // never reaches its end: end it here
        const uintptr_t c1 = pl + kMotionCtrl + kMotionCtrlSize * MC_UPPER;
        if (at<int>(pl, kBdyUpperNo) == 12 && at<uint32_t>(c1, kMcMotionNo) == 0xFFFF && !at<bool>(c1, kMcMotionSetReq))
            back_to_lower(s, h, pl);
    } else if (s.gun_hold >= 0.0f) {                       // the gun held after a shot: back to the legs' motion when it runs out
        s.gun_hold -= at<float>(pl, pl::m_delta_time);
        if (s.gun_hold <= 0.0f) back_to_lower(s, h, pl);
    }
    // the lean comes in and goes out over lean_blend_frames instead of popping with the upper action
    {
        const float target = (s.shot_id != 0 || s.gun_hold >= 0.0f) ? 1.0f : 0.0f;
        const float blend = tune(h.lean_blend_frames);
        const float step = blend > 0.0f ? at<float>(pl, pl::m_delta_time) / blend : 1.0f;
        s.lean_w = target > s.lean_w ? std::min(target, s.lean_w + step) : std::max(target, s.lean_w - step);
    }
}

// uPlayer::setActionPoseLowerBdy 0x7AA8C0 (this@esi, no stack arguments): the lower body into the gun pose. While a
// host's move is active it returns at once. The 8 bytes replaced are `push ecx; cmp dword ptr [esi+141Ch], 0`.
bool __stdcall pose_lower_blocked(uintptr_t pl) { return host_of(pl) >= 0; }
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
// and Blue Rose charge shots): the upper body into the gun pose. While a host's move is active it goes back to the legs'
// motion instead (back_to_lower), after gun_hold_frames with the gun up: the shot action ends at once (the next press
// fires as between shots), but nothing is requested on the upper / aim controllers, so the arms keep the shot's last frame
// and a burst doesn't dip between shots. The same 8 bytes as setActionPoseLowerBdy's are replaced.
bool __stdcall pose_upper_redirect(uintptr_t pl) {
    const int hi = host_of(pl);
    if (hi < 0) return false;
    const Host& h = g_hosts[hi];
    GunState& s = state_for(pl);
    const float hold = tune(h.gun_hold_frames);
    if (hold > 0.0f) { end_upper_shot(s, &h, pl); s.gun_hold = hold; }
    else back_to_lower(s, h, pl);
    return true;
}
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

// A hook on `location` continuing `skip` bytes later (ModBase::install_hook_absolute without the Mod).
bool hook(uintptr_t location, std::unique_ptr<FunctionHook>& h, void* detour, uintptr_t* ret, ptrdiff_t skip) {
    spdlog::info("UpperGun: installing hook at {:x}", location);
    h = std::make_unique<FunctionHook>(location, detour);
    if (!h->create()) { spdlog::error("UpperGun: failed to hook {:x}", location); h.reset(); return false; }
    *ret = location + skip;
    return true;
}

bool install() {
    if (g_installed) return true;
    if (!input_commands::hook_installed) {
        if (!hook(0x7A83B5, g_hook_pre, &input_commands::detour, &input_commands::jmp_ret, 8)) return false;
        input_commands::hook_installed = true;
    }
    if (!input_commands::post_hook_installed) {
        if (!hook(0x7A8453, g_hook_post, &input_commands::detour_post, &input_commands::jmp_ret_post, 8)) return false;
        input_commands::post_hook_installed = true;
    }
    if (!hook(0x7AA8C0, g_hook_pose, &detour_pose_lower, &jmp_ret_pose, 8)) return false;
    if (!hook(0x7AA860, g_hook_pose_upper, &detour_pose_upper, &jmp_ret_pose_upper, 8)) return false;
    input_commands::add_tick(&precheck_tick);
    input_commands::add_post_tick(&post_action_tick);
    g_installed = true;
    return true;
}

} // namespace

bool add_host(const Host& host) {
    if (g_num_hosts >= kMaxHosts) { spdlog::error("UpperGun: no room for host '{}'", host.name); return false; }
    if (!install()) return false;
    g_hosts[g_num_hosts++] = host;
    return true;
}

bool busy(uintptr_t pl) {
    if (upper_busy(pl)) return true;
    const GunState* s = find_state(pl);
    return s && s->gun_hold >= 0.0f;
}

float lean_weight(uintptr_t pl) {
    const GunState* s = find_state(pl);
    return s ? s->lean_w : 0.0f;
}

void end(uintptr_t pl) {
    GunState* s = find_state(pl);
    if (!s) return;
    const Host* h = (s->host >= 0 && s->host < g_num_hosts) ? &g_hosts[s->host] : nullptr;
    end_upper_shot(*s, h, pl);
    s->gun_hold = -1.0f;
    s->gun_buffered = false;
    s->lean_w = 0.0f;
}

// The host's lower pass is done; the upper pass (moveActionUpper -> moveActAtckU, which switches on mAtckId) comes next.
void lower_pass_done(uintptr_t pl) {
    GunState* s = find_state(pl);
    if (!s || !s->shot_id || at<int>(pl, kBdyUpperNo) != 12) return;
    at<uint32_t>(pl, pl::mAtckId) = s->shot_id;
    s->swapped = true;
}

uint32_t effective_atck_id(uintptr_t pl) {
    const GunState* s = find_state(pl);
    if (s && s->swapped && s->host >= 0 && s->host < g_num_hosts) {
        const uint32_t id = host_atck_id(g_hosts[s->host], pl);
        if (id) return id;
    }
    return at<uint32_t>(pl, pl::mAtckId);
}

bool row_allowed(uintptr_t pl, uint32_t atck_id, uint32_t etc) { return atck_unlocked(pl, atck_id) && atck_etc_ok(pl, etc); }
bool buffered(uintptr_t pl) { const GunState* s = find_state(pl); return s && s->gun_buffered; }
uint32_t shot_id(uintptr_t pl) { const GunState* s = find_state(pl); return s ? s->shot_id : 0; }
bool shooting(uintptr_t pl) { const GunState* s = find_state(pl); return s && (s->shot_id != 0 || s->gun_hold >= 0.0f); }

} // namespace upper_gun
