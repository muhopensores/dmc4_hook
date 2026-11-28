#include "CoopContext.hpp"
#include "Coop.hpp"
#include "../../sdk/Rollback.hpp"
#include "CoopDiag.hpp"
#include "CoopPads.hpp"
#include "CoopPvp.hpp"
#include "../../mod.hpp"
#include "../../sdk/Cam.hpp"
#include "../../sdk/SimView.hpp"
#include "../../mods/MultiCharFixes.hpp"
#include "../../mods/EnemyAI.hpp"
#include <cstring>
#include <vector>

namespace {

using coop::players;
using coop::kMaxPlayers;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

constexpr uintptr_t kMediator     = 0xE558B8; // sMediator::mpInstance
constexpr uintptr_t kMedPlayer    = 0x24;     // sMediator::mpPlayer
constexpr uintptr_t kMedCamera    = 0xD0;     // sMediator::mpCamera
constexpr uintptr_t kMedBossCamera = 0xD4;   // sMediator: the boss camera (uCameraBoss::setup 0x4C39E0)
constexpr uintptr_t kPadInstance  = 0xE559C4; // sDevil4Pad::mpInstance

// Units made for a player, by class: DTI and where the class keeps its player.
constexpr uintptr_t kGetDTI = 0x10;           // MtObject vtable: getDTI
constexpr uintptr_t kDtiParent = 0x10;        // MtDTI::mp_parent
struct OwnedClass { uintptr_t dti; uintptr_t owner; };
constexpr OwnedClass kOwnedClasses[] = {
    {0xE5AB70, 0x17B0},   // uPlShl::mpParent (before uShell: its mpSrcActor may be the shot's other source)
    {0xE5B310, 0x1370},   // uShell::mpSrcActor
    {0xE5AD30, 0x2F34},   // uPlWpRightHand::mpParentPl (before uPlWeapon: the Devil Trigger stand's arm, whose
                          // collision reports the arm itself (uCollisionMgr::init 0x83B733), keeps its Nero only here)
    {0xE5ABB0, 0x1378},   // uPlWeapon::mpParent
    {0xE5AA70, 0x13C8},   // uPlGrabShortHit: the Buster impact's hit, its stand arm (create 0x8203E0; a part, see part_slot)
    {0xE5AA90, 0x1370},   // uPlNeroDevil::mpParent
};
constexpr uintptr_t kNeroVtable   = 0xBE4FA0; // uPlayerNero::vftable
constexpr uintptr_t kEnemyDti     = 0xE596C8; // uEnemy::DTI (every uEm* enemy and boss)
constexpr uintptr_t kDanteBossDti = 0xE5A420; // uPlayerDanteBoss::DTI (a uPlayer: the Dante fight)
constexpr uintptr_t kShellDti     = 0xE5B310; // uShell::DTI (enemy shells too: uEm003Shl ... uEmPushShl)
constexpr uintptr_t kPlShlDti     = 0xE5AB70; // uPlShl::DTI (uPlDanteBossShl000: the boss Dante's shots)
constexpr uintptr_t kShellSrc     = 0x1370;   // uShell::mpSrcActor
constexpr uintptr_t kPlShlParent  = 0x17B0;   // uPlShl::mpParent
constexpr uintptr_t kPos          = 0x30;     // uCoord::mPos
constexpr uintptr_t kTargetIsPlayer = 0x14B1; // uEnemy::mTargetEnemyPlayer
constexpr uintptr_t kTargetEnemy  = 0x14B4;   // uEnemy::mpTargetEnemy (the actor it is after)
constexpr uint32_t  kActionDamage = 0x14;     // uPlayer lower action: hit, knocked down, caught by an enemy
constexpr uintptr_t kIsDead       = 0x1E82;   // uPlayer::mIsDead (cleared by the revive)
constexpr uintptr_t kParamTbl     = 0x1E9C;   // uPlayer: rPlParamTbl data, set by uPlayer::setup 0x7A7A45 (his first update)
constexpr uintptr_t kGroupMgr     = 0x98;     // cCollisionGroup::mpCollisionMgr
constexpr uintptr_t kMgrReport    = 0xD8;     // uCollisionMgr::mpReportActor (the actor a hit counts for)
constexpr uintptr_t kLowerAction  = 0x1500;   // uPlayer::mBdyAction[1].no
constexpr uintptr_t kAtckId       = 0x1564;   // uPlayer::mAtckId
constexpr uintptr_t kGrabActor    = 0xCCD0;   // uPlayerNero::mGrabHitInfo.actor
constexpr uint32_t  kActionAttack = 0xC;
constexpr int       kGrace        = 60;       // game ticks after the grab move ends
constexpr uintptr_t kHeldState    = 0x14;     // cUnit state byte
constexpr uint8_t   kFollowsHand  = 0x8F;     // held: follows the Nero's Devil Bringer
constexpr uintptr_t kStand        = 0xCDF8;   // uPlayerNero::mpDevilStand

// Grim Grips (stage sets on move line 9, from the room's .pla): a base (uStageSetSnatchBase, its update 0x88FB40) and
// the point above it (uStageSetSnatch, update 0x88EBD0, its base at +0x1820). Both read sMediator::mpPlayer only:
// - the base turns active (+0x1380) while mpPlayer is a Nero who has the Snatch, standing within 100 cm across and
//   50 cm up or down of it;
// - an idle point (state +0x14 = 0) whose base is active hands mpPlayer the point as his Snatch target (+0x1430 /
//   +0x1434 = 12);
// - the Snatch hit (grabMessage 0x88F7D0, run in the hand's player's context) sets state 2, or 1 for a point without a
//   base. From 2 the point waits for mpPlayer's long grab approach (action 12, attack 3: uPlayerNero::isLongGrabApproach
//   0x88E2C0) -> 3, for him in the landing area (cEventHit::checkInPl reads mpPlayer's position) -> 4, for his action 1
//   -> 1, setting his flags +0x1509 .. +0x150F and sMediator +0x122 (the snatch-jump camera) on the way. In states 1-4
//   its sub-state +0x15 follows mpPlayer's approach (1 when it starts, 2 when it ends), then a timer shows it again.
// Running as player 1, a grip never gave players 2-4 its target, and one they snatched waited for player 1's approach.
// So a base runs as the nearest Nero, an idle point as its base's player, and a snatched point as the Nero who snatched
// it until it is idle again.
constexpr uintptr_t kGripPointDti = 0xE5BC10;   // uStageSetSnatch::DTI
constexpr uintptr_t kGripBaseDti  = 0xE5BBF0;   // uStageSetSnatchBase::DTI
constexpr uintptr_t kGripState    = 0x14;       // point: 0 idle, 1 used (shown again after a timer), 2 snatched, 3 on the way, 4 arrived
constexpr uintptr_t kGripSubState = 0x15;       // point: 0 after the hit, 1 while the approach runs, 2 after it
constexpr uintptr_t kGripBase     = 0x1820;     // point: its base, 0 = none
constexpr uintptr_t kPlayerId     = 0x1494;     // uPlayer::mPlayerID (1 = Nero)
constexpr uintptr_t kBodyAction   = 0x14F0;     // uPlayer::mBdyAction[0].no (isLongGrabApproach reads it)
constexpr uint32_t  kGrabApproach = 3;          // mAtckId of the long grab approach
constexpr int       kMaxGrips     = 32;

struct Hold { uintptr_t enemy = 0; int ticks = 0; };

// Which player an enemy is after (step 4a). Picked once per game tick: the nearest player, switching only to one at
// most kSwitchRatio of the current one's distance; a hit sets it to the attacker and holds it kHitHold ticks, a grab
// to the grabbing player for as long as the hold lasts.
struct Aggro {
    uintptr_t enemy = 0;
    int       slot = 0;          // 0-3
    int       hold = 0;          // ticks left before distance may change it
    uint32_t  frame = 0xFFFFFFFFu;   // the tick it was last picked
};
constexpr int   kMaxAggro    = 128;
constexpr int   kHitHold     = 180;
constexpr int   kSwitchHold  = 60;
constexpr float kSwitchRatio = 0.7f;
constexpr uint32_t kStale    = 300;   // ticks unseen before an entry is reused

constexpr uintptr_t kLockOnMgr     = 0xE558B0; // sLockOnTargetMgr::mpInstance
constexpr uintptr_t kMgrTop        = 0x20;     // mpTop
constexpr uintptr_t kMgrSnatchTop  = 0x24;     // mpSnatchTop
constexpr uintptr_t kTgtNext       = 0x04;     // cLockOnTarget::mpNext
constexpr uintptr_t kTgtModel      = 0x08;     // mpModel (uActor*)
constexpr uintptr_t kTgtLength     = 0x24;     // mPILength
constexpr uintptr_t kTgtIn         = 0x34;     // mIsIn
constexpr uintptr_t kTgtCameraIn   = 0x37;     // mIsCameraIn
constexpr uintptr_t kTgtNearPos    = 0x40;     // mNearPos
constexpr uintptr_t kSphereCount   = 0xEF0;    // uActor::muALockonSphereNum
constexpr uintptr_t kSpheres       = 0xF00;    // uActor::muALockOnSphere[16], world MtSphere first (x y z r)
constexpr uintptr_t kSphereStride  = 0x30;
constexpr uintptr_t kCamera        = 0xE552D4; // sCamera::mpInstance
constexpr uintptr_t kGetCenterPos  = 0x84;     // uActor vtable: getCenterPos(MtVector3* out)
constexpr int       kMaxTargets    = 256;

using GetNearLengthFn = int(__stdcall*)(uintptr_t actor, const float* pos, float* length, char* camera_in, char* in,
                                        float* near_pos);
const GetNearLengthFn get_near_length = reinterpret_cast<GetNearLengthFn>(0x4A8620);   // uActor::getNearLengthSpherIdx
using GetCenterPosFn = float*(__fastcall*)(uintptr_t actor, uintptr_t edx, float* out);

struct SavedTarget { uintptr_t target; float length; uint8_t in, camera_in, inter, no_target; float near_pos[3]; };
SavedTarget lock_saved[kMaxTargets];
int         lock_saved_n = 0;
bool        lock_swapped = false;

// A player 2-4's lock-on values, computed once per game tick and written into the targets for as long as one of his
// units updates (lock_applied); player 1's go back when it ends.
uint32_t g_frame = 0;
struct CacheEntry { uintptr_t target; float length; uint8_t in, camera_in; float near_pos[3]; };
struct LockCache { uint32_t frame = 0xFFFFFFFFu; int n = 0; CacheEntry e[kMaxTargets]; };
LockCache lock_cache[kMaxPlayers];
bool lock_applied = false;
int  lock_applied_slot = -1;

// Readout of the last lock-on update of each player 2-4.
struct LockDiag {
    uint32_t  updates = 0;
    int       targets = 0, valid = 0, stick = 0;
    uintptr_t picked = 0, nearest = 0, center_valid = 0;
    float     picked_len = -1.0f, nearest_len = -1.0f;
    uint8_t   picked_in = 0, nearest_in = 0;
    float     center[3] = {};
};
LockDiag lock_diag[kMaxPlayers];

// As the passive pickers (sub_451200 / sub_4512D0) filter: the model active and on, the target not excluded.
bool pickable(uintptr_t t) {
    const uintptr_t model = at<uintptr_t>(t, kTgtModel);
    if (!model) return false;
    const uint32_t flags = at<uint32_t>(model, 4);
    return (flags & 7) == 2 && (flags & 0x400) && !at<uint8_t>(t, 0x38) && !at<uint8_t>(t, 0x3A);
}

// sCamera::isDispSphere for one viewport: the sphere is outside if it lies behind any of the six planes.
bool sphere_in_view(const sCamera_ViewPort& vp, const float* s) {
    for (const MtVector4& p : vp.mFrustum) {
        if (-s[3] > p.x * s[0] + p.y * s[1] + p.z * s[2] + p.w) {
            return false;
        }
    }
    return true;
}

// Readout, copied on the game thread.
struct GrabDiag {
    uint32_t  hits = 0, swapped_hits = 0;
    uintptr_t owner = 0, enemy = 0, med_player = 0;
    int       owner_slot = -1;
    bool      swapped = false;
    float     start[3] = {}, end[3] = {};
};
struct StandDiag { uintptr_t player = 0, stand = 0, hand = 0, hand_owner = 0, nero_hand = 0; float ret[3] = {}; };
GrabDiag  grab_diag;
StandDiag stand_diag[kMaxPlayers];
Hold      holds[kMaxPlayers];
Aggro     aggro[kMaxAggro];
struct Grip { uintptr_t unit = 0; int slot = -1; uint32_t frame = 0; };   // a base's player, a snatched point's Nero
Grip      grips[kMaxGrips];
uintptr_t enemy_swapped = 0;   // the enemy (or its shell) whose update runs in another player's context now
struct AggroDiag { int per_slot[kMaxPlayers] = {}; uint32_t hits = 0, hit_swaps = 0; int last_attacker = -1, last_target = -1; };
AggroDiag aggro_diag;
uintptr_t saved_player = 0, saved_camera = 0, saved_pad = 0;
// sMediator +0x122 (mSnatchJumpCamera: a Grim Grip jump runs, read by uCameraCtrl::sync 0x4F913D / 0x4F91FC and
// getInitNowArea) per player: a swap to a player 2-4 puts his own in and takes it back out, so player 1's camera
// doesn't follow another player's jump.
constexpr uintptr_t kMedSnatchCamera = 0x122;
uint8_t   snatch_camera[kMaxPlayers] = {};
uint8_t   saved_snatch_camera = 0;
int       snatch_camera_slot = -1;
bool      swapped = false, camera_swapped = false, pad_swapped = false;
uint32_t  swaps = 0, swaps_shown = 0;

bool grab_attack(uint32_t id) {
    return id == 1 || id == 2 || id == 5 || id == 6 || id == 7 || id == 8;
}

// The player (from `first`) whose body, Devil Trigger stand (uPlayerNero +0xCDF8) or stand arm (uPlNeroDevil::mpRightHand
// +0x1374) `owner` is, -1 if none. Pointer compares only: an owner field may hold a unit that is gone.
int part_slot(uintptr_t owner, int first) {
    if (!owner) {
        return -1;
    }
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = first; i < n; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl) continue;
        if (pl == owner) return i;
        if (at<uint32_t>(pl, 0x1494) != 1) continue;   // mPlayerID: only a Nero has a stand
        const uintptr_t stand = at<uintptr_t>(pl, kStand);
        if (stand && (stand == owner || at<uintptr_t>(stand, 0x1374) == owner)) return i;
    }
    return -1;
}

