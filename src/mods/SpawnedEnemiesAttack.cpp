#include "SpawnedEnemiesAttack.hpp"
#include "Survival.hpp"

bool SpawnedEnemiesAttack::mod_enabled = false;
uintptr_t SpawnedEnemiesAttack::jmp_ret1 = NULL;
uintptr_t SpawnedEnemiesAttack::detour1_je = 0x55D0A9;

naked void detour1() {
    _asm {
            mov byte ptr [esi+0x00001DDB], 00
            cmp byte ptr [SpawnedEnemiesAttack::mod_enabled], 1
            je newcode
            cmp byte ptr [Survival::survival_active], 1
            je newcode

        // originalcode:
            jmp dword ptr [SpawnedEnemiesAttack::jmp_ret1]
        newcode:
            jmp dword ptr [SpawnedEnemiesAttack::detour1_je] // skips calling a func that checks if enemies have an rRouteNode

    }
}

// void on_frame(fmilliseconds& dt) {}

void SpawnedEnemiesAttack::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Spawned Scarecrows Attack"), &SpawnedEnemiesAttack::mod_enabled)) {
        if (!SpawnedEnemiesAttack::mod_enabled) {
            Survival::mod_enabled = SpawnedEnemiesAttack::mod_enabled;
        }
    }
    ImGui::SameLine();
    help_marker(_("This is a hack to make spawned scarecrows attack despite lacking an rRouteNode. I have not bug checked this"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> SpawnedEnemiesAttack::on_initialize() {
    if (!install_hook_offset(0x15D08F, SpawnedEnemiesAttack::hook1, &detour1, &SpawnedEnemiesAttack::jmp_ret1, 7)) {
        spdlog::error("Failed to init SpawnedEnemiesAttack mod 1\n");
        return "Failed to init SpawnedEnemiesAttack mod 1";
	}

    return Mod::on_initialize();
}

void SpawnedEnemiesAttack::on_config_load(const utility::Config& cfg){
    SpawnedEnemiesAttack::mod_enabled = cfg.get<bool>("SpawnedEnemiesAttack").value_or(false);
}

void SpawnedEnemiesAttack::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("SpawnedEnemiesAttack", SpawnedEnemiesAttack::mod_enabled);
}
