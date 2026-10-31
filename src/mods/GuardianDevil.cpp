#include "GuardianDevil.hpp"
#include "../sdk/NeroStand.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/StringData.hpp"
#include "../sdk/MotionRegistry.hpp"
#include <cmath>
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#define WIN32_LEAN_AND_MEAN
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

using namespace wallrun;
namespace st = nero_stand;

namespace guardian_devil {

bool  mod_enabled          = false;
int   trigger_source       = TRIG_PAD;
int   trigger_pad          = 0x100;
int   trigger_key          = 0x54;    // T
int   trigger_game         = 0x100;
float cooldown_frames      = 30.0f;
bool  cancel_on_hit        = false;
bool  rush_effect          = true;
float height_offset        = 0.0f;
bool  hold_stays_put       = false;
bool  dt_cancel_keeps_hold = false;
bool  taunt_mimic          = true;
char  taunt_lmt[128]       = "motion\\pl020\\pl020_taunt";
char  taunt_arm_lmt[128]   = "motion\\wp024\\wp024_taunt";
float taunt_back           = 80.0f;
float taunt_side           = 0.0f;
static const ActionSet kDefaultSet = { true, 0x31C, 0x317, 0x330, 0x35F, 135.0f, 170.0f, 172.0f, 150.0f };
ActionSet sets[nero_style::kSlots] = { kDefaultSet, kDefaultSet, kDefaultSet, kDefaultSet };

namespace {

// ---- The game ---------------------------------------------------------------------------------------------------
char* const kRushEffectRun = "effect\\efl\\vie\\vi020_00v0";   // while the rush runs
char* const kRushEffectEnd = "effect\\efl\\vie\\vi020_00v1";   // when it ends
constexpr uintptr_t kCmdFlagWords = 0x2184;                    // uPlayer::mCmdFlag[3]
constexpr uintptr_t kPadBtnOn = 0x1374;                        // uPlayer::mPeripheral.mPadBtnOn: the raw pad bits
constexpr uint32_t  kActProvoke = 0x17;                        // Nero's lower action while he taunts
int g_taunt_set = -1;                                          // the registry handle of the taunt set
// Nero's taunt motions (uPlayerNero::setProvokeMtn 0x7E6F30: three tables by style rank), bank 0 so word == block
bool is_taunt_motion(uint32_t m) { return (m >= 0x96 && m <= 0x9E) || m == 0xA5 || m == 0xA6; }
const char* const kDataFile = ".\\nativePC\\Collision\\wp024.col";

// Hook sites (uPlNeroDevil / uPlayerNero, DX9) and where their detours continue or jump. The resume addresses are
// filled by install_hook_absolute; the others are the game's own branch targets.
constexpr uintptr_t kGateSite       = 0x82A841;   // moveActYamato: `test al,al; jz 82A9D5` on checkPlayerAction's result (8 bytes)
uintptr_t           kGateCancel     = 0x82A9D5;   //   the cancel branch (arm cancelAction, fade out, mActionNo = 0)
constexpr uintptr_t kPhase0Site     = 0x82A8A5;   // moveActYamato phase 0: `cmp byte [edx+0CDF4h],1; jnz 82AA6F` (13 bytes)
uintptr_t           kPhase0Wait     = 0x82AA6F;   //   Nero's mimic event not set yet: wait
uintptr_t           kPhase0Tail     = 0x82A93E;   //   past the trace / detach placement: flags, arm reset, setMotion(mYamatoParam)
constexpr uintptr_t kPositionSite   = 0x8292E3;   // uPlNeroDevil::updatePosition: `cmp byte [esi+22C8h],1` (7 bytes), right after moveMotion; esi = stand
constexpr uintptr_t kFollowSite     = 0x828DC3;   // moveEx, not tracing: `movss xmm1,[eax+0D650h]` (8 bytes), Nero's push-back
uintptr_t           kFollowEnd      = 0x828E85;   //   after the push-back / ground height / yaw -> quat block (the DT read)
constexpr uintptr_t kDtRequestSite  = 0x7E6E2C;   // uPlayerNero::setRedQueenMotion: `cmp al,1; jnz 7E6E8D; push edi` (5 bytes), al = isDevilTrigger
uintptr_t           kDtRequestSkip  = 0x7E6E8D;   //   no DT: the stand isn't told
constexpr uintptr_t kKabutoSite     = 0x82B850;   // uPlNeroDevil::setYamatoMotionStop: `cmp byte [eax+22C9h],1` (7 bytes), then `jnz retn`
uintptr_t           kKabutoReturn   = 0x82B8A1;   //   its retn
constexpr uintptr_t kDtAtOnceSite   = 0x7E5316;   // uPlayerNero::checkActionDevilTrgAtOnce: `cmp dword [eax+22A8h],7; jz 7E5338` (9 bytes), eax = stand
uintptr_t           kDtAtOnceRefuse = 0x7E5338;   //   `xor al,al; retn`
constexpr uintptr_t kDtAppearSite   = 0x7F8DF0;   // uPlayerNero::moveActDevilTrg phase 0: `call uPlNeroDevil::setAction` (5 bytes), eax = 1 Appear, edi = stand
constexpr uintptr_t kDtAppearAirSite = 0x7F90B6;  // uPlayerNero::moveActDevilTrgAir: the same call
uintptr_t           kSetAction      = st::fn_setAction;

// The stand's DT reads that become kShown reads: the low byte of each `[reg+22C9h]` displacement -> 0xD3.
constexpr uintptr_t kShownPatches[10] = {
    0x828BA8,   // main: local + world matrices
    0x828E87,   // moveEx: body matrices
    0x828EEA,   // moveEx: sword matrices
    0x828FD5,   // sync: the sword
    0x829087,   // updateMaterial
    0x8290C7,   // draw
    0x8295C4,   // updateMatrix
    0x8298C6,   // updateTransparency: the arm's alpha follows the body
    0x8299AF,   // updateTransparency: the DT aura effects
    0x82A84B,   // moveActYamato: the mimic's DT gate
};

uint8_t g_dt_cancel = 0;   // mod_enabled && dt_cancel_keeps_hold, for the detours

// ---- Per-player state ---------------------------------------------------------------------------------------------
constexpr int kPlayerSlots = 4;
enum Phase : int { IDLE = 0, ATTACK = 1, FADE = 2 };

struct State {
    uintptr_t   player = 0;
    uint32_t    last_seen = 0;
    int         phase = IDLE;
    float       cooldown = 0.0f;
    int         motion = -1;          // the running attack's motion
    int         slot = 0;             // its style slot
    uintptr_t   effect = 0;           // the rush's running effect
    bool        effect_ended = false; // its end effect played (the button let go in the loop window)
    bool        hold_detached = false;// hold_stays_put took the stand off Nero
    float       hold_pos[3] = {};     // where it was when the hold began (the hold motions' root motion is undone to it)
    bool        held = false;         // the pad / key trigger was down last tick (the press edge)
    bool        taunting = false;     // the running attack is a taunt mimic (ends with Nero's taunt)
    bool        taunt_tried = false;  // this taunt of Nero's was answered (or refused) already
    uint32_t    starts = 0, refused = 0;
    const char* last_refusal = "";
};
State g_states[kPlayerSlots];

State& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (State& s : g_states) if (s.player == player) { s.last_seen = tick; return s; }
    State* victim = &g_states[0];
    for (State& s : g_states) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = State{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}
State* find_state(uintptr_t player) {
    for (State& s : g_states) if (s.player == player) return &s;
    return nullptr;
}

bool is_nero(uintptr_t pl) { return pl && at<uint32_t>(pl, pl::mPlayerID) == 1; }

// ---- The commands -------------------------------------------------------------------------------------------------
enum Cmd : int { CMD_RUSH = 0, CMD_FORWARD = 1, CMD_BACK = 2, CMD_NEUTRAL = 3, CMD_COUNT = 4 };
input_commands::Command g_cmd[CMD_COUNT];
int g_cmd_bit[CMD_COUNT] = { -1, -1, -1, -1 };
const char* const kCmdNames[CMD_COUNT] = { "rush", "forward", "back", "neutral" };

// The game's own heads with the trigger in place of melee: 0x52 (Calibur: centred, back within 15, forward within 15,
// the button within 15), 3 (forward within 2, the button), 4 (back within 2, the button); the neutral one is the button.
void build_commands() {
    using namespace input_commands;
    const uint32_t btn = trigger_bit();
    g_cmd[CMD_RUSH].condition(CND_POSE).lever(LEVER_CENTRED).lever(LEVER_BACK, LEVER_TARGET_45, 15).lever(LEVER_FORWARD, LEVER_TARGET_45, 15).button(btn);
    g_cmd[CMD_FORWARD].condition(CND_POSE).lever(LEVER_FORWARD, LEVER_TARGET_45, 2).button(btn);
    g_cmd[CMD_BACK].condition(CND_POSE).lever(LEVER_BACK, LEVER_TARGET_45, 2).button(btn);
    g_cmd[CMD_NEUTRAL].button(btn);
}

// The pad / key trigger: down now, and the press edge from the state's last tick.
bool keyboard_allowed() { return !g_framework->m_draw_ui && GetForegroundWindow() == g_framework->get_window_handle(); }
bool trigger_down(uintptr_t pl) {
    switch (trigger_source) {
    case TRIG_PAD:  return trigger_pad != 0 && (at<uint32_t>(pl, kPadBtnOn) & (uint32_t)trigger_pad) == (uint32_t)trigger_pad;
    case TRIG_KEY:  return trigger_key != 0 && keyboard_allowed() && (GetAsyncKeyState(trigger_key) & 0x8000) != 0;
    case TRIG_GAME: return (at<uint32_t>(pl, pl::mBtnOn) & (uint32_t)trigger_game) != 0;
    default: return false;
    }
}

bool bit_set(uintptr_t pl, int bit) { return bit >= 0 && (at<uint32_t>(pl, kCmdFlagWords + 4 * (bit >> 5)) & (1u << (bit & 31))) != 0; }
void bit_clear(uintptr_t pl, int bit) { if (bit >= 0) at<uint32_t>(pl, kCmdFlagWords + 4 * (bit >> 5)) &= ~(1u << (bit & 31)); }

// ---- Lock-outs -------------------------------------------------------------------------------------------------------
// Nero's lower-body actions (uPlayer::moveActionLower) and attack ids that refuse a start, or end a running attack with
// cancel_on_hit.
bool nero_locked_out(uintptr_t pl, const char** why) {
    switch (at<uint32_t>(pl, pl::mBdyActionLowerNo)) {
    case 0x14: *why = "Nero is hit";          return true;   // Damage: hits, knock-downs, enemy grabs
    case 0x15: *why = "Nero is dead";         return true;
    case 0x16: *why = "Nero revives";         return true;
    case 0x1A: *why = "catapult";             return true;
    case 0x1B: *why = "holy water";           return true;
    case 0x1C: case 0x1D: case 0x1E: case 0x1F: *why = "Nero is flipped"; return true;
    case 0x20: case 0x21: case 0x22: *why = "DT activation"; return true;
    case 0x0C:
        switch (at<uint32_t>(pl, pl::mAtckId)) {
        case 1: case 2:    *why = "Snatch";        return true;
        case 5: case 6:    *why = "Buster";        return true;
        case 7: case 8:    *why = "Buster block";  return true;
        case 0x22:         *why = "Showdown";      return true;
        case 0x23:         *why = "Maximum Bet";   return true;
        default: break;
        }
        break;
    default: break;
    }
    return false;
}

// ---- The attack ------------------------------------------------------------------------------------------------------
void stop_effect(State& s) {
    if (s.effect) devil4_sdk::effect_cleanup(s.effect);
    s.effect = 0;
}

void write_bytes(uintptr_t stand, const State& s) {
    uint8_t mod = 0;
    if (s.phase == ATTACK)  mod |= st::MOD_ATTACK;
    if (s.hold_detached)    mod |= st::MOD_HOLD;
    if (s.phase != IDLE)    mod |= st::MOD_PLACED;
    at<uint8_t>(stand, st::kMod) = mod;
    at<uint8_t>(stand, st::kShown) = (at<uint8_t>(stand, st::mIsDevilTrigger) || s.phase != IDLE) ? 1 : 0;
}

// The start: what setRedQueenMotion does in DT, on a stand placed on Nero (`back` / `side` cm from him along his
// facing) and detached, fading in at once. `taunt`: the motion is Nero's taunt (no lock-out on his taunt action).
void start(uintptr_t pl, uintptr_t stand, State& s, int slot, int motion, bool taunt = false, float back = 0.0f, float side = 0.0f) {
    const ActionSet& set = sets[slot];
    const char* why = nullptr;
    const int action = at<int>(stand, st::mActionNo);
    if (!set.enabled)                                            why = "no stand in this style";
    else if (motion < 0)                                         why = taunt ? "the taunt LMTs aren't on the stand yet" : "no motion for this input";
    else if (action != st::ACT_NONE && action != st::ACT_YAMATO) why = "the stand is busy";
    else if (at<uint8_t>(stand, st::mIsDevilTriggerReserve))     why = "a DT change is pending";
    else nero_locked_out(pl, &why);
    if (why) { ++s.refused; s.last_refusal = why; return; }

    std::memcpy(&at<float>(stand, st::mPos), &at<float>(pl, pl::mPos), 12);
    at<float>(stand, st::mPos + 4) += height_offset;
    if (back != 0.0f || side != 0.0f) {                          // game axes: forward = (sin yaw, 0, cos yaw), right = (-cos, 0, sin)
        const float yaw = at<float>(pl, st::pl_mFacingYaw);
        at<float>(stand, st::mPos)     += -std::sin(yaw) * back - std::cos(yaw) * side;
        at<float>(stand, st::mPos + 8) += -std::cos(yaw) * back + std::sin(yaw) * side;
    }
    std::memcpy(&at<float>(stand, st::mQuat), &at<float>(pl, pl::mQuat), 16);
    at<float>(stand, st::mAnglY) = at<float>(pl, st::pl_mFacingYaw);
    at<uint8_t>(stand, st::mIsTraceParent) = 0;
    st::set_action(stand, st::ACT_YAMATO);
    const bool rush = !taunt && motion == set.rush;
    st::yamato_param(stand, motion, 4, rush ? set.rush_start : 0.0f);
    at<int>(stand, st::mTpPhase) = st::TP_FADE_IN;
    if (const uintptr_t target = at<uintptr_t>(pl, st::pl_mLockOnTarget)) st::set_snatch_param(stand, target);

    stop_effect(s);
    if (rush && rush_effect) s.effect = (uintptr_t)devil4_sdk::effect_generator(kRushEffectRun, (void*)stand, 0x14);
    s.effect_ended = false;
    s.phase = ATTACK;
    s.motion = motion;
    s.slot = slot;
    s.taunting = taunt;
    s.cooldown = cooldown_frames;
    ++s.starts;
    write_bytes(stand, s);
}

// The stand's action left 4: the motion ended (phase 1's setAction(0)) or the game cancelled it. The stand stays where
// it attacked while it fades out (MOD_PLACED keeps moveEx's follow code off); it follows Nero again once hidden.
void end_attack(uintptr_t stand, State& s) {
    stop_effect(s);
    s.phase = FADE;
    s.taunting = false;
}

// Nero started a taunt: the stand plays the same taunt beside him, once the registry has the files on it.
void taunt_tick(uintptr_t pl, uintptr_t stand, State& s) {
    const bool provoking = at<uint32_t>(pl, pl::mBdyActionLowerNo) == kActProvoke;
    if (!provoking) s.taunt_tried = false;
    if (s.phase == ATTACK && s.taunting && !provoking) {         // Nero stopped taunting: the stand stops too (the game's gate cancels it)
        stop_effect(s);
        s.phase = FADE;
        s.taunting = false;
        return;
    }
    if (!taunt_mimic || !provoking || s.taunt_tried || s.phase == ATTACK) return;
    const uint32_t motion = at<uint32_t>(pl, pl::mMotionCtrl0No);
    if (!is_taunt_motion(motion)) return;                        // the taunt motion is requested a tick after the action starts
    s.taunt_tried = true;
    int word = -1;
    if (g_taunt_set >= 0 && motion_registry::ready(g_taunt_set, pl)) word = motion_registry::word(g_taunt_set, pl, (int)motion);
    start(pl, stand, s, slot_for(pl), word, true, taunt_back, taunt_side);
}
void end_fade(uintptr_t stand, State& s) {
    at<uint8_t>(stand, st::mIsTraceParent) = 1;
    s.phase = IDLE;
}

// The rush: while the trigger is held, a frame in the loop window jumps back; let go, the end effect plays once.
void rush_loop(uintptr_t pl, uintptr_t stand, State& s) {
    const ActionSet& set = sets[s.slot];
    const float frame = at<float>(stand, st::mFrame0);
    if (s.motion == set.rush) {
        if (frame < set.loop_from || frame > set.loop_to) return;
        if (trigger_down(pl)) {
            at<float>(stand, st::mFrame0) = set.loop_dest;
            if (const uintptr_t hand = st::hand_of(stand)) at<float>(hand, st::mFrame0) = set.loop_dest;
        } else if (!s.effect_ended) {
            stop_effect(s);
            if (rush_effect) devil4_sdk::effect_generator(kRushEffectEnd, (void*)stand, 0x14);
            s.effect_ended = true;
        }
    } else if (s.effect && frame >= 2.0f) {
        stop_effect(s);
    }
}

// The mod turned off (or never on) for a player whose stand still carries our state: give the stand back.
void release(uintptr_t stand, State& s) {
    stop_effect(s);
    if (s.phase != IDLE || s.hold_detached) at<uint8_t>(stand, st::mIsTraceParent) = 1;
    s.phase = IDLE;
    s.hold_detached = false;
    s.taunting = false;
    at<uint8_t>(stand, st::kMod) = 0;
    at<uint8_t>(stand, st::kShown) = at<uint8_t>(stand, st::mIsDevilTrigger);
}

// updatePosition, right after moveMotion applied the motion's root motion: a held stand goes back to where the hold began.
void __stdcall hold_pin(uintptr_t stand) {
    const State* s = find_state(at<uintptr_t>(stand, st::mpParent));
    if (s && s->hold_detached) std::memcpy(&at<float>(stand, st::mPos), s->hold_pos, 12);
}

// ---- The detours -------------------------------------------------------------------------------------------------------
// Every hook tests the stand's kMod byte (edi / esi / eax / ecx = the stand at its site) and does nothing while it is 0.
uintptr_t jmp_ret_gate = 0, jmp_ret_phase0 = 0, jmp_ret_follow = 0, jmp_ret_position = 0, jmp_ret_dt_request = 0, jmp_ret_kabuto = 0,
          jmp_ret_dt_at_once = 0, jmp_ret_dt_appear = 0, jmp_ret_dt_appear_air = 0;

// moveActYamato's gate: a guardian attack keeps going whatever Nero does (checkPlayerAction only allows Red Queen attacks).
naked void detour_gate() {
    _asm {
        test byte ptr [edi + 0x22D9], 1
        jz orig
        mov al, 1
    orig:
        test al, al
        jz cancel
        jmp dword ptr [jmp_ret_gate]
    cancel:
        jmp dword ptr [kGateCancel]
    }
}

// moveActYamato phase 0: a guardian attack doesn't wait for Nero's mimic event and skips the trace / detach placement
// (the start placed and detached the stand); the game's `mIsTraceParent = 1` there is written as 0 instead.
naked void detour_phase0() {
    _asm {
        test byte ptr [edi + 0x22D9], 1
        jz orig
        mov byte ptr [edi + 0x22C8], 0
        jmp dword ptr [kPhase0Tail]
    orig:
        cmp byte ptr [edx + 0xCDF4], 1
        jne wait_
        jmp dword ptr [jmp_ret_phase0]
    wait_:
        jmp dword ptr [kPhase0Wait]
    }
}

// moveEx, the stand not tracing Nero: the follow block (Nero's push-back, his ground height, his yaw into the quat) is
// skipped while the mod holds the stand somewhere (an attack, or a held hold).
naked void detour_follow() {
    _asm {
        cmp byte ptr [esi + 0x22D9], 0
        jne skip
        movss xmm1, dword ptr [eax + 0xD650]
        jmp dword ptr [jmp_ret_follow]
    skip:
        jmp dword ptr [kFollowEnd]
    }
}

// updatePosition: a held stand is pinned (the original compare of mIsTraceParent follows, its jnz is at the resume address).
naked void detour_position() {
    _asm {
        test byte ptr [esi + 0x22D9], 2
        jz orig
        pushad
        push esi
        call hold_pin
        popad
    orig:
        cmp byte ptr [esi + 0x22C8], 1
        jmp dword ptr [jmp_ret_position]
    }
}

// setRedQueenMotion: Nero's DT moves don't hand the stand a new motion during a guardian attack.
naked void detour_dt_request() {
    _asm {
        push ecx
        mov ecx, [esi + 0xCDF8]
        test ecx, ecx
        jz keep
        test byte ptr [ecx + 0x22D9], 1
        jz keep
        xor al, al
    keep:
        pop ecx
        cmp al, 1
        jne no_dt
        push edi
        jmp dword ptr [jmp_ret_dt_request]
    no_dt:
        jmp dword ptr [kDtRequestSkip]
    }
}

// setYamatoMotionStop (Kabuto's pause of the stand's motion): skipped during a guardian attack, else the original test.
naked void detour_kabuto() {
    _asm {
        test byte ptr [eax + 0x22D9], 1
        jnz skip
        cmp byte ptr [eax + 0x22C9], 1
        jmp dword ptr [jmp_ret_kabuto]
    skip:
        jmp dword ptr [kKabutoReturn]
    }
}

// checkActionDevilTrgAtOnce: with the option, a stand holding an enemy no longer refuses the instant DT activation.
naked void detour_dt_at_once() {
    _asm {
        cmp byte ptr [g_dt_cancel], 0
        jne pass
        cmp dword ptr [eax + 0x22A8], 7
        jz refuse
    pass:
        jmp dword ptr [jmp_ret_dt_at_once]
    refuse:
        jmp dword ptr [kDtAtOnceRefuse]
    }
}

// moveActDevilTrg / moveActDevilTrgAir: the Appear call (setAction(1), eax = 1, edi = stand) is skipped while the stand
// holds an enemy and the option is on; the activation's effects on Nero still play.
naked void detour_dt_appear() {
    _asm {
        cmp byte ptr [g_dt_cancel], 0
        je call_it
        cmp dword ptr [edi + 0x22A8], 7
        je skip
    call_it:
        call dword ptr [kSetAction]
    skip:
        jmp dword ptr [jmp_ret_dt_appear]
    }
}
naked void detour_dt_appear_air() {
    _asm {
        cmp byte ptr [g_dt_cancel], 0
        je call_it
        cmp dword ptr [edi + 0x22A8], 7
        je skip
    call_it:
        call dword ptr [kSetAction]
    skip:
        jmp dword ptr [jmp_ret_dt_appear_air]
    }
}

bool g_file_exists = false;
void check_data_file() {
    g_file_exists = std::filesystem::exists(kDataFile);
    if (!g_file_exists) mod_enabled = false;
}

} // namespace

// ---- The ticks -----------------------------------------------------------------------------------------------------------
int slot_for(uintptr_t pl) {
    const int s = nero_style::current(pl) - nero_style::BASE;
    return (s >= 0 && s < nero_style::kSlots) ? s : 0;
}

bool active(uintptr_t pl) {
    const State* s = find_state(pl);
    return s && s->phase == ATTACK;
}

uint32_t trigger_bit() { return trigger_source == TRIG_GAME ? (uint32_t)trigger_game : kVirtualButton; }

void register_taunt_set() {
    motion_registry::SetDesc d;
    d.owner = "GuardianDevil taunt";
    d.target = motion_registry::STAND;
    d.stand = taunt_lmt;
    d.stand_sub = taunt_arm_lmt;
    const int old = g_taunt_set;
    g_taunt_set = taunt_lmt[0] ? motion_registry::add(d) : -1;
    if (old >= 0) motion_registry::remove(old);
}

void rebuild_commands() {
    const uint32_t bit = trigger_bit();
    for (input_commands::Command& c : g_cmd)
        for (int k = 0; k < c.num; ++k)
            if (c.steps[k].type == input_commands::CMD_TYPE_BTN) c.steps[k].btn = bit;
}

void pre_tick(uintptr_t pl) {
    g_dt_cancel = (mod_enabled && dt_cancel_keeps_hold) ? 1 : 0;
    if (!is_nero(pl)) return;
    const uintptr_t stand = st::stand_of(pl);
    if (!stand) return;
    if (!mod_enabled) {
        if (State* s = find_state(pl)) release(stand, *s);
        return;
    }
    State& s = state_for(pl);
    const float dt = at<float>(pl, pl::m_delta_time);
    if (s.cooldown > 0.0f) s.cooldown = std::max(0.0f, s.cooldown - dt);

    const int action = at<int>(stand, st::mActionNo);
    if (s.phase == ATTACK) {
        const char* why = nullptr;
        if (action != st::ACT_YAMATO) end_attack(stand, s);
        else if (cancel_on_hit && nero_locked_out(pl, &why)) { stop_effect(s); s.phase = FADE; }   // the game's gate cancels it next tick
    }
    if (s.phase == FADE && (at<int>(stand, st::mTpPhase) == st::TP_HIDDEN || at<uint8_t>(stand, st::mIsDevilTrigger))) end_fade(stand, s);

    if (hold_stays_put && action == st::ACT_EM_BLOCK) {
        if (!s.hold_detached) std::memcpy(s.hold_pos, &at<float>(stand, st::mPos), 12);
        at<uint8_t>(stand, st::mIsTraceParent) = 0;
        s.hold_detached = true;
    } else if (s.hold_detached) {
        at<uint8_t>(stand, st::mIsTraceParent) = 1;
        s.hold_detached = false;
    }

    taunt_tick(pl, stand, s);
    write_bytes(stand, s);
    if (s.phase == ATTACK && !s.taunting) rush_loop(pl, stand, s);

    // the pad / key trigger as the virtual button, for the commands that run after this tick (the local player's pad)
    if (trigger_source != TRIG_GAME && pl == (uintptr_t)devil4_sdk::get_local_player()) {
        const bool down = trigger_down(pl);
        if (down) at<uint32_t>(pl, pl::mBtnOn) |= kVirtualButton;
        if (down && !s.held) at<uint32_t>(pl, pl::mBtnTrg) |= kVirtualButton;
        s.held = down;
    }
}

void post_command_tick(uintptr_t pl) {
    uint32_t fired = 0;
    for (int i = 0; i < CMD_COUNT; ++i) {
        if (bit_set(pl, g_cmd_bit[i])) fired |= 1u << i;
        bit_clear(pl, g_cmd_bit[i]);
    }
    if (!fired || !mod_enabled || !is_nero(pl)) return;
    const uintptr_t stand = st::stand_of(pl);
    if (!stand) return;
    State& s = state_for(pl);
    if (s.cooldown > 0.0f) return;
    const int slot = slot_for(pl);
    const ActionSet& set = sets[slot];
    const int motion = (fired & (1u << CMD_RUSH)) ? set.rush : (fired & (1u << CMD_FORWARD)) ? set.forward
                     : (fired & (1u << CMD_BACK)) ? set.back : set.neutral;
    start(pl, stand, s, slot, motion);
}

} // namespace guardian_devil