// The player 2-4 a player pointer (or one of his parts, part_slot) is, -1 if none.
int slot_of(uintptr_t player) {
    return part_slot(player, 1);
}

bool kind_of(uintptr_t dti, uintptr_t target) {
    for (int depth = 0; dti && depth < 32; depth++, dti = at<uintptr_t>(dti, kDtiParent)) {
        if (dti == target) {
            return true;
        }
    }
    return false;
}

// The player 2-4 `unit` is, or belongs to (his HUD, his shells, weapons and stand), -1 if none.
int own_unit_owner(uintptr_t unit) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    bool any = false;
    for (int i = 1; i < n; i++) {
        if (!players[i].unit) continue;
        any = true;
        if (players[i].unit == unit || (uintptr_t)players[i].hud == unit || (uintptr_t)players[i].reticle == unit) {
            return i;
        }
    }
    if (!any) {
        return -1;
    }
    const uintptr_t vtable = at<uintptr_t>(unit, 0);
    const uintptr_t dti = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t)>(at<uintptr_t>(vtable, kGetDTI))(unit, 0);
    for (const OwnedClass& c : kOwnedClasses) {
        if (kind_of(dti, c.dti)) {
            const int slot = slot_of(at<uintptr_t>(unit, c.owner));
            if (slot >= 0) {
                return slot;
            }
        }
    }
    return -1;
}

