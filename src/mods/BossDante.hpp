#pragma once
#include "../mod.hpp"

// BossDante: the boss Dante (uPlayerDanteBoss) and dmc4_hook's Dante additions. Plan: Vibed\AIPlayer\README.md.
//
// The boss is a Dante: his constructor sub_7BF9B0 (vtable 0xBE4440) calls uPlayerDante::uPlayerDante (0x7BF9BA), where
// MoveTable gives every Dante dmc4_hook's extended attack table (+0x1DCC), so the added moves and the edits to vanilla
// rows (Stinger in the air, ...) reach him. His AI presses buttons: each frame setInterface sub_7C0A80 runs his current
// boss action (a 60-entry table at 0xE15C10, dispatched at 0x7C0B6F) and the action sets mBtn / mAnlg, which become the
// player interface the command checks read.
//
// - Vanilla AI: right after his uPlayerDante constructor (0x7BF9BF) he gets the game's own table (0xC3FEA0) back, so
//   none of the added moves or row edits can start for him. A mode change also applies to a boss already alive.
// - Enhanced AI: he keeps the extended table (the added moves become his on purpose through new actions).
// - Actions beyond the game's 60: the dispatch reads a dmc4_hook copy of the table (the instruction's disp32 at
//   0x7C0B72 repointed), whose entries from 60 on run the moves Dante mods register (sdk/BossMoves.hpp); his think's
//   picks go through uPlayerDanteBoss::setBossAction 0x7C8940, where a registered move may stand in for his own.
namespace boss_dante {

enum Mode : int { MODE_VANILLA = 0, MODE_ENHANCED = 1 };
extern int mode;
extern bool jump_cancels;   // Enhanced: he enemy-steps off the players (sdk/EnemyStep.hpp)
extern float jc_chance;     // rolled once per opening
extern bool take_control;   // test: player 1's pad drives the boss, Nero holds still

constexpr uintptr_t kBossVtable = 0xBE4440;

// The unit is the boss Dante (not the player's Dante, nor the benchmark Dante, vtable 0xBE3FE8).
bool is_boss(uintptr_t unit);

constexpr int kGameActions = 60;   // DANTE_BOSS ACTION_WAIT .. ACTION_RESET_TUTO_SUCC
constexpr int kMaxActions  = 128;

// What an action works with, as the game's own actions do.
namespace act {
// Interface bits (uPlayer::getInterfaceBtnInfo 0x7A96D0, Dante)
constexpr uint32_t BTN_MELEE = 0x1, BTN_GUN = 0x2, BTN_JUMP = 0x4, BTN_TAUNT = 0x8, BTN_LOCK_ON = 0x10,
                   BTN_STYLE = 0x200, BTN_DT = 0x400;
constexpr uint32_t POW_RUN = 2;                    // cInterface::POW_MODE PMODE_RUN

uint32_t& phase(uintptr_t boss);                   // mActionRno0: 0 when the action starts
float usr_param(uintptr_t boss, int i);            // mThinkUsrParam[i] (the picking row's fusr_param)
void press(uintptr_t boss, uint32_t bits);         // held this frame (the interface takes press / release from frame to frame)
float target_yaw(uintptr_t boss);                  // the yaw toward his lock-on target
void stick(uintptr_t boss, float yaw, uint32_t pow = POW_RUN);   // the stick this frame, as a world yaw
void wait_then_next(uintptr_t boss);               // the end phase: count down mNextActionFrame, then pick the next action
}

} // namespace boss_dante

class BossDante : public Mod {
public:
    BossDante() = default;
    std::string get_mod_name() override { return "BossDante"; };
    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    std::unique_ptr<FunctionHook> hook_ctor, hook_after_action, hook_em_step, hook_cmd, hook_set_action;
    std::unique_ptr<Patch> patch_dispatch;
};
