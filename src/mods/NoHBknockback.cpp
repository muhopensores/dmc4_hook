
#include "NoHBknockback.hpp"
#include "MoveIDs.hpp"
#include "MoveIDsNero.hpp"
#include "NeroFullHouse.hpp"

bool      NoHbKnockback::mod_enabled{ false };
uintptr_t NoHbKnockback::no_helm_breaker_knockback_continue{ NULL }; // 0x0051C389
uintptr_t NoHbKnockback::no_helm_breaker_knockback_je{ 0x0051C367 };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void no_helm_breaker_knockback_proc(void) { // ebx+0x98 = player + CE20 // ebx+0xA4 = damage id stuff (e.g. RED-Split_00)
	_asm {
			cmp byte ptr [NoHbKnockback::mod_enabled], 1
			je cheatcode
			cmp byte ptr [NeroFullHouse::mod_enabled], 1
			je nerocheatcode2
		originalcode:
			cmp ecx,0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::no_helm_breaker_knockback_continue]

		cheatcode:
			// Dante:
			cmp dword ptr [MoveIds::move_id],0x20A			// 522 // Low
			je newcode
			cmp dword ptr [MoveIds::move_id],0x213 // 531 // Mid
			je newcode
			cmp dword ptr [MoveIds::move_id],0x214 // 532 // High
			je newcode
			// Nero:
			cmp dword ptr [MoveIdsNero::move_id_nero], 786 // Split
			je newcode
			cmp dword ptr [MoveIdsNero::move_id_nero], 812 // Double Down
			je newcode
			cmp dword ptr [MoveIdsNero::move_id_nero], 814 // Double Down 3 exceed
			je newcode
			jmp originalcode

		nerocheatcode2:
			cmp dword ptr [MoveIdsNero::move_id_nero], 812
			jne originalcode

			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp dword ptr [eax+0x1564], 28    // check streak 1 was pushed to get this moveid
			pop eax
			je newcode
			jmp originalcode

		newcode:
			cmp ecx,0x05
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
	ImGui::Checkbox("No Helm Breaker Knockdown", &mod_enabled);
}

void NoHbKnockback::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
}

void NoHbKnockback::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("no_helmbreaker_knockback", mod_enabled);
}
