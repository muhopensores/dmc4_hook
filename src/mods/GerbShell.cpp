#include "GerbShell.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
#include "KnockbackEdits.hpp"      // the kAttackStatus enum name tables
#include <vector>
#include <cmath>
#include <cstring>
#include <string>

using namespace wallrun;

namespace gerb {

bool  shell_enabled       = true;
bool  shell_show          = false;
char  shell_atk_path[128] = "Collision\\pl000";   // Nero's own file
int   shell_atk_index     = 14;                     // Dropkick: 180 dmg, LvI 3 / LvB 3, TypeB 3 (blown 2), ANG, hit stop 7
float shell_radius        = 60.0f;
float shell_length        = 120.0f;
float shell_height        = 80.0f;
float shell_life          = 90.0f;
int   shell_knock_mode    = KNOCK_JUMP_DIR;
float shell_knock_angle   = 0.0f;
int   shell_damage_type   = -1;     // vanilla attacks leave Type / TypeI at 0 and put the reaction in TypeB
int   shell_damage_type_b = -1;     // the Dropkick entry has it all: TypeB 3, LvI 3, LvB 3
int   shell_attack_lv_i   = -1;
int   shell_attack_lv_b   = -1;
int   shell_attack_id     = -1;
int   shell_range_type    = -1;
int   shell_hit_stop      = -1;
int   shell_hitmark       = -1;
int   shell_hit_se        = -1;
float shell_stylish_point = -1.0f;
float shell_stylish_timer = -1.0f;
float shell_dt_add        = -1.0f;
int   shell_attack_flag   = -1;
int   shell_blown_type    = -1;
int   shell_element       = -1;

static kAttackStatus g_entry_raw{};         // the entry as read from the file at the last spawn (the panel shows it)
static kAttackStatus g_entry_used{};        // the same after the overrides
static bool g_entry_valid = false;
float shell_damage        = -1.0f;
bool  shell_one_hit       = false;
int   shell_kind          = 1;
uint32_t shell_vsattr     = 0;
int   shell_unit_line     = 0x12;

static GerbShell* g_last_shell = nullptr;   // for the readout; cleared by its destructor
static int g_hit_count = 0;                 // onhit calls (uActor::damagePreMessage -> attacker vtable slot 37), for the readout

namespace {

// ---- game functions ----------------------------------------------------------------------------------------------
static uintptr_t fn_collMgrDtor       = 0x0050B480;   // uCollisionMgr::~uCollisionMgr(this@ecx)
static uintptr_t fn_submitLine        = 0x0045D1F0;   // sPrim::submitLine(cprim@ecx, start@edi, end, color, flags*)
static uintptr_t fn_getCPrim          = 0x00A34EE0;   // sPrim::getCPrim(ptrans@esi, sPrim inst, type, unit)
static uintptr_t sPrim_ptr            = 0x00E559D4;   // sPrim::mpInstance


void coll_mgr_dtor(uCollisionMgr* mgr) { ((void(__fastcall*)(uCollisionMgr*, int))fn_collMgrDtor)(mgr, 0); }

void* get_cprim(void* ptrans, int prim_type, void* unit) {   // (`type` is a MASM operator)
    void* res = nullptr;
    _asm {
        pushad
        mov esi, ptrans
        push unit
        push prim_type
        mov ecx, sPrim_ptr
        mov eax, [ecx]
        push eax
        call fn_getCPrim
        mov res, eax
        popad
    }
    return res;
}

void submit_line(void* cprim, const MtVector3* a, const MtVector3* b, unsigned int color, unsigned int* flags) {
    _asm {
        pushad
        mov ecx, cprim
        mov edi, a
        push flags
        push color
        push b
        call fn_submitLine
        popad
    }
}

void update_matrices(void* unit) { custom_utils::updateLmat(unit); custom_utils::updateWmat(unit); }



void assert_group_attrs(cCollisionGroup* g) {
    g->mKind   = (uint)shell_kind;
    g->mAttr   = 1;
    g->mVsAttr = shell_vsattr ? shell_vsattr : custom_utils::attack_vsattr_for_kind(shell_kind);
}

// ---- helpers -----------------------------------------------------------------------------------------------------
inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }

// a yaw about Y as an MT quaternion (x, y, z, w): forward (0, 0, 1) -> (sin yaw, 0, cos yaw), the game's atan2(x, z)
void yaw_quat(float yaw, MtVector4& q) {
    q.x = 0.0f; q.y = std::sin(yaw * 0.5f); q.z = 0.0f; q.w = std::cos(yaw * 0.5f);
}

glm::vec3 world_point(const MtMatrix& m, const glm::vec3& local) {   // rows: right, up, forward, position
    const glm::vec3 r(m.m1.x, m.m1.y, m.m1.z), u(m.m2.x, m.m2.y, m.m2.z), f(m.m3.x, m.m3.y, m.m3.z), p(m.m4.x, m.m4.y, m.m4.z);
    return p + r * local.x + u * local.y + f * local.z;
}

} // namespace


