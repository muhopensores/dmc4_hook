#pragma once

#include "../mod.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun
#include "NeroStyle.hpp"   // nero_style::kSlots / switches(): the style switch poses

// Guardian Devil: Nero's DT stand (uPlNeroDevil, sdk/NeroStand.hpp) attacks on a button, outside DT, with Red Queen
// motions. The stand is driven through the game's own DT mimic path: the start does what uPlayerNero::setRedQueenMotion
// does in DT (setAction(4) + mYamatoParam = motion / attr / start frame), places the stand on Nero, detaches it
// (mIsTraceParent = 0) and fades it in (mTpPhase = 20); uPlNeroDevil::moveActYamato then plays the motion, ends it at
// the motion's end (setAction(0)) and updateTransparency fades it out; the stand follows Nero again once hidden.
//
// Two bytes in the stand's free padding carry the mod's state to the game's code (nothing else writes them, no game
// instruction reads them): kMod (+0x22D9) = MOD_ATTACK while a guardian attack runs (| MOD_PLACED until it has faded out,
// | MOD_GUARD while it is a guard), and kShown (+0x22D3) = mIsDevilTrigger || the mod shows the stand. Ten one-byte patches
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
// The actions (the same in every Nero style): neutral 0x31C, forward 0x317 (Streak), back 0x330, the rush 0x35F from
// frame 135 (its wind-up skipped), looping 170-172 -> 150 while the trigger is held, with the effect
// effect\efl\vie\vi020_00v0 while it punches and v1 when it stops; 30 frames between starts. A motion can instead be a
// block of the stand LMT pair the taunt mimic registers (motion\pl020\pl020_guardian_ex /
// motion\wp024\wp024_guardian_ex: one shared file for every custom stand animation, so they cost one set of registry
// blocks), played by its registry word (motion_registry::word); of the actions only the guard does (blocks 0-2). While the set
// isn't on the stand yet, the vanilla motion plays instead. Starts are refused while the stand is busy (Buster, Snatch, a
// hold, Illusion Sword), while a DT change is pending, and while Nero is hit / down / grabbed / dying, in a DT
// activation, or in Buster / Snatch / Showdown / Maximum Bet; a refused start changes nothing. A hit on Nero doesn't end
// a running attack: the stand finishes it.
//
// Taunt mimic: when Nero starts a taunt (lower action 0x17, motions 0x96-0x9E / 0xA5 / 0xA6 of his bank
// 0), the stand plays the same taunt beside him. The stand's own motion lists (pl020_01..04) have no taunts and its
// joints are numbered differently from Nero's (its left arm is 7-10 where Nero's is 9-12; its right arm is the separate
// model wp024, whose own blocks place the whole arm on the shoulder), so Nero's taunts are retargeted onto both rigs
// with Albam (scripts/stand_taunt_retarget.py there) into the two stand LMTs with Nero's block indices, loaded by the
// motion registry as a STAND set (loose files under nativePC); the stand plays the registry's word for Nero's motion id,
// 80 cm behind Nero. It stops when Nero's taunt ends.
//
// Style switch pose: when NeroStyle switches Nero into a style (nero_style::switches: a real switch, not a new player
// object getting its style written back), the stand appears on him with Nero's DT-on flash (com\ec024_60v0) and the Devil
// Bringer's appear sound (pl000_style00 41, BringerAppear) and plays that style's pose block of the stand LMT (10 for the
// Devil Bringer, 12 Gerbera, 11 Gunslinger; none for the third slot), holds its last frame for 10 frames (the motions
// stopped with A_STOP), then fades out as after an attack. It needs no trigger, costs nothing, opens no guard window and
// sets no cooldown (a stand attack may cut it short). It is refused like an attack (Nero hit / in Buster / Snatch / a DT
// activation, the stand busy), and while a stand attack or guard runs; a block the stand LMT lacks plays nothing (no
// vanilla fallback).
//
// Guard (guard_enabled): a timed stand shield on the Guardian action itself, Nero's aerial defence that works in any
// situation. Every Guardian action start (neutral, forward, back, rush) opens a guard window: from the press a counter
// adds the frame's delta time, as Dante's Just Block counter does (uPlayerDante::checkStyle / movePlayerBefore, checked
// <= 5 by checkJustBlock); while it is at most 5 frames, a hit on Nero is cancelled whole:
// uPlayerNero::procBeforeDamageCalc (vtable slot 83, called by uPlayer::damageMessage before any damage) returns 1, which
// skips the damage, the DT gain from being hit, the hit reaction, blood and the style rank drop. The first guarded hit
// of a window cancels the attack into the guard: the next tick the stand is placed back on Nero with the guard motion (a
// random one of stand LMT blocks 0-2, else 0x302 from frame 0: the sword clash, which uPlayerNero::moveActFlip hands the
// stand in DT when Nero's sword is knocked back), with the vanilla stand block's effect and sound (procBeforeDamageCalc's
// own: stay effect list 0 / 0x28 and SE 69 of the player's second bank) and the clash's pad rumble (reverseMessage's 1);
// its setAction turns the hand's attack collision off, so the guard does nothing to the attacker (no damage or
// push-back). It adds style as the vanilla block does (uPlayer::addComboGauge, ADD_COMBO_GUAGE_ENEMY_GUARD 6, with Nero's
// own block values from his player params) and the block's DT bonus (player params +0x48, added directly; none in Devil
// Trigger). The clash marks every frame with the stand's "disappear outside DT" event bit (11; bit 10, the DT one, from
// frame 20), so uPlNeroDevil::checkDevilDisappear would fade a guard out on its first frame: during a guard (kMod
// MOD_GUARD) it takes its DT branch, and the guard fades from frame 20 as the clash does in DT. Later hits inside the same
// window are cancelled too. After a guard, a Guardian action press guards again instead of attacking: a new window; a
// re-guard window that catches nothing ends the re-guards until the next guard, so each one has to be timed. Re-guards
// last 45 frames from each guard, whether its motion still plays or not (a caught re-guard brings the stand back in the
// guard pose and starts them again). After the guard, 15 frames pass before a stand attack.
// Attacks that skip slot 83 (checkPressureDamage ones, enemy grabs through grabHitMessage) aren't guarded.
//
// DT cost (dt_cost, off by default): each attack's start takes 500 (half a bar) from Nero's DT gauge
// (uPlayer::mDevilTrg.now +0x1F24, 1000 = one bar) and is refused when the gauge has less; the rush also drains 500 a
// second while it is held past its wind-up, and stops looping when the gauge runs dry (as if the trigger were let go).
// In Devil Trigger (the stand's mIsDevilTrigger) the cost and the drain are halved. Guards, the taunt mimic and the
// switch pose are free.
//
// Needs the downloaded nativePC\Collision\wp024.col (the stand's attack data; the mod turns itself off without it).
// The stand hits with that file's attack entries, so the style-points repetition penalty and which enemies a hit
// reaches are that data's, not the mod's.

