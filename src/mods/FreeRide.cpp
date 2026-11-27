#include "FreeRide.hpp"
#include "MoveTable.hpp"
#include "FreeRideShell.hpp"
#include "UpperGun.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/Collision.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MotionRegistry.hpp"
#include "../sdk/Rollback.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace free_ride {

bool mod_enabled  = false;
bool need_forward = true;
bool exceed_fill  = true;
int  hit_mode     = HIT_DAMAGE;

namespace {

// ---------------------------------------------------------------------------------------------------------------
// The tuning (cm, cm per frame, frames).
// ---------------------------------------------------------------------------------------------------------------
// the input
constexpr float mount_range  = 350.0f;    // level distance to the lock-on target (an enemy; bosses aren't ridden)
constexpr float mount_height = 120.0f;    // the target's height difference allowed either way
constexpr float flat_frames  = 6.0f;      // the mount gate: frames the target has to lie flat on the floor first
constexpr float flat_gap     = 12.0f;     // cm above the floor under it an enemy may be and still lie on it
// the ride
constexpr float mount_frames = 14.0f;     // the hop onto the enemy
constexpr float hop_height   = 70.0f;     // the hop's apex above the straight line
constexpr float ride_height  = 45.0f;     // the player's feet above the floor under the enemy
constexpr float ride_speed   = 24.0f;
constexpr float turn_rate    = 6.0f;      // degrees per frame the board turns toward the stick
constexpr float ride_frames  = 240.0f;    // the ride's length (then a hop off)
constexpr float step_down    = 120.0f;    // how far below the board the floor may drop before the ride leaves the ledge
constexpr float hold_frames  = 4.0f;      // the enemy's hit-stop timer is kept at least this high (the mount; the ride in HIT_STOP)
constexpr float up_grace     = 20.0f;     // frames off AS_DOWN before the ride hops off (the hits couldn't keep it down)
constexpr float exceed_frames = 80.0f;    // Nero: frames of riding per Exceed stock (the three over the 240-frame ride)
// the gun (UpperGun reads them through the host)
constexpr float gun_hold_frames = 10.0f;  // frames the gun stays up after the last shot before the arms go back to the ride
constexpr float gun_blend_min   = 6.0f;   // frames: the least blend a shot's upper / aim request gets
constexpr float gun_back_blend  = 8.0f;   // frames the arms take back into the ride's motion after a shot
// the exits: melee hands off to the launcher (the Stinger -> Million Stab way); without one it hops off
constexpr uint32_t nero_finisher  = 0x1F; // uPlayerNero::moveAtckRedQueenHiTime (High Roller; the move picks its jump follow-up itself)
constexpr uint32_t dante_finisher = 0x0B; // with Rebellion: uPlayerDante::moveAtckRevellionHiTime; Gilgamesh / Lucifer: none
constexpr float finish_back = 90.0f;      // cm the player is put back from the enemy before the launcher, on the floor
// the LMT, per character (game paths without extension); the mount always plays the game's enemy-step hop
const char* const lmt_path[2] = { "motion\\pl000\\freeride", "motion\\pl006\\freeride" };
constexpr int slot_start = 0;             // the start-up on the board, between the mount and the ride, until its block ends
constexpr int slot_ride  = 1;             // the ride (looping)
constexpr int ctn_bank = 0, ctn_slot = 5; // the .ctn script borrowed for the LMT motions: the fall motion's, as Gerb borrows (no windows of its own)
// the stand-ins without the LMT (bank 0 motions both characters share)
constexpr int   vanilla_mount_motion = 0x11;   // the enemy step (uPlayer::moveActEmJump 0x827890)
constexpr int   vanilla_ride_motion  = 5;      // the jump (uPlayer::moveActJumpBegin 0x8221D0), held still
constexpr float vanilla_ride_frame   = 12.0f;  // the frame it is held at
// sounds: a request id on one of the player's sound banks
enum SeBank : int { SE_CORE = 0, SE_FOOT = 2 };   // the character's bank, the common footstep bank
struct SeRef { int bank; int id; };
constexpr SeRef se_mount_foot      = { SE_FOOT, 0 };                           // the hop onto the enemy: FT_Jump
constexpr SeRef se_mount_whoosh[2] = { { SE_CORE, 32 }, { SE_CORE, 30 } };     // with it: Nero Whoosh01, Dante mantowoosh (the coat)
constexpr SeRef se_land            = { SE_FOOT, 1 };                           // landing on the enemy: FT_Land
constexpr SeRef se_ride            = { SE_FOOT, 11 };                          // the ride, repeated: FT_Sliding
constexpr float se_ride_interval   = 20.0f;    // frames between two sliding sounds

int lmt_set = -1;                         // the motion registry's handle of our set (a body LMT per character)

// ---------------------------------------------------------------------------------------------------------------
// Game functions, offsets and helpers.
// ---------------------------------------------------------------------------------------------------------------

// the enemy (uActor / uEnemy, DX9)
namespace em {
    constexpr uintptr_t mUnitState      = 0x0004;   // cUnit: & 7 = 1 / 2 while the unit lives (uActor::move's test on mpAttackSender)
    constexpr uintptr_t mPos            = 0x0030;
    constexpr uintptr_t mQuat           = 0x0040;   // MT quaternion x, y, z, w
    constexpr uintptr_t mActorStatus    = 0x0EA8;   // AS_DOWN 0x4, AS_DEAD 0x80
    constexpr uintptr_t mVel            = 0x0EC0;
    constexpr uintptr_t mEmHitStopTimer = 0x1434;   // uEnemy: frames; uEnemy::setHitStop (vtbl 52) writes it
    constexpr uintptr_t mEmHitStop      = 0x1438;   // uEnemy: set by uEnemy::move while the timer runs; every main skips its routine then
}
// uActor::ACTOR_STATUS: 0x1 ground, 0x2 air (uEm000Base::interruptMessage 0x558090 picks the air flinch by it), 0x4 down,
// 0x8 damage, 0x80 dead. Each enemy class writes its own mix in its down states (em000Base: 0xC while falling, 4 in the
// down loop, 0xC getting up; em010 / em029 / em030 8; em017's loop 0), so only AS_DOWN and AS_AIR are read here.
constexpr uint32_t AS_AIR = 0x2, AS_DOWN = 0x4, AS_DEAD = 0x80;

// cLockonTarget (sdk/uActor.hpp): +8 mpModel, +0x10 mPos (16 bytes), +0x20 mType (LOCKON_MODEL_TYPE: 0 enemy, 1 boss, 2 enemy shell,
// 3 object ...).
constexpr uintptr_t kTargetModel = 0x08, kTargetType = 0x20;
constexpr uintptr_t kStyleKind = 0x156C;         // uPlayer::mStyle.kind (kStyleParam): STYLE_KIND_LONG 1 = Gunslinger
constexpr uint32_t  STYLE_KIND_LONG = 1;
constexpr uintptr_t kCurrentSword = 0x1DB4;      // uPlayer: mWeaponCtrl.mShortId, Dante's chosen sword, 4 Rebellion 5 Gilgamesh 6 Lucifer (what checkAtckWeapon
                                                 // tests the rows' atckConditionWp against; the held one, +0x1DC0, follows at the next action start)
constexpr uintptr_t kCmdFlagWords = 0x2184;      // mCmdFlag[3]

// uPlayerNero::moveActAtckL 0x7F9CD0 / uPlayerDante's attack dispatch 0x7CD190 (thiscall, no stack arguments): the
// state function of the current atckId, run the same tick as the hand-off as the vanilla chains do.
constexpr uintptr_t fn_dispatch[2] = { 0x007F9CD0, 0x007CD190 };
constexpr uintptr_t fn_setActionAtck = 0x007AAC80;   // uPlayer::setActionAtck (this@eax, atckId@ecx, body on the stack, callee pops)
constexpr uintptr_t fn_setAction = 0x007AA030;          // uPlayer::setAction (this@ecx, int act; vtbl 101): 25 = the enemy step, uPlayer::moveActEmJump 0x827890
constexpr uintptr_t kEmJumpTimer = 0x1FF4, kWJumpTimer = 0x1FF8;   // uPlayer: checkEmJumpBegin 0x804970 sets them from params 79 / 80 with action 25
// uCollisionMgr (pl::mCollMgr): sCollisionGame::checkPush 0x47BDA0 (and checkAttackObj / checkGrabObj) skips a pair of
// groups when either manager's mppIgnoreModel[16] holds the other's mpDstModel, which is the actor itself. uEm013 puts the
// player there while it carries him (preMain 0x5DE230 -> setIgnoreModel 0x50C1A0; clearIgnoreModel 0x50C1F0 otherwise).
constexpr uintptr_t kIgnoreModels = 0xE8, kIgnoreModelNum = 0x128;
constexpr int kIgnoreModelMax = 16;
// Nero's Exceed gauge: uPlayerNero::cOverDriveGuage, embedded at uPlayerNero+0xCCE4 (RE note Vibed\RE\nero_exceed_gauge.md).
// +0x04 stocks (0-3), +0x10 the bar (0 .. +0x44, what the HUD draws: uCockpitNero::checkPtr), +0x24 the rev button's
// pressure this tick (the game's own revving), +0x50 the effect parent (null = the gauge isn't set up; addStock's gate).
constexpr uintptr_t kOdGauge = 0xCCE4;
constexpr uintptr_t kOdStock = 0x04, kOdValue = 0x10, kOdPress = 0x24, kOdMax = 0x44, kOdParent = 0x50;
constexpr uintptr_t fn_odAddStock = 0x007EA340;   // cOverDriveGuage::addStock (this@edi; int count, bool se, bool seAlt; retn 0Ch)
constexpr uintptr_t fn_setActionFootwork = 0x007AA2C0;  // uPlayer::setActionFootwork (this@esi, float hokan, char isMotSet; callee pops)
// Weapon and style changes live in checkBeforeAction (vtbl 147), which uPlayer::main runs only while the cancel gate is on; the
// ride keeps it off, so it runs the two checks itself. uPlayer::checkWeaponChange 0x8045B0 (this@esi): the change buttons
// (mBtnTrg 0x80 short / 0x100 long) advance mWeaponCtrl's mShortId / mLongId past their cooldowns. uPlayerDante::checkStyleChange
// 0x7B6C20 (this@esi; edi is passed on to getProperSaveData, which ignores it): the d-pad -> mStyle with the 12-frame cooldown,
// flash and sound; it only starts the style pose (action 35) from footwork, so during the ride the switch is instant.
// cWeaponCtrl::update 0x7B0F10 (after the post-action tick) swaps the models only while mIsChangeEnable is set, which
// moveAction 0x821552 sets unless mActionNo == 12 (isWeaponChange): an attack defers the swap to its end, so the ride
// (attack 12) sets the flag itself once the arms are free of a shot.
constexpr uintptr_t fn_checkWeaponChange = 0x008045B0;
constexpr uintptr_t fn_checkStyleChange  = 0x007B6C20;
constexpr uintptr_t kWpIsChangeEnable = 0x1DC8;    // mWeaponCtrl.mIsChangeEnable (cWeaponCtrl +0x40; +0x2C / +0x30 the chosen ids, +0x38 / +0x3C the held)
constexpr uintptr_t fn_changeLongWeapon = 0x007B1200;   // cWeaponCtrl::changeLongWeapon (this@edi = &mWeaponCtrl +0x1D88): the next equipped long weapon, no cooldown
constexpr uintptr_t fn_changeShortWeapon = 0x007B1240;  // cWeaponCtrl::changeShortWeapon (this@edi), likewise
constexpr uintptr_t kWeaponCtrl = 0x1D88;
constexpr uint32_t  WP_PANDORA = 8;

// how a ground move ends (Stinger, Million Stab: setActionFootwork(10.0, 1))
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

void set_action_atck(uintptr_t pl, uint32_t id, int body) {
    uintptr_t fn = fn_setActionAtck;
    __asm {
        pushad
        mov eax, pl
        mov ecx, id
        push body
        call fn
        popad
    }
}

// One Exceed stock with the game's light and sound (what a rev reaching the top of the bar does). Nothing past 3.
void od_add_stock(uintptr_t gauge) {
    uintptr_t fn = fn_odAddStock;
    __asm {
        pushad
        mov edi, gauge
        push 0          // seAlt
        push 1          // se
        push 1          // count
        call fn
        popad
    }
}

// The weapon-change buttons, as checkBeforeAction would read them (the chosen weapon advances; the swap waits for the flag).
void check_weapon_change(uintptr_t pl) {
    uintptr_t fn = fn_checkWeaponChange;
    __asm {
        pushad
        mov esi, pl
        call fn
        popad
    }
}

// The next equipped long / short weapon, as L2 / R2 pick them (the swap follows once mIsChangeEnable is set).
void change_weapon(uintptr_t pl, uintptr_t fn) {
    uintptr_t ctrl = pl + kWeaponCtrl;
    __asm {
        pushad
        mov edi, ctrl
        call fn
        popad
    }
}
void change_long_weapon(uintptr_t pl)  { change_weapon(pl, fn_changeLongWeapon); }
void change_short_weapon(uintptr_t pl) { change_weapon(pl, fn_changeShortWeapon); }

// Pandora is disabled for the ride (its shots and Omen are whole-body moves that go wrong there): no mount with it in hand
// (can_mount), and on the board the weapon cycle skips it: chosen Pandora -> the next gun.
bool pandora_in_hand(uintptr_t pl) {
    return at<int>(pl, pl::mPlayerID) != 1 && (at<uint32_t>(pl, upper_gun::kWpLongId) == WP_PANDORA || at<uint32_t>(pl, upper_gun::kWpEnableLongId) == WP_PANDORA);   // Dante
}
void put_pandora_away(uintptr_t pl) {
    for (int i = 0; i < 4 && at<uint32_t>(pl, upper_gun::kWpLongId) == WP_PANDORA; ++i) change_long_weapon(pl);
}

// Dante's style buttons, as checkBeforeAction would read them.
void check_style_change(uintptr_t pl) {
    uintptr_t fn = fn_checkStyleChange;
    __asm {
        pushad
        mov esi, pl
        mov edi, pl
        call fn
        popad
    }
}

void set_action(uintptr_t pl, int act) { ((void(__fastcall*)(uintptr_t, int, int))fn_setAction)(pl, 0, act); }

void run_dispatch(uintptr_t pl, int character) {
    ((void(__fastcall*)(uintptr_t, int))fn_dispatch[character])(pl, 0);
}

// getLockOnTarget (vtbl 170, thiscall (this, a2, a3, exclude), retn 0Ch) as the game's lever check asks it: (0, 0, 0)
uintptr_t lock_target(uintptr_t pl) {
    const uintptr_t fn = (*(uintptr_t**)pl)[170];
    return ((uintptr_t(__fastcall*)(uintptr_t, int, int, int, uintptr_t))fn)(pl, 0, 0, 0, 0);
}

int character_of(uintptr_t pl) { return at<int>(pl, pl::mPlayerID) == 1 ? NERO : DANTE; }

void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer_old*)pl, attr, speed, start, interp);
}

