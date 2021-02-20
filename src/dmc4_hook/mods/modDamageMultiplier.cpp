#include "../mods.h"
#include "modDamageMultiplier.hpp"

bool DamageMultiplier::modEnabled{ false };
uintptr_t DamageMultiplier::jmp_ret{ NULL };
float DamageMultiplier::enemyHPDisplay{ NULL };
float damagemultiplier = 1.0f;
float xmm4backup{ NULL };

DamageMultiplier::DamageMultiplier()
{
	//onInitialize();
}

naked void detour(void)
{
	_asm {
			cmp byte ptr [DamageMultiplier::modEnabled], 0
			je originalcode

			cmp dword ptr [esi+0x1C], 0x469C4000 // ignore if player
			je originalcode

			//movss dword ptr [ebp-4],xmm4		 // Maybe this was the reason m17 was crashing? testing now
			movss [xmm4backup], xmm4

			movss xmm4, [esi+0x18]				 // get the current life
			subss xmm4, xmm0					 // subtract the current life to the new life ( = currentHitDamage)
            movss xmm0, [esi+0x18]				 // set new life to old life
            mulss xmm4, [damagemultiplier]		 // multiply by "multiplier" the currentHitDamage (xmm2)
            subss xmm0, xmm4					 // subss to new life current hit dammage

			movss xmm4, [xmm4backup]
			//movss xmm4, dword ptr [ebp-4]

		originalcode:
            movss [esi+0x18], xmm0				            // commented out because right now its going through code twice // @siyantodo
			movss [DamageMultiplier::enemyHPDisplay], xmm0  // writes to an address we'll use for orb display. In originalcode so its not dependent on this checkbox
			comiss xmm2, xmm0					            // lost in cmps
			jmp dword ptr [DamageMultiplier::jmp_ret]
	}
}

std::optional<std::string> DamageMultiplier::onInitialize() {

	if (!install_hook_offset(0x0011BFE5, hook, &detour, &jmp_ret, 5)) { // "F3 0F 11 46 18 76 ?? F3 0F 58 ??"
		HL_LOG_ERR("Failed to init DamageMultiplier mod\n");
		return "Failed to init DamageMultiplier mod";
	}
	return Mod::onInitialize();
}

void DamageMultiplier::onGUIframe() {
    ImGui::Checkbox("Player Damage Multiplier", &modEnabled);
    ImGui::PushItemWidth(217);
    ImGui::InputFloat("Multiplier", &damagemultiplier, 0.1f, 1.0f, "%.1f");
    ImGui::PopItemWidth();
    ImGui::Spacing();
}

void DamageMultiplier::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("damage_multiplier").value_or(false);
    damagemultiplier = cfg.get<float>("damage_multiplier_float").value_or(1.0f);
}

void DamageMultiplier::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("damage_multiplier", modEnabled);
    cfg.set < float > ("damage_multiplier_float", damagemultiplier);
}
