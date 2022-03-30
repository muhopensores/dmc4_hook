#include "modInputStates.hpp"
#include "modDeltaTime.hpp"
#include "modActiveBlock.hpp"
#include "modMoveIDs.hpp"
#include "modForceLucifer.hpp"
#include "../utils/MessageDisplay.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uintptr_t InputStates::jmp_return2{ NULL };
uint32_t InputStates::inputpressed{ 0 };
float InputStates::inputTimer{ 0.0f };
float InputStates::inputTimer2{ 0.0f };
float TimerTick{ 1.0f };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
bool InputStates::touchpadRoseEnabled{ false };
bool roseInput{ false };
bool bufferedRose{ false };
bool roseTimerActive{ false };
bool enabledNoDespawnChangeToLucifer = false;
bool enabledNoDespawnEnemy = false;
bool enabledNoDespawnObject = false;
bool roseInfiniteTimer = false;

naked void detour() { // inputpressed // inputs are edx // ActiveBlock & touchpad ectasy, called on tick
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
        je ActiveBlockSetup
        jmp TouchpadRoseCheck

    ActiveBlockSetup:
        push eax
        push edx
        mov eax, 0x06
        mov dl, byte ptr [InputStates::inputpressed+1] // edx
        test al, dl // ActiveBlock
        jnz inctimer
        mov dword ptr [InputStates::inputTimer], 0
        pop edx
        pop eax
        jmp TouchpadRoseCheck

    inctimer: // timer for ActiveBlock
        pop edx
        pop eax
        movss xmm0, [InputStates::inputTimer]
        movss xmm1, [TimerTick]
        mulss xmm1, [DeltaTime::currentDeltaTime]
        addss xmm0, xmm1
        movss [InputStates::inputTimer], xmm0

    TouchpadRoseCheck:
        cmp byte ptr [InputStates::touchpadRoseEnabled], 0
        je code
        pushad
        call InputStates::RoseBuffer
        popad
        cmp byte ptr [roseTimerActive], 1
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
        mov byte ptr [roseTimerActive], 0
        pushad
        call InputStates::onTimerCallback
        popad

    code:
        cmp [eax+0x41], cl
        mov [eax+0x10], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

naked void detour2() { // inputonpress // touchpad ecstasy // player is in edx // called on button press
    _asm {
        mov [edx+0x00001410], eax // originalcode
        cmp byte ptr [InputStates::touchpadRoseEnabled], 0
        je jmpret

        push ecx
        mov ecx, [staticMediatorPtr] // only get player inputs (thanks boss dante)
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp edx, ecx
        pop ecx
        jne jmpret // if not player, jump
        cmp dword ptr [edx+0x1494], 0 // playerid hopefully
        jne jmpret

    // checkrose
        push edx
        mov edx, 0x8 // edx has desired input
        test al, dl
        pop edx
        jnz rosethrow // only send ecstasy input on press rather than hold
    jmpret:
		jmp dword ptr [InputStates::jmp_return2]

    rosethrow:
        mov byte ptr [roseInput], 1
        pushad
        call InputStates::RoseInput
        popad
        jmp jmpret
    }
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
    return Mod::onInitialize();
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
        if (showLucifer == true && equippedWeapon != 6 && !ForceLucifer::modEnabled) {
            showLucifer = false;
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
    uint8_t& weaponChangeDisable = *(uint8_t*)((uintptr_t)uLocalPlr + 0x14F0);

    uintptr_t* luciferPtr = (uintptr_t*)((uintptr_t)uLocalPlr + 0x1D98);
    uintptr_t luciferBase = *luciferPtr;
    bool& showLucifer = *(bool*)(luciferBase + 0x137C);

    showLucifer = true;
    moveBank = 12;
    moveID = 55;
    movePart = 00;
    weaponChangeDisable = 12;
    // m_timer->start();
    InputStates::inputTimer2 = 0.0f;
    roseTimerActive = true;
}

void InputStates::toggleDisableRoseDespawnOnChangingToLucifer(bool enabled) {
    if (enabled) {
        install_patch_offset(0x043600F, patch1, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.revert();
    }
}

void InputStates::toggleDisableRoseDespawnOnHittingEnemy(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435944, patch2, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch2.revert();
    }
}

void InputStates::toggleDisableRoseDespawnOnHittingObject(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435922, patch3, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch3.revert();
    }
}

void InputStates::toggleRoseInfiniteTimer(bool enabled)
{
    if (enabled)
    {
        install_patch_offset(0x0435937, patch4, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else
    {
        patch4.revert();
    }
}

void InputStates::onConfigLoad(const utils::Config& cfg) {
    touchpadRoseEnabled = cfg.get<bool>("taunt_ectasy").value_or(false);
    enabledNoDespawnChangeToLucifer = cfg.get<bool>("no_despawn_rose_lucifer").value_or(false);
    toggleDisableRoseDespawnOnChangingToLucifer(enabledNoDespawnChangeToLucifer);
    enabledNoDespawnEnemy = cfg.get<bool>("no_despawn_rose_enemy").value_or(false);
    toggleDisableRoseDespawnOnHittingEnemy(enabledNoDespawnEnemy);
    enabledNoDespawnObject = cfg.get<bool>("no_despawn_rose_object").value_or(false);
    toggleDisableRoseDespawnOnHittingObject(enabledNoDespawnObject);
    roseInfiniteTimer = cfg.get<bool>("rose_infinite_timer").value_or(false);
    toggleRoseInfiniteTimer(roseInfiniteTimer);
};

void InputStates::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("taunt_ectasy", touchpadRoseEnabled);
    cfg.set<bool>("no_despawn_rose_lucifer", enabledNoDespawnChangeToLucifer);
    cfg.set<bool>("no_despawn_rose_enemy", enabledNoDespawnEnemy);
    cfg.set<bool>("no_despawn_rose_object", enabledNoDespawnObject);
    cfg.set<bool>("rose_infinite_timer", roseInfiniteTimer);
};

void InputStates::onGUIframe() {
    ImGui::Checkbox("Taunt Ecstasy", &touchpadRoseEnabled);
    ImGui::SameLine();
    HelpMarker("Pressing Taunt in the air will perform Ecstasy. Consider using this with Selective Cancellable Ecstasy to make it feel more like 5");
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Rose Survives Lucifer", &enabledNoDespawnChangeToLucifer)) {
        toggleDisableRoseDespawnOnChangingToLucifer(enabledNoDespawnChangeToLucifer);
    }
    ImGui::SameLine();
    HelpMarker("Rose will not get destroyed when swapping to Lucifer");

    if (ImGui::Checkbox("Rose Survives Enemies", &enabledNoDespawnEnemy)) {
        toggleDisableRoseDespawnOnHittingEnemy(enabledNoDespawnEnemy);
    }
    ImGui::SameLine();
    HelpMarker("Rose will hit the enemy and then continue past the enemy");
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Rose Survives Objects", &enabledNoDespawnObject)) {
        toggleDisableRoseDespawnOnHittingObject(enabledNoDespawnObject);
    }
    ImGui::SameLine();
    HelpMarker("Rose will sit on whatever surface it lands on until its timer expires");

    if (ImGui::Checkbox("Infinite Rose Timer", &roseInfiniteTimer)) {
        toggleRoseInfiniteTimer(roseInfiniteTimer);
    }
    ImGui::SameLine();
    HelpMarker("Rose will only despawn when colliding with something or having another placed");
}

#endif