void face(uintptr_t pl, float yaw) {
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

inline glm::vec3 dir_of(float yaw) { return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw)); }
inline glm::vec3 vec(const MtVector3& v) { return glm::vec3(v.x, v.y, v.z); }

// Turns `from` toward `to` by at most `step` radians.
float turn_toward(float from, float to, float step) {
    float d = std::remainder(to - from, 6.2831853f);
    if (d > step) d = step; else if (d < -step) d = -step;
    return from + d;
}

// The stage floor under (x, z): a line from `from_y` down to `to_y` in the ground channel.
bool floor_at(float x, float z, float from_y, float to_y, float* out_y) {
    collision::CollParam param = collision::line_param(collision::COLL_GROUND);
    MtLineSegment seg{ { x, from_y, z }, { x, to_y, z } };
    collision::LineResult hit{};
    if (!collision::find_intersection(&param, &seg, false, &hit)) return false;
    *out_y = hit.hitPos.y;
    return true;
}

bool enemy_alive(uintptr_t e) {
    if (!e) return false;
    const uint32_t state = at<uint32_t>(e, em::mUnitState) & 7;
    return (state == 1 || state == 2) && !(at<uint32_t>(e, em::mActorStatus) & AS_DEAD);
}

// The enemy stands still in its own pose while its hit-stop timer runs (uEnemy::move -> mEmHitStop -> every main skips
// its routine); topping the timer up each tick is the whole hold. Stopping is the release.
void still(uintptr_t e) {
    MtVector3& v = at<MtVector3>(e, em::mVel);
    v.x = v.y = v.z = 0.0f;
}
void hold(uintptr_t e) {
    float& t = at<float>(e, em::mEmHitStopTimer);
    if (t < hold_frames) t = hold_frames;
    still(e);
}
void release(uintptr_t e) {
    if (enemy_alive(e)) at<float>(e, em::mEmHitStopTimer) = 0.0f;
}

