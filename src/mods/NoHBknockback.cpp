#include "NoHBknockback.hpp"

bool NoHbKnockback::nero_toggle = false;
bool NoHbKnockback::dante_toggle = false;

uintptr_t NoHbKnockback::no_helm_breaker_knockback_continue = NULL; // 0x0051C389
uintptr_t NoHbKnockback::no_helm_breaker_knockback_je = 0x0051C367;
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void no_helm_breaker_knockback_proc(void) { // ebx+0x98 = player + CE20 // ebx+0xA4 = damage id stuff (e.g. RED-Split_00)
	_asm {
			cmp byte ptr [NoHbKnockback::nero_toggle], 1
			je NeroCode
			cmp byte ptr [NoHbKnockback::dante_toggle], 1
			je DanteCode
			jmp originalcode

		DanteCode:
			// load eax with player 1
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			test eax,eax
			je poporiginalcode
			mov eax, [eax+0x24]
			test eax,eax
			je poporiginalcode
			cmp [eax+0x1494], 0 // controller id dante
			jne poporiginalcode
			cmp dword ptr [eax+0x2998], 0x20A // 522 // Low
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 0x213 // 531 // Mid
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 0x214 // 532 // High
			je helmbreakeractive
			jmp poporiginalcode

		NeroCode:
			// load eax with player 1
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			test eax,eax
			je poporiginalcode
			mov eax, [eax+0x24]
			test eax,eax
			je poporiginalcode
			cmp [eax+0x1494], 1 // controller id nero
			jne poporiginalcode
			cmp dword ptr [eax+0x2998], 786 // Split
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 812 // Double Down
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 814 // Double Down 3 exceed
			je helmbreakeractive
			jmp poporiginalcode

		helmbreakeractive:
			cmp byte ptr [eax+0x1504],4 // move part, make the grounded part still knock back
			jl AffectKnockback
			jmp poporiginalcode

		poporiginalcode:
			pop eax
		originalcode:
			cmp ecx, 0x05
			jl nohelmbreakerknockbackje
		contcode:
			jmp dword ptr [NoHbKnockback::no_helm_breaker_knockback_continue]

		AffectKnockback:
			pop eax
			cmp ecx, 0x05
			je nohelmbreakerknockbackje
			jmp contcode

		nohelmbreakerknockbackje:
			jmp dword ptr [NoHbKnockback::no_helm_breaker_knockback_je]
	}
}

std::optional<std::string> NoHbKnockback::on_initialize() {
	if (!install_hook_offset(0x11C384, hook, &no_helm_breaker_knockback_proc, &no_helm_breaker_knockback_continue, 5)) {
		spdlog::error("Failed to init NoHelmBreakerKnockback mod\n");
		return "Failed to init NoHelmBreakerKnockback mod";
	}

	return Mod::on_initialize();
}

void NoHbKnockback::on_gui_frame(int display) {
	if (display == 1) {
		ImGui::Checkbox(_("No Helm Splitter Knockdown"), &nero_toggle);
		ImGui::SameLine();
		help_marker(_("Helm Splitter deals no knockdown and instead only stuns the enemy"));
	}
	else {
		ImGui::Checkbox(_("No Helm Breaker Knockdown"), &dante_toggle);
		ImGui::SameLine();
		help_marker(_("Helm Breaker deals no knockdown and instead only stuns the enemy"));
	}
}

void NoHbKnockback::on_config_load(const utility::Config& cfg) {
	nero_toggle = cfg.get<bool>("no_helmsplitter_knockdown").value_or(false);
	dante_toggle = cfg.get<bool>("no_helmbreaker_knockdown").value_or(false);
}

void NoHbKnockback::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("no_helmsplitter_knockdown", nero_toggle);
	cfg.set<bool>("no_helmbreaker_knockdown", dante_toggle);
}
