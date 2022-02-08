#include "modInputStates.hpp"
#include "modDeltaTime.hpp"
#include "modActiveBlock.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uint32_t InputStates::inputpressed{ 0 };
float InputStates::inputTimer{ 0.0f };
float TimerTick{ 1.0f };

InputStates::InputStates()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        cmp byte ptr [ActiveBlock::modEnabled], 0
        je code

        mov [InputStates::inputpressed], edx
        push eax
        push edx
        mov eax, 0x06
        mov dl, byte ptr [InputStates::inputpressed+1] // edx
        test eax, edx
        pop edx
        pop eax
        jnz inctimer
        mov dword ptr [InputStates::inputTimer], 0
        jmp code

    inctimer: // timer for ActiveBlock
        movss xmm0, [InputStates::inputTimer]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer], xmm0

    code:
        cmp [eax+0x41], cl
        mov [eax+10h], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

std::optional<std::string> InputStates::onInitialize()
{
    if (!install_hook_offset(0x3B0844, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    }

    return Mod::onInitialize();
}
#endif
