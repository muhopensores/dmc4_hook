#pragma once

#include "../mod.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun
#include "NeroStyle.hpp"   // nero_style::kSlots / current(): the action set per style

// Guardian Devil: Nero's DT stand (uPlNeroDevil, sdk/NeroStand.hpp) attacks on a button, outside DT, with Red Queen
// motions. The stand is driven through the game's own DT mimic path: the start does what uPlayerNero::setRedQueenMotion
// does in DT (setAction(4) + mYamatoParam = motion / attr / start frame), places the stand on Nero, detaches it
// (mIsTraceParent = 0) and fades it in (mTpPhase = 20); uPlNeroDevil::moveActYamato then plays the motion, ends it at
// the motion's end (setAction(0)) and updateTransparency fades it out; the stand follows Nero again once hidden.
//
// Two bytes in the stand's free padding carry the mod's state to the game's code (nothing else writes them, no game
// instruction reads them): kMod (+0x22D9) = MOD_ATTACK while a guardian attack runs (| MOD_HOLD while the stand is held in
// place during a Buster hold), and kShown (+0x22D3) = mIsDevilTrigger || the mod shows the stand. Ten one-byte patches
// point the stand's DT reads (main, moveEx, sync, updateMaterial, draw, updateMatrix, updateTransparency's hand alpha and
// aura, moveActYamato's gate) at kShown, so the stand updates and draws for a guardian attack as it does in DT, and
// stays hidden otherwise (Buster / Snatch without DT keep the vanilla look). Six hooks test kMod and do nothing while it
// is 0, so vanilla DT behaves as without the mod: the mimic's cancel gate is forced only during a guardian attack (in DT
// the game's own rules cancel the stand when Nero is hit or starts a move it doesn't mimic), the mimic's wait for Nero's
// per-motion event is skipped only then, the detached stand's follow code in moveEx (push-back, ground height, yaw) is
// skipped only then, Nero's DT moves don't overwrite a running guardian attack, and Kabuto's motion stop is skipped
// only then. Nothing here acts outside a guardian attack: no always-on drawing, height or push-back change, and no
// change to other models.
//
// Input: the game's own command checker. The trigger is a gamepad button (raw pad bits, the game's own mapping not
// involved), a keyboard key, or one of Nero's mapped game inputs (Change Gun 0x100, Change Devil Arm 0x80, Taunt 0x8,
// bound in the game's control settings). A pad / key trigger is injected into mBtnOn / mBtnTrg as the virtual button
// 0x800 (produced by no binding, read by no game code) in the pre-check tick, before the commands run. Four commands
// are registered with input_commands (bits 85-95), mirroring the game's own heads with the trigger in place of melee:
// lock-on + back, forward + trigger (Calibur's 0x52 windows: 15 frames each) = the rush; lock-on + forward + trigger =
// forward; lock-on + back + trigger = back; the trigger alone = neutral. The post-command tick reads the four bits
// (rush > forward > back > neutral) and starts the stand.
//
// Per style: the action set (the four motions, the rush's start frame and loop window) is one per NeroStyle slot,
// picked with nero_style::current(player) (slot 0 when Nero styles are off). Starts are refused while the stand is busy
// (Buster, Snatch, a hold, Illusion Sword), while a DT change is pending, and while Nero is hit / down / grabbed / dying,
// in a DT activation, or in Buster / Snatch / Showdown / Maximum Bet; a refused start changes nothing.
//
// Options from the issues: the stand can stay where it is while it holds an enemy (Buster hold) instead of following
// Nero (hold_stays_put: its position is pinned where the hold began, since the hold motions carry root motion), and the instant DT activation (Nero's DT cancel) can run during that hold without summoning the
// stand (dt_cancel_keeps_hold: uPlayerNero::checkActionDevilTrgAtOnce refuses it while the stand holds, and
// moveActDevilTrg / moveActDevilTrgAir call the stand's Appear action; both are gated).
//
// Needs the downloaded nativePC\Collision\wp024.col (the stand's attack data; the mod turns itself off without it).
// The stand hits with that file's attack entries, so the style-points repetition penalty and which enemies a hit
// reaches are that data's, not the mod's.

namespace guardian_devil {

struct ActionSet {
    bool  enabled;        // the stand attacks in this style
    int   neutral;        // the button alone (0x31C)
    int   forward;        // lock-on + forward + button (0x317, Streak)
    int   back;           // lock-on + back + button (0x330)
    int   rush;           // lock-on + back, forward + button (0x35F, the punch rush)
    float rush_start;     // the rush motion's start frame (its wind-up skipped: 135)
    float loop_from;      // while the button is held, a rush frame in [loop_from, loop_to] jumps back to loop_dest
    float loop_to;
    float loop_dest;
};

enum TriggerSource : int { TRIG_PAD = 0 /* gamepad buttons */, TRIG_KEY = 1 /* a keyboard key */, TRIG_GAME = 2 /* a mapped game input */ };
constexpr uint32_t kVirtualButton = 0x800;   // the pad / key trigger as a game button bit, for the command checker

extern bool  mod_enabled;
extern int   trigger_source;       // TriggerSource
extern int   trigger_pad;          // raw pad bits (mPeripheral.mPadBtnOn), all held to count as the trigger
extern int   trigger_key;          // virtual-key code (0 = none)
extern int   trigger_game;         // the mapped game input (mBtnTrg bit): 0x100 Change Gun, 0x80 Change Devil Arm, 0x8 Taunt
extern float cooldown_frames;      // frames between starts (30)
extern bool  cancel_on_hit;        // a hit / grab on Nero ends a running guardian attack (off: the stand finishes it)
extern bool  rush_effect;          // the rush's effects (effect\efl\vie\vi020_00v0 while it runs, v1 at its end)
extern float height_offset;        // cm added to the stand's height at the start (0)
extern bool  hold_stays_put;       // the stand stays in place while it holds an enemy (Buster hold) instead of following Nero
extern bool  dt_cancel_keeps_hold; // the instant DT activation runs during that hold, without summoning the stand
extern ActionSet sets[nero_style::kSlots];

int  slot_for(uintptr_t player);   // the NeroStyle slot whose set applies (0 when styles are off or the player isn't Nero)
bool active(uintptr_t player);     // a guardian attack runs for this Nero
void pre_tick(uintptr_t player);   // input_commands::add_tick: end detection, the hold, the stand bytes, the rush loop
void post_command_tick(uintptr_t player);   // input_commands::add_post_command_tick: the four command bits -> a start
uint32_t trigger_bit();            // the button bit the four commands test (trigger_game, or the virtual button)
void rebuild_commands();           // after a trigger change: rewrites the four commands' button step

} // namespace guardian_devil

class GuardianDevil : public Mod {
public:
    GuardianDevil() = default;
    std::string get_mod_name() override { return "GuardianDevil"; };
    std::vector<std::string> get_search_terms() override { return { "Guardian Devil" }; }

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    void toggle(bool enable);   // the ten kShown patches

    std::unique_ptr<FunctionHook> hook_cmd, hook_gate, hook_phase0, hook_follow, hook_position, hook_dt_request, hook_kabuto,
                                  hook_dt_at_once, hook_dt_appear, hook_dt_appear_air;
    std::unique_ptr<Patch> patch_shown[10];
};