// Lying flat on the floor: knocked down (AS_DOWN) but not in the air (AS_AIR: the launch and the fall of a knockdown carry
// AS_DOWN too, and the hit stop would freeze the enemy in mid-air), not moving up or down, and within `flat_gap` of the floor
// under it. ride_target asks for this for `flat_frames` running (pre_tick counts), so a bounce doesn't count either.
bool enemy_flat(uintptr_t e) {
    const uint32_t st = at<uint32_t>(e, em::mActorStatus);
    if (!(st & AS_DOWN) || (st & AS_AIR)) return false;
    if (std::fabs(at<MtVector3>(e, em::mVel).y) > 1.0f) return false;
    const glm::vec3 p = vec(at<MtVector3>(e, em::mPos));
    float fy;
    if (!floor_at(p.x, p.z, p.y + 50.0f, p.y - 100.0f, &fy)) return false;
    return std::fabs(p.y - fy) < flat_gap;
}

// ---- sounds ----
// sDevil4Sound::requestSe 0xA47AA0 (stdcall: sSoundN::mpInstance, the rSoundRequest bank, request id, owner, 16-byte position,
// 1 play / 2 stop, 1, sSoundN::soundPositionUpdateCb) queues a request on a .sreq bank, as uPlayer::cSECtrl::request 0x4A6E20
// does with the player's banks (uPlayer +0x1D60 = cSECtrl::mpSndReqRsrc[3], filled by cSECtrl::setupRsrc 0x7B17C0: [0]
// pl00X_core, [1] pl00X_style00) and the rMotionSe (.msse) events do with the bank each entry names, e.g. the common footstep
// bank sound\\se\\player\\common\\foot_default (0 FT_Jump, 1 FT_Land, 2-5 walk / run, 6-10 steps, 11 FT_Sliding, 14-15 DOWN; the
// names are the .msse entries' mComment). uEffectVFR::stopSoundSe 0xA45F50 is the same request with mode 2.
constexpr uintptr_t fn_requestSe = 0x00A47AA0, fn_sePositionCb = 0x00486640, kSoundInstance = 0x00E559D8;
constexpr uintptr_t kSeBanks = 0x1D60;
constexpr uintptr_t kDtiSoundRequest = 0x00EAD7E0;   // rSoundRequest::DTI
const char* const kFootBankPath = "sound\\se\\player\\common\\foot_default";
void* g_foot_bank = nullptr;   // our reference to the footstep bank (sResource::create: the game's cached copy), kept for the session

uintptr_t se_bank(uintptr_t pl, int bank) {
    if (bank == SE_FOOT) {
        if (!g_foot_bank) g_foot_bank = devil4_sdk::get_stuff_from_files((MtDTI*)kDtiSoundRequest, (char*)kFootBankPath, 1);
        return (uintptr_t)g_foot_bank;
    }
    if (bank < 0 || bank > 2) return 0;
    return at<uintptr_t>(pl, kSeBanks + 4 * bank);
}
void se_request(uintptr_t pl, const SeRef& se, int mode) {
    const uintptr_t inst = *(uintptr_t*)kSoundInstance;
    const uintptr_t rsrc = se_bank(pl, se.bank);
    if (!inst || !rsrc) return;
    const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const bool play = mode == 1;
    ((void(__stdcall*)(uintptr_t, uintptr_t, int, uintptr_t, const float*, int, int, uintptr_t))fn_requestSe)(
        inst, rsrc, se.id, pl, play ? (const float*)&at<MtVector3>(pl, pl::mPos) : zero, mode, play ? 1 : 0, play ? fn_sePositionCb : 0);
}
void play_se(uintptr_t pl, const SeRef& se) { se_request(pl, se, 1); }
void stop_se(uintptr_t pl, const SeRef& se) { se_request(pl, se, 2); }

// The push capsules: the player's against the enemy's shoved the rider off and the two chased each other, so for the
// ride the enemy sits on the player's collision manager's ignore list (push, attack and grab checks all skip the pair;
// the finisher runs once the entry is gone). The game never clears the list by itself: every way out takes it out.
void ignore_enemy(uintptr_t pl, uintptr_t e, bool on) {
    const uintptr_t mgr = at<uintptr_t>(pl, pl::mCollMgr);
    if (!mgr || !e) return;
    int& num = at<int>(mgr, kIgnoreModelNum);
    if (num < 0 || num > kIgnoreModelMax) return;
    uintptr_t* list = &at<uintptr_t>(mgr, kIgnoreModels);
    int i = 0;
    while (i < num && list[i] != e) ++i;
    if (on) {
        if (i < num) return;
        if (num >= kIgnoreModelMax) { spdlog::warn("FreeRide: the player's ignore list is full; the enemy's push stays on"); return; }
        list[num++] = e;
    } else if (i < num) {
        list[i] = list[num - 1];
        list[--num] = 0;
    }
}

void place_enemy(uintptr_t e, const glm::vec3& p, float yaw) {
    MtVector3& pos = at<MtVector3>(e, em::mPos);
    pos.x = p.x; pos.y = p.y; pos.z = p.z;
    float* q = &at<float>(e, em::mQuat);   // the enemy turns with the board
    q[0] = 0.0f; q[1] = std::sin(yaw * 0.5f); q[2] = 0.0f; q[3] = std::cos(yaw * 0.5f);
}

// Velocity in both of the game's models: mVel, and the XZ speed along mJump.moveAng that calcSpeedXZ rebuilds it from in
// the air (Gerb's note).
void set_velocity(uintptr_t pl, const glm::vec3& v) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    vel.x = v.x; vel.y = v.y; vel.z = v.z;
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    acc.x = acc.y = acc.z = 0.0f;
    const glm::vec3 xz(v.x, 0.0f, v.z);
    at<float>(pl, pl::mMoveVel) = glm::length(xz);
    at<float>(pl, pl::mMoveAcc) = 0.0f;
    if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = std::atan2(xz.x, xz.z);
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

void set_airborne(uintptr_t pl, bool air) {
    uint32_t& st = at<uint32_t>(pl, pl::mActStat);
    st = air ? ((st & ~0x1u) | 0x2u) : ((st & ~0x2u) | 0x1u);
}

// ---- the LMT, per character, through the motion registry: one set with a body file per character, merged into a
// free bank of the player's models by the registry, which hands out the motion words (never bank << 8 | block here) ----
bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

void register_lmt() {
    motion_registry::SetDesc d;
    d.owner = "FreeRide";
    d.body[motion_registry::NERO]  = lmt_path[NERO];
    d.body[motion_registry::DANTE] = lmt_path[DANTE];
    lmt_set = motion_registry::add(d);
}

// The word of one of our slots for this player, or -1 without the file (the stand-ins play). ready() builds the
// player's banks, so this runs from the player's own tick only (the move), never from on_frame or the GUI.
int motion_word(uintptr_t pl, int slot) {
    if (lmt_set < 0 || !motion_registry::ready(lmt_set, pl)) return -1;
    return motion_registry::word(lmt_set, pl, slot);
}

