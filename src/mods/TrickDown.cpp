
#include "TrickDown.hpp"
#include "TimerMem.hpp"

bool TrickDown::mod_enabled{ false };
uintptr_t TrickDown::trick_down_jmp_ret{ NULL };
uintptr_t TrickDown::floor_touch_jmp_ret{ NULL };

static float down_float{ -200.0f };

naked void trick_down_detour(void) { // not gonna player compare because the idea of boss dante using down trick on you is kinda funny
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp dword ptr [TimerMem::timer_mem], 0x42480000 // 50.0f // Compare timer mem to 50. It is reset by the backforward input. If lower, do down trick
			jl downtrickstart
			jmp originalcode

		downtrickstart:
			movss xmm2, [down_float]               // Puts -200 in y axis momentum

		originalcode:
			movss [esi+0x00000EC4], xmm2
			jmp dword ptr [TrickDown::trick_down_jmp_ret]
	}
}

naked void floor_touch_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp dword ptr [TimerMem::timer_mem], 0x42A00000 // 80.0f
			jl retcode

		originalcode:
			movss [esi+0x00000EC4], xmm2
		retcode:
			jmp dword ptr [TrickDown::floor_touch_jmp_ret]
	}
}

std::optional<std::string> TrickDown::on_initialize() {
	if (!install_hook_offset(0x003CB119, trick_down_hook, &trick_down_detour, &trick_down_jmp_ret, 8)) {
		spdlog::error("Failed to init TrickDown1 mod\n");
		return "Failed to init TrickDown1 mod";
	}
    if (!install_hook_offset(0x003CB33D, floor_touch_hook, &floor_touch_detour, &floor_touch_jmp_ret, 8)) {
        spdlog::error("Failed to init TrickDown2 mod\n");
        return "Failed to init TrickDown2 mod";
    }

	return Mod::on_initialize();
}

void TrickDown::on_gui_frame() {
    ImGui::Checkbox("Trick Down", &mod_enabled);
    ImGui::SameLine();
    help_marker("Map Trick Down to backforward + trick\nIf an enemy is directly above Dante, it may register your forward input as a back "
                "input and so queue a trick down");
}

void TrickDown::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("trick_down").value_or(false);
}

void TrickDown::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("trick_down", mod_enabled);
}