// The player 2-4 whose camera `unit` is, -1 if none.
int camera_owner(uintptr_t unit) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = 1; i < n; i++) {
        if (players[i].cam && (uintptr_t)players[i].cam == unit && players[i].unit) {
            return i;
        }
    }
    return -1;
}

// The player (0-3) whose Nero holds `unit`, -1 if none. Held: his grab move runs on it, or the unit follows a hand
// (cUnit +0x14 = 0x8F: uEm000Base::moveEx 0x543F44 then places it at mpPlayer's mpDevilStand +0xCDF8 -> mpRightHand
// +0x1374, which a Dante doesn't have: a hold that outlasted the grab move ran as player 1 and crashed at 0x543F53).
int owner_of(uintptr_t unit) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    const bool follows_hand = at<uint8_t>(unit, kHeldState) == kFollowsHand;
    for (int i = 0; i < n; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl || at<uintptr_t>(pl, 0) != kNeroVtable) {
            continue;
        }
        if (at<uintptr_t>(pl, kGrabActor) == unit &&
            (follows_hand || (at<uint32_t>(pl, kLowerAction) == kActionAttack && grab_attack(at<uint32_t>(pl, kAtckId))))) {
            holds[i] = {unit, kGrace};
            return i;
        }
    }
    for (int i = 0; i < n; i++) {
        if (holds[i].enemy == unit && holds[i].ticks > 0 && players[i].unit) {
            return i;
        }
    }
    // following a hand without a matching grab target (a hold the grab target doesn't record): a Nero with a stand,
    // player 1 first, never a Dante
    if (follows_hand) {
        for (int i = 0; i < n; i++) {
            const uintptr_t pl = players[i].unit;
            if (pl && at<uintptr_t>(pl, 0) == kNeroVtable && at<uintptr_t>(pl, kStand)) {
                holds[i] = {unit, kGrace};
                return i;
            }
        }
    }
    return -1;
}

// A player set up (his first update has run): the lock-on cone check sub_4A8200 (from getNearLengthSpherIdx, every
// enemy's update) reads mpPlayer's +0x1E9C, null before; an enemy running as a player spawned this frame crashed there.
bool ready(uintptr_t player) {
    return player && at<uintptr_t>(player, kParamTbl) != 0;
}

// The player (0-3) a pointer is, -1 if none (pointer compare only).
int player_index(uintptr_t pl) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = 0; pl && i < n; i++) {
        if (players[i].unit == pl) return i;
    }
    return -1;
}

// Rail and pan cameras. A uCameraCtrl picks its area camera from its rDevilCamera (+0x3A0; definitions: u16 count
// +0x64, array +0x6C). The follow areas (type 2) run the camera's own cCameraPlayer (+0x498), but uCameraCtrl::
// changeCamera 0x4F9B30 runs every other kind straight from the resource's object: +0x494 = the object, its owner
// +0xF8 = the camera, its reset (vtbl +0x18) on entering the area, uCameraCtrl::sync 0x4F90B0 its update (vtbl +0x14)
// each frame. Players 2-4's cameras share player 1's rDevilCamera, so two cameras in one rail area drove one object
// (its rail state, smoothing and owner flipping between them). Each extra camera keeps its own copy of every such
// object, swapped in for the length of its update and player 1's put back after.
struct SpecialClass { uintptr_t vtable; uint32_t size; };
constexpr SpecialClass kSpecialCameras[] = {
    {0xB98B60, 0x1F0},    // cCameraPan (cCameraPan::MyDTI::newInstance 0x428050)
    {0xB98BA0, 0x21B0},   // cCameraRail (0x428070)
    {0xB98BE0, 0x21C0},   // cCameraRailEm025 (0x428090)
    {0xB98C20, 0x21B0},   // cCameraRailEm026 (0x4280B0)
};
constexpr uintptr_t kCamResource  = 0x3A0;    // uCameraCtrl::mpResource (rDevilCamera)
constexpr uintptr_t kCamDefCount  = 0x64;     // rDevilCamera: number of area cameras (u16)
constexpr uintptr_t kCamDefs      = 0x6C;     // rDevilCamera: the area cameras' array

struct CamCopy { uintptr_t obj = 0, vtable = 0; std::vector<uint8_t> mine; };
std::vector<CamCopy> cam_copies[kMaxPlayers];   // per extra player: his copy of each special camera object
uintptr_t cam_copies_res[kMaxPlayers] = {};
struct CamHeld { CamCopy* copy; std::vector<uint8_t> player1; };
std::vector<CamHeld> cam_held;                  // swapped in now (player 1's bytes kept here)
size_t cam_held_n = 0;

uint32_t special_size(uintptr_t vtable) {
    for (const SpecialClass& c : kSpecialCameras) {
        if (c.vtable == vtable) return c.size;
    }
    return 0;
}

void cameras_in(int slot, uintptr_t cam) {
    cam_held_n = 0;
    const uintptr_t res = at<uintptr_t>(cam, kCamResource);
    if (!res) return;
    if (cam_copies_res[slot] != res) {
        cam_copies[slot].clear();
        cam_copies_res[slot] = res;
    }
    const uintptr_t defs = at<uintptr_t>(res, kCamDefs);
    const int n = at<uint16_t>(res, kCamDefCount);
    if (!defs) return;
    cam_copies[slot].reserve(cam_copies[slot].size() + n);   // no reallocation while cam_held points into it
    for (int k = 0; k < n; k++) {
        const uintptr_t obj = at<uintptr_t>(defs, 4 * k);
        if (!obj) continue;
        const uintptr_t vtable = at<uintptr_t>(obj, 0);
        const uint32_t size = special_size(vtable);
        if (!size) continue;
        CamCopy* copy = nullptr;
        for (CamCopy& c : cam_copies[slot]) {
            if (c.obj == obj) { copy = &c; break; }
        }
        if (!copy) {
            cam_copies[slot].push_back(CamCopy{});
            copy = &cam_copies[slot].back();
            copy->obj = obj;
        }
        if (copy->vtable != vtable || copy->mine.size() != size) {   // first use, or another object at that address
            copy->vtable = vtable;
            copy->mine.assign(reinterpret_cast<const uint8_t*>(obj), reinterpret_cast<const uint8_t*>(obj) + size);
        }
        if (cam_held.size() <= cam_held_n) cam_held.emplace_back();
        CamHeld& h = cam_held[cam_held_n++];
        h.copy = copy;
        h.player1.assign(reinterpret_cast<const uint8_t*>(obj), reinterpret_cast<const uint8_t*>(obj) + size);
        std::memcpy(reinterpret_cast<void*>(obj), copy->mine.data(), size);
    }
}

