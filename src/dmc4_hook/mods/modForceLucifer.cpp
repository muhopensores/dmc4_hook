#include "../mods.h"
#include "modForceLucifer.hpp"
#include "modInputStates.hpp"

#if 1
bool ForceLucifer::modEnabled{ false };
uintptr_t ForceLucifer::_forceLuciferContinue{ NULL };
bool ForceLucifer::enableForceEcstasyTimer{ false };

naked void forceLucifer_proc(void) {
    _asm {
        cmp [ForceLucifer::modEnabled], 1
        je ForceLuci
    // check2: // disables turning off lucifer when swapping off of the weapon
        cmp byte ptr [ForceLucifer::enableForceEcstasyTimer], 1
        jne code
        cmp [esi+1370h], 6 // ID (this accesses gilg etc too)
        jne code
        cmp byte ptr [InputStates::roseTimerActive], 1
        jne code
        cmp al, 1
        je code
        jmp dword ptr [ForceLucifer::_forceLuciferContinue]

    ForceLuci:
        cmp [esi+0x1370], 6 // ID (this accesses gilg etc too)
        jne code
        mov [esi+0x137C], 1
        jmp code

    code:
        mov [esi+0x0000137C], al
		jmp dword ptr [ForceLucifer::_forceLuciferContinue]
    }
}

std::optional<std::string> ForceLucifer::onInitialize() {
    if (!install_hook_offset(0x435F9D, hook, &forceLucifer_proc, &ForceLucifer::_forceLuciferContinue, 6)) {
        HL_LOG_ERR("Failed to init ForceLucifer mod\n");
        return "Failed to init ForceLucifer mod";
    }

    return Mod::onInitialize();
}

void ForceLucifer::onGUIframe() {
    ImGui::Checkbox("Force Lucifer", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Useful for disabling the Rose despawns");
}

void ForceLucifer::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("force_lucifer").value_or(false);
};

void ForceLucifer::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("force_lucifer", modEnabled);
};

#endif
