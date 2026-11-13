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
int   lmt_set          = -1;
char  lmt_path[128]    = "motion\\pl000\\gunslinger";
int   ctn_bank         = 0;
int   ctn_slot         = 5;            // Gerb's: no cancels of its own until cancel_frame opens them all
bool  root_motion      = true;
float blend_frames     = 6.0f;
bool  face_target      = true;
int   bullets          = 3;
float bullet_interval  = 10.0f;         // the vanilla shot's two barrels
bool  pad_rumble       = true;
int   cmd_bit          = -1;
BulletPower shot_power  = { 2, 0, -1 };   // Twosome Time's close-range level
BulletPower final_power = { 3, 5, 11 };
Shake final_shake     = { 60, 5 };      // well under the lightest preset (150)
Shake neutral_shake   = { 0, 4 };       // it fires fast and is pressed again and again
int   heavy_sound      = 1;
bool  heavy_sound_all  = true;
int   neutral_heavy_sound = 0;
ShotMove dodges[DIR_COUNT] = {
    { true,  0, 22.0f, -1.0f, -1.0f, 33.0f, 0, true, 31.0f },   // back (made at 48 fps): the landing at 32.5 (39 frames, the travel ends at 31)
    { true,  1,  8.0f, -1.0f, -1.0f, 38.0f, 0, true, 39.0f },   // left: 41 frames, travelling to 39
    { true,  3, 10.0f, -1.0f, -1.0f, 35.0f, 0, true, 34.0f },   // forward (made at 48 fps): 45 frames, the travel ends at 34
    { true,  2,  8.0f, -1.0f, -1.0f, 38.0f, 0, true, 40.0f },   // right: 41 frames, travelling to 40
};
const char* const dir_names[DIR_COUNT] = { "Back", "Left", "Forward", "Right" };
ShotMove air_shots[AIR_STEPS] = {
    { true,  5,  7.0f,  4.0f, 12.0f, 22.0f },   // 31 frames
    { true,  4, 13.0f, 10.0f, 17.0f, 20.0f },   // 24 frames; the arm kicks on 13-14
    { true,  7, 20.0f, 20.0f, 28.0f, 30.0f, 3 },   // a charge shot on 20 (the user)
};
float air_momentum[AIR_STEPS] = { 0.5f, 1.0f, 1.0f };   // the second keeps all of it (the user)
ShotMove ground_shot   = { true, 6, 0.0f, 6.0f, 14.0f, 15.0f };   // 19 frames (made at 48 fps); the next shot queues from the second shot
int   fan_shots        = 3;
float fan_frames[FAN_MAX] = { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f, 30.0f };
int   fan_bullets      = 1;
Spin  spin             = { true, 8, 14.0f, 0.0f, 4.0f, 0.0f, { 10.0f, 25.0f, 45.0f }, true };   // after the third shot (12), at the neutral shot's Next move at
float air_gravity      = -1.1f;         // Gerb's
float air_max_fall     = 30.0f;
float air_lift         = 12.0f;         // with Gerb's gravity about 65 cm up, back down by the clip's end (24 frames)
float air_lift_decay   = 0.75f;
bool  air_buffer       = true;
float air_chain_grace  = 12.0f;
AirKick air_kick[AIR_STEPS] = { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 6.0f, 10.0f } };   // the charge shot kicks Nero back and up
float air_kick_decay   = 0.9f;          // about 60 cm of push in all
int   air_cmd_bit      = -1;

namespace {

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
    a.mAttackLvI = std::clamp(p.level, 0, 4);
    a.mHitStopTimer = std::max(p.hit_stop, 0);
    if (p.hit_se >= 0) a.mHitSE = p.hit_se;
    ++g_power_hits;
}

uintptr_t __fastcall shl_dtor(uintptr_t shell, uintptr_t edx, uint32_t flags) {
    if (OurBullet* b = our_bullet(shell)) b->shell = 0;
    return ((uintptr_t(__fastcall*)(uintptr_t, uintptr_t, uint32_t))fn_shl000_dtor)(shell, edx, flags);
}

// One Blue Rose bullet; the shot's first also brings the flash, the gun's recoil, the rumble and the sound, as the
// vanilla shot fires its first barrel. `last` = the move's last bullet; `last_shot` = it belongs to the move's last shot,
// which shakes the camera by `shake`; `heavy` = the charge shot level whose sound is layered on (0 none).
void fire(uintptr_t pl, bool first, bool last, bool last_shot, const Shake* shake = nullptr, int heavy = 0) {
    remember(((uintptr_t(__cdecl*)(int, uintptr_t))fn_shl000_create)(0, pl), last);
    if (!first) return;
    set_gun_effect(pl, 0);
    if (const uintptr_t gun = blue_rose(pl)) set_shot(gun);
    if (pad_rumble) pad_vibration(pl, 0x11);
    gun_se(pl, 0);
    if (heavy > 0 && (heavy_sound_all || last_shot)) gun_se(pl, 0, kBlueRoseShotId + (uint32_t)std::min(heavy, 3));
    if (last_shot && shake) camera_shake(*shake);
}

