#include "../mods.h"
#include "modEnemySlotting.hpp"

bool      EnemySlotting::modEnabled{ false };
uintptr_t EnemySlotting::jmp_ret{ NULL };
static int newEnemySlotLimit = NULL;

// static uintptr_t enemySlottingMov{ 0x14EF4C40 }; // mov ecx,[DevilMayCry4_DX9.exe+A558AC] // mov ecx,[00E558AC]

naked void detour(void) {
	_asm {
		cmp byte ptr [EnemySlotting::modEnabled], 0
		je originalcode

		push eax
		mov eax, [newEnemySlotLimit]
		mov [esi+0x04], eax
		pop eax

	originalcode:
		cmp dword ptr [esi+0x04], 00
		jng jngcode
		jmp dword ptr [EnemySlotting::jmp_ret]

	jngcode: // usually jumps to the ret of the function
		ret
	}
}

std::optional<std::string> EnemySlotting::onInitialize() {
	if (!install_hook_offset(0x55850, hook, &detour, &jmp_ret, 10)) {
		HL_LOG_ERR("Failed to init EnemySlotting mod\n");
		return "Failed to init EnemySlotting mod";
	}
    
	return Mod::onInitialize();
}

void EnemySlotting::onGUIframe() {
	ImGui::Checkbox("Custom Enemy Slot Limit", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Set how many enemies can attack at the same time. Default is usually 1.\n"
				"After unchecking this option, the original value will only be restored after entering a new room");
    ImGui::PushItemWidth(224);
    ImGui::InputInt("Slot Limit", &newEnemySlotLimit, 1, 10);
    ImGui::PopItemWidth();
    ImGui::Spacing();
}

void EnemySlotting::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("enemy_slot_enable").value_or(false);
    newEnemySlotLimit = cfg.get<int>("enemy_slot_limit").value_or(10);
};

void EnemySlotting::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("enemy_slot_enable", modEnabled);
    cfg.set<int>("enemy_slot_limit", newEnemySlotLimit);
};