void cameras_out() {
    for (size_t i = 0; i < cam_held_n; i++) {
        CamHeld& h = cam_held[i];
        void* obj = reinterpret_cast<void*>(h.copy->obj);
        std::memcpy(h.copy->mine.data(), obj, h.copy->mine.size());
        std::memcpy(obj, h.player1.data(), h.player1.size());
    }
    cam_held_n = 0;
}

uintptr_t dti_of(uintptr_t unit) {
    const uintptr_t vtable = at<uintptr_t>(unit, 0);
    return reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t)>(at<uintptr_t>(vtable, kGetDTI))(unit, 0);
}

bool any_extra() {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = 1; i < n; i++) {
        if (players[i].unit) return true;
    }
    return false;
}

// The player (0-3) a unit is or belongs to (his shells, weapons, stand), -1 if none.
int player_slot(uintptr_t unit) {
    if (!unit) return -1;
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = 0; i < n; i++) {
        if (players[i].unit && players[i].unit == unit) return i;
    }
    const uintptr_t dti = dti_of(unit);
    for (const OwnedClass& c : kOwnedClasses) {
        if (kind_of(dti, c.dti)) {
            const int slot = part_slot(at<uintptr_t>(unit, c.owner), 0);
            if (slot >= 0) return slot;
        }
    }
    return -1;
}

Aggro* find_aggro(uintptr_t enemy) {
    for (Aggro& a : aggro) {
        if (a.enemy == enemy && enemy) return &a;
    }
    return nullptr;
}

Aggro& add_aggro(uintptr_t enemy) {
    Aggro* slot = nullptr;
    for (Aggro& a : aggro) {
        if (!a.enemy || a.frame == 0xFFFFFFFFu || g_frame - a.frame > kStale) { slot = &a; break; }
        if (!slot || a.frame < slot->frame) slot = &a;   // else the one seen longest ago
    }
    *slot = Aggro{};
    slot->enemy = enemy;
    return *slot;
}

float dist2(uintptr_t a, uintptr_t b) {
    float d = 0.0f;
    for (int k = 0; k < 3; k++) {
        const float v = at<float>(a, kPos + 4 * k) - at<float>(b, kPos + 4 * k);
        d += v * v;
    }
    return d;
}

// The enemy's player is in his Damage action (hit, down, or caught: uEm019_00::em019_attack_hug and the other grabs on
// a player follow mpPlayer every frame): the enemy stays on him, whoever is nearer or hits it.
bool target_locked(const Aggro& a) {
    const uintptr_t pl = (a.slot >= 0 && a.slot < kMaxPlayers) ? players[a.slot].unit : 0;
    return pl && at<uint32_t>(pl, kLowerAction) == kActionDamage;
}

void pick_target(Aggro& a) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    if (a.slot < 0 || a.slot >= n || !ready(players[a.slot].unit)) {
        a.slot = 0;
        a.hold = 0;
    }
    if (at<uint8_t>(players[a.slot].unit, kIsDead)) {
        a.hold = 0;   // his player is down: the nearest one standing
    }
    if (target_locked(a)) {
        return;
    }
    // provoked by a taunt (EnemyAI's Provoke): after the player who taunted for as long as it lasts
    if (const int by = player_slot(provoke::provoker(a.enemy)); by >= 0 && by < n && ready(players[by].unit) &&
        !at<uint8_t>(players[by].unit, kIsDead)) {
        a.slot = by;
        a.hold = 0;
        return;
    }
    if (a.hold > 0) {
        --a.hold;
        return;
    }
    int best = -1;
    float best_d = 0.0f;
    for (int i = 0; i < n; i++) {
        if (!ready(players[i].unit) || at<uint8_t>(players[i].unit, kIsDead)) continue;   // a downed player is left alone
        const float d = dist2(a.enemy, players[i].unit);
        if (best < 0 || d < best_d) { best = i; best_d = d; }
    }
    if (best < 0 || best == a.slot) return;
    if (at<uint8_t>(players[a.slot].unit, kIsDead)) {
        a.slot = best;
        a.hold = kSwitchHold;
        return;
    }
    const float cur = dist2(a.enemy, players[a.slot].unit);
    if (best_d < cur * kSwitchRatio * kSwitchRatio) {
        a.slot = best;
        a.hold = kSwitchHold;
    }
}

// The player (0-3) an enemy unit, its shell or the boss Dante's shot acts for, -1 if `unit` is none of them. A shell
// counts only once its enemy is in the table (no read through a source that may be gone).
int enemy_target(uintptr_t unit) {
    const uintptr_t dti = dti_of(unit);
    if (kind_of(dti, kEnemyDti) || kind_of(dti, kDanteBossDti)) {
        Aggro* a = find_aggro(unit);
        if (!a) {
            a = &add_aggro(unit);
            a->frame = g_frame - 1;
            a->slot = 0;
            a->hold = 0;
            pick_target(*a);   // a new enemy goes for the nearest player at once
            a->hold = 0;
        }
        if (a->frame != g_frame) {
            pick_target(*a);
            a->frame = g_frame;
        }
        // its cached target (uEnemy::mpTargetEnemy, set from mpPlayer by uEnemy::damageMessage7 0x735FB0, updatePtr
        // and the like) follows: a player there becomes the chosen one (another enemy there is left alone)
        if (kind_of(dti, kEnemyDti) && at<uint8_t>(unit, kTargetIsPlayer)) {
            const uintptr_t chosen = players[a->slot].unit;
            const uintptr_t cached = at<uintptr_t>(unit, kTargetEnemy);
            if (chosen && cached != chosen && player_index(cached) >= 0) {
                at<uintptr_t>(unit, kTargetEnemy) = chosen;
            }
        }
        return a->slot;
    }
    uintptr_t source = 0;
    if (kind_of(dti, kPlShlDti)) {
        source = at<uintptr_t>(unit, kPlShlParent);
    } else if (kind_of(dti, kShellDti)) {
        source = at<uintptr_t>(unit, kShellSrc);
    }
    if (const Aggro* a = source ? find_aggro(source) : nullptr) {
        return a->slot;
    }
    return -1;
}

