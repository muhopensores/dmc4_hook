#include "NeroGunslinger.hpp"
#include "NeroStyle.hpp"
#include "AfterImage.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MotionRegistry.hpp"
#include "../sdk/CtnScript.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <string>

using namespace wallrun;

namespace gunslinger {

bool  mod_enabled      = false;
int   cmd_bit          = -1;
int   air_cmd_bit      = -1;

namespace {

int   lmt_set          = -1;            // the motion_registry handle of kLmtPath (-1 none)
constexpr const char* kLmtPath = "motion\\pl000\\gunslinger";   // game path without extension
constexpr uint32_t kCtnBank = 0, kCtnSlot = 5;   // the vanilla .ctn script borrowed (Gerb's): registry banks have none
constexpr float kBlendFrames     = 4.0f;   // interpolation into a clip
constexpr int   kBullets         = 3;      // bullets a dodge's or an air shot's shot sends (each hits once)
constexpr float kBulletInterval  = 10.0f;  // frames between them (the vanilla shot's two barrels: 5)

// the dodges, by the stick's direction
enum Dir : int { DIR_BACK = 0, DIR_LEFT = 1, DIR_FORWARD = 2, DIR_RIGHT = 3, DIR_COUNT = 4 };
struct ShotMove {
    int   block;                      // its LMT block
    float shot_frame;                 // the clip's frame the shot fires on
    float next_from;                  // from this frame a press for the next move (C_COMMAND) is queued (-1 = none)
    float next_at;                    // from this frame the next move starts, a queued press at once (-1 = none)
    float cancel_frame;               // from this frame every cancel window is open (-1 = the borrowed script's windows)
    int   charge = 0;                 // 0 = Blue Rose's plain shot; 1-3 = a charge shot of that level (one charged bullet)
    float after_image_until = -1.0f;  // AfterImage's trail up to this frame of the clip (-1 = none)
};
constexpr ShotMove dodges[DIR_COUNT] = {
    { 0, 20.0f, -1.0f, -1.0f, 35.0f, 0, 35.0f },   // back: 43 frames, the travel ends at 35
    { 1,  8.0f, -1.0f, -1.0f, 38.0f, 0, 39.0f },   // left: 41 frames, travelling to 39
    { 3,  8.0f, -1.0f, -1.0f, 28.0f, 0, 34.0f },   // forward (made at 48 fps): 45 frames, the travel ends at 34
    { 2,  8.0f, -1.0f, -1.0f, 38.0f, 0, 40.0f },   // right: 41 frames, travelling to 40
};
const char* const dir_names[DIR_COUNT] = { "Back", "Left", "Forward", "Right" };

// the air shots: the air combo's steps, in order. A press any time in a step is queued (next_from 0) and plays the
// next step at its next_at.
constexpr int AIR_STEPS = 3;
constexpr ShotMove air_shots[AIR_STEPS] = {
    { 5,  7.0f, 0.0f, 12.0f, 22.0f },      // 31 frames
    { 4, 13.0f, 0.0f, 17.0f, 20.0f },      // 24 frames; the arm kicks on 13-14
    { 7, 20.0f, 0.0f, 12.0f, 22.0f, 3 },   // a charge shot on 20
};
constexpr float air_momentum[AIR_STEPS] = { 0.5f, 1.0f, 1.0f };   // each step's share of the sideways speed it starts with
constexpr float kAirGravity     = -1.1f;   // Gerb's: added to the fall speed every frame when Nero brings no gravity of his own
constexpr float kAirMaxFall     = 30.0f;   // the fall speed gravity stops adding at
constexpr float kAirLift        = 16.174f; // the hop: the rise speed an air shot starts with at least (cm per frame)
constexpr float kAirLiftDecay   = 0.75f;   // each further air shot before Nero lands lifts this much of the one before
constexpr float kAirChainGrace  = 12.0f;   // frames after an air shot ends (Nero falling) that a press still plays the next step
struct AirKick {
    float knockback;                  // on the shot frame Nero is pushed away from where he faces at this speed (cm per frame)
    float lift;                       // and rises at least this fast (cm per frame); 0 = neither
};
constexpr AirKick air_kick[AIR_STEPS] = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 6.0f, 10.0f } };   // the charge shot kicks Nero back and up
constexpr float kAirKickDecay   = 0.9f;    // the push left after each frame (about 60 cm of push in all)

// the neutral ground shot: Nero fans the hammer, a full shot of one bullet on each of kFanFrames
constexpr ShotMove ground_shot = { 6, 0.0f, 6.0f, 14.0f, 15.0f };   // 19 frames (made at 48 fps); the next shot queues from the second shot
constexpr int   kFanShots = 3;
constexpr float kFanFrames[kFanShots] = { 0.0f, 6.0f, 12.0f };

// the neutral shot held: after the volley Nero spins the gun while the button stays down, charging
constexpr int   kSpinBlock       = 8;
constexpr float kSpinFrom        = 14.0f;   // the neutral shot's frame the spin starts on (its Next move at)
constexpr float kSpinBlend       = 4.0f;    // interpolation from the neutral shot into the spin
constexpr float kSpinCancelFrame = 0.0f;    // from this frame of the spin every cancel window is open
constexpr float kSpinCharge[3]   = { 10.0f, 25.0f, 45.0f };   // frames of spinning each charge level takes

// oomph
struct BulletPower {
    int level;                        // the interrupt level of its hits (mAttackLvI; the vanilla shot's 1)
    int hit_stop;                     // frames the enemy freezes for on a hit
    int hit_se;                       // the hit sound (wp000.atk mHitSE: 9 the shot's, 11 the charge shot's; -1 = keep)
};
constexpr BulletPower shot_power  = { 2, 0, -1 };   // every bullet of these moves: Twosome Time's close-range level
constexpr BulletPower final_power = { 3, 5, 11 };   // the last bullet of a move's last shot
struct Shake {
    int strength;                     // how far the screen moves (the game's lightest preset: 150)
    int frames;                       // how long it lasts
};
constexpr Shake final_shake = { 60, 5 };   // on a dodge's or an air shot's shot; the neutral shot has none
constexpr int kHeavySound = 1;             // the charge shot level whose sound is layered over a dodge's or air shot's