// ---- per player ----
constexpr int PHASE_ENTRY = 0, PHASE_MOUNT = 1, PHASE_RIDE = 2, PHASE_START = 3;   // START: on the board, the start-up block playing, before the ride
constexpr int kPlayerSlots = 8;

struct RideState {
    uintptr_t player = 0, enemy = 0;
    uint32_t  last_seen = 0;
    glm::vec3 start{0.0f};           // the player's position at the mount
    float     floor_y = 0.0f;        // the floor under the board, last found
    float     yaw = 0.0f;            // the board's direction
    float     t = 0.0f;              // frames into the mount / the ride
    bool      jump_held = false;     // the press that started the ride hasn't been let go yet (eaten until it is)
    bool      melee_held = false;
    bool      motion_ended = false;  // the start-up block reached its last frame (the game's frame-end flag, latched)
    bool      change_short = false;  // a weapon-change press under a shot, kept for the shot's end
    bool      change_long = false;
    float     exceed = 0.0f;         // Nero: frames of ride toward the next Exceed stock
    uint32_t  serial = 0;            // this ride's number: the hit shell ends when the player's ride isn't it any more
    float     up_frames = 0.0f;      // hit mode: frames the enemy has been off AS_DOWN
    int       ride_motion = -1;      // the motion on the legs during the ride (the arms go back to it after a shot)
    uint32_t  ride_attr = 0;
    float     ride_speed_mul = 1.0f; // its speed (the stand-in is held still: 0)
    float     ride_start = 0.0f;     // its start frame (the stand-in is held at vanilla_ride_frame)
    bool      ride_custom = false;   // a registry word (its .ctn is borrowed)
    bool      style_active = false;  // Dante: one of the game's Gunslinger moves has the body; the ride goes on from the post tick
    uint32_t  style_id = 0;          // the atckId it started with (readout)
    uintptr_t flat_enemy = 0;        // the lock-on target found lying flat, and for how many frames (pre_tick; ride_target's gate)
    float     flat_t = 0.0f;
    float     se_t = 0.0f;           // frames since the ride's last sliding sound
};
RideState g_rides[kPlayerSlots];

// The motion the state remembers for the ride (request_ride_motion plays it; the arms come back to it after a shot):
// a registry word (`custom`: its .ctn is borrowed) at speed 1 from frame 0, or the stand-in held still at its frame.
void set_ride_motion(RideState& s, int motion, float speed, float start, bool custom) {
    s.ride_motion = motion; s.ride_attr = (uint32_t)A_NULL_TRANS_OFF; s.ride_speed_mul = speed; s.ride_start = start; s.ride_custom = custom;
}
uint32_t g_ride_serial = 0;

// One slot per uPlayer seen, the least recently seen evicted (Gerb's table).
RideState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (RideState& s : g_rides) if (s.player == player) { s.last_seen = tick; return s; }
    RideState* victim = &g_rides[0];
    for (RideState& s : g_rides) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = RideState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

// The readout's look-up from the GUI thread: no eviction, no tick.
RideState* find_state(uintptr_t player) {
    for (RideState& s : g_rides) if (s.player == player) return &s;
    return nullptr;
}

bool riding(uintptr_t pl) {
    const uint32_t id = upper_gun::effective_atck_id(pl);   // the ride's id even while a shot's id is swapped in (the upper pass)
    return at<uint32_t>(pl, pl::mBdyActionLowerNo) == 12 && (id == nero_move_id || id == dante_move_id);
}

// The board goes on under one of the game's Gunslinger moves (the ride action is suspended, the post tick drives it).
bool style_riding(uintptr_t pl) {
    const RideState* s = find_state(pl);
    return s && s->style_active && s->enemy != 0;
}

// The enemy goes free: the ride sound stopped, the hit stop off, the push back, and the state rides nothing. Every way
// off the board ends here (the exits, and the pre-check tick after a ride cut short).
void let_go(uintptr_t pl, RideState& s) {
    stop_se(pl, se_ride);
    release(s.enemy);
    ignore_enemy(pl, s.enemy, false);
    s.enemy = 0;
    s.style_active = false;
}

// The target the ride would take this tick: the lock-on target's model when it is an enemy lying flat (enemy_flat), alive,
// within range.
uintptr_t ride_target_now(uintptr_t pl) {
    const uintptr_t t = lock_target(pl);
    if (!t) return 0;
    const uint32_t type = at<uint32_t>(t, kTargetType);
    if (type != 0) return 0;                            // enemies only (bosses are large and some don't lie down the same way)
    const uintptr_t e = at<uintptr_t>(t, kTargetModel);
    if (!enemy_alive(e) || !enemy_flat(e)) return 0;
    const glm::vec3 d = vec(at<MtVector3>(e, em::mPos)) - vec(at<MtVector3>(pl, pl::mPos));
    if (std::fabs(d.y) > mount_height || d.x * d.x + d.z * d.z > mount_range * mount_range) return 0;
    return e;
}

// ... and lying there for `flat_frames` running (pre_tick counts them): the moment it touches down between two bounces
// doesn't count.
uintptr_t ride_target(uintptr_t pl) {
    const uintptr_t e = ride_target_now(pl);
    if (!e) return 0;
    const RideState* s = find_state(pl);
    return (s && s->flat_enemy == e && s->flat_t >= flat_frames) ? e : 0;
}

input_commands::Command g_cmd;
int g_cmd_bit = -1;
uint32_t g_fired = 0, g_entries = 0, g_finishes = 0, g_hops = 0, g_style_moves = 0;   // readout
bool g_in_reach = false;   // readout: the local player's lock-on target could be ridden (set on the game thread)

// lock-on held, (the stick toward the target within 10 frames of) jump pressed. The game thread reads g_cmd's head and
// steps in place, so a rebuild (the GUI, a config load) writes the steps first and the count last, never a blank head.
void build_command() {
    using namespace input_commands;
    Command c;
    c.condition(CND_POSE);
    if (need_forward) c.lever(LEVER_FORWARD, LEVER_TARGET_45, 10);
    c.button(BTN_JUMP);
    for (int i = 0; i < Command::kMaxSteps; ++i) g_cmd.steps[i] = c.steps[i];
    g_cmd.head_.condition = c.head_.condition;
    g_cmd.num = c.num;
    g_cmd.head();
}

// The command fired for this player this tick: eat the jump press before checkJumpBegin (checkBeforeAction runs it
// ahead of checkAtck) takes it, so the row gets it. Riders: the press that started the ride is eaten until let go.
void pre_tick(uintptr_t pl) {
    if (!mod_enabled || !pl || g_cmd_bit < 0) return;
    {                                                   // how long the lock-on target has lain flat (ride_target's gate)
        RideState& s = state_for(pl);
        const uintptr_t e = ride_target_now(pl);
        if (e != s.flat_enemy) { s.flat_enemy = e; s.flat_t = 0.0f; }
        else if (e) s.flat_t += std::max(at<float>(pl, pl::m_delta_time), 0.01f);
    }
    if (pl == (uintptr_t)devil4_sdk::get_local_player()) g_in_reach = ride_target(pl) != 0;
    if ((at<uint32_t>(pl, kCmdFlagWords + 4 * (g_cmd_bit >> 5)) >> (g_cmd_bit & 31)) & 1) {
        at<uint32_t>(pl, pl::mBtnTrg) &= ~0x4u;
        at<bool>(pl, pl::mMotCnclJumpReserve) = false;
        ++g_fired;
        return;
    }
    if (!riding(pl) && !style_riding(pl)) {              // a ride cut short (hit out of it): the enemy goes free, the push comes back
        if (RideState* s = find_state(pl); s && s->enemy) let_go(pl, *s);
        return;
    }
    RideState& s = state_for(pl);
    if (s.style_active) return;                          // the game's move has the buttons (its own repeats and cancels)
    const uint32_t on = at<uint32_t>(pl, pl::mBtnOn);
    if (s.jump_held) {
        if (!(on & 0x4)) s.jump_held = false;
        else { at<uint32_t>(pl, pl::mBtnTrg) &= ~0x4u; at<bool>(pl, pl::mMotCnclJumpReserve) = false; }
    }
    if (s.melee_held) {
        if (!(on & 0x1)) s.melee_held = false;
        else at<uint32_t>(pl, pl::mBtnTrg) &= ~0x1u;
    }
    // The game's weapon / style checks live behind the cancel gate, which the ride keeps off, so they run here. Not under a
    // shot (the arms in it or held up after it): a gun chosen then would differ from the held one at the shot's last phase,
    // which the shot functions end with setActionFootwork(5, 1) instead of the upper pose (moveAtckEbonyIvoryShot 0x7D8412,
    // CoyoteAceShot too): a whole-body footwork over the ride, which cuts it short. The press is kept and the change made once
    // the arms are free; the swap follows in post_tick.
    if (upper_gun::shooting(pl)) {                       // (busy() is always true here: the ride's own upper half is attack 12)
        const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
        if (trg & 0x80)  s.change_short = true;
        if (trg & 0x100) s.change_long = true;
    } else {
        if (s.change_short) { change_short_weapon(pl); s.change_short = false; }
        if (s.change_long)  { change_long_weapon(pl);  s.change_long = false; }
        check_weapon_change(pl);
        if (character_of(pl) == DANTE) put_pandora_away(pl);
    }
    if (character_of(pl) == DANTE) check_style_change(pl);
}