GerbShell::GerbShell(uintptr_t owner_, const glm::vec3& dir_, float yaw_, uint32_t serial_)
    : owner(owner_), serial(serial_), dir(dir_), yaw(yaw_) {
    // slot 0 (the deleting destructor) is ours too: CustomUnit replaces only the five others
    static const std::vector<uintptr_t> vt = [] {
        std::vector<uintptr_t> v = vtable();
        void (GerbShell::*m)(uint32_t) = &GerbShell::destructor;
        uintptr_t p; std::memcpy(&p, &m, sizeof p);
        v[0] = p;
        return v;
    }();
    this->vtable_ptr = (uintptr_t*)vt.data();

    std::memset(&coll, 0, sizeof(coll));
    custom_utils::uCollisionMgrCons(&coll);
    std::memset(&prim, 0, sizeof(prim));
    std::memset(&atk, 0, sizeof(atk));
    this->mActorType = 5;               // a shell, as the game's own
    g_last_shell = this;
    this->mWorkRate.mType = 9;
    const MtVector3& p = at<MtVector3>(owner, pl::mPos);
    this->mPos.x = p.x; this->mPos.y = p.y + shell_height; this->mPos.z = p.z;
    yaw_quat(yaw, this->mQuat);
}

void GerbShell::destructor(uint32_t flag) {
    if (g_last_shell == this) g_last_shell = nullptr;
    coll_mgr_dtor(&coll);               // releases the groups and the .atk reference init_0 took
    CustomActor::destructor(flag);
}

