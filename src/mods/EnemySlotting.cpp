#include "EnemySlotting.hpp"

bool      EnemySlotting::mod_enabled = false;
uintptr_t EnemySlotting::jmp_ret = NULL;
static int new_enemy_slot_limit = 0;
static int backup_enemy_slot_limit = 0;

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

	MutatorRegistry::define("EnemySlotting")
		.description("hehe")
		.on_init([&] { mod_enabled = true; backup_enemy_slot_limit = new_enemy_slot_limit; new_enemy_slot_limit = 12; })
		.set_timer(35.0, [&] { mod_enabled = false; new_enemy_slot_limit = backup_enemy_slot_limit; });

	return Mod::on_initialize();
}

static int selected_slot = 0;
void EnemySlotting::on_gui_frame(int display) {
	const char* slot_options[] = { _("Default"), "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
	bool was_enabled = selected_slot > 0;
	ImGui::SetNextItemWidth(sameLineItemWidth);
	if (ImGui::Combo(_("Slot Limit"), &selected_slot, slot_options, IM_ARRAYSIZE(slot_options))) {
		mod_enabled = (selected_slot > 0);
		if (mod_enabled) {
			new_enemy_slot_limit = selected_slot - 1;
		}
	}
	ImGui::SameLine();
	help_marker(_("Set how many enemies can attack at the same time.\n"
				"After selecting Default, the original value will only be restored after entering a new fight"));
}

void EnemySlotting::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_slot_enable").value_or(false);
    new_enemy_slot_limit = cfg.get<int>("enemy_slot_limit").value_or(12);
    // old versions used to allow out of range values, this will correct configs
	if (new_enemy_slot_limit > 12) { new_enemy_slot_limit = 12; }
	if (mod_enabled) { selected_slot = new_enemy_slot_limit + 1; }
};

void EnemySlotting::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_slot_enable", mod_enabled);
    cfg.set<int>("enemy_slot_limit", new_enemy_slot_limit);
};