namespace guardian_devil {

enum TriggerSource : int { TRIG_PAD = 0 /* gamepad buttons */, TRIG_KEY = 1 /* a keyboard key */, TRIG_GAME = 2 /* a mapped game input */ };
constexpr uint32_t kVirtualButton = 0x800;   // the pad / key trigger as a game button bit, for the command checker

extern bool  mod_enabled;
extern int   trigger_source;       // the controller's trigger: TRIG_PAD or TRIG_GAME (TRIG_KEY only in old configs)
extern int   trigger_pad;          // raw pad bits (mPeripheral.mPadBtnOn), all held to count as the trigger
extern int   trigger_key;          // virtual-key code (0 = none), a trigger too alongside the controller's, for the player
                                   // who has the keyboard (players::has_keyboard)
extern int   trigger_game;         // the mapped game input (mBtnTrg bit): 0x100 Change Gun, 0x80 Change Devil Arm, 0x8 Taunt
extern bool  guard_enabled;        // a Guardian action's press opens a guard window
extern bool  dt_cost;              // the stand's attacks cost DT
extern bool  cancel_on_hit;        // a hit / grab on Nero ends a running guardian attack (off: the stand finishes it)

int  slot_for(uintptr_t player);   // this Nero's NeroStyle slot (0 when styles are off or the player isn't Nero)
bool active(uintptr_t player);     // a guardian attack runs for this Nero
bool guarding(uintptr_t player);   // a guard's window is open for this Nero (a hit now is cancelled)
void pre_tick(uintptr_t player);   // input_commands::add_tick: end detection, the guard, the poses, the stand bytes, the rush loop
void post_command_tick(uintptr_t player);   // input_commands::add_post_command_tick: the four command bits -> a start
uint32_t trigger_bit();            // the button bit the four commands test (trigger_game, or the virtual button)
void register_taunt_set();         // registers the stand LMTs with the motion registry
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
    void on_gui_frame(int display) override;   // the settings in Nero's tab, test buttons and the readout in the Debug tab

    void toggle(bool enable);   // the ten kShown patches

    std::unique_ptr<FunctionHook> hook_cmd, hook_gate, hook_phase0, hook_follow, hook_dt_request, hook_kabuto, hook_disappear;
    std::unique_ptr<Patch> patch_shown[10];
    std::unique_ptr<Patch> patch_guard[3];   // uPlayerNero's (and its Benchmark / Tutorial copies') vtable slot 83

private:
    void debug_panel();
};
