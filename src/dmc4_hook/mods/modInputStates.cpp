#include "modInputStates.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uint32_t InputStates::inputpressed{ 0 };

InputStates::InputStates()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        mov [eax+10h], edx
        mov [eax+34h], cl

        mov [InputStates::inputpressed], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

std::optional<std::string> InputStates::onInitialize()
{
    if (!install_hook_offset(0x3B0847, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init WorkRate mod\n");
        return "Failed to init WorkRate mod";
    }

    return Mod::onInitialize();
}
#endif
