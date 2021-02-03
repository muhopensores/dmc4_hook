#include "../mods.h"
#include "modNoHBknockback.hpp"
#include "modMoveIDs.hpp"
#include "modMoveIDsNero.hpp"

bool      NoHbKnockback::modEnabled{ false };
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackContinue{ NULL }; // 0x0051C389
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackJE{ 0x0051C367 };

NoHbKnockback::NoHbKnockback() {
	//onInitialize();
}

naked void noHelmBreakerKnockback_proc(void)
{
	_asm {
			cmp byte ptr [NoHbKnockback::modEnabled],0
			je originalcode
			// Dante:
			cmp [MoveIds::moveID],0x20A			// 522 // Low
			je newcode
			cmp [MoveIds::moveID],0x213			// 531 // Mid
			je newcode
			cmp [MoveIds::moveID],0x214			// 532 // High
			je newcode

			// Nero:
			cmp [MoveIdsNero::moveIDNero], 786  // Split
			je newcode
			cmp [MoveIdsNero::moveIDNero], 812  // Double Down
			je newcode
			jmp originalcode

		newcode:
			cmp ecx,0x05
			je nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]

		nohelmbreakerknockbackje:
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackJE]

		originalcode:
			cmp ecx,0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]
	}
}

std::optional<std::string> NoHbKnockback::onInitialize() {

	if (!install_hook_offset(0x11C384, hook, &noHelmBreakerKnockback_proc, &_noHelmBreakerKnockbackContinue, 5))
    {
		HL_LOG_ERR("Failed to init NoHelmBreakerKnockback mod\n");
		return "Failed to init NoHelmBreakerKnockback mod";
	}

	return Mod::onInitialize();
}

void NoHbKnockback::onGUIframe() {
	// from main.cpp
	// line 1352 -> if (ImGui::Checkbox("No Helm Breaker Knockback", &checkNoHelmBreakerKnockback))
	ImGui::Checkbox("No Helm Breaker Knockback", &modEnabled);
}

void NoHbKnockback::onConfigLoad(const utils::Config& cfg) {
	modEnabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
};

void NoHbKnockback::onConfigSave(utils::Config& cfg) {
	cfg.set<bool>("no_helmbreaker_knockback", modEnabled);
};
