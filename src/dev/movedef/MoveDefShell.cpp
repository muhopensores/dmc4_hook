#include "MoveDefShell.hpp"
#include "../../mods/WallRun.hpp"             // wallrun::at / wallrun::pl offsets
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sMediator.hpp"
#include "../../sdk/DebugDraw.hpp"
#include <cmath>
#include <cstring>

using namespace wallrun;

namespace movedef {

namespace {

constexpr uintptr_t fn_collMgrDtor = 0x0050B480;   // uCollisionMgr::~uCollisionMgr(this@ecx)

int g_alive = 0;                   // readout
int g_hits = 0;

void coll_mgr_dtor(uCollisionMgr* mgr) { ((void(__fastcall*)(uCollisionMgr*, int))fn_collMgrDtor)(mgr, 0); }

void update_matrices(void* unit) { custom_utils::updateLmat(unit); custom_utils::updateWmat(unit); }

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }

// The entry's kAttackStatus with the file's overrides (each < 0 = keep), as GerbShell::startup does it.
void apply_overrides(const xmlmove::Shell& d, kAttackStatus& a) {
    if (d.damage_type   >= 0)    a.mDamageType  = a.mDamageTypeI = d.damage_type;   // damagePreMessage copies TypeI over Type
    if (d.damage_type_b >= 0)    a.mDamageTypeB = d.damage_type_b;
    if (d.level_i       >= 0)    a.mAttackLv = a.mAttackLvI = d.level_i;            // and LvI over Lv
    if (d.level_b       >= 0)    a.mAttackLvB = d.level_b;
    if (d.damage        >= 0.0f) a.mDamageValue   = d.damage;
    if (d.attack_id     >= 0)    a.mAttackID      = d.attack_id;
    if (d.range_type    >= 0)    a.mRangeType     = d.range_type;
    if (d.hit_stop      >= 0)    a.mHitStopTimer  = d.hit_stop;
    if (d.hitmark       >= 0)    a.mHitmarkAngle  = d.hitmark;
    if (d.hit_se        >= 0)    a.mHitSE         = d.hit_se;
    if (d.stylish_point >= 0.0f) a.mStylishPoint  = d.stylish_point;
    if (d.stylish_timer >= 0.0f) a.mStylishTimer  = d.stylish_timer;
    if (d.dt_add        >= 0.0f) a.mDTAdd         = d.dt_add;
    if (d.attack_flag   >= 0)    a.mAttackFlag    = d.attack_flag;
    if (d.element       >= 0)    a.mElementType   = d.element;
    a.mBlownAngleType = d.blown_angle >= 0 ? d.blown_angle : (d.knock == 2 ? 0 /* POS */ : 1 /* ANG */);
}

void assert_group_attrs(const xmlmove::Shell& d, cCollisionGroup* g) {
    g->mKind   = (uint)d.kind;
    g->mAttr   = 1;
    g->mVsAttr = d.vsattr ? d.vsattr : custom_utils::attack_vsattr_for_kind(d.kind);
}

} // namespace

AttackShell::AttackShell(const xmlmove::Shell& def_, uintptr_t owner_, const glm::vec3& dir_, float yaw_, uint32_t move_id_, uint32_t serial_)
    : def(def_), owner(owner_), move_id(move_id_), serial(serial_), dir(dir_), yaw(yaw_) {
    std::memset(&coll, 0, sizeof(coll));
    custom_utils::uCollisionMgrCons(&coll);
    std::memset(&prim, 0, sizeof(prim));
    std::memset(&atk, 0, sizeof(atk));
    this->mActorType = 5;              // a shell, as the game's own
    this->mWorkRate.mType = 9;
    const MtVector3& p = at<MtVector3>(owner, pl::mPos);
    this->mPos.x = p.x; this->mPos.y = p.y + def.height; this->mPos.z = p.z;
    mt_yaw_quat(yaw, this->mQuat);
    ++g_alive;
}

void AttackShell::destructor(uint32_t flag) {
    --g_alive;
    coll_mgr_dtor(&coll);              // releases the groups and the .atk reference init_0 took
    CustomActor::destructor(flag);
}