// ---- Nero's Exceed gauge ----
// The ride revs the gauge by itself: the bar fills over `exceed_frames` and a stock is added at the top, over and over
// until the three stocks are in. The bar is written as an absolute value each tick (the game's update runs after the
// move and takes one frame of its release decay off it, which doesn't show), unless the player is revving by hand, whose
// own rev then shows. With three stocks the game holds the bar full itself. What's left in the bar at the dismount decays
// as after a released rev.
void fill_exceed(uintptr_t pl, RideState& s, float dt) {
    const uintptr_t g = pl + kOdGauge;
    if (!at<uintptr_t>(g, kOdParent) || at<int>(g, kOdStock) >= 3) return;
    s.exceed += dt;
    if (s.exceed >= exceed_frames) {
        s.exceed -= exceed_frames;
        od_add_stock(g);
        if (at<int>(g, kOdStock) >= 3) return;
    }
    if (at<float>(g, kOdPress) <= 0.0f) at<float>(g, kOdValue) = at<float>(g, kOdMax) * (s.exceed / exceed_frames);
}

// The ride's motion as the state remembers it (a registry word with the borrowed .ctn, or the stand-in held still), with
// `interp` frames of blend: the mount's hand-over, and the return from a Gunslinger move.
void request_ride_motion(uintptr_t pl, const RideState& s, float interp) {
    if (s.ride_motion < 0) return;
    if (s.ride_custom) devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);   // updateMotion clears the override after each use
    request_motion(pl, s.ride_motion, s.ride_attr, interp, s.ride_start, s.ride_speed_mul);
}

// ---- the exits ----
// Off the board with the game's own enemy step: action 25, what checkEmJumpBegin 0x804970 starts (the 0x11 hop toward the
// stick, or straight up, then the fall), with the timers it sets; the air rules take it from there.
void hop_off(uintptr_t pl, RideState& s) {
    upper_gun::end(pl);                 // the arms out of a shot, the words back, before the whole-body action
    let_go(pl, s);
    ++g_hops;
    set_velocity(pl, glm::vec3(0.0f));
    set_airborne(pl, true);
    const float* prm = at<float*>(pl, pl::mParamTbl0Data);
    at<float>(pl, kEmJumpTimer) = prm[79];
    at<float>(pl, kWJumpTimer)  = prm[80];
    set_action(pl, 25);
}

// Off the board onto the floor `finish_back` cm behind the enemy, facing it, standing: where a ground move takes over.
void dismount_to_floor(uintptr_t pl, RideState& s) {
    upper_gun::end(pl);
    const glm::vec3 p = vec(at<MtVector3>(s.enemy, em::mPos)) - dir_of(s.yaw) * finish_back;
    let_go(pl, s);
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    pos.x = p.x; pos.y = s.floor_y; pos.z = p.z;
    set_velocity(pl, glm::vec3(0.0f));
    set_airborne(pl, false);
    face(pl, s.yaw);
}

// The ride ends under a move that keeps the body (the enemy died or got up, a ledge, a cancel into another action): the
// enemy goes free where it is, the player stays in that action (standing in the air at ride height, so he falls).
void drop_ride(uintptr_t pl, RideState& s) { let_go(pl, s); }

// The launcher: the player back on the floor behind the enemy, then the Stinger -> Million Stab hand-off (setActionAtck
// with both halves, the new move's state function this same tick).
bool finish(uintptr_t pl, RideState& s, int c) {
    if (c == DANTE && at<uint32_t>(pl, kCurrentSword) != 4) return false;   // Dante's launcher is Rebellion's
    const uint32_t id = c == NERO ? nero_finisher : dante_finisher;
    dismount_to_floor(pl, s);
    ++g_finishes;
    set_action_atck(pl, id, -1);
    run_dispatch(pl, c);
    return true;
}

// Dante in Gunslinger: the style button does the held gun's style move (Twosome Time 68 for Ebony & Ivory, Fireworks 79 for
// the Coyote-A, Pandora's 88) ON the board. Those are whole-body moves (uPlayerDante::moveAtckStyleEbonyIvoryTwoSometime
// 0x7D8B90 / ...CoyoteAceNunchaku 0x7DA3A0 run their phases in the LOWER phase word mBdyAction[1].phase, request whole-body
// motions on controller 0 and hand off with setActionAtck(id, -1)), so unlike the plain shots they can't play on the upper
// half over the riding legs: instead the game's own row takes the body (atckAttr 4, the style button; uPlayer::kAtckDefTbl
// rows 69 / 87 / 93 of Dante's table 0xC3FEA0; its ground version, the current style and gun, unlocked and its extra
// condition through checkAtckEtcSub, started as checkAtckSub does: setActionAtck(id, -1) and the dispatch this tick), and
// the ride is suspended: `style_active`, with the board's motion, floor, enemy and hits kept up by the post-action tick
// (style_post) while the move's state function runs instead of ours. The stick aims the shots (setStylGunTwoSome reads it),
// so the board runs straight meanwhile. When the move ends (its last phase calls uPlayer::setActionFootwork 0x7AA2C0:
// action 1, or 13 with the gun held, mAtckId 0) the ride action and its motion come back (resume_ride). Honeycomb (69) and
// the repeats follow from the move itself (it chains with setActionAtck(68 / 69, -1), the lower body staying attack 12);
// a cancel into anything else (Gun Stinger / Backslide 81 / 82 dash away, a melee row, a jump, damage) ends the ride.
bool style_gun_move(uintptr_t pl, RideState& s) {
    if (at<uint32_t>(pl, kStyleKind) != STYLE_KIND_LONG) return false;
    const kAtckDefTbl* row = (const kAtckDefTbl*)HookDanteKADTbl;
    if (!row) return false;
    const uint32_t wp = at<uint32_t>(pl, upper_gun::kWpLongId);   // the chosen gun (a pending change applies as the move starts, as in the game)
    if (wp == WP_PANDORA) return false;                           // never on the board (put_pandora_away), Omen least of all
    int id = -1;
    for (; row->atckAttr != 3; ++row) {
        if (row->atckAttr != 4 || row->atckAs != 1) continue;              // a style-button row, its ground version
        if (row->command.atckCommandNo) continue;                          // disabled row (MoveTable's convention)
        if (row->atckConditionStyle != 0xFFFFFFFFu && row->atckConditionStyle != STYLE_KIND_LONG) continue;
        if (row->atckConditionWp != 0xFFFFFFFFu && row->atckConditionWp != wp) continue;
        if (!upper_gun::row_allowed(pl, row->atckId, row->ukn)) continue;
        id = (int)row->atckId;
        break;
    }
    if (id < 0) return false;
    upper_gun::end(pl);                 // a plain shot's upper action and saved words out of the way first
    ++g_style_moves;
    s.style_active = true;
    s.style_id = (uint32_t)id;
    set_action_atck(pl, (uint32_t)id, -1);   // phase 0 for its state function; its own cancel gate from here
    run_dispatch(pl, DANTE);            // the lower pass as checkAtckSub's tick would run it; the upper pass (the move itself) follows
    return true;
}

