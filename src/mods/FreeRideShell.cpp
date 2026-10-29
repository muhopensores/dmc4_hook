#include "FreeRideShell.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
#include "../sdk/DebugDraw.hpp"
#include <algorithm>
#include <cstring>
#include <string>

using namespace wallrun;

namespace free_ride {

char  hit_atk_path[128] = "Collision\\pl000";
int   hit_atk_index     = 24;        // RED-ComboA_02: 40 dmg, LvI 2, LvB 0, TypeB 2, hit stop 3
float hit_damage        = 15.0f;
float hit_interval      = 12.0f;
float hit_radius        = 60.0f;
float hit_height        = 40.0f;
int   hit_stop          = 0;         // no freeze per hit: the ride keeps moving
int   hit_lv_i          = -1;
int   hit_lv_b          = 0;         // never blown away
bool  hit_show          = false;
int   hit_unit_line     = 0x12;

static RideShell* g_shell = nullptr;         // the live one, for the readout; cleared by its destructor
static uint32_t g_hits = 0, g_groups = 0;    // readout
static kAttackStatus g_entry{};              // the entry as used at the last spawn
static bool g_entry_valid = false;

namespace {

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
    if (hit_damage >= 0.0f) atk.mDamageValue   = hit_damage;
    if (hit_stop   >= 0)    atk.mHitStopTimer  = hit_stop;
    if (hit_lv_i   >= 0)    atk.mAttackLv = atk.mAttackLvI = hit_lv_i;   // damagePreMessage copies LvI over Lv
    if (hit_lv_b   >= 0)    atk.mAttackLvB     = hit_lv_b;
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
    group = custom_utils::add_prim_attack_group(&coll, &prim, &atk, 1, std::max(hit_interval, 1.0f), &this->mWmat);
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

void RideShell::render(void* ptrans) {
    if (!hit_show || ended || !group) return;
    sMediator* smed = devil4_sdk::get_sMediator();
    void* cprim = debug_draw::get_cprim(ptrans, 6, smed ? (void*)smed->player_ptr : (void*)this);
    debug_draw::draw_sphere(cprim, debug_draw::world_point(this->mWmat, glm::vec3(0.0f)), prim.mRadius, 0xFFFF4040);
}

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

void hit_gui() {
    ImGui::InputText(_("Hit attack file"), hit_atk_path, sizeof(hit_atk_path));
    ImGui::SameLine();
    Mod::help_marker(_("rAttackStatusData (.atk) path without the extension; its entry below is what each hit is. The player's own file "
                       "is Collision\\pl000 (24 = Red Queen combo A's third hit: interrupt level 2, no blow)."));
    ImGui::InputInt(_("Hit attack entry"), &hit_atk_index);
    ImGui::InputFloat(_("Hit damage"), &hit_damage, 1.0f, 10.0f, "%.0f");
    ImGui::SameLine();
    Mod::help_marker(_("Damage per hit (mDamageValue). -1 = the entry's own."));
    ImGui::SliderFloat(_("Hit every"), &hit_interval, 2.0f, 60.0f, "%.0f frames");
    ImGui::SameLine();
    Mod::help_marker(_("Frames between hits. An enemy flinches on the ground for its down flinch plus 60 frames after each interrupting "
                       "hit, so anything under that keeps it down; shorter = more damage per second."));
    ImGui::SliderFloat(_("Hit radius"), &hit_radius, 10.0f, 200.0f, "%.0f cm");
    ImGui::SameLine();
    Mod::help_marker(_("The sphere on the ridden enemy. Other enemies inside it are hit too."));
    ImGui::SliderFloat(_("Hit height"), &hit_height, 0.0f, 150.0f, "%.0f cm");
    ImGui::SameLine();
    Mod::help_marker(_("The sphere's centre above the enemy's position (its hurt capsules lie along the ground when it is down)."));
    ImGui::InputInt(_("Hit stop"), &hit_stop);
    ImGui::SameLine();
    Mod::help_marker(_("Frames the enemy and the player freeze on each hit (mHitStopTimer). 0 keeps the ride smooth, -1 = the entry's."));
    ImGui::InputInt(_("Interrupt level"), &hit_lv_i);
    ImGui::SameLine();
    Mod::help_marker(_("mAttackLvI, judged against the enemy's kDefendStatus mMaxInterrupt: the hit must interrupt to restart the down. "
                       "Normal enemies take level 2 at once, Frost needs three level-2 hits, bosses mostly never. -1 = the entry's."));
    ImGui::InputInt(_("Blow level"), &hit_lv_b);
    ImGui::SameLine();
    Mod::help_marker(_("mAttackLvB. Keep 0: a blow verdict knocks the enemy out from under the player. -1 = the entry's."));
    ImGui::Checkbox(_("Show hit sphere"), &hit_show);
    ImGui::SameLine();
    Mod::help_marker(_("Draws the sphere with the game's own line primitive (depth-tested)."));
}

void hit_config_load(const utility::Config& cfg) {
    if (auto p = cfg.get("free_ride_hit_atk")) strcpy_s(hit_atk_path, p->c_str());
    hit_atk_index = cfg.get<int>("free_ride_hit_entry").value_or(hit_atk_index);
    hit_damage    = cfg.get<float>("free_ride_hit_damage").value_or(hit_damage);
    hit_interval  = cfg.get<float>("free_ride_hit_interval").value_or(hit_interval);
    hit_radius    = cfg.get<float>("free_ride_hit_radius").value_or(hit_radius);
    hit_height    = cfg.get<float>("free_ride_hit_height").value_or(hit_height);
    hit_stop      = cfg.get<int>("free_ride_hit_stop").value_or(hit_stop);
    hit_lv_i      = cfg.get<int>("free_ride_hit_lv_i").value_or(hit_lv_i);
    hit_lv_b      = cfg.get<int>("free_ride_hit_lv_b").value_or(hit_lv_b);
    hit_show      = cfg.get<bool>("free_ride_hit_show").value_or(hit_show);
}

void hit_config_save(utility::Config& cfg) {
    cfg.set("free_ride_hit_atk", std::string(hit_atk_path));
    cfg.set<int>("free_ride_hit_entry", hit_atk_index);
    cfg.set<float>("free_ride_hit_damage", hit_damage);
    cfg.set<float>("free_ride_hit_interval", hit_interval);
    cfg.set<float>("free_ride_hit_radius", hit_radius);
    cfg.set<float>("free_ride_hit_height", hit_height);
    cfg.set<int>("free_ride_hit_stop", hit_stop);
    cfg.set<int>("free_ride_hit_lv_i", hit_lv_i);
    cfg.set<int>("free_ride_hit_lv_b", hit_lv_b);
    cfg.set<bool>("free_ride_hit_show", hit_show);
}

} // namespace free_ride
