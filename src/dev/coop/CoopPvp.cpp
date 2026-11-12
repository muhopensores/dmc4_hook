#include "CoopPvp.hpp"
#include "Coop.hpp"
#include "CoopContext.hpp"
#include "../../mod.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MotionRegistry.hpp"
#include "../../sdk/ArcRows.hpp"
#include "../../mods/GerbShell.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

namespace coop_pvp {

bool hits = false;
bool push = true;
bool players_first = true;
bool dt_from_players = true;
float grab_grace = 2.0f;
float grab_window = 0.5f;
int flinch[3][5] = {{-1, -1, 3, 2, 1}, {-1, -1, -1, 3, 2}, {-1, -1, 1, 1, 1}};
int blow[3][5]   = {{-1, -1, -1, 2, 1}, {-1, -1, -1, 3, 1}, {-1, -1, -1, 2, 1}};

} // namespace coop_pvp

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

struct Vec3 { float x, y, z; };

// V1: hits
constexpr uintptr_t kSetVsAttr   = 0x50C650;   // uCollisionMgr::setVsAttr (eax = group, dl = mFrdAtk; mgr, kind, attr; retn 0Ch)
constexpr uintptr_t kOwnHitSite  = 0x47C45E;   // sCollisionGame::checkAttackObj: test dword ptr [edx+0Ch], ecx / jz 47C4AC
constexpr uintptr_t kGroupKind   = 0x04;       // cCollisionGroup::mKind
constexpr uintptr_t kGroupAttr   = 0x08;       // cCollisionGroup::mAttr (1 attack, 2 damage, 4 push, 8 grab)
constexpr uintptr_t kGroupVsAttr = 0x0C;       // cCollisionGroup::mVsAttr
constexpr uintptr_t kGroupClear  = 0x6C;       // cCollisionGroup::mClear
constexpr uintptr_t kGroupMgr    = 0x98;       // cCollisionGroup::mpCollisionMgr
constexpr uintptr_t kGroupNext   = 0x130;      // cCollisionGroup::mpNext[4] (attack, damage, push, grab chains)
constexpr uintptr_t kMgrReport   = 0xD8;       // uCollisionMgr::mpReportActor
constexpr uint32_t  kAttacksPlayers = 0x2;     // side 0's attack bit
constexpr uint32_t  kHitByPlayers   = 0x11;    // sides 0 and 1's damage bits

// Push: setVsAttr gives a player's push group mVsAttrPlPush (+0x2B8, 0x44444400: enemies only); GermanWord's
// Doppelganger reads mVsAttrEmPush (+0x2E0, 0x44444444) there instead (0x50C6B9), for every side-0 push group. Coop adds
// side 0's push bit to the players' own bodies' push groups. checkPush 0x47BDA0 pushes a pair when the outer group's
// mask has the inner's side (checkPushProcess) and the inner's has the outer's (0x47BEFE).
constexpr uint32_t  kPushPlayers    = 0x4;     // side 0's push bit
constexpr uintptr_t kPushPairSite   = 0x47BEFE;  // checkPush: test [edx+0Ch], ecx / jz 47BF52 (edx = inner group)

// V2: grabs
constexpr uintptr_t kGrabPassEnd     = 0x47BCB1;   // sCollisionGame::checkAttackMain after both checkGrabProcess (edi = this): cmp byte [edi+1Ch],0 / jnz
constexpr uintptr_t kChainTop        = 0x20;       // sCollisionGame::mppCollisionChainTop[side * 4 + chain] (0 attack, 1 damage, 2 push, 3 grab; checkGrabProcess reads [edi + side * 10h + 2Ch])
constexpr uintptr_t kCheckGroup      = 0x47D9A0;   // sCollisionGame::checkGroup_ (ecx = scratch; group, group, mode; retn 0Ch; al)
constexpr uintptr_t kGrabPre         = 0x4A7EB0;   // uActor::grabPreMessage (eax = target group; actor, grab group; retn 8)
constexpr uintptr_t kGrabTypeSite    = 0x83E2CB;   // grabHitMessageNormal: mov eax,[edi+0EA4h] (the target's mActorType)
constexpr uintptr_t kPlayerGrabMsg   = 0x8D4630;   // every uPlayer's vtable +0xDC grab message (xor eax,eax / retn 8)
constexpr uintptr_t kShortGrabType   = 0x7EB06E;   // setShortGrabReaction: cmp dword ptr [eax+0EA4h],3 (eax = the held actor)
constexpr uintptr_t kReactionSite    = 0x7EB1AE;   // setShortGrabEnemyReactionFunc: mov edx,[esi+0D664h] (not the boss Dante)
constexpr uintptr_t kMoveActDamage   = 0x8238B0;   // uPlayer::moveActDamage (eax = player)
constexpr uintptr_t kSetActionDamage = 0x7AA930;   // uPlayer::setActionDamage (eax = attack group or 0, ecx = player)
constexpr uintptr_t kSetCollInitPos  = 0x7A56F0;   // uPlayer::setCollisionInitPos (eax = player)
constexpr uintptr_t kSetFootwork     = 0x7AA2C0;   // uPlayer::setActionFootwork (esi = player; float interp, bool; retn 8)
constexpr uintptr_t kHandCtrlVtable  = 0xBEACC0;   // uPlWpRightHand::uCollisionCtrl (the Devil Bringer's grab collision)
constexpr uintptr_t kHandCtrlNero    = 0x1B84;     // its Nero (uPlWpRightHand::mpParentPl +0x2F34 - clsn +0x13B0)
constexpr uintptr_t kHandCtrlMode    = 0x137C;     // 1 Snatch, 3 Buster
constexpr int       kModeSnatch      = 1;
constexpr int       kModeBuster      = 3;
constexpr int       kReactSnatch     = 3;          // what uPlayerDanteBoss::grabMessage 0x7C5BC0 returns: Nero pulls the target
constexpr int       kReactBuster     = 9;          // the boss's first Buster reaction: Nero's mvAtckSGScare / SGAScare

// uPlayer
constexpr uintptr_t kDelta        = 0x10;      // cUnit: frames this tick
constexpr uintptr_t kPos          = 0x30;
constexpr uintptr_t kMotionFrame  = 0x348;     // uModel::MotionArray[0].mFrame
constexpr uintptr_t kActorType    = 0xEA4;     // uActor::mActorType (1 player, 3 enemy ...)
constexpr uintptr_t kActorStatus  = 0xEA8;
constexpr uintptr_t kVel          = 0xEC0;
constexpr uintptr_t kAcc          = 0xED0;
constexpr uintptr_t kPlayerId     = 0x1494;    // 0 Dante, 1 Nero
constexpr uintptr_t kActionNo     = 0x14F0;    // uPlayer::mActionNo (20 = the damage action)
constexpr uintptr_t kInvincible   = 0x150B;    // mIsInvincible
constexpr uintptr_t kHolyTimer    = 0x20A4;
constexpr uintptr_t kReviveTimer  = 0x20A8;
constexpr uintptr_t kLowerAction  = 0x1500;    // mBdyAction[1].no
constexpr uintptr_t kActStat     = 0x1554;    // uPlayer::mActStat (2 = in the air)
constexpr uintptr_t kDamageType   = 0x157C;    // mDamage.type (DMAGE_ID)
constexpr uintptr_t kDamageActor  = 0x1584;    // mDamage.pActor
constexpr uintptr_t kDamageAng    = 0x158C;    // mDamage.ang (moveDmgButtobi flies away from it)
constexpr uintptr_t kRotIsMove    = 0x1708;    // cRotateCtrl at +0x1704
constexpr uintptr_t kRotTarget    = 0x170C;
constexpr uintptr_t kRotNow       = 0x1710;
constexpr uintptr_t kMoveVel      = 0x1E1C;
constexpr uintptr_t kMoveAcc      = 0x1E20;
constexpr uintptr_t kCollMgr      = 0x1E8C;    // uCollisionMgr*: +0x1C mLand
constexpr uintptr_t kCoatMdl      = 0x20C4;    // uDevil4Model*: plays the body's motion numbers
constexpr uintptr_t kMotionEnd    = 0x2A54;    // mMotionFrameEnd[0]
constexpr uintptr_t kGrabActor    = 0xCCD0;    // uPlayerNero::mGrabHitInfo.actor
constexpr uintptr_t kBusterGrabOn = 0xCCE2;    // uPlayerNero: set by the Buster's first phase, cleared by his grabHitMessage
constexpr uint32_t  kActionAttack = 0xC;
constexpr int       kDmgBut       = 3;         // DMG_ID_BUT: moveDmgButtobi, the blow, landing and getting up
constexpr int       kActionDamage = 20;
constexpr int       kDmgPvpGrab   = 0x20;      // ours: past DMAGE_ID's last (0x1A); every reader compares, none indexes

// Dante's bank 8 (pl006_09, the boss's reactions: 0x800 pulled, 0x801 landing, 0x804 Buster, 0x80C landing in the air);
// a Nero gets the same file in his bank 9 (free: his setup never loads it; reserved from the motion registry), with
// Nero's own empty coat file of the Buster-on-Dante set on his coat (as setBusterMotionRsrc does for bank 15).
constexpr int kMotPulled = 0x00, kMotLand = 0x01, kMotBuster = 0x04, kMotBusterFall = 0x05, kMotBusterLand = 0x06,
              kMotBusterAir = 0x07, kMotAirLand = 0x0C;
constexpr int kNeroVictimBank = 9;
const char* kVictimLmt = "motion\\pl006\\pl006_09";
const char* kVictimCoatLmt = "motion\\pl000_03\\pl006\\pl000_03_pl006_00";
// The rows of em_dante.arc PvP needs, put in the resource cache while PvP hits is on (sdk/ArcRows): Nero's side of a
// Buster on a player (the boss Dante set setBusterMotionRsrc loads at grab time: body, coat, stand, arm) and the
// victims' motions.
constexpr uint32_t kMotionListDti = 0x139EE51D;   // rMotionList's arc class id
const char* kEmDanteArc = "rom\\enemy\\em_dante.arc";
const arc_rows::Row kRows[] = {
    {"motion\\pl000\\pl006\\pl000_pl006_00", kMotionListDti, kEmDanteArc},
    {"motion\\pl000_03\\pl006\\pl000_03_pl006_00", kMotionListDti, kEmDanteArc},
    {"motion\\pl020\\pl006\\pl020_pl006_00", kMotionListDti, kEmDanteArc},
    {"motion\\wp024\\pl006\\wp024_pl006_00", kMotionListDti, kEmDanteArc},
    {"motion\\pl006\\pl006_09", kMotionListDti, kEmDanteArc},
};
constexpr float kPi = 3.14159265f;

std::unique_ptr<FunctionHook> hook_vs_attr, hook_own_hit, hook_grab_pass, hook_grab_type, hook_grab_msg, hook_short_grab,
    hook_reaction, hook_move_damage, hook_damage_calc, hook_nero_hit, hook_hold_start, hook_hold_release;
uintptr_t vs_attr_original = 0;
uintptr_t ret_own_hit = 0;                     // 0x47C463: the pair is tested (checkGroup_)
uintptr_t ret_no_hit = 0x47C4AC;               // the next damage group
uintptr_t ret_forced_hit = 0x47C479;           // after a passing checkGroup_: the hit goes on to damagePreMessage
uintptr_t ret_push_pair = 0x47BF03, ret_no_push = 0x47BF52;   // checkPush: the pair is tested / the next group
std::unique_ptr<FunctionHook> hook_push_pair;
uintptr_t ret_grab_pass = 0x47BCB7, ret_grab_pass_cs = 0x47BCC0;
uintptr_t ret_grab_type = 0;
uintptr_t grab_msg_original = 0;
uintptr_t ret_short_grab_ok = 0x7EB07B, ret_short_grab_no = 0x7EB145;
uintptr_t ret_reaction = 0x7EB1B4, ret_reaction_boss = 0x7EB1D1;
uintptr_t move_damage_original = 0;
uintptr_t fn_check_group = kCheckGroup, fn_grab_pre = kGrabPre, fn_set_action_damage = kSetActionDamage,
    fn_set_coll_init_pos = kSetCollInitPos, fn_set_footwork = kSetFootwork;

arc_rows::Result rows;                         // the em_dante rows, while PvP hits is on
bool rows_tried = false;                       // a failed load isn't retried until the stage changes or PvP is toggled
uintptr_t victim_bank[coop::kMaxPlayers] = {}; // the Nero unit whose bank 9 holds the victim motions

// The player (0-3) an actor is or belongs to; the Devil Bringer's grab collision counts for its Nero.
int owner_of_actor(uintptr_t actor) {
    if (!actor) {
        return -1;
    }
    if (at<uintptr_t>(actor, 0) == kHandCtrlVtable) {
        return coop_context::player_of(at<uintptr_t>(actor, kHandCtrlNero));
    }
    if (const uintptr_t gerb_owner = gerb::shell_owner(actor)) {   // a Gerb jump's hitbox (dmc4_hook's GerbShell)
        return coop_context::player_of(gerb_owner);
    }
    return coop_context::player_of(actor);
}

int owner_of_mgr(uintptr_t mgr) {
    return mgr ? owner_of_actor(at<uintptr_t>(mgr, kMgrReport)) : -1;
}

// The player's own body (not his shells or weapons), as a player index; -1 otherwise. A pointer compare only: callers
// pass objects that may not be units (the folded grab-message stub), so no DTI call.
int body_player(uintptr_t actor) {
    if (!actor) {
        return -1;
    }
    for (int i = 0; i < (int)Coop::player_num && i < coop::kMaxPlayers; i++) {
        if (coop::players[i].unit == actor) {
            return i;
        }
    }
    return -1;
}

bool active() {
    return coop_pvp::hits && Coop::mod_enabled;
}

// ---- V1: hits ----

