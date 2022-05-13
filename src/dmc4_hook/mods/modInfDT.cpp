#include "../mods.h"
#include "modInfDT.hpp"
#include "../utils/MessageDisplay.hpp"
#include "modEnemySpawn.hpp"

bool InfDT::modEnabled{ false };
uintptr_t InfDT::jmp_ret{ NULL };
int InfDT::hotkey{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [InfDT::modEnabled], 0
			je originalcode

			mov dword ptr [ebp+0x00001F24], 0x461C4000
			jmp dword ptr [InfDT::jmp_ret]

			originalcode:
			movss xmm0, [ebp+0x00001F24]
			jmp dword ptr [InfDT::jmp_ret]
	}
}

std::optional<std::string> InfDT::onInitialize() {
	if (!install_hook_offset(0x00FF315, hook, &detour, &jmp_ret, 8)) {
		HL_LOG_ERR("Failed to init InfDT mod\n");
		return "Failed to init InfDT mod";
	}
	return Mod::onInitialize();
}

void InfDT::onGUIframe() {
    ImGui::Checkbox("Infinite DT", &modEnabled);
}

void InfDT::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("infinite_dt").value_or(false);
    hotkey = cfg.get<int>("inf_dt_hotkey").value_or(0x71); // F2
}

void InfDT::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("infinite_dt", modEnabled);
    cfg.set<int>("inf_dt_hotkey", hotkey);
}

void InfDT::onUpdateInput(hl::Input& input) {
    if (!input.isDown(EnemySpawn::hotkeySpawnModifier)) {
        if (input.wentDown(hotkey)) {
            if (modEnabled) {
                DISPLAY_MESSAGE("Infinite DT Off");
            }
            else {
                DISPLAY_MESSAGE("Infinite DT On");
            }
            modEnabled = !modEnabled;
        }
    }
}