// A charge shot of `level` (1-3), as the vanilla one fires it: the charged bullet, its flash, the recoil, its sound and rumble.
void fire_charged(uintptr_t pl, int level, const Shake& shake) {
    level = std::clamp(level, 1, 3);
    ((uintptr_t(__cdecl*)(int, uintptr_t))fn_shl001_create)(level - 1, pl);
    set_gun_effect(pl, level);
    if (const uintptr_t gun = blue_rose(pl)) set_shot(gun);
    if (pad_rumble) pad_vibration(pl, 2 + 2 * level);
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

bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

constexpr int PHASE_ENTRY = 0;
constexpr int PHASE_DODGE = 1;
constexpr int PHASE_AIR   = 1;     // the air shot's (its own move id)
constexpr int PHASE_GROUND = 1;    // the neutral ground shot's (its own move id)
constexpr int PHASE_SPIN  = 2;     // the neutral shot held: the spin
constexpr int kAirTicks   = 4;     // ticks off the ground before the dodge turns into a fall (a ledge behind)

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
    float gravity = 0.0f;            // the air shot's: Nero's own when he brings one, else air_gravity
    int air_count = 0;               // air shots since he last stood on the ground (the lift shrinks with each)
    int air_step = 0;                // the air combo's step playing (or last played)
    int fan_next = 0;                // the neutral shot's next shot (fan_frames index)
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
void shoot(uintptr_t pl, State& s, int count, bool last_shot, const Shake& shake, int heavy) {
    fire(pl, true, last_shot && count <= 1, last_shot, &shake, heavy);
    ++g_shots;
    if (count > 1) {
        if (s.pending == 0) s.next_in = bullet_interval;
        s.pending += count - 1;
        s.pending_last = last_shot;
    }
}

// The move's shot once the clip reaches its frame (the dodges and air shots have one): true on the tick it fires.
// AfterImage's trail while the move's clip is before its after_image_until frame.
void after_images(uintptr_t pl, const ShotMove& m) {
    if (m.after_images && (m.after_image_until <= 0.0f || at<float>(pl, pl::mMotionFrame0) < m.after_image_until))
        after_image::trail_request(pl);
}

bool fire_due(uintptr_t pl, State& s, const ShotMove& m) {
    if (s.fired || at<float>(pl, pl::mMotionFrame0) < m.shot_frame) return false;
    s.fired = true;
    if (m.charge > 0) {
        fire_charged(pl, m.charge, final_shake);
        ++g_shots;
    } else {
        shoot(pl, s, bullets, true, final_shake, heavy_sound);
    }
    return true;
}

// The neutral shot's shots as the clip reaches their frames, each a full shot (flash, recoil, sound) with fan_bullets
// bullets.
void fire_fan(uintptr_t pl, State& s) {
    const float frame = at<float>(pl, pl::mMotionFrame0);
    const int shots = std::clamp(fan_shots, 1, FAN_MAX);
    while (s.fan_next < shots && frame >= fan_frames[s.fan_next]) {
        ++s.fan_next;
        shoot(pl, s, fan_bullets, s.fan_next == shots, neutral_shake, neutral_heavy_sound);
    }
}

// The pending bullets, bullet_interval apart (an interval of 0 sends them together), whatever Nero is doing.
void fire_pending(uintptr_t pl, State& s) {
    if (s.pending <= 0) return;
    s.next_in -= at<float>(pl, pl::m_delta_time);
    while (s.pending > 0 && s.next_in <= 0.0f) {
        fire(pl, false, s.pending_last && s.pending == 1, false);
        ++g_shots;
        --s.pending;
        s.next_in += bullet_interval;
    }
}

// The highest charge level the spin may reach: 3, or with learned_only the levels Nero has learned (level 1 always,
// 2 / 3 as the vanilla charge checks them).
int charge_cap(uintptr_t pl) {
    if (!spin.learned_only) return 3;
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
    while (s.charge_level < cap && s.spin_time >= spin.charge_frames[s.charge_level]) {
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
    d.body[motion_registry::NERO] = lmt_path;
    const int old = lmt_set;
    lmt_set = lmt_path[0] ? motion_registry::add(d) : -1;
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
// z zeroed, the drift rebuilt by calcSpeedXZ) carries on with it. Vertically he rises at least at the lift (air_lift,
// times air_lift_decay for each air shot before it since he landed), and gravity goes on adding to the fall as in his
// jump (his own acceleration when he brings one, else air_gravity); the clip's root motion is off. Landing ends it, and
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
        const bool chained = s.air_gap <= 1.0f + std::max(air_chain_grace, 0.0f) + 1e-3f;   // still running, or ended within the grace
        int step = chained ? s.air_step + 1 : 0;
        if (step >= AIR_STEPS || !air_shots[step].enabled) step = 0;
        s.air_step = step;
        const ShotMove& m = air_shots[step];
        const int motion = m.enabled && ready(pl) ? motion_registry::word(lmt_set, pl, m.block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "air shot %d: no motion: %s", step + 1, motion_registry::describe(lmt_set, pl).c_str());
            devil4_sdk::neutral_air_transition((uPlayer_old*)pl);
            return;
        }
        if (face_target) face_lock_target(pl);
        s.gravity = acc.y < 0.0f ? acc.y : air_gravity;
        const float lift = air_lift * std::pow(air_lift_decay, (float)s.air_count++);
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
        devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        request_motion(pl, motion, (uint32_t)A_NULL_TRANS_OFF, blend_frames);
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
        ShotMove windows = m;                                               // buffered: the press queued from the first frame
        if (air_buffer && windows.next_at >= 0.0f) windows.next_from = 0.0f;
        arm_cancel_script(s, pl, windows);
        open_cancel_gate(pl, windows);
        const float dt = at<float>(pl, pl::m_delta_time);
        after_images(pl, m);
        const float fade = std::pow(std::clamp(air_kick_decay, 0.0f, 1.0f), dt);
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
        if (vel.y > -air_max_fall)
            vel.y = std::max(vel.y + s.gravity * at<float>(pl, pl::m_delta_time), -air_max_fall);
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
        const int motion = ground_shot.enabled && ready(pl) ? motion_registry::word(lmt_set, pl, ground_shot.block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "neutral shot: no motion: %s", motion_registry::describe(lmt_set, pl).c_str());
            leave(pl);
            return;
        }
        if (face_target) face_lock_target(pl);
        hold_still(pl);
        devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        request_motion(pl, motion, root_motion ? 0u : (uint32_t)A_NULL_TRANS_OFF, blend_frames);
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
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        s.air_ticks = (coll && !at<bool>(coll, 0x1C)) ? s.air_ticks + 1 : 0;   // mLand
        if (s.air_ticks > kAirTicks) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
        if (!(at<uint32_t>(pl, pl::mBtnOn) & input_commands::BTN_STYLE)) s.held = false;
        if (s.held && spin.enabled && s.fan_next >= std::clamp(fan_shots, 1, FAN_MAX) &&
            at<float>(pl, pl::mMotionFrame0) >= spin.from_frame) {
            const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, spin.block) : -1;
            if (motion >= 0) {
                devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
                request_motion(pl, motion, root_motion ? 0u : (uint32_t)A_NULL_TRANS_OFF, spin.blend);
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
        const ShotMove windows = { true, spin.block, 0.0f, -1.0f, -1.0f, spin.cancel_frame };
        arm_cancel_script(s, pl, windows);
        open_cancel_gate(pl, windows);
        hold_still(pl);
        s.spin_time += at<float>(pl, pl::m_delta_time);
        g_spin_frames = s.spin_time;
        charge_update(pl, s);
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        s.air_ticks = (coll && !at<bool>(coll, 0x1C)) ? s.air_ticks + 1 : 0;   // mLand
        if (s.air_ticks > kAirTicks) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
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
            at<float>(pl, pl::mMotionFrame0) = std::max(spin.loop_from, 0.0f);
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
        if (d < 0 || !dodges[d].enabled) {                                  // post_command_tick keeps these out; a fallback
            snprintf(g_last, sizeof(g_last), "no dodge %s", d < 0 ? "with the stick centred" : dir_names[d]);
            leave(pl);
            return;
        }
        const int motion = ready(pl) ? motion_registry::word(lmt_set, pl, dodges[d].block) : -1;
        if (motion < 0) {                                                   // no LMT, or the block isn't in it
            snprintf(g_last, sizeof(g_last), "%s: no motion: %s", dir_names[d], motion_registry::describe(lmt_set, pl).c_str());
            leave(pl);
            return;
        }
        if (face_target) face_lock_target(pl);
        hold_still(pl);
        devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        request_motion(pl, motion, root_motion ? 0u : (uint32_t)A_NULL_TRANS_OFF, blend_frames);
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
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        s.air_ticks = (coll && !at<bool>(coll, 0x1C)) ? s.air_ticks + 1 : 0;   // mLand
        if (s.air_ticks > kAirTicks) { devil4_sdk::neutral_air_transition((uPlayer_old*)pl); return; }
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
        row->command.buffer = (uint32_t)(air_cmd_bit & 0xFF) | (mod_enabled && air_shots[0].enabled && air_cmd_bit >= 0 ? 0u : 0x100u);
        row->atckConditionStyle = nero_style::gunslinger_condition();
    }
    if (kAtckDefTbl* row = MoveTable::row(0, g_ground_row_marker)) {   // the air shot's command: the grab button alone
        row->command.buffer = (uint32_t)(air_cmd_bit & 0xFF) | (mod_enabled && ground_shot.enabled && air_cmd_bit >= 0 ? 0u : 0x100u);
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

// After the commands ran: a press toward a direction that is off loses its command bit, so the row doesn't run (the
// neutral shot plays instead); a dodge on the ground takes the grab button's bit away from the neutral shot, which
// shares it with the air shot. The readout counts the ticks the dodge command fired on the local Nero.
static void post_command_tick(uintptr_t pl) {
    if (cmd_bit < 0 || !pl || at<int>(pl, pl::mPlayerID) != 1) return;
    uint32_t& flags = at<uint32_t>(pl, 0x2184 + 4 * (cmd_bit >> 5));   // mCmdFlag
    const uint32_t bit = 1u << (cmd_bit & 31);
    if (!(flags & bit)) return;
    const int d = stick_dir(pl);
    if (d < 0 || !dodges[d].enabled) { flags &= ~bit; return; }
    if (air_cmd_bit >= 0 && !(at<uint32_t>(pl, pl::mActStat) & 0x2u))
        at<uint32_t>(pl, 0x2184 + 4 * (air_cmd_bit >> 5)) &= ~(1u << (air_cmd_bit & 31));
    if (pl == (uintptr_t)devil4_sdk::get_local_player()) ++g_cmd_seen;
}

} // namespace gunslinger

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only. Everything the moves are lives in namespace gunslinger above.
// ---------------------------------------------------------------------------------------------------------------
using namespace gunslinger;

static const char* const kDirKey[DIR_COUNT] = { "back", "left", "forward", "right" };   // config keys

// A move's clip block, shot frame (unless it has shot frames of its own) and cancel windows.
static void shot_fields(ShotMove& m, bool shot_frame = true) {
    ImGui::InputInt(_("Block"), &m.block);
    ImGui::SameLine();
    Mod::help_marker(_("The LMT block (the animation's slot in Albam's LMT list) this move plays."));
    if (shot_frame) {
        ImGui::SliderFloat(_("Shot frame"), &m.shot_frame, 0.0f, 60.0f);
        ImGui::SameLine();
        Mod::help_marker(_("The clip's frame (game frames, 60 per second, counted from the clip's first frame) Blue Rose fires on: the first "
                      "bullet with the muzzle flash, the gun's recoil and the sound, as the vanilla shot. The other bullets follow on their "
                      "own, even after the move is cancelled or ends."));
        ImGui::SliderInt(_("Charge level"), &m.charge, 0, 3);
        ImGui::SameLine();
        Mod::help_marker(_("0 = Blue Rose's plain shot. 1-3 = a charge shot of that level instead, as the vanilla charge shot fires it: one "
                      "charged bullet with its flash, sound and rumble. Its hits are the game's charge shot hits, so the oomph settings "
                      "don't change them."));
    }
    ImGui::SliderFloat(_("Next move from"), &m.next_from, -1.0f, 60.0f);
    ImGui::SameLine();
    Mod::help_marker(_("From this frame a press for the next move is remembered (the next Gunslinger move; command moves such as Streak "
                  "count too) and starts at the frame below. -1 = no early window."));
    ImGui::SliderFloat(_("Next move at"), &m.next_at, -1.0f, 60.0f);
    ImGui::SameLine();
    Mod::help_marker(_("From this frame the next move starts: a press remembered since the frame above at once, a new one when it comes. "
                  "Earlier than the cancel frame below, so moves chain before anything else can cancel. -1 = no early window."));
    ImGui::SliderFloat(_("Cancel frame"), &m.cancel_frame, -1.0f, 60.0f);
    ImGui::SameLine();
    Mod::help_marker(_("From this frame of the clip every cancel window (attacks, jump, gun, taunt) is open, so the move can be cancelled "
                  "by anything. -1 = only the borrowed script's windows and the next move's window."));
    ImGui::Checkbox(_("After images"), &m.after_images);
    ImGui::SameLine();
    Mod::help_marker(_("The move leaves after images (the AfterImage mod's trail, with its Mod moves trigger on) while it plays."));
    if (m.after_images) {
        ImGui::SliderFloat(_("After images until"), &m.after_image_until, 0.0f, 60.0f);
        ImGui::SameLine();
        Mod::help_marker(_("The clip's frame the after images stop on (the end of the dodge's travel, say). 0 = the whole clip."));
    }
}

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
                  "direction's shot frame. Grab on the ground without them: the neutral shot, Nero fans the hammer. In the air the air "
                  "shot: grab, and Nero fires while keeping his momentum. Needs motion\\pl000\\gunslinger.lmt."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("Gunslinger");
    if (ImGui::InputText(_("LMT"), lmt_path, sizeof(lmt_path), ImGuiInputTextFlags_EnterReturnsTrue)) register_lmt();
    ImGui::SameLine();
    help_marker(_("Game path of the LMT under nativePC, without .lmt; Enter applies it. The hook places it in a free motion bank "
                  "itself (shared with the other features that add animations)."));
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | %s"), lmt_file_exists(lmt_path) ? "found" : "missing", motion_registry::describe(lmt_set, (uintptr_t)pl).c_str());
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();
        ImGui::SameLine();
        help_marker(_("Reads the LMT from disk again: export from Albam, press this, and the next move plays the new clip."));
    }
    ImGui::InputInt(_("Borrowed .ctn bank"), &ctn_bank);
    ImGui::InputInt(_("Borrowed .ctn slot"), &ctn_slot);
    ImGui::SameLine();
    help_marker(_("A vanilla cancel script the moves run under (every motion needs one, or the game hangs on it). Each move's cancel "
                  "frame replaces it with one that opens every window at that frame."));
    ImGui::Checkbox(_("Root motion"), &root_motion);
    ImGui::SameLine();
    help_marker(_("On: the clip's root_motion bone moves Nero (animate the travel in Blender). Off: he dodges in place. The air shot "
                  "always ignores it: his momentum moves him there."));
    ImGui::SliderFloat(_("Blend frames"), &blend_frames, 0.0f, 15.0f);
    ImGui::SameLine();
    help_marker(_("Frames the body takes to blend from what it was doing into the move."));
    ImGui::Checkbox(_("Face the target"), &face_target);
    ImGui::SameLine();
    help_marker(_("Turn toward the lock-on target as a move starts, so the shot and the dodge line up with it. Turning doesn't "
                  "change the air shot's momentum."));
    ImGui::SliderInt(_("Bullets"), &bullets, 1, 8);
    ImGui::SameLine();
    help_marker(_("How many times the one shot hits: each bullet hits once. Only the first has the flash, recoil and sound, so it "
                  "still looks and sounds like one shot."));
    ImGui::SliderFloat(_("Bullet interval"), &bullet_interval, 0.0f, 20.0f);
    ImGui::SameLine();
    help_marker(_("Frames between the bullets. 0 sends them all on the shot frame; a few frames apart spaces the hits out "
                  "(the vanilla shot's two barrels are 5 apart)."));
    ImGui::Checkbox(_("Pad rumble"), &pad_rumble);
    ImGui::SameLine();
    help_marker(_("The vanilla shot's pad vibration on the first bullet."));
    ImGui::SeparatorText(_("Oomph"));
    ImGui::SameLine();
    help_marker(_("How hard these moves' bullets hit. Only the Gunslinger moves' bullets change; Nero's normal Blue Rose shots stay "
                  "vanilla."));
    for (int i = 0; i < 2; ++i) {
        BulletPower& p = i ? final_power : shot_power;
        ImGui::PushID(i);
        ImGui::TextUnformatted(i ? _("Last bullet of a move") : _("Every bullet"));
        ImGui::SameLine();
        help_marker(i ? _("The last bullet of a move's last shot (the dodge's, the air shot's, the neutral shot's third): its finisher.")
                      : _("Every bullet the Gunslinger moves fire, but the last."));
        ImGui::Indent(lineIndent);
        ImGui::SliderInt(_("Stopping power"), &p.level, 1, 4);
        ImGui::SameLine();
        help_marker(_("The interrupt level of its hits. Each enemy flinches (and hovers in the air) only from hits of a level its build "
                      "doesn't shrug off: Blue Rose's normal shot is 1, Dante's Twosome Time 2, the charge shots 2-4. Higher makes heavier "
                      "enemies flinch and stay up."));
        ImGui::SliderInt(_("Hit stop"), &p.hit_stop, 0, 15);
        ImGui::SameLine();
        help_marker(_("Frames the enemy freezes for when it hits, as melee hits do (Red Queen's Combo A: 3). Gun hits have none."));
        ImGui::SliderInt(_("Hit sound"), &p.hit_se, -1, 11);
        ImGui::SameLine();
        help_marker(_("The sound of the hit landing (Blue Rose's own: 9 the shot, 10 / 11 the charge shots, the heaviest). -1 = the "
                      "shot's own."));
        ImGui::Unindent(lineIndent);
        ImGui::PopID();
    }
    for (int i = 0; i < 2; ++i) {
        Shake& sh = i ? neutral_shake : final_shake;
        ImGui::PushID(100 + i);
        ImGui::TextUnformatted(i ? _("Shake: neutral shot") : _("Shake: dodges and air shots"));
        ImGui::SameLine();
        help_marker(i ? _("The camera shake on the neutral shot's last shot. It fires fast and is pressed again and again, so it has "
                          "none by default.")
                      : _("The camera shake on a dodge's or an air shot's shot."));
        ImGui::Indent(lineIndent);
        ImGui::SliderInt(_("Strength"), &sh.strength, 0, 200);
        ImGui::SameLine();
        help_marker(_("How far the screen moves. The game's lightest shake (the Buster grab's hit) is 150. 0 = no shake."));
        ImGui::SliderInt(_("Frames"), &sh.frames, 1, 30);
        ImGui::SameLine();
        help_marker(_("How long the shake lasts (60 frames a second)."));
        ImGui::Unindent(lineIndent);
        ImGui::PopID();
    }
    ImGui::SliderInt(_("Heavy sound"), &heavy_sound, 0, 3);
    ImGui::SameLine();
    help_marker(_("Layers a charge shot's firing sound (its level 1-3) over the shot's own, for more weight and bass. 0 = the plain "
                  "shot only."));
    ImGui::SliderInt(_("Neutral shot heavy sound"), &neutral_heavy_sound, 0, 3);
    ImGui::SameLine();
    help_marker(_("The same for the neutral shot. It fires fast, so 0 (the plain, lighter shot sound) by default."));
    ImGui::Checkbox(_("Heavy sound on every shot"), &heavy_sound_all);
    ImGui::SameLine();
    help_marker(_("On: every shot gets the heavy sound. Off: only a move's last shot."));
    ImGui::SeparatorText(_("Directions"));
    ImGui::SameLine();
    help_marker(_("One dodge per stick direction, measured against the lock-on target as the game does (forward / back within 45 degrees, "
                  "left / right between). Turn a direction on once its clip is in the LMT; a press toward one that is off does nothing."));
    for (int i = 0; i < DIR_COUNT; ++i) {
        ImGui::PushID(i);
        ImGui::Checkbox(dir_names[i], &dodges[i].enabled);
        ImGui::SameLine();
        help_marker(_("This direction's dodge is on: its block below has its clip."));
        if (dodges[i].enabled) {
            ImGui::Indent(lineIndent);
            shot_fields(dodges[i]);
            ImGui::Unindent(lineIndent);
        }
        ImGui::PopID();
    }
    ImGui::SeparatorText(_("Neutral shot"));
    ImGui::SameLine();
    help_marker(_("Grab on the ground without lock-on + a direction (or toward a direction that is off): Nero stands and fans the "
                  "hammer, a full shot (flash, recoil, sound) on each shot frame."));
    ImGui::PushID("ground");
    if (ImGui::Checkbox(_("Neutral shot"), &ground_shot.enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("The neutral shot is on: its block below has its clip. Off, grab on the ground without a direction does nothing in this "
                  "style."));
    if (ground_shot.enabled) {
        ImGui::Indent(lineIndent);
        shot_fields(ground_shot, false);
        ImGui::SliderInt(_("Shots"), &fan_shots, 1, FAN_MAX);
        ImGui::SameLine();
        help_marker(_("How many times Nero fires in the clip, one shot on each frame below."));
        for (int i = 0; i < std::clamp(fan_shots, 1, FAN_MAX); ++i) {
            char label[32];
            snprintf(label, sizeof(label), "Shot %d frame", i + 1);
            ImGui::SliderFloat(label, &fan_frames[i], 0.0f, 60.0f);
            ImGui::SameLine();
            help_marker(_("The clip's frame (game frames, 60 per second) this shot fires on: a full shot, muzzle flash, recoil and sound. "
                          "Keep them in order."));
        }
        ImGui::SliderInt(_("Bullets per shot"), &fan_bullets, 1, 8);
        ImGui::SameLine();
        help_marker(_("Bullets each shot sends, Bullet interval apart (1 = one hit a shot). The Bullets setting above is the other "
                      "moves'."));
        ImGui::Checkbox(_("Hold to spin"), &spin.enabled);
        ImGui::SameLine();
        help_marker(_("Keep the button held through the volley and Nero spins Blue Rose once it has fired, until the button is let go "
                      "(then he stands). Its clip goes in the block below. Off, the neutral shot always plays out."));
        if (spin.enabled) {
            ImGui::Indent(lineIndent);
            ImGui::InputInt(_("Spin block"), &spin.block);
            ImGui::SameLine();
            help_marker(_("The LMT block (the animation's slot in Albam's LMT list) of the spin."));
            ImGui::SliderFloat(_("Spin from"), &spin.from_frame, 0.0f, 60.0f);
            ImGui::SameLine();
            help_marker(_("The neutral shot's frame the spin starts on, the button held since the press. It waits for the volley's last "
                          "shot either way."));
            ImGui::SliderFloat(_("Loop from"), &spin.loop_from, 0.0f, 60.0f);
            ImGui::SameLine();
            help_marker(_("The spin clip's frame it goes back to when it reaches its end while the button is still held (0 = the whole "
                          "clip again). Smoother: give the clip a Loop Start in Albam's LMT panel, and the game loops it itself, blending "
                          "its last frame into the loop frame (end the clip one frame's step before the loop frame's pose); this "
                          "setting then isn't used. Here the clip stops on its last frame for a frame before jumping back."));
            ImGui::SliderFloat(_("Spin blend"), &spin.blend, 0.0f, 15.0f);
            ImGui::SameLine();
            help_marker(_("Frames the body takes to blend from the neutral shot into the spin."));
            ImGui::SliderFloat(_("Spin cancel frame"), &spin.cancel_frame, -1.0f, 60.0f);
            ImGui::SameLine();
            help_marker(_("From this frame of the spin every cancel window (attacks, jump, gun, taunt) is open, so anything can break it. "
                          "-1 = nothing does: only letting go of the button ends it."));
            for (int i = 0; i < 3; ++i) {
                char label[32];
                snprintf(label, sizeof(label), "Charge level %d at", i + 1);
                ImGui::SliderFloat(label, &spin.charge_frames[i], 0.0f, 120.0f);
                ImGui::SameLine();
                help_marker(_("Frames of spinning (60 a second, from the spin's start) this charge level takes. Letting go fires the vanilla "
                              "charge shot of the level reached, its own animation, bullet and flash; before level 1 Nero just stands. "
                              "Each level lights the vanilla charge glow and plays its sound. Keep them in order."));
            }
            ImGui::Checkbox(_("Learned levels only"), &spin.learned_only);
            ImGui::SameLine();
            help_marker(_("Levels 2 and 3 only once Nero has learned Charge Shot 2 and 3, as the vanilla charge. Off: every level, whatever "
                          "he has learned."));
            ImGui::Unindent(lineIndent);
        }
        ImGui::Unindent(lineIndent);
    }
    ImGui::PopID();
    ImGui::SeparatorText(_("Air shots"));
    ImGui::SameLine();
    help_marker(_("Grab in the air (any stick, lock-on or not): an air combo. A press in a shot's next-move window plays the next shot, "
                  "after the last the first again; any other press starts the first. Each shot: Nero hops, fires Blue Rose and keeps the "
                  "sideways speed he was moving at (from a jump, Gerb, Calibur ...), undecayed, as the air rev does for Rev Flying; the fall "
                  "after the shot carries on with it. His jump's gravity goes on. The clips' root motion is ignored. Landing ends a shot, or "
                  "the clip's end (the default fall)."));
    ImGui::PushID("air");
    for (int i = 0; i < AIR_STEPS; ++i) {
        ImGui::PushID(i);
        char label[32];
        snprintf(label, sizeof(label), "Air shot %d", i + 1);
        if (ImGui::Checkbox(label, &air_shots[i].enabled)) apply_row();
        ImGui::SameLine();
        help_marker(i == 0 ? _("The combo's first shot is on: its block below has its clip. Off, grab in the air does nothing in this style.")
                           : _("This step of the combo is on: its block below has its clip. Off, the combo ends at the step before it (the next "
                               "press starts the first shot again)."));
        if (air_shots[i].enabled) {
            ImGui::Indent(lineIndent);
            shot_fields(air_shots[i]);
            ImGui::SliderFloat(_("Momentum kept"), &air_momentum[i], 0.0f, 2.0f);
            ImGui::SameLine();
            help_marker(_("How much of the sideways speed Nero has as this shot starts it keeps, through the shot and the fall after it: "
                          "1 = all of it, 0.5 = half, 0 = he stops and drops straight down; above 1 speeds him up. A chained shot starts "
                          "from what the shot before it kept."));
            ImGui::SliderFloat(_("Knockback"), &air_kick[i].knockback, 0.0f, 20.0f);
            ImGui::SameLine();
            help_marker(_("On the shot frame Nero is pushed away from where he faces at this speed (cm per frame), the recoil of a heavy "
                          "shot. The push fades by Knockback fade below; the momentum he had keeps going under it. 0 = no push."));
            ImGui::SliderFloat(_("Lift on shot"), &air_kick[i].lift, 0.0f, 30.0f);
            ImGui::SameLine();
            help_marker(_("On the shot frame Nero rises at least this fast (cm per frame); already rising faster, he keeps his speed. 10 "
                          "with the default gravity is about 45 cm up. 0 = none."));
            ImGui::Unindent(lineIndent);
        }
        ImGui::PopID();
    }
    {
        ImGui::Indent(lineIndent);
        ImGui::SliderFloat(_("Lift"), &air_lift, 0.0f, 40.0f);
        ImGui::SameLine();
        help_marker(_("The hop: Nero rises at least this fast (cm per frame) as the air shot starts; already rising faster, he keeps "
                      "his speed. 12 with the default gravity is about 65 cm up. 0 = no hop, only his own momentum."));
        ImGui::SliderFloat(_("Knockback fade"), &air_kick_decay, 0.0f, 1.0f);
        ImGui::SameLine();
        help_marker(_("How much of a shot's knockback is left after each frame: 0.9 = a tenth less every frame (about 10 times the "
                      "knockback speed in cm in all), 1 = it never fades, 0 = a single frame."));
        ImGui::Checkbox(_("Buffer the next shot"), &air_buffer);
        ImGui::SameLine();
        help_marker(_("A press any time during an air shot is remembered and plays the combo's next step at that shot's Next move at "
                      "frame, so the shots buffer into each other (each shot's Next move from is then not used). Off, a press only counts "
                      "from Next move from."));
        ImGui::SliderFloat(_("Combo grace"), &air_chain_grace, 0.0f, 30.0f);
        ImGui::SameLine();
        help_marker(_("Frames after an air shot's clip ends, while Nero falls, that a press still plays the combo's next step instead of "
                      "the first. 0 = only a press during the shot chains. Landing always ends the combo."));
        ImGui::SliderFloat(_("Lift per repeat"), &air_lift_decay, 0.0f, 1.0f);
        ImGui::SameLine();
        help_marker(_("Each further air shot before Nero lands lifts this much of the one before (0.75 = a quarter less each time), "
                      "so chained air shots don't keep him up for ever. 1 = the same every time."));
        ImGui::SliderFloat(_("Gravity"), &air_gravity, -5.0f, 0.0f);
        ImGui::SameLine();
        help_marker(_("Added to the fall speed every frame of the air shot when Nero brings no gravity of his own (his jump's is used "
                      "when he has one)."));
        ImGui::SliderFloat(_("Max fall speed"), &air_max_fall, 1.0f, 60.0f);
        ImGui::SameLine();
        help_marker(_("Gravity stops adding once he falls this fast (cm per frame). A faster fall he came in with is kept."));
        ImGui::Unindent(lineIndent);
    }
    ImGui::PopID();
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {
        const uintptr_t P = (uintptr_t)pl;
        ImGui::Text("command bits %d / %d (fired %u) | dodges %u air %u shots %u hard hits %u charge shots %u | style %d, row style %d | atckId %02X phase %d frame %.1f spin %.0f | %s",
                    cmd_bit, air_cmd_bit, g_cmd_seen, g_dodges, g_air_shots, g_shots, g_power_hits, g_charge_shots, nero_style::current(P),
                    (int)nero_style::gunslinger_condition(), at<uint32_t>(P, pl::mAtckId), at<int>(P, pl::mBdyActionLowerPhase),
                    at<float>(P, pl::mMotionFrame0), g_spin_frames, g_last);
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

static void load_after_images(const utility::Config& cfg, const std::string& k, ShotMove& m) {
    m.after_images      = cfg.get<bool>(k + "_after_images").value_or(m.after_images);
    m.after_image_until = cfg.get<float>(k + "_after_images_until").value_or(m.after_image_until);
}

static void save_after_images(utility::Config& cfg, const std::string& k, const ShotMove& m) {
    cfg.set<bool>(k + "_after_images", m.after_images);
    cfg.set<float>(k + "_after_images_until", m.after_image_until);
}

void NeroGunslinger::on_config_load(const utility::Config& cfg) {
    mod_enabled      = cfg.get<bool>("gunslinger").value_or(false);
    if (auto p = cfg.get("gunslinger_lmt")) strcpy_s(lmt_path, p->c_str());
    ctn_bank         = cfg.get<int>("gunslinger_ctn_bank").value_or(ctn_bank);
    ctn_slot         = cfg.get<int>("gunslinger_ctn_slot").value_or(ctn_slot);
    root_motion      = cfg.get<bool>("gunslinger_root_motion").value_or(root_motion);
    blend_frames     = cfg.get<float>("gunslinger_blend_frames").value_or(blend_frames);
    face_target      = cfg.get<bool>("gunslinger_face_target").value_or(face_target);
    bullets          = std::clamp(cfg.get<int>("gunslinger_bullets").value_or(bullets), 1, 8);
    bullet_interval  = cfg.get<float>("gunslinger_bullet_gap").value_or(bullet_interval);
    pad_rumble       = cfg.get<bool>("gunslinger_pad_rumble").value_or(pad_rumble);
    for (int i = 0; i < 2; ++i) {
        BulletPower& p = i ? final_power : shot_power;
        const std::string k = i ? "gunslinger_final_" : "gunslinger_shot_";
        p.level    = std::clamp(cfg.get<int>(k + "level").value_or(p.level), 1, 4);
        p.hit_stop = std::clamp(cfg.get<int>(k + "hit_stop").value_or(p.hit_stop), 0, 15);
        p.hit_se   = std::clamp(cfg.get<int>(k + "hit_se").value_or(p.hit_se), -1, 11);
    }
    for (int i = 0; i < 2; ++i) {
        Shake& sh = i ? neutral_shake : final_shake;
        const std::string k = i ? "gunslinger_neutral_shake_" : "gunslinger_shake_";
        sh.strength = std::clamp(cfg.get<int>(k + "strength").value_or(sh.strength), 0, 200);
        sh.frames   = std::clamp(cfg.get<int>(k + "frames").value_or(sh.frames), 1, 30);
    }
    heavy_sound      = std::clamp(cfg.get<int>("gunslinger_heavy_sound").value_or(heavy_sound), 0, 3);
    heavy_sound_all  = cfg.get<bool>("gunslinger_heavy_sound_all").value_or(heavy_sound_all);
    neutral_heavy_sound = std::clamp(cfg.get<int>("gunslinger_neutral_heavy_sound").value_or(neutral_heavy_sound), 0, 3);
    // the back dodge's keys from before the directions
    dodges[DIR_BACK].block        = cfg.get<int>("gunslinger_back_dodge_block").value_or(dodges[DIR_BACK].block);
    dodges[DIR_BACK].shot_frame   = cfg.get<float>("gunslinger_shot_frame").value_or(dodges[DIR_BACK].shot_frame);
    dodges[DIR_BACK].cancel_frame = cfg.get<float>("gunslinger_cancel_frame").value_or(dodges[DIR_BACK].cancel_frame);
    for (int i = 0; i < DIR_COUNT; ++i) {
        const std::string k = std::string("gunslinger_") + kDirKey[i];
        ShotMove& m = dodges[i];
        m.enabled      = cfg.get<bool>(k + "_on").value_or(m.enabled);
        m.block        = cfg.get<int>(k + "_block").value_or(m.block);
        m.shot_frame   = cfg.get<float>(k + "_shot_frame").value_or(m.shot_frame);
        m.next_from    = cfg.get<float>(k + "_next_from").value_or(m.next_from);
        m.next_at      = cfg.get<float>(k + "_next_at").value_or(m.next_at);
        m.cancel_frame = cfg.get<float>(k + "_cancel_frame").value_or(m.cancel_frame);
        m.charge       = std::clamp(cfg.get<int>(k + "_charge").value_or(m.charge), 0, 3);
        load_after_images(cfg, k, m);
    }
    {   // the single air shot's keys from before the combo: that clip is now the second step
        ShotMove& m = air_shots[1];
        m.enabled      = cfg.get<bool>("gunslinger_air_on").value_or(m.enabled);
        m.block        = cfg.get<int>("gunslinger_air_block").value_or(m.block);
        m.shot_frame   = cfg.get<float>("gunslinger_air_shot_frame").value_or(m.shot_frame);
        m.next_from    = cfg.get<float>("gunslinger_air_next_from").value_or(m.next_from);
        m.next_at      = cfg.get<float>("gunslinger_air_next_at").value_or(m.next_at);
        m.cancel_frame = cfg.get<float>("gunslinger_air_cancel_frame").value_or(m.cancel_frame);
    }
    for (int i = 0; i < AIR_STEPS; ++i) {
        const std::string k = "gunslinger_air" + std::to_string(i + 1);
        ShotMove& m = air_shots[i];
        m.enabled      = cfg.get<bool>(k + "_on").value_or(m.enabled);
        m.block        = cfg.get<int>(k + "_block").value_or(m.block);
        m.shot_frame   = cfg.get<float>(k + "_shot_frame").value_or(m.shot_frame);
        m.next_from    = cfg.get<float>(k + "_next_from").value_or(m.next_from);
        m.next_at      = cfg.get<float>(k + "_next_at").value_or(m.next_at);
        m.cancel_frame = cfg.get<float>(k + "_cancel_frame").value_or(m.cancel_frame);
        air_momentum[i] = cfg.get<float>(k + "_momentum").value_or(air_momentum[i]);
        m.charge       = std::clamp(cfg.get<int>(k + "_charge").value_or(m.charge), 0, 3);
        load_after_images(cfg, k, m);
        air_kick[i].knockback = cfg.get<float>(k + "_knockback").value_or(air_kick[i].knockback);
        air_kick[i].lift      = cfg.get<float>(k + "_kick_lift").value_or(air_kick[i].lift);
    }
    air_kick_decay = cfg.get<float>("gunslinger_air_kick_decay").value_or(air_kick_decay);
    {
        ShotMove& m = ground_shot;
        m.enabled      = cfg.get<bool>("gunslinger_ground_on").value_or(m.enabled);
        m.block        = cfg.get<int>("gunslinger_ground_block").value_or(m.block);
        m.next_from    = cfg.get<float>("gunslinger_ground_next_from").value_or(m.next_from);
        m.next_at      = cfg.get<float>("gunslinger_ground_next_at").value_or(m.next_at);
        m.cancel_frame = cfg.get<float>("gunslinger_ground_cancel_frame").value_or(m.cancel_frame);
        load_after_images(cfg, "gunslinger_ground", m);
        fan_shots      = std::clamp(cfg.get<int>("gunslinger_ground_shots").value_or(fan_shots), 1, FAN_MAX);
        for (int i = 0; i < FAN_MAX; ++i)
            fan_frames[i] = cfg.get<float>("gunslinger_ground_shot" + std::to_string(i + 1)).value_or(fan_frames[i]);
        fan_bullets    = std::clamp(cfg.get<int>("gunslinger_ground_bullets").value_or(fan_bullets), 1, 8);
    }
    spin.enabled      = cfg.get<bool>("gunslinger_spin_on").value_or(spin.enabled);
    spin.block        = cfg.get<int>("gunslinger_spin_block").value_or(spin.block);
    spin.from_frame   = cfg.get<float>("gunslinger_spin_from").value_or(spin.from_frame);
    spin.loop_from    = cfg.get<float>("gunslinger_spin_loop_from").value_or(spin.loop_from);
    spin.blend        = cfg.get<float>("gunslinger_spin_blend").value_or(spin.blend);
    spin.cancel_frame = cfg.get<float>("gunslinger_spin_cancel_frame").value_or(spin.cancel_frame);
    for (int i = 0; i < 3; ++i)
        spin.charge_frames[i] = cfg.get<float>("gunslinger_spin_charge" + std::to_string(i + 1)).value_or(spin.charge_frames[i]);
    spin.learned_only = cfg.get<bool>("gunslinger_spin_learned_only").value_or(spin.learned_only);
    air_gravity           = cfg.get<float>("gunslinger_air_gravity").value_or(air_gravity);
    air_max_fall          = cfg.get<float>("gunslinger_air_max_fall").value_or(air_max_fall);
    air_lift              = cfg.get<float>("gunslinger_air_lift").value_or(air_lift);
    air_lift_decay        = cfg.get<float>("gunslinger_air_lift_decay").value_or(air_lift_decay);
    air_chain_grace       = cfg.get<float>("gunslinger_air_chain_grace").value_or(air_chain_grace);
    air_buffer            = cfg.get<bool>("gunslinger_air_buffer").value_or(air_buffer);
    apply_row();
    register_lmt();
}

void NeroGunslinger::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gunslinger", mod_enabled);
    cfg.set("gunslinger_lmt", std::string(lmt_path));
    cfg.set<int>("gunslinger_ctn_bank", ctn_bank);
    cfg.set<int>("gunslinger_ctn_slot", ctn_slot);
    cfg.set<bool>("gunslinger_root_motion", root_motion);
    cfg.set<float>("gunslinger_blend_frames", blend_frames);
    cfg.set<bool>("gunslinger_face_target", face_target);
    cfg.set<int>("gunslinger_bullets", bullets);
    cfg.set<float>("gunslinger_bullet_gap", bullet_interval);
    cfg.set<bool>("gunslinger_pad_rumble", pad_rumble);
    for (int i = 0; i < 2; ++i) {
        const BulletPower& p = i ? final_power : shot_power;
        const std::string k = i ? "gunslinger_final_" : "gunslinger_shot_";
        cfg.set<int>(k + "level", p.level);
        cfg.set<int>(k + "hit_stop", p.hit_stop);
        cfg.set<int>(k + "hit_se", p.hit_se);
    }
    for (int i = 0; i < 2; ++i) {
        const Shake& sh = i ? neutral_shake : final_shake;
        const std::string k = i ? "gunslinger_neutral_shake_" : "gunslinger_shake_";
        cfg.set<int>(k + "strength", sh.strength);
        cfg.set<int>(k + "frames", sh.frames);
    }
    cfg.set<int>("gunslinger_heavy_sound", heavy_sound);
    cfg.set<bool>("gunslinger_heavy_sound_all", heavy_sound_all);
    cfg.set<int>("gunslinger_neutral_heavy_sound", neutral_heavy_sound);
    for (int i = 0; i < DIR_COUNT; ++i) {
        const std::string k = std::string("gunslinger_") + kDirKey[i];
        cfg.set<bool>(k + "_on", dodges[i].enabled);
        cfg.set<int>(k + "_block", dodges[i].block);
        cfg.set<float>(k + "_shot_frame", dodges[i].shot_frame);
        cfg.set<float>(k + "_next_from", dodges[i].next_from);
        cfg.set<float>(k + "_next_at", dodges[i].next_at);
        cfg.set<float>(k + "_cancel_frame", dodges[i].cancel_frame);
        cfg.set<int>(k + "_charge", dodges[i].charge);
        save_after_images(cfg, k, dodges[i]);
    }
    for (int i = 0; i < AIR_STEPS; ++i) {
        const std::string k = "gunslinger_air" + std::to_string(i + 1);
        cfg.set<bool>(k + "_on", air_shots[i].enabled);
        cfg.set<int>(k + "_block", air_shots[i].block);
        cfg.set<float>(k + "_shot_frame", air_shots[i].shot_frame);
        cfg.set<float>(k + "_next_from", air_shots[i].next_from);
        cfg.set<float>(k + "_next_at", air_shots[i].next_at);
        cfg.set<float>(k + "_cancel_frame", air_shots[i].cancel_frame);
        cfg.set<float>(k + "_momentum", air_momentum[i]);
        cfg.set<int>(k + "_charge", air_shots[i].charge);
        save_after_images(cfg, k, air_shots[i]);
        cfg.set<float>(k + "_knockback", air_kick[i].knockback);
        cfg.set<float>(k + "_kick_lift", air_kick[i].lift);
    }
    cfg.set<float>("gunslinger_air_kick_decay", air_kick_decay);
    cfg.set<bool>("gunslinger_ground_on", ground_shot.enabled);
    cfg.set<int>("gunslinger_ground_block", ground_shot.block);
    cfg.set<float>("gunslinger_ground_next_from", ground_shot.next_from);
    cfg.set<float>("gunslinger_ground_next_at", ground_shot.next_at);
    cfg.set<float>("gunslinger_ground_cancel_frame", ground_shot.cancel_frame);
    save_after_images(cfg, "gunslinger_ground", ground_shot);
    cfg.set<int>("gunslinger_ground_shots", fan_shots);
    for (int i = 0; i < FAN_MAX; ++i) cfg.set<float>("gunslinger_ground_shot" + std::to_string(i + 1), fan_frames[i]);
    cfg.set<int>("gunslinger_ground_bullets", fan_bullets);
    cfg.set<bool>("gunslinger_spin_on", spin.enabled);
    cfg.set<int>("gunslinger_spin_block", spin.block);
    cfg.set<float>("gunslinger_spin_from", spin.from_frame);
    cfg.set<float>("gunslinger_spin_loop_from", spin.loop_from);
    cfg.set<float>("gunslinger_spin_blend", spin.blend);
    cfg.set<float>("gunslinger_spin_cancel_frame", spin.cancel_frame);
    for (int i = 0; i < 3; ++i) cfg.set<float>("gunslinger_spin_charge" + std::to_string(i + 1), spin.charge_frames[i]);
    cfg.set<bool>("gunslinger_spin_learned_only", spin.learned_only);
    cfg.set<float>("gunslinger_air_gravity", air_gravity);
    cfg.set<float>("gunslinger_air_max_fall", air_max_fall);
    cfg.set<float>("gunslinger_air_lift", air_lift);
    cfg.set<float>("gunslinger_air_lift_decay", air_lift_decay);
    cfg.set<float>("gunslinger_air_chain_grace", air_chain_grace);
    cfg.set<bool>("gunslinger_air_buffer", air_buffer);
}
