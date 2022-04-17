#include "../mods.h"
#include "modForceLucifer.hpp"
#include "modInputStates.hpp"

#if 1
bool ForceLucifer::modEnabled{ false };
uintptr_t ForceLucifer::_forceLuciferContinue{ NULL };
bool ForceLucifer::enableForceEcstasyTimer{ false };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

naked void forceLucifer_proc(void) {
    _asm {
        push eax
        mov eax, [staticMediatorPtr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp [eax+0x1494], 0 // controller id in case Nero uses this
        pop eax
        jne code
        cmp byte ptr [ForceLucifer::modEnabled], 1
        je ForceLuci

    // check2:
        cmp byte ptr [InputStates::roseTimerActive], 1
        jne code
    ForceLuci:
        cmp edi, 6 // ID (this accesses gilg etc too)
        jne code
        cmp al,0 // on/off
        je retcode
        jmp code

    code:
        mov [esi+0x0000137C], al
    retcode:
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
    HelpMarker("Forcing Lucifer means never forcefully despawning rose");
}

void ForceLucifer::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("force_lucifer").value_or(false);
};

void ForceLucifer::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("force_lucifer", modEnabled);
};

#endif