// ---------------------------------------------------------------------------------------------------------------
// Game functions. The shot's calling conventions are the vanilla air shot's (uPlayerNero::moveAtckBlueRoseShotAir
// 0x8010F0, phases 1-3).
// ---------------------------------------------------------------------------------------------------------------
constexpr uintptr_t fn_shl000_create    = 0x0082C6A0;   // uPlNeroShl000::create(int barrel, uActor* owner), cdecl
constexpr uintptr_t fn_shl001_create    = 0x0082D960;   // uPlNeroShl001::create(int level 0-2, uActor* owner), cdecl: the charge shot's bullet
constexpr uintptr_t fn_setGunEffect     = 0x007E7090;   // uPlayerNero::setGunEffect(this@esi, int kind), retn 4: kind 0 = the shot's flash, 1-3 the charge levels'
constexpr uintptr_t fn_setShot          = 0x00831FB0;   // uPlWpBlueRose::setShot(this@eax): the gun's recoil motion
constexpr uintptr_t fn_setGunSe         = 0x007E7BB0;   // uPlayerNero::setGunSe(this@edi, int barrel), retn 4
constexpr uintptr_t fn_setPadVibration  = 0x007AC130;   // uPlayer::setPadVibration(this@edx, kind@eax)
constexpr uintptr_t fn_setActionFootwork = 0x007AA2C0;  // uPlayer::setActionFootwork(this@esi, float hokan, char isMotSet), callee pops
constexpr uintptr_t fn_setActionLand    = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)
constexpr uintptr_t fn_vibReq2          = 0x00531A80;   // uDevilCamera::vibReq_2(this@edi, kind, strength, frames, axis, MtVector3* at), retn 14h
constexpr uintptr_t fn_shl000_hit       = 0x0082C590;   // uPlNeroShl000::hitMessage(this@ecx, own group, other group), retn 8
constexpr uintptr_t fn_shl000_dtor      = 0x0082BF60;   // uPlNeroShl000's scalar deleting destructor(this@ecx, flags), retn 4
constexpr uintptr_t kShl000Vtbl         = 0x00BE8E40;   // uPlNeroShl000::vftable
constexpr uintptr_t fn_setEffectConst  = 0x00480480;   // sDevil4Effect::setEffectConst(list@eax, file@edx, parent, group mask, material mask, unit line, owner), retn 14h
constexpr uintptr_t fn_setEffectSync    = 0x0081FDA0;   // uPlayer::setEffectSync(effect@eax, this@ecx, 1, type, 1), retn 0Ch: the effect kept as `type`
constexpr uintptr_t fn_finishEffectId   = 0x0081FB70;   // uPlayer::finishEffectId(this@eax, type, bool), retn 8
constexpr uintptr_t fn_requestSeWPos    = 0x00488FF0;   // uPlayer::requestSeWPos(rsrc@ecx, this@edx, cResource*, se, owner), retn 0Ch
constexpr uintptr_t fn_checkAtckBit     = 0x007ACE10;   // uPlayer::checkAtckBit(this@ecx, atckId@esi) -> al: the move is learned
constexpr uintptr_t fn_setActionAtckGun = 0x007AAE20;   // uPlayer::setActionAtckGun(this@edi, atckId), retn 4: a gun attack as the gun rows start it
constexpr uintptr_t kSeResource         = 0x1D64;       // uPlayer's sound resource (the charge sounds' bank)
constexpr uintptr_t kCameraOfMediator   = 0xD0;         // sMediator -> uDevilCamera
constexpr uintptr_t kGroupAttack        = 0xA4;         // cCollisionGroup::mAttackStatus (hitStopMessage reads +0xCC = its mHitStopTimer)

constexpr uintptr_t kWeapons     = 0x1D90;   // uPlayer::mWeaponCtrl.mpWeapon[7] (cWeaponCtrl at +0x1D88, list at +8)
constexpr uintptr_t kWeaponId    = 0x1370;   // uPlWeapon::mID: Blue Rose = 1 (cWeaponCtrl::setup<uPlWpBlueRose>)
constexpr uint32_t  kBlueRoseShotId = 44;    // the plain Blue Rose shot's atckId: setGunSe picks the sound by mAtckId - 44

uintptr_t blue_rose(uintptr_t pl) {
    for (int i = 0; i < 7; ++i) {
        const uintptr_t w = at<uintptr_t>(pl, kWeapons + 4 * i);
        if (w && at<int>(w, kWeaponId) == 1) return w;
    }
    return 0;
}

void set_gun_effect(uintptr_t pl, int kind) {
    uintptr_t fn = fn_setGunEffect;
    __asm {
        pushad
        mov esi, pl
        push kind
        call fn
        popad
    }
}

void set_shot(uintptr_t gun) {
    uintptr_t fn = fn_setShot;
    __asm {
        pushad
        mov eax, gun
        call fn
        popad
    }
}

// The sound goes by the shot's atckId (any other id plays nothing): the plain shot's (44), or a charge shot's (45-47,
// its level's sound), lent for the call.
void gun_se(uintptr_t pl, int barrel, uint32_t shot_id = kBlueRoseShotId) {
    uint32_t& id = at<uint32_t>(pl, pl::mAtckId);
    const uint32_t keep = id;
    id = shot_id;
    uintptr_t fn = fn_setGunSe;
    __asm {
        pushad
        mov edi, pl
        push barrel
        call fn
        popad
    }
    id = keep;
}

void pad_vibration(uintptr_t pl, int kind) {
    uintptr_t fn = fn_setPadVibration;
    __asm {
        pushad
        mov edx, pl
        mov eax, kind
        call fn
        popad
    }
}

void set_action_footwork(uintptr_t pl, float hokan, int is_mot_set) {
    uintptr_t fn = fn_setActionFootwork;
    __asm {
        pushad
        mov esi, pl
        push is_mot_set
        push hokan
        call fn
        popad
    }
}

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// A screen shake as the game's presets make it (kind 3, the x axis, as preset 24), of our own strength and length;
// no position, so the camera's distance doesn't weaken it.
void camera_shake(const Shake& sh) {
    if (sh.strength <= 0 || sh.frames <= 0) return;
    const uintptr_t med = *(const uintptr_t*)static_mediator_ptr;
    const uintptr_t cam = med ? *(const uintptr_t*)(med + kCameraOfMediator) : 0;
    if (!cam) return;
    const int strength = sh.strength, frames = sh.frames;
    uintptr_t fn = fn_vibReq2;
    __asm {
        pushad
        mov edi, cam
        push 0
        push 1
        push frames
        push strength
        push 3
        call fn
        popad
    }
}

// The bullets these moves fired that are still alive, and whether each is a move's last.
struct OurBullet { uintptr_t shell; bool last; };
OurBullet g_bullets[32];
int g_bullet_next = 0;
uint32_t g_power_hits = 0;   // readout

OurBullet* our_bullet(uintptr_t shell) {
    for (OurBullet& b : g_bullets) if (b.shell == shell) return &b;
    return nullptr;
}