// After an enemy updated as player 2-4: its postMain (and the other callers of getNearLengthSpherIdx) refilled its
// lock-on targets for that player; player 1's values for them are computed again (mpPlayer is player 1 again).
void refresh_player1_lock(uintptr_t enemy) {
    const uintptr_t mgr = at<uintptr_t>(kLockOnMgr, 0);
    const uintptr_t med = at<uintptr_t>(kMediator, 0);
    const uintptr_t p1 = med ? at<uintptr_t>(med, kMedPlayer) : 0;
    if (!mgr || !ready(p1)) return;
    float center[4] = {};
    reinterpret_cast<GetCenterPosFn>(at<uintptr_t>(at<uintptr_t>(p1, 0), kGetCenterPos))(p1, 0, center);
    for (uintptr_t list : {kMgrTop, kMgrSnatchTop}) {
        int n = 0;
        for (uintptr_t t = at<uintptr_t>(mgr, list); t && n < kMaxTargets; t = at<uintptr_t>(t, kTgtNext), ++n) {
            const uintptr_t actor = at<uintptr_t>(t, kTgtModel);
            if (actor != enemy) continue;
            float length = at<float>(t, kTgtLength);
            char camera_in = (char)at<uint8_t>(t, kTgtCameraIn);
            char in = (char)at<uint8_t>(t, kTgtIn);
            float near_pos[4] = {at<float>(t, kTgtNearPos), at<float>(t, kTgtNearPos + 4), at<float>(t, kTgtNearPos + 8), 0.0f};
            get_near_length(actor, center, &length, &camera_in, &in, near_pos);   // camera test: viewport 0 (view_slot -1)
            at<float>(t, kTgtLength) = length;
            at<uint8_t>(t, kTgtCameraIn) = (uint8_t)camera_in;
            at<uint8_t>(t, kTgtIn) = (uint8_t)in;
            for (int k = 0; k < 3; k++) at<float>(t, kTgtNearPos + 4 * k) = near_pos[k];
        }
    }
}

Grip& grip_entry(uintptr_t unit) {
    Grip* slot = nullptr;
    for (Grip& g : grips) {
        if (g.unit == unit) return g;
        if (!slot && (!g.unit || g_frame - g.frame > kStale)) slot = &g;
    }
    if (!slot) {   // the one seen longest ago
        slot = &grips[0];
        for (Grip& g : grips) {
            if (g.frame < slot->frame) slot = &g;
        }
    }
    *slot = Grip{};
    slot->unit = unit;
    return *slot;
}

bool grip_user(int i) {   // a Nero set up and standing
    const uintptr_t pl = players[i].unit;
    return ready(pl) && at<uint32_t>(pl, kPlayerId) == 1 && !at<uint8_t>(pl, kIsDead);
}

// The player (0-3) a Grim Grip's base or point runs as, -1 for none (player 1, as the game has it), -2 if `unit` is
// neither (see kGripPointDti).
int grip_owner(uintptr_t unit) {
    const uintptr_t dti = dti_of(unit);
    if (dti != kGripPointDti && dti != kGripBaseDti) {
        return -2;
    }
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    Grip& g = grip_entry(unit);
    g.frame = g_frame;
    if (dti == kGripBaseDti) {   // the nearest Nero: the one it can turn active for
        int best = -1;
        float best_d = 0.0f;
        for (int i = 0; i < n; i++) {
            if (!grip_user(i)) continue;
            const float d = dist2(unit, players[i].unit);
            if (best < 0 || d < best_d) { best = i; best_d = d; }
        }
        g.slot = best;
        return best;
    }
    // the Nero whose Snatch or approach is on it now
    for (int i = 0; i < n; i++) {
        const uintptr_t pl = players[i].unit;
        if (!grip_user(i) || at<uintptr_t>(pl, kGrabActor) != unit) continue;
        const uint32_t id = at<uint32_t>(pl, kAtckId);
        const bool attacking = at<uint32_t>(pl, kLowerAction) == kActionAttack || at<uint32_t>(pl, kBodyAction) == kActionAttack;
        if (attacking && (grab_attack(id) || id == kGrabApproach)) {
            g.slot = i;
            return i;
        }
    }
    // in use: the one who snatched it, until it is idle again
    const uint8_t state = at<uint8_t>(unit, kGripState);
    if (g.slot >= 0 && players[g.slot].unit && (state >= 2 || (state == 1 && at<uint8_t>(unit, kGripSubState) < 2))) {
        return g.slot;
    }
    g.slot = -1;
    // idle: its base's player gets its target
    if (state == 0) {
        if (const uintptr_t base = at<uintptr_t>(unit, kGripBase)) {
            for (const Grip& b : grips) {
                if (b.unit == base && g_frame - b.frame <= 1) return b.slot;
            }
        }
    }
    return -1;
}

} // namespace