// ---- The mod -----------------------------------------------------------------------------------------------------------------
using namespace guardian_devil;

namespace {

void key_name(int vk, char* out, int size) {
    const UINT sc = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
    if (!vk) snprintf(out, size, "none");
    else if (!sc || !GetKeyNameTextA((LONG)(sc << 16), out, size)) snprintf(out, size, "VK 0x%02X", vk);
}

int  g_capture = -1;         // TRIG_PAD / TRIG_KEY while a capture runs, -1 none
uint32_t g_capture_bits = 0; // pad capture: the buttons pressed together so far

// The GUI's capture: pad buttons pressed together (done on release), or the first key down.
void capture_tick() {
    if (g_capture < 0) return;
    if (g_capture == TRIG_PAD) {
        const uintptr_t pl = (uintptr_t)devil4_sdk::get_local_player();
        const uint32_t bits = pl ? at<uint32_t>(pl, kPadBtnOn) : 0;
        if (bits) g_capture_bits |= bits;
        else if (g_capture_bits) { trigger_pad = (int)g_capture_bits; g_capture_bits = 0; g_capture = -1; }
    } else {
        for (int vk = 8; vk < 255; ++vk) {
            if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
            if (GetAsyncKeyState(vk) & 0x8000) { trigger_key = vk == VK_ESCAPE ? 0 : vk; g_capture = -1; break; }
        }
    }
}

} // namespace

