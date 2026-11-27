#include "FreeRideShell.hpp"
#include "../sdk/Rollback.hpp"
#include "../sdk/Devil4.hpp"
#include <cstring>

using namespace wallrun;

namespace free_ride {

static RideShell* g_shell = nullptr;         // the live one, for the readout; cleared by its destructor
static uint32_t g_hits = 0, g_groups = 0;    // readout
static kAttackStatus g_entry{};              // the entry as used at the last spawn
static bool g_entry_valid = false;

namespace {

// The hit: the entry, with the damage, hit stop and blow level overridden (its interrupt level stays the entry's 2).
char hit_atk_path[] = "Collision\\pl000";   // rAttackStatusData path (no extension)
constexpr int   hit_atk_index = 24;     // RED-ComboA_02: 40 dmg, LvI 2, LvB 0, TypeB 2, hit stop 3
constexpr float hit_damage    = 15.0f;  // mDamageValue
constexpr float hit_interval  = 12.0f;  // frames between hits (the group's life): under the down flinch + 60 frames, so it stays down
constexpr float hit_radius    = 60.0f;  // cm, the sphere on the enemy (other enemies inside it are hit too)
constexpr float hit_height    = 40.0f;  // cm above the enemy's position (its hurt capsules lie along the ground when it is down)
constexpr int   hit_stop      = 0;      // mHitStopTimer: no freeze per hit, the ride keeps moving
constexpr int   hit_lv_b      = 0;      // mAttackLvB: never blown away (a blow verdict knocks the enemy out from under the player)
constexpr int   hit_unit_line = 0x12;   // sUnit line the shell is added on

constexpr uintptr_t kEmPos = 0x30;           // uActor::mPos (the enemy)
constexpr uintptr_t fn_collMgrDtor = 0x0050B480;   // uCollisionMgr::~uCollisionMgr(this@ecx)

void coll_mgr_dtor(uCollisionMgr* mgr) { ((void(__fastcall*)(uCollisionMgr*, int))fn_collMgrDtor)(mgr, 0); }
void update_matrices(void* unit) { custom_utils::updateLmat(unit); custom_utils::updateWmat(unit); }

} // namespace

RideShell::RideShell(uintptr_t owner_, uintptr_t enemy_, uint32_t serial_) : owner(owner_), enemy(enemy_), serial(serial_) {
    std::memset(&coll, 0, sizeof(coll));
    custom_utils::uCollisionMgrCons(&coll);
    std::memset(&prim, 0, sizeof(prim));
    std::memset(&atk, 0, sizeof(atk));
    this->mActorType = 5;               // a shell, as the game's own
    g_shell = this;
    this->mWorkRate.mType = 9;
    const MtVector3& p = at<MtVector3>(enemy, kEmPos);
    this->mPos.x = p.x; this->mPos.y = p.y + hit_height; this->mPos.z = p.z;
    mt_yaw_quat(0.0f, this->mQuat);
}

void RideShell::destructor(uint32_t flag) {
    if (g_shell == this) g_shell = nullptr;
    coll_mgr_dtor(&coll);               // releases the groups and the .atk reference init_0 took
    CustomActor::destructor(flag);
}

// Once the unit system has the shell: the .atk, the entry, the sphere, the first group.
void RideShell::startup() {
    update_matrices(this);
    uintptr_t rsrc = (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)rAtckStatDTI, hit_atk_path, 1);
    if (!rsrc) { spdlog::warn("FreeRideShell: '{}.atk' did not load", hit_atk_path); end(); return; }
    custom_utils::load_atk_col((void*)rsrc, nullptr, &coll, this);      // uCollisionMgr::init_0(this, &coll, 0, atk)
    if (!custom_utils::read_attack_entry((const void*)rsrc, hit_atk_index, atk)) {
        spdlog::warn("FreeRideShell: '{}.atk' has no entry {}", hit_atk_path, hit_atk_index);
        devil4_sdk::release_resource((cResource*)rsrc); end(); return;
    }
    devil4_sdk::release_resource((cResource*)rsrc);                      // init_0 holds its own reference
    atk.mDamageValue  = hit_damage;
    atk.mHitStopTimer = hit_stop;
    atk.mAttackLvB    = hit_lv_b;
    atk.mBlownAngleType = 0;            // POS: away from the shell, should a blow ever come through
    g_entry = atk; g_entry_valid = true;

    prim.mType  = PRIM_TYPE_SPHERE;
    prim.mBone0 = prim.mBone1 = -1;
    prim.mRadius = hit_radius;
    prim.mPos0.x = prim.mPos0.y = prim.mPos0.z = 0.0f;
    prim.mPos1 = prim.mPos0;
    prim.mAttr = 0; prim.mId = 0; prim.mShrink = 0.0f; prim.mDummy = -1;
    arm();
}

// A new group = a new unique ID = the next hit the enemy takes. Kind 1 Ps (a player shell), attack, the vs bits of that
// kind, life = the interval, following the shell's matrix.
void RideShell::arm() {
    if (ended) return;
    group = custom_utils::add_prim_attack_group(&coll, &prim, &atk, 1, hit_interval, &this->mWmat);
    if (!group) { spdlog::warn("FreeRideShell: setCollision failed"); end(); return; }
    ++g_groups;
}

void RideShell::tick() {
    if (ended) return;
    if (!ride_active(owner, serial)) { end(); return; }
    const MtVector3& p = at<MtVector3>(enemy, kEmPos);
    this->mPos.x = p.x; this->mPos.y = p.y + hit_height; this->mPos.z = p.z;
    update_matrices(this);
    custom_utils::collide(&coll);       // uCollisionMgr::move: the group's world matrix and its life
    if (group && !custom_utils::group_in_table(&coll, group)) group = nullptr;   // freed by move (timer >= mLife): that hit is spent
    if (!group) arm();
    age += this->m_delta_time;
}

void RideShell::onhit(void* /*atk_param*/, void* /*dfd_param*/) { ++g_hits; }

void RideShell::render(void* /*ptrans*/) {}   // nothing drawn (and not the game's uActor render)

void RideShell::end() {
    if (ended) return;
    ended = true;
    die();                              // the unit system destroys it (-> destructor, which closes the collision manager)
}

// ---- FreeRide's side -----------------------------------------------------------------------------------------------
bool spawn_ride_shell(uintptr_t player, uintptr_t enemy, uint32_t serial) {
    if (!player || !enemy) return false;
    return RideShell::spawn(hit_unit_line, player, enemy, serial) != nullptr;
}

void hit_readout() {
    if (g_shell && !g_shell->ended)
        ImGui::Text("hit shell: alive, age %.0f, group %s | groups made %u, hits %u", g_shell->age, g_shell->group ? "armed" : "spent", g_groups, g_hits);
    else
        ImGui::Text("hit shell: none | groups made %u, hits %u", g_groups, g_hits);
    if (g_entry_valid)
        ImGui::Text("entry '%s' as used: dmg %.0f lvI %d lvB %d typeB %d hitstop %d stylish %.0f dt %.0f", g_entry.mAsName, g_entry.mDamageValue,
                    g_entry.mAttackLvI, g_entry.mAttackLvB, g_entry.mDamageTypeB, g_entry.mHitStopTimer, g_entry.mStylishPoint, g_entry.mDTAdd);
}

void register_shell_rollback() { rollback::add_range("FreeRide shell", &g_shell, sizeof g_shell); }

} // namespace free_ride