// Nero's charged shot (uPlNeroShl001, wp000.col groups, side 1) and the Gunslinger fan's bullets (uPlNeroShl000) passed
// through players in game while the Gunslinger dodge's bullets (uPlNeroShl000 too) hit: per class, what its attack
// groups get from setVsAttr, and how the hit pass weighs them against a player's damage group (Diagnostics).
constexpr uintptr_t kBulletVtable     = 0xBE8E40;   // uPlNeroShl000::vftable
constexpr uintptr_t kChargeShotVtable = 0xBE8FA8;   // uPlNeroShl001::vftable
struct ChargeDiag {
    uint32_t groups = 0, pairs = 0, overlaps = 0;
    int owner = -1;
    uint32_t kind = 0, attr = 0, vs = 0;
    float shot[3] = {}, body[3] = {}, dist = -1.0f;   // last pair: bounds centres (cCollisionGroup +0x70 min, +0x80 max)
    // closest pair since the reset: distance, the shell's own mPos then, the shot box's size, the shooter's mPos
    float min_dist = -1.0f, shell[3] = {}, size[3] = {}, shooter[3] = {};
    // ... and its shapes (cCollisionGroup +0x94 count, +0x9C cCollision*[]; cCollision +4 kCollPrim (+0 type, +0xC
    // radius), +8 scale, +0x10 / +0x20 its box): the shot's first shape, the body's shape count, and how many shot x
    // body shape pairs have overlapping boxes (checkGroup_'s first test, 0x47DB77)
    int shot_shapes = 0, body_shapes = 0, shape_overlaps = 0, shot_type = -1;
    float shot_radius = 0.0f, shot_scale = 0.0f, shot_box[3] = {};
    int body_bone = -2;   // the body group's first shape: joint (-1 = the model's own matrix), radius, centre
    float body_radius = 0.0f, body_centre[3] = {};
    // the hit itself: uActor::damagePreMessage calls with a player's body as the actor hit (Coop.cpp's hook), the
    // victim's HP then, and the attack ID the group carried
    uint32_t damage_calls = 0;
    float damage_hp = -1.0f;
    int damage_attack_id = -1;
    uint32_t forced = 0;   // hits Coop's own sweep let through (the charged shot)
    // the sweep checkGroup_ tests (0x47DBB6: an attack group's sphere becomes a capsule from cCollision +0x40 mMat0 to
    // +0xC0, the last frame's): their translation rows (+0x70, +0xF0), the body sphere's (+0x70), and the closest
    // distance from that segment to it minus both radii (<= 0 = touching)
    float sweep_now[3] = {}, sweep_old[3] = {}, sweep_body[3] = {}, sweep_gap = 0.0f;
};

bool shape_boxes_overlap(uintptr_t a, uintptr_t b) {
    for (int k = 0; k < 3; k++) {
        if (at<float>(a, 0x10 + 4 * k) > at<float>(b, 0x20 + 4 * k) || at<float>(b, 0x10 + 4 * k) > at<float>(a, 0x20 + 4 * k)) {
            return false;
        }
    }
    return true;
}

void bounds_centre(uintptr_t group, float* c) {
    for (int k = 0; k < 3; k++) {
        c[k] = 0.5f * (at<float>(group, 0x70 + 4 * k) + at<float>(group, 0x80 + 4 * k));
    }
}
ChargeDiag charge_diag[2];   // [0] Blue Rose bullets (uPlNeroShl000), [1] charged shots (uPlNeroShl001)

// The Buster's impact hit (uPlGrabShortHit) on a thrown player: made, its box found and given the players bit, weighed
// against a player by the hit pass (Diagnostics).
constexpr uintptr_t kImpactVtable = 0xBE8898;   // uPlGrabShortHit::vftable
struct ImpactDiag {
    uint32_t made = 0, armed = 0, no_box = 0, pairs = 0;
    uint32_t vs = 0;
    int owner = -1;
};
ImpactDiag impact_diag;

bool is_impact(uintptr_t mgr) {
    const uintptr_t r = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
    return r && at<uintptr_t>(r, 0) == kImpactVtable;
}

// 0 a Blue Rose bullet, 1 a charged shot, -1 neither.
int shot_class(uintptr_t mgr) {
    const uintptr_t r = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
    const uintptr_t vt = r ? at<uintptr_t>(r, 0) : 0;
    return vt == kBulletVtable ? 0 : vt == kChargeShotVtable ? 1 : -1;
}

void __stdcall after_vs_attr(uintptr_t group, uintptr_t mgr) {
    if (!group || !Coop::mod_enabled) {
        return;
    }
    if (coop_pvp::push && mgr && (at<uint32_t>(group, kGroupAttr) & 4) && body_player(at<uintptr_t>(mgr, kMgrReport)) >= 0) {
        at<uint32_t>(group, kGroupVsAttr) |= kPushPlayers;
    }
    const int shot = shot_class(mgr);
    const int owner = owner_of_mgr(mgr);
    if (owner >= 0 && coop_pvp::hits) {
        const uint32_t attr = at<uint32_t>(group, kGroupAttr);
        uint32_t& vs = at<uint32_t>(group, kGroupVsAttr);
        if (attr & 1) {
            vs |= kAttacksPlayers;
        }
        if (attr & 2) {
            vs |= kHitByPlayers;
        }
    }
    if (shot >= 0) {
        ChargeDiag& c = charge_diag[shot];
        c.groups++;
        c.owner = owner;
        c.kind = at<uint32_t>(group, 0x04);
        c.attr = at<uint32_t>(group, kGroupAttr);
        c.vs = at<uint32_t>(group, kGroupVsAttr);
    }
}

bool thrown_player(int i);   // after the grab code: player i is in a Buster throw
bool __stdcall check_group(void* scratch, uintptr_t grab_group, uintptr_t target_group, uint32_t mode);

// The player whose stand arm `arm` is (the Buster impact's +0x13C8), -1 if none.
int arm_owner(uintptr_t arm) {
    for (int i = 0; arm && i < (int)Coop::player_num && i < coop::kMaxPlayers; i++) {
        const uintptr_t pl = coop::players[i].unit;
        if (!pl || at<uint32_t>(pl, 0x1494) != 1) continue;
        const uintptr_t stand = at<uintptr_t>(pl, 0xCDF8);
        if (stand && at<uintptr_t>(stand, 0x1374) == arm) return i;
    }
    return -1;
}

// A player's shot (any uPlShl: Blue Rose bullets and charged shots, Ebony & Ivory, Coyote's pellets) against another
// player: at the hit test its collision shapes still sit where it was fired (cCollision +0x70 and +0xF0, the two ends
// checkGroup_ sweeps between, both at the spawn point, measured in game on the charged shot; the shotgun only hit point
// blank), so checkGroup_ misses the player it flies through. Coop sweeps the shot itself: each attack shape (its pos0,
// and pos1 for a capsule, turned by the shell's world matrix +0xA0) from the shell's position (+0x30) to where
// uPlShl::updatePos 0x830650 puts it next (+0x17A0), against every sphere of the body group (centre = the shape's
// mMat0 (+0x40 rows) times the prim's pos0 (+0x10), radius = prim +0xC x scale +8). A stopped shot (+0x17B4, set by
// uPlNeroShl001::hitMessage 0x82D820 and the like) is left alone, and a shot hits a player through Coop's sweep once.
constexpr uintptr_t kPlShlDti     = 0xE5AB70;   // uPlShl::DTI
constexpr uintptr_t kDtiParent    = 0x10;       // MtDTI::mp_parent
constexpr uintptr_t kGetDtiSlot   = 0x10;       // vtable: getDTI
constexpr uintptr_t kShellWmat    = 0xA0;       // uModel::mWmat
constexpr uintptr_t kShellNextPos = 0x17A0;
constexpr uintptr_t kShellStopped = 0x17B4;

bool is_player_shot(uintptr_t actor) {
    if (!actor || !at<uintptr_t>(actor, 0)) return false;
    uintptr_t dti = reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t, uintptr_t)>(at<uintptr_t>(at<uintptr_t>(actor, 0), kGetDtiSlot))(actor, 0);
    for (int depth = 0; dti && depth < 32; depth++, dti = at<uintptr_t>(dti, kDtiParent)) {
        if (dti == kPlShlDti) return true;
    }
    return false;
}

struct ForcedHit { uintptr_t shell = 0, victim = 0; uint32_t id = 0; };   // id = the attack group's mUniqueID (+0x14): shells reuse addresses
ForcedHit forced_hits[32];
int forced_next = 0;

bool already_forced(uintptr_t shell, uintptr_t victim, uint32_t id) {
    for (const ForcedHit& f : forced_hits) {
        if (f.shell == shell && f.victim == victim && f.id == id) return true;
    }
    return false;
}

bool swept_touch(const float* a, const float* b, float r, uintptr_t group) {
    const int n = at<int>(group, 0x94);
    const uintptr_t* shapes = at<uintptr_t*>(group, 0x9C);
    const float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
    for (int s = 0; shapes && s < n && s < 64; s++) {
        const uintptr_t sh = shapes[s];
        const uintptr_t prim = sh ? at<uintptr_t>(sh, 4) : 0;
        if (!prim || at<int>(prim, 0) != 0) continue;   // spheres
        float q[3];
        for (int k = 0; k < 3; k++) {
            q[k] = at<float>(prim, 0x10) * at<float>(sh, 0x40 + 4 * k) + at<float>(prim, 0x14) * at<float>(sh, 0x50 + 4 * k) +
                   at<float>(prim, 0x18) * at<float>(sh, 0x60 + 4 * k) + at<float>(sh, 0x70 + 4 * k);
        }
        const float w[3] = {q[0] - a[0], q[1] - a[1], q[2] - a[2]};
        float t = dd > 1e-6f ? (w[0] * d[0] + w[1] * d[1] + w[2] * d[2]) / dd : 0.0f;
        t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
        const float ex = a[0] + t * d[0] - q[0], ey = a[1] + t * d[1] - q[1], ez = a[2] + t * d[2] - q[2];
        const float reach = r + at<float>(prim, 0xC) * at<float>(sh, 8);
        if (ex * ex + ey * ey + ez * ez <= reach * reach) return true;
    }
    return false;
}

// One point of a shot's shape (local offset `ofs`) at the shell's position `at_pos`, turned by its world matrix.
void shot_point(uintptr_t shell, const float* at_pos, uintptr_t ofs, uintptr_t prim, float* out) {
    for (int k = 0; k < 3; k++) {
        out[k] = at_pos[k] + at<float>(prim, ofs) * at<float>(shell, kShellWmat + 4 * k) +
                 at<float>(prim, ofs + 4) * at<float>(shell, kShellWmat + 0x10 + 4 * k) +
                 at<float>(prim, ofs + 8) * at<float>(shell, kShellWmat + 0x20 + 4 * k);
    }
}

bool shot_touches(uintptr_t shell, uintptr_t attack_group, uintptr_t damage_group) {
    if (at<uint8_t>(shell, kShellStopped)) return false;
    const float now[3] = {at<float>(shell, 0x30), at<float>(shell, 0x34), at<float>(shell, 0x38)};
    const float next[3] = {at<float>(shell, kShellNextPos), at<float>(shell, kShellNextPos + 4), at<float>(shell, kShellNextPos + 8)};
    const int n = at<int>(attack_group, 0x94);
    const uintptr_t* shapes = at<uintptr_t*>(attack_group, 0x9C);
    for (int s = 0; shapes && s < n && s < 64; s++) {
        const uintptr_t sh = shapes[s];
        const uintptr_t prim = sh ? at<uintptr_t>(sh, 4) : 0;
        if (!prim) continue;
        const int type = at<int>(prim, 0);
        if (type != 0 && type != 3) continue;
        const float r = at<float>(prim, 0xC) * at<float>(sh, 8);
        for (int end = 0; end < (type == 3 ? 2 : 1); end++) {
            float a[3], c[3];
            shot_point(shell, now, end ? 0x20 : 0x10, prim, a);
            shot_point(shell, next, end ? 0x20 : 0x10, prim, c);
            if (swept_touch(a, c, r, damage_group)) return true;
        }
    }
    return false;
}