// A gun atckId the ride stays under: Dante's gun rows 60-90 (the style moves and what they chain into), less the two
// command moves that carry the player away (Backslide 81, Gun Stinger 82).
bool style_gun_id(uint32_t id) { return id >= 60 && id <= 90 && id != 81 && id != 82; }

// One tick of the board in PHASE_RIDE: the Exceed rev, the facing (steering toward the stick when `steer`), the motion
// along the floor, the hold / hits and the enemy under the player. false = the ride can't go on (a ledge, the enemy got up).
bool ride_tick(uintptr_t pl, RideState& s, int c, float dt, bool steer, float speed = ride_speed) {
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    if (c == NERO && exceed_fill) fill_exceed(pl, s, dt);

    if (steer && at<int>(pl, pl::mAnlgPow) != 0)            // steer: the board turns toward the stick
        s.yaw = turn_toward(s.yaw, at<float>(pl, pl::mAnlgRadian), glm::radians(turn_rate) * dt);
    face(pl, s.yaw);

    const glm::vec3 next = vec(pos) + dir_of(s.yaw) * speed * dt;
    float fy;
    if (!floor_at(next.x, next.z, next.y + 60.0f, next.y - ride_height - step_down, &fy)) return false;   // a ledge
    s.floor_y = fy;
    const glm::vec3 v(dir_of(s.yaw).x * speed, (fy + ride_height - pos.y) / dt, dir_of(s.yaw).z * speed);
    set_velocity(pl, v);
    if (hit_mode == HIT_DAMAGE) {
        still(s.enemy);                                     // the hits keep it down (FreeRideShell)
        if (at<uint32_t>(s.enemy, em::mActorStatus) & AS_DOWN) s.up_frames = 0.0f;
        else if ((s.up_frames += dt) > up_grace) return false;   // it got up: the hits don't interrupt it
    } else {
        hold(s.enemy);                                      // frozen in its pose by the hit stop
    }
    place_enemy(s.enemy, glm::vec3(next.x, fy, next.z), s.yaw);
    return true;
}

// The ride proper: its motion (the LMT's ride block from frame 0, else the stand-in held still) and PHASE_RIDE, with the
// timer at 0.
void begin_ride(uintptr_t pl, RideState& s) {
    s.t = 0.0f;
    s.se_t = 0.0f;
    play_se(pl, se_ride);
    if (const int ride = motion_word(pl, slot_ride); ride >= 0) {
        set_ride_motion(s, ride, 1.0f, 0.0f, true);
        request_ride_motion(pl, s, 10.0f);
    } else {
        set_ride_motion(s, vanilla_ride_motion, 0.0f, vanilla_ride_frame, false);
        request_ride_motion(pl, s, 10.0f);
    }
    at<int>(pl, pl::mBdyActionLowerPhase) = PHASE_RIDE;
}

// Back on the board after a Gunslinger move: the ride action with both halves (setActionAtck resets the phase word; the
// ride goes straight on in PHASE_RIDE, its timer where it was), the ride's motion over `gun_back_blend` frames (the
// footwork the move requested is overridden before updateMotion), the facing and the air status the ride keeps.
void resume_ride(uintptr_t pl, RideState& s, int c, float dt) {
    s.style_active = false;
    set_action_atck(pl, c == NERO ? nero_move_id : dante_move_id, -1);
    at<int>(pl, pl::mBdyActionLowerPhase) = PHASE_RIDE;
    set_airborne(pl, true);
    face(pl, s.yaw);
    request_ride_motion(pl, s, gun_back_blend);
    if (!ride_tick(pl, s, c, dt, false)) hop_off(pl, s);   // this tick's velocity (setActionFootwork zeroed it)
}

// After both halves' state functions while a Gunslinger move has the body: the board goes on; when the move is over the
// ride comes back, when something else took the body the ride is dropped.
void style_post(uintptr_t pl, RideState& s) {
    const int c = character_of(pl);
    const float dt = std::max(at<float>(pl, pl::m_delta_time), 0.01f);
    if (!enemy_alive(s.enemy)) { drop_ride(pl, s); return; }
    s.t += dt;
    const uint32_t lower = at<uint32_t>(pl, pl::mBdyActionLowerNo);
    const uint32_t id = at<uint32_t>(pl, pl::mAtckId);
    if (lower == 12 && style_gun_id(id)) {                   // the move (or what it chained into) is still running
        if (!ride_tick(pl, s, c, dt, false)) drop_ride(pl, s);
        return;
    }
    const int action = at<int>(pl, upper_gun::kActionNo);
    if (action != 1 && action != 13) { drop_ride(pl, s); return; }   // a jump, damage, a melee row, a dash: the ride is over
    if (s.t >= ride_frames) { hop_off(pl, s); return; }   // the ride ran out under the move
    resume_ride(pl, s, c, dt);
}

// The post-action tick: the board under a Gunslinger move, and a weapon change chosen during the ride applied this tick
// (cWeaponCtrl::update runs after this and only swaps with the flag set, which moveAction withholds from attack 12), once the
// arms are out of a shot (a swap under the shot's motion would pull its weapon away; the choice keeps until then).
void post_tick(uintptr_t pl) {
    if (!mod_enabled || !pl) return;
    RideState* sp = find_state(pl);
    if (!sp) return;
    if (sp->style_active) style_post(pl, *sp);
    if (riding(pl) && !upper_gun::shooting(pl)) at<bool>(pl, kWpIsChangeEnable) = true;
}

