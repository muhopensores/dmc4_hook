#pragma once

#include "FreeRide.hpp"
#include "../sdk/Custom.hpp"
#include "../sdk/uCollisionMgr.hpp"

// FreeRideShell: the hits that keep a ridden enemy down. A shell (the game's projectile unit, Gerb's hitbox shell is the
// template) sits on the enemy for the ride with one sphere attack group that hits it every `hit_interval` frames for a
// little damage. Each hit is the game's own: the enemy's damage verdict (uDamage::calc 0x51BE10, the interrupt gauge
// from its .dfd), and an interrupted enemy that is lying down replays its down flinch (uEm000Base::interruptMessage
// 0x558090: AS_DOWN -> action 97 setDamageDown, motion 65, then setDownLoop(60) and the wake-up), so a hit every few
// frames keeps it on the ground without freezing it, the way DMC3's Free Ride hurts what it rides.
//
// Why a new group per hit: uActor::damagePreMessage 0x4A7F30 keeps the last 16 attack-group unique IDs an actor took
// (mIDQueue) and skips a group it has already taken, so one group hits once however long it lives. The shell gives its
// group a life of `hit_interval` frames and makes a fresh one (a new ID from sMediator::getCollisionUniqueID through
// uCollisionMgr::setCollision) when the manager has freed the old one.
//
// How the game does the same (uPlOwnHit::setup 0xA2FB60, a player hit without a weapon): a 20 cm sphere kCollPrim,
// uCollisionMgr::init_0(this, &mgr, 0, 0), the attack from Collision\PlOwnHit.atk, setCollision(-1, ...), kind 4,
// attr 1, setAttr(0, 4), life 20, mpRefMat = its matrix. Player shells (uPlNeroShl000::setup) also pass themselves as
// the manager's report actor, so that is the convention here too.
//
// The entry: Nero's Collision\pl000.atk 24 "RED-ComboA_02" (40 damage, interrupt level 2, blow level 0, hit stop 3) with
// the damage, hit stop and blow level overridden below. Level 2 interrupts every normal enemy on one hit (their .dfd
// mMaxInterrupt[2] = 1; Frost needs 3, Berial lying down 1), blow level 0 never knocks anything away (mMaxBlown[0] = -1
// in every file), so the enemy flinches on the ground and stays.

namespace free_ride {

extern char  hit_atk_path[128];   // rAttackStatusData path (no extension)
extern int   hit_atk_index;       // entry of that file to copy
extern float hit_damage;          // < 0 = the entry's, else mDamageValue
extern float hit_interval;        // frames between hits (the group's life)
extern float hit_radius;          // cm, the sphere on the enemy
extern float hit_height;          // cm above the enemy's position
extern int   hit_stop;            // -1 = the entry's, else mHitStopTimer (frames the enemy and the player freeze per hit)
extern int   hit_lv_i;            // -1 = the entry's, else mAttackLvI (and mAttackLv): the interrupt level
extern int   hit_lv_b;            // -1 = the entry's, else mAttackLvB: the blow level (0 = never blown away)
extern bool  hit_show;            // draw the sphere (in-game lines)
extern int   hit_unit_line;       // sUnit line the shell is added on

// Spawns the shell for the ride `serial` of `player` on `enemy`. Returns false when it couldn't be made.
bool spawn_ride_shell(uintptr_t player, uintptr_t enemy, uint32_t serial);

void hit_gui();
void hit_readout();               // the live shell, one line
void hit_config_load(const utility::Config& cfg);
void hit_config_save(utility::Config& cfg);

// The unit. Layout as GerbShell: uActor, then its own collision manager, then the prim / attack copy the game keeps
// pointers to.
struct RideShell : CustomUnit<RideShell> {
    uCollisionMgr coll;
    kCollPrim     prim;
    kAttackStatus atk;
    cCollisionGroup* group = nullptr;
    uintptr_t owner = 0, enemy = 0;
    uint32_t  serial = 0;
    float     age = 0.0f;
    bool      ended = false;

    RideShell(uintptr_t owner, uintptr_t enemy, uint32_t serial);
    void destructor(uint32_t flag);     // vtable slot 0: the collision manager's destructor, then the actor's
    void startup();                     // +0x14: loads the .atk, makes the sphere, arms the first group
    void tick();                        // +0x78: follow the enemy, run the collision, re-arm, end with the ride
    void onhit(void* atk_param, void* dfd_param);   // +0x94
    void render(void* ptrans);          // +0x24: the outline
    void arm();                         // a new attack group (= a new hit)
    void end();
};

} // namespace free_ride