// 0 = the game tests the pair, 1 = no hit, 2 = a hit without the game's test (the charged shot, above). No hit: a
// player's own groups, and a player in a Buster throw hits nobody (he plays the boss Dante's motions, whose collision
// events switch on attack groups of his own .col: enemies and the grabbing Nero were hit at the landing).
int __stdcall same_player(uintptr_t attack_group, uintptr_t damage_group) {
    if (!Coop::mod_enabled || !attack_group || !damage_group) {
        return 0;
    }
    const uintptr_t amgr = at<uintptr_t>(attack_group, kGroupMgr);
    int attacker = owner_of_mgr(amgr);
    if (attacker < 0 && is_impact(amgr)) {
        attacker = arm_owner(at<uintptr_t>(at<uintptr_t>(amgr, kMgrReport), 0x13C8));
    }
    if (attacker >= 0 && !is_impact(amgr) && thrown_player(attacker)) {
        return 1;
    }
    const int target = owner_of_mgr(at<uintptr_t>(damage_group, kGroupMgr));
    const int shot = target >= 0 && target != attacker ? shot_class(amgr) : -1;
    if (shot >= 0) {
        ChargeDiag& c = charge_diag[shot];
        c.pairs++;
        bounds_centre(attack_group, c.shot);
        bounds_centre(damage_group, c.body);
        const float dx = c.shot[0] - c.body[0], dy = c.shot[1] - c.body[1], dz = c.shot[2] - c.body[2];
        c.dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (c.min_dist < 0.0f || c.dist < c.min_dist) {
            c.min_dist = c.dist;
            const uintptr_t shell = at<uintptr_t>(amgr, kMgrReport);
            const uintptr_t shooter = attacker >= 0 ? coop::players[attacker].unit : 0;
            for (int k = 0; k < 3; k++) {
                c.shell[k] = at<float>(shell, 0x30 + 4 * k);
                c.size[k] = at<float>(attack_group, 0x80 + 4 * k) - at<float>(attack_group, 0x70 + 4 * k);
                c.shooter[k] = shooter ? at<float>(shooter, 0x30 + 4 * k) : 0.0f;
            }
            const int ns = at<int>(attack_group, 0x94), nb = at<int>(damage_group, 0x94);
            const uintptr_t* ss = at<uintptr_t*>(attack_group, 0x9C);
            const uintptr_t* bs = at<uintptr_t*>(damage_group, 0x9C);
            c.shot_shapes = ns;
            c.body_shapes = nb;
            c.shape_overlaps = 0;
            c.shot_type = -1;
            for (int s = 0; ss && s < ns && s < 64; s++) {
                const uintptr_t sh = ss[s];
                if (!sh) continue;
                if (s == 0) {
                    const uintptr_t prim = at<uintptr_t>(sh, 4);
                    c.shot_type = prim ? at<int>(prim, 0) : -1;
                    c.shot_radius = prim ? at<float>(prim, 0xC) : 0.0f;
                    c.shot_scale = at<float>(sh, 8);
                    for (int k = 0; k < 3; k++) {
                        c.shot_box[k] = at<float>(sh, 0x20 + 4 * k) - at<float>(sh, 0x10 + 4 * k);
                    }
                }
                for (int d = 0; bs && d < nb && d < 64; d++) {
                    if (bs[d] && shape_boxes_overlap(sh, bs[d])) c.shape_overlaps++;
                }
            }
            if (ss && ns > 0 && ss[0] && bs && nb > 0 && bs[0]) {
                float a[3], o[3], q[3];
                for (int k = 0; k < 3; k++) {
                    a[k] = c.sweep_now[k] = at<float>(ss[0], 0x70 + 4 * k);
                    o[k] = c.sweep_old[k] = at<float>(ss[0], 0xF0 + 4 * k);
                    q[k] = c.sweep_body[k] = at<float>(bs[0], 0x70 + 4 * k);
                }
                float d[3] = {o[0] - a[0], o[1] - a[1], o[2] - a[2]}, w[3] = {q[0] - a[0], q[1] - a[1], q[2] - a[2]};
                const float dd = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
                float t = dd > 1e-6f ? (w[0] * d[0] + w[1] * d[1] + w[2] * d[2]) / dd : 0.0f;
                t = t < 0.0f ? 0.0f : t > 1.0f ? 1.0f : t;
                const float ex = a[0] + t * d[0] - q[0], ey = a[1] + t * d[1] - q[1], ez = a[2] + t * d[2] - q[2];
                const uintptr_t sp = at<uintptr_t>(ss[0], 4), bp = at<uintptr_t>(bs[0], 4);
                const float rs = sp ? at<float>(sp, 0xC) * at<float>(ss[0], 8) : 0.0f;
                const float rb = bp ? at<float>(bp, 0xC) * at<float>(bs[0], 8) : 0.0f;
                c.sweep_gap = std::sqrt(ex * ex + ey * ey + ez * ez) - rs - rb;
            }
            c.body_bone = -2;
            if (bs && nb > 0 && bs[0]) {
                const uintptr_t prim = at<uintptr_t>(bs[0], 4);
                c.body_bone = prim ? at<int>(prim, 4) : -2;
                c.body_radius = prim ? at<float>(prim, 0xC) * at<float>(bs[0], 8) : 0.0f;
                for (int k = 0; k < 3; k++) {
                    c.body_centre[k] = 0.5f * (at<float>(bs[0], 0x10 + 4 * k) + at<float>(bs[0], 0x20 + 4 * k));
                }
            }
        }
        alignas(16) uint8_t scratch[64];
        if (check_group(scratch, attack_group, damage_group, 1)) {
            c.overlaps++;
        }
    }
    if (target >= 0 && is_impact(amgr)) {
        impact_diag.pairs++;
        impact_diag.owner = attacker;
    }
    if (attacker >= 0 && attacker == target) {
        return 1;
    }
    const uintptr_t shell = at<uintptr_t>(amgr, kMgrReport);
    const uintptr_t victim = at<uintptr_t>(at<uintptr_t>(damage_group, kGroupMgr), kMgrReport);
    if (attacker >= 0 && target >= 0 && body_player(victim) >= 0 && is_player_shot(shell) && !already_forced(shell, victim, at<uint32_t>(attack_group, 0x14)) &&
        shot_touches(shell, attack_group, damage_group)) {
        forced_hits[forced_next] = {shell, victim, at<uint32_t>(attack_group, 0x14)};
        forced_next = (forced_next + 1) % 32;
        if (shot >= 0) charge_diag[shot].forced++;
        return 2;
    }
    return 0;
}

bool grabbed_player(int i);   // after the grab code: player i is held by a grab (Snatch, Buster, hold)

// Two players' bodies push each other unless it's off or one is in a grab (he sits on the grabbing Nero). true = no push.
bool __stdcall push_blocked(uintptr_t outer, uintptr_t inner) {
    if (!Coop::mod_enabled || !outer || !inner) {
        return false;
    }
    const int a = body_player(at<uintptr_t>(at<uintptr_t>(outer, kGroupMgr), kMgrReport));
    const int b = body_player(at<uintptr_t>(at<uintptr_t>(inner, kGroupMgr), kMgrReport));
    if (a < 0 || b < 0) {
        return false;
    }
    return !coop_pvp::push || a == b || grabbed_player(a) || grabbed_player(b);
}

// edx = the inner group, [ebp+8] = the outer one.
naked_extern void detour_push_pair() {
    __asm {
        test dword ptr [edx+0Ch], ecx
        jz no_push
        pushad
        push edx
        push dword ptr [ebp+8]
        call push_blocked
        test al, al
        popad
        jnz no_push
        jmp dword ptr [ret_push_pair]
    no_push:
        jmp dword ptr [ret_no_push]
    }
}

// setVsAttr's arguments are pushed again for the original, which pops them (retn 0Ch); eax is the group throughout.
naked_extern void detour_vs_attr() {
    __asm {
        cmp byte ptr [coop_pvp::hits], 0
        jne extra
        cmp byte ptr [coop_pvp::push], 0
        je plain
    extra:
        push dword ptr [esp+0Ch]
        push dword ptr [esp+0Ch]
        push dword ptr [esp+0Ch]
        call dword ptr [vs_attr_original]
        pushad
        push dword ptr [esp+24h]   // mgr: the first argument, above pushad and the return address
        push eax
        call after_vs_attr
        popad
        ret 0Ch
    plain:
        jmp dword ptr [vs_attr_original]
    }
}

// edx = the damage group, [ebp+8] = the attack group.
naked_extern void detour_own_hit() {
    __asm {
        test dword ptr [edx+0Ch], ecx
        jz no_hit
        cmp byte ptr [coop_pvp::hits], 0
        je hit
        pushad
        push edx
        push dword ptr [ebp+8]
        call same_player
        cmp eax, 1
        popad
        je no_hit
        ja forced
    hit:
        jmp dword ptr [ret_own_hit]
    no_hit:
        jmp dword ptr [ret_no_hit]
    forced:
        jmp dword ptr [ret_forced_hit]
    }
}

// ---- V2: grabs ----

naked_extern bool __stdcall check_group(void* scratch, uintptr_t grab_group, uintptr_t target_group, uint32_t mode) {
    __asm {
        mov ecx, [esp+4]
        push dword ptr [esp+10h]
        push dword ptr [esp+10h]
        push dword ptr [esp+10h]
        call dword ptr [fn_check_group]
        ret 10h
    }
}

naked_extern void __stdcall grab_pre(uintptr_t target_group, uintptr_t target, uintptr_t grab_group) {
    __asm {
        mov eax, [esp+4]
        push dword ptr [esp+0Ch]
        push dword ptr [esp+0Ch]
        call dword ptr [fn_grab_pre]
        ret 0Ch
    }
}

naked_extern void __stdcall set_action_damage(uintptr_t player) {
    __asm {
        mov ecx, [esp+4]
        xor eax, eax
        call dword ptr [fn_set_action_damage]
        ret 4
    }
}

naked_extern void __stdcall set_coll_init_pos(uintptr_t player) {
    __asm {
        mov eax, [esp+4]
        call dword ptr [fn_set_coll_init_pos]
        ret 4
    }
}

naked_extern void __stdcall set_footwork(uintptr_t player, float interp) {
    __asm {
        push esi
        mov esi, [esp+8]
        push 1
        push dword ptr [esp+10h]
        call dword ptr [fn_set_footwork]
        pop esi
        ret 8
    }
}

// The game's grab pass tests grab groups against sides 2-7 only (checkGrabProcess 0x47C4E0), and players have no
// grab-receiving groups. So each grab group of a Nero's hand is tested here against the other players' body damage
// groups with the game's own shape test, and a contact goes through the game's call (grabPreMessage -> the hand's
// grabHitMessage, which Coop runs in the hand's Nero's context).
struct Victim {
    bool active = false;
    int kind = 0;               // kModeSnatch / kModeBuster
    uintptr_t grabber = 0;
    int phase = 0;
    float timer = 0.0f;
    bool script = false;        // a Nero victim's .ctn script goes in on the tick after a motion request
    uint32_t ticks = 0;         // victim_move calls of this hold (Diagnostics)
    uintptr_t impact = 0;       // the Buster's impact hit, until its box gets the players bit
    uint32_t armor = 0;         // the victim's own uDamage mState bits 0-2, put back on release
    bool armored = false;
};

// Super armor while thrown, as the boss: uPlayerDanteBoss::damageMessage 0x7C54A0 -> setStatusSuperArmor 0x7C5B10 sets
// uDamage::mState (+0x84) |= 7 (1 no light reaction, 2 no flinch, 4 no blow) for every hit while he is in his Buster
// action (55), so Nero's throw hits and its landing impact hurt without knocking him out of it. Without it a held
// player was knocked down by the throw's own hits before Nero slammed him.
constexpr uintptr_t kDamageState = 0x15B4 + 0x84;   // uPlayer::mDamage (uDamage) mState
constexpr uint32_t  kArmorBits   = 7;

void armor_on(uintptr_t player, Victim& v) {
    uint32_t& s = at<uint32_t>(player, kDamageState);
    v.armor = s & kArmorBits;
    v.armored = true;
    s |= kArmorBits;
}

void armor_off(uintptr_t player, Victim& v) {
    if (v.armored) {
        uint32_t& s = at<uint32_t>(player, kDamageState);
        s = (s & ~kArmorBits) | v.armor;
        v.armored = false;
    }
}

// Why the last hold ended (Diagnostics): the reason, how long it lasted, and the grabbing Nero's state then.
enum HoldEnd { HOLD_NONE, HOLD_NO_MOTION, HOLD_NO_GRABBER, HOLD_PVP_OFF, HOLD_LET_GO, HOLD_MOTION_END, HOLD_LANDED,
               HOLD_FELL, HOLD_THROWN };
const char* kHoldEndNames[] = {"-", "victim motion didn't load", "grabber gone", "PvP off", "Nero let go",
                               "victim motion ended", "landed (Snatch)", "airborne end (Snatch)", "thrown (Buster done)"};
constexpr uintptr_t kReactionFn = 0xD66C;   // uPlayerNero: the grab reaction function (mvAtckSGScare ...), 0 = none
constexpr uintptr_t kActPhase   = 0x1504;   // uPlayer::mBdyAction[1].phase
struct HoldDiag {
    uint32_t holds = 0;
    int end = HOLD_NONE, kind = 0;
    uint32_t ticks = 0, nero_action = 0, nero_phase = 0;
    bool nero_holds_him = false, nero_motion_end = false;
    uintptr_t nero_fn = 0;
    bool on_hand = false;   // last held frame: placed by the joints (else at the arm's origin)
    float gap = -1.0f;      // its chest's distance from the hand before placing, cm
};
HoldDiag hold_diag;

void note_hold_end(const Victim& v, uintptr_t player, int why) {
    HoldDiag& d = hold_diag;
    d.end = why;
    d.kind = v.kind;
    d.ticks = v.ticks;
    const uintptr_t g = v.grabber;
    if (g) {
        d.nero_action = at<uint32_t>(g, kLowerAction);
        d.nero_phase = at<uint32_t>(g, kActPhase);
        d.nero_holds_him = at<uintptr_t>(g, kGrabActor) == player;
        d.nero_motion_end = at<bool>(g, kMotionEnd);
        d.nero_fn = at<uint32_t>(g, kPlayerId) == 1 ? at<uintptr_t>(g, kReactionFn) : 0;
    }
}
Victim victims[coop::kMaxPlayers];

// Grace after a grab: once a player's grab (Snatch, Buster, hold) ends, no Nero can grab him for coop_pvp::grab_grace
// seconds of his own game time, so a stunned player can't be snatched or bustered again and again.
bool was_grabbed[coop::kMaxPlayers] = {};
float grace_left[coop::kMaxPlayers] = {};   // seconds

// Grab window after a flinch: a player stays grabbable for coop_pvp::grab_window seconds of his own game time after his
// hit reaction ends (the PvP flinch thresholds make reactions short). Still refused while uPlayer::canBeGrabbed
// 0x7AEF60 would (invincible, Holy Water, getting up from a revive). The boss Dante has no window: canBeGrabbed lets
// him be grabbed at any time.
float window_left[coop::kMaxPlayers] = {};   // seconds

bool grabbed_player(int i) {
    return i >= 0 && i < coop::kMaxPlayers && victims[i].active;
}

bool thrown_player(int i) {
    if (i < 0 || i >= coop::kMaxPlayers || !victims[i].active || victims[i].kind != kModeBuster) {
        return false;
    }
    const uintptr_t pl = coop::players[i].unit;
    return pl && at<int>(pl, kDamageType) == kDmgPvpGrab;
}

