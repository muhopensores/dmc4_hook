
#include "EnemySlotting.hpp"

bool      EnemySlotting::mod_enabled{ false };
uintptr_t EnemySlotting::jmp_ret{ NULL };
static int new_enemy_slot_limit = NULL;

// static uintptr_t enemySlottingMov{ 0x14EF4C40 }; // mov ecx,[DevilMayCry4_DX9.exe+A558AC] // mov ecx,[00E558AC]

naked void detour(void) {
	_asm {
		cmp byte ptr [EnemySlotting::mod_enabled], 0
		je originalcode

		push eax
		mov eax, [new_enemy_slot_limit]
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

std::optional<std::string> EnemySlotting::on_initialize() {
	if (!install_hook_offset(0x55850, hook, &detour, &jmp_ret, 10)) {
		spdlog::error("Failed to init EnemySlotting mod\n");
		return "Failed to init EnemySlotting mod";
	}
    
	return Mod::on_initialize();
}

void EnemySlotting::on_gui_frame() {
	ImGui::Checkbox("Custom Enemy Slot Limit", &mod_enabled);
    ImGui::SameLine();
    help_marker("Set how many enemies can attack at the same time. Default is usually 1.\n"
				"After unchecking this option, the original value will only be restored after entering a new room");
    ImGui::PushItemWidth(224);
    ImGui::InputInt("Slot Limit", &new_enemy_slot_limit, 1, 10);
    ImGui::PopItemWidth();
    ImGui::Spacing();
}

void EnemySlotting::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_slot_enable").value_or(false);
    new_enemy_slot_limit = cfg.get<int>("enemy_slot_limit").value_or(10);
};

void EnemySlotting::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_slot_enable", mod_enabled);
    cfg.set<int>("enemy_slot_limit", new_enemy_slot_limit);
};
