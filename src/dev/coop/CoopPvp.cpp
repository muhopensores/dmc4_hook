#include "CoopPvp.hpp"
#include "Coop.hpp"
#include "CoopContext.hpp"
#include "../../mod.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MotionRegistry.hpp"
#include "../../sdk/ArcRows.hpp"
#include <algorithm>
#include <cmath>
#include <memory>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

namespace coop_pvp {

bool hits = false;
bool players_first = true;
bool dt_from_players = true;
int flinch[2][5] = {{-1, -1, 3, 2, 1}, {-1, -1, -1, 3, 2}};
int blow[2][5]   = {{-1, -1, -1, 2, 1}, {-1, -1, -1, 3, 1}};

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

// V2: grabs
constexpr uintptr_t kGrabPassEnd     = 0x47BCB1;   // sCollisionGame::checkAttackMain after both checkGrabProcess (edi = this): cmp byte [edi+1Ch],0 / jnz
constexpr uintptr_t kChainTop        = 0x24;       // sCollisionGame::mppCollisionChainTop[kind * 4 + chain]
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
constexpr uint32_t  kActionAttack = 0xC;
constexpr int       kDmgBut       = 3;         // DMG_ID_BUT: moveDmgButtobi, the blow, landing and getting up
constexpr int       kActionDamage = 20;
constexpr int       kDmgPvpGrab   = 0x20;      // ours: past DMAGE_ID's last (0x1A); every reader compares, none indexes

// Dante's bank 8 (pl006_09, the boss's reactions: 0x800 pulled, 0x801 landing, 0x804 Buster, 0x80C landing in the air);
// a Nero gets the same file in his bank 9 (free: his setup never loads it; reserved from the motion registry), with
// Nero's own empty coat file of the Buster-on-Dante set on his coat (as setBusterMotionRsrc does for bank 15).
constexpr int kMotPulled = 0x00, kMotLand = 0x01, kMotBuster = 0x04, kMotAirLand = 0x0C;
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
    hook_reaction, hook_move_damage, hook_damage_calc, hook_nero_hit;
uintptr_t vs_attr_original = 0;
uintptr_t ret_own_hit = 0;                     // 0x47C463: the pair is tested (checkGroup_)
uintptr_t ret_no_hit = 0x47C4AC;               // the next damage group
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
    return coop_context::player_of(actor);
}

int owner_of_mgr(uintptr_t mgr) {
    return mgr ? owner_of_actor(at<uintptr_t>(mgr, kMgrReport)) : -1;
}

// The player's own body (not his shells or weapons), as a player index; -1 otherwise.
int body_player(uintptr_t actor) {
    const int i = actor ? coop_context::player_of(actor) : -1;
    return i >= 0 && coop::players[i].unit == actor ? i : -1;
}

bool active() {
    return coop_pvp::hits && Coop::mod_enabled;
}

// ---- V1: hits ----

void __stdcall after_vs_attr(uintptr_t group, uintptr_t mgr) {
    if (!group || !Coop::mod_enabled || owner_of_mgr(mgr) < 0) {
        return;
    }
    const uint32_t attr = at<uint32_t>(group, kGroupAttr);
    uint32_t& vs = at<uint32_t>(group, kGroupVsAttr);
    if (attr & 1) {
        vs |= kAttacksPlayers;
    }
    if (attr & 2) {
        vs |= kHitByPlayers;
    }
}

bool __stdcall same_player(uintptr_t attack_group, uintptr_t damage_group) {
    if (!Coop::mod_enabled || !attack_group || !damage_group) {
        return false;
    }
    const int attacker = owner_of_mgr(at<uintptr_t>(attack_group, kGroupMgr));
    return attacker >= 0 && attacker == owner_of_mgr(at<uintptr_t>(damage_group, kGroupMgr));
}

