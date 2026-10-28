#pragma once

#include "XmlMove.hpp"
#include "../../sdk/Custom.hpp"
#include "../../sdk/uCollisionMgr.hpp"
#include "../../sdk/MtMath.hpp"

// MoveDef <shell> (D18): a code-defined attack capsule that rides the player while the move runs, GerbShell's recipe
// (mods/GerbShell.hpp, uEm019_ice::setup 0x680B80) with its settings from the move file instead of the panel. The
// shell's own uCollisionMgr is init_0'd with the .atk, one kCollPrim capsule from the player's position + height,
// `length` along the move's direction, its group filled with the entry's kAttackStatus and the overrides. The knock
// direction is the game's (uActor::calcDamageVec): blown angle type 1 ANG = the shell's yaw, 0 POS = away from it.
// GerbShell stays as it is (the C++ Gerb keeps its own); this one is MoveDef's.

namespace movedef {

// MoveDef.cpp: true while `move_id`'s entry `serial` still runs on `player` (lower action 12 = attack, mAtckId = the
// move, the same entry). The shell ends the tick this turns false.
bool shell_alive(uintptr_t player, uint32_t move_id, uint32_t serial);

// Spawns a shell for `player` along `dir` (unit, world). Returns false when disabled or the spawn fails.
bool spawn_shell(const xmlmove::Shell& def, uintptr_t player, const glm::vec3& dir, uint32_t move_id, uint32_t serial);

int shells_alive();        // readout
int shell_hits();

struct AttackShell : CustomUnit<AttackShell> {
    uCollisionMgr coll;
    kCollPrim     prim;
    kAttackStatus atk;
    xmlmove::Shell def;                // a plain copy (no heap members: the unit's memory is freed without C++ destructors)
    cCollisionGroup* group = nullptr;
    uintptr_t owner = 0;
    uint32_t  move_id = 0, serial = 0;
    glm::vec3 dir{0.0f, 0.0f, 1.0f};
    float     yaw = 0.0f;              // the shell's facing = the knock direction (knock 0 / 1)
    float     age = 0.0f;
    bool      ended = false;

    AttackShell(const xmlmove::Shell& def, uintptr_t owner, const glm::vec3& dir, float yaw, uint32_t move_id, uint32_t serial);
    void destructor(uint32_t flag);    // the collision manager's destructor, then the actor's
    void startup();                    // loads the .atk and registers the capsule
    void tick();                       // follow the player, run the collision, end with the move
    void onhit(void* atk_param, void* dfd_param);
    void render(void* ptrans);         // the outline with show="true"
    void end();
};

} // namespace movedef