void GuardianDevil::toggle(bool enable) {
    for (int i = 0; i < 10; ++i) {
        if (enable) install_patch_absolute(kShownPatches[i], patch_shown[i], "\xD3", 1);
        else patch_shown[i].reset();
    }
}

std::optional<std::string> GuardianDevil::on_initialize() {
    using namespace input_commands;
    build_commands();
    for (int i = 0; i < CMD_COUNT; ++i) {
        g_cmd_bit[i] = next_free_bit();
        if (g_cmd_bit[i] < 0 || !add(g_cmd[i].head(), g_cmd_bit[i], &mod_enabled)) {
            spdlog::error("GuardianDevil: no free command bit for the {} command (85-95 are all taken)", kCmdNames[i]);
            g_cmd_bit[i] = -1;
        }
    }
    register_taunt_set();
    add_tick(&pre_tick);
    add_post_command_tick(&post_command_tick);
    if (!input_commands::hook_installed) {     // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("GuardianDevil: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init GuardianDevil (command hook)";
        }
        input_commands::hook_installed = true;
    }
    struct Site { uintptr_t addr; std::unique_ptr<FunctionHook>* hook; void* detour; uintptr_t* ret; ptrdiff_t len; const char* name; };
    const Site sites[] = {
        { kGateSite,        &hook_gate,          &detour_gate,          &jmp_ret_gate,          8,  "moveActYamato gate" },
        { kPhase0Site,      &hook_phase0,        &detour_phase0,        &jmp_ret_phase0,        13, "moveActYamato phase 0" },
        { kFollowSite,      &hook_follow,        &detour_follow,        &jmp_ret_follow,        8,  "moveEx follow block" },
        { kPositionSite,    &hook_position,      &detour_position,      &jmp_ret_position,      7,  "updatePosition" },
        { kDtRequestSite,   &hook_dt_request,    &detour_dt_request,    &jmp_ret_dt_request,    5,  "setRedQueenMotion" },
        { kKabutoSite,      &hook_kabuto,        &detour_kabuto,        &jmp_ret_kabuto,        7,  "setYamatoMotionStop" },
        { kDtAtOnceSite,    &hook_dt_at_once,    &detour_dt_at_once,    &jmp_ret_dt_at_once,    9,  "checkActionDevilTrgAtOnce" },
        { kDtAppearSite,    &hook_dt_appear,     &detour_dt_appear,     &jmp_ret_dt_appear,     5,  "moveActDevilTrg Appear" },
        { kDtAppearAirSite, &hook_dt_appear_air, &detour_dt_appear_air, &jmp_ret_dt_appear_air, 5,  "moveActDevilTrgAir Appear" },
    };
    for (const Site& site : sites) {
        if (!install_hook_absolute(site.addr, *site.hook, site.detour, site.ret, site.len)) {
            spdlog::error("GuardianDevil: failed to hook {} (0x{:X})", site.name, site.addr);
            return std::string("Failed to init GuardianDevil (") + site.name + ")";
        }
    }
    return Mod::on_initialize();
}

