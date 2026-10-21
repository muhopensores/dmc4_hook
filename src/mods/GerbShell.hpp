#pragma once

#include "Gerb.hpp"
#include "../sdk/Custom.hpp"
#include "../sdk/uCollisionMgr.hpp"

// GerbShell: the hitbox of a Gerb jump, a shell (the game's word for a projectile) that rides the player through the
// jump and knocks what it hits in the direction of the jump.
//
// How a shell carries an attack in the game (uEm019_ice::setup 0x680B80, Frost's ice shot; RE note
// player_move_pipeline.md "Shells and code-defined hitboxes"):
//   1. rAttackStatusData* atk = sResource::create(rAttackStatusData::DTI, "Collision\\em019")   (the .atk, cached)
//   2. uCollisionMgr::init_0(actor, &mCollMgr, col = 0, atk)          the shell's own collision manager
//   3. mAttackStatus = *atk->mppAttackStatus[6]->mpAttackStatus       one 0x58-byte kAttackStatus copied out
//   4. g = uCollisionMgr::setCollision(-1, &mCollMgr, -1, &prim, 1, -1, -1)   a kCollPrim defined in code, no .col
//   5. g->mAttackStatus = mAttackStatus; g->mKind = 4 (Es); g->mAttr = 1 (attack); g->mVsAttr = 0;
//      cCollisionGroup::setAttr(g, 0, 4); g->mLife = 240; g->mpRefMat = &mWmat; g->mOffset = 0
//   6. cResource::release(atk)
// This does the same with kind 1 (Ps, a player shell) so enemies take it as the player's attack, and the group's
// matrix follows the shell, which follows the player.
//
// The knock direction is the game's own: uActor::calcDamageVec 0x4A9BD0 reads the group's mBlownAngleType:
//   0 BLOWN_ANGLE_TYPE_POS  = away from the attacker's position (the shell sits on the player)
//   1 BLOWN_ANGLE_TYPE_ANG  = along the attacker's facing (uCoord::getAngle of the shell: its mQuat)
// so the shell's yaw IS the knock direction in mode ANG. Modes here: the jump direction in world space, the
// player's facing, or away from the player (POS).
//
// Drawn with the game's own line primitive (sPrim::submitLine, as RayTest does) from the unit's render slot, so the
// outline is depth-tested like the world; `shell_show` toggles it.

namespace gerb {

enum KnockMode : int { KNOCK_JUMP_DIR = 0, KNOCK_PLAYER_FACING = 1, KNOCK_AWAY_FROM_PLAYER = 2 };

extern bool  shell_enabled;
extern bool  shell_show;            // draw the capsule outline (in-game lines)
extern char  shell_atk_path[128];   // rAttackStatusData path (no extension), e.g. Collision\pl000 or a custom one
extern int   shell_atk_index;       // entry of that file to copy
extern float shell_radius;          // cm
extern float shell_length;          // cm, the capsule runs this far from the player along the jump
extern float shell_height;          // cm above the player's feet
extern float shell_life;            // frames, a hard cap (the shell also dies with the jump)
extern int   shell_knock_mode;      // KnockMode
extern float shell_knock_angle;     // degrees added to the knock yaw (modes 0 / 1)
extern int   shell_damage_type;     // -1 = the entry's, else kAttackStatus::mDamageType and mDamageTypeI (vanilla attacks keep 0)
extern int   shell_damage_type_b;   // -1 = the entry's, else mDamageTypeB: the reaction. Nero's data: 2 blown (Streak), 3 blown 2
                                    // (Dropkick), 4 upper / launch (High Roller), 5 ground (Split)
extern int   shell_attack_lv_i;     // -1 = the entry's, else mAttackLvI (and mAttackLv): the interrupt level vs kDefendStatus::mMaxInterrupt
extern int   shell_attack_lv_b;     // -1 = the entry's, else mAttackLvB: the blow level vs mMaxBlown. Streak / High Roller 2, Dropkick 3
// the rest of kAttackStatus, each -1 (or < 0) = the entry's
extern int   shell_attack_id;       // mAttackID (ATTACK_ID; 13 = no damage message)
extern int   shell_range_type;      // mRangeType 0 melee, 1 ranged, 2 grab
extern int   shell_hit_stop;        // mHitStopTimer, frames
extern int   shell_hitmark;         // mHitmarkAngle (HIT_MARK_ANGLE: the hit spark's direction / size)
extern int   shell_hit_se;          // mHitSE
extern float shell_stylish_point;   // mStylishPoint
extern float shell_stylish_timer;   // mStylishTimer
extern float shell_dt_add;          // mDTAdd
extern int   shell_attack_flag;     // mAttackFlag (0 default, 1 no death)
extern int   shell_blown_type;      // -1 = from the knock mode, else mBlownAngleType as is (0 POS, 1 ANG; 2-5 fall to world +Z in DX9)
extern int   shell_element;         // mElementType
extern float shell_damage;          // < 0 = the entry's, else mDamageValue
extern bool  shell_one_hit;         // the shell ends on its first hit
extern int   shell_kind;            // cCollisionGroup::mKind: 0 Pl, 1 Ps (player shell), 2 Em, 3 Em2, 4 Es, 5 Es2, 6 Set, 7 Stg
extern uint32_t shell_vsattr;       // 0 = the game's table for that kind (cCollisionGroup::setAttr), else written as is
extern int   shell_unit_line;       // sUnit line the shell is added on (Nero's shells 0x12; every line ticks before the attack pass)

// Spawns the shell for the jump that just launched. `dir` = the jump direction (unit), `serial` = the jump's serial
// in its JumpState, used by the shell to notice the jump ended. Returns false when the .atk can't be read.
bool spawn_shell(uintptr_t player, const glm::vec3& dir, uint32_t serial);

// Gerb.cpp: true while the jump `serial` of `player` is still airborne in Gerb's move (the shell's lifetime).
bool jump_active(uintptr_t player, uint32_t serial);

void shell_gui();
void shell_debug_readout(uintptr_t player);   // the live shell's group next to the player's own attack groups
void shell_config_load(const utility::Config& cfg);
void shell_config_save(utility::Config& cfg);

// The unit itself. Layout: uActor (0x1370), then our own collision manager (like uShell's layout, whose
// destructor CustomActor::destructor reuses), then the prim / attack copy the game keeps pointers to.
struct GerbShell : CustomUnit<GerbShell> {
    uCollisionMgr coll;
    kCollPrim     prim;
    kAttackStatus atk;
    cCollisionGroup* group = nullptr;
    uintptr_t owner = 0;
    uint32_t  serial = 0;
    glm::vec3 dir{0.0f, 0.0f, 1.0f};   // the jump direction (world)
    float     yaw = 0.0f;              // the shell's facing = the knock direction (modes 0 / 1)
    float     age = 0.0f;
    bool      ended = false;

    GerbShell(uintptr_t owner, const glm::vec3& dir, float yaw, uint32_t serial);
    void destructor(uint32_t flag);     // vtable slot 0 (CustomUnit wires it): the collision manager's destructor, then the actor's
    void startup();                     // +0x14: loads the .atk and registers the capsule
    void tick();                        // +0x78: follow the player, run the collision, end with the jump
    void onhit(void* atk_param, void* dfd_param);   // +0x94
    void render(void* ptrans);          // +0x24: the outline, in-game lines
    void end();
};

} // namespace gerb
