
#include "TimerMem.hpp"
#include "DeltaTime.hpp"
#include "PlayerTracker.hpp"
#include "TrickDown.hpp"

bool TimerMem::instant_honeycomb_enabled{ false };
uintptr_t TimerMem::timer_jmp_ret{ NULL };
uintptr_t TimerMem::back_forward_jmp_ret{ 0x00805A60 };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

float TimerMem::timer_mem = 0.0f;
static float timer_mem_tick = 2.0f;
static float xmmbackup = 0.0f;

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
			cmp eax,ecx
			pop eax
			jne originalcode // only inc timer if player
			cmp byte ptr [ecx+0x1494], 0 // dante controller id
			jne originalcode

		// inc timer
			movss xmm5, [TimerMem::timer_mem]
			movss [xmmbackup], xmm6
			movss xmm6, [timer_mem_tick] // Timer starts at 0, has x added to it every tick and is reset every time a backforward input is made
			mulss xmm6, [DeltaTime::current_delta_time]
			addss xmm5, xmm6
			movss xmm6, [xmmbackup]
			movss [TimerMem::timer_mem], xmm5

        // trick down check
			cmp byte ptr [TrickDown::mod_enabled], 1
			je tricktimercompare
			jmp honeycombcheck

		tricktimercompare:
			cmp dword ptr [TimerMem::timer_mem], 0x41a00000 // 20
			jl replacetrick                                // If trick down is enabled, replace trickster dash and sky star to trick when timer is under (float)x
			jmp dontreplacetrick						   // By putting replacements on a timer you make a buffer for the input and have a convenient off state

		replacetrick:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetrick
			push eax
			mov eax, 0x00C413A4        // Trickster Dash Address
			mov dword ptr [eax], 0x5D  // Trick ID
			mov eax, 0x00C413DC        // Sky Star Address
			mov dword ptr [eax], 0x5D  // Trick ID
			pop eax
			jmp honeycombcheck

		dontreplacetrick:
			push eax
			mov eax, 0x00C413A4        // Trickster Dash Address
			mov dword ptr [eax], 0x5B  // Trickster Dash ID
			mov eax, 0x00C413DC        // Sky Star Address
			mov dword ptr [eax], 0x5C  // Sky Star ID
			pop eax

		honeycombcheck:
			cmp byte ptr [TimerMem::instant_honeycomb_enabled], 1
			je honeycombtimercompare   // If instant honeycomb is enabled, replace twosome time with honeycomb fire when timer is under (float)x
			jmp dontreplacetwosome

		honeycombtimercompare:
			cmp dword ptr [TimerMem::timer_mem], 0x41a00000 // 20
			jl replacetwosome
			jmp dontreplacetwosome

		replacetwosome:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetwosome
			push eax
			mov eax, 0x00C40DBC       // Twosome Time Address
			mov dword ptr [eax], 0x45 // Honeycomb Fire ID
			pop eax
			jmp originalcode

		dontreplacetwosome:
			push eax
			mov eax, 0x00C40DBC       // Twosome Time Address
			mov dword ptr [eax],0x44  // Twosome Time ID
			pop eax

		originalcode:
			test bl,01
			mov [ecx+0x00000EA8], 00000000
			jmp dword ptr [TimerMem::timer_jmp_ret]
	}
}

naked void back_forward_detour(void) { // resets timer, player in ebx
	_asm {
        // trick compare
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp ebx, eax
			pop eax
			jne originalcode // filter out any non player and checks if valid, crashes without
			cmp byte ptr [ebx+0x1494], 0 // dante controller id
			jne originalcode

			cmp byte ptr [TrickDown::mod_enabled], 0
			je honeycombcompare

			cmp dword ptr [TimerMem::timer_mem],0x41200000 // = 10 // If timer is less than 10, don't reset. With this, timer is only reset once per backforward.
			jl originalcode                     // 10 is over before the moves can finish, so no worry them not being available
			cmp al,0x3
			je resettimer

		honeycombcompare:
			cmp byte ptr [TimerMem::instant_honeycomb_enabled], 0
			je originalcode

			cmp dword ptr [TimerMem::timer_mem], 0x41200000 // = 10
			jl originalcode
			cmp al,0x3
			je resettimer
			jmp originalcode

		resettimer:
			mov dword ptr [TimerMem::timer_mem], 0x00000000 // = 0

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

void TimerMem::on_gui_frame() {
    ImGui::Checkbox("Instant Honeycomb", &instant_honeycomb_enabled);
    ImGui::SameLine();
    help_marker("Honeycomb Mapped To backforward + Gunslinger");
}

void TimerMem::on_config_load(const utility::Config& cfg) {
    instant_honeycomb_enabled = cfg.get<bool>("instant_honeycomb").value_or(false);
}

void TimerMem::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("instant_honeycomb", instant_honeycomb_enabled);
}
