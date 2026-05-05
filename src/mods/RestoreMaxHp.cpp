#include "RestoreMaxHp.hpp"
#include "EnemyTracker.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sMediator.hpp"

bool RestoreMaxHp::mod_enabled = false;

static const float two_minutes_timer = 7200.0f;
static bool reset_timer = false;
static bool limit_to_ground = false;
static bool limit_to_air = false;

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

void RestoreMaxHp::restore_health_and_timer(sMediator* s_med_ptr, uPlayer* player) {
    uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
    while (enemy) {
        uintptr_t enemy_base = (uintptr_t)(enemy);
        if (enemy_base) {
            int damage_info_offset = EnemyTracker::get_enemy_specific_damage_offset(enemy->ID);
            if (damage_info_offset != 0) {
                float& enemy_hp = *(float*)(enemy_base + damage_info_offset + 0x18);
                float& enemy_max_hp = *(float*)(enemy_base + damage_info_offset + 0x1C);
                enemy_hp = enemy_max_hp;
            }
        }
        enemy = enemy->nextEnemy;
    }
    if (reset_timer && player->grounded == 1 && (player->inputHold[0] & 0x10) && (player->inputHold[0] & 0x08)) {
        s_med_ptr->bpTimer = two_minutes_timer;
    }
}

void RestoreMaxHp::on_frame(fmilliseconds& dt) {
    uPlayer* player = devil4_sdk::get_local_player();
    sMediator* s_med_ptr = reinterpret_cast<sMediator*>(*reinterpret_cast<uintptr_t*>(static_mediator_ptr));
    if (!player) {
        return;
    }
    bool is_grounded = (player->grounded == 1);
    if (reset_timer && is_grounded && (player->inputHold[0] & 0x10) && (player->inputHold[0] & 0x08)) {
        s_med_ptr->bpTimer = two_minutes_timer;
    }
    if (!mod_enabled) {
        return;
    }
    if ((limit_to_ground && !is_grounded) || (limit_to_air && is_grounded)) {
        return;
    }
    if (!(player->inputHold[0] & 0x10) || !(player->inputHold[0] & 0x08)) {
        return;
    }
    if (!limit_to_ground && !limit_to_air ||
        (limit_to_ground && is_grounded) ||
        (limit_to_air && !is_grounded)) {
        restore_health_and_timer(s_med_ptr, player);
        return;
    }
    restore_health_and_timer(s_med_ptr, player);
}

void RestoreMaxHp::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Reset Enemy HP"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Press Lock On + Taunt to restore Max HP to enemies"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Reset Timer"), &reset_timer);
        ImGui::SameLine();
        help_marker(_("Press Lock On + Taunt to reset the BP timer"));
        if (mod_enabled || reset_timer) {
            ImGui::Indent(lineIndent);
            if (ImGui::Checkbox(_("Disable Aerial Resets"), &limit_to_ground)) {
                if (limit_to_ground)
                    limit_to_air = false;
            }
            ImGui::SameLine();
            help_marker(_("Disable \"Reset Enemy HP\" and \"Reset Timer\" while aerial (Useful for Taunt Ecstasy)"));
            if (ImGui::Checkbox(_("Disable Grounded Resets"), &limit_to_air)) {
                if (limit_to_air)
                    limit_to_ground = false;
            }
            ImGui::Unindent(lineIndent);
        }
    }
}

void RestoreMaxHp::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("restore_max_hp").value_or(false);
    reset_timer = cfg.get<bool>("reset_bp_timer").value_or(false);
    limit_to_ground = cfg.get<bool>("reset_hp_limit_to_ground").value_or(false);
    limit_to_air = cfg.get<bool>("reset_hp_limit_to_air").value_or(false);
}

void RestoreMaxHp::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("restore_max_hp", mod_enabled);
    cfg.set<bool>("reset_bp_timer", reset_timer);
    cfg.set<bool>("reset_hp_limit_to_ground", limit_to_ground);
    cfg.set<bool>("reset_hp_limit_to_air", limit_to_air);
}
