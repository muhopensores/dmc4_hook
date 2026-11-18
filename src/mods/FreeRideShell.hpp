#pragma once

#include "FreeRide.hpp"
#include "../sdk/Custom.hpp"
#include "../sdk/uCollisionMgr.hpp"

// FreeRideShell: the hits that keep a ridden enemy down. A shell (the game's projectile unit, Gerb's hitbox shell is the
// template) sits on the enemy for the ride with one sphere attack group that hits it every 12 frames for 15
// damage, with no hit stop. Each hit is the game's own: the enemy's damage verdict (uDamage::calc 0x51BE10, the interrupt gauge
// from its .dfd), and an interrupted enemy that is lying down replays its down flinch (uEm000Base::interruptMessage
// 0x558090: AS_DOWN -> action 97 setDamageDown, motion 65, then setDownLoop(60) and the wake-up), so a hit every few
// frames keeps it on the ground without freezing it, the way DMC3's Free Ride hurts what it rides.
//
// Why a new group per hit: uActor::damagePreMessage 0x4A7F30 keeps the last 16 attack-group unique IDs an actor took
// (mIDQueue) and skips a group it has already taken, so one group hits once however long it lives. The shell gives its
// group a life of 12 frames and makes a fresh one (a new ID from sMediator::getCollisionUniqueID through
// uCollisionMgr::setCollision) when the manager has freed the old one.
//
// How the game does the same (uPlOwnHit::setup 0xA2FB60, a player hit without a weapon): a 20 cm sphere kCollPrim,
// uCollisionMgr::init_0(this, &mgr, 0, 0), the attack from Collision\PlOwnHit.atk, setCollision(-1, ...), kind 4,
// attr 1, setAttr(0, 4), life 20, mpRefMat = its matrix. Player shells (uPlNeroShl000::setup) also pass themselves as
// the manager's report actor, so that is the convention here too.
//
// The entry: Nero's Collision\pl000.atk 24 "RED-ComboA_02" (40 damage, interrupt level 2, blow level 0, hit stop 3) with
// the damage (15), hit stop (0) and blow level (0) overridden (FreeRideShell.cpp). A 60 cm sphere, 40 cm above the enemy's
// position (its hurt capsules lie along the ground when it is down); other enemies inside it are hit too. Level 2
// interrupts every normal enemy on one hit (their .dfd mMaxInterrupt[2] = 1; Frost needs 3, Berial lying down 1), blow
// level 0 never knocks anything away (mMaxBlown[0] = -1 in every file), so the enemy flinches on the ground and stays.

namespace free_ride {

// Spawns the shell for the ride `serial` of `player` on `enemy`. Returns false when it couldn't be made.
bool spawn_ride_shell(uintptr_t player, uintptr_t enemy, uint32_t serial);

void hit_readout();               // the live shell, one line

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
    void render(void* ptrans);          // +0x24: nothing drawn
    void arm();                         // a new attack group (= a new hit)
    void end();
};

} // namespace free_ride
