#include "modInputStates.hpp"
#include "modDeltaTime.hpp"
#include "modActiveBlock.hpp"
#include "modMoveIDs.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uintptr_t InputStates::jmp_return2{ NULL };
uint32_t InputStates::inputpressed{ 0 };
float InputStates::inputTimer{ 0.0f };
float TimerTick{ 1.0f };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
bool InputStates::touchpadRoseEnabled{ false };
bool roseInput{ false };
bool bufferedRose{ false };

InputStates::InputStates()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        push ecx
        mov ecx, [staticMediatorPtr] // only get player inputs (thanks boss dante)
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        add ecx, 0x1408 // start of input stuff is at uPlayer+0x1408
        cmp eax, ecx
        pop ecx
        jne code // if not player, jump to code
        mov [InputStates::inputpressed], edx
        cmp byte ptr [ActiveBlock::modEnabled], 1
        jne checkRose // code
    // active block setup
        push eax
        push edx
        mov eax, 0x06
        mov dl, byte ptr [InputStates::inputpressed+1] // edx
        test al, dl // ActiveBlock
        jnz inctimer
        mov dword ptr [InputStates::inputTimer], 0
        pop edx
        pop eax
        jmp checkRose // code

    inctimer: // timer for ActiveBlock
        pop edx
        pop eax
        movss xmm0, [InputStates::inputTimer]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer], xmm0
        jmp checkRose

    checkRose: // rose setup
        push eax
        push edx
        mov eax, 0x8 // edx has desired input
        mov dl, byte ptr [InputStates::inputpressed]
        test al, dl
        pop edx
        pop eax
        jnz rosethrow
    code:
        cmp [eax+0x41], cl
        mov [eax+10h], edx
		jmp dword ptr [InputStates::jmp_return]

    rosethrow:
        mov byte ptr [roseInput], 1
        jmp code
    }
}

naked void detour2() // called when you press touchpad or jump during cancellable frames, no buffers :(
{
    _asm {
    code:
        mov ecx, [eax+0x0000140C]

        push ecx
        mov ecx, [staticMediatorPtr] // only get player inputs (thanks boss dante)
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp eax, ecx
        pop ecx
        jne jmpret // if not player, jump to code

    // checkrose
        push eax
        push edx
        mov eax, 0x8 // edx has desired input
        mov dl, byte ptr [InputStates::inputpressed]
        test al, dl
        pop edx
        pop eax
        jnz rosethrow
    jmpret:
		jmp dword ptr [InputStates::jmp_return2]

    rosethrow:
        mov byte ptr [roseInput], 1
        jmp jmpret
    }
}

std::optional<std::string> InputStates::onInitialize()
{
    if (!install_hook_offset(0x3B0844, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    } /*
    if (!install_hook_offset(0x403351, hook2, &detour2, &jmp_return2, 6))
    {
        HL_LOG_ERR("Failed to init InputStates2 mod\n");
        return "Failed to init InputStates2 mod";
    }*/
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
}

void PlayRose(void){
    sMediator* sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    uint8_t* moveBank = (uint8_t*)uLocalPlr + 0x1500;
    uint8_t* moveID = (uint8_t*)uLocalPlr + 0x1564;
    uint8_t* movePart = (uint8_t*)uLocalPlr + 0x1504;



    //uintptr_t* luciferPtr = (uintptr_t*)uLocalPlr + 0x1D98;
    //uintptr_t luciferBase = *(uintptr_t*)luciferPtr;
    //bool* showLucifer = (bool*)luciferBase + 0x137C;



    // please save me i just want mov ecx, [eax+0x1D98]
    //                            mov ecx, [ecx+0x137C]


    //*showLucifer = true;
    *moveBank = 12;
    *moveID = 55;
    *movePart = 00;
}

void InputStates::onFrame(fmilliseconds& dt) {
    sMediator* sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    bool* grounded = (bool*)uLocalPlr + 0x2008;
    uint8_t* cancellable = (uint8_t*)uLocalPlr + 0x1E15;
    if (uLocalPlr)
    {
        if (touchpadRoseEnabled) // if cheat is enabled
        {
            // input
            if (roseInput) // if touchpad is pressed
            {
                if (*grounded == false)
                {
                    if (*cancellable == 0x10) // if in free frames
                    {
                        PlayRose();
                    }
                    if (*cancellable == 0x30) // if in buffer frames
                    {
                        bufferedRose = true;
                    }
                    roseInput = false;
                }
                else roseInput = false;
            }
            // buffer
            if (bufferedRose)
            {
                if (*grounded == false) // grounded?
                {
                    if (*cancellable == 0x10) // if in free frames
                    {
                        bufferedRose = false;
                        PlayRose();
                    }
                    if (*cancellable == 0x00) // if another attack starts, kill the buffer
                    {
                        bufferedRose = false;
                    }
                }
                else bufferedRose = false; // grounded.
            }
        }
        else roseInput = false;
    }
}

#endif

/*
        push edx
        mov edx, [eax+0x1D98] // lucifer
        mov [edx+0x137C], 1 // show lucifer
        pop edx
        mov [eax+0x1500], 12 // move bank
        mov [eax+0x1564], 55 // moveid
        mov [eax+0x1504], 0 // part of move
*/