// The player a grab group grabs for, -1 if none. The Snatch's box is on the Devil Bringer's collision (uPlWpRightHand::
// uCollisionCtrl, its mode at +0x137C); the Buster's is on Nero's own body collision, and uPlayerNero::grabHitMessage
// 0x7E67F0 hands a contact to his stand's arm (mpDevilStand +0xCDF8 -> +0x1374, vtable +0xD8) only while +0xCCE2 is
// set (the Buster's first phase sets it, the hand-over clears it). Outside that a contact would only put the group's ID
// in the victim's queue (grabPreMessage) and block the real one, so it isn't one.
int grab_source(uintptr_t reporter) {
    if (!reporter) {
        return -1;
    }
    if (at<uintptr_t>(reporter, 0) == kHandCtrlVtable) {
        return owner_of_actor(reporter);
    }
    const int i = body_player(reporter);
    if (i < 0 || at<uint32_t>(reporter, kPlayerId) != 1 || !at<bool>(reporter, kBusterGrabOn)) {
        return -1;
    }
    return i;
}

// How far grab attempts on players get, for the Diagnostics panel: frames a Nero's grab group was out, contacts with
// another player's body, contacts on a stunned player (grabPreMessage called), grab messages reached (the hand's ray
// checks and grabHitMessageNormal passed), and the last contact's victim action / DMAGE_ID.
struct GrabDiag {
    uint32_t hand_frames = 0, contacts = 0, sent = 0, messages = 0;
    uint32_t stunned_boxes = 0, stunned_off = 0, stunned_status = 0, stunned_apart = 0;   // a stunned player's body boxes while a hand is out
    uint32_t stunned_grace = 0;   // ... skipped: his grace after a grab was running
    int last_action = -1, last_type = -1, last_reaction = -1, last_mode = -1;
    bool last_stunned = false, last_invincible = false, last_window = false;
    float last_holy = 0.0f, last_revive = 0.0f;
};
GrabDiag grab_diag;

// A player can be grabbed only while stunned: in the damage action with a hit reaction (DMAGE_ID 1 HIT, 2 ZURI, 3 BUT,
// 5 KIRI, 6 HITIME launched, 10 COMBO, 11 AIR_COMBO), and not while uPlayer::canBeGrabbed 0x7AEF60 refuses
// (invincible, Holy Water, getting up from a revive).
bool grab_refused(uintptr_t player) {
    return at<bool>(player, kInvincible) || at<float>(player, kHolyTimer) > 0.0f || at<float>(player, kReviveTimer) > 0.0f;
}

bool stunned(uintptr_t player) {
    if (at<int>(player, kActionNo) != kActionDamage || grab_refused(player)) {
        return false;
    }
    switch (at<int>(player, kDamageType)) {
    case 1: case 2: case 3: case 5: case 6: case 10: case 11:
        return true;
    default:
        return false;
    }
}

// A Gerb jump's hitbox (dmc4_hook's GerbShell) builds its attack group itself, so setVsAttr never runs on it, and it
// writes its kind's mask back every tick (its unit line runs before the collision pass): the attacks-players bit goes
// on here, at the start of the attack pass (checkAttackMain 0x47BC53, before its eight checkAttackProcess calls), for
// its groups in the attack chains of sides 0 and 1.
constexpr uintptr_t kAttackPassStart = 0x47BC53;   // checkAttackMain: mov eax, 7 (edi = sCollisionGame)
uintptr_t ret_attack_pass = 0x47BC58;
std::unique_ptr<FunctionHook> hook_attack_pass;

void __stdcall arm_gerb_shells(uintptr_t game) {
    if (!active() || !game) {
        return;
    }
    const uintptr_t* chains = reinterpret_cast<const uintptr_t*>(game + kChainTop);
    for (int kind = 0; kind < 2; kind++) {
        for (uintptr_t g = chains[kind * 4 + 0]; g; g = at<uintptr_t>(g, kGroupNext)) {
            const uintptr_t mgr = at<uintptr_t>(g, kGroupMgr);
            if (mgr && gerb::shell_owner(at<uintptr_t>(mgr, kMgrReport))) {
                at<uint32_t>(g, kGroupVsAttr) |= kAttacksPlayers;
            }
        }
    }
}

naked_extern void detour_attack_pass() {
    __asm {
        pushad
        push edi
        call arm_gerb_shells
        popad
        mov eax, 7
        jmp dword ptr [ret_attack_pass]
    }
}

void __stdcall grab_players(uintptr_t game) {
    if (!active() || !game) {
        return;
    }
    alignas(16) uint8_t scratch[64];
    const uintptr_t* chains = reinterpret_cast<const uintptr_t*>(game + kChainTop);
    for (int kind = 0; kind < 2; kind++) {
        for (uintptr_t g = chains[kind * 4 + 3]; g; g = at<uintptr_t>(g, kGroupNext + 4 * 3)) {
            if (at<bool>(g, kGroupClear)) continue;
            const uintptr_t mgr = at<uintptr_t>(g, kGroupMgr);
            const uintptr_t src = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
            const int grabber = grab_source(src);
            if (grabber < 0) continue;
            if (!rows.arc) continue;   // Nero's Buster-on-Dante set and the victims' motions aren't in the cache
            grab_diag.hand_frames++;
            for (uintptr_t d = chains[0 * 4 + 1]; d; d = at<uintptr_t>(d, kGroupNext + 4 * 1)) {
                const uintptr_t dmgr = at<uintptr_t>(d, kGroupMgr);
                const uintptr_t target = dmgr ? at<uintptr_t>(dmgr, kMgrReport) : 0;
                const int victim = body_player(target);
                if (victim < 0 || victim == grabber) continue;
                const bool in_window = !stunned(target) && window_left[victim] > 0.0f && !grab_refused(target) &&
                                       !victims[victim].active;
                const bool is_stunned = stunned(target) || in_window;
                if (is_stunned) grab_diag.stunned_boxes++;
                if (at<bool>(d, kGroupClear)) {
                    if (is_stunned) grab_diag.stunned_off++;
                    continue;
                }
                if (at<int8_t>(target, kActorStatus) < 0) {
                    if (is_stunned) grab_diag.stunned_status++;
                    continue;
                }
                if (grace_left[victim] > 0.0f) {
                    if (is_stunned) grab_diag.stunned_grace++;
                    continue;
                }
                if (!check_group(scratch, g, d, 2)) {
                    if (is_stunned) grab_diag.stunned_apart++;
                    continue;
                }
                grab_diag.contacts++;
                grab_diag.last_action = at<int>(target, kActionNo);
                grab_diag.last_type = at<int>(target, kDamageType);
                grab_diag.last_invincible = at<bool>(target, kInvincible);
                grab_diag.last_holy = at<float>(target, kHolyTimer);
                grab_diag.last_revive = at<float>(target, kReviveTimer);
                grab_diag.last_stunned = is_stunned;
                grab_diag.last_window = in_window;
                if (!grab_diag.last_stunned) continue;
                grab_diag.sent++;
                grab_pre(d, target, g);
                break;
            }
        }
    }
}

naked_extern void detour_grab_pass() {
    __asm {
        pushad
        push edi
        call grab_players
        popad
        cmp byte ptr [edi+1Ch], 0
        jnz in_cs
        jmp dword ptr [ret_grab_pass]
    in_cs:
        jmp dword ptr [ret_grab_pass_cs]
    }
}

bool __stdcall is_pvp_target(uintptr_t actor) {
    return active() && body_player(actor) >= 0;
}

// grabHitMessageNormal handles actor types 3-6 only; a player (1) is taken down the enemy branch (3), the boss Dante's.
naked_extern void detour_grab_type() {
    __asm {
        mov eax, [edi+0EA4h]
        cmp eax, 1
        jne done
        pushad
        push edi
        call is_pvp_target
        test al, al
        popad
        jz done
        mov eax, 3
    done:
        jmp dword ptr [ret_grab_type]
    }
}

int motion_of(uintptr_t player, int block) {
    if (at<uint32_t>(player, kPlayerId) == 0) {
        return 0x800 | block;   // Dante: his own bank 8
    }
    const int i = body_player(player);
    if (i < 0 || !rows.arc) {
        return -1;
    }
    if (victim_bank[i] != player) {   // a new Nero unit (rooms rebuild the players): load his bank once
        if (!devil4_sdk::load_motion_bank((void*)player, kVictimLmt, kNeroVictimBank)) {
            return -1;
        }
        if (void* coat = at<void*>(player, kCoatMdl)) {
            devil4_sdk::load_motion_bank(coat, kVictimCoatLmt, kNeroVictimBank);
        }
        victim_bank[i] = player;
    }
    return (kNeroVictimBank << 8) | block;
}

const uint8_t kEndOnly[4] = {0xFF, 0, 0, 0};   // a .ctn script that only ends: no cancel window

bool play(uintptr_t player, Victim& v, int block, uint32_t attr, float interp, float start, float speed = 1.0f) {
    const int motion = motion_of(player, block);
    if (motion < 0) {
        return false;
    }
    if (at<uint32_t>(player, kPlayerId) != 0) {
        devil4_sdk::borrow_ctn((void*)player, 0, 0);   // for the request; the end-only script replaces it next tick
        v.script = true;
    }
    devil4_sdk::indexed_anim_call((uint32_t)motion, (uPlayer*)player, attr, speed, start, interp);
    return true;
}

void face(uintptr_t player, float yaw) {
    at<float>(player, kRotTarget) = yaw;
    at<float>(player, kRotNow) = yaw;
    at<bool>(player, kRotIsMove) = false;
}

void stop(uintptr_t player) {
    at<Vec3>(player, kVel) = {0.0f, 0.0f, 0.0f};
    at<Vec3>(player, kAcc) = {0.0f, 0.0f, 0.0f};
    at<float>(player, kMoveVel) = 0.0f;
    at<float>(player, kMoveAcc) = 0.0f;
}

bool landed(uintptr_t player) {
    const uintptr_t mgr = at<uintptr_t>(player, kCollMgr);
    return mgr && at<bool>(mgr, 0x1C);
}

Vec3 forward(float yaw, float len) {
    return {std::sin(yaw) * len, 0.0f, std::cos(yaw) * len};
}

// Let go: the game's blow (moveDmgButtobi) flies him away from where he faces, lands and gets him up.
void release_blown(uintptr_t player, Victim& v) {
    armor_off(player, v);
    at<float>(player, kDamageAng) = at<float>(player, kRotNow);
    at<int>(player, kDamageType) = kDmgBut;
    set_action_damage(player);
    at<uintptr_t>(player, kDamageActor) = v.grabber;
    v.active = false;
}

void release_footwork(uintptr_t player, Victim& v) {
    armor_off(player, v);
    v.active = false;
    at<int>(player, kDamageType) = 0;
    set_footwork(player, 10.0f);
}

// Nero's hold (enemy block): with the Buster button held, uPlayerNero::checkEnemyBlock 0x7E9060 turns the Buster into
// attack 7 / 8 (moveAtckRightHandShortEnemyBlock: one motion, then footwork) and his stand into action 7
// (uPlNeroDevil::moveActEmBlock 0x82AA80), which keeps holding while his mGrabHitInfo.actor (+0xCCD0) is set. A held
// enemy puts itself on the stand's arm every frame (uEm000Base::moveEx 0x543F44: the arm's mPos, its yaw + pi). The arm's
// mPos is the stand's own (its sync copies it; its LMT moves the hand), and the enemy's held motion is authored from there.
// The boss Dante is never held, so a held player's pose is the Buster motion paused at kHoldFrame, authored from the
// grabbing Nero's origin: his upper chest goes on the palm of the arm's hand instead, from the joints' world matrices.
constexpr uintptr_t kAtckId       = 0x1564;   // uPlayer::mAtckId
constexpr uintptr_t kNeroStand    = 0xCDF8;   // uPlayerNero::mpDevilStand
constexpr uintptr_t kStandArm     = 0x1374;   // uPlNeroDevil::mpRightHand
constexpr uintptr_t kStandAction  = 0x22A8;   // uPlNeroDevil::mActionNo
constexpr int       kStandEmBlock = 7;
constexpr int       kPhaseHeld    = 10;
constexpr float     kHoldFrame    = 20.0f;
constexpr uintptr_t kGetAngle     = 0xA32CC0;  // uCoord::getAngle (thiscall; MtVector3* out; returns it)

bool nero_throws(uintptr_t g, uintptr_t player) {
    const uint32_t atck = at<uint32_t>(g, kAtckId);
    return at<uint32_t>(g, kLowerAction) == kActionAttack && at<uintptr_t>(g, kGrabActor) == player &&
           atck != 7 && atck != 8;
}

bool nero_holds(uintptr_t g, uintptr_t player) {
    if (at<uint32_t>(g, kPlayerId) != 1 || at<uintptr_t>(g, kGrabActor) != player) {
        return false;
    }
    const uint32_t atck = at<uint32_t>(g, kAtckId);
    if (at<uint32_t>(g, kLowerAction) == kActionAttack && (atck == 7 || atck == 8)) {
        return true;
    }
    const uintptr_t stand = at<uintptr_t>(g, kNeroStand);
    return stand && at<int>(stand, kStandAction) == kStandEmBlock;
}

constexpr uintptr_t kJointNum   = 0x2E0;   // uModel::mJointNum
constexpr uintptr_t kJoints     = 0x2E4;   // uModel::mpJoint (uModel::Joint, 0x90 each)
constexpr uintptr_t kJointTable = 0x2E8;   // uModel::mJointTable (u8[256], joint number -> index, 255 = none)
constexpr uintptr_t kJointSize  = 0x90;
constexpr uintptr_t kJointWpos  = 0x80;    // Joint::mWmat (+0x50, row vectors): its translation row
// the arm model wp024: 2 wrist, 3 / 6 / 9 / 12 / 16 the fingers' first joints; the player: 2 / 3 upper spine / neck
constexpr int kArmWrist = 2, kArmFingers[5] = {3, 6, 9, 12, 16};
constexpr int kChestA = 2, kChestB = 3;