namespace coop_context {

int player_of(uintptr_t unit) {
    return player_slot(unit);
}

bool swap_to(uintptr_t player, uintptr_t camera = 0, uintptr_t pad = 0) {
    const uintptr_t med = at<uintptr_t>(kMediator, 0);
    if (!med || swapped) {
        return false;
    }
    saved_player = at<uintptr_t>(med, kMedPlayer);
    at<uintptr_t>(med, kMedPlayer) = player;
    camera_swapped = camera != 0;
    if (camera_swapped) {
        saved_camera = at<uintptr_t>(med, kMedCamera);
        at<uintptr_t>(med, kMedCamera) = camera;
    }
    pad_swapped = pad != 0;
    if (pad_swapped) {
        saved_pad = at<uintptr_t>(kPadInstance, 0);
        at<uintptr_t>(kPadInstance, 0) = pad;
    }
    swapped = true;
    ++swaps;
    return true;
}

// mIsInterTarget as the game would keep it for this player alone (inter_flags_for in the header).
bool keeps_inter(uintptr_t player) {
    return MultiCharFixes::targeting_fix || (at<uint8_t>(player, 0x1554) & 4);   // uPlayer::mActStat
}

void set_inter_flags(uintptr_t mgr, uintptr_t player) {
    const uintptr_t own = keeps_inter(player) ? at<uintptr_t>(player, 0x2000) : 0;   // uPlayer::mpLockTarget
    for (uintptr_t list : {kMgrTop, kMgrSnatchTop}) {
        int n = 0;
        for (uintptr_t t = at<uintptr_t>(mgr, list); t && n < kMaxTargets; t = at<uintptr_t>(t, kTgtNext), ++n) {
            at<uint8_t>(t, 0x39) = (own && t == own) ? 1 : 0;
        }
    }
}

bool fixed_views = false;
unsigned long fixed_views_thread = 0;

bool in_fixed_views() {
    return fixed_views && GetCurrentThreadId() == fixed_views_thread;
}

int aggro_slot(uintptr_t enemy) {
    const Aggro* a = find_aggro(enemy);
    return a ? a->slot : -1;
}

int lock_entries(int slot, uintptr_t* actors, float* lengths, uint8_t* in, uint8_t* camera_in, int max) {
    if (slot < 0 || slot >= kMaxPlayers) return 0;
    const LockCache& c = lock_cache[slot];
    int n = 0;
    for (int i = 0; i < c.n && n < max; ++i, ++n) {
        actors[n] = at<uintptr_t>(c.e[i].target, kTgtModel);
        lengths[n] = c.e[i].length;
        in[n] = c.e[i].in;
        camera_in[n] = c.e[i].camera_in;
    }
    return n;
}

// the camera whose view player `slot` sees (slot 0: viewport 0's, which may be a boss or event camera)
static uintptr_t view_camera(int slot) {
    if (slot >= 1 && slot < kMaxPlayers) return (uintptr_t)players[slot].cam;
    const sCamera* cam = at<const sCamera*>(kCamera, 0);
    return cam ? (uintptr_t)cam->viewports[0].mpCamera : 0;
}

// The targets' values from player `slot`'s position and view (mpPlayer must already be him: sub_4A8200's cone reads it).
void compute_lock(uintptr_t mgr, int slot, uintptr_t player) {
    LockCache& c = lock_cache[slot];
    c.frame = g_frame;
    c.n = 0;
    float center[4] = {};
    const uintptr_t vtable = at<uintptr_t>(player, 0);
    reinterpret_cast<GetCenterPosFn>(at<uintptr_t>(vtable, kGetCenterPos))(player, 0, center);
    const sCamera* cam = at<const sCamera*>(kCamera, 0);
    const sCamera_ViewPort* vp = cam ? &cam->viewports[slot] : nullptr;
    if (vp && !vp->mpCamera) {
        vp = &cam->viewports[0];
    }
    float planes[6][4];
    const bool fixed = in_fixed_views();
    const bool own_view = fixed && sim_view::camera_frustum(view_camera(slot), planes);
    for (uintptr_t list : {kMgrTop, kMgrSnatchTop}) {
        int n = 0;
        for (uintptr_t t = at<uintptr_t>(mgr, list); t && n < kMaxTargets && c.n < kMaxTargets; t = at<uintptr_t>(t, kTgtNext), ++n) {
            CacheEntry& e = c.e[c.n++];
            e.target = t;
            e.length = at<float>(t, kTgtLength);
            e.in = at<uint8_t>(t, kTgtIn);
            e.camera_in = at<uint8_t>(t, kTgtCameraIn);
            for (int k = 0; k < 3; k++) e.near_pos[k] = at<float>(t, kTgtNearPos + 4 * k);
            const uintptr_t actor = at<uintptr_t>(t, kTgtModel);
            if (!actor) continue;
            float near_pos[4] = {e.near_pos[0], e.near_pos[1], e.near_pos[2], 0.0f};
            char in = (char)e.in;
            get_near_length(actor, center, &e.length, nullptr, &in, near_pos);   // also places the spheres in the world
            bool camera_in = false;
            const int spheres = at<int>(actor, kSphereCount);
            for (int k = 0; (vp || fixed) && k < spheres && k < 16 && !camera_in; k++) {
                const float* sphere = reinterpret_cast<const float*>(actor + kSpheres + k * kSphereStride);
                camera_in = fixed ? (own_view && sim_view::sphere_in(planes, sphere)) : sphere_in_view(*vp, sphere);
            }
            e.in = (uint8_t)in;
            e.camera_in = camera_in ? 1 : 0;
            for (int k = 0; k < 3; k++) e.near_pos[k] = near_pos[k];
        }
    }
    LockDiag& d = lock_diag[slot];
    ++d.updates;
    for (int k = 0; k < 3; k++) d.center[k] = center[k];
}

void apply_lock(int slot, uintptr_t player) {
    const uintptr_t mgr = at<uintptr_t>(kLockOnMgr, 0);
    if (!mgr || lock_applied || !ready(player)) return;
    LockCache& c = lock_cache[slot];
    if (c.frame != g_frame) compute_lock(mgr, slot, player);
    lock_saved_n = 0;
    for (uintptr_t list : {kMgrTop, kMgrSnatchTop}) {
        int n = 0;
        for (uintptr_t t = at<uintptr_t>(mgr, list); t && n < kMaxTargets && lock_saved_n < kMaxTargets; t = at<uintptr_t>(t, kTgtNext), ++n) {
            SavedTarget& s = lock_saved[lock_saved_n++];
            s.target = t;
            s.length = at<float>(t, kTgtLength);
            s.in = at<uint8_t>(t, kTgtIn);
            s.camera_in = at<uint8_t>(t, kTgtCameraIn);
            s.inter = at<uint8_t>(t, 0x39);
            s.no_target = at<uint8_t>(t, 0x38);
            for (int k = 0; k < 3; k++) s.near_pos[k] = at<float>(t, kTgtNearPos + 4 * k);
            for (int i = 0; i < c.n; i++) {
                const CacheEntry& e = c.e[i];
                if (e.target != t) continue;
                at<float>(t, kTgtLength) = e.length;
                at<uint8_t>(t, kTgtIn) = e.in;
                at<uint8_t>(t, kTgtCameraIn) = e.camera_in;
                for (int k = 0; k < 3; k++) at<float>(t, kTgtNearPos + 4 * k) = e.near_pos[k];
                break;
            }
        }
    }
    set_inter_flags(mgr, player);
    coop_pvp::targets_for(player);   // the players' targets: mIsNoTarget as for him
    lock_applied = true;
    lock_applied_slot = slot;
}

void restore_lock() {
    if (!lock_applied) return;
    for (int i = 0; i < lock_saved_n; i++) {
        const SavedTarget& s = lock_saved[i];
        at<float>(s.target, kTgtLength) = s.length;
        at<uint8_t>(s.target, kTgtIn) = s.in;
        at<uint8_t>(s.target, kTgtCameraIn) = s.camera_in;
        at<uint8_t>(s.target, 0x39) = s.inter;
        at<uint8_t>(s.target, 0x38) = s.no_target;
        for (int k = 0; k < 3; k++) at<float>(s.target, kTgtNearPos + 4 * k) = s.near_pos[k];
    }
    lock_saved_n = 0;
    lock_applied = false;
    lock_applied_slot = -1;
}

int view_slot = -1;

bool sphere_in_viewport(const void* camera_system, int slot, const float* sphere) {
    if (in_fixed_views()) {
        float planes[6][4];
        return sim_view::camera_frustum(view_camera(slot), planes) && sim_view::sphere_in(planes, sphere);
    }
    const sCamera* cam = static_cast<const sCamera*>(camera_system);
    const sCamera_ViewPort& vp = cam->viewports[slot];
    return vp.mpCamera != nullptr && sphere_in_view(vp, sphere);
}

// Player `slot`'s context: him, his camera (or `camera`), his pad, and (`lock`) his lock-on values in the targets.
bool swap_for(int slot, uintptr_t camera = 0, bool lock = true) {
    if (!camera) camera = (uintptr_t)players[slot].cam;
    if (!swap_to(players[slot].unit, camera, (uintptr_t)coop_pads::pad_for(slot))) {
        return false;
    }
    if (const uintptr_t med = at<uintptr_t>(kMediator, 0); med && slot >= 1) {
        saved_snatch_camera = at<uint8_t>(med, kMedSnatchCamera);
        at<uint8_t>(med, kMedSnatchCamera) = snatch_camera[slot];
        snatch_camera_slot = slot;
    }
    view_slot = slot >= 1 ? slot : -1;
    if (lock) {
        apply_lock(slot, players[slot].unit);
    }
    return true;
}

bool enter(uintptr_t unit) {
    // player 1 on an XInput pad: his camera and the boss camera (sMediator +0xD4, player 1's;
    // uCameraBoss::moveRightStickAxisY) read the stick from sDevil4Pad::mpInstance, so give them his stand-in (the
    // game's player and camera stay as they are)
    if (coop_pads::uses_stand_in(0) && unit) {
        const uintptr_t med = at<uintptr_t>(kMediator, 0);
        if (med && ((players[0].cam && unit == (uintptr_t)players[0].cam) || unit == at<uintptr_t>(med, kMedBossCamera))) {
            return swap_to(at<uintptr_t>(med, kMedPlayer), 0, (uintptr_t)coop_pads::pad_for(0));
        }
    }
    const int cam_owner = camera_owner(unit);
    if (cam_owner >= 0) {
        if (!swap_for(cam_owner, unit)) {
            return false;
        }
        cameras_in(cam_owner, unit);   // his own rail / pan camera objects
        return true;
    }
    const int own = own_unit_owner(unit);
    if (own >= 0) {
        return swap_for(own);
    }
    if (!any_extra()) {
        return false;
    }
    // a Grim Grip's base or point: as the player using it (player 1: no swap); the point's target values for player 1
    // are computed again after, as for an enemy
    if (const int grip = grip_owner(unit); grip != -2) {
        if (grip >= 1 && ready(players[grip].unit) && swap_for(grip, 0, false)) {
            enemy_swapped = unit;
            return true;
        }
        return false;
    }
    // a held enemy: the holding player (player 1 too: no swap), the aggro follows
    const int owner = owner_of(unit);
    int target = owner;
    if (owner >= 0) {
        if (Aggro* a = find_aggro(unit)) { a->slot = owner; a->hold = kHitHold; }
    } else {
        target = enemy_target(unit);
    }
    // an enemy (its shells, the boss Dante) after a player 2-4: his player, his camera (camera shakes) and pad, not
    // his lock-on values (its own targets get refilled for him; player 1's are put back after)
    if (target >= 1 && ready(players[target].unit) && swap_for(target, 0, false)) {
        enemy_swapped = unit;
        return true;
    }
    return false;
}

bool enter_player(uintptr_t player) {
    const int slot = slot_of(player);
    return slot >= 0 && swap_for(slot);
}

void leave() {
    if (!swapped) {
        return;
    }
    cameras_out();
    restore_lock();
    view_slot = -1;
    if (const uintptr_t med = at<uintptr_t>(kMediator, 0)) {
        at<uintptr_t>(med, kMedPlayer) = saved_player;
        if (camera_swapped) {
            at<uintptr_t>(med, kMedCamera) = saved_camera;
        }
        if (snatch_camera_slot >= 1) {
            snatch_camera[snatch_camera_slot] = at<uint8_t>(med, kMedSnatchCamera);
            at<uint8_t>(med, kMedSnatchCamera) = saved_snatch_camera;
        }
    }
    snatch_camera_slot = -1;
    if (pad_swapped) {
        at<uintptr_t>(kPadInstance, 0) = saved_pad;
    }
    swapped = false;
    camera_swapped = false;
    pad_swapped = false;
    if (enemy_swapped) {
        refresh_player1_lock(enemy_swapped);
        enemy_swapped = 0;
    }
}

bool hit_enter(uintptr_t attacker_group, uintptr_t target) {
    if (!attacker_group || !target || !any_extra()) {
        return false;
    }
    const uintptr_t mgr = at<uintptr_t>(attacker_group, kGroupMgr);
    const uintptr_t attacker = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
    const int a_slot = player_slot(attacker);
    const int t_slot = player_slot(target);
    AggroDiag& d = aggro_diag;
    ++d.hits;
    d.last_attacker = a_slot;
    d.last_target = t_slot;
    if (a_slot >= 0 && t_slot < 0) {
        if (Aggro* a = find_aggro(target); a && (a->slot == a_slot || !target_locked(*a))) { a->slot = a_slot; a->hold = kHitHold; }
    }
    // the hit counts for the player hit, else for the attacker: damage, Devil Trigger gain, style, orbs, reactions
    const int slot = t_slot >= 1 ? t_slot : (t_slot < 0 && a_slot >= 1 ? a_slot : -1);
    if (slot >= 1 && ready(players[slot].unit) && swap_for(slot, 0, false)) {
        ++d.hit_swaps;
        return true;
    }
    return false;
}

void note_grab_hit(uintptr_t hand_owner, bool did_swap, const void* target_group) {
    GrabDiag& d = grab_diag;
    ++d.hits;
    d.swapped_hits += did_swap ? 1 : 0;
    d.owner = hand_owner;
    d.swapped = did_swap;
    d.owner_slot = -1;
    for (int i = 0; i < kMaxPlayers; i++) {
        if (hand_owner && players[i].unit == hand_owner) d.owner_slot = i;
    }
    const uintptr_t med = at<uintptr_t>(kMediator, 0);
    d.med_player = med ? at<uintptr_t>(med, kMedPlayer) : 0;
    d.enemy = 0;
    // grabHitMessageNormal: the enemy = target group -> mpCollisionMgr (+0x98) -> actor (+0xE4)
    if (const uintptr_t mgr = target_group ? at<uintptr_t>((uintptr_t)target_group, 0x98) : 0) {
        d.enemy = at<uintptr_t>(mgr, 0xE4);
    }
    if (d.enemy) {
        for (int k = 0; k < 3; k++) {
            d.start[k] = at<float>(d.enemy, 0x1D20 + 4 * k);   // uEm000Base::mGrabStartPos (other classes: whatever is there)
            d.end[k] = at<float>(d.enemy, 0x1D30 + 4 * k);     // uEm000Base::mGrabEndPos
        }
    }
}

void game_tick(bool extra_players) {
    for (int i = 0; i < kMaxPlayers; i++) {
        StandDiag& s = stand_diag[i];
        s = StandDiag{};
        s.player = players[i].unit;
        if (!s.player || at<uintptr_t>(s.player, 0) != kNeroVtable) continue;
        s.stand = at<uintptr_t>(s.player, 0xCDF8);              // uPlayerNero::mpDevilStand
        s.hand = s.stand ? at<uintptr_t>(s.stand, 0x1374) : 0;  // uPlNeroDevil::mpRightHand
        if (s.hand) {
            s.hand_owner = at<uintptr_t>(s.hand, 0x2F34);       // uPlWpRightHand::mpParentPl
            for (int k = 0; k < 3; k++) s.ret[k] = at<float>(s.hand, 0x33E0 + 4 * k);   // mRetPos
        }
    }
    ++g_frame;
    coop_diag::set_forced(extra_players);
    coop_diag::game_tick();
    for (Hold& h : holds) {
        if (h.ticks > 0 && --h.ticks == 0) {
            h.enemy = 0;
        }
    }
    // the game tick also runs on frames without a unit update (above 60 fps): keep the last frame that had swaps
    static int idle = 0;
    if (swaps) { swaps_shown = swaps; idle = 0; }
    else if (++idle > 120) swaps_shown = 0;
    swaps = 0;
}

void reset() {
    for (Hold& h : holds) {
        h = Hold{};
    }
    for (Aggro& a : aggro) {
        a = Aggro{};
    }
    for (Grip& g : grips) {
        g = Grip{};
    }
    for (uint8_t& c : snatch_camera) {
        c = 0;
    }
    snatch_camera_slot = -1;
    enemy_swapped = 0;
    cam_held_n = 0;   // the room's rDevilCamera goes with it
    for (int i = 0; i < kMaxPlayers; i++) {
        cam_copies[i].clear();
        cam_copies_res[i] = 0;
    }
    lock_saved_n = 0;
    lock_applied = false;
    lock_applied_slot = -1;
    lock_swapped = false;
    swapped = false;
    camera_swapped = false;
    pad_swapped = false;
    view_slot = -1;
}

uint32_t swaps_last_frame() { return swaps_shown; }

bool lock_on_begin(uintptr_t player) {
    const int slot = slot_of(player);
    if (slot < 1 || !at<uintptr_t>(kLockOnMgr, 0)) {
        return false;
    }
    if (lock_applied && lock_applied_slot == slot) {
        return false;                     // inside his own update: his values are in already
    }
    lock_swapped = swap_for(slot);
    return lock_swapped;
}

void inter_flags_for(uintptr_t player) {
    const uintptr_t mgr = at<uintptr_t>(kLockOnMgr, 0);
    if (!mgr || !player) return;
    bool extra = false;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) extra |= players[i].unit != 0;
    if (!extra) return;                                   // one player: the game's own flags are his
    bool known = false;
    for (int i = 0; i < kMaxPlayers; i++) known |= players[i].unit == player;
    if (!known) return;
    set_inter_flags(mgr, player);
}