void move_impl(uintptr_t pl) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    RideState& s = state_for(pl);
    const int c = character_of(pl);
    const float dt = std::max(at<float>(pl, pl::m_delta_time), 0.01f);
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);

    switch (phase) {
    case PHASE_ENTRY: {
        ++g_entries;
        const uintptr_t e = ride_target(pl);
        if (!e) { set_action_footwork(pl, 10.0f, 1); return; }    // the target got up or went away since the press
        s.enemy = e;
        s.start = vec(pos);
        s.t = 0.0f;
        s.jump_held = (at<uint32_t>(pl, pl::mBtnOn) & 0x4) != 0;
        s.melee_held = (at<uint32_t>(pl, pl::mBtnOn) & 0x1) != 0;
        s.exceed = 0.0f;
        const glm::vec3 ep = vec(at<MtVector3>(e, em::mPos));
        const glm::vec3 to = ep - s.start;
        s.yaw = (to.x * to.x + to.z * to.z > 1.0f) ? std::atan2(to.x, to.z) : at<float>(pl, pl::mRotateNow);
        if (!floor_at(ep.x, ep.z, ep.y + 100.0f, ep.y - 300.0f, &s.floor_y)) s.floor_y = ep.y;
        face(pl, s.yaw);
        hold(e);
        ignore_enemy(pl, e, true);
        s.motion_ended = false;
        s.change_short = s.change_long = false;
        request_motion(pl, vanilla_mount_motion, (uint32_t)A_NULL_TRANS_OFF, 3.0f, 0.0f, 1.0f);   // the game's hop
        set_airborne(pl, true);
        play_se(pl, se_mount_foot);
        play_se(pl, se_mount_whoosh[c == NERO ? 0 : 1]);
        phase = PHASE_MOUNT;
        return;
    }
    case PHASE_MOUNT: {
        if (!enemy_alive(s.enemy)) { hop_off(pl, s); return; }
        hold(s.enemy);
        // a straight line to the board with a parabola on top, sampled where the player should be after this tick
        const glm::vec3 ep = vec(at<MtVector3>(s.enemy, em::mPos));
        const glm::vec3 end(ep.x, s.floor_y + ride_height, ep.z);
        const float n = std::max(mount_frames, 1.0f);
        s.t += dt;
        const float k = std::min(s.t / n, 1.0f);
        const glm::vec3 want = s.start + (end - s.start) * k + glm::vec3(0.0f, hop_height * 4.0f * k * (1.0f - k), 0.0f);
        set_velocity(pl, (want - vec(pos)) / dt);
        if (k < 1.0f) return;
        play_se(pl, se_land);               // on the board
        s.t = 0.0f;
        s.serial = ++g_ride_serial;
        s.up_frames = 0.0f;
        if (hit_mode == HIT_DAMAGE) {       // the hits hold it from here: its own down flinch, not the hit stop
            release(s.enemy);
            spawn_ride_shell(pl, s.enemy, s.serial);
        }
        if (const int start = motion_word(pl, slot_start); start >= 0) {   // the start-up on the board first (no stand-in: no file = none)
            devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
            request_motion(pl, start, (uint32_t)A_NULL_TRANS_OFF, 3.0f, 0.0f, 1.0f);
            set_ride_motion(s, start, 1.0f, 0.0f, true);
            s.motion_ended = false;
            phase = PHASE_START;
            return;
        }
        begin_ride(pl, s);
        return;
    }
    case PHASE_START: {
        // On the board, the start-up block playing: the board holds its place over the enemy (floor, hits and the hold as in
        // the ride, no travel, no steering, no shots: the arms are in the block). It ends when the block does (the game's
        // lower-body frame-end flag, read from the second tick on: the first may still carry the mount's); a looping block
        // that never ends gives way after 300 frames. Jump and melee leave as from the ride.
        if (!enemy_alive(s.enemy)) { hop_off(pl, s); return; }
        s.t += dt;
        const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
        if (!s.jump_held && (trg & 0x4)) { hop_off(pl, s); return; }
        if (!s.melee_held && (trg & 0x1)) {
            if (finish(pl, s, c)) return;
            hop_off(pl, s);
            return;
        }
        if (s.t >= 2.0f && at<bool>(pl, pl::mMotionFrameEnd0)) s.motion_ended = true;
        const bool over = s.motion_ended || s.t >= 300.0f;
        if (!ride_tick(pl, s, c, dt, false, 0.0f)) { hop_off(pl, s); return; }
        if (over) begin_ride(pl, s);
        return;
    }
    case PHASE_RIDE: {
        if (!enemy_alive(s.enemy)) { hop_off(pl, s); return; }
        s.t += dt;
        if ((s.se_t += dt) >= se_ride_interval) { s.se_t = 0.0f; play_se(pl, se_ride); }
        const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
        if (!s.jump_held && (trg & 0x4)) { hop_off(pl, s); return; }   // jump: the enemy step off the board
        if (!s.melee_held && (trg & 0x1)) {                    // melee: the launcher, else a hop
            if (finish(pl, s, c)) return;
            hop_off(pl, s);
            return;
        }
        if (c == DANTE && (trg & 0x200) && style_gun_move(pl, s)) return;   // Gunslinger: the gun's style move takes the body, the board goes on (style_post)
        if (s.t >= ride_frames) { hop_off(pl, s); return; }
        if (!ride_tick(pl, s, c, dt, true)) hop_off(pl, s);     // a ledge, or the enemy got up
        return;
    }
    default:
        return;
    }
}

// ---- the gun during the ride: UpperGun's host (RE/player_move_pipeline.md "Upper-body gun shots") ----
// The ride is attack 12 with its own mAtckId, which the upper-only shot shares: UpperGun keeps the ride's id in the
// word through the pre-check tick and the lower pass (the dispatch reaches move()), the gun's through the upper pass
// (lower_pass_done at the end of move()), and puts the ride's back in the post-action tick; riding() reads the
// effective id. The cancel gate stays off during the ride (the shot's phase 0 turns it on; with it on, checkAction's
// case 12 would run the enemy-step / air-hike checks and checkAtck over a move that handles its own exits).
bool gun_active(uintptr_t pl) { return mod_enabled && riding(pl) && at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_RIDE; }
bool gun_facing(uintptr_t pl, float* yaw) { const RideState* s = find_state(pl); if (!s) return false; *yaw = s->yaw; return true; }
bool gun_lower_motion(uintptr_t pl, upper_gun::Motion* m) {
    const RideState* s = find_state(pl);
    if (!s || s->ride_motion < 0) return false;
    m->motion = s->ride_motion; m->attr = s->ride_attr; m->speed = s->ride_speed_mul;
    return true;
}
// before the arms' request after a shot: a registry word has no .ctn of its own, so the upper half borrows the ride's
// (the lower half's flag is cleared again: left armed, the next lower request (the hop off) would read it instead of its own)
void gun_before_back(uintptr_t pl) {
    const RideState* s = find_state(pl);
    if (!s || !s->ride_custom) return;
    devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
    at<bool>(pl, pl::mMotionSetCharTblParam + 12) = false;   // [1] = the lower half
}
uint32_t gun_lower_id(uintptr_t pl) { return character_of(pl) == NERO ? nero_move_id : dante_move_id; }

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// The predicate and the move.
// ---------------------------------------------------------------------------------------------------------------
bool can_mount(uintptr_t pl) {
    if (!pl || (at<uint32_t>(pl, pl::mActStat) & 0x2)) return false;    // the rows are ground only
    if (pandora_in_hand(pl)) return false;                              // disabled with Pandora (see put_pandora_away)
    return ride_target(pl) != 0;
}

// The hit shell's lifetime: the ride `serial` of `pl` is still on the board.
bool ride_active(uintptr_t pl, uint32_t serial) {
    const RideState* s = find_state(pl);
    return s && s->serial == serial && s->enemy != 0 &&
           (s->style_active || (riding(pl) && (at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_RIDE ||
                                              at<int>(pl, pl::mBdyActionLowerPhase) == PHASE_START)));
}

void __stdcall move(uintptr_t pl, uint32_t) {
    move_impl(pl);
    upper_gun::lower_pass_done(pl);     // the upper pass comes next: a shot's id into mAtckId for its state function
}

// The rows MoveFreeRide added (one per character): byte 0 of the command word = the command bit, byte 1 = 1 makes the
// row unmatchable (the table's convention for a disabled custom move).
static int g_row_marker[2] = { 0, 0 };

void apply_rows() {
    for (int c = 0; c < 2; ++c) {
        kAtckDefTbl* row = MoveTable::row(c == NERO ? 0 : 1, g_row_marker[c]);
        if (!row) continue;
        row->command.buffer = (uint32_t)(g_cmd_bit & 0xFF) | ((mod_enabled && g_cmd_bit >= 0) ? 0u : 0x100u);
    }
}

} // namespace free_ride

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only.
// ---------------------------------------------------------------------------------------------------------------
using namespace free_ride;