namespace {

void hex_input(const char* label, int& value, const char* help) {
    ImGui::PushItemWidth(80.0f);
    ImGui::InputScalar(label, ImGuiDataType_S32, &value, nullptr, nullptr, "%03X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    Mod::help_marker(help);
}

void action_set_editor(int slot) {
    ActionSet& set = sets[slot];
    ImGui::PushID(slot);
    ImGui::Checkbox(_("Guardian in this style"), &set.enabled);
    ImGui::SameLine();
    Mod::help_marker(_("Off: the trigger does nothing while this style is picked (the stand stays the game's)."));
    hex_input(_("Neutral"), set.neutral, _("Motion the stand plays on the trigger alone (hex; the stand's Red Queen motion numbers). Default 31C."));
    hex_input(_("Forward"), set.forward, _("Lock-on + forward + the trigger. Default 317 (Streak)."));
    hex_input(_("Back"), set.back, _("Lock-on + back + the trigger. Default 330."));
    hex_input(_("Rush"), set.rush, _("Lock-on + back, forward + the trigger (Calibur's input: back, then forward within 15 frames, the trigger "
                                     "within 15 more). Default 35F (the punch rush). Holding the trigger keeps it going (the loop below)."));
    ImGui::PushItemWidth(Mod::sameLineItemWidth);
    ImGui::SliderFloat(_("Rush start frame"), &set.rush_start, 0.0f, 300.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("The frame the rush motion starts at (its wind-up is skipped at 135)."));
    ImGui::SliderFloat(_("Loop from"), &set.loop_from, 0.0f, 300.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("While the trigger is held, a rush frame between Loop from and Loop to jumps back to Loop to frame. Let go in that "
                       "window and the rush ends with its end effect."));
    ImGui::SliderFloat(_("Loop to"), &set.loop_to, 0.0f, 300.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("The end of the loop window (172 by default; a 2-frame window catches the frame at 60 fps)."));
    ImGui::SliderFloat(_("Loop to frame"), &set.loop_dest, 0.0f, 300.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("Where the rush jumps back to while held (150: the punches, past the wind-up)."));
    ImGui::PopItemWidth();
    static int copy_from = 0;
    ImGui::PushItemWidth(Mod::sameLineItemWidth);
    if (ImGui::BeginCombo(_("Copy from slot"), nero_style::slots[copy_from].name[0] ? nero_style::slots[copy_from].name : "Slot")) {
        for (int i = 0; i < nero_style::kSlots; ++i) {
            char label[48];
            snprintf(label, sizeof label, "%d: %s", i + 1, nero_style::slots[i].name[0] ? nero_style::slots[i].name : "(unnamed)");
            if (ImGui::Selectable(label, copy_from == i)) copy_from = i;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button(_("Copy"))) { const bool on = set.enabled; set = sets[copy_from]; set.enabled = on; }
    ImGui::SameLine();
    Mod::help_marker(_("Copies that slot's motions and rush frames into this one (this slot's on / off is kept)."));
    ImGui::PopID();
}

const char* phase_name(int phase) { return phase == ATTACK ? "attack" : phase == FADE ? "fading" : "idle"; }

} // namespace

void GuardianDevil::on_gui_frame(int display) {
    if (display != DISPLAY_NERO_A) return;
    ImGui::BeginGroup();
    if (!g_file_exists) {
        if (ImGui::Button(_("Download Guardian Devil Files"))) {
            ShellExecuteA(NULL, "open", "https://github.com/muhopensores/dmc4_hook/releases", NULL, NULL, SW_SHOWNORMAL);
        }
        ImGui::SameLine();
        help_marker(_("Clicking this button will open https://github.com/muhopensores/dmc4_hook/releases\n"
                      "From here you can download optional files for mods that require them, found in the Assets section of each "
                      "dmc4_hook release\n"
                      "Once you've downloaded and installed these files (I recommend using Fluffy Mod Manager), "
                      "restart the game and tick \"HDD File Priority\" in the Debug tab before you load into a stage\n"
                      "This mod lets Nero's stand attack on a button outside Devil Trigger"));
        ImGui::EndGroup();
        return;
    }
    if (ImGui::Checkbox(_("Guardian Devil"), &mod_enabled)) toggle(mod_enabled);
    ImGui::SameLine();
    help_marker(_("Nero's Devil Trigger stand attacks on a button, outside DT, with Red Queen motions: the trigger alone, lock-on + "
                  "forward or back + the trigger, and lock-on + back, forward + the trigger for the punch rush (hold to keep "
                  "punching). The trigger is one of Nero's game inputs (Change Gun by default), bound on the pad and keyboard in "
                  "the game's control settings. Each Nero style has its own set of motions. The stand refuses while it is busy "
                  "(Buster, Snatch, a hold) and while Nero is hit, in Buster, Snatch, Showdown, Maximum Bet or a DT activation. "
                  "Off, the stand is the game's."));
    if (!mod_enabled) { ImGui::EndGroup(); return; }
    ImGui::Indent(lineIndent);
    ImGui::PushID("GuardianDevil");

    capture_tick();
    static const char* kSources[] = { "Gamepad buttons", "Keyboard key", "Game input" };
    ImGui::PushItemWidth(sameLineItemWidth);
    if (ImGui::BeginCombo(_("Trigger"), kSources[trigger_source])) {
        for (int i = 0; i < 3; ++i)
            if (ImGui::Selectable(kSources[i], trigger_source == i)) { trigger_source = i; rebuild_commands(); }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("What triggers the stand. Gamepad buttons: raw pad buttons (held together), whatever the game maps them to. "
                  "Keyboard key: a key, while the game is in front and this menu is closed. Game input: one of Nero's mapped "
                  "inputs, bound in the game's control settings. Either way the stick part is the game's own command checker: "
                  "lock-on + forward / back, or lock-on + back, forward for the rush."));
    ImGui::Indent(lineIndent);
    if (trigger_source == TRIG_PAD) {
        ImGui::PushItemWidth(sameLineItemWidth);
        const auto info = devil4_sdk::getButtonInfo((uint16_t)trigger_pad);
        char label[64];
        if (info.first == (uint16_t)trigger_pad && info.second) snprintf(label, sizeof label, "%s", utility::text_lookup((char*)info.second));
        else snprintf(label, sizeof label, "buttons 0x%X", trigger_pad);
        if (ImGui::BeginCombo(_("Pad button"), label)) {
            for (const auto& pair : buttonPairs)
                if (ImGui::Selectable(utility::text_lookup((char*)pair.second), trigger_pad == pair.first)) trigger_pad = pair.first;
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (g_capture == TRIG_PAD) ImGui::TextDisabled(_("press the buttons, then let go"));
        else if (ImGui::Button(_("Capture"))) { g_capture = TRIG_PAD; g_capture_bits = 0; }
        ImGui::SameLine();
        help_marker(_("One pad button from the list, or Capture to press one or several pad buttons together (the combination "
                      "is taken when you let go)."));
    } else if (trigger_source == TRIG_KEY) {
        char name[64];
        key_name(trigger_key, name, sizeof name);
        ImGui::Text("%s: %s", _("Key"), name);
        ImGui::SameLine();
        if (g_capture == TRIG_KEY) ImGui::TextDisabled(_("press a key (Escape = none)"));
        else if (ImGui::Button(_("Capture"))) g_capture = TRIG_KEY;
        ImGui::SameLine();
        help_marker(_("Capture, then press the key to use. Escape clears it."));
    } else {
        static const struct { int bit; const char* name; } kGameInputs[] = { { 0x100, "Change Gun" }, { 0x80, "Change Devil Arm" }, { 0x8, "Taunt" } };
        const char* game_name = "custom";
        for (const auto& t : kGameInputs) if (t.bit == trigger_game) game_name = t.name;
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::BeginCombo(_("Game input"), game_name)) {
            for (const auto& t : kGameInputs)
                if (ImGui::Selectable(t.name, t.bit == trigger_game)) { trigger_game = t.bit; rebuild_commands(); }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        help_marker(_("A mapped game input, bound on your pad and keyboard in the game's control settings. Change Gun and Change "
                      "Devil Arm do nothing on Nero, but they only work if Nero's settings let you bind them; Taunt also taunts "
                      "when the stand refuses."));
    }
    ImGui::Unindent(lineIndent);
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Cooldown"), &cooldown_frames, 0.0f, 120.0f, "%.0f frames");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("Frames after a start before the trigger works again (30)."));
    ImGui::Checkbox(_("Cancel when Nero is hit"), &cancel_on_hit);
    ImGui::SameLine();
    help_marker(_("On: a hit, grab or knock-down on Nero ends the stand's attack (it fades out). Off: the stand finishes its attack."));
    ImGui::Checkbox(_("Devil Rush effect"), &rush_effect);
    ImGui::SameLine();
    help_marker(_("The rush's effects on the stand (effect\\efl\\vie\\vi020_00v0 while it punches, v1 when it stops)."));
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Height offset"), &height_offset, -60.0f, 60.0f, "%.0f cm");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("Added to the stand's height when it is placed on Nero for an attack (0). Negative lowers it, for attacks that "
                  "should reach knocked-down enemies."));
    ImGui::Checkbox(_("Stand holds its place during a hold"), &hold_stays_put);
    ImGui::SameLine();
    help_marker(_("While the stand holds an enemy (a Buster hold) it stays where it is instead of following Nero, until the hold ends."));
    ImGui::Checkbox(_("Instant DT during a hold"), &dt_cancel_keeps_hold);
    ImGui::SameLine();
    help_marker(_("The game refuses the instant DT activation (Nero's DT cancel) while the stand holds an enemy and takes the slow "
                  "path instead. On: the instant activation runs, cancelling Nero's move with the aura, and the stand keeps the "
                  "hold instead of being summoned to Nero."));

    ImGui::SeparatorText(_("Taunt mimic"));
    ImGui::Checkbox(_("Stand mimics Nero's taunt"), &taunt_mimic);
    ImGui::SameLine();
    help_marker(_("When Nero taunts, the stand appears beside him and plays the same taunt, and stops when he does. The stand's own "
                  "motion files have no taunts and its bones are numbered differently, so Nero's taunts are retargeted onto the "
                  "stand and its arm with Albam (scripts/stand_taunt_retarget.py) into the two LMTs below, loose files under "
                  "nativePC, which the motion registry puts on the stand."));
    ImGui::PushItemWidth(sameLineWidth);
    bool lmt_changed = ImGui::InputText(_("Taunt LMT (stand)"), taunt_lmt, sizeof taunt_lmt);
    ImGui::SameLine();
    help_marker(_("Game path without .lmt, read from nativePC: the stand body's taunts, with Nero's block numbers."));
    lmt_changed |= ImGui::InputText(_("Taunt LMT (arm)"), taunt_arm_lmt, sizeof taunt_arm_lmt);
    ImGui::SameLine();
    help_marker(_("The same for the arm model (wp024). Clear it to leave the arm still."));
    ImGui::PopItemWidth();
    if (lmt_changed) register_taunt_set();
    ImGui::SameLine();
    if (ImGui::Button(_("Reload LMTs"))) motion_registry::refresh();
    ImGui::SameLine();
    help_marker(_("Reads every registry LMT from disk again (after re-exporting the files)."));
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Taunt: behind Nero"), &taunt_back, -200.0f, 200.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("Where the stand stands for the taunt: cm behind Nero along his facing (negative = in front)."));
    ImGui::SliderFloat(_("Taunt: to the right"), &taunt_side, -200.0f, 200.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("cm to Nero's right (negative = left)."));
    ImGui::PopItemWidth();
    if (const uintptr_t P = (uintptr_t)devil4_sdk::get_local_player(); P && is_nero(P) && g_taunt_set >= 0)
        ImGui::TextDisabled("taunt set: %s", motion_registry::describe(g_taunt_set, P).c_str());

    ImGui::SeparatorText(_("Actions per style"));
    static int gui_slot = 0;
    ImGui::PushItemWidth(sameLineItemWidth);
    {
        char label[48];
        snprintf(label, sizeof label, "%d: %s", gui_slot + 1, nero_style::slots[gui_slot].name[0] ? nero_style::slots[gui_slot].name : "(unnamed)");
        if (ImGui::BeginCombo(_("Style slot"), label)) {
            for (int i = 0; i < nero_style::kSlots; ++i) {
                snprintf(label, sizeof label, "%d: %s", i + 1, nero_style::slots[i].name[0] ? nero_style::slots[i].name : "(unnamed)");
                if (ImGui::Selectable(label, gui_slot == i)) gui_slot = i;
            }
            ImGui::EndCombo();
        }
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("The Nero style whose stand actions are edited below (the slots of the Nero styles mod). The first slot's set "
                  "also serves when Nero styles are off."));
    action_set_editor(gui_slot);

    if (const uintptr_t P = (uintptr_t)devil4_sdk::get_local_player()) {
        if (is_nero(P)) {
            const uintptr_t stand = st::stand_of(P);
            const State* s = find_state(P);
            const int slot = slot_for(P);
            ImGui::Text("%s | stand action %d tp %d trace %d | slot %d (%s) | motion %03X | cooldown %.0f | bytes mod %d shown %d | cmd bits %d %d %d %d | starts %u refused %u (%s)",
                        s ? phase_name(s->phase) : "idle",
                        stand ? at<int>(stand, st::mActionNo) : -1, stand ? at<int>(stand, st::mTpPhase) : -1, stand ? at<uint8_t>(stand, st::mIsTraceParent) : -1,
                        slot, nero_style::slots[slot].name, s ? s->motion : -1, s ? s->cooldown : 0.0f,
                        stand ? at<uint8_t>(stand, st::kMod) : -1, stand ? at<uint8_t>(stand, st::kShown) : -1,
                        g_cmd_bit[0], g_cmd_bit[1], g_cmd_bit[2], g_cmd_bit[3],
                        s ? s->starts : 0u, s ? s->refused : 0u, s ? s->last_refusal : "");
        } else {
            ImGui::TextDisabled(_("(the local player isn't Nero)"));
        }
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
    ImGui::EndGroup();
}

// The keys are the ones the mod has always had (dt_knuckle, knuckle_input), the rest under dt_knuckle_.
void GuardianDevil::on_config_load(const utility::Config& cfg) {
    mod_enabled    = cfg.get<bool>("dt_knuckle").value_or(false);
    trigger_pad    = cfg.get<int>("knuckle_input").value_or(trigger_pad);   // the raw pad buttons, as it always was
    trigger_source = cfg.get<int>("dt_knuckle_trigger_source").value_or(trigger_source);
    trigger_key    = cfg.get<int>("dt_knuckle_trigger_key").value_or(trigger_key);
    trigger_game   = cfg.get<int>("dt_knuckle_trigger_game").value_or(trigger_game);
    if (trigger_source < TRIG_PAD || trigger_source > TRIG_GAME) trigger_source = TRIG_PAD;
    rebuild_commands();
    cooldown_frames      = cfg.get<float>("dt_knuckle_cooldown").value_or(cooldown_frames);
    cancel_on_hit        = cfg.get<bool>("dt_knuckle_cancel_on_hit").value_or(cancel_on_hit);
    rush_effect          = cfg.get<bool>("dt_knuckle_rush_effect").value_or(rush_effect);
    height_offset        = cfg.get<float>("dt_knuckle_height_offset").value_or(height_offset);
    hold_stays_put       = cfg.get<bool>("dt_knuckle_hold_stays_put").value_or(hold_stays_put);
    dt_cancel_keeps_hold = cfg.get<bool>("dt_knuckle_dt_cancel_keeps_hold").value_or(dt_cancel_keeps_hold);
    taunt_mimic          = cfg.get<bool>("dt_knuckle_taunt_mimic").value_or(taunt_mimic);
    if (auto p = cfg.get("dt_knuckle_taunt_lmt")) strcpy_s(taunt_lmt, p->c_str());
    if (auto p = cfg.get("dt_knuckle_taunt_arm_lmt")) strcpy_s(taunt_arm_lmt, p->c_str());
    taunt_back           = cfg.get<float>("dt_knuckle_taunt_back").value_or(taunt_back);
    taunt_side           = cfg.get<float>("dt_knuckle_taunt_side").value_or(taunt_side);
    register_taunt_set();
    for (int i = 0; i < nero_style::kSlots; ++i) {
        ActionSet& set = sets[i];
        const std::string p = "dt_knuckle_s" + std::to_string(i) + "_";
        set.enabled    = cfg.get<bool>(p + "enabled").value_or(set.enabled);
        set.neutral    = cfg.get<int>(p + "neutral").value_or(set.neutral);
        set.forward    = cfg.get<int>(p + "forward").value_or(set.forward);
        set.back       = cfg.get<int>(p + "back").value_or(set.back);
        set.rush       = cfg.get<int>(p + "rush").value_or(set.rush);
        set.rush_start = cfg.get<float>(p + "rush_start").value_or(set.rush_start);
        set.loop_from  = cfg.get<float>(p + "loop_from").value_or(set.loop_from);
        set.loop_to    = cfg.get<float>(p + "loop_to").value_or(set.loop_to);
        set.loop_dest  = cfg.get<float>(p + "loop_dest").value_or(set.loop_dest);
    }
    check_data_file();
    toggle(mod_enabled);
}

void GuardianDevil::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dt_knuckle", mod_enabled);
    cfg.set<int>("knuckle_input", trigger_pad);
    cfg.set<int>("dt_knuckle_trigger_source", trigger_source);
    cfg.set<int>("dt_knuckle_trigger_key", trigger_key);
    cfg.set<int>("dt_knuckle_trigger_game", trigger_game);
    cfg.set<float>("dt_knuckle_cooldown", cooldown_frames);
    cfg.set<bool>("dt_knuckle_cancel_on_hit", cancel_on_hit);
    cfg.set<bool>("dt_knuckle_rush_effect", rush_effect);
    cfg.set<float>("dt_knuckle_height_offset", height_offset);
    cfg.set<bool>("dt_knuckle_hold_stays_put", hold_stays_put);
    cfg.set<bool>("dt_knuckle_dt_cancel_keeps_hold", dt_cancel_keeps_hold);
    cfg.set<bool>("dt_knuckle_taunt_mimic", taunt_mimic);
    cfg.set("dt_knuckle_taunt_lmt", std::string(taunt_lmt));
    cfg.set("dt_knuckle_taunt_arm_lmt", std::string(taunt_arm_lmt));
    cfg.set<float>("dt_knuckle_taunt_back", taunt_back);
    cfg.set<float>("dt_knuckle_taunt_side", taunt_side);
    for (int i = 0; i < nero_style::kSlots; ++i) {
        const ActionSet& set = sets[i];
        const std::string p = "dt_knuckle_s" + std::to_string(i) + "_";
        cfg.set<bool>(p + "enabled", set.enabled);
        cfg.set<int>(p + "neutral", set.neutral);
        cfg.set<int>(p + "forward", set.forward);
        cfg.set<int>(p + "back", set.back);
        cfg.set<int>(p + "rush", set.rush);
        cfg.set<float>(p + "rush_start", set.rush_start);
        cfg.set<float>(p + "loop_from", set.loop_from);
        cfg.set<float>(p + "loop_to", set.loop_to);
        cfg.set<float>(p + "loop_dest", set.loop_dest);
    }
}
