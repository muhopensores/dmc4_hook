#include "../mods.h"
#include "modEnemySlotting.hpp"

bool      EnemySlotting::modEnabled{ false };
uintptr_t EnemySlotting::jmp_ret{ NULL }; // 0x00737732
int enemyslotlimit{};

static uintptr_t enemySlottingMov{ 0x00E558AC }; // mov ecx,[DevilMayCry4_DX9.exe+A558AC] // mov ecx,[00E558AC]

EnemySlotting::EnemySlotting() {
	//onInitialize();
}

naked void detour(void)
{
	_asm {
		cmp byte ptr [EnemySlotting::modEnabled],0
		je originalcode

		mov eax, [enemyslotlimit]
		mov ecx, [0x00E558AC] // compiles as mov ecx,00E558AC rather than mov ecx,[00E558AC]
		jmp dword ptr [EnemySlotting::jmp_ret]

	originalcode:
		mov eax, [esi+0x28]
		mov ecx, [enemySlottingMov] // compiles as mov ecx,[7B7CB1E8]
		jmp dword ptr [EnemySlotting::jmp_ret]
	}
}

std::optional<std::string> EnemySlotting::onInitialize() {
    /*
	if (!install_hook_offset(0x337729, hook, &detour, &jmp_ret, 9)) {
		HL_LOG_ERR("Failed to init EnemySlotting mod\n");
		return "Failed to init EnemySlotting mod";
	}
    */
	return Mod::onInitialize();
}

void EnemySlotting::onGUIframe() {
	ImGui::Checkbox("Custom Enemy Slot Limit", &modEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("Set how many enemies can attack at the same time.\nDefault is usually 1");
    ImGui::PushItemWidth(215);
    ImGui::InputInt("Slot Limit ", &enemyslotlimit, 1, 10, ImGuiInputTextFlags_AllowTabInput);
    ImGui::PopItemWidth();
}

void EnemySlotting::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("enemy_slot_enable").value_or(false);
    enemyslotlimit = cfg.get<int>("enemy_slot_limit").value_or(1);
};

void EnemySlotting::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("enemy_slot_enable", modEnabled);
    cfg.set<int>("enemy_slot_limit", enemyslotlimit);
};

/*
[ENABLE]
//aobscanmodule(Slotting,DevilMayCry4_DX9.exe,8B 46 28 8B 0D AC 58 E5 00) // should be unique
define(Slotting,"DevilMayCry4_DX9.exe"+337729)
alloc(newmem,$1000)
label(code)
label(return)

newmem:

code:
  mov eax,1 //[esi+28]
  mov ecx,[DevilMayCry4_DX9.exe+A558AC] // mov ecx,[00E558AC]
  jmp return

Slotting:
  jmp newmem
  nop
  nop
  nop
  nop
return:
registersymbol(Slotting)

[DISABLE]
Slotting:
  db 8B 46 28 8B 0D AC 58 E5 00
unregistersymbol(Slotting)
dealloc(newmem)

"DevilMayCry4_DX9.exe"+337727: FF E2                 -  jmp edx
// ---------- INJECTING HERE ----------
"DevilMayCry4_DX9.exe"+337729: 8B 46 28              -  mov eax,[esi+28]
"DevilMayCry4_DX9.exe"+33772C: 8B 0D AC 58 E5 00     -  mov ecx,[DevilMayCry4_DX9.exe+A558AC]
// ---------- DONE INJECTING  ----------
"DevilMayCry4_DX9.exe"+337732: 50                    -  push eax

*/