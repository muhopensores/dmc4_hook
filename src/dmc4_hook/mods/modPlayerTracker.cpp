#include "../mods.h"
#include "modPlayerTracker.hpp"

// bool      PlayerTracker::modEnabled{ false };
uintptr_t PlayerTracker::lock_on_jmp_ret{ NULL };
uintptr_t PlayerTracker::lock_off_jmp_ret{ NULL };

int PlayerTracker::lockOnAlloc;

PlayerTracker::PlayerTracker()
{
	//onInitialize();
}

naked void lockOnDetour(void)
{
	_asm {
			// cmp byte ptr [PlayerTracker::modEnabled],0
			// je originalcode

			mov [PlayerTracker::lockOnAlloc], 1
            mov [edi+0x000016D0], 1
			jmp dword ptr [PlayerTracker::lock_on_jmp_ret]
	}
}

naked void lockOffDetour(void)
{
    _asm {
			mov [PlayerTracker::lockOnAlloc], 0
			mov [edi+0x000016D0], 0
			jmp dword ptr [PlayerTracker::lock_off_jmp_ret]
    }
}

std::optional<std::string> PlayerTracker::onInitialize() {

	if (!install_hook_offset(0x003A8337, lockOnHook, &lockOnDetour, &lock_on_jmp_ret, 10)) {
		HL_LOG_ERR("Failed to init PlayerTracker mod\n");
		return "Failed to init PlayerTracker mod";
	}
	if (!install_hook_offset(0x003A838C, lockOffHook, &lockOffDetour, &lock_off_jmp_ret, 10)) {
		HL_LOG_ERR("Failed to init PlayerTracker mod\n");
		return "Failed to init PlayerTracker mod";
	}

	return Mod::onInitialize();
}
/*
void PlayerTracker::onGUIframe() {

}

void PlayerTracker::onConfigLoad(const utils::Config& cfg) {
    //modEnabled = cfg.get<bool>("xx_mod_enable").value_or(false);
};

void PlayerTracker::onConfigSave(utils::Config& cfg) {
    //cfg.set<bool>("xx_mod_enable", modEnabled);
};
*/