void remember(uintptr_t shell, bool last) {
    if (!shell) return;
    OurBullet* slot = our_bullet(0);
    if (!slot) { slot = &g_bullets[g_bullet_next]; g_bullet_next = (g_bullet_next + 1) % 32; }   // full: the oldest goes
    *slot = { shell, last };
}

// uPlNeroShl000::hitMessage, then our bullets' hit made harder (the group's attack status the call just copied)
void __fastcall shl_hit(uintptr_t shell, uintptr_t edx, uintptr_t own, uintptr_t other) {
    ((void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t))fn_shl000_hit)(shell, edx, own, other);
    const OurBullet* b = mod_enabled ? our_bullet(shell) : nullptr;
    if (!b || !own) return;
    const BulletPower& p = b->last ? final_power : shot_power;
    kAttackStatus& a = *(kAttackStatus*)(own + kGroupAttack);
    a.mAttackLvI = p.level;
    a.mHitStopTimer = p.hit_stop;
    if (p.hit_se >= 0) a.mHitSE = p.hit_se;
    ++g_power_hits;
}

uintptr_t __fastcall shl_dtor(uintptr_t shell, uintptr_t edx, uint32_t flags) {
    if (OurBullet* b = our_bullet(shell)) b->shell = 0;
    return ((uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uint32_t))fn_shl000_dtor)(shell, edx, flags);
}

// One Blue Rose bullet; the shot's first also brings the flash, the gun's recoil, the rumble and the sound, as the
// vanilla shot fires its first barrel. `last` = the move's last bullet; `last_shot` = it belongs to the move's last shot,
// which shakes the camera by `shake` (none: no shake); `heavy` = the charge shot level whose sound is layered on (0 none).
void fire(uintptr_t pl, bool first, bool last, bool last_shot, const Shake* shake = nullptr, int heavy = 0) {
    remember(((uintptr_t(__cdecl*)(int, uintptr_t))fn_shl000_create)(0, pl), last);
    if (!first) return;
    set_gun_effect(pl, 0);
    if (const uintptr_t gun = blue_rose(pl)) set_shot(gun);
    pad_vibration(pl, 0x11);
    gun_se(pl, 0);
    if (heavy > 0) gun_se(pl, 0, kBlueRoseShotId + (uint32_t)std::min(heavy, 3));
    if (last_shot && shake) camera_shake(*shake);
}

// A charge shot of `level` (1-3), as the vanilla one fires it: the charged bullet, its flash, the recoil, its sound and rumble.
void fire_charged(uintptr_t pl, int level, const Shake& shake) {
    level = std::clamp(level, 1, 3);
    ((uintptr_t(__cdecl*)(int, uintptr_t))fn_shl001_create)(level - 1, pl);
    set_gun_effect(pl, level);
    if (const uintptr_t gun = blue_rose(pl)) set_shot(gun);
    pad_vibration(pl, 2 + 2 * level);
    gun_se(pl, 0, kBlueRoseShotId + (uint32_t)level);
    camera_shake(shake);
}

// The vanilla charge's glow and sounds (uPlayerNero::updateChargeShotEffect 0x7E8180): effect file 0x29 kept as type
// 0xB (the glow), 0x2A as type 0xC (the level's flash), sounds 14 (charging), 10 / 11 / 12 (levels), 13 (let go).
constexpr uint32_t kGlowType = 0xB, kFlashType = 0xC;

uintptr_t effect_const(uintptr_t pl, uint32_t file, uint32_t group) {
    uintptr_t fn = fn_setEffectConst, result = 0;
    __asm {
        pushad
        push pl
        push 14h
        push 0FFFFFFFFh
        push group
        push pl
        mov eax, 1
        mov edx, file
        call fn
        mov result, eax
        popad
    }
    return result;
}

void keep_effect(uintptr_t pl, uint32_t file, uint32_t group, uint32_t kind) {
    const uintptr_t effect = effect_const(pl, file, group);
    if (!effect) return;
    uintptr_t fn = fn_setEffectSync;
    __asm {
        pushad
        push 1
        push kind
        push 1
        mov eax, effect
        mov ecx, pl
        call fn
        popad
    }
}

void finish_effect(uintptr_t pl, uint32_t kind) {
    uintptr_t fn = fn_finishEffectId;
    __asm {
        pushad
        push 1
        push kind
        mov eax, pl
        call fn
        popad
    }
}

void player_se(uintptr_t pl, uint32_t se) {
    const uintptr_t rsrc = at<uintptr_t>(pl, kSeResource);
    if (!rsrc) return;
    uintptr_t fn = fn_requestSeWPos;
    __asm {
        pushad
        push pl
        push se
        push rsrc
        mov ecx, rsrc
        mov edx, pl
        call fn
        popad
    }
}

bool learned(uintptr_t pl, uint32_t atck_id) {
    uintptr_t fn = fn_checkAtckBit;
    uint8_t result = 0;
    __asm {
        pushad
        mov ecx, pl
        mov esi, atck_id
        call fn
        mov result, al
        popad
    }
    return result != 0;
}

void start_gun_attack(uintptr_t pl, uint32_t atck_id) {
    uintptr_t fn = fn_setActionAtckGun;
    __asm {
        pushad
        mov edi, pl
        push atck_id
        call fn
        popad
    }
}

void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp) {
    devil4_sdk::indexed_anim_call((uint32_t)motion, (uPlayer_old*)pl, attr, 1.0f, 0.0f, interp);   // requestSetMotion_2, whole body
}

// getLockOnTarget (vtbl 170, thiscall (this, a2, a3, exclude), retn 0Ch) as the game's lever check asks it: (0, 0, 0)
uintptr_t lock_target(uintptr_t pl) {
    const uintptr_t fn = (*(uintptr_t**)pl)[170];
    return ((uintptr_t(__fastcall*)(uintptr_t, int, int, int, uintptr_t))fn)(pl, 0, 0, 0, 0);
}

// Where the stick leans against the lock-on target's direction (else the facing), as the game's lever check measures it:
// forward / back within 45 deg, left / right between; -1 = centred.
int stick_dir(uintptr_t pl) {
    if (at<int>(pl, pl::mAnlgPow) == 0) return -1;
    float ref = at<float>(pl, pl::mRotateNow);
    if (const uintptr_t t = lock_target(pl)) {
        const MtVector3& tp = at<MtVector3>(t, 0x10);
        const MtVector3& pp = at<MtVector3>(pl, pl::mPos);
        const float dx = tp.x - pp.x, dz = tp.z - pp.z;
        if (dx * dx + dz * dz > 1.0f) ref = std::atan2(dx, dz);
    }
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const float sx = std::sin(yaw), sz = std::cos(yaw);
    const float along = sx * std::sin(ref) + sz * std::cos(ref);
    const float side  = -sx * std::cos(ref) + sz * std::sin(ref);   // against the right axis (the game's yaw grows to the left)
    if (along >= 0.70710678f)  return DIR_FORWARD;
    if (along <= -0.70710678f) return DIR_BACK;
    return side >= 0.0f ? DIR_RIGHT : DIR_LEFT;
}

