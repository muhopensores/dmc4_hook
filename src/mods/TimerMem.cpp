#include "TimerMem.hpp"
#include "TrickDown.hpp"
#include "MoveTable.hpp"

bool TimerMem::instant_honeycomb_enabled = false;
uintptr_t TimerMem::timer_jmp_ret = NULL;
uintptr_t TimerMem::back_forward_jmp_ret = 0x00805A60;
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

float TimerMem::timer_mem = 0.0f;
static float xmm5backup = 0.0f;
static float xmm5backup2 = 0.0f; // Used in reset
static float back_forward_time = 10.0f;
static float reset_timer = 10.0f;

naked void timer_detour(void) { // ticks timer, player in ecx
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 1
			je timerstart
			cmp byte ptr [TimerMem::instant_honeycomb_enabled], 1
			je timerstart
			jmp originalcode // if nothing is using the timer, skip it

		timerstart:
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp eax, ecx
			pop eax
			jne originalcode // only inc timer if player
			cmp byte ptr [ecx+0x1494], 0 // dante controller id
			jne originalcode

		// inc timer
			movss [xmm5backup], xmm5
			movss xmm5, [TimerMem::timer_mem]
			addss xmm5, [ecx+0x10]
			movss [TimerMem::timer_mem], xmm5
			movss xmm5, [xmm5backup]

        // trick down check
			cmp byte ptr [TrickDown::mod_enabled], 1
			je tricktimercompare
			jmp honeycombcheck

		tricktimercompare:
			movss [xmm5backup], xmm5
			movss xmm5, [TimerMem::timer_mem]
			comiss xmm5, [back_forward_time]
			movss xmm5, [xmm5backup]
			jb replacetrick      // If trick down is enabled, replace trickster dash and sky star to trick when timer is under (float)x
			jmp dontreplacetrick // By putting replacements on a timer you make a buffer for the input and have a convenient off state

		replacetrick:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetrick
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 96 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x5D // trickster dash
			mov dword ptr [eax+0x4+0x38], 0x5D // sky star
			pop ebx
			pop eax
			jmp honeycombcheck

		dontreplacetrick:
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 96 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x5B // trickster dash
			mov dword ptr [eax+0x4+0x38], 0x5C // sky star
			pop ebx
			pop eax
		honeycombcheck:
			cmp byte ptr [TimerMem::instant_honeycomb_enabled], 1
			je honeycombtimercompare   // If instant honeycomb is enabled, replace twosome time with honeycomb fire when timer is under (float)x
			jmp dontreplacetwosome

		honeycombtimercompare:
			movss [xmm5backup], xmm5
			movss xmm5, [TimerMem::timer_mem]
			comiss xmm5, [back_forward_time]
			movss xmm5, [xmm5backup]
			jb replacetwosome
			jmp dontreplacetwosome

		replacetwosome:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetwosome
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 69 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x45 // Replace Twosome Time ID with Honeycomb Fire ID
			pop ebx
			pop eax
			jmp originalcode

		dontreplacetwosome:
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 69 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x44 // Restore Honeycomb Fire ID
			pop ebx
			pop eax

		originalcode:
			test bl, 01
			mov dword ptr [ecx+0x00000EA8], 00000000
			jmp dword ptr [TimerMem::timer_jmp_ret]
	}
}

naked void back_forward_detour(void) { // resets timer, player in ebx
	_asm {
			cmp byte ptr [TimerMem::instant_honeycomb_enabled], 1
			je newcode
			cmp byte ptr [TrickDown::mod_enabled], 1
			je newcode
			jmp originalcode

		newcode:
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp ebx, eax
			pop eax
			jne originalcode // filter out any non player and checks if valid, crashes without
			cmp byte ptr [ebx+0x1494], 0 // dante controller id
			jne originalcode

			cmp al, 0x3
			je ResetTimer
			jmp originalcode

		ResetTimer:
			movss [xmm5backup2], xmm5
			movss xmm5, [TimerMem::timer_mem]
			comiss xmm5, [reset_timer]
			movss xmm5, [xmm5backup2]
			jl originalcode // If timer is less than 10, don't reset. With this, timer is only reset once per backforward.
			mov dword ptr [TimerMem::timer_mem], 0
		originalcode:
			mov [ebp+0x00], al
			jmp dword ptr [TimerMem::back_forward_jmp_ret] // next opcode was jmp x so rather than returning I do that
	}
}

std::optional<std::string> TimerMem::on_initialize() {
	if (!install_hook_offset(0x003AD767, timer_hook, &timer_detour, &timer_jmp_ret, 13)) {
		spdlog::error("Failed to init TimerMem mod\n");
		return "Failed to init TimerMem mod";
	}
	if (!install_hook_offset(0x00405BC1, back_forward_hook, &back_forward_detour, 0, 8)) {
		spdlog::error("Failed to init TimerMem mod\n");
		return "Failed to init TimerMem mod";
    }
	return Mod::on_initialize();
}

void TimerMem::on_gui_frame(int display) {
    ImGui::Checkbox(_("Instant Honeycomb"), &instant_honeycomb_enabled);
    ImGui::SameLine();
    help_marker(_("Honeycomb Mapped To backforward + Gunslinger"));
}

void TimerMem::on_config_load(const utility::Config& cfg) {
    instant_honeycomb_enabled = cfg.get<bool>("instant_honeycomb").value_or(false);
}

void TimerMem::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("instant_honeycomb", instant_honeycomb_enabled);
}