// Called by the unit system once the shell is in the list: the ice's setup, with a player-shell kind.
void GerbShell::startup() {
    update_matrices(this);
    uintptr_t rsrc = (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)rAtckStatDTI, shell_atk_path, 1);
    if (!rsrc) { spdlog::warn("GerbShell: '{}.atk' did not load", shell_atk_path); end(); return; }
    custom_utils::load_atk_col((void*)rsrc, nullptr, &coll, this);      // uCollisionMgr::init_0(this, &coll, 0, atk)
    if (!custom_utils::read_attack_entry((const void*)rsrc, shell_atk_index, atk)) {
        spdlog::warn("GerbShell: '{}.atk' has no entry {}", shell_atk_path, shell_atk_index);
        devil4_sdk::release_resource((cResource*)rsrc); end(); return;
    }
    devil4_sdk::release_resource((cResource*)rsrc);                      // init_0 holds its own reference
    g_entry_raw = atk;
    if (shell_damage_type   >= 0)   atk.mDamageType  = atk.mDamageTypeI = shell_damage_type;   // damagePreMessage copies TypeI over Type
    if (shell_damage_type_b >= 0)   atk.mDamageTypeB = shell_damage_type_b;
    if (shell_attack_lv_i   >= 0)   atk.mAttackLv = atk.mAttackLvI = shell_attack_lv_i;   // damagePreMessage copies LvI over Lv
    if (shell_attack_lv_b   >= 0)   atk.mAttackLvB = shell_attack_lv_b;
    if (shell_damage        >= 0.0f) atk.mDamageValue = shell_damage;
    if (shell_attack_id     >= 0)    atk.mAttackID      = shell_attack_id;
    if (shell_range_type    >= 0)    atk.mRangeType     = shell_range_type;
    if (shell_hit_stop      >= 0)    atk.mHitStopTimer  = shell_hit_stop;
    if (shell_hitmark       >= 0)    atk.mHitmarkAngle  = shell_hitmark;
    if (shell_hit_se        >= 0)    atk.mHitSE         = shell_hit_se;
    if (shell_stylish_point >= 0.0f) atk.mStylishPoint  = shell_stylish_point;
    if (shell_stylish_timer >= 0.0f) atk.mStylishTimer  = shell_stylish_timer;
    if (shell_dt_add        >= 0.0f) atk.mDTAdd         = shell_dt_add;
    if (shell_attack_flag   >= 0)    atk.mAttackFlag    = shell_attack_flag;
    if (shell_element       >= 0)    atk.mElementType   = shell_element;
    atk.mBlownAngleType = shell_blown_type >= 0 ? shell_blown_type : (shell_knock_mode == KNOCK_AWAY_FROM_PLAYER ? 0 /* POS */ : 1 /* ANG */);
    g_entry_used = atk; g_entry_valid = true;

    // the capsule: from the shell's origin, `shell_length` along the jump, in the shell's own frame (its forward is
    // the knock direction, not necessarily the jump)
    const float c = std::cos(-yaw), s = std::sin(-yaw);
    const glm::vec3 d = dir * shell_length;
    const glm::vec3 local(c * d.x + s * d.z, d.y, -s * d.x + c * d.z);
    prim.mType  = PRIM_TYPE_CAPSULE;
    prim.mBone0 = prim.mBone1 = -1;
    prim.mRadius = shell_radius;
    prim.mPos0.x = 0.0f;    prim.mPos0.y = 0.0f;    prim.mPos0.z = 0.0f;
    prim.mPos1.x = local.x; prim.mPos1.y = local.y; prim.mPos1.z = local.z;
    prim.mAttr = 0; prim.mId = 0; prim.mShrink = 0.0f; prim.mDummy = -1;

    // the ice's steps 4-5: the group with this capsule, the attack copy, kind 1 Ps (a player shell; 0 Pl, 2 Em, 4 Es = the
    // ice's), attr 1 = attack, the vs bits of that kind, life, following the shell's matrix
    group = custom_utils::add_prim_attack_group(&coll, &prim, &atk, shell_kind, shell_life, &this->mWmat);
    if (!group) { spdlog::warn("GerbShell: setCollision failed"); end(); return; }
    assert_group_attrs(group);         // the panel's kind / vs overrides, re-asserted each tick
}

void GerbShell::tick() {
    if (ended) return;
    if (!jump_active(owner, serial)) { end(); return; }
    const MtVector3& p = at<MtVector3>(owner, pl::mPos);
    this->mPos.x = p.x; this->mPos.y = p.y + shell_height; this->mPos.z = p.z;
    yaw_quat(yaw, this->mQuat);
    update_matrices(this);
    custom_utils::collide(&coll);       // uCollisionMgr::move: the group's world matrix and its life
    if (group && !custom_utils::group_in_table(&coll, group)) group = nullptr;   // freed by move (timer >= mLife)
    if (group) assert_group_attrs(group);
    age += this->m_delta_time;
    if (age >= shell_life) end();
}

void GerbShell::onhit(void* /*atk_param*/, void* /*dfd_param*/) {
    ++g_hit_count;
    if (shell_one_hit) end();
}

