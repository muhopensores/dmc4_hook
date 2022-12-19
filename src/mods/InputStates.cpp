#include "InputStates.hpp"
#include "DeltaTime.hpp"
#include "ActiveBlock.hpp"
#include "MoveIDs.hpp"
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
float timer_tick{ 1.0f };

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

bool InputStates::touchpad_rose_enabled{ false };
bool InputStates::rose_timer_active{ false };

static bool b_rose_input{ false };
static bool b_buffered_rose{ false };

constexpr uintptr_t change_to_lucifer_call = 0x00836190;

void easy_fast_drive_check(void) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    uintptr_t* player_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x24);
    uintptr_t player_base = *player_ptr;
    if (player_base) {
        int8_t& controller_id = *(int8_t*)(player_base + 0x1494);
        int& move_id = *(int*)(player_base + 0x2998);
        int8_t& melee_input_on_press = *(int8_t*)(player_base + 0x1410);
        int8_t& style_input_on_hold = *(int8_t*)(player_base + 0x140D);
        int8_t& cancel_to_melee = *(int8_t*)(player_base + 0x1E13);
        float& animation_frame = *(float*)(player_base + 0x348);
        if (melee_input_on_press & 0x1 && style_input_on_hold & 0x6 && controller_id == 0 && move_id == 0x232 && animation_frame < 7.0f) {
            cancel_to_melee = 0x10;
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
        movss xmm1, [timer_tick]
        mulss xmm1, [DeltaTime::current_delta_time]
        addss xmm0, xmm1
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
        movss xmm1, [timer_tick]
        mulss xmm1, [DeltaTime::current_delta_time]
        addss xmm0, xmm1
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
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    uPlayer* u_local_plr = s_med_ptr->player_ptr;
    if (u_local_plr) {
        uPlayer* u_local_plr = s_med_ptr->player_ptr;
        // uint8_t& weaponID = *(uint8_t*)((uintptr_t)uLocalPlr + 0x1DB4);
        uint8_t& equipped_weapon = *(uint8_t*)((uintptr_t)u_local_plr + 0x1DC0); // might fix a small bug might break everything
        uintptr_t* lucifer_ptr = (uintptr_t*)((uintptr_t)u_local_plr + 0x1D98);
        uintptr_t lucifer_base = *lucifer_ptr;
        bool& show_lucifer = *(bool*)(lucifer_base + 0x137C);
        if (show_lucifer == true && equipped_weapon != 6) {
            if (!ForceLucifer::mod_enabled){
                    show_lucifer = false;
            }
        }
    }
}

void InputStates::rose_input(void) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    uPlayer* u_local_plr = s_med_ptr->player_ptr;
    if (u_local_plr) {
        uint8_t& grounded = *(uint8_t*)((uintptr_t)u_local_plr + 0xEA8);
        uint8_t& cancellable = *(uint8_t*)((uintptr_t)u_local_plr + 0x1E15);
        int& move_id = *(int*)((uintptr_t)u_local_plr + 0x2998); // i hate myself
        // input
        if (b_rose_input) { // if touchpad is pressed
            if (grounded == 2) { // aerial?
                if (cancellable == 0x10 || move_id == 0x10 || move_id == 0x11) { // if in free frames, post trick, airhike
                    InputStates::play_rose();
                }
                if (cancellable == 0x30) { // if in buffer frames
                    b_buffered_rose = true;
                }
                b_rose_input = false; // in active frames
            }
            else
                b_rose_input = false; // grounded.
        }
    }
}

void InputStates::rose_buffer(void) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    uPlayer* u_local_plr = s_med_ptr->player_ptr;
    if (u_local_plr) {
        uint8_t& grounded = *(uint8_t*)((uintptr_t)u_local_plr + 0xEA8);
        uint8_t& cancellable = *(uint8_t*)((uintptr_t)u_local_plr + 0x1E15);
        // buffer
        if (b_buffered_rose) {
            if (grounded == 2) { // aerial?
                if (cancellable == 0x10) { // if in free frames
                    b_buffered_rose = false;
                    InputStates::play_rose();
                }
                if (cancellable == 0x00) { // if another attack starts, kill the buffer
                    b_buffered_rose = false;
                }
            }
            else
                b_buffered_rose = false; // grounded.
        }
    }
}

void InputStates::play_rose(void) {
    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    uPlayer* u_local_plr = s_med_ptr->player_ptr;
    uint8_t& move_bank = *(uint8_t*)((uintptr_t)u_local_plr + 0x1500);
    uint8_t& move_id = *(uint8_t*)((uintptr_t)u_local_plr + 0x1564);
    uint8_t& move_part = *(uint8_t*)((uintptr_t)u_local_plr + 0x1504);
    uint8_t& weight = *(uint8_t*)((uintptr_t)u_local_plr + 0x1E7D);
    uint8_t& weapon_change_disable = *(uint8_t*)((uintptr_t)u_local_plr + 0x14F0);

    uintptr_t* lucifer_ptr = (uintptr_t*)((uintptr_t)u_local_plr + 0x1D98);
    uintptr_t lucifer_base = *lucifer_ptr;
    bool& show_lucifer = *(bool*)(lucifer_base + 0x137C);

    show_lucifer = true;
    move_bank = 12;
    move_id = 55;
    move_part = 00;
    weapon_change_disable = 12;
    weight ++; // we miss out on a weight+1 because we miss initial call at DevilMayCry4_DX9.exe+3AAD35
    // m_timer->start();
    InputStates::input_timer2 = 0.0f;
    InputStates::rose_timer_active = true;
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
    ImGui::Checkbox("Taunt Ecstasy", &touchpad_rose_enabled);
    ImGui::SameLine();
    help_marker("Pressing Taunt in the air will perform Ecstasy. Consider using this with Selective Cancellable Ecstasy to make it feel more like 5");
}

#endif
