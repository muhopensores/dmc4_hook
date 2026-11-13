#include "BossDante.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cUnit.hpp"
#include "../sdk/EnemyStep.hpp"
#include "../sdk/Players.hpp"
#include "../sdk/InputCommand.hpp"
#include <cmath>
#include <cstring>

namespace boss_dante {

int mode = MODE_VANILLA;
bool jump_cancels = true;
float jc_chance = 0.4f;
bool take_control = false;

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
int g_registered = 0;
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

// His action has set this frame's buttons: press jump when a step off a player is open and this opening's roll passed.
void __stdcall after_action(uintptr_t boss) {
    if (control_boss(boss)) return;
    if (!enhanced_steps() || !is_boss(boss)) return;
    if (g_step.boss != boss) g_step = StepState{ boss };
    const bool open = save_skill(boss) && at<uint8_t>(boss, kEmJumpEnable) == 1 && (at<uint32_t>(boss, kActStat) & 2) &&
                      at<float>(boss, kEmJumpTimer) <= 0.0f && player_in_reach(boss);
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

bool register_action(int id, ActionFn fn) {
    if (id < kGameActions || id >= kMaxActions || !fn) return false;
    if (g_actions[id] == g_actions[0]) ++g_registered;
    g_actions[id] = (uintptr_t)fn;
    return true;
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
    // the dispatch's own table, copied; ids from 60 on run action 0 (wait) until an action is registered
    std::memcpy(g_actions, (const void*)kActionTable, kGameActions * sizeof(uintptr_t));
    for (int i = kGameActions; i < kMaxActions; ++i) g_actions[i] = g_actions[0];
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
    ImGui::Text(_("Bosses set up: %u | new actions: %d"), g_bosses_seen, g_registered);
    ImGui::Text(_("Jump cancels: %u of %u openings | Dante's Enemy Step skill: %s"), g_jcs, g_rolls,
                save ? (at<uint8_t>(save, 0xDA) == 1 ? _("yes") : _("no")) : "-");
    ImGui::PopID();
}

void BossDante::on_config_load(const utility::Config& cfg) {
    mode = cfg.get<int>("boss_dante_ai").value_or(MODE_VANILLA);
    if (mode != MODE_VANILLA && mode != MODE_ENHANCED) mode = MODE_VANILLA;
    jump_cancels = cfg.get<bool>("boss_dante_jump_cancels").value_or(true);
    jc_chance = cfg.get<float>("boss_dante_jc_chance").value_or(0.4f);
}

void BossDante::on_config_save(utility::Config& cfg) {
    cfg.set<int>("boss_dante_ai", mode);
    cfg.set<bool>("boss_dante_jump_cancels", jump_cancels);
    cfg.set<float>("boss_dante_jc_chance", jc_chance);
}
