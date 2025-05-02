#include "TrickDown.hpp"
#include "TimerMem.hpp"

bool TrickDown::mod_enabled = false;
uintptr_t TrickDown::trick_down_jmp_ret = NULL;
uintptr_t TrickDown::floor_touch_jmp_ret = NULL;
uintptr_t TrickDown::landing_anim_jmp_ret = NULL;

static float down_float = -200.0f;
static float timerMemComparison = 20.0f; // Initial input + time it takes to get to trick, it reads 15ish
static float xmmBackup = 0.0f;
bool TrickDown::downFlag = false;

naked void trick_down_detour(void) { // not gonna player compare because the idea of boss dante using down trick on you is kinda funny
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			mov byte ptr [TrickDown::downFlag], 0

			movss [xmmBackup], xmm7
			movss xmm7, [TimerMem::timer_mem] // Compare timer mem. It is reset by the backforward input
			comiss xmm7, [timerMemComparison]
			movss xmm7, [xmmBackup]
			jb downtrickstart
			jmp originalcode

		downtrickstart:
			movss xmm2, [down_float] // Puts -200 in y axis momentum
			mov byte ptr [TrickDown::downFlag], 1
		originalcode:
			movss [esi+0x00000EC4], xmm2
			jmp dword ptr [TrickDown::trick_down_jmp_ret]
	}
}

naked void floor_touch_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp byte ptr [TrickDown::downFlag], 1
			jne originalcode
			movss xmm2, [down_float] // Puts -200 in y axis momentum

		originalcode:
			movss [esi+0x00000EC4], xmm2
		// retcode:
			jmp dword ptr [TrickDown::floor_touch_jmp_ret]
	}
}

naked void landing_anim_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp byte ptr [TrickDown::downFlag], 1
			jne originalcode
			push 0x01
			jmp cont

		originalcode:
			push 0x0B
		cont:
			mov ecx,esi
			call edx
		// retcode:
			jmp dword ptr [TrickDown::landing_anim_jmp_ret]
	}
}

std::optional<std::string> TrickDown::on_initialize() {
	if (!install_hook_offset(0x3CB119, trick_down_hook, &trick_down_detour, &trick_down_jmp_ret, 8)) {
		spdlog::error("Failed to init TrickDown1 mod\n");
		return "Failed to init TrickDown1 mod";
	}
    if (!install_hook_offset(0x3CB33D, floor_touch_hook, &floor_touch_detour, &floor_touch_jmp_ret, 8)) {
        spdlog::error("Failed to init TrickDown2 mod\n");
        return "Failed to init TrickDown2 mod";
    }
    if (!install_hook_offset(0x3CB38E, landing_anim_hook, &landing_anim_detour, &landing_anim_jmp_ret, 6)) {
        spdlog::error("Failed to init TrickDown mod 3\n");
        return "Failed to init TrickDown mod 3";
    }
	return Mod::on_initialize();
}

void TrickDown::on_gui_frame(int display) {
    ImGui::Checkbox(_("Down Trick"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Map Down Trick to backforward + trick\nIf an enemy is directly above Dante, it may register your forward input as a back "
                "input and so queue a down trick"));
}

void TrickDown::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("trick_down").value_or(false);
}

void TrickDown::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("trick_down", mod_enabled);
}