// World position of joint number n of a model, as its last matrix update left it.
bool joint_pos(uintptr_t model, int n, Vec3& out) {
    const uintptr_t joints = at<uintptr_t>(model, kJoints);
    const uintptr_t table = at<uintptr_t>(model, kJointTable);
    if (!joints || !table) {
        return false;
    }
    const uint8_t idx = at<uint8_t>(table, n & 0xFF);
    if (idx == 0xFF || idx >= at<uint32_t>(model, kJointNum)) {
        return false;
    }
    out = at<Vec3>(joints + idx * kJointSize, kJointWpos);
    return true;
}

// The palm: halfway from the wrist to the fingers' first joints (the ones the model has).
bool palm_pos(uintptr_t model, int wrist, const int* fingers, Vec3& out) {
    Vec3 w, f, sum = {};
    int n = 0;
    if (!joint_pos(model, wrist, w)) {
        return false;
    }
    for (int k = 0; k < 5; k++) {
        if (joint_pos(model, fingers[k], f)) {
            sum.x += f.x; sum.y += f.y; sum.z += f.z;
            n++;
        }
    }
    if (!n) {
        return false;
    }
    out = {0.5f * (w.x + sum.x / n), 0.5f * (w.y + sum.y / n), 0.5f * (w.z + sum.z / n)};
    return true;
}

void place_on_arm(uintptr_t player, uintptr_t g) {
    const uintptr_t stand = at<uintptr_t>(g, kNeroStand);
    const uintptr_t arm = stand ? at<uintptr_t>(stand, kStandArm) : 0;
    if (!arm) {
        return;
    }
    alignas(16) float ang[4] = {};
    using AngleFn = const float*(__fastcall*)(uintptr_t, uintptr_t, float*);
    const float* a = reinterpret_cast<AngleFn>(kGetAngle)(arm, 0, ang);
    float yaw = (a ? a[1] : 0.0f) + kPi;
    if (yaw > kPi) yaw -= 2.0f * kPi;
    stop(player);
    Vec3& pos = at<Vec3>(player, kPos);
    Vec3 hand, c0, c1;
    if (palm_pos(arm, kArmWrist, kArmFingers, hand) && joint_pos(player, kChestA, c0) && joint_pos(player, kChestB, c1)) {
        // the chest relative to his origin in last frame's matrices, turned from last frame's yaw to the new one
        const float dx = 0.5f * (c0.x + c1.x) - pos.x, dy = 0.5f * (c0.y + c1.y) - pos.y, dz = 0.5f * (c0.z + c1.z) - pos.z;
        const float t = yaw - at<float>(player, kRotNow), s = std::sin(t), c = std::cos(t);
        const float rx = dx * c + dz * s, rz = -dx * s + dz * c;
        hold_diag.gap = std::sqrt((hand.x - pos.x - dx) * (hand.x - pos.x - dx) + (hand.y - pos.y - dy) * (hand.y - pos.y - dy) +
                                  (hand.z - pos.z - dz) * (hand.z - pos.z - dz));
        hold_diag.on_hand = true;
        pos = {hand.x - rx, hand.y - dy, hand.z - rz};
    } else {
        hold_diag.on_hand = false;
        pos = at<Vec3>(arm, kPos);
    }
    face(player, yaw);
    set_coll_init_pos(player);
}

// The Buster's impact (uPlayerDanteBoss::setPlGrabShortHit 0x7C53B0): uPlGrabShortHit::create 0x8203E0 (edi = position;
// type, the actor hit, the grabbing Nero's stand arm; cdecl) at the victim's vtable +0x84 point. Its 20 cm attack box
// (uPlGrabShortHit::setup 0x820450) is made with its own shape, so setVsAttr never runs on it: cCollisionGroup::setAttr
// mode 1 gives it 0x22222200, no players; arm_impact adds the bit. Its owner for Coop: the arm (+0x13C8, CoopContext).
constexpr uintptr_t kGrabShortHitCreate = 0x8203E0;
constexpr int       kImpactType         = 0x91;
constexpr uintptr_t kVtblHitPoint       = 0x84;     // uActor: a point on the actor (into a buffer, returned)
constexpr uintptr_t kImpactMgr          = 0x13D0;   // uPlGrabShortHit's uCollisionMgr
constexpr uintptr_t kMgrGroups          = 0x358;    // uCollisionMgr::mppCollisionGroup
uintptr_t fn_grab_short_hit = kGrabShortHitCreate;

naked_extern uintptr_t __stdcall grab_short_hit(const float* pos, int type, uintptr_t target, uintptr_t arm) {
    __asm {
        push edi
        mov edi, [esp+8]
        push dword ptr [esp+14h]
        push dword ptr [esp+14h]
        push dword ptr [esp+14h]
        call dword ptr [fn_grab_short_hit]
        add esp, 0Ch
        pop edi
        ret 10h
    }
}

uintptr_t impact_hit(uintptr_t player, uintptr_t grabber) {
    const uintptr_t stand = at<uint32_t>(grabber, kPlayerId) == 1 ? at<uintptr_t>(grabber, kNeroStand) : 0;
    const uintptr_t arm = stand ? at<uintptr_t>(stand, kStandArm) : 0;
    if (!arm) {
        return 0;
    }
    alignas(16) float buf[4] = {};
    using PointFn = const float*(__fastcall*)(uintptr_t, uintptr_t, float*);
    const float* p = reinterpret_cast<PointFn>(at<uintptr_t>(at<uintptr_t>(player, 0), kVtblHitPoint))(player, 0, buf);
    impact_diag.made++;
    return grab_short_hit(p ? p : buf, kImpactType, player, arm);
}

void arm_impact(uintptr_t hit) {
    const uintptr_t group = at<uintptr_t>(hit + kImpactMgr, kMgrGroups);
    if (group && (at<uint32_t>(group, kGroupAttr) & 1)) {
        at<uint32_t>(group, kGroupVsAttr) |= kAttacksPlayers;
        impact_diag.armed++;
        impact_diag.vs = at<uint32_t>(group, kGroupVsAttr);
    } else {
        impact_diag.no_box++;
    }
}

// uPlayer::moveActDamage for a held player, from uPlayerDanteBoss::moveActSnatch 0x7C3EE0 / moveActBuster 0x7C4630
// (generic reactions only: no Devil Trigger version, no counter).
bool victim_move_impl(uintptr_t player);

// moveActDamage's own tail (0x8239B8): while the player's hit stop runs (+0x1E68 > 0) it sets +0x1201, which freezes his
// motion that frame (uPlayer::moveAction does the same for other actions, 0x821520). Replacing moveActDamage skipped it,
// so a thrown player ignored the 14 frames of hit stop Nero's throw starts with (uPlayerNero::moveEx 0x7E4150: +0xCDFC
// -> vtable +0xD0 on the held actor, the stand and Nero) and ran ahead of him.
constexpr uintptr_t kHitStop      = 0x1E68;
constexpr uintptr_t kHitStopFrame = 0x1201;

bool __stdcall victim_move(uintptr_t player) {
    if (!victim_move_impl(player)) {
        return false;
    }
    if (at<float>(player, kHitStop) > 0.0f) {
        at<uint8_t>(player, kHitStopFrame) = 1;
    }
    return true;
}

bool victim_move_impl(uintptr_t player) {
    if (at<int>(player, kDamageType) != kDmgPvpGrab) {
        return false;
    }
    const int i = body_player(player);
    if (i < 0 || !victims[i].active || !active()) {
        if (i >= 0) {
            note_hold_end(victims[i], player, HOLD_PVP_OFF);
            armor_off(player, victims[i]);
            victims[i].active = false;
        }
        at<int>(player, kDamageType) = kDmgBut;   // PvP turned off mid-grab: the game's blow
        set_action_damage(player);
        return true;
    }
    Victim& v = victims[i];
    v.ticks++;
    if (v.script) {
        devil4_sdk::install_ctn_script((void*)player, kEndOnly);
        v.script = false;
    }
    const uintptr_t g = v.grabber;
    if (!g || body_player(g) < 0) {
        note_hold_end(v, player, HOLD_NO_GRABBER);
        release_blown(player, v);
        return true;
    }
    const float dt = at<float>(player, kDelta);
    Vec3& pos = at<Vec3>(player, kPos);
    const Vec3 gpos = at<Vec3>(g, kPos);
    const float gyaw = at<float>(g, kRotNow);

    if (v.kind == kModeBuster) {
        // uPlayerDanteBoss::moveActBuster 0x7C4630, generic path: held (0x804, 0x807 with Nero in the air), then 107 cm
        // back and 0x805 falling (speed y -100, gravity -1.5), on landing 0x806 and the impact hit (setPlGrabShortHit
        // 0x91), footwork at its end. Nero's hold (enemy block) in between: phase kPhaseHeld.
        if (v.phase == kPhaseHeld) {
            if (nero_throws(g, player)) {   // let go into the throw (checkEnemyBlockFinish): from its start
                v.phase = 0;
            } else if (!nero_holds(g, player)) {
                note_hold_end(v, player, HOLD_LET_GO);
                release_blown(player, v);
                return true;
            } else {
                at<float>(player, kDelta) = at<float>(g, kDelta);
                place_on_arm(player, g);
                return true;
            }
        }
        if (v.phase <= 1) {
            if (v.phase == 1 && nero_holds(g, player)) {   // Nero kept the button held: the hold (checkEnemyBlock)
                stop(player);
                if (!play(player, v, kMotBuster, 0, 5.0f, kHoldFrame, 0.0f)) {
                    note_hold_end(v, player, HOLD_NO_MOTION);
                    release_blown(player, v);
                    return true;
                }
                v.phase = kPhaseHeld;
                place_on_arm(player, g);
                return true;
            }
            const bool holding = nero_throws(g, player);
            if (v.phase == 0) {
                stop(player);
                const int block = (at<uint32_t>(g, kActStat) & 2) ? kMotBusterAir : kMotBuster;
                if (!play(player, v, block, 0, 0.0f, 0.0f)) {
                    note_hold_end(v, player, HOLD_NO_MOTION);
                    release_blown(player, v);
                    return true;
                }
                v.phase = 1;
            }
            // in Nero's frame, as the boss: his position, facing him, his frame time
            at<float>(player, kDelta) = at<float>(g, kDelta);
            pos = gpos;
            float yaw = gyaw + kPi;
            if (yaw > kPi) yaw -= 2.0f * kPi;
            face(player, yaw);
            set_coll_init_pos(player);
            if (!holding) {
                note_hold_end(v, player, HOLD_LET_GO);
                release_blown(player, v);
                return true;
            }
            if (at<bool>(player, kMotionEnd)) {   // the motion's last pose stands 107 cm back
                const Vec3 off = forward(yaw, -107.4399f);
                pos.x += off.x;
                pos.z += off.z;
                if (!play(player, v, kMotBusterFall, 1, 0.0f, 0.0f)) {
                    note_hold_end(v, player, HOLD_NO_MOTION);
                    release_blown(player, v);
                    return true;
                }
                stop(player);
                at<float>(player, kVel + 4) = -100.0f;
                at<float>(player, kAcc + 4) = -1.5f;
                set_coll_init_pos(player);
                v.phase = 2;
            }
            return true;
        }
        if (v.phase == 2) {
            if (landed(player)) {
                stop(player);
                if (!play(player, v, kMotBusterLand, 4, 10.0f, 0.0f)) {
                    note_hold_end(v, player, HOLD_NO_MOTION);
                    release_footwork(player, v);
                    return true;
                }
                at<uint32_t>(player, kActStat) = (at<uint32_t>(player, kActStat) & ~3u) | 1u;
                v.impact = impact_hit(player, g);
                v.phase = 3;
            }
            return true;
        }
        if (v.impact) {   // its box is made in its setup, later in the frame it was created in (5 frames of life)
            arm_impact(v.impact);
            v.impact = 0;
        }
        if (at<bool>(player, kMotionEnd)) {
            note_hold_end(v, player, HOLD_THROWN);
            release_footwork(player, v);
        }
        return true;
    }

    // Snatch: pulled toward Nero, then landing
    switch (v.phase) {
    case 0: {
        face(player, std::atan2(gpos.x - pos.x, gpos.z - pos.z));
        stop(player);
        if (!play(player, v, kMotPulled, 1, 0.0f, 0.0f)) {
            note_hold_end(v, player, HOLD_NO_MOTION);
            release_footwork(player, v);
            return true;
        }
        v.timer = 9.0f;
        v.phase = 1;
        break;
    }
    case 1:
        v.timer -= dt;
        if (v.timer <= 0.0f) {
            Vec3 target = gpos;
            if (std::fabs(gpos.y - pos.y) >= 300.0f) {   // far above or below: stop 300 cm in front of him
                const Vec3 f = forward(gyaw, 300.0f);
                target.x += f.x;
                target.z += f.z;
            }
            at<Vec3>(player, kVel) = {(target.x - pos.x) * 0.1f, (target.y - pos.y) * 0.1f, (target.z - pos.z) * 0.1f};
            at<Vec3>(player, kAcc) = {0.0f, 0.0f, 0.0f};
            v.timer = 10.0f;
            v.phase = 2;
        }
        break;
    case 2:
        v.timer -= dt;
        if (v.timer <= 0.0f) {
            const float yaw = at<float>(player, kRotNow);
            const bool ground = landed(player);
            if (!play(player, v, ground ? kMotLand : kMotAirLand, 5, ground ? 0.0f : 3.0f, ground ? 0.0f : 5.0f)) {
                note_hold_end(v, player, HOLD_NO_MOTION);
                release_footwork(player, v);
                return true;
            }
            const Vec3 f = forward(yaw, ground ? 18.0f : 15.0f);
            at<Vec3>(player, kVel) = {f.x, 0.0f, f.z};
            at<Vec3>(player, kAcc) = {0.0f, -1.0f, 0.0f};
            v.phase = 3;
        }
        break;
    default:
        if (at<float>(player, kMotionFrame) > 15.0f) {
            at<float>(player, kVel) = 0.0f;
            at<float>(player, kVel + 8) = 0.0f;
        }
        if (at<bool>(player, kMotionEnd)) {
            if (landed(player)) {
                note_hold_end(v, player, HOLD_LANDED);
                stop(player);
                release_footwork(player, v);
            } else {
                note_hold_end(v, player, HOLD_FELL);
                release_blown(player, v);
            }
        }
        break;
    }
    return true;
}

