#include "NoHBknockback.hpp"
#include "NeroFullHouse.hpp"

bool      NoHbKnockback::mod_enabled{ false };
uintptr_t NoHbKnockback::no_helm_breaker_knockback_continue{ NULL }; // 0x0051C389
uintptr_t NoHbKnockback::no_helm_breaker_knockback_je{ 0x0051C367 };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void no_helm_breaker_knockback_proc(void) { // ebx+0x98 = player + CE20 // ebx+0xA4 = damage id stuff (e.g. RED-Split_00)
	_asm {
			cmp byte ptr [NoHbKnockback::mod_enabled], 1
			je helmbreakercode
			cmp byte ptr [NeroFullHouse::mod_enabled], 1
			je paylinecode
			jmp originalcode

		poporiginalcode:
			pop eax
		originalcode:
			cmp ecx, 0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::no_helm_breaker_knockback_continue]

		helmbreakercode:
			// load eax with player 1
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]

			// Dante:
			cmp dword ptr [eax+0x2998], 0x20A // 522 // Low
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 0x213 // 531 // Mid
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 0x214 // 532 // High
			je helmbreakeractive
			// Nero:
			cmp dword ptr [eax+0x2998], 786 // Split
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 812 // Double Down
			je helmbreakeractive
			cmp dword ptr [eax+0x2998], 814 // Double Down 3 exceed
			je helmbreakeractive
			jmp poporiginalcode

		helmbreakeractive:
			cmp byte ptr [eax+0x1504],4 // move part, make the grounded part still knock back
			jl newcode
			jmp poporiginalcode

		paylinecode:
			// load eax with player 1
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]

			cmp dword ptr [eax+0x2998], 812
			jne poporiginalcode
			cmp dword ptr [eax+0x1564], 28    // check streak 1 was pushed to get this moveid
			je newcode
			jmp poporiginalcode

		newcode:
			pop eax
			cmp ecx, 0x05
			je nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::no_helm_breaker_knockback_continue]

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

void NoHbKnockback::on_gui_frame() {
	ImGui::Checkbox(_("No Helm Breaker Knockdown"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Helm Breaker + Helm Splitter deal no knockback or knockdown and instead only stun the enemy"));
}

void NoHbKnockback::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
}

void NoHbKnockback::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("no_helmbreaker_knockback", mod_enabled);
}
