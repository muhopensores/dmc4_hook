#include "BossDante.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cUnit.hpp"
#include "../sdk/EnemyStep.hpp"
#include "../sdk/Players.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/BossMoves.hpp"
#include "Macro.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <shellapi.h>

namespace boss_dante {

int mode = MODE_VANILLA;
bool jump_cancels = true;
float jc_chance = 0.4f;
float move_rate = 1.0f;
bool take_control = false;
bool use_clips = true;

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// ---- Game addresses ------------------------------------------------------------------------------------------------
constexpr uintptr_t kCtorAfterDante  = 0x7BF9BF;   // uPlayerDanteBoss ctor sub_7BF9B0: mov dword ptr [esi], 0BE4440h (6 bytes)
constexpr uintptr_t kActionTable     = 0xE15C10;   // the boss actions (60 thiscall functions)
constexpr uintptr_t kDispatchDisp    = 0x7C0B72;   // setInterface 0x7C0B6F: mov edx, [eax*4+0E15C10h], its disp32
constexpr uintptr_t kNativeDanteTbl  = 0xC3FEA0;   // the game's Dante attack table (uPlayerDante::uPlayerDante 0x7B21E5)
constexpr uintptr_t kWaitThenNext    = 0x7C7630;   // the actions' end phase (SE checkActionFrame), esi = the boss
constexpr uintptr_t kAfterAction     = 0x7C0B7A;   // setInterface, right after the action call: movss xmm5, [esi+160D0h] (8 bytes)
constexpr uintptr_t kSave            = 0xE558C8;   // sSave::mpInstance: the enemy-step skill at +0xDA (Dante) / +0x1DA (Nero)
// uPlayer / uPlayerDanteBoss
constexpr uintptr_t kAtckDefTbl      = 0x1DCC;     // uPlayer::kAtckDefTblPtr
constexpr uintptr_t kPos             = 0x30;
constexpr uintptr_t kTargetPos       = 0x30B0;     // mLockOn.targetPos
constexpr uintptr_t kActionRno0      = 0x160B4;
constexpr uintptr_t kThinkUsrParam   = 0x16140;    // float[4]
constexpr uintptr_t kBtn             = 0x161F4;    // mBtn (cleared at the start of every setInterface)
constexpr uintptr_t kAnlgPow         = 0x161FC;    // mAnlg.pow
constexpr uintptr_t kAnlgRadian      = 0x16200;    // mAnlg.radian
constexpr uintptr_t kAnlgFlag        = 0x16210;    // mAnlgFlag
// the enemy step's gates (uPlayer::checkEmJumpBegin 0x804970)
constexpr uintptr_t kPlayerId        = 0x1494;     // 0 Dante, 1 Nero: which save skill is read
constexpr uintptr_t kEmJumpEnable    = 0x2005;
constexpr uintptr_t kActStat         = 0x1554;     // mActStat: & 2 = the action allows a step
constexpr uintptr_t kEmJumpTimer     = 0x1FF4;     // > 0 = no step yet

alignas(4) uintptr_t g_actions[kMaxActions] = {};
uint32_t g_bosses_seen = 0;   // readout

uintptr_t table_for_mode() {
    return mode == MODE_VANILLA ? kNativeDanteTbl : HookDanteKADTbl;
}

// Right after his uPlayerDante constructor (MoveTable has just given him the extended table).
void __stdcall on_boss_created(uintptr_t boss) {
    ++g_bosses_seen;
    if (mode == MODE_VANILLA) at<uintptr_t>(boss, kAtckDefTbl) = kNativeDanteTbl;
}

uintptr_t ret_ctor = kCtorAfterDante + 6;

naked void detour_ctor() {
    __asm {
        mov dword ptr [esi], 0x00BE4440   // kBossVtable
        pushad
        push esi
        call on_boss_created
        popad
        jmp dword ptr [ret_ctor]
    }
}

// ---- Jump cancels off the players ----------------------------------------------------------------------------------
// He already runs the player Dante's enemy-step check (his checkAction thunk 0x7C0A70 -> uPlayerDante::checkAction
// 0x7B72A0 -> checkEmJumpBegin); the game never offers him a target. In Enhanced the human players are his targets
// (sdk/EnemyStep.hpp), and after each action a reflex presses jump when a step is open, rolled once per opening.
struct StepState { uintptr_t boss = 0; bool open = false; bool press = false; };
StepState g_step;
uint32_t g_jcs = 0, g_rolls = 0;   // readout
uint32_t g_rng = 0x2545F491;

float roll() {
    g_rng ^= g_rng << 13; g_rng ^= g_rng >> 17; g_rng ^= g_rng << 5;
    return (g_rng & 0xFFFFFF) / float(0x1000000);
}

bool save_skill(uintptr_t unit) {
    const uintptr_t save = at<uintptr_t>(kSave, 0);
    if (!save) return false;
    switch (at<int32_t>(unit, kPlayerId)) {
    case 0: return at<uint8_t>(save, 0xDA) == 1;
    case 1: return at<uint8_t>(save, 0x1DA) == 1;
    default: return false;
    }
}

uintptr_t player_in_reach(uintptr_t boss) {
    for (int i = 0, n = player_info::count(); i < n; ++i) {
        const uintptr_t pl = player_info::player(i);
        if (pl && !is_boss(pl) && enemy_step::in_reach(boss, pl)) return pl;
    }
    return 0;
}

bool enhanced_steps() { return mode == MODE_ENHANCED && jump_cancels; }

uintptr_t boss_step_target(uintptr_t jumper) {
    if (!enhanced_steps() || !is_boss(jumper)) return 0;
    const uintptr_t pl = player_in_reach(jumper);
    if (pl) ++g_jcs;
    return pl;
}

// ---- Control the boss (test) ---------------------------------------------------------------------------------------
// Player 1's pad drives the boss: in the shared pre-check tick (uPlayer::main 0x7A83B5, after executeCommandCheck and
// before Nero's actions read anything) Nero's interface is saved and cleared, and after the boss's AI action his
// mBtn / mAnlg are replaced with it. setInterface then turns them into his interface as it does the AI's (mBtn ->
// mBtnOn, trg / rel from mBtnOld; mAnlg -> mInterface.mAnlg, the same kAnlgInfo the player's interface has).
// sMediator::mpPlayer stays Nero: the boss's AI and reactions read it as his opponent.
constexpr uintptr_t kIfBtnOn = 0x140C, kIfBtnTrg = 0x1410, kIfBtnRel = 0x1414, kIfAnlg = 0x141C;   // uPlayer::mInterface
constexpr uintptr_t kCmdFlag = 0x2184;                 // mCmdFlag[3]
constexpr uintptr_t kBtnOnJumpFrame = 0x160D0, kBtnOnShortFrame = 0x160D4;   // the AI's held jump / melee
constexpr size_t kAnlgSize = 0x14;                     // uPlayer::cInterface::kAnlgInfo
struct PadCopy { bool valid = false; uint32_t btn = 0; uint8_t anlg[kAnlgSize] = {}; };
PadCopy g_pad;
int g_boss_alive = 0;   // frames since a boss last ran his action (Nero is only held while one does)

void control_tick(uintptr_t player) {
    if (!take_control || g_boss_alive <= 0 || is_boss(player) || player_info::index_of(player) != 0) return;
    --g_boss_alive;
    g_pad.btn = at<uint32_t>(player, kIfBtnOn);
    std::memcpy(g_pad.anlg, (const void*)(player + kIfAnlg), kAnlgSize);
    g_pad.valid = true;
    at<uint32_t>(player, kIfBtnOn) = at<uint32_t>(player, kIfBtnTrg) = at<uint32_t>(player, kIfBtnRel) = 0;
    std::memset((void*)(player + kIfAnlg), 0, kAnlgSize);
    std::memset((void*)(player + kCmdFlag), 0, 12);
}

bool control_boss(uintptr_t boss) {
    if (!take_control || mode != MODE_ENHANCED || !is_boss(boss)) {
        g_boss_alive = 0;
        return false;
    }
    g_boss_alive = 4;
    if (!g_pad.valid) return true;
    at<uint32_t>(boss, kBtn) = g_pad.btn;
    std::memcpy((void*)(boss + kAnlgPow), g_pad.anlg, kAnlgSize);
    at<uint8_t>(boss, kAnlgFlag) = at<uint32_t>(boss, kAnlgPow) != 0;
    at<float>(boss, kBtnOnJumpFrame) = 0.0f;
    at<float>(boss, kBtnOnShortFrame) = 0.0f;
    return true;
}

// ---- Input fixes for the extended table ----------------------------------------------------------------------------
// Helm Breaker (action 48, sub_7C3400) presses lock-on, then melee once Dante's row condition 4 holds (Round Trip not
// out, high enough: sub_7B75A0), and never sets the stick: the interface keeps the last stick an action gave, usually
// forward from running or jumping at the player. The vanilla table has no air row on lock-on + forward + melee, so it
// fell through to Helm Breaker's row (atckId 7, air, no stick condition). With Aerial Stinger, Stinger's row (command 3,
// lock-on + forward + melee) is allowed in the air and comes first: the stale stick turned Helm Breaker into Air
// Stinger. Centring the stick while the action presses leaves only Helm Breaker's row.
constexpr uintptr_t kAction = 0x160AC;          // mAction: the boss action running
constexpr int kActHelmBreaker = 48;

void fix_inputs(uintptr_t boss) {
    if (mode != MODE_ENHANCED) return;
    if (at<int32_t>(boss, kAction) == kActHelmBreaker && act::phase(boss) < 2) {
        std::memset((void*)(boss + kAnlgPow), 0, kAnlgSize);   // pow 0 = PMODE_NONE: the stick centred
        at<uint8_t>(boss, kAnlgFlag) = 1;
    }
}

// ---- Registered moves (sdk/BossMoves.hpp) --------------------------------------------------------------------------
// Each registered move is boss action kFirstAction + its index, all run by run_move. Its state lives in the boss's
// mActionFree[8] (+0x16084), which setBossAction clears for every new action:
//   [0] frames waited (window / confirmation)   [1] the input step   [2] frames into the step
//   [3] 1 once the move is confirmed            [4] frames since the inputs ended
// Phases (mActionRno0): 0 weapons + the first window, 1 the input script, 2 the move runs, 3 the game's end phase.
constexpr uintptr_t kActionFree   = 0x16084;   // float[8]
constexpr uintptr_t kDeltaTime    = 0x10;      // cUnit::m_delta_time (frames)
constexpr uintptr_t kNextSword    = 0x1DB4, kNextGun = 0x1DB8;   // cWeaponCtrl mShortId / mLongId: the next weapons (setWepId)
constexpr uintptr_t kSwordOut     = 0x1DC0, kGunOut = 0x1DC4;    // mEnableShortId / mEnableLongId: the ones out
constexpr uintptr_t kStyle        = 0x156C;    // mStyle.kind
constexpr uintptr_t kMotCncl      = 0x30C0, kMotCnclStat = 0x4, kMotCnclSize = 0x2C;   // uPlayer::mMotCncl[8].mStat
constexpr uintptr_t kLowerAction  = 0x1500;    // mBdyAction[1].no: 12 = attack
constexpr uintptr_t kAtckId       = 0x1564;
constexpr uintptr_t kCollMgr      = 0x1E8C;    // mpCollMgr: +0x1C mLand
constexpr uintptr_t kMediator     = 0xE558B8;  // sMediator::mpInstance: +0x24 mpPlayer
constexpr float kPi = 3.14159265f;

// offered: his think picked an action it may stand in for; fits: and its situation, style, cooldown and check held
// (then it rolled); picked: the roll passed.
struct MoveStats { uint32_t offered = 0, fits = 0, picked = 0, started = 0, finished = 0, failed = 0; float last = -1.0e9f; };
MoveStats g_move_stats[boss_moves::kMaxMoves];
uint32_t g_air_picks[kGameActions] = {};   // what his think picked while airborne (readout)
uint32_t g_air_replaced = 0;

const char* const kActionNames[kGameActions] = {
    "wait", "footwork", "near player", "pose footwork", "pose walk F", "pose walk B", "pose walk L", "pose walk R",
    "step F", "step B", "step L", "step R", "style", "style F", "style B", "style L", "style R", "provoke",
    "provoke walk", "jump", "double jump", "jump land wait", "Stinger jump", "air escape", "cover out", "wait act frame",
    "wait frame", "tutorial next", "short 0", "short 1", "short 2", "long", "long shotgun", "long Pandora 3",
    "Pandora box", "air shot", "air shot F", "air shot B", "air shot L", "air shot R", "long tutorial B",
    "long tutorial C", "High Time", "Stinger", "style attack", "style High Time", "style Stinger", "style and short",
    "Helm Breaker", "descent kick", "interrupted", "blown", "dead", "reverse", "snatched", "bustered", "change style",
    "change style long", "end tutorial", "reset tutorial",
};
float g_clock = 0.0f;   // frames the boss has run (cooldowns)

float* action_free(uintptr_t boss) { return reinterpret_cast<float*>(boss + kActionFree); }

bool grounded(uintptr_t actor) {
    const uintptr_t coll = at<uintptr_t>(actor, kCollMgr);
    return coll && at<uint8_t>(coll, 0x1C) != 0;
}

bool window_open(uintptr_t boss, int window) {
    return window < 0 || at<uint32_t>(boss, kMotCncl + kMotCnclSize * window + kMotCnclStat) == 2;
}

uintptr_t opponent() {
    const uintptr_t med = at<uintptr_t>(kMediator, 0);
    return med ? at<uintptr_t>(med, 0x24) : 0;
}

void apply_stick(uintptr_t boss, boss_moves::Stick s) {
    using namespace boss_moves;
    if (s == STICK_FREE) return;
    if (s == STICK_CENTRE) {
        std::memset((void*)(boss + kAnlgPow), 0, kAnlgSize);
        at<uint8_t>(boss, kAnlgFlag) = 1;
        return;
    }
    float yaw = act::target_yaw(boss);
    if (s == STICK_AWAY) yaw += kPi;
    else if (s == STICK_LEFT) yaw += kPi * 0.5f;
    else if (s == STICK_RIGHT) yaw -= kPi * 0.5f;
    act::stick(boss, yaw);
}

bool confirmed(uintptr_t boss, const boss_moves::Move& m) {
    return m.atck_id >= 0 && at<int32_t>(boss, kLowerAction) == 12 && at<int32_t>(boss, kAtckId) == m.atck_id;
}

void __fastcall run_move(uintptr_t boss, uintptr_t) {
    const int index = boss_moves::index_of_action(at<int32_t>(boss, kAction));
    const boss_moves::Move* m = boss_moves::get(index);
    uint32_t& ph = act::phase(boss);
    if (!m) ph = 3;
    float* f = action_free(boss);
    const float dt = at<float>(boss, kDeltaTime);
    switch (ph) {
    case 0:   // the weapons (setBossAction put Rebellion / Ebony & Ivory back), then the first window
        if (m->sword >= 0) at<int32_t>(boss, kNextSword) = m->sword;
        if (m->gun >= 0) at<int32_t>(boss, kNextGun) = m->gun;
        if ((m->sword < 0 || at<int32_t>(boss, kSwordOut) == m->sword) && (m->gun < 0 || at<int32_t>(boss, kGunOut) == m->gun) &&
            window_open(boss, m->cancel)) {
            ph = 1; f[0] = f[1] = f[2] = 0.0f;
        }
        else if ((f[0] += dt) > m->start_timeout) { ++g_move_stats[index].failed; ph = 3; }
        break;
    case 1: {   // the input script, one step after another
        const int step = (int)f[1];
        if (step >= m->num_inputs) { ph = 2; f[0] = f[4] = 0.0f; break; }
        const boss_moves::Input& in = m->inputs[step];
        if (f[2] == 0.0f && !window_open(boss, in.wait)) {
            if ((f[0] += dt) > m->start_timeout) { ++g_move_stats[index].failed; ph = 3; }
            break;
        }
        act::press(boss, in.buttons);
        apply_stick(boss, in.stick);
        if ((f[2] += 1.0f) >= (float)in.frames) { f[1] += 1.0f; f[2] = 0.0f; f[0] = 0.0f; }
        break;
    }
    case 2:   // the move runs: hand back once it has ended (or never started)
        f[4] += dt;
        if (m->atck_id >= 0) {
            if (confirmed(boss, *m)) {
                if (f[3] == 0.0f) { f[3] = 1.0f; ++g_move_stats[index].started; }
            } else if (f[3] != 0.0f) {
                if (f[4] >= m->min_run) ph = 3;
            } else if ((f[0] += dt) > m->start_timeout) {
                ++g_move_stats[index].failed;
                ph = 3;
            }
        } else if (f[4] >= m->min_run) {
            ph = 3;
        }
        break;
    default:
        act::wait_then_next(boss);
        break;
    }
}

// ---- Clips (Macro files as boss actions) ---------------------------------------------------------------------------
// Each [Dante] section with a BOSS line in a .txt file of kClipFolder is boss action kFirstClip + its index, run by
// run_clip. The BOSS line (any number of them, key=value words) sets when he may play it:
//   replaces=air_shots,helm_breaker   which of his own picks it may stand in for (required): his action names
//                                     (kActionNames, spaces as _) or the groups air_shots, air_attacks,
//                                     ground_attacks, steps
//   where=ground|air|any   dist=300..1200   rise=-200..150   (cm to his target, its height minus his)
//   style=sm|gs|ts|rg|ds   sword=rebellion|gilgamesh|lucifer   gun=ebony|coyote|pandora
//   chance=0.3   cooldown=300   timeout=600   (frames; timeout = how long the clip may run, default its length + waits)
// His state in mActionFree (cleared by setBossAction): [0] frames waited for the weapons, [1] the frame index (u32),
// [2] ticks in the current wait (u32), [3] / [4] a wait's start state (u32), [5] frames the clip has run.
constexpr int kFirstClip = boss_moves::kFirstAction + boss_moves::kMaxMoves;
constexpr int kMaxClips  = kMaxActions - kFirstClip;
constexpr uintptr_t kBtnInfo      = 0x7A96D0;   // uPlayer::getInterfaceBtnInfo: ecx = player, eax = pad buttons, edi = out
constexpr uintptr_t kHitstop      = 0x1201, kHitstopTimer = 0x1E68;
constexpr uintptr_t kFrame        = 0x348, kFrameMax = 0x34C;   // MotionArray[0] mFrame / mFrameMax
constexpr uintptr_t kMgrDamage    = 0x19;       // uCollisionMgr::mDamage: hit in the last collision pass
constexpr int kAnalogWalk = 100;                // |stick| up to this is a walk (Macro's WALK_* = 80, MOVE_* = 127)

struct Clip {
    std::string name, file, where_text;
    std::vector<MacroFrame> frames;
    boss_moves::Move when;   // name, style, weapons and the situation from the BOSS line
    float timeout = 0.0f;
    MoveStats stats;
};
std::vector<std::unique_ptr<Clip>> g_clips;
std::vector<std::string> g_clip_errors;
uint32_t g_clip_files = 0, g_clip_sections = 0;

std::string upper_word(std::string s) {
    for (char& ch : s) ch = (ch == ' ' || ch == '-') ? '_' : (char)std::toupper((unsigned char)ch);
    return s;
}

bool parse_float_text(const std::string& t, float& v) {
    if (t.empty()) return false;
    char* end = nullptr;
    v = std::strtof(t.c_str(), &end);
    return end && *end == '\0' && std::isfinite(v);
}

// "a..b", "..b" or "a.." (either end open)
bool parse_range(const std::string& t, float& lo, float& hi) {
    const size_t dots = t.find("..");
    if (dots == std::string::npos) return false;
    const std::string a = t.substr(0, dots), b = t.substr(dots + 2);
    if (!a.empty() && !parse_float_text(a, lo)) return false;
    if (!b.empty() && !parse_float_text(b, hi)) return false;
    return lo <= hi;
}

bool replaces_bits(const std::string& list, uint64_t& bits, std::string& bad) {
    size_t start = 0;
    while (start <= list.size()) {
        const size_t comma = list.find(',', start);
        const std::string word = upper_word(list.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        start = comma == std::string::npos ? list.size() + 1 : comma + 1;
        if (word.empty()) continue;
        if (word == "AIR_SHOTS") { bits |= boss_moves::AIR_SHOTS; continue; }
        if (word == "AIR_ATTACKS") { bits |= boss_moves::AIR_ATTACKS; continue; }
        if (word == "GROUND_ATTACKS") { bits |= boss_moves::GROUND_ATTACKS; continue; }
        if (word == "STEPS") { bits |= boss_moves::STEPS; continue; }
        bool found = false;
        for (int a = 0; a < kGameActions && !found; ++a) {
            if (upper_word(kActionNames[a]) == word) {
                bits |= boss_moves::bit((boss_moves::Action)a);
                found = true;
            }
        }
        if (!found) { bad = word; return false; }
    }
    return bits != 0;
}

bool parse_boss_line(const std::string& line, Clip& clip, std::string& error) {
    using namespace boss_moves;
    size_t pos = 0;
    while (pos < line.size()) {
        while (pos < line.size() && std::isspace((unsigned char)line[pos])) ++pos;
        if (pos >= line.size()) break;
        size_t end = pos;
        while (end < line.size() && !std::isspace((unsigned char)line[end])) ++end;
        const std::string word = line.substr(pos, end - pos);
        pos = end;
        const size_t eq = word.find('=');
        if (eq == std::string::npos) { error = "'" + word + "' is not key=value"; return false; }
        const std::string key = upper_word(word.substr(0, eq)), value = word.substr(eq + 1), uvalue = upper_word(value);
        Move& m = clip.when;
        bool ok = true;
        if (key == "REPLACES") {
            std::string bad;
            ok = replaces_bits(value, m.stands_in, bad);
            if (!ok) { error = bad.empty() ? "replaces= names nothing" : "replaces=: no action called " + bad; return false; }
        }
        else if (key == "WHERE") {
            if (uvalue == "GROUND") m.where = WHERE_GROUND;
            else if (uvalue == "AIR") m.where = WHERE_AIR;
            else if (uvalue == "ANY") m.where = WHERE_ANY;
            else ok = false;
        }
        else if (key == "DIST") ok = parse_range(value, m.min_dist, m.max_dist);
        else if (key == "RISE") ok = parse_range(value, m.min_rise, m.max_rise);
        else if (key == "CHANCE") ok = parse_float_text(value, m.chance) && m.chance >= 0.0f && m.chance <= 1.0f;
        else if (key == "COOLDOWN") ok = parse_float_text(value, m.cooldown) && m.cooldown >= 0.0f;
        else if (key == "TIMEOUT") ok = parse_float_text(value, clip.timeout) && clip.timeout > 0.0f;
        else if (key == "STYLE") {
            if (uvalue == "SM" || uvalue == "SWORDMASTER") m.style = STYLE_SWORDMASTER;
            else if (uvalue == "GS" || uvalue == "GUNSLINGER") m.style = STYLE_GUNSLINGER;
            else if (uvalue == "TS" || uvalue == "TRICKSTER") m.style = STYLE_TRICKSTER;
            else if (uvalue == "RG" || uvalue == "ROYALGUARD" || uvalue == "ROYAL_GUARD") m.style = STYLE_ROYAL_GUARD;
            else if (uvalue == "DS" || uvalue == "DARKSLAYER") m.style = STYLE_DARKSLAYER;
            else ok = false;
        }
        else if (key == "SWORD") {
            if (uvalue == "REBELLION") m.sword = SWORD_REBELLION;
            else if (uvalue == "GILGAMESH") m.sword = SWORD_GILGAMESH;
            else if (uvalue == "LUCIFER") m.sword = SWORD_LUCIFER;
            else ok = false;
        }
        else if (key == "GUN") {
            if (uvalue == "EBONY" || uvalue == "EBONY_IVORY" || uvalue == "EBONY_AND_IVORY") m.gun = GUN_EBONY_IVORY;
            else if (uvalue == "COYOTE" || uvalue == "SHOTGUN" || uvalue == "COYOTE_A") m.gun = GUN_COYOTE;
            else if (uvalue == "PANDORA") m.gun = GUN_PANDORA;
            else ok = false;
        }
        else { error = "unknown key " + key; return false; }
        if (!ok) { error = "bad value in " + word; return false; }
    }
    return true;
}

std::string clip_summary(const boss_moves::Move& m) {
    char text[160];
    const char* where = m.where == boss_moves::WHERE_AIR ? "air" : m.where == boss_moves::WHERE_GROUND ? "ground" : "any";
    std::snprintf(text, sizeof(text), "%s, %.0f..%.0f cm, chance %.2f, cooldown %.0f", where, m.min_dist,
                  m.max_dist > 99999.0f ? 0.0f : m.max_dist, m.chance, m.cooldown);
    return text;
}

void load_clips() {
    namespace fs = std::filesystem;
    g_clips.clear();
    g_clip_errors.clear();
    g_clip_files = g_clip_sections = 0;
    std::error_code ec;
    if (!fs::is_directory(kClipFolder, ec)) return;
    std::vector<fs::path> files;
    for (const fs::directory_entry& e : fs::directory_iterator(kClipFolder, ec))
        if (e.is_regular_file(ec) && _stricmp(e.path().extension().string().c_str(), ".txt") == 0) files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const fs::path& path : files) {
        ++g_clip_files;
        const std::string file = path.filename().string();
        std::vector<MacroClip> sections;
        std::string error;
        if (!Macro::parse_clip_file(path.string(), sections, error)) {
            g_clip_errors.push_back(file + ": " + error);
            continue;
        }
        for (MacroClip& section : sections) {
            if (section.directives.empty()) continue;   // a plain Macro clip: not the boss's
            ++g_clip_sections;
            const std::string where = file + " [" + section.name + "]";
            if (section.character_role != MACRO_CHARACTER_DANTE) {
                g_clip_errors.push_back(where + ": a boss clip needs a [Dante] section");
                continue;
            }
            if ((int)g_clips.size() >= kMaxClips) {
                g_clip_errors.push_back(where + ": too many clips (the limit is " + std::to_string(kMaxClips) + ")");
                continue;
            }
            auto clip = std::make_unique<Clip>();
            clip->name = section.name;
            clip->file = file;
            clip->when.cooldown = 300.0f;
            clip->when.chance = 0.3f;
            bool ok = true;
            for (const std::string& line : section.directives) {
                if (!parse_boss_line(line, *clip, error)) {
                    g_clip_errors.push_back(where + ": BOSS " + error);
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;
            if (!clip->when.stands_in) {
                g_clip_errors.push_back(where + ": BOSS needs replaces= (which of his moves it may stand in for)");
                continue;
            }
            clip->frames = std::move(section.frames);
            if (clip->timeout <= 0.0f) {
                float t = (float)clip->frames.size() + 60.0f;
                for (const MacroFrame& f : clip->frames) t += (float)f.wait_max_ticks;
                clip->timeout = t;
            }
            clip->when.name = clip->name.c_str();
            clip->where_text = clip_summary(clip->when);
            g_clips.push_back(std::move(clip));
        }
    }
    spdlog::info("BossDante: {} clip(s) from {} file(s) in {}, {} problem(s)", g_clips.size(), g_clip_files, kClipFolder,
                 g_clip_errors.size());
}

// A starter file (example.txt, never overwritten): one clip inside comments, so it loads as nothing until the comment
// marks are removed.
void write_clip_example() {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(kClipFolder, ec);
    const fs::path p = fs::path(kClipFolder) / "example.txt";
    if (fs::exists(p, ec)) return;
    std::ofstream out(p);
    out << "# Boss Dante clip example. A clip is a Macro clip (same lines as Macro's files) in a [Dante] section with a\n"
           "# BOSS line saying when he may play it instead of one of his own moves. Forward on the stick means toward\n"
           "# his target. Remove the # in front of the clip's lines, save, and press Load clips in the Boss Dante panel.\n"
           "#\n"
           "# BOSS keys: replaces=<his moves or groups, comma separated: air_shots, air_attacks, ground_attacks, steps,\n"
           "#   stinger, high_time, helm_breaker, double_jump, air_escape, ...> (required)\n"
           "#   where=ground|air|any  dist=300..1200  rise=-200..150 (cm)  style=sm|gs|ts|rg|ds\n"
           "#   sword=rebellion|gilgamesh|lucifer  gun=ebony|coyote|pandora  chance=0.3  cooldown=300  timeout=600 (frames)\n"
           "#\n"
           "# Stinger, then High Time once the Stinger hits:\n"
           "#[][Dante] Stinger into High Time\n"
           "#BOSS replaces=stinger,long where=ground dist=300..1100 sword=rebellion chance=0.5 cooldown=480\n"
           "#2 LOCK_ON+FORWARD\n"
           "#1 LOCK_ON+FORWARD+MELEE\n"
           "#WAIT_UNTIL HIT 40\n"
           "#2 LOCK_ON+BACKWARD\n"
           "#1 LOCK_ON+BACKWARD+MELEE\n"
           "#20 LOCK_ON\n";
}

uintptr_t fn_btn_info = kBtnInfo;

// The game's button map (the save's Dante bindings): pad buttons -> his interface bits, as a player's pad becomes his.
uint32_t interface_bits(uintptr_t boss, uint32_t pad) {
    uint32_t out = 0;
    uint32_t* p = &out;
    __asm {
        pushad
        mov ecx, boss
        mov eax, pad
        mov edi, p
        call dword ptr [fn_btn_info]
        popad
    }
    return out;
}

void play_frame(uintptr_t boss, const MacroFrame& fr) {
    const uint32_t pad = Macro::frame_pad_buttons(fr, MACRO_CHARACTER_DANTE);
    if (pad) act::press(boss, interface_bits(boss, pad));
    at<float>(boss, kBtnOnJumpFrame) = 0.0f;   // his AI's held jump / melee: the clip holds its own
    at<float>(boss, kBtnOnShortFrame) = 0.0f;
    if (fr.has_left_analog && (fr.left_x || fr.left_y)) {
        const float a = std::atan2((float)fr.left_x, (float)fr.left_y);   // 0 = forward, + = right
        const int mag = (std::max)(std::abs((int)fr.left_x), std::abs((int)fr.left_y));
        act::stick(boss, act::target_yaw(boss) - a, mag <= kAnalogWalk ? act::POW_WALK : act::POW_RUN);
    } else {
        std::memset((void*)(boss + kAnlgPow), 0, kAnlgSize);
        at<uint8_t>(boss, kAnlgFlag) = 1;
    }
}

bool compare_f(float v, uint32_t cmp, float t) {
    switch (cmp) {
    case MACRO_WAIT_COMPARE_EQ: return std::fabs(v - t) <= 0.001f;
    case MACRO_WAIT_COMPARE_NE: return std::fabs(v - t) > 0.001f;
    case MACRO_WAIT_COMPARE_GT: return v > t;
    case MACRO_WAIT_COMPARE_GTE: return v >= t;
    case MACRO_WAIT_COMPARE_LT: return v < t;
    case MACRO_WAIT_COMPARE_LTE: return v <= t;
    default: return false;
    }
}

bool compare_u(uint32_t v, uint32_t cmp, uint32_t t) {
    switch (cmp) {
    case MACRO_WAIT_COMPARE_EQ: return v == t;
    case MACRO_WAIT_COMPARE_NE: return v != t;
    case MACRO_WAIT_COMPARE_GT: return v > t;
    case MACRO_WAIT_COMPARE_GTE: return v >= t;
    case MACRO_WAIT_COMPARE_LT: return v < t;
    case MACRO_WAIT_COMPARE_LTE: return v <= t;
    default: return false;
    }
}

// An enemy step off a player is open for him now (the game's own gates, and a player in reach).
bool step_open(uintptr_t boss) {
    return save_skill(boss) && at<uint8_t>(boss, kEmJumpEnable) == 1 && (at<uint32_t>(boss, kActStat) & 2) &&
           at<float>(boss, kEmJumpTimer) <= 0.0f && player_in_reach(boss);
}

// A WAIT_UNTIL line, checked on him (Macro checks the local player). `u` = his mActionFree as u32.
bool clip_wait_met(uintptr_t boss, const MacroFrame& fr, uint32_t* u) {
    switch (fr.wait_condition) {
    case MACRO_WAIT_ENEMY_STEP: return step_open(boss);
    case MACRO_WAIT_GROUNDED: return grounded(boss);
    case MACRO_WAIT_AIRBORNE: return !grounded(boss);
    case MACRO_WAIT_STYLE: return at<int32_t>(boss, kStyle) == fr.wait_arg;
    case MACRO_WAIT_CAN_EXCEED: return true;   // Nero's; nothing to wait for on Dante
    case MACRO_WAIT_HITSTOP: return at<uint8_t>(boss, kHitstop) != 0 || at<float>(boss, kHitstopTimer) > 0.0f;
    case MACRO_WAIT_HIT_CONFIRMED: {          // his target took a hit
        const uintptr_t target = opponent();
        const uintptr_t coll = target ? at<uintptr_t>(target, kCollMgr) : 0;
        return coll && at<uint8_t>(coll, kMgrDamage) != 0;
    }
    case MACRO_WAIT_LOCKED_ON: return true;   // he always is, on his target
    case MACRO_WAIT_ANIM_FRAME: {
        const float f = at<float>(boss, kFrame);
        return std::isfinite(f) && compare_f(f, fr.wait_compare, fr.wait_value);
    }
    case MACRO_WAIT_MOVEID2: return compare_u(at<uint32_t>(boss, kAtckId), fr.wait_compare, fr.wait_value_u32);
    case MACRO_WAIT_MOVEID2_CHANGED:
        if (!u[3]) { u[3] = 1; u[4] = at<uint32_t>(boss, kAtckId); return false; }
        return at<uint32_t>(boss, kAtckId) != u[4];
    case MACRO_WAIT_FRAME_REACHED_MAX: {
        const float f = at<float>(boss, kFrame), mx = at<float>(boss, kFrameMax);
        return std::isfinite(f) && std::isfinite(mx) && mx > 0.0f && mx < 100000.0f && f >= mx - 0.001f;
    }
    default: return true;
    }
}

void __fastcall run_clip(uintptr_t boss, uintptr_t) {
    const int index = at<int32_t>(boss, kAction) - kFirstClip;
    Clip* clip = (index >= 0 && index < (int)g_clips.size()) ? g_clips[index].get() : nullptr;
    uint32_t& ph = act::phase(boss);
    if (!clip && ph < 3) ph = 3;
    float* f = action_free(boss);
    uint32_t* u = reinterpret_cast<uint32_t*>(f);
    const float dt = at<float>(boss, kDeltaTime);
    switch (ph) {
    case 0: {   // the weapons (setBossAction put Rebellion / Ebony & Ivory back)
        const boss_moves::Move& m = clip->when;
        if (m.sword >= 0) at<int32_t>(boss, kNextSword) = m.sword;
        if (m.gun >= 0) at<int32_t>(boss, kNextGun) = m.gun;
        if ((m.sword < 0 || at<int32_t>(boss, kSwordOut) == m.sword) && (m.gun < 0 || at<int32_t>(boss, kGunOut) == m.gun)) {
            ph = 1;
            ++clip->stats.started;
        }
        else if ((f[0] += dt) > 60.0f) { ++clip->stats.failed; ph = 3; }
        break;
    }
    case 1: {   // one frame of the clip per tick; a WAIT_UNTIL frame holds until it is met or times out, as in Macro
        if ((f[5] += dt) > clip->timeout) { ++clip->stats.failed; ph = 3; break; }
        const uint32_t i = u[1];
        if (i >= clip->frames.size()) { ++clip->stats.finished; ph = 3; break; }
        const MacroFrame& fr = clip->frames[i];
        play_frame(boss, fr);
        bool advance = true;
        if (fr.wait_condition != MACRO_WAIT_NONE && !clip_wait_met(boss, fr, u))
            advance = fr.wait_max_ticks != 0 && ++u[2] >= fr.wait_max_ticks;
        if (advance) { u[1] = i + 1; u[2] = u[3] = u[4] = 0; }
        break;
    }
    default:
        act::wait_then_next(boss);
        break;
    }
}

// setBossAction 0x7C8940 (esi = the boss, edi = the action his think or reaction think picked): a registered move that
// may stand in for it, whose situation holds and whose roll passes, is played instead.
int __stdcall pick_action(uintptr_t boss, int proposed) {
    if (mode != MODE_ENHANCED || take_control || !is_boss(boss) || proposed < 0 || proposed >= kGameActions) return proposed;
    const bool air = !grounded(boss);
    if (air) ++g_air_picks[proposed];
    const float dx = at<float>(boss, kTargetPos) - at<float>(boss, kPos);
    const float dz = at<float>(boss, kTargetPos + 8) - at<float>(boss, kPos + 8);
    const float dist = std::sqrt(dx * dx + dz * dz);
    const float rise = at<float>(boss, kTargetPos + 4) - at<float>(boss, kPos + 4);
    const uintptr_t target = opponent();
    struct Candidate { int action; const boss_moves::Move* move; MoveStats* stats; };
    Candidate fits[boss_moves::kMaxMoves + kMaxClips];
    int n_fits = 0;
    auto consider = [&](int action, const boss_moves::Move& m, MoveStats& st) {
        if (!(m.stands_in & boss_moves::bit((boss_moves::Action)proposed))) return;
        if (m.enabled && !*m.enabled) return;
        ++st.offered;
        if ((m.where == boss_moves::WHERE_AIR && !air) || (m.where == boss_moves::WHERE_GROUND && air)) return;
        if (m.style >= 0 && at<int32_t>(boss, kStyle) != m.style) return;
        if (dist < m.min_dist || dist > m.max_dist || rise < m.min_rise || rise > m.max_rise) return;
        if (g_clock - st.last < m.cooldown) return;
        if (m.check && !m.check(boss, target)) return;
        ++st.fits;
        fits[n_fits++] = Candidate{ action, &m, &st };
    };
    for (int i = 0, n = boss_moves::count(); i < n; ++i)
        consider(boss_moves::kFirstAction + i, *boss_moves::get(i), g_move_stats[i]);
    if (use_clips)
        for (int i = 0; i < (int)g_clips.size(); ++i) consider(kFirstClip + i, g_clips[i]->when, g_clips[i]->stats);
    if (!n_fits) return proposed;
    // in a random order, so the moves registered first don't take every pick they share with later ones
    const int first = (int)(roll() * n_fits) % n_fits;
    for (int k = 0; k < n_fits; ++k) {
        const Candidate& c = fits[(first + k) % n_fits];
        if (roll() >= c.move->chance * move_rate) continue;
        c.stats->last = g_clock;
        ++c.stats->picked;
        if (air) ++g_air_replaced;
        return c.action;
    }
    return proposed;
}

uintptr_t ret_set_action = 0x7C8949;

naked void detour_set_action() {
    __asm {
        pushad
        push edi
        push esi
        call pick_action          // __stdcall: pops both; the action to play in eax
        mov [esp], eax            // the saved edi (pushad: edi at [esp])
        popad
        mov [esi+0x160AC], edi    // the replaced bytes, the cmp last for the jnz that follows
        cmp edi, 0x13
        jmp dword ptr [ret_set_action]
    }
}

// His action has set this frame's buttons: press jump when a step off a player is open and this opening's roll passed.
// A clip presses its own jumps, so the reflex leaves it alone.
void __stdcall after_action(uintptr_t boss) {
    if (is_boss(boss)) g_clock += at<float>(boss, kDeltaTime);
    if (control_boss(boss)) return;
    if (is_boss(boss)) fix_inputs(boss);
    if (!enhanced_steps() || !is_boss(boss) || at<int32_t>(boss, kAction) >= kFirstClip) return;
    if (g_step.boss != boss) g_step = StepState{ boss };
    const bool open = step_open(boss);
    if (open && !g_step.open) {
        ++g_rolls;
        g_step.press = roll() < jc_chance;
    }
    g_step.open = open;
    if (open && g_step.press) {
        at<uint32_t>(boss, kBtn) |= 0x4;   // act::BTN_JUMP
        g_step.press = false;
    }
}

uintptr_t ret_after_action = kAfterAction + 8;

naked void detour_after_action() {
    __asm {
        pushad
        push esi
        call after_action
        popad
        movss xmm5, dword ptr [esi+0x160D0]
        jmp dword ptr [ret_after_action]
    }
}

// A boss already alive takes the table of the mode just picked.
void apply_mode_to_live() {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return;
    const uintptr_t table = table_for_mode();
    if (!table) return;
    for (int line = 0; line < 32; ++line) {
        int n = 0;
        for (cUnit* c = su->mMoveLine[line].mTop; c && n < 4096; c = c->mp_next_unit, ++n) {
            if (is_boss((uintptr_t)c)) at<uintptr_t>((uintptr_t)c, kAtckDefTbl) = table;
        }
    }
}

} // namespace

bool is_boss(uintptr_t unit) {
    return unit && at<uintptr_t>(unit, 0) == kBossVtable;
}

namespace act {

uint32_t& phase(uintptr_t boss) { return at<uint32_t>(boss, kActionRno0); }

float usr_param(uintptr_t boss, int i) { return (i >= 0 && i < 4) ? at<float>(boss, kThinkUsrParam + 4 * i) : 0.0f; }

void press(uintptr_t boss, uint32_t bits) { at<uint32_t>(boss, kBtn) |= bits; }

float target_yaw(uintptr_t boss) {
    const float dx = at<float>(boss, kTargetPos) - at<float>(boss, kPos);
    const float dz = at<float>(boss, kTargetPos + 8) - at<float>(boss, kPos + 8);
    return std::atan2(dx, dz);   // the game's yaw: forward = (sin yaw, cos yaw) on x / z
}

void stick(uintptr_t boss, float yaw, uint32_t pow) {
    at<uint32_t>(boss, kAnlgPow) = pow;
    at<float>(boss, kAnlgRadian) = yaw;
    at<uint8_t>(boss, kAnlgFlag) = 1;
}

uintptr_t fn_wait_then_next = kWaitThenNext;

void wait_then_next(uintptr_t boss) {
    __asm {
        push esi
        mov esi, [boss]
        call dword ptr [fn_wait_then_next]
        pop esi
    }
}

} // namespace act

} // namespace boss_dante

// ---- The mod -------------------------------------------------------------------------------------------------------
using namespace boss_dante;

std::optional<std::string> BossDante::on_initialize() {
    // the dispatch's own table, copied; ids from 60 on run the registered moves (sdk/BossMoves.hpp)
    static_assert(boss_moves::kFirstAction == kGameActions && kGameActions + boss_moves::kMaxMoves < kMaxActions, "action ids");
    std::memcpy(g_actions, (const void*)kActionTable, kGameActions * sizeof(uintptr_t));
    for (int i = kGameActions; i < kFirstClip; ++i) g_actions[i] = (uintptr_t)&run_move;
    for (int i = kFirstClip; i < kMaxActions; ++i) g_actions[i] = (uintptr_t)&run_clip;
    const uintptr_t table = (uintptr_t)g_actions;
    char disp[4];
    std::memcpy(disp, &table, 4);
    install_patch_absolute(kDispatchDisp, patch_dispatch, disp, 4);

    if (!install_hook_absolute(kCtorAfterDante, hook_ctor, &detour_ctor, nullptr, 6)) {
        spdlog::error("BossDante: failed to hook the boss constructor (0x7BF9BF)");
        return "Failed to init BossDante (constructor)";
    }
    if (!install_hook_absolute(kAfterAction, hook_after_action, &detour_after_action, nullptr, 8)) {
        spdlog::error("BossDante: failed to hook setInterface after the action (0x7C0B7A)");
        return "Failed to init BossDante (after action)";
    }
    if (!enemy_step::hook_installed) {   // one installer: Coop uses the same hook
        if (!install_hook_absolute(enemy_step::kSite, hook_em_step, &enemy_step::detour, &enemy_step::jmp_ret, 5)) {
            spdlog::error("BossDante: failed to hook uPlayer::checkEmJumpBegin's target scan (0x804A37)");
            return "Failed to init BossDante (enemy step)";
        }
        enemy_step::hook_installed = true;
    }
    if (!install_hook_absolute(0x7C8940, hook_set_action, &detour_set_action, nullptr, 9)) {
        spdlog::error("BossDante: failed to hook uPlayerDanteBoss::setBossAction (0x7C8940)");
        return "Failed to init BossDante (setBossAction)";
    }
    enemy_step::add_target_source(&boss_step_target);
    input_commands::add_tick(&control_tick);
    if (!input_commands::hook_installed) {   // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("BossDante: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init BossDante (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void BossDante::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("BossDante");
    ImGui::SeparatorText(_("Boss Dante"));
    const char* modes[] = { _("Vanilla"), _("Enhanced") };
    ImGui::SetNextItemWidth(sameLineItemWidth);
    if (ImGui::Combo(_("Boss Dante AI"), &mode, modes, IM_ARRAYSIZE(modes))) apply_mode_to_live();
    ImGui::SameLine();
    help_marker(_("Vanilla: the boss Dante fights with the game's own moves, whatever Dante mods are on (they would "
                  "otherwise reach him too, and some, like Aerial Stinger, break his AI). Enhanced: he keeps the moves "
                  "the Dante mods add, and uses them through his own new actions (being added)."));
    ImGui::BeginDisabled(mode != MODE_ENHANCED);
    ImGui::Checkbox(_("Jump cancels off players"), &jump_cancels);
    ImGui::SameLine();
    help_marker(_("Enhanced only. He can enemy-step off the players as they can off him: when he is close to a player "
                  "during a move that allows a jump cancel, he may press jump. Needs Dante's Enemy Step skill in the "
                  "save, as the game's own check does."));
    ImGui::SetNextItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("New move rate"), &move_rate, 0.0f, 3.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("Enhanced only. Multiplies the chance of every added move (Air Stinger, Air Drive, Judgement Cut, ...) "
                  "each time his AI picks a move it may replace. 1 = each move's own chance, 0 = never, 2 = twice as "
                  "often (a chance can't go above every time)."));
    ImGui::SetNextItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Jump cancel chance"), &jc_chance, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("The chance he takes a jump cancel, rolled once each time one opens (he gets close to a player during "
                  "a move that allows it). 0 = never, 1 = every time."));
    if (ImGui::Checkbox(_("Debug: control boss Dante"), &take_control)) g_pad = PadCopy{};
    ImGui::SameLine();
    help_marker(_("Enhanced only, for testing. Player 1's pad drives the boss Dante in his fight instead of his AI, and "
                  "Nero stands still meanwhile. The camera and the boss's opponent stay Nero. Not saved."));
    ImGui::EndDisabled();
    const uintptr_t save = at<uintptr_t>(kSave, 0);
    ImGui::Text(_("Bosses set up: %u | registered moves: %d"), g_bosses_seen, boss_moves::count());
    if (boss_moves::count() > 0 && ImGui::TreeNode(_("Moves"))) {
        ImGui::SameLine();
        help_marker(_("Per added move. Offered: his AI picked a move this one may replace. Fits: and the distance, height, "
                      "air / ground, style, cooldown and the move's own check allowed it, so its chance was rolled. "
                      "Picked: the roll passed. Started: the game confirmed the move. Failed: it never came out (the "
                      "weapon, the cancel window or the move's confirmation didn't come in time)."));
        for (int i = 0; i < boss_moves::count(); ++i) {
            const boss_moves::Move& m = *boss_moves::get(i);
            const MoveStats& s = g_move_stats[i];
            ImGui::Text(_("%s%s: offered %u, fits %u, picked %u, started %u, failed %u"), m.name,
                        (m.enabled && !*m.enabled) ? _(" (mod off)") : "", s.offered, s.fits, s.picked, s.started, s.failed);
        }
        ImGui::TreePop();
    }
    {   // his most frequent picks in the air
        int top[4] = { -1, -1, -1, -1 };
        uint32_t total = 0;
        for (int a = 0; a < kGameActions; ++a) {
            total += g_air_picks[a];
            if (!g_air_picks[a]) continue;
            for (int k = 0; k < 4; ++k) {
                if (top[k] < 0 || g_air_picks[a] > g_air_picks[top[k]]) {
                    for (int j = 3; j > k; --j) top[j] = top[j - 1];
                    top[k] = a;
                    break;
                }
            }
        }
        char line[256];
        int len = snprintf(line, sizeof(line), _("In the air his AI picked %u moves, %u replaced"), total, g_air_replaced);
        for (int k = 0; k < 4 && top[k] >= 0 && len > 0 && len < (int)sizeof(line); ++k)
            len += snprintf(line + len, sizeof(line) - len, "%s %s %u", k ? "," : ":", kActionNames[top[k]], g_air_picks[top[k]]);
        ImGui::TextUnformatted(line);
        ImGui::SameLine();
        help_marker(_("What his own AI chose while he was in the air (each air shot is a pick of its own), the four most "
                      "frequent, and how many of those picks an added move took over."));
    }
    ImGui::BeginDisabled(mode != MODE_ENHANCED);
    ImGui::Checkbox(_("Clips"), &use_clips);
    ImGui::EndDisabled();
    ImGui::SameLine();
    help_marker(_("Enhanced only. Macro clips as his moves: each [Dante] section with a BOSS line in the clips folder "
                  "(nativePC\\boss_dante, .txt files in Macro's format) may stand in for one of his own moves, as the added "
                  "moves do. The BOSS line says which of his moves it replaces and when (ground / air, distance, style, "
                  "chance, cooldown); the clip's stick forward means toward his target, and WAIT_UNTIL lines are checked on "
                  "him. Create example writes example.txt with every key explained."));
    ImGui::SameLine();
    if (ImGui::Button(_("Load clips"))) load_clips();
    ImGui::SameLine();
    if (ImGui::Button(_("Open clips folder"))) {
        std::error_code ec;
        std::filesystem::create_directories(kClipFolder, ec);
        ShellExecuteA(nullptr, "open", std::filesystem::absolute(kClipFolder, ec).string().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    if (ImGui::Button(_("Create example"))) write_clip_example();
    ImGui::SameLine();
    help_marker(_("Load clips reads the clips folder again (also done when the game starts). Open clips folder opens it in "
                  "Explorer (made if missing). Create example writes example.txt there: one clip inside comments, so it "
                  "loads as nothing until you remove the # marks."));
    ImGui::Text(_("Clips: %d loaded from %u files (%u boss sections), %d problems"), (int)g_clips.size(), g_clip_files,
                g_clip_sections, (int)g_clip_errors.size());
    for (const std::string& e : g_clip_errors) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.4f, 1.0f), "  %s", e.c_str());
    if (!g_clips.empty() && ImGui::TreeNode(_("Clip list"))) {
        ImGui::SameLine();
        help_marker(_("Per clip, with its file and conditions. Offered / fits / picked as for the added moves. Started: it "
                      "began its inputs. Finished: it played to its last line. Failed: its weapon never came out or it ran "
                      "past its timeout. A clip a hit interrupts is neither finished nor failed."));
        for (const auto& clip : g_clips) {
            const MoveStats& st = clip->stats;
            ImGui::Text(_("%s (%s; %s): offered %u, fits %u, picked %u, started %u, finished %u, failed %u"), clip->name.c_str(),
                        clip->file.c_str(), clip->where_text.c_str(), st.offered, st.fits, st.picked, st.started, st.finished,
                        st.failed);
        }
        ImGui::TreePop();
    }
    if (ImGui::Button(_("Reset boss move counters"))) {
        for (MoveStats& m : g_move_stats) m = MoveStats{};
        for (auto& clip : g_clips) clip->stats = MoveStats{};
        std::memset(g_air_picks, 0, sizeof(g_air_picks));
        g_air_replaced = 0;
    }
    ImGui::SameLine();
    help_marker(_("Sets the move counters above back to zero, to watch one fight or one phase."));
    ImGui::Text(_("Jump cancels: %u of %u openings | Dante's Enemy Step skill: %s"), g_jcs, g_rolls,
                save ? (at<uint8_t>(save, 0xDA) == 1 ? _("yes") : _("no")) : "-");
    ImGui::PopID();
}

void BossDante::on_config_load(const utility::Config& cfg) {
    mode = cfg.get<int>("boss_dante_ai").value_or(MODE_VANILLA);
    if (mode != MODE_VANILLA && mode != MODE_ENHANCED) mode = MODE_VANILLA;
    jump_cancels = cfg.get<bool>("boss_dante_jump_cancels").value_or(true);
    jc_chance = cfg.get<float>("boss_dante_jc_chance").value_or(0.4f);
    move_rate = cfg.get<float>("boss_dante_move_rate").value_or(1.0f);
    use_clips = cfg.get<bool>("boss_dante_clips").value_or(true);
    load_clips();
}

void BossDante::on_config_save(utility::Config& cfg) {
    cfg.set<int>("boss_dante_ai", mode);
    cfg.set<bool>("boss_dante_jump_cancels", jump_cancels);
    cfg.set<float>("boss_dante_jc_chance", jc_chance);
    cfg.set<float>("boss_dante_move_rate", move_rate);
    cfg.set<bool>("boss_dante_clips", use_clips);
}
