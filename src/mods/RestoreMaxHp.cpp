
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
    uPlayer* player = devil4_sdk::get_local_player();
    SMediator* s_med_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
    if (player) {
        if (mod_enabled) {
            if (player->grounded == 1) {
                goto reset_hp_and_timer;
            }
            if (!limit_to_ground) {
                reset_hp_and_timer:
                if (player->inputHold[0] & 0x10 && player->inputHold[0] & 0x08) {
                    for (uint32_t i = 1; i <= s_med_ptr->enemyCount[2]; i++) {
                        uintptr_t enemy_base = (uintptr_t)s_med_ptr->uEnemies[i-1];
                        if (enemy_base) {
                            int damage_info_offset = get_enemy_specific_damage_offset(s_med_ptr->uEnemies[i-1]->ID);
                            if (damage_info_offset != NULL) {
                                float& enemy_hp     = *(float*)(enemy_base + damage_info_offset + 0x18);
                                float& enemy_max_hp = *(float*)(enemy_base + damage_info_offset + 0x1C);
                                enemy_hp            = enemy_max_hp;
                            }
                        }
                    }
                    if (s_med_ptr->uBoss1) {
                        s_med_ptr->uBoss1->HP = s_med_ptr->uBoss1->HPMax;
                    }
                    if (reset_timer) {
                        if (player->grounded == 1 && player->inputHold[0] & 0x10 && player->inputHold[0] & 0x08) {
                            s_med_ptr->bpTimer = two_minutes_timer;
                        }
                    }
                }
            }
        }
    }
}
void RestoreMaxHp::on_gui_frame() {
    ImGui::Checkbox(_("Restore Enemy HP", &mod_enabled));
    ImGui::SameLine();
    help_marker(_("Press Lock On + Taunt to restore Max HP to enemies"));
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Reset Timer"), &reset_timer);
    ImGui::SameLine();
    help_marker(_("Press Lock On + Taunt to reset the BP timer"));
    if (mod_enabled || reset_timer) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox("Disable Aerial Resets", &limit_to_ground);
        ImGui::SameLine();
        help_marker(_("Disable \"Restore Enemy HP\" and \"Reset Timer\" while aerial (Useful for Taunt Ecstasy)"));
        ImGui::Unindent(lineIndent);
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