// The capsule as in-game lines: the axis, four longitudinal edges, and rings at both ends (3 planes each).
void GerbShell::render(void* ptrans) {
    if (!shell_show || ended || !group) return;
    sMediator* smed = devil4_sdk::get_sMediator();
    void* cprim = get_cprim(ptrans, 6, smed ? (void*)smed->player_ptr : (void*)this);
    if (!cprim) return;
    unsigned int flags[2] = { 0xFFFFFFFF, 16 };
    const unsigned int color = 0xFF00FFFF;
    auto line = [&](const glm::vec3& a, const glm::vec3& b) {
        MtVector3 va{ a.x, a.y, a.z, 0 }, vb{ b.x, b.y, b.z, 0 };
        submit_line(cprim, &va, &vb, color, flags);
    };
    const glm::vec3 p0 = world_point(this->mWmat, glm::vec3(prim.mPos0.x, prim.mPos0.y, prim.mPos0.z));
    const glm::vec3 p1 = world_point(this->mWmat, glm::vec3(prim.mPos1.x, prim.mPos1.y, prim.mPos1.z));
    glm::vec3 axis = p1 - p0;
    const float len = glm::length(axis);
    axis = len > 1e-3f ? axis / len : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 helper = std::fabs(axis.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 u = glm::normalize(glm::cross(axis, helper)), v = glm::cross(axis, u);
    const float r = prim.mRadius;
    const int N = 16;
    line(p0, p1);
    for (int k = 0; k < 4; ++k) {
        const float a = k * 1.5707963f;
        const glm::vec3 o = (u * std::cos(a) + v * std::sin(a)) * r;
        line(p0 + o, p1 + o);
    }
    for (const glm::vec3& c : { p0, p1 }) {
        for (int i = 0; i < N; ++i) {
            const float a0 = i * 6.2831853f / N, a1 = (i + 1) * 6.2831853f / N;
            line(c + (u * std::cos(a0) + v * std::sin(a0)) * r, c + (u * std::cos(a1) + v * std::sin(a1)) * r);   // ring around the axis
            line(c + (axis * std::cos(a0) + u * std::sin(a0)) * r, c + (axis * std::cos(a1) + u * std::sin(a1)) * r);
            line(c + (axis * std::cos(a0) + v * std::sin(a0)) * r, c + (axis * std::cos(a1) + v * std::sin(a1)) * r);
        }
    }
}

void GerbShell::end() {
    if (ended) return;
    ended = true;
    die();                              // the unit system destroys it (-> destructor, which closes the collision manager)
}

// ---- Gerb's side -----------------------------------------------------------------------------------------------------
bool spawn_shell(uintptr_t player, const glm::vec3& dir, uint32_t serial) {
    if (!shell_enabled || !player) return false;
    const glm::vec3 level(dir.x, 0.0f, dir.z);
    float yaw;
    switch (shell_knock_mode) {
    case KNOCK_PLAYER_FACING: yaw = at<float>(player, pl::mRotateNow); break;
    default:                  yaw = glm::length(level) > 1e-3f ? yaw_of(level) : at<float>(player, pl::mRotateNow); break;   // vertical jumps: the facing
    }
    yaw += shell_knock_angle * 3.14159265f / 180.0f;
    GerbShell* s = GerbShell::spawn(shell_unit_line, player, dir, yaw, serial);
    return s != nullptr;
}

// The live shell's group and the player's own groups (mppCollisionGroup = mgr + 0x358, 32 slots): kind / attr / vs.
void shell_debug_readout(uintptr_t player) {
    if (g_last_shell && g_last_shell->group && custom_utils::group_in_table(&g_last_shell->coll, g_last_shell->group)) {
        const cCollisionGroup* g = g_last_shell->group;
        ImGui::Text("shell: groups in mgr %u, age %.0f, hits so far %d", g_last_shell->coll.mCollisionGroupNum, g_last_shell->age, g_hit_count);
        ImGui::Text("shell group: kind %u attr %u vs %08X life %.0f timer %.0f prim %d r %.0f  mgr enable %d mode %u  unit state %u",
                    g->mKind, g->mAttr, g->mVsAttr, g->mLife, g->mTimer, (int)g_last_shell->prim.mType, g_last_shell->prim.mRadius,
                    (int)g_last_shell->coll.mEnable, at<uint32_t>((uintptr_t)&g_last_shell->coll, 0x350), at<uint32_t>((uintptr_t)g_last_shell, 4) & 7);
        if (g->mNum > 0 && g->mppCollision && g->mppCollision[0]) {
            const cCollision* c = g->mppCollision[0];
            ImGui::Text("  capsule world (%.0f %.0f %.0f) scale %.2f aabbHit %d hit %d | shell pos (%.0f %.0f %.0f) group aabbHit %d",
                        c->mMat0.m4.x, c->mMat0.m4.y, c->mMat0.m4.z, c->mScale, (int)c->mAABBHit, (int)c->mHit,
                        g_last_shell->mPos.x, g_last_shell->mPos.y, g_last_shell->mPos.z, (int)g->mAABBHit);
        }
    } else {
        ImGui::Text("shell group: none alive (shell %p, group %p%s)", (void*)g_last_shell, g_last_shell ? (void*)g_last_shell->group : nullptr,
                    g_last_shell && g_last_shell->group ? ", not in the manager's table" : "");
    }
    const uintptr_t mgr = player ? at<uintptr_t>(player, pl::mCollMgr) : 0;
    if (!mgr) return;
    std::string s = "player groups:";
    for (int i = 0; i < 32; ++i) {
        const cCollisionGroup* g = at<cCollisionGroup*>(mgr, 0x358 + 4 * i);
        if (!g) continue;
        char b[64]; snprintf(b, sizeof b, " [%d k%u a%u %08X]", i, g->mKind, g->mAttr, g->mVsAttr); s += b;
    }
    ImGui::TextWrapped("%s", s.c_str());
}

// A combo over an enum name table with a leading "(entry's)" item for -1.
static void combo_override(const char* label, int* value, const char* const* names, int count, const char* keep_label = "(entry's)") {
    std::vector<const char*> items; items.reserve(count + 1);
    items.push_back(keep_label);
    for (int i = 0; i < count; ++i) items.push_back(names[i]);
    int cur = (*value >= 0 && *value < count) ? *value + 1 : 0;
    if (ImGui::Combo(label, &cur, items.data(), (int)items.size())) *value = cur - 1;
}

void shell_gui() {
    ImGui::Checkbox(_("Hitbox shell"), &shell_enabled);
    ImGui::SameLine();
    Mod::help_marker(_("A shell (the game's projectile unit) rides the player through the jump with one capsule attack group, "
                  "the way Frost's ice shot carries its hit (uEm019_ice::setup), and knocks what it hits in the direction "
                  "of the jump. Lives while the jump is airborne."));
    if (!shell_enabled) return;
    ImGui::SameLine(Mod::sameLineWidth);
    ImGui::Checkbox(_("Show shell"), &shell_show);
    ImGui::SameLine();
    Mod::help_marker(_("Draws the capsule with the game's own line primitive (depth-tested, like RayTest's ray)."));
    ImGui::InputText(_("Attack file"), shell_atk_path, sizeof(shell_atk_path));
    ImGui::SameLine();
    Mod::help_marker(_("rAttackStatusData (.atk) path without the extension; its entry below is copied into the shell's group. "
                  "The player's own file is Collision\\pl000."));
    ImGui::InputInt(_("Attack entry"), &shell_atk_index);
    {
        const char* mode_names[] = { "Jump direction (world)", "Player facing", "Away from the player" };
        ImGui::Combo(_("Knock direction"), &shell_knock_mode, mode_names, 3);
        ImGui::SameLine();
        Mod::help_marker(_("Where the enemy flies: along the jump in world space (the shell faces the jump, blown angle type "
                      "ANG), along the player's facing (ANG with the player's yaw), or straight away from the player "
                      "(blown angle type POS, the game's position mode). Vertical jumps use the facing for the first two."));
        if (shell_knock_mode != KNOCK_AWAY_FROM_PLAYER) ImGui::SliderFloat(_("Knock angle offset"), &shell_knock_angle, -180.0f, 180.0f);
    }
    ImGui::SliderFloat(_("Shell radius"), &shell_radius, 5.0f, 300.0f);
    ImGui::SliderFloat(_("Shell length"), &shell_length, 0.0f, 400.0f);
    ImGui::SliderFloat(_("Shell height"), &shell_height, 0.0f, 200.0f);
    ImGui::SliderFloat(_("Shell life (frames)"), &shell_life, 1.0f, 300.0f);
    ImGui::Checkbox(_("One hit then end"), &shell_one_hit);

    // ---- the attack entry and every field of it ---------------------------------------------------------------
    if (ImGui::CollapsingHeader(_("Attack reaction (kAttackStatus overrides, -1 = the entry's)"))) {
        if (g_entry_valid) {
            const kAttackStatus& e = g_entry_raw;
            ImGui::TextWrapped("entry '%s': id %d dmg %.0f lv %d lvI %d lvB %d range %d hitstop %d type %d typeI %d typeB %d mark %d se %d "
                               "stylish %.0f/%.0f dt %.0f flag %d blownAng %d elem %d",
                               e.mAsName, e.mAttackID, e.mDamageValue, e.mAttackLv, e.mAttackLvI, e.mAttackLvB, e.mRangeType, e.mHitStopTimer,
                               e.mDamageType, e.mDamageTypeI, e.mDamageTypeB, e.mHitmarkAngle, e.mHitSE, e.mStylishPoint, e.mStylishTimer,
                               e.mDTAdd, e.mAttackFlag, e.mBlownAngleType, e.mElementType);
            const kAttackStatus& u = g_entry_used;
            ImGui::TextWrapped("used:   lvI %d lvB %d typeI %d typeB %d blownAng %d dmg %.0f hitstop %d mark %d", u.mAttackLvI, u.mAttackLvB,
                               u.mDamageTypeI, u.mDamageTypeB, u.mBlownAngleType, u.mDamageValue, u.mHitStopTimer, u.mHitmarkAngle);
        } else {
            ImGui::Text(_("(no shell spawned yet: the entry is read at the first launch)"));
        }
        combo_override(_("Damage type B (reaction)"), &shell_damage_type_b, damage_type_names, IM_ARRAYSIZE(damage_type_names) - 1);
        ImGui::SameLine();
        Mod::help_marker(_("mDamageTypeB, the reaction. Nero's own entries keep Type / TypeI at 0 and use this: 2 blown (Streak), 3 blown 2 "
                           "(Dropkick), 4 upper / launch (High Roller), 5 ground (Split), 6 bound."));
        ImGui::InputInt(_("Attack level B"), &shell_attack_lv_b);
        ImGui::SameLine();
        Mod::help_marker(_("mAttackLvB: the blow level, judged against the enemy's kDefendStatus mMaxBlown. Streak / High Roller 2, Dropkick 3; "
                           "0 blows nothing."));
        ImGui::InputInt(_("Attack level I"), &shell_attack_lv_i);
        ImGui::SameLine();
        Mod::help_marker(_("mAttackLvI (copied over mAttackLv at the hit): the interrupt level, judged against mMaxInterrupt. Streak 2, "
                           "Dropkick 3; 0 interrupts nothing."));
        combo_override(_("Damage type / type I"), &shell_damage_type, damage_type_names, IM_ARRAYSIZE(damage_type_names) - 1);
        ImGui::SameLine();
        Mod::help_marker(_("mDamageType and mDamageTypeI (uActor::damagePreMessage copies I over the type before the enemy reads it). "
                           "Vanilla attacks leave both 0."));
        combo_override(_("Blown angle type"), &shell_blown_type, blown_angle_type_names, IM_ARRAYSIZE(blown_angle_type_names) - 1, "(from the knock mode)");
        ImGui::SameLine();
        Mod::help_marker(_("mBlownAngleType as uActor::calcDamageVec reads it: 0 Position = away from the shell, 1 Angle = the shell's facing "
                           "(what the knock modes set). 2-5 aren't handled in DX9 (world +Z)."));
        ImGui::InputFloat(_("Damage"), &shell_damage);
        ImGui::InputInt(_("Hit stop (frames)"), &shell_hit_stop);
        combo_override(_("Hit mark angle"), &shell_hitmark, hit_mark_angle_names, IM_ARRAYSIZE(hit_mark_angle_names) - 1);
        ImGui::SameLine();
        Mod::help_marker(_("mHitmarkAngle: the hit spark's direction and size class (the Dropkick's 62 = BLOW2 from the left)."));
        ImGui::InputInt(_("Hit SE"), &shell_hit_se);
        combo_override(_("Range type"), &shell_range_type, range_type_names, IM_ARRAYSIZE(range_type_names) - 1);
        ImGui::SameLine();
        Mod::help_marker(_("mRangeType: melee / ranged / grab. A grab (2) passes damagePreMessage's 0x200 status gate like a grab does."));
        combo_override(_("Attack id"), &shell_attack_id, attack_id_names, IM_ARRAYSIZE(attack_id_names) - 1);
        ImGui::SameLine();
        Mod::help_marker(_("mAttackID (ATTACK_ID): what move the hit counts as for the enemy's routing and the style meter. 13 = no damage "
                           "message at all."));
        ImGui::InputFloat(_("Stylish points"), &shell_stylish_point);
        ImGui::InputFloat(_("Stylish timer"), &shell_stylish_timer);
        ImGui::InputFloat(_("DT add"), &shell_dt_add);
        combo_override(_("Attack flag"), &shell_attack_flag, attack_flag_names, IM_ARRAYSIZE(attack_flag_names) - 1);
        combo_override(_("Element"), &shell_element, element_type_dmc4_names, IM_ARRAYSIZE(element_type_dmc4_names) - 1);
    }
    ImGui::InputInt(_("Unit line"), &shell_unit_line);
    ImGui::SameLine();
    Mod::help_marker(_("sUnit line the shell is added on. Nero's shells use 0x12, CustomProjectile 1; the main loop ticks every line before "
                       "the attack pass, so this mostly orders the shell against other units."));
    ImGui::InputInt(_("Group kind"), &shell_kind);
    ImGui::SameLine();
    Mod::help_marker(_("cCollisionGroup::mKind, the chain the group is filed under: 0 Pl (the player's own body / melee), 1 Ps "
                       "(player shell, Nero's bullets), 2 Em, 3 Em2, 4 Es (enemy shell, the ice), 5 Es2, 6 Set, 7 Stg."));
    ImGui::InputScalar(_("Vs attr (0 = table)"), ImGuiDataType_U32, &shell_vsattr, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    Mod::help_marker(_("Written into the group as is when not 0. Nibble k = target kind k: bit 2 = this attack reaches that kind, "
                       "bit 1 = (on a damage group) that kind's attacks reach it. The player's melee and shells carry 0x22222200."));
}

void shell_config_load(const utility::Config& cfg) {
    shell_enabled       = cfg.get<bool>("gerb_shell").value_or(shell_enabled);
    shell_show          = cfg.get<bool>("gerb_shell_show").value_or(shell_show);
    // the .atk path isn't persisted: utility::Config has no string values (WallRun's LMT paths aren't either)
    shell_atk_index     = cfg.get<int>("gerb_shell_atk_index").value_or(shell_atk_index);
    shell_radius        = cfg.get<float>("gerb_shell_radius").value_or(shell_radius);
    shell_length        = cfg.get<float>("gerb_shell_length").value_or(shell_length);
    shell_height        = cfg.get<float>("gerb_shell_height").value_or(shell_height);
    shell_life          = cfg.get<float>("gerb_shell_life").value_or(shell_life);
    shell_knock_mode    = cfg.get<int>("gerb_shell_knock_mode").value_or(shell_knock_mode);
    shell_knock_angle   = cfg.get<float>("gerb_shell_knock_angle").value_or(shell_knock_angle);
    shell_damage_type   = cfg.get<int>("gerb_shell_damage_type").value_or(shell_damage_type);
    shell_damage_type_b = cfg.get<int>("gerb_shell_damage_type_b").value_or(shell_damage_type_b);
    shell_attack_lv_i   = cfg.get<int>("gerb_shell_attack_lv_i").value_or(shell_attack_lv_i);
    shell_attack_lv_b   = cfg.get<int>("gerb_shell_attack_lv_b").value_or(shell_attack_lv_b);
    shell_attack_id     = cfg.get<int>("gerb_shell_attack_id").value_or(shell_attack_id);
    shell_range_type    = cfg.get<int>("gerb_shell_range_type").value_or(shell_range_type);
    shell_hit_stop      = cfg.get<int>("gerb_shell_hit_stop").value_or(shell_hit_stop);
    shell_hitmark       = cfg.get<int>("gerb_shell_hitmark").value_or(shell_hitmark);
    shell_hit_se        = cfg.get<int>("gerb_shell_hit_se").value_or(shell_hit_se);
    shell_stylish_point = cfg.get<float>("gerb_shell_stylish_point").value_or(shell_stylish_point);
    shell_stylish_timer = cfg.get<float>("gerb_shell_stylish_timer").value_or(shell_stylish_timer);
    shell_dt_add        = cfg.get<float>("gerb_shell_dt_add").value_or(shell_dt_add);
    shell_attack_flag   = cfg.get<int>("gerb_shell_attack_flag").value_or(shell_attack_flag);
    shell_blown_type    = cfg.get<int>("gerb_shell_blown_type").value_or(shell_blown_type);
    shell_element       = cfg.get<int>("gerb_shell_element").value_or(shell_element);
    shell_damage        = cfg.get<float>("gerb_shell_damage").value_or(shell_damage);
    shell_one_hit       = cfg.get<bool>("gerb_shell_one_hit").value_or(shell_one_hit);
    shell_kind          = cfg.get<int>("gerb_shell_kind").value_or(shell_kind);
    shell_unit_line     = cfg.get<int>("gerb_shell_unit_line").value_or(shell_unit_line);
    shell_vsattr        = (uint32_t)cfg.get<int>("gerb_shell_vsattr").value_or((int)shell_vsattr);
}

void shell_config_save(utility::Config& cfg) {
    cfg.set<bool>("gerb_shell", shell_enabled);
    cfg.set<bool>("gerb_shell_show", shell_show);
    cfg.set<int>("gerb_shell_atk_index", shell_atk_index);
    cfg.set<float>("gerb_shell_radius", shell_radius);
    cfg.set<float>("gerb_shell_length", shell_length);
    cfg.set<float>("gerb_shell_height", shell_height);
    cfg.set<float>("gerb_shell_life", shell_life);
    cfg.set<int>("gerb_shell_knock_mode", shell_knock_mode);
    cfg.set<float>("gerb_shell_knock_angle", shell_knock_angle);
    cfg.set<int>("gerb_shell_damage_type", shell_damage_type);
    cfg.set<int>("gerb_shell_damage_type_b", shell_damage_type_b);
    cfg.set<int>("gerb_shell_attack_lv_i", shell_attack_lv_i);
    cfg.set<int>("gerb_shell_attack_lv_b", shell_attack_lv_b);
    cfg.set<int>("gerb_shell_attack_id", shell_attack_id);
    cfg.set<int>("gerb_shell_range_type", shell_range_type);
    cfg.set<int>("gerb_shell_hit_stop", shell_hit_stop);
    cfg.set<int>("gerb_shell_hitmark", shell_hitmark);
    cfg.set<int>("gerb_shell_hit_se", shell_hit_se);
    cfg.set<float>("gerb_shell_stylish_point", shell_stylish_point);
    cfg.set<float>("gerb_shell_stylish_timer", shell_stylish_timer);
    cfg.set<float>("gerb_shell_dt_add", shell_dt_add);
    cfg.set<int>("gerb_shell_attack_flag", shell_attack_flag);
    cfg.set<int>("gerb_shell_blown_type", shell_blown_type);
    cfg.set<int>("gerb_shell_element", shell_element);
    cfg.set<float>("gerb_shell_damage", shell_damage);
    cfg.set<bool>("gerb_shell_one_hit", shell_one_hit);
    cfg.set<int>("gerb_shell_kind", shell_kind);
    cfg.set<int>("gerb_shell_unit_line", shell_unit_line);
    cfg.set<int>("gerb_shell_vsattr", (int)shell_vsattr);
}

} // namespace gerb
