
#include "HpInOrbsDisplay.hpp"
#include "DamageMultiplier.hpp"

bool HpInOrbsDisplay::mod_enabled{ false };
uintptr_t HpInOrbsDisplay::jmp_ret{ NULL };
static float xmm0backup{ NULL };

naked void detour(void) {
    _asm {
			cmp byte ptr [HpInOrbsDisplay::mod_enabled], 0
			je originalcode

            // cmp dword ptr [enemyHPDisplay], 0x00000000		// Check to see if enemy is dead
            // checking to see if enemyHP >= 0
            // this clobbers xmm0 register but it does
            // not seem to affect the game? not sure.
            movss [xmm0backup], xmm0
			xorps xmm0, xmm0
			comiss xmm0, [DamageMultiplier::enemy_hp_display]
			jae originalcode // If yes, show default Orb Count
            movss xmm0, [xmm0backup]
			cvttss2si eax, [DamageMultiplier::enemy_hp_display] // If no, write Enemy HP Display to orbs rather than Orb Count	// cvttss2si
			jmp dword ptr [HpInOrbsDisplay::jmp_ret]

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

void HpInOrbsDisplay::on_gui_frame() {
    ImGui::Checkbox("Enemy HP Red Orb Display", &mod_enabled);
    ImGui::SameLine();
    help_marker("Display the last hit enemy's HP in the Red Orb count");
}

void HpInOrbsDisplay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_hp_red_orb_display").value_or(false);
}

void HpInOrbsDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_hp_red_orb_display", mod_enabled);
}