void face(uintptr_t pl, float yaw) {
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

// Turns Nero toward the lock-on target, when there is one a metre or more away.
void face_lock_target(uintptr_t pl) {
    if (const uintptr_t t = lock_target(pl)) {
        const MtVector3& tp = at<MtVector3>(t, 0x10);
        const MtVector3& pp = at<MtVector3>(pl, pl::mPos);
        const float dx = tp.x - pp.x, dz = tp.z - pp.z;
        if (dx * dx + dz * dz > 1.0f) face(pl, std::atan2(dx, dz));
    }
}

void hold_still(uintptr_t pl) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    vel.x = vel.z = 0.0f;
    acc.x = acc.y = acc.z = 0.0f;
    at<float>(pl, pl::mMoveVel) = 0.0f;
    at<float>(pl, pl::mMoveAcc) = 0.0f;
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

// The ground moves follow the floor as the game's own attacks do (Stinger, the combos, the shots all turn this on as
// they start; the attack action's row has it off): mActLimit.fall is the ground snap, uPlayer::main pressing Nero down
// by mFallHeight (20 cm a frame) before the collision, and updateCollisionCorrectScr handing it back when no floor
// caught him. Without it a dodge stays at the height it started from and a slope or step down leaves it off the ground.
constexpr uintptr_t kScrClsnRslt  = 0x2008;   // uPlayer::mScrClsnRslt: updateSctClsnInfo's floor ray, 100 cm above to 100 below his feet
constexpr uintptr_t kScrClsnTri   = 0x201C;   // the triangle it hit (+8 its attributes)
constexpr uint32_t  kWallChannels = 0x7F0000;
constexpr int       kAirTicks     = 4;          // ticks over no floor before a ground move turns into a fall (a ledge behind)

void follow_floor(uintptr_t pl) { at<uint32_t>(pl, pl::mActLimit) |= 1u; }

// And they drop only where the game would make a standing Nero fall (uPlayer::checkFloor 0x803D60): off the ground
// with no floor within a metre below, or only a wall there, for more than kAirTicks ticks. Over a dip, a slope or a
// step the snap brings him down and the move plays on.
bool left_floor(uintptr_t pl, int& air_ticks) {
    follow_floor(pl);
    const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
    bool over_nothing = coll && !at<bool>(coll, 0x1C);                  // mLand
    if (over_nothing && at<uint32_t>(pl, kScrClsnRslt)) {
        const uintptr_t tri = at<uintptr_t>(pl, kScrClsnTri);
        over_nothing = tri && (at<uint32_t>(tri, 8) & kWallChannels);
    }
    air_ticks = over_nothing ? air_ticks + 1 : 0;
    return air_ticks > kAirTicks;
}

bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

constexpr int PHASE_ENTRY = 0;
constexpr int PHASE_DODGE = 1;
constexpr int PHASE_AIR   = 1;     // the air shot's (its own move id)
constexpr int PHASE_GROUND = 1;    // the neutral ground shot's (its own move id)
constexpr int PHASE_SPIN  = 2;     // the neutral shot held: the spin

constexpr int kPlayerSlots = 8;
static ctn::Script g_cancel_scripts[kPlayerSlots];   // the bytes outlive the motion: static

struct State {
    uintptr_t player = 0;
    uint32_t last_seen = 0;
    bool script_set = false;         // the cancel script has replaced the borrowed one
    int dir = DIR_BACK;              // the dodge's direction (Dir)
    bool fired = false;              // the move's shot has fired
    int pending = 0;                 // bullets of a shot still to come (pre_tick fires them)
    bool pending_last = false;       // they belong to the move's last shot (the last of them is the move's last bullet)
    float next_in = 0.0f;            // frames to the next of them
    int air_ticks = 0;
    float gravity = 0.0f;            // the air shot's: Nero's own when he brings one, else kAirGravity
    int air_count = 0;               // air shots since he last stood on the ground (the lift shrinks with each)
    int air_step = 0;                // the air combo's step playing (or last played)
    int fan_next = 0;                // the neutral shot's next shot (kFanFrames index)
    bool held = false;               // the neutral shot's button has stayed down since the press
    float spin_time = 0.0f;          // frames the spin has run
    int charge_level = 0;            // the spin's charge level reached (0-3)
    bool charge_fx = false;          // its glow is on
    uint32_t ticks = 0;              // Nero's ticks (pre_tick)
    float air_gap = 1e9f;            // frames since the air shot last ran (pre_tick counts them; landing ends the combo)
    float drift_x = 0.0f, drift_z = 0.0f;   // the air shot's sideways speed (cm per frame)
    float kick_x = 0.0f, kick_z = 0.0f;     // the shot's push on top of it (air_kick), fading
    // Nero's position at the last two pre-check ticks and the frames each of those ticks ran: the speed he moved at
    float last_x = 0.0f, last_z = 0.0f, last_dt = 0.0f, prev_x = 0.0f, prev_z = 0.0f, prev_dt = 0.0f;
    int seen = 0;                    // positions recorded (2 = the speed can be measured)
};
State g_states[kPlayerSlots];
int slot_of(const State& s) { return (int)(&s - g_states); }

// One slot per uPlayer seen; a full table evicts the one seen longest ago (a live player is seen every tick).
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

// The move's cancel script, by frame: the next move's window (a press queued from next_from, the move starting from
// next_at; the Gunslinger rows cancel on C_COMMAND, as every command row), then every window at cancel_frame. Nothing
// set (all -1) keeps the borrowed script.
void arm_cancel_script(State& s, uintptr_t pl, const ShotMove& m) {
    if (s.script_set) return;
    s.script_set = true;
    struct Step { float frame; int kind; } steps[3] = { { m.next_from, 0 }, { m.next_at, 1 }, { m.cancel_frame, 2 } };
    std::sort(std::begin(steps), std::end(steps), [](const Step& a, const Step& b) { return a.frame < b.frame; });
    ctn::Script& sc = g_cancel_scripts[slot_of(s) % kPlayerSlots];
    sc.reset();
    bool any = false;
    for (const Step& st : steps) {
        if (st.frame < 0.0f) continue;
        sc.wait(st.frame);
        if (st.kind == 0)      sc.reserve({ C_COMMAND });
        else if (st.kind == 1) sc.open({ C_COMMAND });
        else                   sc.open_all();
        any = true;
    }
    if (any) devil4_sdk::install_ctn_script((void*)pl, sc.end().data());
}

// The gate every cancel goes through (uPlayer::main runs checkAction only while it is set); the vanilla attack
// functions set it at their cancel frame, and so does this, from the move's first window.
void open_cancel_gate(uintptr_t pl, const ShotMove& m) {
    float first = -1.0f;
    for (const float f : { m.next_from, m.next_at, m.cancel_frame })
        if (f >= 0.0f && (first < 0.0f || f < first)) first = f;
    if (first >= 0.0f && at<float>(pl, pl::mMotionFrame0) >= first)
        at<bool>(pl, pl::mIsCancelCheckEnable) = true;
}

void leave(uintptr_t pl) {
    if (at<uint32_t>(pl, pl::mActStat) & 0x2) devil4_sdk::neutral_air_transition((uPlayer_old*)pl);
    else set_action_footwork(pl, 10.0f, 1);
}

static input_commands::Command g_dodge_cmd;   // lock-on held + a stick direction + grab; built in on_initialize
static input_commands::Command g_air_cmd;     // the grab button (the row is air only)
static uint32_t g_cmd_seen = 0, g_dodges = 0, g_air_shots = 0, g_shots = 0;   // readout
static char g_last[96] = "";
static float g_spin_frames = 0.0f;   // readout: the last spin's length so far
static uint32_t g_charge_shots = 0;  // readout

// A shot of `count` bullets: the first now, the rest left to pre_tick's timer. `last_shot` = the move's last shot.
void shoot(uintptr_t pl, State& s, int count, bool last_shot, const Shake* shake, int heavy) {
    fire(pl, true, last_shot && count <= 1, last_shot, shake, heavy);
    ++g_shots;
    if (count > 1) {
        if (s.pending == 0) s.next_in = kBulletInterval;
        s.pending += count - 1;
        s.pending_last = last_shot;
    }
}

// The move's shot once the clip reaches its frame (the dodges and air shots have one): true on the tick it fires.
// AfterImage's trail while the move's clip is before its after_image_until frame.
void after_images(uintptr_t pl, const ShotMove& m) {
    if (m.after_image_until > 0.0f && at<float>(pl, pl::mMotionFrame0) < m.after_image_until)
        after_image::trail_request(pl);
}

bool fire_due(uintptr_t pl, State& s, const ShotMove& m) {
    if (s.fired || at<float>(pl, pl::mMotionFrame0) < m.shot_frame) return false;
    s.fired = true;
    if (m.charge > 0) {
        fire_charged(pl, m.charge, final_shake);
        ++g_shots;
    } else {
        shoot(pl, s, kBullets, true, &final_shake, kHeavySound);
    }
    return true;
}

// The neutral shot's shots as the clip reaches their frames, each a full shot (flash, recoil, the plain sound) of one
// bullet; no camera shake (it fires fast and is pressed again and again).
void fire_fan(uintptr_t pl, State& s) {
    const float frame = at<float>(pl, pl::mMotionFrame0);
    while (s.fan_next < kFanShots && frame >= kFanFrames[s.fan_next]) {
        ++s.fan_next;
        shoot(pl, s, 1, s.fan_next == kFanShots, nullptr, 0);
    }
}

// The pending bullets, kBulletInterval apart, whatever Nero is doing.
void fire_pending(uintptr_t pl, State& s) {
    if (s.pending <= 0) return;
    s.next_in -= at<float>(pl, pl::m_delta_time);
    while (s.pending > 0 && s.next_in <= 0.0f) {
        fire(pl, false, s.pending_last && s.pending == 1, false);
        ++g_shots;
        --s.pending;
        s.next_in += kBulletInterval;
    }
}

// The highest charge level the spin may reach: the levels Nero has learned (level 1 always, 2 / 3 as the vanilla charge
// checks them).
int charge_cap(uintptr_t pl) {
    if (!learned(pl, kBlueRoseShotId + 2)) return 1;
    return learned(pl, kBlueRoseShotId + 3) ? 3 : 2;
}

// The charge's glow and level sounds as the vanilla charge plays them, for each level the spin reaches.
void charge_start(uintptr_t pl, State& s) {
    s.charge_level = 0;
    s.charge_fx = true;
    keep_effect(pl, 0x29, 1, kGlowType);
    player_se(pl, 14);
}

void charge_update(uintptr_t pl, State& s) {
    static const uint32_t glow[4] = { 1, 2, 4, 8 }, flash[4] = { 0, 1, 2, 4 };
    const int cap = charge_cap(pl);
    while (s.charge_level < cap && s.spin_time >= kSpinCharge[s.charge_level]) {
        const int level = ++s.charge_level;
        finish_effect(pl, kGlowType);
        keep_effect(pl, 0x2A, flash[level], kFlashType);
        keep_effect(pl, 0x29, glow[level], kGlowType);
        player_se(pl, 9 + (uint32_t)level);
    }
}

void charge_end(uintptr_t pl, State& s) {
    if (!s.charge_fx) return;
    s.charge_fx = false;
    finish_effect(pl, kGlowType);
    finish_effect(pl, kFlashType);
    player_se(pl, 13);
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// The LMT goes through the motion registry, which picks a free bank and merges it with the other mods' files.
// ---------------------------------------------------------------------------------------------------------------
bool ready(uintptr_t pl) {
    return pl && lmt_set >= 0 && motion_registry::ready(lmt_set, pl);
}

void register_lmt() {
    motion_registry::SetDesc d;
    d.owner = "NeroGunslinger";
    d.body[motion_registry::NERO] = kLmtPath;
    const int old = lmt_set;
    lmt_set = motion_registry::add(d);
    if (old >= 0) motion_registry::remove(old);
}

// ---------------------------------------------------------------------------------------------------------------
// The moves.
// ---------------------------------------------------------------------------------------------------------------

// The air shots. A start right after an air shot ran (the row fired in its next-move window) plays the combo's next
// step, any other start the first; after the last step that is on, the first again.
// Momentum: the sideways speed Nero was moving at goes on undecayed, as the air rev keeps his velocity for Rev Flying
// (uPlayerNero::moveActODJump 0x7F9950 never touches mVel.x / z). Moves keep that speed in different places: Gerb in
// mVel, a jump and Calibur in mAddSpd, which uPlayer::calcPosition 0x7ACEE0 adds only while mIsAddSpeed and then clears
// (calcSpeedXZ 0x7ACAB0 re-arms it every tick from mMoveVel, the stored inertia: Calibur keeps its dash speed there
// until its final hit, and a jump cancel out of it (the enemy step zeroes mVel, not mMoveVel) keeps it until the jump
// frame, which is what Rev Flying carries). So the speed is the larger of that inertia and his last tick's movement
// (pre_tick's positions: Gerb's mVel, whose mMoveVel mirrors it), carried on mAddSpd, re-armed every tick,
// with mMoveVel / mJumpMoveAng in step (mMoveAcc 0) so the default fall after the shot (setActionFall 0x7AA410: mVel.x /
// z zeroed, the drift rebuilt by calcSpeedXZ) carries on with it. Vertically he rises at least at the lift (kAirLift,
// times kAirLiftDecay for each air shot before it since he landed), and gravity goes on adding to the fall as in his
// jump (his own acceleration when he brings one, else kAirGravity); the clip's root motion is off. Landing ends it, and
// so does the clip's end (the default fall).
static void carry_drift(uintptr_t pl, const State& s) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    const float x = s.drift_x + s.kick_x, z = s.drift_z + s.kick_z;
    vel.x = vel.z = 0.0f;
    add.x = x;
    add.y = 0.0f;
    add.z = z;
    at<bool>(pl, pl::mIsAddSpeed) = true;
    const float speed = std::sqrt(x * x + z * z);
    at<float>(pl, pl::mMoveVel) = speed;
    at<float>(pl, pl::mMoveAcc) = 0.0f;
    if (speed > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = std::atan2(x, z);
}

static void air_move(uintptr_t pl, State& s, int& phase) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);

    switch (phase) {
    case PHASE_ENTRY: {
        const bool chained = s.air_gap <= 1.0f + kAirChainGrace + 1e-3f;   // still running, or ended within the grace
        int step = chained ? s.air_step + 1 : 0;
        if (step >= AIR_STEPS) step = 0;
        s.air_step = step;
        const ShotMove& m = air_shots[step];
        const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, m.block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "air shot %d: no motion: %s", step + 1, motion_registry::describe(lmt_set, pl).c_str());
            devil4_sdk::neutral_air_transition((uPlayer_old*)pl);
            return;
        }
        face_lock_target(pl);
        s.gravity = acc.y < 0.0f ? acc.y : kAirGravity;
        const float lift = kAirLift * std::pow(kAirLiftDecay, (float)s.air_count++);
        if (vel.y < lift) vel.y = lift;
        // the larger of the stored inertia (mMoveVel along mJumpMoveAng: a jump's, or Calibur's, which only its final
        // hit zeroes and a jump cancel keeps until the jump frame) and last tick's movement (Gerb's mVel, root motion)
        const float inertia = at<float>(pl, pl::mMoveVel), ang = at<float>(pl, pl::mJumpMoveAng);
        const float ix = std::sin(ang) * inertia, iz = std::cos(ang) * inertia;
        float mx = 0.0f, mz = 0.0f;
        if (s.seen >= 2 && s.prev_dt > 0.0f) {
            mx = (s.last_x - s.prev_x) / s.prev_dt;
            mz = (s.last_z - s.prev_z) / s.prev_dt;
        }
        const bool stored = ix * ix + iz * iz > mx * mx + mz * mz;
        s.drift_x = (stored ? ix : mx) * air_momentum[step];
        s.drift_z = (stored ? iz : mz) * air_momentum[step];
        s.kick_x = s.kick_z = 0.0f;
        carry_drift(pl, s);
        devil4_sdk::borrow_ctn((void*)pl, kCtnBank, kCtnSlot);
        request_motion(pl, motion, (uint32_t)A_NULL_TRANS_OFF, kBlendFrames);
        at<uint32_t>(pl, pl::mActStat) = (at<uint32_t>(pl, pl::mActStat) & ~0x1u) | 0x2u;   // airborne: the air rules and rows
        s.script_set = false;                                               // the cancel script goes in next tick
        s.fired = false;
        ++g_air_shots;
        snprintf(g_last, sizeof(g_last), "air shot %d%s: motion %04X, gravity %.2f, drift %.1f cm/frame (%s)", step + 1,
                 chained ? " (chained)" : "", motion, s.gravity, std::sqrt(s.drift_x * s.drift_x + s.drift_z * s.drift_z),
                 stored ? "inertia" : "movement");
        phase = PHASE_AIR;
        return;
    }
    case PHASE_AIR: {
        const ShotMove& m = air_shots[s.air_step];
        s.air_gap = 0.0f;
        arm_cancel_script(s, pl, m);
        open_cancel_gate(pl, m);
        const float dt = at<float>(pl, pl::m_delta_time);
        after_images(pl, m);
        const float fade = std::pow(kAirKickDecay, dt);
        s.kick_x *= fade;
        s.kick_z *= fade;
        if (fire_due(pl, s, m)) {                                           // the shot's kick: back from the facing, and up
            const AirKick& k = air_kick[s.air_step];
            const float yaw = at<float>(pl, pl::mRotateNow);
            s.kick_x = -std::sin(yaw) * k.knockback;
            s.kick_z = -std::cos(yaw) * k.knockback;
            if (k.lift > 0.0f && vel.y < k.lift) vel.y = k.lift;
        }
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        if (vel.y > -kAirMaxFall)
            vel.y = std::max(vel.y + s.gravity * at<float>(pl, pl::m_delta_time), -kAirMaxFall);
        acc.x = acc.y = acc.z = 0.0f;                                       // the gravity above is the only acceleration
        carry_drift(pl, s);
        if (at<bool>(pl, pl::mMotionFrameEnd0)) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
        return;
    }
    default:
        return;
    }
}