// setVsAttr's arguments are pushed again for the original, which pops them (retn 0Ch); eax is the group throughout.
naked_extern void detour_vs_attr() {
    __asm {
        cmp byte ptr [coop_pvp::hits], 0
        je plain
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
        test al, al
        popad
        jnz no_hit
    hit:
        jmp dword ptr [ret_own_hit]
    no_hit:
        jmp dword ptr [ret_no_hit]
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
};
Victim victims[coop::kMaxPlayers];

// A player can be grabbed only while stunned: in the damage action with a hit reaction (DMAGE_ID 1 HIT, 2 ZURI, 3 BUT,
// 5 KIRI, 6 HITIME launched, 10 COMBO, 11 AIR_COMBO), and not while uPlayer::canBeGrabbed 0x7AEF60 refuses
// (invincible, Holy Water, getting up from a revive).
bool stunned(uintptr_t player) {
    if (at<int>(player, kActionNo) != kActionDamage || at<bool>(player, kInvincible) ||
        at<float>(player, kHolyTimer) > 0.0f || at<float>(player, kReviveTimer) > 0.0f) {
        return false;
    }
    switch (at<int>(player, kDamageType)) {
    case 1: case 2: case 3: case 5: case 6: case 10: case 11:
        return true;
    default:
        return false;
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
            const uintptr_t hand = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
            if (!hand || at<uintptr_t>(hand, 0) != kHandCtrlVtable) continue;
            const int grabber = owner_of_actor(hand);
            if (grabber < 0) continue;
            if (!rows.arc) continue;   // Nero's Buster-on-Dante set and the victims' motions aren't in the cache
            for (uintptr_t d = chains[0 * 4 + 1]; d; d = at<uintptr_t>(d, kGroupNext + 4 * 1)) {
                if (at<bool>(d, kGroupClear)) continue;
                const uintptr_t dmgr = at<uintptr_t>(d, kGroupMgr);
                const uintptr_t target = dmgr ? at<uintptr_t>(dmgr, kMgrReport) : 0;
                const int victim = body_player(target);
                if (victim < 0 || victim == grabber || !stunned(target)) continue;
                if (at<int8_t>(target, kActorStatus) < 0) continue;
                if (!check_group(scratch, g, d, 2)) continue;
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

bool play(uintptr_t player, Victim& v, int block, uint32_t attr, float interp, float start) {
    const int motion = motion_of(player, block);
    if (motion < 0) {
        return false;
    }
    if (at<uint32_t>(player, kPlayerId) != 0) {
        devil4_sdk::borrow_ctn((void*)player, 0, 0);   // for the request; the end-only script replaces it next tick
        v.script = true;
    }
    devil4_sdk::indexed_anim_call((uint32_t)motion, (uPlayer*)player, attr, 1.0f, start, interp);
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
    at<float>(player, kDamageAng) = at<float>(player, kRotNow);
    at<int>(player, kDamageType) = kDmgBut;
    set_action_damage(player);
    at<uintptr_t>(player, kDamageActor) = v.grabber;
    v.active = false;
}

void release_footwork(uintptr_t player, Victim& v) {
    v.active = false;
    at<int>(player, kDamageType) = 0;
    set_footwork(player, 10.0f);
}

// uPlayer::moveActDamage for a held player, from uPlayerDanteBoss::moveActSnatch 0x7C3EE0 / moveActBuster 0x7C4630
// (generic reactions only: no Devil Trigger version, no counter).
bool __stdcall victim_move(uintptr_t player) {
    if (at<int>(player, kDamageType) != kDmgPvpGrab) {
        return false;
    }
    const int i = body_player(player);
    if (i < 0 || !victims[i].active || !active()) {
        if (i >= 0) victims[i].active = false;
        at<int>(player, kDamageType) = kDmgBut;   // PvP turned off mid-grab: the game's blow
        set_action_damage(player);
        return true;
    }
    Victim& v = victims[i];
    if (v.script) {
        devil4_sdk::install_ctn_script((void*)player, kEndOnly);
        v.script = false;
    }
    const uintptr_t g = v.grabber;
    if (!g || body_player(g) < 0) {
        release_blown(player, v);
        return true;
    }
    const float dt = at<float>(player, kDelta);
    Vec3& pos = at<Vec3>(player, kPos);
    const Vec3 gpos = at<Vec3>(g, kPos);
    const float gyaw = at<float>(g, kRotNow);

    if (v.kind == kModeBuster) {
        const bool holding = at<uint32_t>(g, kLowerAction) == kActionAttack && at<uintptr_t>(g, kGrabActor) == player;
        if (v.phase == 0) {
            stop(player);
            if (!play(player, v, kMotBuster, 0, 0.0f, 0.0f)) {
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
        const bool ended = at<bool>(player, kMotionEnd);
        if (!holding || ended) {
            if (ended) {   // the motion's last pose stands 107 cm back (moveActBuster, before 0x805)
                const Vec3 off = forward(yaw, -107.4399f);
                pos.x += off.x;
                pos.z += off.z;
                set_coll_init_pos(player);
            }
            release_blown(player, v);
        }
        return true;
    }

    // Snatch: pulled toward Nero, then landing
    switch (v.phase) {
    case 0: {
        face(player, std::atan2(gpos.x - pos.x, gpos.z - pos.z));
        stop(player);
        if (!play(player, v, kMotPulled, 1, 0.0f, 0.0f)) {
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
                stop(player);
                release_footwork(player, v);
            } else {
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
using GrabMsgFn = int(__fastcall*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

int __fastcall grab_message(uintptr_t actor, uintptr_t edx, uintptr_t grab_group, uintptr_t target_group) {
    const int victim = active() ? body_player(actor) : -1;
    const uintptr_t mgr = grab_group ? at<uintptr_t>(grab_group, kGroupMgr) : 0;
    const uintptr_t hand = mgr ? at<uintptr_t>(mgr, kMgrReport) : 0;
    if (victim < 0 || !hand || at<uintptr_t>(hand, 0) != kHandCtrlVtable) {
        return reinterpret_cast<GrabMsgFn>(grab_msg_original)(actor, edx, grab_group, target_group);
    }
    const int grabber = owner_of_actor(hand);
    if (grabber < 0 || grabber == victim) {
        return 0;
    }
    const int mode = at<int>(hand, kHandCtrlMode);
    Victim& v = victims[victim];
    v = {};
    v.active = true;
    v.kind = mode == kModeSnatch ? kModeSnatch : kModeBuster;
    v.grabber = coop::players[grabber].unit;
    at<int>(actor, kDamageType) = kDmgPvpGrab;
    set_action_damage(actor);
    at<uintptr_t>(actor, kDamageActor) = v.grabber;
    return v.kind == kModeSnatch ? kReactSnatch : kReactBuster;
}

// setShortGrabReaction goes on only for a held enemy (type 3); a held player too.
bool __stdcall held_player_ok(uintptr_t actor) {
    return is_pvp_target(actor);
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
alignas(4) uint8_t pvp_defend[2][kDefendSize];
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
    const int set = _strnicmp((const char*)entry, "d-majin", 7) == 0 ? 1 : 0;
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
        float c[4] = {};
        center_of(unit, c);
        at<Vec3>(t, 0x10) = {c[0], c[1], c[2]};
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
    if (!make(hook_nero_hit, kNeroHitMessage, (void*)&nero_hit_message)) {
        return "Failed to init Coop: Nero's hit camera shake";
    }
    nero_hit_original = hook_nero_hit->get_original();

    motion_registry::reserve("Coop PvP (held Nero)", motion_registry::NERO, kNeroVictimBank);
    return std::nullopt;
}

void tick() {
    lock_tick();
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

} // namespace coop_pvp