void AttackShell::startup() {
    update_matrices(this);
    const uintptr_t rsrc = (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)rAtckStatDTI, def.atk, 1);
    if (!rsrc) { spdlog::warn("MoveDef: <shell> '{}.atk' did not load", def.atk); end(); return; }
    custom_utils::load_atk_col((void*)rsrc, nullptr, &coll, this);      // uCollisionMgr::init_0(this, &coll, 0, atk)
    if (!custom_utils::read_attack_entry((const void*)rsrc, def.entry, atk)) {
        spdlog::warn("MoveDef: <shell> '{}.atk' has no entry {}", def.atk, def.entry);
        devil4_sdk::release_resource((cResource*)rsrc); end(); return;
    }
    devil4_sdk::release_resource((cResource*)rsrc);                      // init_0 holds its own reference
    apply_overrides(def, atk);

    // the capsule from the shell's origin, `length` along the direction, in the shell's own frame (its forward is the
    // knock direction, not necessarily the move's)
    const float c = std::cos(-yaw), s = std::sin(-yaw);
    const glm::vec3 d = dir * def.length;
    const glm::vec3 local(c * d.x + s * d.z, d.y, -s * d.x + c * d.z);
    prim.mType  = PRIM_TYPE_CAPSULE;
    prim.mBone0 = prim.mBone1 = -1;
    prim.mRadius = def.radius;
    prim.mPos0.x = 0.0f;    prim.mPos0.y = 0.0f;    prim.mPos0.z = 0.0f;
    prim.mPos1.x = local.x; prim.mPos1.y = local.y; prim.mPos1.z = local.z;
    prim.mAttr = 0; prim.mId = 0; prim.mShrink = 0.0f; prim.mDummy = -1;

    group = custom_utils::add_prim_attack_group(&coll, &prim, &atk, def.kind, def.life, &this->mWmat);
    if (!group) { spdlog::warn("MoveDef: <shell> setCollision failed"); end(); return; }
    assert_group_attrs(def, group);
}

void AttackShell::tick() {
    if (ended) return;
    if (!shell_alive(owner, move_id, serial)) { end(); return; }
    const MtVector3& p = at<MtVector3>(owner, pl::mPos);
    this->mPos.x = p.x; this->mPos.y = p.y + def.height; this->mPos.z = p.z;
    mt_yaw_quat(yaw, this->mQuat);
    update_matrices(this);
    custom_utils::collide(&coll);      // uCollisionMgr::move: the group's world matrix and its life
    if (group && !custom_utils::group_in_table(&coll, group)) group = nullptr;   // freed by move (timer >= mLife)
    if (group) assert_group_attrs(def, group);
    age += this->m_delta_time;
    if (age >= def.life) end();
}

void AttackShell::onhit(void*, void*) {
    ++g_hits;
    if (def.one_hit) end();
}

void AttackShell::render(void* ptrans) {
    if (!def.show || ended || !group) return;
    sMediator* smed = devil4_sdk::get_sMediator();
    void* cprim = debug_draw::get_cprim(ptrans, 6, smed ? (void*)smed->player_ptr : (void*)this);
    const glm::vec3 p0 = debug_draw::world_point(this->mWmat, glm::vec3(prim.mPos0.x, prim.mPos0.y, prim.mPos0.z));
    const glm::vec3 p1 = debug_draw::world_point(this->mWmat, glm::vec3(prim.mPos1.x, prim.mPos1.y, prim.mPos1.z));
    debug_draw::draw_capsule(cprim, p0, p1, prim.mRadius, 0xFF00FFFF);
}

void AttackShell::end() {
    if (ended) return;
    ended = true;
    die();                             // the unit system destroys it (-> destructor)
}

bool spawn_shell(const xmlmove::Shell& def, uintptr_t player, const glm::vec3& dir, uint32_t move_id, uint32_t serial) {
    if (!player) return false;
    const glm::vec3 level(dir.x, 0.0f, dir.z);
    float yaw;
    if (def.knock == 1) yaw = at<float>(player, pl::mRotateNow);                          // facing
    else yaw = glm::length(level) > 1e-3f ? yaw_of(level) : at<float>(player, pl::mRotateNow);   // vertical: the facing
    yaw += def.knock_angle * 3.14159265f / 180.0f;
    return AttackShell::spawn(def.unit_line, def, player, dir, yaw, move_id, serial) != nullptr;
}

int shells_alive() { return g_alive; }
int shell_hits() { return g_hits; }

} // namespace movedef
