#include "modActiveBlock.hpp"
#include "modInputStates.hpp"

bool ActiveBlock::modEnabled{ false };
uintptr_t ActiveBlock::jmp_return{ NULL };
uintptr_t ActiveBlock::alt_ret{ 0x007BBAC1 };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

naked void detour() {
    _asm {
        cmp [ActiveBlock::modEnabled], 0
        je code

        push ecx
        mov ecx, [staticMediatorPtr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp esi, ecx
        pop ecx
        jne code

        cmp dword ptr [InputStates::inputTimer], 0x40a00000 // 5.0f
        ja code
        cmp dword ptr [InputStates::inputTimer], 0x00000000
        je code
        cmp dword ptr [esi+0x00014D98], 3 // [currentStyle], 3
        jne code
        jmp dword ptr [ActiveBlock::alt_ret]

    code:
        cmp dword ptr [esi+0x000152A0],00
		jmp dword ptr [ActiveBlock::jmp_return]
    }
}

std::optional<std::string> ActiveBlock::onInitialize() {
    if (!install_hook_offset(0x3BBAAE, hook, &detour, &jmp_return, 7)) {
        HL_LOG_ERR("Failed to init ActiveBlock mod\n");
        return "Failed to init ActiveBlock mod";
    }
    return Mod::onInitialize();
}

void ActiveBlock::onGUIframe() {
    ImGui::Checkbox("Active Block", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Block during uncancellable frames");
}

void ActiveBlock::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("active_block").value_or(false);
}

void ActiveBlock::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("active_block", modEnabled);
}
