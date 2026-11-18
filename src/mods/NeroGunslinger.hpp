#pragma once

#include "../mod.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun

// Gunslinger: the moves of Nero's Gunslinger style (Vibed\NeroStyles\README.md "Decided": full-body gun moves, C++ like
// Gerb / FreeRide, not MoveDef). So far the ground dodge shots, one row for the four directions (as Gerb's directional row),
// the neutral ground shot and the air shots:
//   0x43 (move_id)      lock-on + a stick direction + grab (the Snatch / Gerb button), on the ground: Nero dodges back, left,
//                       forward or right (the stick against the lock-on target, as the game's lever check) and fires Blue
//                       Rose on that direction's shot frame.
//   0x47 (ground_move_id) grab on the ground without the dodge's lock-on + direction: Nero fans the hammer, a full shot
//                       (flash, recoil, sound) on frames 0 / 6 / 12, standing (the clip's root motion as the dodges').
//                       Its row shares the air shot's command (the grab button alone; the rows' ground / air conditions
//                       pick one), and a dodge press clears that bit on the ground so the dodge wins. With the
//                       button still held once the volley has fired (from frame 14), Nero spins Blue Rose
//                       (block 8, looped from its start) and charges it, far faster than the vanilla charge
//                       (levels at 10 / 25 / 45 frames of spinning, the levels Nero has learned, with the vanilla charge
//                       glow and level sounds); letting go fires the
//                       vanilla charge shot of the level reached (uPlayer::setActionAtckGun with atckId 44 + level, as
//                       the gun rows start it; uPlayerNero::moveAtckBlueRoseChargeShot reads the level from the id),
//                       or before level 1 stands him up.
//   0x46 (air_move_id)  grab in the air (any stick, lock-on or not): an air combo. Each press in a step's next-move
//                       window plays the next step (after the last, the first again); a press outside one starts the
//                       first; a press any time in a step is queued. Every step: Nero hops (16.174 cm a frame, 0.75 of
//                       it on each further air shot before he lands, as the vanilla air shot), fires Blue Rose on the
//                       step's shot frame and keeps the sideways speed he was moving at (a share per step), undecayed, as the air rev keeps his velocity for Rev Flying (uPlayerNero::moveActODJump
//                       0x7F9950): the larger of the stored inertia (mMoveVel along mJumpMoveAng: a jump's, or Calibur's
//                       through a jump cancel) and his last tick's movement (Gerb's mVel), carried on mAddSpd
//                       re-armed every tick, with mMoveVel /
//                       mJumpMoveAng in step so the fall after the shot carries on with it; his jump's gravity goes on.
//                       The clip's root motion is ignored, landing ends the shot.
// Each move has its own LMT block, shot frame, next-move window and cancel frame (fixed in the .cpp).
// The travel is the clip's root motion (joint 255), so the move only holds Nero's own velocity at 0 and lets the motion
// carry him. The shot is the vanilla shot's own calls (uPlayerNero::moveAtckBlueRoseShotAir 0x8010F0): the bullet
// (uPlNeroShl000::create, which aims at the lock-on target or along the facing by itself), the muzzle flash on the left
// hand, Blue Rose's recoil motion, the sound and the pad rumble. One shot sends 3 bullets 10 frames
// apart (each hits on its own, as the vanilla shot's two barrels do), so a single shot in the clip hits several times.
// The bullets after the first run on their own timer (pre_tick), so they all land whatever Nero does next: a chain into
// the next move or a clip shorter than the spacing doesn't drop them.
// (DanteJdc gets its multi-hit by clearing its shell's hit byte +0x15 in the shell's update, 0x81F8A3; the same for
// Blue Rose would hook uPlNeroShl000, every Blue Rose bullet's class, so extra bullets are used instead.)
// Oomph: the bullets these moves fire are remembered (fire), and uPlNeroShl000's hitMessage (vtbl +0x94, wrapped) gives
// theirs a harder hit: uActor::damagePreMessage calls it before the enemy handles the hit, and it copies the wp000.atk
// entry for the distance into the hit's collision group (rAttackStatusData::get 0x5099E0, group +0xA4), so raising that
// copy's interrupt level (mAttackLvI: the enemy's interrupt gauges up to that level fill, uDamage::calc 0x51BE10, so
// heavier enemies flinch and hover; the shots are level 1, Dante's Twosome Time 2), its hit stop (mHitStopTimer, the
// frames uEnemy::setHitStop freezes the enemy for; 0 on every gun entry) and its hit sound changes that hit only. The
// wrapped destructor (vtbl +0) forgets a bullet, so a vanilla bullet that reuses its memory isn't touched. The move's
// last shot also shakes the camera, lighter than any of the game's presets (uDevilCamera::vibReq_2 0x531A80: strength
// and length of its own; the presets of vibReq 0x531B60 are 150-425 strong over 5-32 frames), and every shot can layer a
// charged shot's sound over the plain one (setGunSe by the lent atckId 45-47).
// A move with a charge level fires a charge shot instead, as the vanilla charge shot (uPlayerNero::
// moveAtckBlueRoseChargeShotAir 0x801BF0): one charged bullet (uPlNeroShl001::create(level - 1, owner), which reads
// nothing of Nero's state but its parameters), the charged flash (setGunEffect kind = level), the gun's recoil, the
// level's sound (setGunSe by the lent atckId 44 + level) and rumble (4 / 6 / 8). Its hits keep wp000.atk's charge
// entries (interrupt levels 2-4), so the oomph doesn't apply to it. An air shot can also kick Nero on its shot
// frame: pushed away from where he faces and lifted (air_kick), the push fading by 0.9 a frame on top of
// the momentum he carries, as the vanilla air charge shot pushes him back (its mMoveVel recoil, damped every frame).
//
// Input: a game command (input_commands: lock-on held, the stick leaning any way, then the grab button, 0x200, within
// 2 frames), on a free command bit of Nero's that is the row's command; the air shot's command is the grab button alone,
// on a bit of its own. The rows' style condition
// is NeroStyle's Gunslinger value (nero_style::gunslinger_condition): in the Gunslinger style the Snatch and Gerb rows
// are off (they need their own styles), so the button is free there, and in the other styles it keeps them.
//
// The motions come from motion\pl000\gunslinger.lmt (Albam's LMT export) through the motion registry, one block per
// move (ShotMove::block). Blue Rose is drawn while the clip's first-table events have bit 26 (gun_display) on: key it in
// Albam. The phase lives in mBdyAction[1].phase, which setAction zeroes: 0 entry, 1 the dodge or the air shot.

namespace gunslinger {

constexpr uint32_t move_id = 0x43;            // Nero's atckId (Gerb has 0x40 / 0x41, FreeRide 0x42)
constexpr uint32_t air_move_id = 0x46;        // the air shot's (the MoveDef examples gerb / gerb_vertical use 0x44 / 0x45)
constexpr uint32_t ground_move_id = 0x47;     // the neutral ground shot's

extern bool mod_enabled;
extern int  cmd_bit;                  // the dodge command's mCmdFlag bit (next_free_bit), -1 none
extern int  air_cmd_bit;              // the air shot command's, -1 none

bool ready(uintptr_t player);         // the LMT is on this player's models (the registry builds them if needed)
void register_lmt();                  // (re-)registers the LMT with the motion registry
void __stdcall move(uintptr_t player, uint32_t atck_id);   // the per-tick state function
void apply_row();                     // the rows' command bits, enable bytes and style condition

} // namespace gunslinger

class NeroGunslinger : public Mod {
public:
    NeroGunslinger() = default;
    std::string get_mod_name() override { return "NeroGunslinger"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_cmd;
    std::unique_ptr<Patch> patch_shl_hit, patch_shl_dtor;   // uPlNeroShl000::vftable +0x94 / +0
};