naked_extern void detour_move_damage() {
    __asm {
        pushad
        push eax
        call victim_move
        test al, al
        popad
        jz plain
        ret
    plain:
        jmp dword ptr [move_damage_original]
    }
}

// Every uPlayer's grab message (vtable +0xDC: ecx = the actor, the grab group, the target group; retn 8). For a player
// grabbed by another: start his reaction and answer as the boss Dante does (3 = pulled, 9 = Nero's generic Buster).
// 0x8D4630 is a folded `xor eax,eax / retn 8` shared by 157 vtable slots of many classes, so nothing is read from the
// object or the arguments until the object is known to be a player's body (a pointer compare, no DTI call).
using GrabMsgFn = int(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

int __fastcall grab_message(uintptr_t actor, uintptr_t edx, uintptr_t grab_group, uintptr_t target_group) {
    const int victim = active() ? body_player(actor) : -1;
    const uintptr_t mgr = victim >= 0 && grab_group ? at<uintptr_t>(grab_group, kGroupMgr) : 0;
    const uintptr_t src = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
    const bool hand = src && at<uintptr_t>(src, 0) == kHandCtrlVtable;
    const int grabber = !src ? -1 : hand ? owner_of_actor(src) : body_player(src);
    if (victim < 0 || grabber < 0) {
        return reinterpret_cast<GrabMsgFn>(grab_msg_original)(actor, edx, grab_group, target_group);
    }
    if (grabber == victim) {
        return 0;
    }
    // The hand box's mMvType: 1 Snatch (MV_TYPE_LONG_GRAB), 2 the Devil Trigger Snatch (LONG_GRAB_D), 3 Buster
    // (SHORT_GRAB), 0 once the Snatch's ray hit something (update clears it); only 3 is a Buster. A Nero's own body box
    // (uPlayerNero::grabHitMessage, +0xCCE2) is the Buster.
    const int hand_mode = hand ? at<int>(src, kHandCtrlMode) : -1;
    const int mode = !hand || hand_mode == kModeBuster ? kModeBuster : kModeSnatch;
    grab_diag.last_mode = hand_mode;
    grab_diag.messages++;
    grab_diag.last_reaction = mode == kModeSnatch ? kReactSnatch : kReactBuster;
    hold_diag.holds++;
    Victim& v = victims[victim];
    armor_off(actor, v);
    v = {};
    v.active = true;
    v.kind = mode == kModeSnatch ? kModeSnatch : kModeBuster;
    v.grabber = coop::players[grabber].unit;
    if (v.kind == kModeBuster) {
        armor_on(actor, v);
    }
    at<int>(actor, kDamageType) = kDmgPvpGrab;
    set_action_damage(actor);
    at<uintptr_t>(actor, kDamageActor) = v.grabber;
    return v.kind == kModeSnatch ? kReactSnatch : kReactBuster;
}

// setShortGrabReaction goes on only for a held enemy (type 3); a held player too.
bool __stdcall held_player_ok(uintptr_t actor) {
    return is_pvp_target(actor);
}

// checkEnemyBlock 0x7E9128: cmp dword ptr [ecx+0EA4h],3 (ecx = the held actor; 3 goes on to the uEnemy DTI and
// vtable +0x188 checks at 0x7E9135, else 0x7E90A3 refuses). A held player skips them to the hold (0x7E915F): his
// vtable +0x188 is uPlayer's setStingerJumpEffect, not a can-be-held test.
uintptr_t ret_hold_enemy = 0x7E9135, ret_hold_refuse = 0x7E90A3, ret_hold_accept = 0x7E915F;

naked_extern void detour_hold_start() {
    __asm {
        cmp dword ptr [ecx+0EA4h], 3
        je enemy
        pushad
        push ecx
        call is_pvp_target
        test al, al
        popad
        jz refuse
        jmp dword ptr [ret_hold_accept]
    enemy:
        jmp dword ptr [ret_hold_enemy]
    refuse:
        jmp dword ptr [ret_hold_refuse]
    }
}

// setShortGrabEnemyReactionAfterEnemyBlock 0x7EB730 (esi = Nero; no frame), past its boss Dante refusal: 0x7EB75D
// switches on the held enemy's kind (+0x1410, a player's button state). A held player gets the enemies' default:
// reaction phase (+0xD668) 0 and true, so the throw (bossDantefunc, set by setShortGrabReaction) runs from its start.
uintptr_t ret_hold_release = 0x7EB763;

naked_extern void detour_hold_release() {
    __asm {
        pushad
        push dword ptr [esi+0D664h]
        call is_pvp_target
        test al, al
        popad
        jz plain
        mov dword ptr [esi+0D668h], 0
        mov al, 1
        ret
    plain:
        mov eax, [esi+0D664h]
        jmp dword ptr [ret_hold_release]
    }
}

naked_extern void detour_short_grab() {
    __asm {
        cmp dword ptr [eax+0EA4h], 3
        je ok
        pushad
        push eax
        call held_player_ok
        test al, al
        popad
        jz no
    ok:
        jmp dword ptr [ret_short_grab_ok]
    no:
        jmp dword ptr [ret_short_grab_no]
    }
}

// A held player gets the boss Dante's set (bank 27, pl000_pl006_00, bossDantefunc) with the Devil Trigger bit (edi & 2)
// dropped: the generic Scare / AScare whatever Nero's state.
naked_extern void detour_reaction() {
    __asm {
        pushad
        push dword ptr [esi+0D664h]
        call is_pvp_target
        test al, al
        popad
        jz plain
        and edi, 1
        jmp dword ptr [ret_reaction_boss]
    plain:
        mov edx, [esi+0D664h]
        jmp dword ptr [ret_reaction]
    }
}

// ---- V4: flinching ----
// uDamage::calc 0x51BE10 (esi = the uDamage, a player's at +0x15B4; attacker group, defender group; retn 8): each hit
// adds to the flinch / blow counters of the attack's level (kAttackStatus mAttackLvI / mAttackLvB), and a counter at
// its maximum flinches / blows the player; the maxima are copied every hit from mpDefendStatus (+0xB8: his .dfd entry,
// kDefendStatus: name[16], resist[3], mMaxInterrupt int16[5] +0x1C, mMaxBlown int16[5] +0x26; -1 = never). Counters
// reset after the reaction or 180 frames after the last hit (the uDamage's update 0x51B7D0). Players' d-normal flinches
// on every hit of level 2+, and every real player attack is level 2+: for a hit by another player the entry is swapped
// for a copy with the PvP thresholds (coop_pvp::flinch / blow; [1] while his entry is the Devil Trigger one, d-majin)
// for that call. "no-deffend" (no thresholds at all) is left as it is.
constexpr uintptr_t kDamageCalc   = 0x51BE10;
constexpr uintptr_t kPlayerDamage = 0x15B4;   // uPlayer::mDamage.mgr
constexpr uintptr_t kDefendStatus = 0xB8;     // uDamage::mpDefendStatus
constexpr size_t    kDefendSize   = 0x30;
uintptr_t damage_calc_original = 0;
alignas(4) uint8_t pvp_defend[3][kDefendSize];
uintptr_t swapped_damage = 0, swapped_entry = 0;

// The attacker's Devil Trigger for a hit on a player. uDamage::calc credits mpPlayer (DT and style) only when the hit
// unit's uDamage +0xC8 is 2 (enemies) or 7 (the boss Dante, set in his setup); players' is 0 (uPlayer::setup
// 0x7A7AF5), so a hit on a player paid nothing. After calc the attacker gets what calc gives for an enemy: the attack's
// mDTAdd (cCollisionGroup +0xEC) x the game's factor (0xDECDC8), not while in Devil Trigger (vtable +0x218), capped at
// his maximum. Style stays out (the style meter is shared).
constexpr uintptr_t kGroupDtAdd   = 0xEC;
constexpr uintptr_t kDtGainFactor = 0xDECDC8;
constexpr uintptr_t kIsDevilTrg   = 0x218;
constexpr uintptr_t kDtNow = 0x1F24, kDtMax = 0x1F28;
uintptr_t pvp_attacker = 0, pvp_attack_group = 0;

void __stdcall defend_begin(uintptr_t damage, uintptr_t attack_group, uintptr_t defend_group) {
    swapped_damage = 0;
    pvp_attacker = 0;
    if (!active() || !damage || !attack_group) {
        return;
    }
    const uintptr_t player = damage - kPlayerDamage;
    const int victim = body_player(player);
    const int attacker = owner_of_mgr(at<uintptr_t>(attack_group, kGroupMgr));
    if (victim < 0 || attacker < 0 || attacker == victim) {
        return;
    }
    pvp_attacker = coop::players[attacker].unit;
    pvp_attack_group = attack_group;
    const uintptr_t entry = at<uintptr_t>(damage, kDefendStatus);
    if (!entry || _strnicmp((const char*)entry, "no-deffend", 10) == 0) {
        return;
    }
    // in the air (not landed: launched, knocked up, jumping) the air set, so a launched player can be juggled; else his
    // Devil Trigger entry (d-majin) picks the DT set
    const int set = !landed(player) ? 2 : _strnicmp((const char*)entry, "d-majin", 7) == 0 ? 1 : 0;
    uint8_t* mine = pvp_defend[set];
    memcpy(mine, (const void*)entry, kDefendSize);
    for (int lv = 0; lv < 5; lv++) {
        reinterpret_cast<int16_t*>(mine + 0x1C)[lv] = (int16_t)coop_pvp::flinch[set][lv];
        reinterpret_cast<int16_t*>(mine + 0x26)[lv] = (int16_t)coop_pvp::blow[set][lv];
    }
    swapped_damage = damage;
    swapped_entry = entry;
    at<uintptr_t>(damage, kDefendStatus) = (uintptr_t)mine;
}

void __stdcall defend_end() {
    if (swapped_damage) {
        at<uintptr_t>(swapped_damage, kDefendStatus) = swapped_entry;
        swapped_damage = 0;
    }
    const uintptr_t pl = pvp_attacker;
    pvp_attacker = 0;
    if (!pl || !coop_pvp::dt_from_players || body_player(pl) < 0) {
        return;
    }
    const float gain = at<float>(pvp_attack_group, kGroupDtAdd) * at<float>(kDtGainFactor, 0);
    const bool in_dt = reinterpret_cast<bool(__thiscall*)(uintptr_t)>(at<uintptr_t>(at<uintptr_t>(pl, 0), kIsDevilTrg))(pl);
    if (gain <= 0.0f || in_dt) {
        return;
    }
    float& now = at<float>(pl, kDtNow);
    now = std::min(now + gain, at<float>(pl, kDtMax));
}

// uPlayerNero::hitMessage 0x7E6500 (thiscall; the attack group, the target group; retn 8) shakes the camera on some
// hits (uPlayerDanteBoss::setCameraVib -> sMediator::mpCamera +0xD0). A hit runs in the context of the player hit, so
// that shook the victim's camera: the hitting Nero's own camera is mpCamera for the call.
// It also requests his hit slow for mAtckId 0x24 / 0x25 without hit stop (uPlayer::cHitSlowCtrl::request 0x7B1F40:
// +0x1488 on, +0x148C duration, +0x1490 rate); while it runs uPlayer::main 0x7A8466 sets the game's work rate (sWorkRate
// +0x28) to the rate, mSpeed (0xB9A23C) after: the whole game slows, the other players too. On a player hit the slow
// runs at mSpeed: no slowdown, and what waits for its end (checkODHiTimeEfctAfterHitSlow's effect) still comes.
constexpr uintptr_t kNeroHitMessage = 0x7E6500;
constexpr uintptr_t kMediatorPtr    = 0xE558B8;
constexpr uintptr_t kMedCamera      = 0xD0;
constexpr uintptr_t kHitSlowOn      = 0x1488;
constexpr uintptr_t kHitSlowTime    = 0x148C;
constexpr uintptr_t kHitSlowRate    = 0x1490;
constexpr uintptr_t kNormalSpeed    = 0xB9A23C;   // mSpeed
using HitMessageFn = void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);
uintptr_t nero_hit_original = 0;

void __fastcall nero_hit_message(uintptr_t nero, uintptr_t edx, uintptr_t attack_group, uintptr_t target_group) {
    const uintptr_t med = at<uintptr_t>(kMediatorPtr, 0);
    const int i = Coop::mod_enabled ? body_player(nero) : -1;
    const uintptr_t cam = i >= 0 ? (uintptr_t)coop::players[i].cam : 0;
    const uintptr_t saved = med ? at<uintptr_t>(med, kMedCamera) : 0;
    const bool swap = med && cam && saved != cam;
    const bool slow_was = at<bool>(nero, kHitSlowOn);
    const float time_was = at<float>(nero, kHitSlowTime);
    if (swap) at<uintptr_t>(med, kMedCamera) = cam;
    reinterpret_cast<HitMessageFn>(nero_hit_original)(nero, edx, attack_group, target_group);
    if (swap) at<uintptr_t>(med, kMedCamera) = saved;
    const bool requested = at<bool>(nero, kHitSlowOn) && (!slow_was || at<float>(nero, kHitSlowTime) != time_was);
    if (requested && i >= 0 && active() && target_group) {
        const int victim = owner_of_mgr(at<uintptr_t>(target_group, kGroupMgr));
        if (victim >= 0 && victim != i) {
            at<float>(nero, kHitSlowRate) = at<float>(kNormalSpeed, 0);
        }
    }
}

