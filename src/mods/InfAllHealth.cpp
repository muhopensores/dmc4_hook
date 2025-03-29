#include "InfAllHealth.hpp"
#include "EnemySpawn.hpp"
#include "iostream"

#if 1
bool InfAllHealth::mod_enabled{false};
static bool player_invincible{false};
static bool enemy_invincible{false};

uintptr_t InfAllHealth::jmp_return{NULL};

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void detour() {
    _asm {
        cmp byte ptr [InfAllHealth::mod_enabled], 1
        je retcode

    // playercheck:
        push ecx
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        lea ecx, [ecx+0x15B4]
        cmp ecx, esi
        pop ecx
        jne enemycheck // not player
    // player:
        cmp byte ptr [player_invincible], 1
        je retcode
        jmp code

    enemycheck:
        cmp byte ptr [enemy_invincible], 1
        je retcode
        jmp code

    code:
        subss xmm0,xmm1
        // comiss xmm2,xmm0 // we have a copy of this in damage mult so don't need it here
    retcode:
        jmp dword ptr [InfAllHealth::jmp_return]
    }
}

std::optional<std::string> InfAllHealth::on_initialize() {
    if (!install_hook_offset(0x11BFD9, hook, &detour, &jmp_return, 7)) {
        spdlog::error("Failed to init InfAllHealth mod\n");
        return "Failed to init InfAllHealth mod";
    }

    utility::create_keyboard_hotkey(m_hotkeys, {VK_F1}, "Infinite Health (All)", "inf_hp_key");

    console->system().RegisterCommand("god", "Infinite player HP", [/*this*/]() {
        player_invincible = !player_invincible;
    });

    console->system().RegisterCommand("enemygod", "Infinite enemy HP", [/*this*/]() {
        enemy_invincible = !enemy_invincible;
    });

    return Mod::on_initialize();
}

void InfAllHealth::on_gui_frame() {
    if (ImGui::Checkbox(_("Infinite Health (All)"), &mod_enabled)) {
        if (mod_enabled) {
            player_invincible = true;
            enemy_invincible  = true;
        } else {
            player_invincible = false;
            enemy_invincible  = false;
        }
    }
    ImGui::SameLine();
    help_marker(_("Disables any entity's health decreasing. By default this is hotkeyed to F1"));
    ImGui::Indent(lineIndent);
    if (ImGui::Checkbox(_("Infinite Health (Player)"), &player_invincible)) {
        if (player_invincible && enemy_invincible)
            mod_enabled = true;
        else
            mod_enabled = false;
    }
    ImGui::SameLine();
    help_marker(_("Disables the player's health decreasing"));

    if (ImGui::Checkbox(_("Infinite Health (Enemy)"), &enemy_invincible)) {
        if (player_invincible && enemy_invincible)
            mod_enabled = true;
        else
            mod_enabled = false;
    }
    ImGui::SameLine();
    help_marker(_("Disables enemy's health decreasing"));
    ImGui::Unindent(lineIndent);
}

void InfAllHealth::on_config_load(const utility::Config& cfg) {
    mod_enabled       = cfg.get<bool>("infinite_health_all").value_or(false);
    player_invincible = cfg.get<bool>("infinite_health_player").value_or(false);
    enemy_invincible  = cfg.get<bool>("infinite_health_enemy").value_or(false);
    if (mod_enabled) {
        player_invincible = true;
        enemy_invincible  = true;
    }
};

void InfAllHealth::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_health_all", mod_enabled);
    cfg.set<bool>("infinite_health_player", player_invincible);
    cfg.set<bool>("infinite_health_enemy", enemy_invincible);
};

void InfAllHealth::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        if (mod_enabled) {
            DISPLAY_MESSAGE("Infinite Health (All) Off");
            mod_enabled = false;
            player_invincible = false;
            enemy_invincible = false;
        }
        else {
            DISPLAY_MESSAGE("Infinite Health (All) On");
            mod_enabled = true;
            player_invincible = true;
            enemy_invincible = true;
        }
    }
}
#endif
