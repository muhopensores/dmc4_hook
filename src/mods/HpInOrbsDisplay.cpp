#include "HpInOrbsDisplay.hpp"
#include "HealthSettings.hpp"

bool HpInOrbsDisplay::mod_enabled = false;
uintptr_t HpInOrbsDisplay::jmp_ret = NULL;
naked void detour(void) {
    _asm {
			cmp byte ptr [HpInOrbsDisplay::mod_enabled], 0
			je originalcode

            // cmp dword ptr [enemyHPDisplay], 0x00000000		// Check to see if enemy is dead
            // checking to see if enemyHP >= 0
            sub esp, 4
            movss [esp], xmm0
			xorps xmm0, xmm0
			comiss xmm0, [HealthSettings::enemy_hp_display]
			jae poporiginal // If yes, show default Orb Count
            movss xmm0, [esp]
            add esp, 4
			cvttss2si eax, [HealthSettings::enemy_hp_display] // If no, write Enemy HP Display to orbs rather than Orb Count	// cvttss2si
			jmp dword ptr [HpInOrbsDisplay::jmp_ret]

        poporiginal:
            movss xmm0, [esp]
            add esp, 4
		originalcode:
			mov eax, [eax+0x00000114]
			jmp dword ptr [HpInOrbsDisplay::jmp_ret]
    }
}

std::optional<std::string> HpInOrbsDisplay::on_initialize() {
    if (!install_hook_offset(0x00FDD35, hook, &detour, &jmp_ret, 6)) {
        spdlog::error("Failed to init HpInOrbsDisplay mod\n");
        return "Failed to init HpInOrbsDisplay mod";
    }
    return Mod::on_initialize();
}

void HpInOrbsDisplay::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Enemy HP Red Orb Display"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Display the last hit enemy's HP in the Red Orb count"));
    }
}

void HpInOrbsDisplay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_hp_red_orb_display").value_or(false);
}

void HpInOrbsDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_hp_red_orb_display", mod_enabled);
}
