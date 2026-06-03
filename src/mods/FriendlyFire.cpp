#include "FriendlyFire.hpp"
#include "../sdk/Devil4.hpp"
bool FriendlyFire::mod_enabled = false;
uintptr_t FriendlyFire::jmp_ret1 = NULL;

void FriendlyFire::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x10C690, patch1, "\x8B\xB1\x00\x03\x00\x00", 6); // hit players 
        install_patch_offset(0x10C69B, patch2, "\x8B\x91\x10\x03\x00\x00", 6); // idk 
        install_patch_offset(0x10C6AB, patch3, "\x8B\x91\x04\x03\x00\x00", 6); // not changing means I can't damage anything 
        install_patch_offset(0x10C6E4, patch4, "\x8B\xB1\x00\x03\x00\x00", 6); // not changing means I can't shoot players 
        install_patch_offset(0x10C6FF, patch5, "\x8B\x91\x04\x03\x00\x00", 6); // idk 
    }
    else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
    }
}

static constexpr uintptr_t danteSpawnAddr         = 0x7B2130;
static constexpr uintptr_t some_struct            = 0x00E552CC;
static constexpr uintptr_t fptr_update_actor_list = 0x008DC540;
static uintptr_t doppelAddr                       = NULL;
void spawn_dante() {
    if (!devil4_sdk::get_local_player())
        return;
    __asm {
		pushad
		pushfd
        call dword ptr [danteSpawnAddr]
        mov [doppelAddr], eax
        mov esi, eax
        mov eax, [some_struct]
        mov eax, [eax]
        push 0x0F
        call fptr_update_actor_list
		popfd
		popad
    }
}

/*naked void detour1() {
    _asm {
            cmp byte ptr [FriendlyFire::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [FriendlyFire::jmp_ret1]
    }
}*/

// void FriendlyFire::on_frame(fmilliseconds& dt) {}

void FriendlyFire::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("FriendlyFire"), &mod_enabled)) {
            toggle1(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("FriendlyFireHelpMarker"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::Button("spawn extra dante")) {
                spawn_dante();
            }
            ImGui::Unindent(lineIndent);
        }
    }
}

// void FriendlyFire::on_game_pause(bool toggle) {}
// bool FriendlyFire::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FriendlyFire::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init FriendlyFire mod 1\n");
        return "Failed to init FriendlyFire mod 1";
	}*/

    return Mod::on_initialize();
}

void FriendlyFire::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FriendlyFire").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void FriendlyFire::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FriendlyFire", mod_enabled);
}
