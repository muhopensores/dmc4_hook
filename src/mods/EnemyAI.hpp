#pragma once
#include "../mod.hpp"

// EnemyAI: enemy behaviour built on the game's own AI (cThinkMgr, Vibed\RE\think_mgr.md), in the Environment tab's
// Enemy AI page. Two parts, each with its own switch: Provoke and Anti-air.
//
// Provoke: a taunt provokes the enemies near the player who taunted (or, locked on, his target alone). A provoked enemy
// runs faster, reaches its Devil Trigger sooner, and once in DT glows at its edges and picks its next attack sooner.
// Plan and the game facts: Vibed\Provoke\README.md; the AI side: Vibed\RE\think_mgr.md.
//
// - The taunt: uPlayer::checkProvoke 0x803E40 calls uPlayer::setProvoke 0x7AE9F0 (esi = the player: action 0x17 for
//   Dante and Nero, +0x1F38 = the taunt counts for style). Hooked after that call (0x803E67).
// - Speed: sUnit::move writes each unit's frame delta (+0x10) at 0x8DC314; a provoked enemy's is multiplied there, so its
//   motion, movement, DT timer and think cooldowns all run at that rate. The provoke's own time runs on the unscaled delta.
// - Devil Trigger (uEnemy, uEnemy::move 0x7308B0): +0x1488 timer, +0x148C OK, +0x148D timer enabled, +0x148E in DT,
//   +0x148F sign shown; the game counts it on Dante Must Die (sMediator +0x140 = 3) or in BP above floor 80 (mission
//   +0xE0 = 50, sArea +0x3830 -> +0x74 > 0x50), outside secret missions (+0x126), and the enemy's own checkAction calls
//   setDevilTrigger once OK is set. A provoked enemy's timer loses `dt_rate - 1` times more; with dt_any_difficulty the
//   mod counts it elsewhere too (sign at 900 or less through vtable +0x13C, OK at 0).
// - Think cooldowns: cThinkMgr::NextThinkTimer 0x45BEA0 (ecx = the manager, xmm2 = the rate; each enemy's preMain with
//   its delta). The manager lies inside its enemy (base <= mgr < base + DTI size); in DT its rate is multiplied.
// - Aura: a copy of the enemy's model (a CustomUnit, the game's own drawing) posed from its joints at every draw, its
//   materials turned into rim_glow's edge glow in aura_color, each joint scaled by aura_scale about its own origin so
//   the copy wraps the enemy instead of z-fighting with it.
namespace provoke {

extern bool  mod_enabled;
extern float radius;              // cm around the player who taunted
extern float duration;            // seconds; a new taunt in range starts it again
extern float speed;               // the provoked enemy's speed
extern float dt_rate;             // its DT timer runs this much faster again (on top of the speed)
extern bool  dt_any_difficulty;   // count provoked enemies' DT timers on every difficulty
extern float think_rate;          // in DT, its think cooldowns run this much faster again
extern bool  counted_only;        // only taunts that count for style provoke
extern bool  targeted;            // locked on to an enemy: the taunt provokes it, at any distance
extern bool  targeted_and_radius; // and the radius too
extern bool  aura;
extern float aura_color[3];
extern float aura_brightness;
extern float aura_reach;
extern float aura_scale;          // the copy's size: each joint's matrix scaled about its own origin

// The player who provoked `enemy`, 0 when it isn't provoked (Coop moves its aggro to him).
uintptr_t provoker(uintptr_t enemy);

} // namespace provoke

// Anti-air: enemies use their own anti-air moves more readily while the target is above them. An enemy's anti-air moves
// are the think-table rows that only pass with the target above it (heightA of 100 cm or more; mfHeight = target y - its
// y, uEnemy::calcTargetPos). While the target is in such a row's widened window, cThinkMgr::thinkCheck 0x45C5E0 (stdcall:
// the manager, the 0x68-byte row by value; retn 6Ch) gets the row's copy adjusted on the stack:
// - window: heightA x window_factor, so they react lower in a jump;
// - roll: inside a random block (row flag 0x400000 starts one; mbRndStart +0x60, mfRndProb +0x64, mfRndSubProb +0x68,
//   the row's share fsys_param[4] +0x50) the roll is set to land on this row;
// - cooldown: flag bit 61 (high dword 0x20000000) skips its cooldown / pick-count gates.
// How many enemies may attack at once stays EnemySlotting's.
//
// Think trace (Debug tab, EnemyAIThinkTrace.cpp): the same thinkCheck hook and one on cThinkMgr::GetActionNo 0x45C240
// record the decisions of the enemy player 1 is locked on to.
namespace anti_air {

extern bool  enabled;
extern float window_factor;   // the rows' lower height bound times this (1 = the game's)
extern bool  win_roll;
extern bool  skip_cooldown;

} // namespace anti_air

class EnemyAI : public Mod {
public:
    EnemyAI() = default;
    std::string get_mod_name() override { return "EnemyAI"; };
    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    std::unique_ptr<FunctionHook> hook_taunt, hook_delta, hook_think, hook_think_check, hook_get_action_no;
};
