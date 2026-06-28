#include "TrickDown.hpp"

bool TrickDown::mod_enabled = false;
// uintptr_t TrickDown::trick_begin_jmp_ret = NULL;
uintptr_t TrickDown::buffered_action_jmp_ret = NULL;
uintptr_t TrickDown::trick_down_jmp_ret = NULL;
uintptr_t TrickDown::floor_touch_jmp_ret = NULL;
uintptr_t TrickDown::landing_anim_jmp_ret = NULL;
bool TrickDown::downFlag = false;
static constexpr float down_float = -200.0f;
static constexpr float backforward_timer_comparison = 5.0f; // same as LiveMoveTableSwaps

// if downFlag is true, LiveMoveTableSwaps swaps sky star and trickster dash to trick, allowing this to work even if forward isn't currently held
// a cleaner fix would of course be adding a backforward trick input to MoveTable, but I don't really like adding to it

#if 0
naked void trick_begin_detour(void) { // not gonna player compare because the idea of boss dante using down trick on you is kinda funny
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp byte ptr [esi+0x27F8], 3 // backforward
			jne normalTrick
			sub esp, 4
			movss [esp], xmm0
			movss xmm0, [esi+0x2800]
			comiss xmm0, [backforward_timer_comparison]
			jb PopNormalTrick
			movss xmm0, [esp]
			add esp, 4
			mov byte ptr [TrickDown::downFlag], 1
			jmp originalcode

		PopNormalTrick:
			movss xmm0, [esp]
			add esp, 4
		normalTrick:
			mov byte ptr [TrickDown::downFlag], 0
		originalcode:
			mov ebx, 00000001
			jmp dword ptr [TrickDown::trick_begin_jmp_ret]
	}
}
#endif

// This is for more than just trick and so could probably do with being somewhere more accessible but not todayy
naked void buffered_action_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp dword ptr [ebp+4], 0x5d // trick inc?
			jne originalcode

			cmp byte ptr [esi+0x27F8], 3 // backforward
			jne normalTrick
			sub esp, 4
			movss [esp], xmm0
			movss xmm0, [esi+0x2800]
			comiss xmm0, [backforward_timer_comparison]
			jb popNormaltrick
			movss xmm0, [esp]
			add esp, 4
			mov byte ptr [TrickDown::downFlag], 1
			jmp originalcode

		popNormaltrick:
			movss xmm0, [esp]
			add esp, 4
		normalTrick:
			mov byte ptr [TrickDown::downFlag], 0
			jmp originalcode

		popcode:
			movss xmm0, [esp]
			add esp, 4
		originalcode:
			mov ecx, [esi+0x0000156C]
			jmp dword ptr [TrickDown::buffered_action_jmp_ret]
	}
}

naked void trick_down_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp byte ptr [TrickDown::downFlag], 1
			jne originalcode
			movss xmm2, [down_float] // Overwrite y axis momentum with -200 
		originalcode:
			movss [esi+0x00000EC4], xmm2
		retcode:
			jmp dword ptr [TrickDown::trick_down_jmp_ret]
	}
}

naked void floor_touch_detour(void) {
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 0
			je originalcode

			cmp byte ptr [TrickDown::downFlag], 1
			jne originalcode
			movss xmm2, [down_float] // Overwrite y axis momentum with -200 
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
    /*if (!install_hook_offset(0x3CAED1, trick_begin_hook, &trick_begin_detour, &trick_begin_jmp_ret, 5)) {
        spdlog::error("Failed to init TrickDown mod 0\n");
        return "Failed to init TrickDown1 mod 0";
    }*/
    if (!install_hook_offset(0x3B73CD, buffered_action_hook, &buffered_action_detour, &buffered_action_jmp_ret, 6)) {
        spdlog::error("Failed to init TrickDown mod 0\n");
        return "Failed to init TrickDown mod 0";
    }

	if (!install_hook_offset(0x3CB119, trick_down_hook, &trick_down_detour, &trick_down_jmp_ret, 8)) {
		spdlog::error("Failed to init TrickDown mod 1\n");
		return "Failed to init TrickDown mod 1";
	}
    if (!install_hook_offset(0x3CB33D, floor_touch_hook, &floor_touch_detour, &floor_touch_jmp_ret, 8)) {
        spdlog::error("Failed to init TrickDown mod 2\n");
        return "Failed to init TrickDown2 mod 2";
    }
    if (!install_hook_offset(0x3CB38E, landing_anim_hook, &landing_anim_detour, &landing_anim_jmp_ret, 6)) {
        spdlog::error("Failed to init TrickDown mod 3\n");
        return "Failed to init TrickDown mod 3";
    }
	return Mod::on_initialize();
}

void TrickDown::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Down Trick"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Map Down Trick to backforward + trick"));
    }
}

void TrickDown::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("trick_down").value_or(false);
}

void TrickDown::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("trick_down", mod_enabled);
}
