#include "GuardianDevil.hpp"
#include "../sdk/NeroStand.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/Players.hpp"
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
bool  guard_enabled        = true;
bool  dt_cost              = false;
bool  cancel_on_hit        = false;

namespace {

// ---- The fixed values ------------------------------------------------------------------------------------------------
// The stand's actions, the same in every NeroStyle slot: a motion of an action is its vanilla stand motion number, or a
// block of the stand LMT when its block isn't -1 (motion_registry::word; the vanilla motion while the set isn't on the
// stand yet).
struct ActionSet {
    int   neutral;        // the trigger alone (0x31C)
    int   forward;        // lock-on + forward + the trigger (0x317, Streak)
    int   back;           // lock-on + back + the trigger (0x330)
    int   rush;           // lock-on + back, forward + the trigger (0x35F, the punch rush)
    float rush_start;     // the rush motion's start frame (its wind-up skipped: 135)
    float loop_from;      // while the trigger is held, a rush frame in [loop_from, loop_to] jumps back to loop_dest
    float loop_to;
    float loop_dest;
    int   guard;          // the guard's motion (0x302, the stand's sword clash)
    float guard_start;    // its start frame
    int   neutral_block;
    int   forward_block;
    int   back_block;
    int   rush_block;
    int   guard_block;    // block 0 of the stand LMT (the guardian_ex package's guard)
    int   guard_blocks;   // each guard picks one of this many blocks from guard_block on at random (blocks 0-2)
};
constexpr ActionSet kSet = { 0x31C, 0x317, 0x330, 0x35F, 135.0f, 170.0f, 172.0f, 150.0f, 0x302, 0.0f, -1, -1, -1, -1, 0, 3 };
// The stand LMT block played when Nero switches into a NeroStyle slot (-1 = none): slots 0 / 1 / 3 are the Devil Bringer,
// Gerbera and Gunslinger, whose poses are blocks 10, 12 and 11 of guardian_ex.
constexpr int kSwitchBlock[nero_style::kSlots] = { 10, 12, -1, 11 };

constexpr float kCooldown      = 30.0f;    // frames between starts
constexpr float kTauntBack     = 80.0f;    // the taunt mimic's stand: cm behind Nero along his facing
constexpr float kGuardWindow   = 5.0f;     // frames after a press in which a hit is guarded (Dante's Just Block window)
constexpr float kGuardRecovery = 15.0f;    // frames after a guard before the next stand attack
constexpr float kReguardTime   = 45.0f;    // frames after a guard in which a press guards again instead of attacking
constexpr float kActionCost    = 500.0f;   // with dt_cost: DT each start takes (half a bar; the guard is free)
constexpr float kRushDrain     = 500.0f;   // with dt_cost: DT per second while the rush is held past its wind-up
constexpr float kCostInDt      = 0.5f;     // the cost and the drain while Nero is in Devil Trigger
// The stand LMT pair (Nero's taunts retargeted at his block numbers, the custom stand moves in the free blocks), loose
// files under nativePC: mod-manager package guardian_ex.
const char* const kStandLmt    = "motion\\pl020\\pl020_guardian_ex";
const char* const kStandArmLmt = "motion\\wp024\\wp024_guardian_ex";

// ---- The game ---------------------------------------------------------------------------------------------------
char* const kRushEffectRun = "effect\\efl\\vie\\vi020_00v0";   // while the rush runs
char* const kRushEffectEnd = "effect\\efl\\vie\\vi020_00v1";   // when it ends
char* const kSwitchEffect  = "effect\\efl\\com\\ec024_60v0";   // a switch pose: Nero's DT-on flash (list 1 index 0x40,
                                                               // uPlayerNero::setDevilTriggerCommon's), on the stand
constexpr int kSwitchSe = 41;                                  // with BringerAppear (pl000_style00 request 41)
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

bool g_test_guard = false; // the GUI's Play Guard Motion: the local Nero's next tick plays the style's guard motion

// ---- Per-player state ---------------------------------------------------------------------------------------------
constexpr int kPlayerSlots = 8;
enum Phase : int { IDLE = 0, ATTACK = 1, FADE = 2 };
enum Kind : int { KIND_ATTACK = 0 /* an input's attack or a guard */, KIND_TAUNT = 1 /* the taunt mimic */, KIND_SWITCH = 2 /* a style switch pose */ };

struct State {
    uintptr_t   player = 0;
    uint32_t    last_seen = 0;
    int         phase = IDLE;
    float       cooldown = 0.0f;
    int         motion = -1;          // the running attack's motion
    uintptr_t   effect = 0;           // the rush's running effect
    bool        effect_ended = false; // its end effect played (the button let go in the loop window)
    bool        drained = false;      // the rush ran the DT gauge dry: it stops looping
    bool        held = false;         // the pad / key trigger was down last tick (the press edge)
    bool        taunting = false;     // the running attack is a taunt mimic (ends with Nero's taunt)
    bool        rushing = false;      // the running attack is the rush (its loop and effects)
    bool        taunt_tried = false;  // this taunt of Nero's was answered (or refused) already
    bool        switching = false;    // the running action is a style switch pose
    bool        pose_stopped = false; // the pose's motions are stopped on their last frame (A_STOP set by us)
    bool        pose_held = false;    // the pose has had its hold
    float       pose_hold = 0.0f;     // frames held so far
    bool        switches_known = false;// seen_switches holds NeroStyle's count for this Nero
    uint32_t    seen_switches = 0;    // NeroStyle's switch count last tick (a higher one = a switch)
    bool        window = false;       // a guard window is open: an attack's press or a re-guard opened it
    float       guard_timer = 0.0f;   // frames since that press (delta time, as Dante's Just Block counter)
    bool        window_blocked = false;// the open window caught a hit
    bool        guard_pending = false;// a hit was guarded: the stand turns to the guard on the next tick
    bool        guarding = false;     // the running action is the guard
    bool        reguard_ok = true;    // a press may guard again (off after a re-guard caught nothing)
    float       reguard_left = 0.0f;  // frames left in which a press re-guards (kReguardTime from each guard)
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
    case TRIG_KEY:  return key != 0 && player_info::has_keyboard(pl) && keyboard_allowed() && (GetAsyncKeyState(key) & 0x8000) != 0;
    case TRIG_GAME: return game != 0 && (at<uint32_t>(pl, pl::mBtnOn) & (uint32_t)game) != 0;
    default: return false;
    }
}
bool key_down(uintptr_t pl) { return input_down(pl, TRIG_KEY, 0, trigger_key, 0); }
bool trigger_down(uintptr_t pl) { return input_down(pl, trigger_source, trigger_pad, 0, trigger_game) || key_down(pl); }

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
    if (s.phase != IDLE)    mod |= st::MOD_PLACED;
    if (s.phase == ATTACK && s.guarding) mod |= st::MOD_GUARD;
    at<uint8_t>(stand, st::kMod) = mod;
    at<uint8_t>(stand, st::kShown) = (at<uint8_t>(stand, st::mIsDevilTrigger) || s.phase != IDLE) ? 1 : 0;
}

