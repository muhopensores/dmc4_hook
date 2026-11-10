#include "CoopContext.hpp"
#include "Coop.hpp"
#include "CoopDiag.hpp"
#include "../../mod.hpp"
#include "../../sdk/Cam.hpp"

namespace {

using coop::players;
using coop::kMaxPlayers;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

constexpr uintptr_t kMediator     = 0xE558B8; // sMediator::mpInstance
constexpr uintptr_t kMedPlayer    = 0x24;     // sMediator::mpPlayer
constexpr uintptr_t kMedCamera    = 0xD0;     // sMediator::mpCamera
constexpr uintptr_t kNeroVtable   = 0xBE4FA0; // uPlayerNero::vftable
constexpr uintptr_t kLowerAction  = 0x1500;   // uPlayer::mBdyAction[1].no
constexpr uintptr_t kAtckId       = 0x1564;   // uPlayer::mAtckId
constexpr uintptr_t kGrabActor    = 0xCCD0;   // uPlayerNero::mGrabHitInfo.actor
constexpr uint32_t  kActionAttack = 0xC;
constexpr int       kGrace        = 60;       // game ticks after the grab move ends

struct Hold { uintptr_t enemy = 0; int ticks = 0; };

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

struct SavedTarget { uintptr_t target; float length; uint8_t in, camera_in; float near_pos[3]; };
SavedTarget lock_saved[kMaxTargets];
int         lock_saved_n = 0;
bool        lock_swapped = false;

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
uintptr_t saved_player = 0, saved_camera = 0;
bool      swapped = false, camera_swapped = false;
uint32_t  swaps = 0, swaps_shown = 0;

bool grab_attack(uint32_t id) {
    return id == 1 || id == 2 || id == 5 || id == 6 || id == 7 || id == 8;
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

// The extra player holding `unit`, -1 if none.
int owner_of(uintptr_t unit) {
    const int n = (int)Coop::player_num < kMaxPlayers ? (int)Coop::player_num : kMaxPlayers;
    for (int i = 1; i < n; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl || at<uintptr_t>(pl, 0) != kNeroVtable) {
            continue;
        }
        if (at<uint32_t>(pl, kLowerAction) == kActionAttack && grab_attack(at<uint32_t>(pl, kAtckId)) &&
            at<uintptr_t>(pl, kGrabActor) == unit) {
            holds[i] = {unit, kGrace};
            return i;
        }
    }
    for (int i = 1; i < n; i++) {
        if (holds[i].enemy == unit && holds[i].ticks > 0 && players[i].unit) {
            return i;
        }
    }
    return -1;
}

} // namespace

namespace coop_context {

bool swap_to(uintptr_t player, uintptr_t camera = 0) {
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
    swapped = true;
    ++swaps;
    return true;
}

bool enter(uintptr_t unit) {
    const int cam_owner = camera_owner(unit);
    if (cam_owner >= 0) {
        return swap_to(players[cam_owner].unit, unit);
    }
    const int owner = owner_of(unit);
    return owner >= 0 && swap_to(players[owner].unit);
}

bool enter_player(uintptr_t player) {
    if (!player) {
        return false;
    }
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        if (players[i].unit == player) {
            return swap_to(player);
        }
    }
    return false;
}

void leave() {
    if (!swapped) {
        return;
    }
    if (const uintptr_t med = at<uintptr_t>(kMediator, 0)) {
        at<uintptr_t>(med, kMedPlayer) = saved_player;
        if (camera_swapped) {
            at<uintptr_t>(med, kMedCamera) = saved_camera;
        }
    }
    swapped = false;
    camera_swapped = false;
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
    coop_diag::set_forced(extra_players);
    coop_diag::game_tick();
    for (Hold& h : holds) {
        if (h.ticks > 0 && --h.ticks == 0) {
            h.enemy = 0;
        }
    }
    swaps_shown = swaps;
    swaps = 0;
}

void reset() {
    for (Hold& h : holds) {
        h = Hold{};
    }
    swapped = false;
    camera_swapped = false;
}

uint32_t swaps_last_frame() { return swaps_shown; }