void lock_on_report(uintptr_t player) {
    const int slot = slot_of(player);
    if (slot < 1 || !lock_applied || lock_applied_slot != slot) return;
    LockDiag& d = lock_diag[slot];   // what the game picked from his numbers, while they are in
    d.targets = lock_saved_n;
    d.valid = 0;
    d.nearest = 0;
    d.nearest_len = -1.0f;
    d.stick = at<int>(player, 0x141C);          // mInterface.mAnlg.pow (0 = centred)
    d.picked = at<uintptr_t>(player, 0x3088);   // mLockOn.pInterTarget
    d.picked_len = -1.0f;
    for (int i = 0; i < lock_saved_n; i++) {
        const uintptr_t t = lock_saved[i].target;
        if (!pickable(t)) continue;
        ++d.valid;
        const float len = at<float>(t, kTgtLength);
        if (!d.nearest || len < d.nearest_len) { d.nearest = t; d.nearest_len = len; d.nearest_in = at<uint8_t>(t, kTgtIn); }
        if (t == d.picked) { d.picked_len = len; d.picked_in = at<uint8_t>(t, kTgtIn); }
    }
}

void lock_on_end() {
    if (lock_swapped) {
        leave();
        lock_swapped = false;
    }
}

void draw_diag() {
    {
        int per_slot[kMaxPlayers] = {};
        int live = 0;
        for (const Aggro& a : aggro) {
            if (!a.enemy || a.frame == 0xFFFFFFFFu || g_frame - a.frame > 2) continue;
            ++live;
            if (a.slot >= 0 && a.slot < kMaxPlayers) ++per_slot[a.slot];
        }
        const AggroDiag& ad = aggro_diag;
        ImGui::Text(_("Enemies: %d, after P1 %d / P2 %d / P3 %d / P4 %d"), live, per_slot[0], per_slot[1], per_slot[2], per_slot[3]);
        ImGui::Text(_("Hits: %u, run as a player 2-4: %u, last attacker P%d target P%d (0 = not a player)"), ad.hits,
                    ad.hit_swaps, ad.last_attacker + 1, ad.last_target + 1);
    }
    const GrabDiag& d = grab_diag;
    ImGui::Text(_("Grab hits: %u, swapped: %u"), d.hits, d.swapped_hits);
    ImGui::Text(_("Last: hand owner %08X (player %d), swapped %s, mpPlayer during it %08X"), (unsigned)d.owner,
                d.owner_slot + 1, d.swapped ? _("yes") : _("no"), (unsigned)d.med_player);
    ImGui::Text(_("Enemy %08X start %.0f %.0f %.0f end %.0f %.0f %.0f"), (unsigned)d.enemy, d.start[0], d.start[1],
                d.start[2], d.end[0], d.end[1], d.end[2]);
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const LockDiag& l = lock_diag[i];
        ImGui::Text(_("P%d lock-on: %u updates, %d targets (%d pickable), stick %d, center %.0f %.0f %.0f"), i + 1, l.updates,
                    l.targets, l.valid, l.stick, l.center[0], l.center[1], l.center[2]);
        ImGui::Text(_("   picked %08X dist %.0f in %d | nearest %08X dist %.0f in %d"), (unsigned)l.picked, l.picked_len,
                    l.picked_in, (unsigned)l.nearest, l.nearest_len, l.nearest_in);
    }
    for (int i = 0; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const StandDiag& s = stand_diag[i];
        ImGui::Text(_("P%d %08X stand %08X hand %08X hand owner %08X ret %.0f %.0f %.0f"), i + 1, (unsigned)s.player,
                    (unsigned)s.stand, (unsigned)s.hand, (unsigned)s.hand_owner, s.ret[0], s.ret[1], s.ret[2]);
    }
}

