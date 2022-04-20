#include "../mods.h"
#include "modHpInOrbsDisplay.hpp"
#include "modDamageMultiplier.hpp"

bool HpInOrbsDisplay::modEnabled{ false };
uintptr_t HpInOrbsDisplay::jmp_ret{ NULL };
float xmm0backup{ NULL };

naked void detour(void) {
    _asm {
			cmp byte ptr [HpInOrbsDisplay::modEnabled], 0
			je originalcode

            // cmp dword ptr [enemyHPDisplay],0x00000000		// Check to see if enemy is dead
            // checking to see if enemyHP >= 0
            // this clobbers xmm0 register but it does
            // not seem to affect the game? not sure.
            movss [xmm0backup], xmm0
			xorps xmm0, xmm0
			comiss xmm0, [DamageMultiplier::enemyHPDisplay]
			jae originalcode // If yes, show default Orb Count
            movss xmm0, [xmm0backup]
			cvttss2si eax, [DamageMultiplier::enemyHPDisplay] // If no, write Enemy HP Display to orbs rather than Orb Count	// cvttss2si
			jmp dword ptr [HpInOrbsDisplay::jmp_ret]

		originalcode:
			mov eax, [eax+0x00000114]
			jmp dword ptr [HpInOrbsDisplay::jmp_ret]
    }
}

std::optional<std::string> HpInOrbsDisplay::onInitialize() {
    if (!install_hook_offset(0x00FDD35, hook, &detour, &jmp_ret, 6)) {
        HL_LOG_ERR("Failed to init HpInOrbsDisplay mod\n");
        return "Failed to init HpInOrbsDisplay mod";
    }
    return Mod::onInitialize();
}

void HpInOrbsDisplay::onGUIframe() {
    ImGui::Checkbox("Enemy HP Red Orb Display", &modEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("Display the last hit enemy's HP in the Red Orb count");
}

void HpInOrbsDisplay::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("enemy_hp_red_orb_display").value_or(false);
}

void HpInOrbsDisplay::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("enemy_hp_red_orb_display", modEnabled);
}
