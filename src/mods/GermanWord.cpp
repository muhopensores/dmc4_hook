#include "GermanWord.hpp"
#include "../sdk/Devil4.hpp"

bool GermanWord::mod_enabled = false;
// uintptr_t GermanWord::jmp_ret1 = NULL;

static constexpr uintptr_t danteSpawnAddr = 0x7B2130;
static uPlayer* danteSpawnedAddr          = nullptr;

static constexpr uintptr_t neroSpawnAddr  = 0x7E1A70;
static uPlayer* neroSpawnedAddr             = nullptr;

static bool spawn_ai_dante_toggle = false;
static bool spawn_ai_nero_toggle = false;

// let players push players
void GermanWord::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x10C6B9, patch1, "\x8B\x91\xE0\x02\x00\x00", 6); // mVsAttrPlPush to mVsAttrEmPush - mov edx,[ecx+000002E0] (0x44444444)
    }
    else {
        patch1.reset(); // mVsAttrPlPush - mov edx,[ecx+000002B8] (0x44444400)
    }
}

// comboing these with other edits let you change doppel to be the ai chara but I do not remember the sick pause combo to do it (you literally have to pause)
void GermanWord::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x838078, patch2, "\xFB\xFB\x30\x06", 4);
    } else {
        install_patch_offset(0x838078, patch2, "\xF8\xF0\x1D\x00", 4);
        patch2.reset();
    }
}

void GermanWord::toggle3(bool enable) {
    if (enable) {
        install_patch_offset(0x838078, patch3, "\xFB\xFB\x30\x05", 4);
    } else {
        install_patch_offset(0x838078, patch3, "\xF8\xF0\x1D\x00", 4);
        // patch3.reset();
    }
}


/*naked void detour1() {
    _asm {
            cmp byte ptr [GermanWord::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [GermanWord::jmp_ret1]
    }
}*/

// void GermanWord::on_frame(fmilliseconds& dt) {}

static constexpr uintptr_t some_struct            = 0x00E552CC;
static constexpr uintptr_t fptr_update_actor_list = 0x008DC540;
static void spawn_dante() {
    if (!devil4_sdk::get_local_player()) { return; }
    __asm {
		pushad
		pushfd
        call dword ptr [danteSpawnAddr]
        mov [danteSpawnedAddr], eax
        mov esi, eax
        mov eax, [some_struct]
        mov eax, [eax]
        push 13 // players
        call fptr_update_actor_list
		popfd
		popad
    }
}

static void spawn_nero() {
    if (!devil4_sdk::get_local_player()) { return; }
    __asm {
		pushad
		pushfd
        call dword ptr [neroSpawnAddr]
        mov [neroSpawnedAddr], eax
        mov esi, eax
        mov eax, [some_struct]
        mov eax, [eax]
        push 13 // players
        call fptr_update_actor_list
		popfd
		popad
    }
}

void GermanWord::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Players Push Players##GermanWord"), &mod_enabled)) {
            toggle1(mod_enabled);
        }

        if (ImGui::Button(_("Spawn Dante##GermanWord"))) {
            spawn_dante();
        }
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Become AI Dante##GermanWord"), &spawn_ai_dante_toggle)) {
            toggle2(spawn_ai_dante_toggle);
        }
        ImGui::SameLine();
        help_marker("Tick then untick");

        if (ImGui::Button(_("Spawn Nero##GermanWord"))) {
            spawn_nero();
        }
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Become AI Nero##GermanWord"), &spawn_ai_nero_toggle)) {
            toggle3(spawn_ai_nero_toggle);
        }
        ImGui::SameLine();
        help_marker("Tick then untick");

        if (ImGui::Button(_("Destroy Last Spawned Dante (Crashy)##GermanWord"))) {
            if (danteSpawnedAddr) {
                // insert other projectiles here
                for (int i = 0; i < 15; i++) {
                    if (danteSpawnedAddr->luciferPins[i]) {
                        uactor_sdk::despawn(danteSpawnedAddr->luciferPins[i]);
                    }
                }
                uactor_sdk::despawn(danteSpawnedAddr);
                danteSpawnedAddr = nullptr;
            }
        }
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Button(_("Destroy Last Spawned Nero (Crashy)##GermanWord"))) {
            if (neroSpawnedAddr) {
                // insert nero stand + projectiles here
                uactor_sdk::despawn(neroSpawnedAddr);
                neroSpawnedAddr = nullptr;
            }
        }
    }
}

// void GermanWord::on_game_pause(bool toggle) {}
// bool GermanWord::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GermanWord::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init GermanWord mod 1\n");
        return "Failed to init GermanWord mod 1";
	}*/

    return Mod::on_initialize();
}

void GermanWord::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("GermanWord").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void GermanWord::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("GermanWord", mod_enabled);
}