// The neutral ground shot: Nero stands (the clip's root motion as the dodges') and fans the hammer.
static void ground_move(uintptr_t pl, State& s, int& phase) {
    switch (phase) {
    case PHASE_ENTRY: {
        const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, ground_shot.block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "neutral shot: no motion: %s", motion_registry::describe(lmt_set, pl).c_str());
            leave(pl);
            return;
        }
        face_lock_target(pl);
        hold_still(pl);
        follow_floor(pl);
        devil4_sdk::borrow_ctn((void*)pl, kCtnBank, kCtnSlot);
        request_motion(pl, motion, 0u, kBlendFrames);                       // the clip's root motion moves him
        s.script_set = false;                                               // the cancel script goes in next tick
        s.fan_next = 0;
        s.air_ticks = 0;
        s.held = (at<uint32_t>(pl, pl::mBtnOn) & input_commands::BTN_STYLE) != 0;
        ++g_dodges;
        snprintf(g_last, sizeof(g_last), "neutral shot: motion %04X", motion);
        phase = PHASE_GROUND;
        return;
    }
    case PHASE_GROUND: {
        arm_cancel_script(s, pl, ground_shot);
        open_cancel_gate(pl, ground_shot);
        hold_still(pl);
        after_images(pl, ground_shot);
        fire_fan(pl, s);
        if (left_floor(pl, s.air_ticks)) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
        if (!(at<uint32_t>(pl, pl::mBtnOn) & input_commands::BTN_STYLE)) s.held = false;
        if (s.held && s.fan_next >= kFanShots && at<float>(pl, pl::mMotionFrame0) >= kSpinFrom) {
            const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, kSpinBlock) : -1;
            if (motion >= 0) {
                devil4_sdk::borrow_ctn((void*)pl, kCtnBank, kCtnSlot);
                request_motion(pl, motion, 0u, kSpinBlend);
                s.script_set = false;                                       // the spin's cancel script goes in next tick
                s.spin_time = 0.0f;
                charge_start(pl, s);
                snprintf(g_last, sizeof(g_last), "neutral shot held: spin, motion %04X", motion);
                phase = PHASE_SPIN;
                return;
            }
            s.held = false;                                                 // no clip: the neutral shot plays out
            snprintf(g_last, sizeof(g_last), "spin: no motion: %s", motion_registry::describe(lmt_set, pl).c_str());
        }
        if (at<bool>(pl, pl::mMotionFrameEnd0)) { set_action_footwork(pl, 10.0f, 1); return; }
        return;
    }
    case PHASE_SPIN: {                                                      // the spin until the button is let go
        const ShotMove windows = { kSpinBlock, 0.0f, -1.0f, -1.0f, kSpinCancelFrame };
        arm_cancel_script(s, pl, windows);
        open_cancel_gate(pl, windows);
        hold_still(pl);
        s.spin_time += at<float>(pl, pl::m_delta_time);
        g_spin_frames = s.spin_time;
        charge_update(pl, s);
        if (left_floor(pl, s.air_ticks)) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
        if (!(at<uint32_t>(pl, pl::mBtnOn) & input_commands::BTN_STYLE)) {     // let go: the charge shot of the level reached
            const int level = s.charge_level;
            charge_end(pl, s);
            if (level > 0) {
                g_charge_shots++;
                snprintf(g_last, sizeof(g_last), "spin released: charge shot level %d after %.0f frames", level, s.spin_time);
                start_gun_attack(pl, kBlueRoseShotId + (uint32_t)level);
            } else {
                set_action_footwork(pl, 10.0f, 1);
            }
            return;
        }
        if (at<bool>(pl, pl::mMotionFrameEnd0)) {                           // the clip's end, still held: round again
            at<float>(pl, pl::mMotionFrame0) = 0.0f;
            at<bool>(pl, pl::mMotionFrameEnd0) = false;
        }
        return;
    }
    default:
        return;
    }
}

