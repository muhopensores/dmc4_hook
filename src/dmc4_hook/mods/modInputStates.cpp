#include "modInputStates.hpp"
#include "modDeltaTime.hpp"
#include "modActiveBlock.hpp"
#include "modMoveIDs.hpp"
#include "modForceLucifer.hpp" // used to stop rose despawning when leaving lucifer
#include "modFasterFastDrive.hpp" // for easy fast drive
#include "../utils/MessageDisplay.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uintptr_t InputStates::jmp_return2{ NULL };
uintptr_t InputStates::jmp_return3{ NULL };

uint32_t InputStates::inputpressed{ 0 };
float InputStates::inputTimer{ 0.0f };
float InputStates::inputTimer2{ 0.0f };
float TimerTick{ 1.0f };

constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

bool InputStates::touchpadRoseEnabled{ false };
bool InputStates::roseTimerActive{ false };

static bool roseInput{ false };
static bool bufferedRose{ false };

constexpr uintptr_t changeToLuciferCall = 0x00836190;

void EasyFastDriveCheck(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uintptr_t* playerPtr = (uintptr_t*)((uintptr_t)sMedPtr + 0x24);
    uintptr_t playerBase = *playerPtr;
    if (playerBase) {
        int8_t& controllerID = *(int8_t*)(playerBase + 0x1494);
        int& moveID = *(int*)(playerBase + 0x2998);
        int8_t& meleeInputOnPress = *(int8_t*)(playerBase + 0x1410);
        int8_t& styleInputOnHold = *(int8_t*)(playerBase + 0x140D);
        int8_t& cancelToMelee = *(int8_t*)(playerBase + 0x1E13);
        float& animationFrame = *(float*)(playerBase + 0x348);
        if (meleeInputOnPress & 0x1 && styleInputOnHold & 0x6 && controllerID == 0 && moveID == 0x232 && animationFrame < 7.0f) {
            cancelToMelee = 0x10;
        }
    }
}

naked void detour() { // inputpressed // inputs are edx // player is in edi // ActiveBlock & touchpad ectasy, called on tick
    _asm {
        push eax
        mov eax, [staticMediatorPtr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp eax,edi
        pop eax
        jne code

        mov [InputStates::inputpressed], edx

        cmp byte ptr [ActiveBlock::modEnabled], 1
        je ActiveBlockSetup
        jmp TouchpadRoseCheck

    ActiveBlockSetup:
        push eax
        push edx
        mov eax, 0x06
        mov dl, byte ptr [InputStates::inputpressed+1] // edx
        test al, dl // ActiveBlock
        pop edx
        pop eax
        jnz inctimer
        mov dword ptr [InputStates::inputTimer], 0
        jmp TouchpadRoseCheck

    inctimer: // timer for ActiveBlock
        movss xmm0, [InputStates::inputTimer]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer], xmm0

    TouchpadRoseCheck:
        cmp byte ptr [InputStates::touchpadRoseEnabled], 0
        je code
        push eax
        call InputStates::RoseBuffer
        pop eax
        cmp byte ptr [InputStates::roseTimerActive], 1
        je IncRoseTimer
        jmp code

    IncRoseTimer: // timer for HideLucifer
        movss xmm0, [InputStates::inputTimer2]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer2], xmm0
        cmp dword ptr [InputStates::inputTimer2], 0x43480000 // 200.0f
        jl code
        mov byte ptr [InputStates::roseTimerActive], 0
        push eax
        push ecx
        call InputStates::onTimerCallback
        pop ecx
        pop eax

    code:
        cmp [eax+0x41], cl
        mov [eax+0x10], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

naked void detour2() { // inputonpress // touchpad ecstasy // player is in edx // called on button press
    _asm {
        mov [edx+0x00001410], eax // originalcode

        push eax
        mov eax, [staticMediatorPtr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp eax,edx
        pop eax
        jne jmpret

        cmp dword ptr [edx+0x1494], 0 // dante controller id
        jne jmpret

        cmp byte ptr [FasterFastDrive::easyFastDriveEnabled], 1
        jne check2
        push eax
        call EasyFastDriveCheck
        pop eax
    check2:
        cmp byte ptr [InputStates::touchpadRoseEnabled], 0
        je jmpret

    // checkrose
        push edx
        mov edx, 0x8 // edx has desired input
        test al, dl
        pop edx
        jnz rosethrow
    jmpret:
		jmp dword ptr [InputStates::jmp_return2]

    rosethrow:
        mov byte ptr [roseInput], 1
        push ecx
        push eax
        push edx
        call InputStates::RoseInput
        pop edx
        pop eax
        pop ecx
        jmp jmpret
    }
}

naked void detourChangingToLucifer() {
    _asm {
        cmp byte ptr [InputStates::roseTimerActive], 1
        jne code
        jmp dword ptr [InputStates::jmp_return3]

    code:
        call [changeToLuciferCall]
		jmp dword ptr [InputStates::jmp_return3]
    }
}

void InputStates::onTimerCallback() { // hide lucifer after rose if weaponid is not lucifer
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    if (uLocalPlr) {
        uPlayer* uLocalPlr = sMedPtr->playerPtr;
        // uint8_t& weaponID = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1DB4);
        uint8_t& equippedWeapon = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1DC0); // might fix a small bug might break everything
        uintptr_t* luciferPtr = (uintptr_t*)((uintptr_t)uLocalPlr + 0x1D98);
        uintptr_t luciferBase = *luciferPtr;
        bool& showLucifer = *(bool*)(luciferBase + 0x137C);
        if (showLucifer == true && equippedWeapon != 6) {
            if (!ForceLucifer::modEnabled){
                    showLucifer = false;
            }
        }
    }
}

void InputStates::RoseInput(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    if (uLocalPlr) {
        uint8_t& grounded = *(uint8_t*)((uintptr_t)uLocalPlr + 0xEA8);
        uint8_t& cancellable = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1E15);
        int& moveID = *(int*)((uintptr_t)uLocalPlr + 0x2998); // i hate myself
        // input
        if (roseInput) { // if touchpad is pressed
            if (grounded == 2) { // aerial?
                if (cancellable == 0x10 || moveID == 0x10 || moveID == 0x11) { // if in free frames, post trick, airhike
                    InputStates::PlayRose();
                }
                if (cancellable == 0x30) { // if in buffer frames
                    bufferedRose = true;
                }
                roseInput = false; // in active frames
            }
            else
                roseInput = false; // grounded.
        }
    }
}

