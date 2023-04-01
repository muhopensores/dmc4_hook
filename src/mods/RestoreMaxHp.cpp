
#include "RestoreMaxHp.hpp"

bool RestoreMaxHp::mod_enabled{ false };

static float two_minutes_timer = 7200.0f;
static bool reset_timer = false;
static bool limit_to_ground = false;

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

std::optional<std::string> RestoreMaxHp::on_initialize() {
    return Mod::on_initialize();
}

/*
I don't think there's a pointer chain in common, they just set up esi before calling damage, e.g.
frost has
DevilMayCry4_DX9.exe+1AD0ED - lea esi,[ebx+00001504] 
[...]
call damage

scarecrow has
DevilMayCry4_DX9.exe+157D9F - lea eax,[ebx+0000152C]
DevilMayCry4_DX9.exe+157E4D - mov esi,eax
[...]
call damage
*/

int RestoreMaxHp::get_enemy_specific_damage_offset(int enemy_id) {
    switch (enemy_id) {
    // 0x1500
    case 0x8: // Mephisto
        return 0x1500;
    case 0x9: // Faust
        return 0x1500;
    case 0xB: // Assault
        return 0x1500;
    case 0x10: // Gladius
        return 0x1500;

    // 0x1504
    case 0x5: // Alto
        return 0x1504;
    case 0x6: // Bianco
        return 0x1504;
    case 0xA: // Frost
        return 0x1504;
    case 0xC: // Blitz
        return 0x1504;

    // 1508
    case 0xF: // Cutlass
        return 0x1508;

    // 0x152C
    case 0x0: // Scarecrow Leg
        return 0x152C;
    case 0x1: // Scarecrow Arm
        return 0x152C;
    case 0x3: // Scarecrow Mega
        return 0x152C;

    // 7FC4
    case 0x11: // Basilisk
        return 0x7FC4;
    }
    return NULL;
    // seeds aren't on the enemy list
    // i am too lazy to find a fault but they probably aren't either
}

void RestoreMaxHp::on_frame(fmilliseconds& dt) {
    // uintptr_t* sMedPtr = *(uintptr_t**)staticMediatorPtr;
    // uintptr_t* uLocalPlr = *(uintptr_t**)((uintptr_t)sMedPtr + 0x24);
    uintptr_t* s_med_ptr = (uintptr_t*)static_mediator_ptr;
    uintptr_t s_mediator = *s_med_ptr;
    uintptr_t* player_ptr = (uintptr_t*)(s_mediator + 0x24);
    uintptr_t u_local_plr = *player_ptr;
    if (u_local_plr) {
        uint8_t& grounded = *(uint8_t*)(u_local_plr + 0xEA8);
        uint8_t& desired_input = *(uint8_t*)(u_local_plr + 0x140C);
        if (mod_enabled) {
            if (grounded == 1) {
                goto reset_hp_and_timer;
            }
            else if (!limit_to_ground) {
                reset_hp_and_timer:
                if (desired_input & 0x10 && desired_input & 0x08) {
                    int8_t& enemy_count = *(int8_t*)(s_mediator + 0x1E8);
                    for (int i = 1; i <= enemy_count; i++) {
                        uintptr_t* enemy_ptr = (uintptr_t*)(s_mediator + 0x1B4 + i * 4);
                        uintptr_t enemy_base = *enemy_ptr;
                        if (enemy_base) {
                            int& enemy_id = *(int*)(enemy_base + 0x1410);
                            int damage_info_offset = get_enemy_specific_damage_offset(enemy_id);
                            if (damage_info_offset != NULL) {
                                float& enemy_hp = *(float*)(enemy_base + damage_info_offset + 0x18);
                                float& enemy_max_hp = *(float*)(enemy_base + damage_info_offset + 0x1C);
                                enemy_hp = enemy_max_hp;
                            }
                        }
                    }
                    uintptr_t* boss_ptr = (uintptr_t*)(s_mediator + 0xB0);
                    uintptr_t boss_base = *boss_ptr;
                    if (boss_base) {
                        float& boss_hp = *(float*)(boss_base + 0x151C);
                        float& boss_max_hp = *(float*)(boss_base + 0x1520);
                        boss_hp = boss_max_hp;
                    }
                    if (reset_timer) {
                        if (grounded == 1 && desired_input & 0x10 && desired_input & 0x08) {
                            float& bp_timer = *(float*)(s_mediator + 0x250);
                            bp_timer = two_minutes_timer;
                        }
                    }
                }
            }
        }
    }
}

void RestoreMaxHp::on_gui_frame() {
    ImGui::Checkbox("Restore Enemy HP", &mod_enabled);
    ImGui::SameLine();
    help_marker("Press Lock On + Taunt to restore Max HP to enemies");
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox("Reset Timer", &reset_timer);
    ImGui::SameLine();
    help_marker("Press Lock On + Taunt to reset the BP timer");
    if (mod_enabled || reset_timer) {
        ImGui::Checkbox("Disable Aerial Resets", &limit_to_ground);
        ImGui::SameLine();
        help_marker("Disable \"Restore Enemy HP\" and \"Reset Timer\" while aerial (Useful for Taunt Ecstasy)");
    }
}

void RestoreMaxHp::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("restore_max_hp").value_or(false);
    reset_timer = cfg.get<bool>("reset_bp_timer").value_or(false);
    limit_to_ground = cfg.get<bool>("reset_hp_limit_to_ground").value_or(false);
}

void RestoreMaxHp::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("restore_max_hp", mod_enabled);
    cfg.set<bool>("reset_bp_timer", reset_timer);
    cfg.set<bool>("reset_hp_limit_to_ground", limit_to_ground);
}