void __stdcall move(uintptr_t pl, uint32_t atck_id) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    State& s = state_for(pl);
    if (atck_id == air_move_id) { air_move(pl, s, phase); return; }
    if (atck_id == ground_move_id) { ground_move(pl, s, phase); return; }

    switch (phase) {
    case PHASE_ENTRY: {
        const int d = stick_dir(pl);
        if (d < 0) {                                                        // post_command_tick keeps these out; a fallback
            snprintf(g_last, sizeof(g_last), "no dodge with the stick centred");
            leave(pl);
            return;
        }
        const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, dodges[d].block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "%s: no motion: %s", dir_names[d], motion_registry::describe(lmt_set, pl).c_str());
            leave(pl);
            return;
        }
        face_lock_target(pl);
        hold_still(pl);
        follow_floor(pl);
        devil4_sdk::borrow_ctn((void*)pl, kCtnBank, kCtnSlot);
        request_motion(pl, motion, 0u, kBlendFrames);                       // the clip's root motion moves him
        s.script_set = false;                                               // the cancel script goes in next tick
        s.dir = d;
        s.fired = false;
        s.air_ticks = 0;
        ++g_dodges;
        snprintf(g_last, sizeof(g_last), "%s dodge: motion %04X", dir_names[d], motion);
        phase = PHASE_DODGE;
        return;
    }
    case PHASE_DODGE: {
        arm_cancel_script(s, pl, dodges[s.dir]);
        open_cancel_gate(pl, dodges[s.dir]);
        hold_still(pl);
        after_images(pl, dodges[s.dir]);
        fire_due(pl, s, dodges[s.dir]);
        if (left_floor(pl, s.air_ticks)) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
        if (at<bool>(pl, pl::mMotionFrameEnd0)) { set_action_footwork(pl, 10.0f, 1); return; }
        return;
    }
    default:
        return;
    }
}