bool lock_on_begin(uintptr_t player) {
    int slot = -1;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        if (player && players[i].unit == player) slot = i;
    }
    const uintptr_t mgr = at<uintptr_t>(kLockOnMgr, 0);
    if (slot < 1 || !mgr) {
        return false;
    }
    lock_swapped = swap_to(player);
    float center[4] = {};
    const uintptr_t vtable = at<uintptr_t>(player, 0);
    reinterpret_cast<GetCenterPosFn>(at<uintptr_t>(vtable, kGetCenterPos))(player, 0, center);
    const sCamera* cam = at<const sCamera*>(kCamera, 0);
    const sCamera_ViewPort* vp = cam ? &cam->viewports[slot] : nullptr;
    if (vp && !vp->mpCamera) {
        vp = &cam->viewports[0];
    }
    lock_saved_n = 0;
    for (uintptr_t list : {kMgrTop, kMgrSnatchTop}) {
        int n = 0;
        for (uintptr_t t = at<uintptr_t>(mgr, list); t && n < kMaxTargets; t = at<uintptr_t>(t, kTgtNext), ++n) {
            if (lock_saved_n >= kMaxTargets) break;
            SavedTarget& s = lock_saved[lock_saved_n++];
            s.target = t;
            s.length = at<float>(t, kTgtLength);
            s.in = at<uint8_t>(t, kTgtIn);
            s.camera_in = at<uint8_t>(t, kTgtCameraIn);
            for (int k = 0; k < 3; k++) s.near_pos[k] = at<float>(t, kTgtNearPos + 4 * k);

            const uintptr_t actor = at<uintptr_t>(t, kTgtModel);
            if (!actor) continue;
            float length = s.length, near_pos[4] = {s.near_pos[0], s.near_pos[1], s.near_pos[2], 0.0f};
            char in = (char)s.in;
            get_near_length(actor, center, &length, nullptr, &in, near_pos);   // also places the spheres in the world
            bool camera_in = false;
            const int spheres = at<int>(actor, kSphereCount);
            for (int k = 0; vp && k < spheres && k < 16 && !camera_in; k++) {
                camera_in = sphere_in_view(*vp, reinterpret_cast<const float*>(actor + kSpheres + k * kSphereStride));
            }
            at<float>(t, kTgtLength) = length;
            at<uint8_t>(t, kTgtIn) = (uint8_t)in;
            at<uint8_t>(t, kTgtCameraIn) = camera_in ? 1 : 0;
            for (int k = 0; k < 3; k++) at<float>(t, kTgtNearPos + 4 * k) = near_pos[k];
        }
    }
    return true;
}

void lock_on_end() {
    for (int i = 0; i < lock_saved_n; i++) {
        const SavedTarget& s = lock_saved[i];
        at<float>(s.target, kTgtLength) = s.length;
        at<uint8_t>(s.target, kTgtIn) = s.in;
        at<uint8_t>(s.target, kTgtCameraIn) = s.camera_in;
        for (int k = 0; k < 3; k++) at<float>(s.target, kTgtNearPos + 4 * k) = s.near_pos[k];
    }
    lock_saved_n = 0;
    if (lock_swapped) {
        leave();
        lock_swapped = false;
    }
}

void draw_diag() {
    const GrabDiag& d = grab_diag;
    ImGui::Text(_("Grab hits: %u, swapped: %u"), d.hits, d.swapped_hits);
    ImGui::Text(_("Last: hand owner %08X (player %d), swapped %s, mpPlayer during it %08X"), (unsigned)d.owner,
                d.owner_slot + 1, d.swapped ? _("yes") : _("no"), (unsigned)d.med_player);
    ImGui::Text(_("Enemy %08X start %.0f %.0f %.0f end %.0f %.0f %.0f"), (unsigned)d.enemy, d.start[0], d.start[1],
                d.start[2], d.end[0], d.end[1], d.end[2]);
    for (int i = 0; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const StandDiag& s = stand_diag[i];
        ImGui::Text(_("P%d %08X stand %08X hand %08X hand owner %08X ret %.0f %.0f %.0f"), i + 1, (unsigned)s.player,
                    (unsigned)s.stand, (unsigned)s.hand, (unsigned)s.hand_owner, s.ret[0], s.ret[1], s.ret[2]);
    }
}

} // namespace coop_context