std::optional<std::string> FreeRide::on_initialize() {
    // rollback netplay: the rides go with the frames
    rollback::add_range("FreeRide rides", g_rides, sizeof g_rides);
    rollback::add_range("FreeRide serial", &g_ride_serial, sizeof g_ride_serial);
    free_ride::register_shell_rollback();
    using namespace input_commands;
    // COMMAND attr, start | cancel | repeat, any weapon / style, ground only (atckAs 1), cancel words as Gerb's rows
    // (C_COMMAND open or buffered, or C_ALL). The command byte is written by apply_rows (0x100 = disabled until then).
    const uint32_t nero_row[14]  = { 2, nero_move_id,  7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    const uint32_t dante_row[14] = { 2, dante_move_id, 7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    g_row_marker[NERO]  = MoveTable::add_row(0, nero_row);
    g_row_marker[DANTE] = MoveTable::add_row(1, dante_row);
    move_registry::register_move(move_registry::NERO, nero_move_id, &mod_enabled, &free_ride::move);     // the ID gate + dispatch
    move_registry::register_move(move_registry::DANTE, dante_move_id, &mod_enabled, &free_ride::move);

    build_command();                           // + can_mount: a downed enemy in range
    g_cmd_bit = next_free_bit();
    if (g_cmd_bit < 0 || !add(g_cmd.head(), g_cmd_bit, &mod_enabled, &can_mount)) {
        spdlog::error("FreeRide: no free command bit (85-95 are all taken)");
        g_cmd_bit = -1;
    }
    add_tick(&pre_tick);
    {                                          // the gun during the ride (UpperGun; WallRun may have installed the hooks already)
        upper_gun::Host h;
        h.name = "FreeRide";
        h.active = &gun_active;                // (no `enabled`: the gun is always on; no shot_end_ticks: the shots end on their own)
        h.facing = &gun_facing;
        h.lower_motion = &gun_lower_motion;
        h.before_back = &gun_before_back;
        h.lower_atck_id = &gun_lower_id;
        h.clear_cancel_gate = true;
        h.upper_blend_min = &gun_blend_min;
        h.back_blend_frames = &gun_back_blend;
        h.gun_hold_frames = &gun_hold_frames;
        if (!upper_gun::add_host(h)) spdlog::error("FreeRide: UpperGun refused the host; no shooting during the ride");
    }
    {                                          // the post-action tick: the board under a Gunslinger move, weapon swaps (UpperGun's add_host installs the hook; else here)
        if (!input_commands::post_hook_installed) {
            if (!install_hook_absolute(0x7A8453, hook_post, &input_commands::detour_post, &input_commands::jmp_ret_post, 8))
                spdlog::error("FreeRide: failed to hook uPlayer::main after moveAction (0x7A8453); no Gunslinger moves or weapon swaps on the board");
            else input_commands::post_hook_installed = true;
        }
        if (input_commands::post_hook_installed && !add_post_tick(&post_tick)) spdlog::error("FreeRide: no room for the post-action tick");
    }
    if (!input_commands::hook_installed) {     // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("FreeRide: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init FreeRide (command hook)";
        }
        input_commands::hook_installed = true;
    }
    register_lmt();
    apply_rows();
    return Mod::on_initialize();
}

void FreeRide::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) { debug_panel(); return; }   // the Debug tab: the readout
    if (display != DISPLAY_NERO_A && display != DISPLAY_DANTE_A) return;   // both characters' tabs: one setting for both
    if (ImGui::Checkbox(_("Free Ride"), &mod_enabled)) apply_rows();
    ImGui::SameLine();
    help_marker(_("DMC3's Free Ride for Nero and Dante: lock-on + forward + jump next to a knocked-down enemy hops onto it and surfs "
                  "it along the ground. The stick steers; jump steps off (the enemy step), melee launches it (Nero's High Roller, "
                  "Dante's High Time with Rebellion). Little hits keep the enemy down under the board (it takes damage, as in "
                  "DMC3). The guns fire from the upper half during the ride; in Gunslinger, Dante's style button plays the gun's "
                  "style move on the board. Optional motions: motion\\pl000\\freeride.lmt (Nero) / motion\\pl006\\freeride.lmt "
                  "(Dante): block 0 is the start-up on the board, block 1 the ride; the mount is the game's hop."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("FreeRide");

    if (ImGui::Checkbox(_("Stick toward the target"), &need_forward)) build_command();
    ImGui::SameLine();
    help_marker(_("On: lock-on + forward (toward the target, within 45 degrees) + jump. Off: lock-on + jump alone."));
    if (display == DISPLAY_NERO_A) {
        ImGui::Checkbox(_("Rev the Exceed gauge"), &exceed_fill);
        ImGui::SameLine();
        help_marker(_("The ride fills Nero's Exceed gauge by itself: the bar rises over 80 frames and a stock lights at the top, until "
                      "all three are in (over the 240-frame ride). Revving by hand still works on top of it."));
    }
    {
        const char* modes[] = { "Hits (damage keeps it down)", "Hit stop (frozen)" };
        int mode = hit_mode == HIT_DAMAGE ? 0 : 1;
        if (ImGui::Combo(_("Hold the enemy by"), &mode, modes, 2)) hit_mode = mode == 0 ? HIT_DAMAGE : HIT_STOP;
        ImGui::SameLine();
        help_marker(_("How the enemy is kept under the board. Hits: a hitbox on it deals a little damage every 12 frames, and each "
                      "hit replays its down flinch so it stays down the game's own way (and takes damage, as in DMC3); the ride hops "
                      "off if it gets up anyway. Hit stop: its hit-stop timer is topped up every tick, so it stands frozen in its "
                      "pose. The hop onto it always uses the hit stop."));
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void FreeRide::debug_panel() {
    if (!mod_enabled) return;
    ImGui::PushID("FreeRide");
    ImGui::TextUnformatted(_("Free Ride"));
    ImGui::Indent(lineIndent);
    if (uPlayer_old* pl = devil4_sdk::get_local_player()) {      // live readout
        const uintptr_t P = (uintptr_t)pl;
        const int c = character_of(P);
        const RideState* s = find_state(P);
        const uintptr_t ridden = s ? s->enemy : 0;
        ImGui::Text("command bit %d | fired %u | entries %u | finishers %u | hops %u | style moves %u%s", g_cmd_bit, g_fired, g_entries, g_finishes, g_hops, g_style_moves,
                    (s && s->style_active) ? " (one running)" : "");
        ImGui::Text("riding %d phase %d t %.1f | target in reach %s (flat %.0f) | ridden %p hitstop %.1f | floor %.1f",
                    (int)riding(P), at<int>(P, pl::mBdyActionLowerPhase), s ? s->t : 0.0f, g_in_reach ? "yes" : "no", s ? s->flat_t : 0.0f, (void*)ridden,
                    enemy_alive(ridden) ? at<float>(ridden, em::mEmHitStopTimer) : 0.0f, s ? s->floor_y : 0.0f);
        if (hit_mode == HIT_DAMAGE) hit_readout();
        ImGui::Text("gun: shots %d | %s | upper action %d | atck id %u (effective %u) | shooting %s", upper_gun::shots, upper_gun::last,
                    at<int>(P, upper_gun::kBdyUpperNo), at<uint32_t>(P, pl::mAtckId), upper_gun::effective_atck_id(P),
                    upper_gun::shooting(P) ? "yes" : "no");
        if (c == NERO)
            ImGui::Text("exceed: stocks %d | bar %.1f / %.1f | ride %.0f / %.0f", at<int>(P + kOdGauge, kOdStock),
                        at<float>(P + kOdGauge, kOdValue), at<float>(P + kOdGauge, kOdMax), s ? s->exceed : 0.0f, exceed_frames);
        ImGui::Text(_("LMT file: %s | %s"), lmt_file_exists(lmt_path[c]) ? "found" : "missing",
                    lmt_set >= 0 ? motion_registry::describe(lmt_set, P).c_str() : "no set registered");
        if (ImGui::Button(_("Reload LMT"))) motion_registry::refresh();   // read again from disk on the next ride
        ImGui::SameLine();
        help_marker(_("Reads the LMTs again from disk; the next ride uses them."));
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void FreeRide::on_config_load(const utility::Config& cfg) {
    mod_enabled  = cfg.get<bool>("free_ride").value_or(false);
    need_forward = cfg.get<bool>("free_ride_need_forward").value_or(need_forward);
    exceed_fill  = cfg.get<bool>("free_ride_exceed").value_or(exceed_fill);
    hit_mode     = cfg.get<int>("free_ride_hit_mode").value_or(hit_mode) == HIT_STOP ? HIT_STOP : HIT_DAMAGE;
    build_command();                           // the steps follow need_forward
    apply_rows();
}

void FreeRide::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("free_ride", mod_enabled);
    cfg.set<bool>("free_ride_need_forward", need_forward);
    cfg.set<bool>("free_ride_exceed", exceed_fill);
    cfg.set<int>("free_ride_hit_mode", hit_mode);
}
