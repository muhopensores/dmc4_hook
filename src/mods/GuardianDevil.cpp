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
#include <random>
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
char  taunt_lmt[128]       = "motion\\pl020\\pl020_guardian_ex";
char  taunt_arm_lmt[128]   = "motion\\wp024\\wp024_guardian_ex";
float taunt_back           = 80.0f;
float taunt_side           = 0.0f;
bool  guard_enabled        = true;
float guard_window         = 5.0f;
bool  guard_effects        = true;
bool  guard_style          = true;
bool  guard_dt             = true;
float guard_dt_gain        = -1.0f;
float guard_recovery       = 15.0f;
float reguard_time         = 45.0f;
bool  dt_cost              = false;
float cost_neutral         = 500.0f;   // half a bar per move
float cost_forward         = 500.0f;
float cost_back            = 500.0f;
float cost_rush            = 500.0f;
float cost_guard           = 0.0f;
float rush_drain           = 500.0f;
float cost_in_dt           = 0.5f;    // in Devil Trigger every cost and the drain are halved
static const ActionSet kDefaultSet = { true, 0x31C, 0x317, 0x330, 0x35F, 135.0f, 170.0f, 172.0f, 150.0f, 0x302, 0.0f,
                                           -1, -1, -1, -1, 0, 3 };   // the guard plays block 0, 1 or 2 of the stand LMT (guardian_ex)
ActionSet sets[nero_style::kSlots] = { kDefaultSet, kDefaultSet, kDefaultSet, kDefaultSet };

namespace {

// ---- The game ---------------------------------------------------------------------------------------------------
char* const kRushEffectRun = "effect\\efl\\vie\\vi020_00v0";   // while the rush runs
char* const kRushEffectEnd = "effect\\efl\\vie\\vi020_00v1";   // when it ends
// A guard's feedback is the vanilla stand block's (uPlayerNero::procBeforeDamageCalc 0x7E6620): the stay effect list 0 /
// 0x28 (effect\efl\com\ec032_00v1) and SE 69 on the player's second sound bank (uPlayer +0x1D64 = pl000_style00), with
// the pad rumble uPlayer::reverseMessage gives a clash (setPadVibration 1).
constexpr int kGuardEffectList = 0, kGuardEffectIndex = 0x28;
constexpr float kGuardEffectHeight = 100.0f;                   // cm above Nero's feet: his chest
constexpr int kGuardSe = 69;
constexpr int kGuardVibration = 1;
constexpr uintptr_t kSeBanks = 0x1D60;                         // uPlayer::cSECtrl::mpSndReqRsrc[3]: [0] pl00X_core, [1] pl00X_style00
constexpr uintptr_t fn_requestSe = 0x00A47AA0;                 // sDevil4Sound::requestSe (stdcall)
constexpr uintptr_t fn_sePositionCb = 0x00486640;              // sSoundN::soundPositionUpdateCb
constexpr uintptr_t kSoundInstance = 0x00E559D8;               // sSoundN::mpInstance
uintptr_t fn_setPadVibration = 0x007AC130;                     // uPlayer::setPadVibration(this@edx, id@eax)
// The block's style: uPlayer::addComboGauge (type@edi, player@esi; stack: style, style, DT bonus (floats), 0; retn 10h) with
// ADD_COMBO_GUAGE_ENEMY_GUARD and the player params' +0x44 / +0x40 / +0x48, as procBeforeDamageCalc passes them.
uintptr_t fn_addComboGauge = 0x007AF560;
constexpr uintptr_t kPlayerParam = 0x1EA8;                     // uPlayer: the player's parameter table
constexpr uintptr_t kCmdFlagWords = 0x2184;                    // uPlayer::mCmdFlag[3]
constexpr uintptr_t kPadBtnOn = 0x1374;                        // uPlayer::mPeripheral.mPadBtnOn: the raw pad bits
constexpr uintptr_t kDevilTrgNow = 0x1F24;                     // uPlayer::mDevilTrg.now: the DT gauge (1000 = one bar)
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
constexpr uintptr_t kSwordParentJoint = 0x13AC;  // the sword model's parent joint: uPlNeroDevil::updatePosition writes 150 (the arm's
                                                 // hand) while the stand's motion has event bit 31 (kMotSeqParam 0xC3D6C8), else 151 (sheath)
constexpr uintptr_t kDisappearSite  = 0x829B39;   // uPlNeroDevil::checkDevilDisappear: `cmp byte [eax+22C9h],1` (7 bytes), then `jz` the DT test
uintptr_t           kDisappearDT    = 0x829B70;   //   the DT test (event bit 10)
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
bool g_test_guard = false; // the GUI's Play Guard Motion: the local Nero's next tick plays the style's guard motion

// ---- Per-player state ---------------------------------------------------------------------------------------------
constexpr int kPlayerSlots = 8;
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
    bool        drained = false;      // the rush ran the DT gauge dry: it stops looping
    bool        hold_detached = false;// hold_stays_put took the stand off Nero
    float       hold_pos[3] = {};     // where it was when the hold began (the hold motions' root motion is undone to it)
    bool        held = false;         // the pad / key trigger was down last tick (the press edge)
    bool        taunting = false;     // the running attack is a taunt mimic (ends with Nero's taunt)
    bool        rushing = false;      // the running attack is the rush (its loop and effects)
    bool        taunt_tried = false;  // this taunt of Nero's was answered (or refused) already
    bool        window = false;       // a guard window is open: an attack's press or a re-guard opened it
    float       guard_timer = 0.0f;   // frames since that press (delta time, as Dante's Just Block counter)
    bool        window_blocked = false;// the open window caught a hit
    bool        guard_pending = false;// a hit was guarded: the stand turns to the guard on the next tick
    bool        guarding = false;     // the running action is the guard
    bool        reguard_ok = true;    // a press may guard again (off after a re-guard caught nothing)
    float       reguard_left = 0.0f;  // frames left in which a press re-guards (reguard_time from each guard)
    bool        reguard_window = false;// the open window is a re-guard's (it guards whatever the stand does)
    uint32_t    guards = 0, blocked = 0;
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

// The trigger: down now; the press edge comes from the state's last tick.
bool keyboard_allowed() { return !g_framework->m_draw_ui && GetForegroundWindow() == g_framework->get_window_handle(); }
bool input_down(uintptr_t pl, int source, int pad, int key, int game) {
    switch (source) {
    case TRIG_PAD:  return pad != 0 && (at<uint32_t>(pl, kPadBtnOn) & (uint32_t)pad) == (uint32_t)pad;
    case TRIG_KEY:  return key != 0 && keyboard_allowed() && (GetAsyncKeyState(key) & 0x8000) != 0;
    case TRIG_GAME: return game != 0 && (at<uint32_t>(pl, pl::mBtnOn) & (uint32_t)game) != 0;
    default: return false;
    }
}
bool trigger_down(uintptr_t pl) { return input_down(pl, trigger_source, trigger_pad, trigger_key, trigger_game); }

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
    if (s.phase == ATTACK && s.guarding) mod |= st::MOD_GUARD;
    at<uint8_t>(stand, st::kMod) = mod;
    at<uint8_t>(stand, st::kShown) = (at<uint8_t>(stand, st::mIsDevilTrigger) || s.phase != IDLE) ? 1 : 0;
}

