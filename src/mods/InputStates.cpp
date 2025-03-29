#include "InputStates.hpp"
#include "ActiveBlock.hpp"
#include "ForceLucifer.hpp" // used to stop rose despawning when leaving lucifer
#include "FasterFastDrive.hpp" // for easy fast drive
#include "utility/MessageDisplay.hpp"
#if 1

uintptr_t InputStates::jmp_return{ NULL };
uintptr_t InputStates::jmp_return2{ NULL };
uintptr_t InputStates::jmp_return3{ NULL };

uint32_t InputStates::inputpressed{ 0 };
float InputStates::input_timer{ 0.0f };
float InputStates::input_timer2{ 0.0f };

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

bool InputStates::touchpad_rose_enabled{ false };
bool InputStates::rose_timer_active{ false };

static bool b_rose_input{ false };
static bool b_buffered_rose{ false };

constexpr uintptr_t change_to_lucifer_call = 0x00836190;

void easy_fast_drive_check(void) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        if (player->inputPress[0] & 0x1 && player->inputHold[1] & 0x6 && player->controllerID == 0 && player->moveIDBest == 0x232 && player->animFrame < 7.0f) {
            player->meleeCancelPermissions1[3] = (uint8_t)0x10;
        }
    }
}

naked void detour() { // inputpressed // inputs are edx // player is in edi // ActiveBlock & touchpad ectasy, called on tick
    _asm {
        push eax
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp eax,edi
        pop eax
        jne code

        mov [InputStates::inputpressed], edx

        cmp byte ptr [ActiveBlock::mod_enabled], 1
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
        mov dword ptr [InputStates::input_timer], 0
        jmp TouchpadRoseCheck

    inctimer: // timer for ActiveBlock
        movss xmm0, [InputStates::input_timer]
        addss xmm0, [edi+0x10] // add delta to existing timer
        movss [InputStates::input_timer], xmm0

    TouchpadRoseCheck:
        cmp byte ptr [InputStates::touchpad_rose_enabled], 0
        je code
        push ecx
        push eax
        push edx
        call InputStates::rose_buffer
        pop edx
        pop eax
        pop ecx
        cmp byte ptr [InputStates::rose_timer_active], 1
        je IncRoseTimer
        jmp code

    IncRoseTimer: // timer for HideLucifer
        movss xmm0, [InputStates::input_timer2]
        addss xmm0, [edi+0x10]
        movss [InputStates::input_timer2], xmm0
        cmp dword ptr [InputStates::input_timer2], 0x43480000 // 200.0f
        jl code
        mov byte ptr [InputStates::rose_timer_active], 0
        push ecx
        push eax
        push edx
        call InputStates::on_timer_callback
        pop edx
        pop eax
        pop ecx

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
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp eax,edx
        pop eax
        jne jmpret

        cmp dword ptr [edx+0x1494], 0 // dante controller id
        jne jmpret

        cmp byte ptr [FasterFastDrive::easy_fast_drive_enabled], 1
        jne check2
        push ecx
        push eax
        push edx
        call easy_fast_drive_check
        pop edx
        pop eax
        pop ecx
    check2:
        cmp byte ptr [InputStates::touchpad_rose_enabled], 0
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
        mov byte ptr [b_rose_input], 1
        push ecx
        push eax
        push edx
        call InputStates::rose_input
        pop edx
        pop eax
        pop ecx
        jmp jmpret
    }
}

naked void detour_changing_to_lucifer() {
    _asm {
        cmp byte ptr [InputStates::rose_timer_active], 1
        jne code
        jmp dword ptr [InputStates::jmp_return3]

    code:
        call [change_to_lucifer_call]
		jmp dword ptr [InputStates::jmp_return3]
    }
}

void InputStates::on_timer_callback() { // hide lucifer after rose if weaponid is not lucifer
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        if (player->lucifer->visible == true && player->currentSword != 6 && player->controllerID == 0) {
            if (!ForceLucifer::mod_enabled) {
                player->lucifer->visible = false;
            }
        }
    }
}

void InputStates::rose_input(void) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        if (b_rose_input) { // if touchpad is pressed
            if (player->grounded == 2) {
                if (player->jumpCancelPermissions1[1] == 0x10 || player->moveIDBest == 0x10 || player->moveIDBest == 0x11) { // if (in free frames, post trick, airhike)
                    InputStates::play_rose();
                }
                if (player->jumpCancelPermissions1[1] == 0x30) { // if in buffer frames
                    b_buffered_rose = true;
                }
                b_rose_input = false; // in active frames
            } else
                b_rose_input = false; // grounded.
        }
    }
}

void InputStates::rose_buffer(void) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        if (b_buffered_rose) {
            if (player->grounded == 2) { // aerial?
                if (player->jumpCancelPermissions1[1] == 0x10) { // if in free frames
                    b_buffered_rose = false;
                    InputStates::play_rose();
                }
                if (player->jumpCancelPermissions1[1] == 0x00) { // if another attack starts, kill the buffer
                    b_buffered_rose = false;
                }
            } else
                b_buffered_rose = false; // grounded.
        }
    }
}

void InputStates::play_rose(void) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        player->lucifer->visible = true;
        player->moveBank         = 12;
        player->moveID2          = 55;
        player->movePart         = 00;
        player->cancels1[0]      = 0;
        player->cancels1[1]      = 0;
        player->cancels1[2]      = 0;
        player->cancels1[3]      = 0;
        player->canWeaponChange  = 12;
        player->weight++; // we miss out on a weight+1 because we miss initial call at DevilMayCry4_DX9.exe+3AAD35
        InputStates::input_timer2      = 0.0f;
        InputStates::rose_timer_active = true;
    }
}

std::optional<std::string> InputStates::on_initialize() {
    if (!install_hook_offset(0x3B0844, hook, &detour, &jmp_return, 6)) { // ActiveBlock
        spdlog::error("Failed to init InputStates mod\n");
        return "Failed to init InputStates mod";
    } 

    if (!install_hook_offset(0x3AA002, hook2, &detour2, &jmp_return2, 6)) { // TauntEcstasy
        spdlog::error("Failed to init InputStates2 mod\n");
        return "Failed to init InputStates2 mod";
    }
        
    if (!install_hook_offset(0x43600F, hook3, &detour_changing_to_lucifer, &jmp_return3, 5)) { // Changing To Lucifer 
        spdlog::error("Failed to init InputStates3 mod\n");
        return "Failed to init InputStates3 mod";
    }

    return Mod::on_initialize();
}

void InputStates::on_config_load(const utility::Config& cfg) {
    touchpad_rose_enabled = cfg.get<bool>("taunt_ectasy").value_or(false);
}

void InputStates::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("taunt_ectasy", touchpad_rose_enabled);
}

void InputStates::on_gui_frame() {
    ImGui::Checkbox(_("Taunt Ecstasy"), &touchpad_rose_enabled);
    ImGui::SameLine();
    help_marker(_("Pressing Taunt in the air will perform Ecstasy. Consider using this with Selective Cancellable Ecstasy to make it feel more like 5"));
}

#endif
