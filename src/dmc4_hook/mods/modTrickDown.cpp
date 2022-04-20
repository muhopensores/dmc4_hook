#include "../mods.h"
#include "modTrickDown.hpp"
#include "modTimerMem.hpp"

bool TrickDown::modEnabled{ false };
uintptr_t TrickDown::trick_down_jmp_ret{ NULL };
uintptr_t TrickDown::floor_touch_jmp_ret{ NULL };

float downFloat{ -200.0f };

naked void trickDownDetour(void) { // not gonna player compare because the idea of boss dante using down trick on you is kinda funny
	_asm {
			cmp byte ptr [TrickDown::modEnabled], 0
			je originalcode

			cmp dword ptr [TimerMem::timerMem], 0x42480000 // 50.0f // Compare timer mem to 50. It is reset by the backforward input. If lower, do down trick
			jl downtrickstart
			jmp originalcode

		downtrickstart:
			movss xmm2, [downFloat]               // Puts -200 in y axis momentum

		originalcode:
			movss [esi+0x00000EC4], xmm2
			jmp dword ptr [TrickDown::trick_down_jmp_ret]
	}
}

naked void floorTouchDetour(void) {
	_asm {
			cmp byte ptr [TrickDown::modEnabled], 0
			je originalcode

			cmp dword ptr [TimerMem::timerMem], 0x42A00000 // 80.0f
			jl retcode

		originalcode:
			movss [esi+0x00000EC4], xmm2
		retcode:
			jmp dword ptr [TrickDown::floor_touch_jmp_ret]
	}
}

std::optional<std::string> TrickDown::onInitialize() {
	if (!install_hook_offset(0x003CB119, trickDownHook, &trickDownDetour, &trick_down_jmp_ret, 8)) {
		HL_LOG_ERR("Failed to init TrickDown1 mod\n");
		return "Failed to init TrickDown1 mod";
	}
    if (!install_hook_offset(0x003CB33D, floorTouchHook, &floorTouchDetour, &floor_touch_jmp_ret, 8)) {
        HL_LOG_ERR("Failed to init TrickDown2 mod\n");
        return "Failed to init TrickDown2 mod";
    }

	return Mod::onInitialize();
}

void TrickDown::onGUIframe() {
    ImGui::Checkbox("Trick Down", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Map Trick Down to backforward + trick");
}

void TrickDown::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("trick_down").value_or(false);
}

void TrickDown::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("trick_down", modEnabled);
}