void InputStates::RoseBuffer(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    if (uLocalPlr) {
        uint8_t& grounded = *(uint8_t*)((uintptr_t)uLocalPlr + 0xEA8);
        uint8_t& cancellable = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1E15);
        // buffer
        if (bufferedRose) {
            if (grounded == 2) { // aerial?
                if (cancellable == 0x10) { // if in free frames
                    bufferedRose = false;
                    InputStates::PlayRose();
                }
                if (cancellable == 0x00) { // if another attack starts, kill the buffer
                    bufferedRose = false;
                }
            }
            else
                bufferedRose = false; // grounded.
        }
    }
}

void InputStates::PlayRose(void) {
    sMediator* sMedPtr = *(sMediator**)staticMediatorPtr;
    uPlayer* uLocalPlr = sMedPtr->playerPtr;
    uint8_t& moveBank = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1500);
    uint8_t& moveID = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1564);
    uint8_t& movePart = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1504);
    uint8_t& weight = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1E7D);
    uint8_t& weaponChangeDisable = *(uint8_t*)((uintptr_t)uLocalPlr + 0x14F0);

    uintptr_t* luciferPtr = (uintptr_t*)((uintptr_t)uLocalPlr + 0x1D98);
    uintptr_t luciferBase = *luciferPtr;
    bool& showLucifer = *(bool*)(luciferBase + 0x137C);

    showLucifer = true;
    moveBank = 12;
    moveID = 55;
    movePart = 00;
    weaponChangeDisable = 12;
    weight ++; // we miss out on a weight+1 because we miss initial call at DevilMayCry4_DX9.exe+3AAD35
    // m_timer->start();
    InputStates::inputTimer2 = 0.0f;
    InputStates::roseTimerActive = true;
}

std::optional<std::string> InputStates::onInitialize() {
    if (!install_hook_offset(0x3B0844, hook, &detour, &jmp_return, 6)) { // ActiveBlock
        HL_LOG_ERR("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    } 

    if (!install_hook_offset(0x3AA002, hook2, &detour2, &jmp_return2, 6)) { // TauntEcstasy
        HL_LOG_ERR("Failed to init InputStates2 mod\n");
        return "Failed to init InputStates2 mod";
    }

    if (!install_hook_offset(0x43600F, hook3, &detourChangingToLucifer, &jmp_return3, 5)) { // Changing To Lucifer 
        HL_LOG_ERR("Failed to init InputStates3 mod\n");
        return "Failed to init InputStates3 mod";
    }

    return Mod::onInitialize();
}

void InputStates::onConfigLoad(const utils::Config& cfg) {
    touchpadRoseEnabled = cfg.get<bool>("taunt_ectasy").value_or(false);
};

void InputStates::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("taunt_ectasy", touchpadRoseEnabled);
};

void InputStates::onGUIframe() {
    ImGui::Checkbox("Taunt Ecstasy", &touchpadRoseEnabled);
    ImGui::SameLine();
    HelpMarker("Pressing Taunt in the air will perform Ecstasy. Consider using this with Selective Cancellable Ecstasy to make it feel more like 5");
}

#endif