// Players 2-4's copies of the rail / pan camera objects: per slot the resource, then each copy (object, vtable, bytes).
void save_cam_copies(std::vector<uint8_t>& out) {
    auto put = [&](const void* p, size_t n) {
        out.insert(out.end(), static_cast<const uint8_t*>(p), static_cast<const uint8_t*>(p) + n);
    };
    for (int slot = 0; slot < kMaxPlayers; slot++) {
        put(&cam_copies_res[slot], sizeof cam_copies_res[slot]);
        const uint32_t n = (uint32_t)cam_copies[slot].size();
        put(&n, sizeof n);
        for (const CamCopy& c : cam_copies[slot]) {
            const uint32_t size = (uint32_t)c.mine.size();
            put(&c.obj, sizeof c.obj);
            put(&c.vtable, sizeof c.vtable);
            put(&size, sizeof size);
            put(c.mine.data(), size);
        }
    }
}

void load_cam_copies(const uint8_t* in, size_t bytes) {
    const uint8_t* end = in + bytes;
    auto get = [&](void* p, size_t n) {
        if ((size_t)(end - in) < n) return false;
        std::memcpy(p, in, n);
        in += n;
        return true;
    };
    for (int slot = 0; slot < kMaxPlayers; slot++) {
        uint32_t n = 0;
        if (!get(&cam_copies_res[slot], sizeof cam_copies_res[slot]) || !get(&n, sizeof n)) return;
        cam_copies[slot].clear();
        cam_copies[slot].reserve(n);
        for (uint32_t k = 0; k < n; k++) {
            CamCopy c;
            uint32_t size = 0;
            if (!get(&c.obj, sizeof c.obj) || !get(&c.vtable, sizeof c.vtable) || !get(&size, sizeof size)) return;
            c.mine.resize(size);
            if (!get(c.mine.data(), size)) return;
            cam_copies[slot].push_back(std::move(c));
        }
    }
}

void register_rollback() {
    rollback::add_range("Coop frame", &g_frame, sizeof g_frame);
    rollback::add_range("Coop lock-on cache", lock_cache, sizeof lock_cache);
    rollback::add_range("Coop holds", holds, sizeof holds);
    rollback::add_range("Coop aggro", aggro, sizeof aggro);
    rollback::add_range("Coop snatch camera", snatch_camera, sizeof snatch_camera);
    rollback::add_range("Coop Grim Grips", grips, sizeof grips);
    rollback::add("Coop rail cameras", &save_cam_copies, &load_cam_copies);
}

} // namespace coop_context
