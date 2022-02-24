#include "modInputStates.hpp"
#include "modDeltaTime.hpp"
#include "modActiveBlock.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uintptr_t InputStates::jmp_return2{ NULL };
uint32_t InputStates::inputpressed{ 0 };
float InputStates::inputTimer{ 0.0f };
float TimerTick{ 1.0f };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
bool InputStates::touchpadRoseEnabled{ false };

InputStates::InputStates()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        cmp byte ptr [ActiveBlock::modEnabled], 1
        je getplayerinputs
        jmp code

    getplayerinputs:
        push ecx
        mov ecx, [staticMediatorPtr] // only get player inputs (thanks boss dante)
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        add ecx, 0x1408 // start of input stuff is at uPlayer+0x1408
        cmp eax, ecx
        pop ecx
        jne code
        mov [InputStates::inputpressed], edx
    // active block setup
        push eax
        push edx
        mov eax, 0x06
        mov dl, byte ptr [InputStates::inputpressed+1] // edx
        test eax, edx // ActiveBlock
        jnz inctimer
        mov dword ptr [InputStates::inputTimer], 0
        pop edx
        pop eax
        jmp code

    inctimer: // timer for ActiveBlock
        pop edx
        pop eax
        movss xmm0, [InputStates::inputTimer]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer], xmm0
        jmp code

    code:
        cmp [eax+0x41], cl
        mov [eax+10h], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

naked void detour2() // called when you press touchpad or jump during buffer/cancellable frames
{
    _asm {
        mov ecx, [eax+0x0000140C] // ecx has inputs, eax has player // code
        cmp byte ptr [InputStates::touchpadRoseEnabled], 0
        je jmpret

        push edx
        mov edx, 0x8 // edx has desired input
        test cl, dl
        pop edx
        jnz rosethrow
        jmp jmpret

    rosethrow:
        cmp [eax+0x2008], 1 // grounded?
        je jmpret
        push edx
        mov edx, [eax+0x1D98] // lucifer
        mov [edx+0x137C], 1 // show lucifer
        pop edx
        mov [eax+0x1500], 12 // move bank
        mov [eax+0x1564], 55 // moveid
        mov [eax+0x1504], 0 // part of move
        jmp jmpret
    jmpret:
		jmp dword ptr [InputStates::jmp_return2]
    }
}

std::optional<std::string> InputStates::onInitialize()
{
    if (!install_hook_offset(0x3B0844, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    }
    if (!install_hook_offset(0x403351, hook2, &detour2, &jmp_return2, 6))
    {
        HL_LOG_ERR("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    }

    return Mod::onInitialize();
}

void InputStates::onConfigLoad(const utils::Config& cfg)
{
    touchpadRoseEnabled = cfg.get<bool>("taunt_ectasy").value_or(false);
};

void InputStates::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("taunt_ectasy", touchpadRoseEnabled);
};

void InputStates::onGUIframe()
{
    ImGui::Checkbox("Taunt Ecstasy", &touchpadRoseEnabled);
    ImGui::SameLine();
    HelpMarker("Consider selective cancellable ecstasy to make this feel like 5");
}

#endif
