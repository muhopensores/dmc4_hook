#include "PluginGame.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../sdk/Devil4.hpp"
#include "../../mods/EnemySpawn.hpp"

// Thin __cdecl wrappers: devil4_sdk's functions are mostly __stdcall and take C++ types; plugins get C types and NULL
// checks. Left out on purpose: unit creation and the raw allocators (spawn_or_something, easy_spawn, mt_allocate_heap,
// unit_deallocate: plugin units come with their own API), uactor_sdk's constructors / collision calls, and
// get_joint_from_index (broken: pushad then ret without popad, and it reads index * 9 as an address).

namespace plugins {
namespace {

constexpr int kLines = 32;
constexpr uintptr_t kStylishCount = 0x00E558CC;   // sStylishCount::mpInstance (not get_stylish_count(): it caches its first read)
constexpr uintptr_t kWorkRate = 0x00E558D0;       // sWorkRate::mpInstance (same: get_work_rate() caches)

void* __cdecl object(int which) {
    switch (which) {
    case DMC4_OBJ_AREA: return devil4_sdk::get_sArea();
    case DMC4_OBJ_UNIT: return devil4_sdk::get_sUnit();
    case DMC4_OBJ_RENDER: return devil4_sdk::get_sRender();
    case DMC4_OBJ_MEDIATOR: return devil4_sdk::get_sMediator();
    case DMC4_OBJ_LOCAL_PLAYER: return devil4_sdk::get_local_player();
    case DMC4_OBJ_LOCAL_CAMERA: return devil4_sdk::get_local_camera();
    case DMC4_OBJ_PLAYER_CAMERA: return devil4_sdk::get_player_camera();
    case DMC4_OBJ_STYLISH_COUNT: return *reinterpret_cast<void**>(kStylishCount);
    case DMC4_OBJ_WORK_RATE: return *reinterpret_cast<void**>(kWorkRate);
    default: return nullptr;
    }
}

int __cdecl is_paused() { return devil4_sdk::is_paused(); }
int __cdecl in_gameplay() { return !devil4_sdk::is_not_in_gameplay(); }
int __cdecl is_playing() { return devil4_sdk::is_playing(); }
int __cdecl in_bloody_palace() { return devil4_sdk::is_in_bp(); }
int __cdecl is_loading() { return devil4_sdk::is_loading_arc() || devil4_sdk::is_loading_enemy(); }
int __cdecl enemy_count() { return devil4_sdk::get_enemy_count(); }
float __cdecl style_rank() { return *reinterpret_cast<void**>(kStylishCount) ? devil4_sdk::get_current_style_rank() : 0.0f; }
int __cdecl key_down(uint32_t vk) { return vk < 256 && devil4_sdk::internal_kb_check(vk); }

void* __cdecl line_top(int line) { return line >= 0 && line < kLines ? devil4_sdk::get_moveline_top(line) : nullptr; }
int __cdecl in_moveline(void* unit, int line) {
    return unit && line >= 0 && line < kLines && devil4_sdk::check_exists_in_moveline(unit, line);
}
void* __cdecl dti_of(void* obj) { return obj ? devil4_sdk::get_DTI(obj) : nullptr; }
void __cdecl center(void* unit, float* out) {
    if (!unit || !out) return;
    Vector3f v{};
    uactor_sdk::get_center_pos(unit, &v);
    out[0] = v.x;
    out[1] = v.y;
    out[2] = v.z;
}

void* __cdecl load_resource(void* dti, const char* path, uint32_t mode) {
    if (!dti || !path) return nullptr;
    return devil4_sdk::get_stuff_from_files(static_cast<MtDTI*>(dti), const_cast<char*>(path), mode);
}
void __cdecl release_resource(void* resource) {
    if (resource) devil4_sdk::release_resource(static_cast<cResource*>(resource));
}

void* __cdecl effect(const char* path, void* owner, int unit_line, uint32_t group, uint32_t material, int stationary,
                     const float* pos, const float* rot) {
    if (!path) return nullptr;
    return devil4_sdk::effect_generator(const_cast<char*>(path), owner, (uint8_t)unit_line, group, material, stationary != 0,
                                        const_cast<float*>(pos), const_cast<float*>(rot));
}
void __cdecl effect_end(void* effect) {
    if (effect) devil4_sdk::effect_cleanup(reinterpret_cast<uintptr_t>(effect));
}
void* __cdecl effect_stay(int list, int index, const float* quat, const float* pos, void* owner, int register_on_owner) {
    if (!quat || !pos) return nullptr;
    return devil4_sdk::effect_stay(list, index, quat, pos, owner, register_on_owner != 0);
}
void* __cdecl effect_const(int list, int index, void* parent, uint32_t group, uint32_t material, int unit_line, void* owner) {
    return devil4_sdk::effect_const(list, index, parent, group, material, unit_line, owner);
}
void __cdecl register_effect(void* effect, void* player, int type, int id) {
    if (effect && player) devil4_sdk::register_effect(effect, player, type, id);
}
void __cdecl kill_effect(void* player, int id) {
    if (player) devil4_sdk::kill_effect(player, id);
}
void __cdecl finish_effect(void* player, int id) {
    if (player) devil4_sdk::finish_effect(player, id);
}

void __cdecl player_se(void* player, int id) {
    if (player) devil4_sdk::play_player_se(player, id);
}

void __cdecl request_motion(void* player, int body, int motion, uint32_t attr, float interp, float start, float speed) {
    if (player && body >= DMC4_BODY_LOWER && body <= DMC4_BODY_THIRD)
        devil4_sdk::request_motion_body(player, body, motion, attr, interp, start, speed);
}
void __cdecl request_motion_all(void* player, uint32_t motion, uint32_t mode, float speed, float start, float interp) {
    if (player) devil4_sdk::indexed_anim_call(motion, static_cast<uPlayer_old*>(player), mode, speed, start, interp);
}
void __cdecl neutral_air(void* player) {
    if (player) devil4_sdk::neutral_air_transition(static_cast<uPlayer_old*>(player));
}
void* __cdecl held_weapon(void* player) { return player ? devil4_sdk::held_weapon(player) : nullptr; }
uint32_t __cdecl ground_material(void* player) { return player ? devil4_sdk::ground_material(player) : 0; }
int __cdecl load_motion_bank(void* model, const char* path, int bank) {
    return model && path && bank >= 0 && bank < 16 && devil4_sdk::load_motion_bank(model, path, bank);
}
void __cdecl unload_motion_bank(void* model, int bank) {
    if (model && bank >= 0 && bank < 16) devil4_sdk::unload_motion_bank(model, bank);
}
void* __cdecl motion_bank(void* model, int bank) {
    return model && bank >= 0 && bank < 16 ? reinterpret_cast<void*>(devil4_sdk::get_motion_bank(model, bank)) : nullptr;
}
void __cdecl borrow_ctn(void* player, uint32_t bank, uint32_t index) {
    if (player) devil4_sdk::borrow_ctn(player, bank, index);
}
void __cdecl install_ctn_script(void* player, const void* script) {
    if (player && script) devil4_sdk::install_ctn_script(player, script);
}

// version 5: EnemySpawn's list and factories (spawned near the local player, as its hotkeys do)
int __cdecl enemy_type_count() { return EnemySpawn::enemy_count(); }
const char* __cdecl enemy_type_name(int type) { return EnemySpawn::enemy_name(type); }
int __cdecl spawn_enemy(int type) {
    if (!EnemySpawn::enemy_name(type) || !devil4_sdk::get_local_player()) return 0;
    EnemySpawn::spawn_em00x(static_cast<SpawnableEnemyType>(type));
    return 1;
}

const dmc4_game_api g_game = {
    sizeof(dmc4_game_api),
    object,
    is_paused,
    in_gameplay,
    is_playing,
    in_bloody_palace,
    is_loading,
    enemy_count,
    style_rank,
    key_down,
    line_top,
    in_moveline,
    dti_of,
    center,
    load_resource,
    release_resource,
    effect,
    effect_end,
    effect_stay,
    effect_const,
    register_effect,
    kill_effect,
    finish_effect,
    player_se,
    request_motion,
    request_motion_all,
    neutral_air,
    held_weapon,
    ground_material,
    load_motion_bank,
    unload_motion_bank,
    motion_bank,
    borrow_ctn,
    install_ctn_script,
    enemy_type_count,
    enemy_type_name,
    spawn_enemy,
};

} // namespace

const dmc4_game_api* game_api() { return &g_game; }

} // namespace plugins
