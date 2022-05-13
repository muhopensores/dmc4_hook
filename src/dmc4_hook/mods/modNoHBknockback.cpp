#include "../mods.h"
#include "modNoHBknockback.hpp"
#include "modMoveIDs.hpp"
#include "modMoveIDsNero.hpp"
#include "modNeroFullHouse.hpp"

bool      NoHbKnockback::modEnabled{ false };
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackContinue{ NULL }; // 0x0051C389
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackJE{ 0x0051C367 };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

naked void noHelmBreakerKnockback_proc(void) { // ebx+0x98 = player + CE20 // ebx+0xA4 = damage id stuff (e.g. RED-Split_00)
	_asm {
			cmp byte ptr [NoHbKnockback::modEnabled], 1
			je cheatcode
			cmp byte ptr [NeroFullHouse::modEnabled], 1
			je nerocheatcode2
		originalcode:
			cmp ecx,0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]

		cheatcode:
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
			cmp [MoveIdsNero::moveIDNero], 814 // Double Down 3 exceed
			je newcode
			jmp originalcode

		nerocheatcode2:
			cmp [MoveIdsNero::moveIDNero], 812
			jne originalcode

			push eax
			mov eax, [staticMediatorPtr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp [eax+0x1564], 28 // check streak 1 was pushed to get this moveid
			pop eax
			je newcode
			jmp originalcode

		newcode:
			cmp ecx,0x05
			je nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]

		nohelmbreakerknockbackje:
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackJE]
	}
}

std::optional<std::string> NoHbKnockback::onInitialize() {
	if (!install_hook_offset(0x11C384, hook, &noHelmBreakerKnockback_proc, &_noHelmBreakerKnockbackContinue, 5)) {
		HL_LOG_ERR("Failed to init NoHelmBreakerKnockback mod\n");
		return "Failed to init NoHelmBreakerKnockback mod";
	}

	return Mod::onInitialize();
}

void NoHbKnockback::onGUIframe() {
	ImGui::Checkbox("No Helm Breaker Knockdown", &modEnabled);
}

void NoHbKnockback::onConfigLoad(const utils::Config& cfg) {
	modEnabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
}

void NoHbKnockback::onConfigSave(utils::Config& cfg) {
	cfg.set<bool>("no_helmbreaker_knockback", modEnabled);
}