// The start: what setRedQueenMotion does in DT, on a stand placed on Nero (`back` cm behind him along his facing) and
// detached, fading in at once. `kind`: an attack (or guard), the taunt mimic, or a style switch pose (neither
// opens a guard window; a pose sets no cooldown).
// `guard`: a guarded hit turns the running attack into the guard motion from its start frame (the attack's window stays
// as it is) and the guard's recovery starts. Any other attack opens a guard window.
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

// A switch pose holds its last frame for kPoseHold frames before the stand ends it and fades: motion slot 0 of the stand
// and of its arm gets uModel's A_STOP (attr word +0x33A bit 0x10: uModel::updateFrame doesn't advance the frame, so the
// motion doesn't step past mFrameMax - 1 and isn't ended), then the bit is cleared and the motion ends on its next step.
constexpr float     kPoseHold     = 10.0f;
constexpr uintptr_t kMotAttr0     = 0x33A;    // MotionArray[0].mAttr (u16)
constexpr uintptr_t kMotFrameMax0 = 0x34C;    // MotionArray[0].mFrameMax (float, the clip's frame count)
constexpr uint16_t  kMotStop      = 0x10;     // A_STOP

void stop_motion(uintptr_t model, bool on) {
    if (!model) return;
    uint16_t& attr = at<uint16_t>(model, kMotAttr0);
    attr = on ? (attr | kMotStop) : (attr & ~kMotStop);
}
void unstop_pose(uintptr_t stand, State& s) {
    if (!s.pose_stopped) return;
    stop_motion(stand, false);
    stop_motion(st::hand_of(stand), false);
    s.pose_stopped = false;
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
int guard_motion(uintptr_t pl) {
    static std::minstd_rand rng(GetTickCount());
    int block = kSet.guard_block;
    if (block >= 0 && kSet.guard_blocks > 1) {
        const int pick = block + (int)(rng() % (uint32_t)kSet.guard_blocks);
        if (g_taunt_set < 0 || !motion_registry::ready(g_taunt_set, pl) || motion_registry::word(g_taunt_set, pl, pick) >= 0) block = pick;
    }
    return resolve_motion(pl, kSet.guard, block);
}

void start(uintptr_t pl, uintptr_t stand, State& s, int motion, Kind kind = KIND_ATTACK, float back = 0.0f, bool guard = false,
           float cost = 0.0f, bool rush = false) {
    const char* why = nullptr;
    const int action = at<int>(stand, st::mActionNo);
    if (!dt_cost) cost = 0.0f;
    else if (at<uint8_t>(stand, st::mIsDevilTrigger)) cost *= kCostInDt;   // Nero's DT state is the stand's
    if (cost > 0.0f && at<float>(pl, kDevilTrgNow) < cost)       why = "not enough DT";
    else if (motion < 0)                                         why = kind == KIND_TAUNT  ? "the taunt LMTs aren't on the stand yet"
                                                                     : kind == KIND_SWITCH ? "the switch pose's LMT block isn't on the stand"
                                                                     :                       "no motion for this input";
    else if (action != st::ACT_NONE && action != st::ACT_YAMATO) why = "the stand is busy";
    else if (at<uint8_t>(stand, st::mIsDevilTriggerReserve))     why = "a DT change is pending";
    else nero_locked_out(pl, &why);
    if (why) { ++s.refused; s.last_refusal = why; return; }

    std::memcpy(&at<float>(stand, st::mPos), &at<float>(pl, pl::mPos), 12);
    if (back != 0.0f) {                                          // game axes: forward = (sin yaw, 0, cos yaw)
        const float yaw = at<float>(pl, st::pl_mFacingYaw);
        at<float>(stand, st::mPos)     -= std::sin(yaw) * back;
        at<float>(stand, st::mPos + 8) -= std::cos(yaw) * back;
    }
    std::memcpy(&at<float>(stand, st::mQuat), &at<float>(pl, pl::mQuat), 16);
    at<float>(stand, st::mAnglY) = at<float>(pl, st::pl_mFacingYaw);
    at<uint8_t>(stand, st::mIsTraceParent) = 0;
    unstop_pose(stand, s);
    st::set_action(stand, st::ACT_YAMATO);
    st::yamato_param(stand, motion, 4, guard ? kSet.guard_start : rush ? kSet.rush_start : 0.0f);
    at<int>(stand, st::mTpPhase) = st::TP_FADE_IN;
    if (const uintptr_t target = at<uintptr_t>(pl, st::pl_mLockOnTarget)) st::set_snatch_param(stand, target);

    if (cost > 0.0f) at<float>(pl, kDevilTrgNow) -= cost;
    stop_effect(s);
    if (rush) s.effect = (uintptr_t)devil4_sdk::effect_generator(kRushEffectRun, (void*)stand, 0x14);
    s.effect_ended = false;
    s.drained = false;
    s.phase = ATTACK;
    s.motion = motion;
    s.taunting = kind == KIND_TAUNT;
    s.switching = kind == KIND_SWITCH;
    s.pose_held = false;
    s.pose_hold = 0.0f;
    s.rushing = rush;
    s.guarding = guard;
    s.guard_pending = false;
    if (guard) {
        s.reguard_ok = true;
        s.reguard_left = kReguardTime;
    } else if (kind != KIND_SWITCH) {                            // a pose leaves the guard state (re-guards) as it is
        s.window = guard_enabled && kind == KIND_ATTACK;
        s.guard_timer = 0.0f;
        s.window_blocked = false;
        s.reguard_window = false;
        s.reguard_left = 0.0f;
    }
    if (kind != KIND_SWITCH) s.cooldown = guard ? kGuardRecovery : kCooldown;
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
    s.switching = false;
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
    if (!provoking || s.taunt_tried || s.phase == ATTACK) return;
    const uint32_t motion = at<uint32_t>(pl, pl::mMotionCtrl0No);
    if (!is_taunt_motion(motion)) return;                        // the taunt motion is requested a tick after the action starts
    s.taunt_tried = true;
    int word = -1;
    if (g_taunt_set >= 0 && motion_registry::ready(g_taunt_set, pl)) word = motion_registry::word(g_taunt_set, pl, (int)motion);
    start(pl, stand, s, word, KIND_TAUNT, kTauntBack);
}

// The switch pose's start: the DT-on flash on the stand and the Devil Bringer's appear sound, from the stand's place.
void switch_feedback(uintptr_t pl, uintptr_t stand) {
    devil4_sdk::effect_generator(kSwitchEffect, (void*)stand, 0x14);
    const uintptr_t inst = *(uintptr_t*)kSoundInstance;
    const uintptr_t bank = at<uintptr_t>(pl, kSeBanks + 4);
    if (inst && bank)
        ((void(__stdcall*)(uintptr_t, uintptr_t, int, uintptr_t, const float*, int, int, uintptr_t))fn_requestSe)(
            inst, bank, kSwitchSe, pl, &at<float>(stand, st::mPos), 1, 1, fn_sePositionCb);
}

// NeroStyle switched Nero's style: the stand plays the new style's switch pose (its kSwitchBlock), on Nero, with
// switch_feedback, unless an attack or a guard runs (a pose or a taunt is replaced). Clearing the style (no style) plays
// nothing.
void switch_tick(uintptr_t pl, uintptr_t stand, State& s) {
    int style = nero_style::NONE;
    const uint32_t count = nero_style::switches(pl, &style);
    const bool switched = s.switches_known && count > s.seen_switches;   // a lower count: NeroStyle started over for this Nero
    s.seen_switches = count;
    s.switches_known = true;
    if (!switched) return;
    const int slot = style - nero_style::BASE;
    if (slot < 0 || slot >= nero_style::kSlots || kSwitchBlock[slot] < 0) return;
    if (s.phase == ATTACK && !s.taunting && !s.switching) { ++s.refused; s.last_refusal = "style switch: the stand is attacking"; return; }
    const uint32_t starts = s.starts;
    start(pl, stand, s, resolve_motion(pl, -1, kSwitchBlock[slot]), KIND_SWITCH);
    if (s.starts != starts) switch_feedback(pl, stand);
}

// The pose's hold: a tick before the stand's motion would reach its last frame (this tick runs before the stand's update),
// put it on that frame and stop it (and the arm's), kPoseHold frames, then let it end. Anything that ends or replaces the
// pose first lets the motions go.
void pose_hold_tick(uintptr_t stand, State& s, float dt) {
    if (s.phase != ATTACK || !s.switching) { unstop_pose(stand, s); return; }
    if (s.pose_stopped) {
        s.pose_hold += dt;
        if (s.pose_hold >= kPoseHold) { unstop_pose(stand, s); s.pose_held = true; }
        return;
    }
    if (s.pose_held) return;
    const float last = at<float>(stand, kMotFrameMax0) - 1.0f;
    if (last < 0.0f || at<float>(stand, st::mFrame0) < last - 1.0f) return;
    at<float>(stand, st::mFrame0) = last;
    stop_motion(stand, true);
    stop_motion(st::hand_of(stand), true);   // a shorter arm motion has already stopped on its own last frame
    s.pose_stopped = true;
    s.pose_hold = 0.0f;
}
void end_fade(uintptr_t stand, State& s) {
    at<uint8_t>(stand, st::mIsTraceParent) = 1;
    s.phase = IDLE;
}

// The rush: while the trigger is held, a frame in the loop window jumps back; let go, the end effect plays once. With
// dt_cost, holding it past the wind-up (from the loop's target frame) drains the gauge; dry, it stops looping.
void rush_loop(uintptr_t pl, uintptr_t stand, State& s) {
    const float frame = at<float>(stand, st::mFrame0);
    if (s.rushing) {
        bool held = trigger_down(pl) && !s.drained && !s.effect_ended;
        if (held && dt_cost && frame >= kSet.loop_dest) {
            float& gauge = at<float>(pl, kDevilTrgNow);
            const float rate = at<uint8_t>(stand, st::mIsDevilTrigger) ? kRushDrain * kCostInDt : kRushDrain;
            const float take = rate * at<float>(pl, pl::m_delta_time) / 60.0f;
            if (gauge >= take) gauge -= take;
            else { gauge = 0.0f; s.drained = true; held = false; }
        }
        if (frame < kSet.loop_from || frame > kSet.loop_to) return;
        if (held) {
            at<float>(stand, st::mFrame0) = kSet.loop_dest;
            if (const uintptr_t hand = st::hand_of(stand)) at<float>(hand, st::mFrame0) = kSet.loop_dest;
        } else if (!s.effect_ended) {
            stop_effect(s);
            devil4_sdk::effect_generator(kRushEffectEnd, (void*)stand, 0x14);
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

// The vanilla block's style for a guard, without its DT bonus (add_guard_dt adds that).
void add_guard_style(uintptr_t pl) {
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

// The DT a guard adds: the vanilla stand block's bonus (player params +0x48, what procBeforeDamageCalc hands addComboGauge).
// Added here rather than through addComboGauge, which skips it while a recent guard is still in sStylishCount's list; like
// addComboGauge, nothing while Nero is in Devil Trigger.
float guard_dt_amount(uintptr_t pl) {
    const uintptr_t prm = at<uintptr_t>(pl, kPlayerParam);
    return prm ? at<float>(prm, 0x48) : 0.0f;
}
void add_guard_dt(uintptr_t pl, uintptr_t stand) {
    if (at<uint8_t>(stand, st::mIsDevilTrigger)) return;
    float& now = at<float>(pl, kDevilTrgNow);
    now = std::min(now + guard_dt_amount(pl), at<float>(pl, kDevilTrgNow + 4));   // mDevilTrg.max
}

// A Guardian action press after a guard: a new window, once the last one has closed, unless a re-guard missed.
void reguard(State& s) {
    if (!guard_enabled || !s.reguard_ok || (s.window && s.guard_timer <= kGuardWindow)) return;
    s.window = true;
    s.guard_timer = 0.0f;
    s.window_blocked = false;
    s.reguard_window = true;
}

// The mod turned off (or never on) for a player whose stand still carries our state: give the stand back.
void release(uintptr_t stand, State& s) {
    stop_effect(s);
    if (s.phase != IDLE) at<uint8_t>(stand, st::mIsTraceParent) = 1;
    s.phase = IDLE;
    unstop_pose(stand, s);
    s.taunting = false;
    s.switching = false;
    s.guarding = false;
    s.window = false;
    s.window_blocked = false;
    s.reguard_window = false;
    s.reguard_left = 0.0f;
    s.guard_pending = false;
    at<uint8_t>(stand, st::kMod) = 0;
    at<uint8_t>(stand, st::kShown) = at<uint8_t>(stand, st::mIsDevilTrigger);
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
uintptr_t jmp_ret_gate = 0, jmp_ret_phase0 = 0, jmp_ret_follow = 0, jmp_ret_dt_request = 0, jmp_ret_kabuto = 0, jmp_ret_disappear = 0;

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
// skipped while the mod holds the stand where it placed it (an attack, and its fade-out).
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
    if (!s || !s->window || s->guard_timer > kGuardWindow) return false;
    return s->phase == ATTACK || s->reguard_window;               // an attack's window ends with it; a re-guard's doesn't
}

uint32_t trigger_bit() { return trigger_source == TRIG_GAME ? (uint32_t)trigger_game : kVirtualButton; }

void register_taunt_set() {
    motion_registry::SetDesc d;
    d.owner = "GuardianDevil taunt";
    d.target = motion_registry::STAND;
    d.stand = kStandLmt;
    d.stand_sub = kStandArmLmt;
    const int old = g_taunt_set;
    g_taunt_set = motion_registry::add(d);
    if (old >= 0) motion_registry::remove(old);
}

void rebuild_commands() {
    const uint32_t bit = trigger_bit();
    for (input_commands::Command& c : g_cmd)
        for (int k = 0; k < c.num; ++k)
            if (c.steps[k].type == input_commands::CMD_TYPE_BTN) c.steps[k].btn = bit;
}

void pre_tick(uintptr_t pl) {
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
        if (s.guard_timer > kGuardWindow) {                      // closed: a re-guard that caught nothing ends the re-guards
            if (s.reguard_window && !s.window_blocked) {
                s.reguard_ok = false;
                s.reguard_left = 0.0f;
            }
            s.window = false;
            s.reguard_window = false;
        }
    }

    if (s.phase == ATTACK) {
        const char* why = nullptr;
        if (at<int>(stand, st::mActionNo) != st::ACT_YAMATO) end_attack(stand, s);
        else if (cancel_on_hit && nero_locked_out(pl, &why)) { stop_effect(s); s.phase = FADE; }   // the game's gate cancels it next tick
    }
    if (s.phase == FADE && (at<int>(stand, st::mTpPhase) == st::TP_HIDDEN || at<uint8_t>(stand, st::mIsDevilTrigger))) end_fade(stand, s);

    // a guarded hit: the stand turns to the guard (back on Nero in its clash pose), with the block's effect and sound
    if (s.guard_pending) {
        guard_feedback(pl);
        add_guard_style(pl);
        start(pl, stand, s, guard_motion(pl), KIND_ATTACK, 0.0f, true);
        add_guard_dt(pl, stand);
    }
    s.guard_pending = false;
    // the GUI's test: the guard motion as a guard plays it, without a hit (no effect, style or DT)
    if (g_test_guard && pl == (uintptr_t)devil4_sdk::get_local_player()) {
        g_test_guard = false;
        start(pl, stand, s, guard_motion(pl), KIND_ATTACK, 0.0f, true);
    }

    switch_tick(pl, stand, s);
    taunt_tick(pl, stand, s);
    pose_hold_tick(stand, s, dt);
    write_bytes(stand, s);
    if (s.phase == ATTACK && !s.taunting && !s.switching && !s.guarding) rush_loop(pl, stand, s);

    // the trigger's bit for the commands that run after this tick: the pad buttons and the key as the virtual button; with a
    // game input the pad sets that bit itself, so the key sets it too. Each player's own pad, the key for the player who
    // has the keyboard.
    if (player_info::index_of(pl) >= 0) {
        const bool down = trigger_source == TRIG_GAME ? key_down(pl) : trigger_down(pl);
        const uint32_t bit = trigger_bit();
        if (down) at<uint32_t>(pl, pl::mBtnOn) |= bit;
        if (down && !s.held) at<uint32_t>(pl, pl::mBtnTrg) |= bit;
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
    if (s.reguard_left > 0.0f) { reguard(s); return; }             // after a guard a press guards again (kReguardTime)
    if (s.cooldown > 0.0f) return;
    const int cmd = (fired & (1u << CMD_RUSH)) ? CMD_RUSH : (fired & (1u << CMD_FORWARD)) ? CMD_FORWARD
                  : (fired & (1u << CMD_BACK)) ? CMD_BACK : CMD_NEUTRAL;
    const int motion = cmd == CMD_RUSH    ? resolve_motion(pl, kSet.rush, kSet.rush_block)
                     : cmd == CMD_FORWARD ? resolve_motion(pl, kSet.forward, kSet.forward_block)
                     : cmd == CMD_BACK    ? resolve_motion(pl, kSet.back, kSet.back_block)
                     :                      resolve_motion(pl, kSet.neutral, kSet.neutral_block);
    start(pl, stand, s, motion, KIND_ATTACK, 0.0f, false, kActionCost, cmd == CMD_RUSH);
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
        { kDtRequestSite,   &hook_dt_request,    &detour_dt_request,    &jmp_ret_dt_request,    5,  "setRedQueenMotion" },
        { kKabutoSite,      &hook_kabuto,        &detour_kabuto,        &jmp_ret_kabuto,        7,  "setYamatoMotionStop" },
        { kDisappearSite,   &hook_disappear,     &detour_disappear,     &jmp_ret_disappear,     7,  "checkDevilDisappear" },
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

const char* phase_name(int phase) { return phase == ATTACK ? "attack" : phase == FADE ? "fading" : "idle"; }

// An input's settings: the controller's (pad buttons or a game input) and a keyboard key, both live. `commands`: the
// trigger, whose four commands are rebuilt when the button bit changes.
void input_editor(const char* label, const char* help, int& source, int& pad, int& key, int& game, bool commands) {
    static const struct { int source; const char* name; } kSources[] = { { TRIG_PAD, "Gamepad buttons" }, { TRIG_GAME, "Game input" } };
    ImGui::PushID(label);
    ImGui::PushItemWidth(Mod::sameLineItemWidth);
    if (ImGui::BeginCombo(label, source == TRIG_GAME ? kSources[1].name : kSources[0].name)) {
        for (const auto& s : kSources)
            if (ImGui::Selectable(s.name, source == s.source)) { source = s.source; if (commands) rebuild_commands(); }
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
    char name[64];
    if (key) key_name(key, name, sizeof name);
    else snprintf(name, sizeof name, "%s", _("none"));
    ImGui::Text("%s: %s", _("Keyboard key"), name);
    ImGui::SameLine();
    if (g_capture == TRIG_KEY && g_capture_dst == &key) ImGui::TextDisabled(_("press a key (Escape = none)"));
    else if (ImGui::Button(_("Capture##key"))) { g_capture = TRIG_KEY; g_capture_dst = &key; }
    ImGui::SameLine();
    Mod::help_marker(_("A key that works as the trigger too, alongside the controller's: for the player who has the keyboard (in "
                       "co-op, the one picked in Coop's Keyboard setting), while the game is in front and this menu is closed. "
                       "Capture, then press the key. Escape clears it."));
    ImGui::Unindent(Mod::lineIndent);
    ImGui::PopID();
}

} // namespace

void GuardianDevil::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) { debug_panel(); return; }   // the Debug tab: test buttons and the readout
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
                  "punching). The trigger is set below. The stand refuses while it is busy (Buster, Snatch, a hold) and while "
                  "Nero is hit, in Buster, Snatch, Showdown, Maximum Bet or a DT activation. It also plays Nero's taunts beside "
                  "him, and a pose when he switches styles (the Nero styles mod). Off, the stand is the game's."));
    if (!mod_enabled) { ImGui::EndGroup(); return; }
    ImGui::Indent(lineIndent);
    ImGui::PushID("GuardianDevil");

    capture_tick();
    input_editor(_("Trigger"),
                 _("What triggers the stand on a controller. Gamepad buttons: raw pad buttons (held together), whatever the "
                   "game maps them to. Game input: one of Nero's mapped inputs, bound in the game's control settings. The "
                   "keyboard key below works too, at the same time. Either way the stick part is the game's own command "
                   "checker: lock-on + forward / back, or lock-on + back, forward for the rush."),
                 trigger_source, trigger_pad, trigger_key, trigger_game, true);
    ImGui::Checkbox(_("Stand actions cost DT"), &dt_cost);
    ImGui::SameLine();
    help_marker(_("Each stand attack takes half a bar of DT from Nero's gauge when it starts, and is refused when the gauge has "
                  "less. The punch rush also drains half a bar a second while it is held past its wind-up, and stops when the "
                  "gauge runs dry. In Devil Trigger the cost and the drain are halved. Guards, the taunt mimic and the style "
                  "switch pose are free."));
    ImGui::Checkbox(_("Cancel when Nero is hit"), &cancel_on_hit);
    ImGui::SameLine();
    help_marker(_("On: a hit, grab or knock-down on Nero ends the stand's attack (it fades out). Off: the stand finishes its attack."));
    ImGui::Checkbox(_("Stand guard"), &guard_enabled);
    ImGui::SameLine();
    help_marker(_("A timed shield on the Guardian action: a hit that lands within 5 frames after you start a stand attack does "
                  "nothing to Nero (no damage, no flinch, no style drop), and the attack turns into a guard: the stand appears "
                  "on Nero in its sword clash, with the game's own stand block spark, sound and pad rumble, and the guard adds "
                  "style and DT as that block does (no DT in Devil Trigger). For 45 frames after a guard, pressing the button "
                  "again guards the next hit instead of attacking (time each press: one that catches nothing ends the re-guards "
                  "until the next guard). The guard does nothing to the enemy. After a guard, no stand attack starts for 15 "
                  "frames. Not guarded: enemy grabs and the few attacks the game handles as pressure."));
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
    ImGui::EndGroup();
}

void GuardianDevil::debug_panel() {
    if (!mod_enabled || !g_file_exists) return;
    ImGui::PushID("GuardianDevil");
    ImGui::TextUnformatted(_("Guardian Devil"));
    ImGui::Indent(lineIndent);
    if (guard_enabled) {
        if (ImGui::Button(_("Play Guard Motion"))) g_test_guard = true;
        ImGui::SameLine();
        help_marker(_("Plays the guard motion on the stand now, as a guard would (one of its stand LMT blocks at random), without a "
                      "hit: no effect, sound, style or DT. The readout below says which motion played and where it came from, or "
                      "why the stand refused."));
    }
    if (ImGui::Button(_("Reload LMTs"))) motion_registry::refresh();
    ImGui::SameLine();
    help_marker(_("Reads every registry LMT from disk again (after re-exporting the files), the stand's too: "
                  "motion\\pl020\\pl020_guardian_ex and motion\\wp024\\wp024_guardian_ex under nativePC, with its taunts, guards "
                  "and style switch poses."));
    if (const uintptr_t P = (uintptr_t)devil4_sdk::get_local_player(); P && is_nero(P) && g_taunt_set >= 0)
        ImGui::TextDisabled("taunt set: %s", motion_registry::describe(g_taunt_set, P).c_str());

    if (const uintptr_t P = (uintptr_t)devil4_sdk::get_local_player()) {
        if (is_nero(P)) {
            const uintptr_t stand = st::stand_of(P);
            const State* s = find_state(P);
            const int slot = slot_for(P);
            ImGui::Text("%s | stand action %d tp %d trace %d | slot %d (%s) | motion %03X | cooldown %.0f | bytes mod %d shown %d | cmd bits %d %d %d %d | starts %u refused %u (%s)",
                        s ? (s->phase == ATTACK && s->switching ? "switch pose" : s->phase == ATTACK && s->taunting ? "taunt"
                             : phase_name(s->phase)) : "idle",
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
                        guard_dt_amount(P), (s && s->drained) ? " (rush ran dry)" : "");
        } else {
            ImGui::TextDisabled(_("(the local player isn't Nero)"));
        }
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

// The keys are the ones the mod has always had (dt_knuckle, knuckle_input), the rest under dt_knuckle_.
void GuardianDevil::on_config_load(const utility::Config& cfg) {
    mod_enabled    = cfg.get<bool>("dt_knuckle").value_or(false);
    trigger_pad    = cfg.get<int>("knuckle_input").value_or(trigger_pad);   // the raw pad buttons, as it always was
    trigger_source = cfg.get<int>("dt_knuckle_trigger_source").value_or(trigger_source);
    trigger_key    = cfg.get<int>("dt_knuckle_trigger_key").value_or(trigger_key);
    trigger_game   = cfg.get<int>("dt_knuckle_trigger_game").value_or(trigger_game);
    if (trigger_source != TRIG_GAME) trigger_source = TRIG_PAD;   // a key-only trigger (old TRIG_KEY) keeps its key, beside the pad's
    rebuild_commands();
    guard_enabled  = cfg.get<bool>("dt_knuckle_guard").value_or(guard_enabled);
    dt_cost        = cfg.get<bool>("dt_knuckle_dt_cost").value_or(dt_cost);
    cancel_on_hit  = cfg.get<bool>("dt_knuckle_cancel_on_hit").value_or(cancel_on_hit);
    check_data_file();
    toggle(mod_enabled);
}

void GuardianDevil::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dt_knuckle", mod_enabled);
    cfg.set<int>("knuckle_input", trigger_pad);
    cfg.set<int>("dt_knuckle_trigger_source", trigger_source);
    cfg.set<int>("dt_knuckle_trigger_key", trigger_key);
    cfg.set<int>("dt_knuckle_trigger_game", trigger_game);
    cfg.set<bool>("dt_knuckle_guard", guard_enabled);
    cfg.set<bool>("dt_knuckle_dt_cost", dt_cost);
    cfg.set<bool>("dt_knuckle_cancel_on_hit", cancel_on_hit);
}
