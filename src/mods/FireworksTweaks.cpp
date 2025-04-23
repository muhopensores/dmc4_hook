#include "FireworksTweaks.hpp"

#if 1
bool FireworksTweaks::mod_enabled = false;
uintptr_t FireworksTweaks::jmp_ret1 = NULL;
uintptr_t FireworksTweaks::jmp_ret2 = NULL;

float FireworksTweaks::fireworksStartFrame = 5.0f;

naked void detour1() { // Edit arguments before player animation call
    _asm {
            cmp byte ptr [FireworksTweaks::mod_enabled], 1
            jne originalcode

            fld dword ptr [FireworksTweaks::fireworksStartFrame]
            jmp cont

        originalcode:
            fldz
        cont:
            mov ecx,ebx
            fstp dword ptr [esp+0x04]
            jmp dword ptr [FireworksTweaks::jmp_ret1]
    }
}

naked void detour2() { // Edit arguments before gun animation call
    _asm {
            cmp byte ptr [FireworksTweaks::mod_enabled], 1
            jne originalcode
            
            // we use edx as it gets xor'd anyway
            mov edx,[ecx+0x1e8c] // characterSettingsOne
            test edx,edx
            je originalcode
            cmp [edx+0x1c], 1 // groundedActual
            jne originalcode // don't replace start frame if aerial

            fld dword ptr [FireworksTweaks::fireworksStartFrame]
        originalcode:
            fstp dword ptr [esp]
            push esi
            xor edx, edx
            jmp dword ptr [FireworksTweaks::jmp_ret2]
    }
}

std::optional<std::string> FireworksTweaks::on_initialize() {
    if (!install_hook_offset(0x3DA475, hook1, &detour1, &jmp_ret1, 8)) { // Edit arguments before player animation call
        spdlog::error("Failed to init FireworksTweaks mod 1\n");
        return "Failed to init FireworksTweaks mod 1";
	}
    if (!install_hook_offset(0x432267, hook2, &detour2, &jmp_ret2, 6)) { // Edit arguments before gun animation call
        spdlog::error("Failed to init FireworksTweaks mod 2\n");
        return "Failed to init FireworksTweaks mod 2";
	}

    return Mod::on_initialize();
}

void FireworksTweaks::on_gui_frame(int display) {
    ImGui::Checkbox(_("Fireworks Tweaks"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Edits the start frame, cancel frame and cancel properties of grounded Fireworks"));
}

// void on_frame(fmilliseconds& dt) {}

// void on_game_pause(bool toggle) {}

// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void FireworksTweaks::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FireworksTweaks").value_or(false);
}

void FireworksTweaks::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FireworksTweaks", mod_enabled);
}
#endif