// The start: what setRedQueenMotion does in DT, on a stand placed on Nero (`back` / `side` cm from him along his
// facing) and detached, fading in at once. `taunt`: the motion is Nero's taunt (no lock-out on his taunt action).
// `guard`: a guarded hit turns the running attack into the style's guard motion from its start frame (the attack's
// window stays as it is) and the guard's recovery starts. Any other attack opens a guard window.
// `cost`: the DT it takes (with dt_cost on; refused when the gauge has less).
// A motion of an action set: the stand LMT block's registry word when it has one and the set is on the stand, else the
// vanilla motion number.
std::string g_motion_source = "-";   // where the last resolved motion came from (the readout)

// The stand's body motion as uPlNeroDevil::updatePosition reads its sword-in-hand bit 31 (uActor::getNumberFromMotSeq
// 0x4AAE70: the motion at stand +0x2240; +4 its number (0xFFFF = none), +0x18 the frame, +0xC0 the first event table:
// +0x40 the run count, +0x44 the runs of (u32 value, u32 frames)), for the readout.
std::string sword_event_debug(uintptr_t stand) {
    char buf[160];
    const uintptr_t m = at<uintptr_t>(stand, 0x2240);
    if (!m) return "no body motion";
    const unsigned no = at<uint16_t>(m, 4);
    const float frame = at<float>(m, 0x18);
    const uintptr_t table = at<uintptr_t>(m, 0xC0);
    if (!table) {
        snprintf(buf, sizeof buf, "body motion %03X frame %.1f: no event table", no, frame);
        return buf;
    }
    const int count = at<int>(table, 0x40);
    const uintptr_t runs = at<uintptr_t>(table, 0x44);
    uint32_t value = 0;
    int end = 0;
    for (int i = 0; runs && i < count && i < 256; ++i) {
        end += at<int>(runs, 8 * i + 4);
        if (end > (int)frame) { value = at<uint32_t>(runs, 8 * i); break; }
    }
    snprintf(buf, sizeof buf, "body motion %03X frame %.1f: %d event runs, now %08X (sword in hand, bit 31: %s)", no, frame, count,
             value, (value >> 31) & 1 ? "on" : "off");
    return buf;
}

int resolve_motion(uintptr_t pl, int motion, int block) {
    if (block < 0) { g_motion_source = "vanilla (LMT block -1)"; return motion; }
    if (g_taunt_set < 0) { g_motion_source = "vanilla: no stand LMT path"; return motion; }
    if (!motion_registry::ready(g_taunt_set, pl)) {
        g_motion_source = "vanilla: stand LMT not on the stand (" + motion_registry::describe(g_taunt_set, pl) + ")";
        return motion;
    }
    const int word = motion_registry::word(g_taunt_set, pl, block);
    if (word < 0) { g_motion_source = "vanilla: block " + std::to_string(block) + " isn't in the stand LMT"; return motion; }
    g_motion_source = "LMT block " + std::to_string(block);
    return word;
}

// The guard's motion: a random block of guard_block .. guard_block + guard_blocks - 1 (one the stand LMT lacks falls back to the
// first).
int guard_motion(uintptr_t pl, const ActionSet& set) {
    static std::minstd_rand rng(GetTickCount());
    int block = set.guard_block;
    if (block >= 0 && set.guard_blocks > 1) {
        const int pick = block + (int)(rng() % (uint32_t)set.guard_blocks);
        if (g_taunt_set < 0 || !motion_registry::ready(g_taunt_set, pl) || motion_registry::word(g_taunt_set, pl, pick) >= 0) block = pick;
    }
    return resolve_motion(pl, set.guard, block);
}

