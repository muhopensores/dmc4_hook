#include "../mods.h"
#include "modRoseRemovesPins.hpp"

bool RoseRemovesPins::modEnabled{ false };
uintptr_t RoseRemovesPins::jmp_ret{ NULL };
uintptr_t RoseRemovesPins::jmp_out{ 0x00815970 };

RoseRemovesPins::RoseRemovesPins()
{
	//onInitialize();
}

naked void detour(void)
{
	_asm {
			cmp byte ptr [RoseRemovesPins::modEnabled], 0
			je originalcode

			cmp byte ptr [esi+0x14], 04
			je removepinsje
			cmp byte ptr [esi+0x14], 03
			je threecode
			jmp dword ptr [RoseRemovesPins::jmp_ret]

		originalcode:
			cmp byte ptr [esi+0x14], 04
			je removepinsje
			jmp dword ptr [RoseRemovesPins::jmp_ret]

		threecode:
			mov byte ptr [esi+0x14], 00
		removepinsje:
			jmp dword ptr [RoseRemovesPins::jmp_out]
	}
}

std::optional<std::string> RoseRemovesPins::onInitialize()
{
	if (!install_hook_offset(0x04158C3, hook, &detour, &jmp_ret, 10))
    {
		HL_LOG_ERR("Failed to init RoseRemovesPins mod\n");
		return "Failed to init RoseRemovesPins mod";
	}
	return Mod::onInitialize();
}

void RoseRemovesPins::onGUIframe() {
    ImGui::Checkbox("Rose Removes Pins", &modEnabled);
}

void RoseRemovesPins::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("rose_removes_pins").value_or(false);
};

void RoseRemovesPins::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("rose_removes_pins", modEnabled);
};