naked_extern void detour_damage_calc() {
    __asm {
        pushad
        push dword ptr [esp+28h]   // defender group (above pushad and the return address: attacker +24h, defender +28h)
        push dword ptr [esp+28h]   // attacker group
        push esi
        call defend_begin
        popad
        push dword ptr [esp+8]
        push dword ptr [esp+8]
        call dword ptr [damage_calc_original]
        pushad
        call defend_end
        popad
        ret 8
    }
}

// ---- V2: lock-on ----
// Lock-on targets are cLockOnTarget (0x50 bytes, vtable 0xB9A464) in sLockOnTargetMgr's list (0xE558B0: +0x1C thread
// safe, +0x04 its critical section, +0x20 mpTop; linked by +0x04). Enemies and the boss Dante (uPlayerDanteBoss::setup
// 0x7BFD76) append their own under the lock and call cLockOnTarget::sort(1, 1) 0x450C50 (cdecl: camera-in first, then
// distance); sub_48B280 (ecx = the manager; target, notify; retn 8) takes one out and tells mpPlayer (vtable +0x2B0);
// aRoom::cleanRoom empties the list (sLockOnTargetMgr::removeTop). Each target carries the values the pickers read
// (uPlayer::updateLockOn's sub_451200 / sub_4512D0 ...): mpModel +0x08, mPos +0x10, mType +0x20 (0: 30 m, 300 m with a
// boss; sub_451060), mPILength +0x24, mMaxHP / mHp +0x28 / +0x2C, mIsIn +0x34, misCatch +0x36, mIsCameraIn +0x37,
// mIsNoTarget +0x38 (skipped), mNearPos +0x40; an enemy's postMain fills them for mpPlayer, from its lock-on spheres
// (uActor +0xEF0 count, +0xF00 LOCKONSPHERE_DAT[16]: world sphere, +0x10 joint, +0x20 offset; the boss: one, joint 0,
// radius 50) through uActor::getNearLengthSpherIdx 0x4A8620.
// While PvP hits is on each player has a target of ours (with the boss's sphere), kept in the list from the game thread
// (tick); before each player's updateLockOn his own target (and a dead player's) is skipped and, for player 1, the
// values are computed for him (CoopContext computes every target's for players 2-4).
constexpr uintptr_t kLockMgr        = 0xE558B0;
constexpr uintptr_t kJobSafe        = 0xE559B9;   // cSystem::mJobSafe
constexpr uintptr_t kTargetVtable   = 0xB9A464;
constexpr uintptr_t kTargetSort     = 0x450C50;
constexpr uintptr_t kTargetRemove   = 0x48B280;
constexpr uintptr_t kTargetLost     = 0x2B0;      // uPlayer vtable: a lock-on target is gone (its model)
constexpr uintptr_t kNearLength     = 0x4A8620;
constexpr uintptr_t kGetCenterPos   = 0x84;       // uActor vtable: getCenterPos(MtVector3* out)
constexpr uintptr_t kSphereNum      = 0xEF0;
constexpr uintptr_t kSphere         = 0xF00;
constexpr uintptr_t kHp = 0x15CC, kHpMax = 0x15D0, kIsDead = 0x1E82, kParamTbl = 0x1E9C;
constexpr size_t    kTargetSize     = 0x50;

struct alignas(16) Target { uint8_t raw[kTargetSize]; };
Target lock_targets[coop::kMaxPlayers];
uintptr_t fn_target_remove = kTargetRemove;

naked_extern void __stdcall target_remove(uintptr_t mgr, uintptr_t target) {
    __asm {
        mov ecx, [esp+4]
        push 1
        push dword ptr [esp+0Ch]
        call dword ptr [fn_target_remove]
        ret 8
    }
}

bool mgr_locked(uintptr_t mgr) {
    return at<bool>(mgr, 0x1C) || at<bool>(kJobSafe, 0);
}

bool in_list(uintptr_t mgr, uintptr_t target) {
    int n = 0;
    for (uintptr_t t = at<uintptr_t>(mgr, 0x20); t && n < 1024; t = at<uintptr_t>(t, 0x04), ++n) {
        if (t == target) return true;
    }
    return false;
}

void add_target(uintptr_t mgr, uintptr_t target) {
    const bool lock = mgr_locked(mgr);
    if (lock) EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(mgr + 4));
    at<uintptr_t>(target, 0x04) = 0;
    uintptr_t t = at<uintptr_t>(mgr, 0x20);
    if (!t) {
        at<uintptr_t>(mgr, 0x20) = target;
    } else {
        while (at<uintptr_t>(t, 0x04)) t = at<uintptr_t>(t, 0x04);
        at<uintptr_t>(t, 0x04) = target;
    }
    reinterpret_cast<void(__cdecl*)(int, int)>(kTargetSort)(1, 1);
    if (lock) LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(mgr + 4));
}

// Out of the list, and out of every player's lock (the game only tells mpPlayer).
void drop_target(uintptr_t mgr, uintptr_t target) {
    const uintptr_t model = at<uintptr_t>(target, 0x08);
    at<bool>(target, 0x38) = true;
    if (mgr && in_list(mgr, target)) {
        target_remove(mgr, target);
    }
    for (int i = 0; i < coop::kMaxPlayers && model; i++) {
        const uintptr_t pl = coop::players[i].unit;
        if (!pl || pl == model) continue;
        reinterpret_cast<void(__fastcall*)(uintptr_t, uintptr_t, uintptr_t)>(at<uintptr_t>(at<uintptr_t>(pl, 0), kTargetLost))(pl, 0, model);
    }
}

void center_of(uintptr_t actor, float out[4]) {
    reinterpret_cast<float*(__fastcall*)(uintptr_t, uintptr_t, float*)>(at<uintptr_t>(at<uintptr_t>(actor, 0), kGetCenterPos))(actor, 0, out);
}

void lock_tick() {
    const uintptr_t mgr = at<uintptr_t>(kLockMgr, 0);
    if (!mgr) return;
    const bool want = active();
    for (int j = 0; j < coop::kMaxPlayers; j++) {
        const uintptr_t t = (uintptr_t)lock_targets[j].raw;
        const uintptr_t unit = j < (int)Coop::player_num ? coop::players[j].unit : 0;
        const bool listed = in_list(mgr, t);
        const bool ready = unit && at<uintptr_t>(unit, kParamTbl);   // set up: his first update has run
        if (!want || !ready || (listed && at<uintptr_t>(t, 0x08) != unit)) {
            if (listed) drop_target(mgr, t);
            if (!want || !ready) continue;
        }
        if (!in_list(mgr, t)) {
            memset(lock_targets[j].raw, 0, kTargetSize);
            at<uintptr_t>(t, 0x00) = kTargetVtable;
            at<uintptr_t>(t, 0x08) = unit;
            at<float>(t, 0x24) = 100000.0f;   // out of range until measured
            at<float>(t, 0x30) = 30.0f;       // as an enemy's
            at<bool>(t, 0x36) = true;         // misCatch
            if (at<uint32_t>(unit, kSphereNum) == 0) {   // the boss Dante's: one sphere on joint 0, radius 50
                at<uint32_t>(unit, kSphereNum) = 1;
                at<float>(unit, kSphere + 0x0C) = 50.0f;
                at<uint32_t>(unit, kSphere + 0x10) = 0;
                at<Vec3>(unit, kSphere + 0x20) = {0.0f, 0.0f, 0.0f};
            }
            add_target(mgr, t);
        }
        // The aim point (the guns' bullets fly at the shooter's target +0x10: uPlNeroShl000::setup 0x82BFD0,
        // uPlNeroShl001::setup 0x82C870, uPlDanteShl000::setup 0x806420): joint 0, the hips, as the boss's sphere. A
        // player's hurt group (pl000.col / pl006.col group 6) is a 45 cm sphere on joint 0 (122 cm up) and a 25 cm one
        // on joint 4 (head, 174 cm). getCenterPos is the joint of vtable +0x88 (uPlayer::getUpperBdyVibJoint), at the
        // spheres' edge, so a shot at it from the side grazed past them.
        Vec3 hips;
        if (joint_pos(unit, 0, hips)) {
            at<Vec3>(t, 0x10) = hips;
        } else {
            float c[4] = {};
            center_of(unit, c);
            at<Vec3>(t, 0x10) = {c[0], c[1], c[2]};
        }
        at<float>(t, 0x28) = at<float>(unit, kHpMax);
        at<float>(t, 0x2C) = at<float>(unit, kHp);
    }
}

void drop_all_targets() {
    const uintptr_t mgr = at<uintptr_t>(kLockMgr, 0);
    for (Target& target : lock_targets) {
        const uintptr_t t = (uintptr_t)target.raw;
        if (mgr && in_list(mgr, t)) drop_target(mgr, t);
        memset(target.raw, 0, kTargetSize);
    }
}

void __stdcall before_lock_on_impl(uintptr_t player) {
    const uintptr_t mgr = at<uintptr_t>(kLockMgr, 0);
    if (!mgr || !active()) return;
    const bool player1 = body_player(player) == 0;
    float center[4] = {};
    if (player1) center_of(player, center);
    for (int j = 0; j < coop::kMaxPlayers; j++) {
        const uintptr_t t = (uintptr_t)lock_targets[j].raw;
        const uintptr_t unit = at<uintptr_t>(t, 0x08);
        if (!unit || !in_list(mgr, t)) continue;
        at<bool>(t, 0x38) = unit == player || at<bool>(unit, kIsDead);   // never himself, nor a downed player
        if (!player1 || unit == player) continue;
        float length = at<float>(t, 0x24);
        char camera_in = (char)at<uint8_t>(t, 0x37), in = (char)at<uint8_t>(t, 0x34);
        float near_pos[4] = {at<float>(t, 0x40), at<float>(t, 0x44), at<float>(t, 0x48), 0.0f};
        reinterpret_cast<int(__stdcall*)(uintptr_t, const float*, float*, char*, char*, float*)>(kNearLength)(
            unit, center, &length, &camera_in, &in, near_pos);   // camera test: viewport 0, player 1's
        at<float>(t, 0x24) = length;
        at<uint8_t>(t, 0x37) = (uint8_t)camera_in;
        at<uint8_t>(t, 0x34) = (uint8_t)in;
        at<Vec3>(t, 0x40) = {near_pos[0], near_pos[1], near_pos[2]};
    }
}

// Players first: while one of the other players' targets is pickable for this player (in range and in his view, with
// his values in: after CoopContext's lock_on_begin), every other target is skipped (mIsNoTarget) for his updateLockOn
// only, and put back after.
constexpr int kMaxHidden = 512;
uintptr_t hidden[kMaxHidden];
int hidden_n = 0;

bool pickable(uintptr_t t) {
    return !at<bool>(t, 0x38) && at<float>(t, 0x24) < 3000.0f && (at<bool>(t, 0x34) || at<bool>(t, 0x37));
}

void __stdcall prefer_players_impl(uintptr_t player) {
    hidden_n = 0;
    const uintptr_t mgr = at<uintptr_t>(kLockMgr, 0);
    if (!mgr || !active() || !coop_pvp::players_first) return;
    bool any = false;
    for (Target& target : lock_targets) {
        const uintptr_t t = (uintptr_t)target.raw;
        any = any || (at<uintptr_t>(t, 0x08) && at<uintptr_t>(t, 0x08) != player && in_list(mgr, t) && pickable(t));
    }
    if (!any) return;
    int n = 0;
    for (uintptr_t t = at<uintptr_t>(mgr, 0x20); t && n < 1024 && hidden_n < kMaxHidden; t = at<uintptr_t>(t, 0x04), ++n) {
        bool ours = false;
        for (Target& target : lock_targets) ours = ours || t == (uintptr_t)target.raw;
        if (ours || at<bool>(t, 0x38)) continue;
        at<bool>(t, 0x38) = true;
        hidden[hidden_n++] = t;
    }
}

void restore_hidden() {
    for (int i = 0; i < hidden_n; i++) at<bool>(hidden[i], 0x38) = false;
    hidden_n = 0;
}

bool make(std::unique_ptr<FunctionHook>& h, uintptr_t at_addr, void* detour) {
    h = std::make_unique<FunctionHook>(at_addr, (uintptr_t)detour);
    return h->create();
}

} // namespace

#pragma pop_macro("naked")