// The rows on_initialize added: byte 0 of the command word = the command's bit, byte 1 = 1 makes the row unmatchable
// (disabled); the style word is NeroStyle's Gunslinger value (-1 = any style while that mod or its gate is off).
static int g_row_marker = 0, g_air_row_marker = 0, g_ground_row_marker = 0;

void apply_row() {
    if (kAtckDefTbl* row = MoveTable::row(0, g_row_marker)) {
        row->command.buffer = (uint32_t)(cmd_bit & 0xFF) | (mod_enabled && cmd_bit >= 0 ? 0u : 0x100u);
        row->atckConditionStyle = nero_style::gunslinger_condition();
    }
    if (kAtckDefTbl* row = MoveTable::row(0, g_air_row_marker)) {
        row->command.buffer = (uint32_t)(air_cmd_bit & 0xFF) | (mod_enabled && air_cmd_bit >= 0 ? 0u : 0x100u);
        row->atckConditionStyle = nero_style::gunslinger_condition();
    }
    if (kAtckDefTbl* row = MoveTable::row(0, g_ground_row_marker)) {   // the air shot's command: the grab button alone
        row->command.buffer = (uint32_t)(air_cmd_bit & 0xFF) | (mod_enabled && air_cmd_bit >= 0 ? 0u : 0x100u);
        row->atckConditionStyle = nero_style::gunslinger_condition();
    }
}

// Nero's pre-check tick: keeps the rows in step with NeroStyle (its slot and gate can change any time), fires the
// bullets a shot still has to come, counts the air shots since he last stood on the ground and records his position
// (the air shot's speed is the last tick's movement).
static void pre_tick(uintptr_t pl) {
    if (!pl || at<int>(pl, pl::mPlayerID) != 1) return;
    apply_row();
    State& s = state_for(pl);
    ++s.ticks;
    s.air_gap += at<float>(pl, pl::m_delta_time);
    const MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    s.prev_x = s.last_x;
    s.prev_z = s.last_z;
    s.prev_dt = s.last_dt;
    s.last_x = pos.x;
    s.last_z = pos.z;
    s.last_dt = at<float>(pl, pl::m_delta_time);
    if (s.seen < 2) ++s.seen;
    fire_pending(pl, s);
    if (s.charge_fx && !(at<uint32_t>(pl, pl::mAtckId) == ground_move_id && at<int>(pl, pl::mBdyActionLowerNo) == 12 &&
                         at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_SPIN))
        charge_end(pl, s);                                              // the spin ended some other way: the glow goes
    if (!(at<uint32_t>(pl, pl::mActStat) & 0x2u)) {                    // not AST_JUMP: on the ground
        s.air_count = 0;
        s.air_gap = 1e9f;
    }
}