void start(uintptr_t pl, uintptr_t stand, State& s, int slot, int motion, bool taunt = false, float back = 0.0f, float side = 0.0f,
           bool guard = false, float cost = 0.0f, bool rush = false) {
    const ActionSet& set = sets[slot];
    const char* why = nullptr;
    const int action = at<int>(stand, st::mActionNo);
    if (!dt_cost) cost = 0.0f;
    else if (at<uint8_t>(stand, st::mIsDevilTrigger)) cost *= cost_in_dt;   // Nero's DT state is the stand's
    if (!set.enabled)                                            why = "no stand in this style";
    else if (cost > 0.0f && at<float>(pl, kDevilTrgNow) < cost)  why = "not enough DT";
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
    st::yamato_param(stand, motion, 4, guard ? set.guard_start : rush ? set.rush_start : 0.0f);
    at<int>(stand, st::mTpPhase) = st::TP_FADE_IN;
    if (const uintptr_t target = at<uintptr_t>(pl, st::pl_mLockOnTarget)) st::set_snatch_param(stand, target);

    if (cost > 0.0f) at<float>(pl, kDevilTrgNow) -= cost;
    stop_effect(s);
    if (rush && rush_effect) s.effect = (uintptr_t)devil4_sdk::effect_generator(kRushEffectRun, (void*)stand, 0x14);
    s.effect_ended = false;
    s.drained = false;
    s.phase = ATTACK;
    s.motion = motion;
    s.slot = slot;
    s.taunting = taunt;
    s.rushing = rush;
    s.guarding = guard;
    s.guard_pending = false;
    if (guard) {
        s.reguard_ok = true;
        s.reguard_left = reguard_time;
    } else {
        s.window = guard_enabled && !taunt;
        s.guard_timer = 0.0f;
        s.window_blocked = false;
        s.reguard_window = false;
        s.reguard_left = 0.0f;
    }
    s.cooldown = guard ? guard_recovery : cooldown_frames;
    ++s.starts;
    if (guard) ++s.guards;
    write_bytes(stand, s);
}