namespace coop_pvp {

std::optional<std::string> install() {
    if (!make(hook_vs_attr, kSetVsAttr, &detour_vs_attr)) {
        return "Failed to init Coop: PvP collision masks";
    }
    vs_attr_original = hook_vs_attr->get_original();
    if (!make(hook_own_hit, kOwnHitSite, &detour_own_hit)) {
        return "Failed to init Coop: PvP own hits";
    }
    ret_own_hit = kOwnHitSite + 5;
    if (!make(hook_attack_pass, kAttackPassStart, &detour_attack_pass)) {
        return "Failed to init Coop: Gerb hitboxes on players";
    }
    if (!make(hook_push_pair, kPushPairSite, &detour_push_pair)) {
        return "Failed to init Coop: players pushing players";
    }
    if (!make(hook_grab_pass, kGrabPassEnd, &detour_grab_pass)) {
        return "Failed to init Coop: PvP grab pass";
    }
    if (!make(hook_grab_type, kGrabTypeSite, &detour_grab_type)) {
        return "Failed to init Coop: PvP grab target type";
    }
    ret_grab_type = kGrabTypeSite + 6;
    if (!make(hook_grab_msg, kPlayerGrabMsg, (void*)&grab_message)) {
        return "Failed to init Coop: PvP grab message";
    }
    grab_msg_original = hook_grab_msg->get_original();
    if (!make(hook_short_grab, kShortGrabType, &detour_short_grab)) {
        return "Failed to init Coop: PvP Buster reaction";
    }
    if (!make(hook_reaction, kReactionSite, &detour_reaction)) {
        return "Failed to init Coop: PvP Buster motion set";
    }
    if (!make(hook_move_damage, kMoveActDamage, &detour_move_damage)) {
        return "Failed to init Coop: PvP held player";
    }
    move_damage_original = hook_move_damage->get_original();
    if (!make(hook_damage_calc, kDamageCalc, &detour_damage_calc)) {
        return "Failed to init Coop: PvP flinching";
    }
    damage_calc_original = hook_damage_calc->get_original();
    if (!make(hook_hold_start, 0x7E9128, &detour_hold_start)) {
        return "Failed to init Coop: PvP hold start";
    }
    if (!make(hook_hold_release, 0x7EB75D, &detour_hold_release)) {
        return "Failed to init Coop: PvP hold release";
    }
    if (!make(hook_nero_hit, kNeroHitMessage, (void*)&nero_hit_message)) {
        return "Failed to init Coop: Nero's hit camera shake";
    }
    nero_hit_original = hook_nero_hit->get_original();

    motion_registry::reserve("Coop PvP (held Nero)", motion_registry::NERO, kNeroVictimBank);
    return std::nullopt;
}

// A held player who left the grab another way (died, a stage change, the action changed) gets his own armor bits back.
void victims_tick() {
    for (int i = 0; i < coop::kMaxPlayers; i++) {
        Victim& v = victims[i];
        const uintptr_t unit = i < (int)Coop::player_num ? coop::players[i].unit : 0;
        if (v.active) {
            was_grabbed[i] = true;
            grace_left[i] = 0.0f;
        } else if (was_grabbed[i]) {
            was_grabbed[i] = false;
            grace_left[i] = coop_pvp::grab_grace;
        } else if (grace_left[i] > 0.0f) {
            grace_left[i] = unit ? grace_left[i] - at<float>(unit, kDelta) / 60.0f : 0.0f;
        }
        if (!unit || v.active) {
            window_left[i] = 0.0f;
        } else if (stunned(unit)) {
            window_left[i] = coop_pvp::grab_window;
        } else if (window_left[i] > 0.0f) {
            window_left[i] -= at<float>(unit, kDelta) / 60.0f;
        }
        if (!v.armored) continue;
        const uintptr_t pl = unit;
        if (!pl) {
            v = {};
            continue;
        }
        if (!v.active || at<int>(pl, kActionNo) != kActionDamage || at<int>(pl, kDamageType) != kDmgPvpGrab) {
            armor_off(pl, v);
            v.active = false;
        }
    }
}

void tick() {
    lock_tick();
    victims_tick();
    if (active()) {
        if (!rows.arc && !rows_tried) {
            rows_tried = true;
            arc_rows::load("coop\\pvp_em_dante", std::vector<arc_rows::Row>(std::begin(kRows), std::end(kRows)), rows);
        }
    } else if (rows.arc) {
        arc_rows::release(rows);   // a held Nero's banks keep their own references
    }
}

void retry() {
    rows_tried = false;
}

void before_lock_on(uintptr_t player) {
    before_lock_on_impl(player);
}

void prefer_players(uintptr_t player) {
    prefer_players_impl(player);
}

void after_lock_on() {
    restore_hidden();
}

void stage_end() {
    drop_all_targets();
    arc_rows::release(rows);
    rows_tried = false;
    for (uintptr_t& b : victim_bank) b = 0;
    for (Victim& v : victims) v = {};
}

bool grabs_ready() {
    return rows.arc != 0;
}

const char* grabs_error() {
    return rows.error.c_str();
}

void note_damage_pre(uintptr_t attack_group, uintptr_t target) {
    if (!attack_group || !target || body_player(target) < 0) {
        return;
    }
    const int shot = shot_class(at<uintptr_t>(attack_group, kGroupMgr));
    if (shot < 0) {
        return;
    }
    ChargeDiag& c = charge_diag[shot];
    c.damage_calls++;
    c.damage_hp = at<float>(target, kHp);
    c.damage_attack_id = at<int>(attack_group, 0xB4);   // cCollisionGroup::mAttackStatus.mAttackID (damagePreMessage 0x4A804E)
}

void draw_grab_diag() {
    if (ImGui::Button(_("Reset grab counters"))) {
        grab_diag = {};
        hold_diag = {};
        charge_diag[0] = {};
        charge_diag[1] = {};
        impact_diag = {};
    }
    ImGui::SameLine();
    Mod::help_marker(_("Sets the PvP counters below back to zero, to watch one attempt"));
    const GrabDiag& d = grab_diag;
    ImGui::TextWrapped("PvP grabs: hand out %u frames, contacts %u, sent %u, messages %u", d.hand_frames, d.contacts, d.sent,
                d.messages);
    ImGui::SameLine();
    Mod::help_marker(_("How far grabbing another player gets. Hand out: frames a Snatch / Buster grab box was active. Contacts: "
                  "it touched another player's body. Sent: he was stunned, so the grab went to the game. Messages: the "
                  "game accepted it (line of sight from the arm, the grabbing Nero's state) and the victim was told"));
    ImGui::TextWrapped("stunned player's boxes: %u seen, %u off, %u status, %u in grace, %u out of reach", d.stunned_boxes,
                d.stunned_off, d.stunned_status, d.stunned_grace, d.stunned_apart);
    ImGui::SameLine();
    Mod::help_marker(_("While a grab box is out: body boxes of another player who is stunned (grabbable). Off: the box is "
                       "switched off. Status: the game marks him as not hittable. In grace: he was grabbed moments ago (Grab "
                       "grace). Out of reach: the grab box didn't touch it"));
    ImGui::TextWrapped("last contact: action %d, damage type %d, invincible %d, holy %.1f, revive %.1f, stunned %s, reaction %d",
                d.last_action, d.last_type, d.last_invincible ? 1 : 0, d.last_holy, d.last_revive,
                d.last_stunned ? (d.last_window ? "yes (window after a flinch)" : "yes") : "no", d.last_reaction);
    ImGui::TextWrapped("last grab box: %s", d.last_mode < 0 ? "Nero's body (Buster)" : d.last_mode == 3 ? "hand, Buster (3)" :
                       d.last_mode == 1 ? "hand, Snatch (1)" : d.last_mode == 2 ? "hand, DT Snatch (2)" : "hand, mode 0 (Snatch)");
    ImGui::SameLine();
    Mod::help_marker(_("Which box grabbed the last player and how it was read: the Devil Bringer's hand box (its move type: 1 "
                       "Snatch, 2 Devil Trigger Snatch, 3 Buster, 0 after a Snatch's reach hit something) or Nero's own body "
                       "box, which only the Buster uses"));
    ImGui::SameLine();
    Mod::help_marker(_("The player touched last: action 20 = hit reaction, its damage type 1 / 2 / 3 / 5 / 6 / 10 / 11 can be "
                  "grabbed, unless invincible or a holy / revive timer runs. Reaction 3 = pulled by a Snatch, 9 = Buster"));
    const HoldDiag& h = hold_diag;
    const char* fn_name = h.nero_fn == 0x7EBC00 ? "SGScare" : h.nero_fn == 0x7EBE40 ? "SGAScare" : h.nero_fn ? "other" : "none";
    ImGui::TextWrapped("holds %u, last ended: %s after %u ticks (%s)", h.holds, kHoldEndNames[h.end], h.ticks,
                h.kind == kModeSnatch ? "Snatch" : h.kind == kModeBuster ? "Buster" : "-");
    ImGui::SameLine();
    Mod::help_marker(_("Why the last grab of a player ended, and how many of the victim's updates it lasted. Nero let go: "
                       "the grabbing Nero left his grab move or stopped holding this player. Victim motion ended: his "
                       "thrown animation finished (the normal end of a Buster)"));
    ImGui::TextWrapped("Nero then: lower action %u phase %u, holds him %s, motion ended %s, reaction %s (%08X)", h.nero_action,
                h.nero_phase, h.nero_holds_him ? "yes" : "no", h.nero_motion_end ? "yes" : "no", fn_name,
                (unsigned)h.nero_fn);
    ImGui::TextWrapped("held: %s, chest %.0f cm from the hand before placing", h.on_hand ? "on the stand's hand" :
                       "at the arm's origin (joints missing)", h.gap);
    ImGui::SameLine();
    Mod::help_marker(_("During Nero's hold the held player's upper chest is put on the palm of the stand's hand every frame. "
                       "The distance is how far it had drifted since the frame before: a few cm while the hand moves, "
                       "large only on the first frame of a hold"));
    ImGui::SameLine();
    Mod::help_marker(_("The grabbing Nero when the hold ended. Lower action 12 = an attack (the grab move); phase 2 = its "
                       "reaction (the throw) is running. Reaction SGScare / SGAScare = the boss Dante's Buster, none = Nero "
                       "never got a throw to play"));
    for (int pass = 0; pass < 2; pass++) {
        const int shot = 1 - pass;   // the charged shot first: the panel's bottom gets cut off
        const ChargeDiag& c = charge_diag[shot];
        ImGui::PushID(shot);
        ImGui::TextWrapped("%s: %u attack boxes (last: player %d, side %u, attr %X, mask %08X), tested on a player %u, "
                           "shapes touching %u; last pair: shot at %.0f %.0f %.0f, body at %.0f %.0f %.0f, %.0f apart",
                           shot ? "charged shot" : "Blue Rose bullet", c.groups, c.owner + 1, c.kind, c.attr, c.vs, c.pairs,
                           c.overlaps, c.shot[0], c.shot[1], c.shot[2], c.body[0], c.body[1], c.body[2], c.dist);
        ImGui::SameLine();
        Mod::help_marker(_("Nero's Blue Rose shots against players: the plain bullet (also every Gunslinger bullet) and the "
                           "charged shot. Attack boxes: how many it made and the last one's owner (0 = not known as a "
                           "player's), side and mask; the mask needs bit 2 (attacks players). Tested on a player: times "
                           "the game's hit test weighed one of its boxes against another player's body. Shapes touching: "
                           "how many of those the game's own shape test passed"));
        ImGui::TextWrapped("  closest: %.0f apart; shell at %.0f %.0f %.0f, box size %.0f %.0f %.0f, shooter at %.0f %.0f %.0f",
                           c.min_dist, c.shell[0], c.shell[1], c.shell[2], c.size[0], c.size[1], c.size[2], c.shooter[0],
                           c.shooter[1], c.shooter[2]);
        ImGui::SameLine();
        Mod::help_marker(_("The pair where the shot's box came closest to another player's body: how far apart their "
                           "centres were, where the shot itself was, how big its box was (swept from its last position) "
                           "and where the player who fired it stood"));
        ImGui::TextWrapped("  its shapes: shot %d (first: type %d, radius %.0f, scale %.2f, box %.0f %.0f %.0f), body %d, "
                           "shape boxes overlapping %d", c.shot_shapes, c.shot_type, c.shot_radius, c.shot_scale,
                           c.shot_box[0], c.shot_box[1], c.shot_box[2], c.body_shapes, c.shape_overlaps);
        ImGui::TextWrapped("  body's first shape: joint %d, radius %.0f, at %.0f %.0f %.0f", c.body_bone, c.body_radius,
                           c.body_centre[0], c.body_centre[1], c.body_centre[2]);
        ImGui::TextWrapped("  hit calls on a player %u (last: his HP then %.0f, attack ID %d), let through by Coop's sweep %u",
                           c.damage_calls, c.damage_hp, c.damage_attack_id, c.forced);
        ImGui::TextWrapped("  sweep tested: now %.0f %.0f %.0f, last frame %.0f %.0f %.0f, body sphere %.0f %.0f %.0f, gap %.0f",
                           c.sweep_now[0], c.sweep_now[1], c.sweep_now[2], c.sweep_old[0], c.sweep_old[1], c.sweep_old[2],
                           c.sweep_body[0], c.sweep_body[1], c.sweep_body[2], c.sweep_gap);
        ImGui::SameLine();
        Mod::help_marker(_("The two ends of the swept shot the game's shape test uses (where the shot is now and where it was "
                           "last frame), the centre of the body sphere it was tested against, and how far apart they "
                           "really are once both radii are taken off: 0 or below means they touch"));
        ImGui::SameLine();
        Mod::help_marker(_("Times the game went on to hurt a player with this shot (uActor::damagePreMessage on his body), his HP "
                           "at that moment and the attack ID the shot carried. Above 0 while he takes no damage = the hit "
                           "lands and something refuses the damage; 0 = the hit test never let it through"));
        ImGui::SameLine();
        Mod::help_marker(_("At that closest pair: the shot's collision shapes (type 0 sphere, 3 capsule; its radius and "
                           "the scale the game multiplies it by), the body's shape count, and how many shot / body shape "
                           "pairs had overlapping boxes, the first thing the game's shape test checks"));
        ImGui::PopID();
    }
    const ImpactDiag& im = impact_diag;
    ImGui::TextWrapped("Buster impact: made %u, armed %u (mask %08X), no box yet %u, weighed on a player %u (owner player %d)",
                im.made, im.armed, im.vs, im.no_box, im.pairs, im.owner + 1);
    ImGui::SameLine();
    Mod::help_marker(_("The hit that lands a thrown player (the Buster's damage). Armed: its box got the attacks-players "
                       "bit (mask bit 2) the frame after it was made; no box yet = it wasn't set up by then. Weighed on a "
                       "player: times the game's hit test tried it against a player's body; owner = the player it counts "
                       "for (0 = nobody, then Coop skips nothing but the hit counts for no one)"));
}

} // namespace coop_pvp
