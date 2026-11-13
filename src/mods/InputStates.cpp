#include "InputStates.hpp"
#include "ActiveBlock.hpp"
#include "ForceLucifer.hpp" // used to stop rose despawning when leaving lucifer
#include "FasterFastDrive.hpp" // for easy fast drive
#include "DarkSoulsStamina.hpp" // stamina
#include "../sdk/Devil4.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/Players.hpp"
#if 1

uintptr_t InputStates::jmp_return = NULL;
uintptr_t InputStates::jmp_return2 = NULL;
uintptr_t InputStates::jmp_return3 = NULL;

uint32_t InputStates::inputpressed = 0;
float InputStates::input_timer = 0.0f;

bool InputStates::touchpad_rose_enabled = false;

namespace {
struct Rose {
    bool input    = false;   // Taunt pressed, not yet played or buffered
    bool buffered = false;   // pressed in the buffer frames: plays at the next free frame
    bool active   = false;   // the hide timer runs
    float timer   = 0.0f;
};
Rose roses[4];               // per player_info index
uint8_t rose_flag = 0;       // a hook's answer from rose_active

Rose* rose_of(uintptr_t player) {
    const int i = player_info::index_of(player);
    return i >= 0 && i < 4 ? &roses[i] : nullptr;
}

void play_rose(uPlayer_old* player, Rose& r) {
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
    r.timer  = 0.0f;
    r.active = true;
}

void hide_lucifer(uPlayer_old* player) { // hide lucifer after rose if weaponid is not lucifer
    if (player->lucifer->visible == true && player->currentSword != 6 && player->controllerID == 0) {
        if (!ForceLucifer::mod_enabled) {
            player->lucifer->visible = false;
        }
    }
}
} // namespace

constexpr uintptr_t change_to_lucifer_call = 0x00836190;

void easy_fast_drive_check(void) {
    uPlayer_old* player = devil4_sdk::get_local_player();
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
        pushad
        push edi
        call InputStates::rose_tick
        popad

    code:
        cmp [eax+0x41], cl
        mov [eax+0x10], edx
		jmp dword ptr [InputStates::jmp_return]
    }
}

naked void detour2() { // inputonpress // touchpad ecstasy // player is in edx // called on tick
    _asm {
        cmp byte ptr [DarkSoulsStamina::stamina_enabled], 1
        je staminaCode
        cmp byte ptr [DarkSoulsStamina::meme_stamina_enabled], 1
        je staminaCode

        originalcode:
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
        jmp jmpret

    jmpret:
		jmp dword ptr [InputStates::jmp_return2]

    rosethrow:
        pushad
        push edx
        call InputStates::rose_press
        popad
        jmp jmpret

    staminaCode: // the player's own gauge (DarkSoulsStamina::gauge_player / exhausted); not a player: no stamina
        push ecx
        xor ecx, ecx
    staminaFind:
        cmp edx, DarkSoulsStamina::gauge_player[ecx*4]
        je staminaFound
        inc ecx
        cmp ecx, 4 // DarkSoulsStamina::kSlots
        jb staminaFind
        pop ecx
        jmp originalcode
    staminaFound:
        cmp byte ptr DarkSoulsStamina::exhausted[ecx], 0
        pop ecx
        jne jmpret
        jmp originalcode
    }
}

// uPlWpLucifer::setEnable 0x83600F (esi = the Lucifer, +0x1378 its owner): no initRose while the owner's Rose runs
naked void detour_changing_to_lucifer() {
    _asm {
        pushad
        push dword ptr [esi+0x1378]
        call InputStates::rose_active
        mov [rose_flag], al
        popad
        cmp byte ptr [rose_flag], 0
        je code
        jmp dword ptr [InputStates::jmp_return3]

    code:
        call [change_to_lucifer_call]
		jmp dword ptr [InputStates::jmp_return3]
    }
}

bool __stdcall InputStates::rose_active(uintptr_t player) {
    const Rose* r = rose_of(player);
    return r && r->active;
}

void __stdcall InputStates::rose_press(uintptr_t pl) {
    Rose* r = rose_of(pl);
    if (!r) return;
    uPlayer_old* player = (uPlayer_old*)pl;
    if (player->grounded == 2) {
        if (player->jumpCancelPermissions1[1] == 0x10 || player->moveIDBest == 0x10 || player->moveIDBest == 0x11) { // if (in free frames, post trick, airhike)
            play_rose(player, *r);
        }
        if (player->jumpCancelPermissions1[1] == 0x30) { // if in buffer frames
            r->buffered = true;
        }
    }
    r->input = false; // played, buffered, in active frames or grounded
}

void __stdcall InputStates::rose_tick(uintptr_t pl) {
    Rose* r = rose_of(pl);
    if (!r) return;
    uPlayer_old* player = (uPlayer_old*)pl;
    if (r->buffered) {
        if (player->grounded == 2) { // aerial?
            if (player->jumpCancelPermissions1[1] == 0x10) { // if in free frames
                r->buffered = false;
                play_rose(player, *r);
            }
            if (player->jumpCancelPermissions1[1] == 0x00) { // if another attack starts, kill the buffer
                r->buffered = false;
            }
        } else
            r->buffered = false; // grounded.
    }
    if (r->active) { // timer for HideLucifer
        r->timer += *(float*)(pl + 0x10);   // the player's delta
        if (r->timer >= 200.0f) {
            r->active = false;
            hide_lucifer(player);
        }
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

void InputStates::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Taunt Ecstasy"), &touchpad_rose_enabled);
        ImGui::SameLine();
        help_marker(_("Press Taunt in the air to perform Ecstasy. Consider using this with Selective Cancellable Ecstasy to make it feel "
                      "more like 5"));
    }
}

#endif
