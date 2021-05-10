#include "../mods.h"
#include "modDamageMultiplier.hpp"

#include "glm/gtx/compatibility.hpp"

bool DamageMultiplier::modEnabled{ false };
bool g_mustStyle{ false };
uintptr_t DamageMultiplier::jmp_ret{ NULL };
float DamageMultiplier::enemyHPDisplay{ NULL };
float damagemultiplier = 1.0f;
float xmm4backup{ NULL };

static float getCurrentStyleRank() {
	constexpr uintptr_t sStylishCountPtr = 0x00E558CC;
	sStylishCount* sc = (sStylishCount*)*(uintptr_t*)sStylishCountPtr;
	if (!sc) { return 0.0f; }
	uint32_t rank = sc->currentStyleTier;
	float normalizedRank = glm::smoothstep(0.0f, 7.0f, (float)rank);
	return normalizedRank;
}

// shameless copypaste from 
// https://www.iquilezles.org/www/articles/functions/functions.htm
// Let m be the threshold (anything above m stays unchanged),
// and n the value things will take when your input is zero.
// Then, the following function does the soft clipping (in a cubic fashion):
static float almostIdentity( float x, float m, float n )
{
	if( x>m ) return x;
	const float a = 2.0f*n - m;
	const float b = 2.0f*m - 3.0f*n;
	const float t = x/m;
	return (a*t + b)*t*t + n;
}

static void mustStyleMultiplier() {
	damagemultiplier = almostIdentity(getCurrentStyleRank(), 0.3f, 0.3f);
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
	if (modEnabled) {
		ImGui::Checkbox("Must style mode", &g_mustStyle);
	}
}

void DamageMultiplier::onFrame(fmilliseconds & dt) {
	if (!g_mustStyle) { return; }
	mustStyleMultiplier();
}

void DamageMultiplier::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("damage_multiplier").value_or(false);
    damagemultiplier = cfg.get<float>("damage_multiplier_float").value_or(1.0f);
}

void DamageMultiplier::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("damage_multiplier", modEnabled);
    cfg.set < float > ("damage_multiplier_float", damagemultiplier);
}