// The stand's action left 4: the motion ended (phase 1's setAction(0)) or the game cancelled it. The stand stays where
// it attacked while it fades out (MOD_PLACED keeps moveEx's follow code off); it follows Nero again once hidden.
void end_attack(uintptr_t stand, State& s) {
    stop_effect(s);
    s.phase = FADE;
    s.taunting = false;
    s.guarding = false;
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

// The rush: while the trigger is held, a frame in the loop window jumps back; let go, the end effect plays once. With
// dt_cost, holding it past the wind-up (from the loop's target frame) drains the gauge; dry, it stops looping.
void rush_loop(uintptr_t pl, uintptr_t stand, State& s) {
    const ActionSet& set = sets[s.slot];
    const float frame = at<float>(stand, st::mFrame0);
    if (s.rushing) {
        bool held = trigger_down(pl) && !s.drained && !s.effect_ended;
        if (held && dt_cost && rush_drain > 0.0f && frame >= set.loop_dest) {
            float& gauge = at<float>(pl, kDevilTrgNow);
            const float rate = at<uint8_t>(stand, st::mIsDevilTrigger) ? rush_drain * cost_in_dt : rush_drain;
            const float take = rate * at<float>(pl, pl::m_delta_time) / 60.0f;
            if (gauge >= take) gauge -= take;
            else { gauge = 0.0f; s.drained = true; held = false; }
        }
        if (frame < set.loop_from || frame > set.loop_to) return;
        if (held) {
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

// A guard's effect at Nero's chest, its sound on him and the pad rumble.
void pad_vibration(uintptr_t pl, int id) {
    _asm {
        pushad
        mov edx, pl
        mov eax, id
        call fn_setPadVibration
        popad
    }
}
void guard_feedback(uintptr_t pl) {
    if (!guard_effects) return;
    float pos[4];
    std::memcpy(pos, &at<float>(pl, pl::mPos), 12);
    pos[1] += kGuardEffectHeight;
    pos[3] = 0.0f;
    devil4_sdk::effect_stay(kGuardEffectList, kGuardEffectIndex, &at<float>(pl, pl::mQuat), pos, (void*)pl);
    const uintptr_t inst = *(uintptr_t*)kSoundInstance;
    const uintptr_t bank = at<uintptr_t>(pl, kSeBanks + 4);
    if (inst && bank)
        ((void(__stdcall*)(uintptr_t, uintptr_t, int, uintptr_t, const float*, int, int, uintptr_t))fn_requestSe)(
            inst, bank, kGuardSe, pl, &at<float>(pl, pl::mPos), 1, 1, fn_sePositionCb);
    pad_vibration(pl, kGuardVibration);
}

// The vanilla block's style for a guard, without its DT bonus (the guard can cost DT instead).
void add_guard_style(uintptr_t pl) {
    if (!guard_style) return;
    const uintptr_t prm = at<uintptr_t>(pl, kPlayerParam);
    if (!prm) return;
    const float style_a = at<float>(prm, 0x44), style_b = at<float>(prm, 0x40), dt_bonus = 0.0f;
    _asm {
        pushad
        push 0
        push dt_bonus
        push style_b
        push style_a
        mov edi, 6
        mov esi, pl
        call fn_addComboGauge
        popad
    }
}

// The DT a guard adds: the vanilla stand block's bonus (player params +0x48, what procBeforeDamageCalc hands addComboGauge)
// unless guard_dt_gain sets one. Added here rather than through addComboGauge, which skips it while a recent guard is
// still in sStylishCount's list; like addComboGauge, nothing while Nero is in Devil Trigger.
float guard_dt_amount(uintptr_t pl) {
    if (guard_dt_gain >= 0.0f) return guard_dt_gain;
    const uintptr_t prm = at<uintptr_t>(pl, kPlayerParam);
    return prm ? at<float>(prm, 0x48) : 0.0f;
}
void add_guard_dt(uintptr_t pl, uintptr_t stand) {
    if (!guard_dt || at<uint8_t>(stand, st::mIsDevilTrigger)) return;
    float& now = at<float>(pl, kDevilTrgNow);
    now = std::min(now + guard_dt_amount(pl), at<float>(pl, kDevilTrgNow + 4));   // mDevilTrg.max
}

// A Guardian action press after a guard: a new window, once the last one has closed, unless a re-guard missed.
void reguard(State& s) {
    if (!guard_enabled || !s.reguard_ok || (s.window && s.guard_timer <= guard_window)) return;
    s.window = true;
    s.guard_timer = 0.0f;
    s.window_blocked = false;
    s.reguard_window = true;
}

// The mod turned off (or never on) for a player whose stand still carries our state: give the stand back.
void release(uintptr_t stand, State& s) {
    stop_effect(s);
    if (s.phase != IDLE || s.hold_detached) at<uint8_t>(stand, st::mIsTraceParent) = 1;
    s.phase = IDLE;
    s.hold_detached = false;
    s.taunting = false;
    s.guarding = false;
    s.window = false;
    s.window_blocked = false;
    s.reguard_window = false;
    s.reguard_left = 0.0f;
    s.guard_pending = false;
    at<uint8_t>(stand, st::kMod) = 0;
    at<uint8_t>(stand, st::kShown) = at<uint8_t>(stand, st::mIsDevilTrigger);
}

// updatePosition, right after moveMotion applied the motion's root motion: a held stand goes back to where the hold began.
void __stdcall hold_pin(uintptr_t stand) {
    const State* s = find_state(at<uintptr_t>(stand, st::mpParent));
    if (s && s->hold_detached) std::memcpy(&at<float>(stand, st::mPos), s->hold_pos, 12);
}

// ---- The guard -------------------------------------------------------------------------------------------------------
// uPlayerNero::procBeforeDamageCalc (vtable slot 83, __thiscall, 2 args): uPlayer::damageMessage calls it before any
// damage; 1 = the hit is taken care of (nothing else of it happens to Nero). An open guard window answers 1 (its first
// such hit asks for the guard); anything else goes to the game's own (the vanilla stand block during a Buster hold lives
// there).
constexpr uintptr_t kBeforeDamage = 0x7E6620;
constexpr uintptr_t kBeforeDamageSlots[3] = {
    0xBE50EC,   // uPlayerNero::vftable + 83 * 4
    0xBE5424,   // uPlayerNeroBenchmark::vftable + 83 * 4
    0xBE63AC,   // uPlayerNeroTutorial::vftable + 83 * 4
};
using BeforeDamageFn = char(__thiscall*)(uintptr_t player, uintptr_t attacker_group, uintptr_t own_group);

// __fastcall with an unused edx is __thiscall's layout (this in ecx, the callee pops the stack arguments).
char __fastcall guard_before_damage(uintptr_t pl, void*, uintptr_t attacker_group, uintptr_t own_group) {
    if (guarding(pl)) {
        if (State* s = find_state(pl)) {
            ++s->blocked;
            if (!s->window_blocked) s->guard_pending = true;
            s->window_blocked = true;
        }
        return 1;
    }
    return reinterpret_cast<BeforeDamageFn>(kBeforeDamage)(pl, attacker_group, own_group);
}

// ---- The detours -------------------------------------------------------------------------------------------------------
// Every hook tests the stand's kMod byte (edi / esi / eax / ecx = the stand at its site) and does nothing while it is 0.
uintptr_t jmp_ret_disappear = 0;
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

// checkDevilDisappear (updateTransparency's fade-out test while shown): a guard takes the DT test, else the original.
naked void detour_disappear() {
    _asm {
        test byte ptr [eax + 0x22D9], 8
        jnz dt
        cmp byte ptr [eax + 0x22C9], 1
        jmp dword ptr [jmp_ret_disappear]
    dt:
        jmp dword ptr [kDisappearDT]
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

bool guarding(uintptr_t pl) {
    if (!mod_enabled || !guard_enabled) return false;
    const State* s = find_state(pl);
    if (!s || !s->window || s->guard_timer > guard_window) return false;
    if (s->phase != ATTACK && !s->reguard_window) return false;    // an attack's window ends with it; a re-guard's doesn't
    if (!dt_cost || s->window_blocked) return true;               // the guard's cost is taken once per window, by its first hit
    const uintptr_t stand = st::stand_of(pl);
    const float cost = (stand && at<uint8_t>(stand, st::mIsDevilTrigger)) ? cost_guard * cost_in_dt : cost_guard;
    return at<float>(pl, kDevilTrgNow) >= cost;
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
    if (s.reguard_left > 0.0f) s.reguard_left = std::max(0.0f, s.reguard_left - dt);
    if (s.window) {
        s.guard_timer += dt;
        if (s.guard_timer > guard_window) {                      // closed: a re-guard that caught nothing ends the re-guards
            if (s.reguard_window && !s.window_blocked) {
                s.reguard_ok = false;
                s.reguard_left = 0.0f;
            }
            s.window = false;
            s.reguard_window = false;
        }
    }

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

    // a guarded hit: the stand turns to the guard (back on Nero in its clash pose), with the block's effect and sound
    if (s.guard_pending) {
        guard_feedback(pl);
        add_guard_style(pl);
        start(pl, stand, s, s.slot, guard_motion(pl, sets[s.slot]), false, 0.0f, 0.0f, true, cost_guard);
        add_guard_dt(pl, stand);
    }
    s.guard_pending = false;
    // the GUI's test: the style's guard motion as a guard plays it, without a hit (no effect, style or cost)
    if (g_test_guard && pl == (uintptr_t)devil4_sdk::get_local_player()) {
        g_test_guard = false;
        const int slot = slot_for(pl);
        start(pl, stand, s, slot, guard_motion(pl, sets[slot]), false, 0.0f, 0.0f, true, 0.0f);
    }

    taunt_tick(pl, stand, s);
    write_bytes(stand, s);
    if (s.phase == ATTACK && !s.taunting && !s.guarding) rush_loop(pl, stand, s);

    // the pad / key trigger as the virtual button, for the commands that run after this tick (the local player's pad)
    if (trigger_source != TRIG_GAME && pl == (uintptr_t)devil4_sdk::get_local_player()) {
        const bool down = trigger_down(pl);
        if (down) at<uint32_t>(pl, pl::mBtnOn) |= kVirtualButton;
        if (down && !s.held) at<uint32_t>(pl, pl::mBtnTrg) |= kVirtualButton;
        s.held = down;
    }
}

void post_command_tick(uintptr_t pl) {
    if (!is_nero(pl)) return;          // the bits are Nero's: on Dante they belong to other commands, so never read or clear them there
    uint32_t fired = 0;
    for (int i = 0; i < CMD_COUNT; ++i) {
        if (bit_set(pl, g_cmd_bit[i])) fired |= 1u << i;
        bit_clear(pl, g_cmd_bit[i]);
    }
    if (!fired || !mod_enabled) return;
    const uintptr_t stand = st::stand_of(pl);
    if (!stand) return;
    State& s = state_for(pl);
    if (s.reguard_left > 0.0f) { reguard(s); return; }             // after a guard a press guards again (reguard_time)
    if (s.cooldown > 0.0f) return;
    const int slot = slot_for(pl);
    const ActionSet& set = sets[slot];
    const int cmd = (fired & (1u << CMD_RUSH)) ? CMD_RUSH : (fired & (1u << CMD_FORWARD)) ? CMD_FORWARD
                  : (fired & (1u << CMD_BACK)) ? CMD_BACK : CMD_NEUTRAL;
    const int motion = cmd == CMD_RUSH    ? resolve_motion(pl, set.rush, set.rush_block)
                     : cmd == CMD_FORWARD ? resolve_motion(pl, set.forward, set.forward_block)
                     : cmd == CMD_BACK    ? resolve_motion(pl, set.back, set.back_block)
                     :                      resolve_motion(pl, set.neutral, set.neutral_block);
    const float cost = cmd == CMD_RUSH ? cost_rush : cmd == CMD_FORWARD ? cost_forward : cmd == CMD_BACK ? cost_back : cost_neutral;
    start(pl, stand, s, slot, motion, false, 0.0f, 0.0f, false, cost, cmd == CMD_RUSH);
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
int* g_capture_dst = nullptr;// the setting the capture writes (the trigger's or the guard's pad / key)

// The GUI's capture: pad buttons pressed together (done on release), or the first key down.
void capture_tick() {
    if (g_capture < 0 || !g_capture_dst) return;
    if (g_capture == TRIG_PAD) {
        const uintptr_t pl = (uintptr_t)devil4_sdk::get_local_player();
        const uint32_t bits = pl ? at<uint32_t>(pl, kPadBtnOn) : 0;
        if (bits) g_capture_bits |= bits;
        else if (g_capture_bits) { *g_capture_dst = (int)g_capture_bits; g_capture_bits = 0; g_capture = -1; }
    } else {
        for (int vk = 8; vk < 255; ++vk) {
            if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
            if (GetAsyncKeyState(vk) & 0x8000) { *g_capture_dst = vk == VK_ESCAPE ? 0 : vk; g_capture = -1; break; }
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
        g_cmd_bit[i] = next_free_bit(CHAR_NERO);
        if (g_cmd_bit[i] < 0 || !add_for(CHAR_NERO, g_cmd[i].head(), g_cmd_bit[i], &mod_enabled)) {
            spdlog::error("GuardianDevil: no free command bit for the {} command (Nero's 85-95 are all taken)", kCmdNames[i]);
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
        { kDisappearSite,   &hook_disappear,     &detour_disappear,     &jmp_ret_disappear,     7,  "checkDevilDisappear" },
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
    // the guard: Nero's vtable slot 83 points at guard_before_damage (inert without an open guard window)
    const uintptr_t guard_fn = reinterpret_cast<uintptr_t>(&guard_before_damage);
    for (int i = 0; i < 3; ++i) {
        if (*reinterpret_cast<const uintptr_t*>(kBeforeDamageSlots[i]) != kBeforeDamage) {
            spdlog::error("GuardianDevil: vtable slot at 0x{:X} isn't procBeforeDamageCalc; the guard is off", kBeforeDamageSlots[i]);
            continue;
        }
        char bytes[4];
        std::memcpy(bytes, &guard_fn, 4);
        install_patch_absolute(kBeforeDamageSlots[i], patch_guard[i], bytes, 4);
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

// The stand LMT block a motion plays instead of its vanilla number (-1 = the vanilla one).
void block_input(const char* id, int& block) {
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID(id);
    ImGui::PushItemWidth(90.0f);
    ImGui::InputInt(_("LMT block"), &block, 1, 10);
    ImGui::PopItemWidth();
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
    if (block < -1) block = -1;
    if (block > 255) block = 255;
    ImGui::SameLine();
    Mod::help_marker(_("-1: the vanilla motion number on the left. 0 or more: that block (slot) of the stand LMT (the files under "
                       "Taunt mimic, made in Albam), which plays instead; until those files are on the stand the vanilla motion "
                       "plays."));
}

void action_set_editor(int slot) {
    ActionSet& set = sets[slot];
    ImGui::PushID(slot);
    ImGui::Checkbox(_("Guardian in this style"), &set.enabled);
    ImGui::SameLine();
    Mod::help_marker(_("Off: the trigger does nothing while this style is picked (the stand stays the game's)."));
    hex_input(_("Neutral"), set.neutral, _("Motion the stand plays on the trigger alone (hex; the stand's Red Queen motion numbers). Default 31C."));
    block_input("neutral", set.neutral_block);
    hex_input(_("Forward"), set.forward, _("Lock-on + forward + the trigger. Default 317 (Streak)."));
    block_input("forward", set.forward_block);
    hex_input(_("Back"), set.back, _("Lock-on + back + the trigger. Default 330."));
    block_input("back", set.back_block);
    hex_input(_("Rush"), set.rush, _("Lock-on + back, forward + the trigger (Calibur's input: back, then forward within 15 frames, the trigger "
                                     "within 15 more). Default 35F (the punch rush). Holding the trigger keeps it going (the loop below)."));
    block_input("rush", set.rush_block);
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
    hex_input(_("Guard"), set.guard, _("Motion the stand plays when it guards (hex). Default 302: its sword clash, the motion it plays "
                                       "in Devil Trigger when Nero's sword is knocked back."));
    block_input("guard", set.guard_block);
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushItemWidth(90.0f);
    ImGui::InputInt(_("Guard variants"), &set.guard_blocks, 1, 1);
    ImGui::PopItemWidth();
    ImGui::Unindent(Mod::lineIndent);
    set.guard_blocks = std::clamp(set.guard_blocks, 1, 16);
    ImGui::SameLine();
    Mod::help_marker(_("How many guard animations to pick from at random: the LMT block above and the ones after it (3 with block 0: "
                       "blocks 0, 1 and 2). 1 = always that block. A block the stand LMT doesn't have plays the first one."));
    ImGui::PushItemWidth(Mod::sameLineItemWidth);
    ImGui::SliderFloat(_("Guard start frame"), &set.guard_start, 0.0f, 60.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("The frame the guard motion starts at (0)."));
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

// An input's settings: where it comes from (pad buttons / key / game input) and the binding. `commands`: the trigger,
// whose four commands are rebuilt when the button bit changes.
void input_editor(const char* label, const char* help, int& source, int& pad, int& key, int& game, bool commands) {
    static const char* kSources[] = { "Gamepad buttons", "Keyboard key", "Game input" };
    ImGui::PushID(label);
    ImGui::PushItemWidth(Mod::sameLineItemWidth);
    if (ImGui::BeginCombo(label, kSources[source])) {
        for (int i = 0; i < 3; ++i)
            if (ImGui::Selectable(kSources[i], source == i)) { source = i; if (commands) rebuild_commands(); }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    Mod::help_marker(help);
    ImGui::Indent(Mod::lineIndent);
    if (source == TRIG_PAD) {
        ImGui::PushItemWidth(Mod::sameLineItemWidth);
        const auto info = devil4_sdk::getButtonInfo((uint16_t)pad);
        char text[64];
        if (!pad) snprintf(text, sizeof text, "%s", _("none"));
        else if (info.first == (uint16_t)pad && info.second) snprintf(text, sizeof text, "%s", utility::text_lookup((char*)info.second));
        else snprintf(text, sizeof text, "buttons 0x%X", pad);
        if (ImGui::BeginCombo(_("Pad button"), text)) {
            for (const auto& pair : buttonPairs)
                if (ImGui::Selectable(utility::text_lookup((char*)pair.second), pad == pair.first)) pad = pair.first;
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (g_capture == TRIG_PAD && g_capture_dst == &pad) ImGui::TextDisabled(_("press the buttons, then let go"));
        else if (ImGui::Button(_("Capture"))) { g_capture = TRIG_PAD; g_capture_bits = 0; g_capture_dst = &pad; }
        ImGui::SameLine();
        Mod::help_marker(_("One pad button from the list, or Capture to press one or several pad buttons together (the combination "
                           "is taken when you let go)."));
    } else if (source == TRIG_KEY) {
        char name[64];
        key_name(key, name, sizeof name);
        ImGui::Text("%s: %s", _("Key"), name);
        ImGui::SameLine();
        if (g_capture == TRIG_KEY && g_capture_dst == &key) ImGui::TextDisabled(_("press a key (Escape = none)"));
        else if (ImGui::Button(_("Capture"))) { g_capture = TRIG_KEY; g_capture_dst = &key; }
        ImGui::SameLine();
        Mod::help_marker(_("Capture, then press the key to use. Escape clears it."));
    } else {
        static const struct { int bit; const char* name; } kGameInputs[] = { { 0x100, "Change Gun" }, { 0x80, "Change Devil Arm" }, { 0x8, "Taunt" } };
        const char* game_name = "custom";
        for (const auto& t : kGameInputs) if (t.bit == game) game_name = t.name;
        ImGui::PushItemWidth(Mod::sameLineItemWidth);
        if (ImGui::BeginCombo(_("Game input"), game_name)) {
            for (const auto& t : kGameInputs)
                if (ImGui::Selectable(t.name, t.bit == game)) { game = t.bit; if (commands) rebuild_commands(); }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        Mod::help_marker(_("A mapped game input, bound on your pad and keyboard in the game's control settings. Change Gun and Change "
                           "Devil Arm do nothing on Nero, but they only work if Nero's settings let you bind them; Taunt also taunts."));
    }
    ImGui::Unindent(Mod::lineIndent);
    ImGui::PopID();
}

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
    input_editor(_("Trigger"),
                 _("What triggers the stand. Gamepad buttons: raw pad buttons (held together), whatever the game maps them to. "
                   "Keyboard key: a key, while the game is in front and this menu is closed. Game input: one of Nero's mapped "
                   "inputs, bound in the game's control settings. Either way the stick part is the game's own command checker: "
                   "lock-on + forward / back, or lock-on + back, forward for the rush."),
                 trigger_source, trigger_pad, trigger_key, trigger_game, true);
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

    ImGui::SeparatorText(_("DT cost"));
    ImGui::Checkbox(_("Stand actions cost DT"), &dt_cost);
    ImGui::SameLine();
    help_marker(_("Each stand action takes DT from Nero's gauge when it starts (1000 = one bar), and is refused when the gauge "
                  "has less. The punch rush also drains the gauge while it is held, and stops when the gauge runs dry. The "
                  "taunt mimic is free."));
    if (dt_cost) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        const struct { const char* label; float* value; const char* help; } costs[] = {
            { _("Neutral cost"), &cost_neutral, _("DT the trigger alone takes (500 = half a bar).") },
            { _("Forward cost"), &cost_forward, _("DT lock-on + forward + the trigger takes (500).") },
            { _("Back cost"),    &cost_back,    _("DT lock-on + back + the trigger takes (500).") },
            { _("Rush cost"),    &cost_rush,    _("DT the punch rush takes when it starts (500); holding it drains more, below.") },
            { _("Guard cost"),   &cost_guard,   _("DT a guard takes (0: the guard stays free).") },
        };
        for (const auto& c : costs) {
            ImGui::SliderFloat(c.label, c.value, 0.0f, 3000.0f, "%.0f");
            ImGui::SameLine();
            help_marker(c.help);
        }
        ImGui::SliderFloat(_("Rush drain"), &rush_drain, 0.0f, 3000.0f, "%.0f / s");
        ImGui::SameLine();
        help_marker(_("DT per second while the punch rush is held past its wind-up (500 = half a bar a second). When the gauge "
                      "runs dry the rush stops as if the trigger were let go."));
        ImGui::SliderFloat(_("Cost in Devil Trigger"), &cost_in_dt, 0.0f, 1.0f, "x%.2f");
        ImGui::SameLine();
        help_marker(_("While Nero is in Devil Trigger every cost above and the rush drain are multiplied by this (0.5 = half; "
                      "0 = free in DT, 1 = the same as outside it)."));
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }

    ImGui::SeparatorText(_("Guard"));
    ImGui::Checkbox(_("Stand guard"), &guard_enabled);
    ImGui::SameLine();
    help_marker(_("A timed shield on the Guardian action: a hit that lands within the window after you start a stand attack "
                  "does nothing to Nero (no damage, no flinch, no style drop), and the attack turns into a guard: the stand "
                  "appears on Nero in its sword clash. While it guards, pressing the button again guards the next hit instead "
                  "of attacking (time each press: one that catches nothing ends the re-guards until the next guard). The guard "
                  "does nothing to the enemy. After a guard, no stand attack starts until the recovery is over. Not guarded: "
                  "enemy grabs and the few attacks the game handles as pressure. The guard's motion is set per style below."));
    if (guard_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::SliderFloat(_("Guard window"), &guard_window, 1.0f, 15.0f, "%.0f frames");
        ImGui::SameLine();
        help_marker(_("How long after a stand attack's press a hit is still guarded (5 frames, Dante's Just Block window). Counted "
                      "in game time, so slow motion stretches it as it does for the game."));
        ImGui::SliderFloat(_("Guard recovery"), &guard_recovery, 0.0f, 120.0f, "%.0f frames");
        ImGui::SameLine();
        help_marker(_("Frames after a guard before the next stand attack can start (15). Guarding again doesn't wait for it."));
        ImGui::SliderFloat(_("Re-guard time"), &reguard_time, 0.0f, 120.0f, "%.0f frames");
        ImGui::SameLine();
        help_marker(_("For this long after a guard, pressing the Guardian action guards the next hit instead of attacking, "
                      "however short the guard animation is (45 frames). A re-guard that catches a hit brings the stand back in "
                      "its guard pose and starts this time again; one that catches nothing ends the re-guards until the next "
                      "guard."));
        ImGui::PopItemWidth();
        ImGui::Checkbox(_("Guard effect and sound"), &guard_effects);
        ImGui::SameLine();
        help_marker(_("A guard plays the game's own stand block spark and sound, and the pad rumble of a sword clash."));
        ImGui::Checkbox(_("Guard adds DT"), &guard_dt);
        ImGui::SameLine();
        help_marker(_("A guard that catches a hit fills the Devil Trigger gauge, as the game's own stand block does. Nothing "
                      "while Nero is in Devil Trigger (as the game's other DT gains). With a guard cost, the cost is taken too."));
        if (guard_dt) {
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::SliderFloat(_("Guard DT gain"), &guard_dt_gain, -1.0f, 3000.0f, guard_dt_gain < 0.0f ? "game value" : "%.0f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            help_marker(_("DT per guard (1000 = one bar). All the way left = the game's own stand block value, read from Nero's "
                          "parameters (the readout shows it)."));
            if (guard_dt_gain > -1.0f && guard_dt_gain < 0.0f) guard_dt_gain = 0.0f;
        }
        ImGui::Checkbox(_("Guard adds style"), &guard_style);
        ImGui::SameLine();
        help_marker(_("A guard adds style points as the game's own stand block does (without the block's Devil Trigger gain)."));
        if (ImGui::Button(_("Play Guard Motion"))) g_test_guard = true;
        ImGui::SameLine();
        help_marker(_("Plays the current style's guard motion on the stand now, as a guard would (its LMT block or vanilla number, "
                      "and start frame), without a hit: no effect, sound, style or DT cost. The readout below says which "
                      "motion played and where it came from, or why the stand refused."));
        ImGui::Unindent(lineIndent);
    }

    ImGui::SeparatorText(_("Taunt mimic"));
    ImGui::Checkbox(_("Stand mimics Nero's taunt"), &taunt_mimic);
    ImGui::SameLine();
    help_marker(_("When Nero taunts, the stand appears beside him and plays the same taunt, and stops when he does. The stand's own "
                  "motion files have no taunts and its bones are numbered differently, so Nero's taunts are retargeted onto the "
                  "stand and its arm with Albam (scripts/stand_taunt_retarget.py) into the two LMTs below, loose files under "
                  "nativePC, which the motion registry puts on the stand."));
    ImGui::PushItemWidth(sameLineWidth);
    bool lmt_changed = ImGui::InputText(_("Stand LMT (body)"), taunt_lmt, sizeof taunt_lmt);
    ImGui::SameLine();
    help_marker(_("Game path without .lmt, read from nativePC: the stand body's taunts, with Nero's block numbers. Its other "
                  "blocks hold custom stand moves, which the action sets' LMT block fields play."));
    lmt_changed |= ImGui::InputText(_("Stand LMT (arm)"), taunt_arm_lmt, sizeof taunt_arm_lmt);
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
            ImGui::Text("last motion from: %s | sword in %s", g_motion_source.c_str(),
                        !stand ? "-" : at<int>(stand, kSwordParentJoint) == 150 ? "the hand (150)"
                                     : at<int>(stand, kSwordParentJoint) == 151 ? "the sheath (151)" : "?");
            if (stand) ImGui::Text("%s", sword_event_debug(stand).c_str());
            ImGui::Text("guard %s%s | re-guard %.0f | %.1f frames since the press | guards %u blocked %u | DT %.0f (+%.0f a guard)%s",
                        guarding(P) ? "OPEN" : (s && s->guarding) ? "playing" : "-", (s && !s->reguard_ok) ? " (re-guards locked)" : "",
                        s ? s->reguard_left : 0.0f, s ? s->guard_timer : 0.0f, s ? s->guards : 0u, s ? s->blocked : 0u, at<float>(P, kDevilTrgNow),
                        guard_dt ? guard_dt_amount(P) : 0.0f, (s && s->drained) ? " (rush ran dry)" : "");
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
    // the files were pl020_taunt / wp024_taunt (the guardian_taunt package) before they held more than taunts: a saved old
    // default moves to the new names, any other saved path is kept
    if (auto p = cfg.get("dt_knuckle_taunt_lmt"); p && *p != "motion\\pl020\\pl020_taunt") strcpy_s(taunt_lmt, p->c_str());
    if (auto p = cfg.get("dt_knuckle_taunt_arm_lmt"); p && *p != "motion\\wp024\\wp024_taunt") strcpy_s(taunt_arm_lmt, p->c_str());
    taunt_back           = cfg.get<float>("dt_knuckle_taunt_back").value_or(taunt_back);
    taunt_side           = cfg.get<float>("dt_knuckle_taunt_side").value_or(taunt_side);
    guard_enabled        = cfg.get<bool>("dt_knuckle_guard").value_or(guard_enabled);
    guard_window         = cfg.get<float>("dt_knuckle_guard_window").value_or(guard_window);
    guard_recovery       = cfg.get<float>("dt_knuckle_guard_recovery").value_or(guard_recovery);
    reguard_time         = cfg.get<float>("dt_knuckle_reguard_time").value_or(reguard_time);
    guard_effects        = cfg.get<bool>("dt_knuckle_guard_effects").value_or(guard_effects);
    guard_style          = cfg.get<bool>("dt_knuckle_guard_style").value_or(guard_style);
    guard_dt             = cfg.get<bool>("dt_knuckle_guard_dt").value_or(guard_dt);
    guard_dt_gain        = cfg.get<float>("dt_knuckle_guard_dt_gain").value_or(guard_dt_gain);
    dt_cost              = cfg.get<bool>("dt_knuckle_dt_cost").value_or(dt_cost);
    cost_neutral         = cfg.get<float>("dt_knuckle_cost_neutral").value_or(cost_neutral);
    cost_forward         = cfg.get<float>("dt_knuckle_cost_forward").value_or(cost_forward);
    cost_back            = cfg.get<float>("dt_knuckle_cost_back").value_or(cost_back);
    cost_rush            = cfg.get<float>("dt_knuckle_cost_rush").value_or(cost_rush);
    cost_guard           = cfg.get<float>("dt_knuckle_cost_guard").value_or(cost_guard);
    rush_drain           = cfg.get<float>("dt_knuckle_rush_drain").value_or(rush_drain);
    cost_in_dt           = cfg.get<float>("dt_knuckle_cost_in_dt").value_or(cost_in_dt);
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
        set.guard      = cfg.get<int>(p + "guard_motion").value_or(set.guard);
        set.guard_start = cfg.get<float>(p + "guard_motion_start").value_or(set.guard_start);
        set.neutral_block = cfg.get<int>(p + "neutral_block").value_or(set.neutral_block);
        set.forward_block = cfg.get<int>(p + "forward_block").value_or(set.forward_block);
        set.back_block    = cfg.get<int>(p + "back_block").value_or(set.back_block);
        set.rush_block    = cfg.get<int>(p + "rush_block").value_or(set.rush_block);
        set.guard_block   = cfg.get<int>(p + "guard_block").value_or(set.guard_block);
        set.guard_blocks  = cfg.get<int>(p + "guard_blocks").value_or(set.guard_blocks);
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
    cfg.set<bool>("dt_knuckle_guard", guard_enabled);
    cfg.set<float>("dt_knuckle_guard_window", guard_window);
    cfg.set<float>("dt_knuckle_guard_recovery", guard_recovery);
    cfg.set<float>("dt_knuckle_reguard_time", reguard_time);
    cfg.set<bool>("dt_knuckle_guard_effects", guard_effects);
    cfg.set<bool>("dt_knuckle_guard_style", guard_style);
    cfg.set<bool>("dt_knuckle_guard_dt", guard_dt);
    cfg.set<float>("dt_knuckle_guard_dt_gain", guard_dt_gain);
    cfg.set<bool>("dt_knuckle_dt_cost", dt_cost);
    cfg.set<float>("dt_knuckle_cost_neutral", cost_neutral);
    cfg.set<float>("dt_knuckle_cost_forward", cost_forward);
    cfg.set<float>("dt_knuckle_cost_back", cost_back);
    cfg.set<float>("dt_knuckle_cost_rush", cost_rush);
    cfg.set<float>("dt_knuckle_cost_guard", cost_guard);
    cfg.set<float>("dt_knuckle_rush_drain", rush_drain);
    cfg.set<float>("dt_knuckle_cost_in_dt", cost_in_dt);
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
        cfg.set<int>(p + "guard_motion", set.guard);
        cfg.set<float>(p + "guard_motion_start", set.guard_start);
        cfg.set<int>(p + "neutral_block", set.neutral_block);
        cfg.set<int>(p + "forward_block", set.forward_block);
        cfg.set<int>(p + "back_block", set.back_block);
        cfg.set<int>(p + "rush_block", set.rush_block);
        cfg.set<int>(p + "guard_block", set.guard_block);
        cfg.set<int>(p + "guard_blocks", set.guard_blocks);
    }
}
