#include "modInfAllHealth.hpp"
#include "modEnemySpawn.hpp"
#include "../utils/MessageDisplay.hpp" // TODO(): DISPLAY_MESSAGE should probably be included in mod.hpp or something
#include "iostream"
#if 1
bool InfAllHealth::modEnabled{ false };
static bool playerInvincible{ false };
static bool enemyInvincible{ false };
int InfAllHealth::hotkey{ NULL };
uintptr_t InfAllHealth::jmp_return{ NULL };

constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

naked void detour() {
    _asm {
        cmp byte ptr [InfAllHealth::modEnabled], 1
        je retcode

    // playercheck:
        push ecx
        mov ecx, [staticMediatorPtr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        lea ecx, [ecx+0x15B4]
        cmp ecx, esi
        pop ecx
        jne enemycheck // not player
    // player:
        cmp byte ptr [playerInvincible], 1
        je retcode
        jmp code

    enemycheck:
        cmp byte ptr [enemyInvincible], 1
        je retcode
        jmp code

    code:
        subss xmm0,xmm1
        // comiss xmm2,xmm0 // we have a copy of this in damage mult so don't need it here
    retcode:
        jmp dword ptr [InfAllHealth::jmp_return]
    }
}

std::optional<std::string> InfAllHealth::onInitialize() {
    if (!install_hook_offset(0x11BFD9, hook, &detour, &jmp_return, 7)) {
        HL_LOG_ERR("Failed to init InfAllHealth mod\n");
        return "Failed to init InfAllHealth mod";
    }

    return Mod::onInitialize();
}

void InfAllHealth::onGUIframe() {
    if (ImGui::Checkbox("Infinite Health (All)", &modEnabled)) {
        if (modEnabled) {
            playerInvincible = true;
            enemyInvincible = true;
        }
        else {
            playerInvincible = false;
            enemyInvincible = false;
        }
    }

    if (ImGui::Checkbox("Infinite Health (Player)", &playerInvincible)) {
        if (playerInvincible && enemyInvincible)
            modEnabled = true;
        else
            modEnabled = false;
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Infinite Health (Enemy)", &enemyInvincible)) {
        if (playerInvincible && enemyInvincible)
            modEnabled = true;
        else
            modEnabled = false;
    }
}

void InfAllHealth::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("infinite_health_all").value_or(false);
    playerInvincible = cfg.get<bool>("infinite_health_player").value_or(false);
    enemyInvincible = cfg.get<bool>("infinite_health_enemy").value_or(false);
    hotkey = cfg.get<int>("inf_hp_hotkey").value_or(0x70);
    if (modEnabled) {
        playerInvincible = true;
        enemyInvincible = true;
    }
};

void InfAllHealth::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("infinite_health_all", modEnabled);
    cfg.set<bool>("infinite_health_player", playerInvincible);
    cfg.set<bool>("infinite_health_enemy", enemyInvincible);
    cfg.set<int>("inf_hp_hotkey", hotkey);
};

void InfAllHealth::onUpdateInput(hl::Input& input) {
    if (!input.isDown(EnemySpawn::hotkeySpawnModifier)) {
        if (input.wentDown(hotkey)) {
            if (modEnabled) {
                DISPLAY_MESSAGE("Infinite Health (All) Off");
                modEnabled = false;
                playerInvincible = false;
                enemyInvincible = false;
            }
            else {
                DISPLAY_MESSAGE("Infinite Health (All) On");
                modEnabled = true;
                playerInvincible = true;
                enemyInvincible = true;
            }
        }
    }
}
#endif