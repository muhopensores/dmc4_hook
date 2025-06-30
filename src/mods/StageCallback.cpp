#include "StageCallback.hpp"
#include <Mods.hpp>
uintptr_t StageCallback::jmp_ret1 = NULL;
uintptr_t StageCallback::jmp_ret2 = NULL;

void __stdcall stage_start() {
	return;
}

void __stdcall stage_end() {
    return;
}

naked void detour1() {
	_asm {
			call stage_start
		//originalcode:
			mov eax,[ebp+0x2c]
			push eax
			lea ecx,[esp+0x5c]
			jmp [StageCallback::jmp_ret1]
	}
}

naked void detour2() {
	_asm {
			call stage_end
		//originalcode:
			mov dword ptr [eax+0xd0],ebx
			jmp [StageCallback::jmp_ret2]
	}
}


std::optional<std::string> StageCallback::on_initialize() {
    if (!install_hook_offset(0xDC45, hook1, &detour1, &jmp_ret1, 8)) {
        spdlog::error("Failed to init StageCallback mod1\n");
        return "Failed to init StageCallback mod1";
    }

    if (!install_hook_absolute(0x040dcfa, hook2, &detour2, &jmp_ret2, 6)) {
        spdlog::error("Failed to init StageCallback mod2\n");
        return "Failed to init StageCallback mod2";
    }
    return Mod::on_initialize();
}
