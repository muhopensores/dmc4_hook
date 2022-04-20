#include "../mods.h"
#include "modTimerMem.hpp"
#include "modDeltaTime.hpp"
#include "modPlayerTracker.hpp"
#include "modTrickDown.hpp"

bool TimerMem::instantHoneycombEnabled{ false };
uintptr_t TimerMem::timer_jmp_ret{ NULL };
uintptr_t TimerMem::back_forward_jmp_ret{ 0x00805A60 };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

float TimerMem::timerMem = 0.0f;
float timerMemTick = 2.0f;
float xmmbackup = 0.0f;

naked void timerDetour(void) { // ticks timer, player in ecx
	_asm {
			cmp byte ptr [TrickDown::modEnabled], 1
			je timerstart
			cmp byte ptr [TimerMem::instantHoneycombEnabled], 1
			je timerstart
			jmp originalcode // if nothing is using the timer, skip it

		timerstart:
			push eax
			mov eax, [staticMediatorPtr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp eax,ecx
			pop eax
			jne originalcode // only inc timer if player
			cmp [ecx+0x1494], 0 // dante controller id
			jne originalcode

		// inc timer
			movss xmm5, [TimerMem::timerMem]
			movss [xmmbackup], xmm6
			movss xmm6, [timerMemTick] // Timer starts at 0, has x added to it every tick and is reset every time a backforward input is made
			mulss xmm6, [DeltaTime::currentDeltaTime]
			addss xmm5, xmm6
			movss xmm6, [xmmbackup]
			movss [TimerMem::timerMem], xmm5

        // trick down check
			cmp byte ptr [TrickDown::modEnabled], 1
			je tricktimercompare
			jmp honeycombcheck

		tricktimercompare:
			cmp dword ptr [TimerMem::timerMem], 0x41a00000 // 20
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
			cmp byte ptr [TimerMem::instantHoneycombEnabled], 1
			je honeycombtimercompare   // If instant honeycomb is enabled, replace twosome time with honeycomb fire when timer is under (float)x
			jmp dontreplacetwosome

		honeycombtimercompare:
			cmp dword ptr [TimerMem::timerMem], 0x41a00000 // 20
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

naked void backForwardDetour(void) { // resets timer, player in ebx
	_asm {
        // trick compare
			push eax
			mov eax, [staticMediatorPtr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp ebx, eax
			pop eax
			jne originalcode // filter out any non player and checks if valid, crashes without
			cmp [ebx+0x1494], 0 // dante controller id
			jne originalcode

			cmp byte ptr [TrickDown::modEnabled], 0
			je honeycombcompare

			cmp [TimerMem::timerMem],0x41200000 // = 10 // If timer is less than 10, don't reset. With this, timer is only reset once per backforward.
			jl originalcode                     // 10 is over before the moves can finish, so no worry them not being available
			cmp al,0x3
			je resettimer

		honeycombcompare:
			cmp byte ptr [TimerMem::instantHoneycombEnabled], 0
			je originalcode

			cmp [TimerMem::timerMem], 0x41200000 // = 10
			jl originalcode
			cmp al,0x3
			je resettimer
			jmp originalcode

		resettimer:
			mov dword ptr [TimerMem::timerMem], 0x00000000 // = 0

		originalcode:
			mov [ebp+0x00], al
			jmp dword ptr [TimerMem::back_forward_jmp_ret] // next opcode was jmp x so rather than returning I do that
	}
}

std::optional<std::string> TimerMem::onInitialize() {

	if (!install_hook_offset(0x003AD767, timerHook, &timerDetour, &timer_jmp_ret, 13)) {
		HL_LOG_ERR("Failed to init TimerMem mod\n");
		return "Failed to init TimerMem mod";
	}
	if (!install_hook_offset(0x00405BC1, backForwardHook, &backForwardDetour, 0, 8)) {
		HL_LOG_ERR("Failed to init TimerMem mod\n");
		return "Failed to init TimerMem mod";
    }
	return Mod::onInitialize();
}

void TimerMem::onGUIframe() {
    ImGui::Checkbox("Instant Honeycomb", &instantHoneycombEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("Honeycomb Mapped To backforward + Gunslinger");
}

void TimerMem::onConfigLoad(const utils::Config& cfg) {
    instantHoneycombEnabled = cfg.get<bool>("instant_honeycomb").value_or(false);
}

void TimerMem::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("instant_honeycomb", instantHoneycombEnabled);
}
