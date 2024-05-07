
#include "DamageMultiplier.hpp"
#include "../sdk/Devil4.hpp"

#include "glm/gtx/compatibility.hpp"

bool DamageMultiplier::mod_enabled{ false };
static bool g_must_style{ false };
uintptr_t DamageMultiplier::jmp_ret{ NULL };
float DamageMultiplier::enemy_hp_display{ NULL };
static float damagemultiplier{ 1.0f };
static float xmm4backup{ NULL };

static float get_current_style_rank() {

	SStylishCount* sc = devil4_sdk::get_stylish_count();
	if (!sc) { return 0.0f; }

	uint32_t rank = sc->current_style_tier;
	float normalized_rank = glm::smoothstep(0.0f, 7.0f, (float)rank);

	return normalized_rank;
}

// shameless copypaste from 
// https://www.iquilezles.org/www/articles/functions/functions.htm
// Let m be the threshold (anything above m stays unchanged),
// and n the value things will take when your input is zero.
// Then, the following function does the soft clipping (in a cubic fashion):
static float almost_identity( float x, float m, float n )
{
	if( x>m ) return x;
	const float a = 2.0f*n - m;
	const float b = 2.0f*m - 3.0f*n;
	const float t = x/m;
	return (a*t + b)*t*t + n;
}

static void must_style_multiplier() {
    if (!g_must_style) { return; }
	damagemultiplier = almost_identity(get_current_style_rank(), 0.3f, 0.3f);
}

#include <xmmintrin.h> // simd microsoft specific thingies

// should be zero initialized due to static init
static __m128 xxmm0;
static __m128 xxmm1;
static __m128 xxmm2;
static __m128 xxmm3;
static __m128 xxmm4;

naked void detour(void) {
	_asm {
			cmp byte ptr [DamageMultiplier::mod_enabled], 0
			je originalcode

			cmp dword ptr [esi+0x1C], 0x469C4000 // ignore if player
			je originalcode
			//movss dword ptr [ebp-4],xmm4		 // Maybe this was the reason m17 was crashing? testing now
			movss [xmm4backup], xmm4

			movss xmm4, [esi+0x18]				 // get the current life

			subss xmm4, xmm0					 // subtract the current life to the new life ( = currentHitDamage)
            movss xmm0, [esi+0x18]				 // set new life to old life

            movdqa [xxmm0], xmm0                 // simd i
            movdqa [xxmm1], xmm1
            movdqa [xxmm2], xmm2
            movdqa [xxmm3], xmm3
            movdqa [xxmm4], xmm4
            push eax                             // save reg
            call must_style_multiplier           // moved must style update here for micro-optimizations because
                                                 // DamageMultiplier::OnFrame() showed up on luke stackwalker once
            pop eax                              // restore reg
            movdqa xmm0, [xxmm0]                 // simd o
            movdqa xmm1, [xxmm1]   
            movdqa xmm2, [xxmm2]   
            movdqa xmm3, [xxmm3]   
            movdqa xmm4, [xxmm4]   
			mulss xmm4, [damagemultiplier]		 // multiply by "multiplier" the currentHitDamage (xmm2)
            subss xmm0, xmm4					 // subss to new life current hit dammage

			movss xmm4, [xmm4backup]
			//movss xmm4, dword ptr [ebp-4]

		originalcode:
            movss [esi+0x18], xmm0
			movss [DamageMultiplier::enemy_hp_display], xmm0 // writes to an address we'll use for orb display. In originalcode so its not dependent on this checkbox
			comiss xmm2, xmm0					             // lost in cmps
			jmp dword ptr [DamageMultiplier::jmp_ret]
	}
}

std::optional<std::string> DamageMultiplier::on_initialize() {
	if (!install_hook_offset(0x0011BFE5, hook, &detour, &jmp_ret, 5)) { // "F3 0F 11 46 18 76 ?? F3 0F 58 ??"
		spdlog::error("Failed to init DamageMultiplier mod\n");
		return "Failed to init DamageMultiplier mod";
	}

    console->system().RegisterCommand("damagemult", "Set outgoing damage multiplier", 
        [](float value) {
            g_must_style = false;
            DamageMultiplier::mod_enabled = true;
            damagemultiplier = value;
        }, 
        csys::Arg<float>("1.0 default"));    

	return Mod::on_initialize();
}

void DamageMultiplier::on_gui_frame() {
    if (ImGui::Checkbox(_("Player Damage Multiplier"), &mod_enabled)) {
        if (!mod_enabled) g_must_style = false;
    }
    ImGui::SameLine();
    help_marker(_("Add a multiplier to outgoing damage"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Must style mode"), &g_must_style)) {
        mod_enabled = true;
    }
    ImGui::SameLine();
    help_marker(_("Damage scales with Style\n0 damage at no rank to 1.0 damage at SSS\nDue to the way this is coded, it cannot currently be used with Damage Multiplier"));

    if (mod_enabled && !g_must_style) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::InputFloat(_("Multiplier"), &damagemultiplier, 0.1f, 1.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        help_marker(_("Less than 1 = you deal less damage than default\nMore than 1 = you deal more"));
        ImGui::Unindent(lineIndent);
    }
}

/*void DamageMultiplier::onFrame(fmilliseconds & dt) {
	if (!g_mustStyle) { return; }
	mustStyleMultiplier();
}*/

void DamageMultiplier::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("damage_multiplier").value_or(false);
    damagemultiplier = cfg.get<float>("damage_multiplier_float").value_or(1.0f);
    g_must_style = cfg.get<bool>("must_style").value_or(false);
}

void DamageMultiplier::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("damage_multiplier", mod_enabled);
    cfg.set<float>("damage_multiplier_float", damagemultiplier);
    cfg.set<bool>("must_style", g_must_style);
}