// After the commands ran: a press with the stick centred loses its command bit, so the row doesn't run (the neutral
// shot plays instead); a dodge on the ground takes the grab button's bit away from the neutral shot, which shares it
// with the air shot. The readout counts the ticks the dodge command fired on the local Nero.
static void post_command_tick(uintptr_t pl) {
    if (cmd_bit < 0 || !pl || at<int>(pl, pl::mPlayerID) != 1) return;
    uint32_t& flags = at<uint32_t>(pl, 0x2184 + 4 * (cmd_bit >> 5));   // mCmdFlag
    const uint32_t bit = 1u << (cmd_bit & 31);
    if (!(flags & bit)) return;
    const int d = stick_dir(pl);
    if (d < 0) { flags &= ~bit; return; }
    if (air_cmd_bit >= 0 && !(at<uint32_t>(pl, pl::mActStat) & 0x2u))
        at<uint32_t>(pl, 0x2184 + 4 * (air_cmd_bit >> 5)) &= ~(1u << (air_cmd_bit & 31));
    if (pl == (uintptr_t)devil4_sdk::get_local_player()) ++g_cmd_seen;
}

} // namespace gunslinger

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only. Everything the moves are lives in namespace gunslinger above.
// ---------------------------------------------------------------------------------------------------------------
using namespace gunslinger;

std::optional<std::string> NeroGunslinger::on_initialize() {
    using namespace input_commands;
    // the rows: COMMAND attr, cancel words as Gerb's / FreeRide's (C_COMMAND open, or C_ALL); the dodges ground only
    // (atckAs 1), the air shot air only (2)
    const uint32_t row[14]     = { 2, move_id,     7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    const uint32_t air_row[14] = { 2, air_move_id, 7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 2, 0x03000001, 0, 0, 0, 0x07000005 };
    const uint32_t ground_row[14] = { 2, ground_move_id, 7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    g_row_marker = MoveTable::add_row(0, row);
    g_air_row_marker = MoveTable::add_row(0, air_row);
    g_ground_row_marker = MoveTable::add_row(0, ground_row);
    move_registry::register_move(move_registry::NERO, move_id, &mod_enabled, &gunslinger::move);   // the ID gate + Nero dispatch
    move_registry::register_move(move_registry::NERO, air_move_id, &mod_enabled, &gunslinger::move);
    move_registry::register_move(move_registry::NERO, ground_move_id, &mod_enabled, &gunslinger::move);
    // lock-on held, the stick leaning any way, the grab button within 2 frames; the direction is read when the dodge starts
    g_dodge_cmd.condition(CND_POSE).lever(LEVER_ANY, LEVER_TARGET_45, 2).button(BTN_STYLE);
    cmd_bit = next_free_bit(CHAR_NERO);
    if (cmd_bit < 0 || !add_for(CHAR_NERO, g_dodge_cmd.head(), cmd_bit, &mod_enabled)) {
        spdlog::error("Gunslinger: no free command bit for the dodges (Nero's {}-{} are all taken)", FIRST_FREE_BIT, LAST_BIT);
        cmd_bit = -1;
    }
    g_air_cmd.button(BTN_STYLE);                 // the grab button: the air shot's row (air only) and the neutral shot's (ground)
    air_cmd_bit = next_free_bit(CHAR_NERO);
    if (air_cmd_bit < 0 || !add_for(CHAR_NERO, g_air_cmd.head(), air_cmd_bit, &mod_enabled)) {
        spdlog::error("Gunslinger: no free command bit for the air shot (Nero's {}-{} are all taken)", FIRST_FREE_BIT, LAST_BIT);
        air_cmd_bit = -1;
    }
    add_tick(&pre_tick);
    add_post_command_tick(&post_command_tick);
    {   // our bullets' hits made harder, and forgotten when freed (uPlNeroShl000 only: no other class uses this vtable)
        const uintptr_t hit = (uintptr_t)&shl_hit, dtor = (uintptr_t)&shl_dtor;
        install_patch_absolute(kShl000Vtbl + 0x94, patch_shl_hit, (const char*)&hit, 4);
        install_patch_absolute(kShl000Vtbl + 0x00, patch_shl_dtor, (const char*)&dtor, 4);
    }
    if (!input_commands::hook_installed) {     // one installer: whichever mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("Gunslinger: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init Gunslinger (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void NeroGunslinger::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;        // debug page
    if (ImGui::Checkbox(_("Gunslinger"), &mod_enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("Nero's Gunslinger style moves, while the Gunslinger style is picked (Nero styles; without that mod they work in any "
                  "style). On the ground the dodge shots: lock-on + a stick direction + grab (the Snatch / Gerb button); Nero dodges back, "
                  "left, forward or right (the stick against the target) with the clip's own root motion and fires Blue Rose on that "
                  "direction's shot frame. Grab on the ground without them: the neutral shot, Nero fans the hammer; hold it to spin and "
                  "charge. In the air the air shot: grab, and Nero fires while keeping his momentum. Needs motion\\pl000\\gunslinger.lmt."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("Gunslinger");
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {
        const uintptr_t P = (uintptr_t)pl;
        ImGui::Text(_("File: %s | %s"), lmt_file_exists(kLmtPath) ? "found" : "missing", motion_registry::describe(lmt_set, P).c_str());
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();
        ImGui::SameLine();
        help_marker(_("Reads motion\\pl000\\gunslinger.lmt from disk again: export from Albam, press this, and the next move plays the "
                      "new clip."));
        ImGui::Text("command bits %d / %d (fired %u) | dodges %u air %u shots %u hard hits %u charge shots %u | style %d, row style %d | atckId %02X phase %d frame %.1f spin %.0f | %s",
                    cmd_bit, air_cmd_bit, g_cmd_seen, g_dodges, g_air_shots, g_shots, g_power_hits, g_charge_shots, nero_style::current(P),
                    (int)nero_style::gunslinger_condition(), at<uint32_t>(P, pl::mAtckId), at<int>(P, pl::mBdyActionLowerPhase),
                    at<float>(P, pl::mMotionFrame0), g_spin_frames, g_last);
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void NeroGunslinger::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("gunslinger").value_or(false);
    apply_row();
    register_lmt();
}

void NeroGunslinger::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gunslinger", mod_enabled);
}
