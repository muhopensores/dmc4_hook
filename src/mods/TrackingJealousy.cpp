#include "TrackingJealousy.hpp"

bool TrackingJealousy::mod_enabled = false;
uintptr_t TrackingJealousy::jmp_ret1 = NULL;

naked void detour1() { // player in ebp
    _asm {
            cmp byte ptr [TrackingJealousy::mod_enabled], 1
            jne originalcode

            test byte ptr [ebp+0x140C], 0x10 // is holding lock on
            je originalcode

            // not necessary but let's play it safe
            push eax
            push ecx
            push edx

            fld dword ptr ds:[0x00C0F1C4]
            mov eax, [ebp+00]
            mov edx, [eax+0x000002B4]
            push 00
            push ecx
            fstp dword ptr [esp]
            push 00
            push 00
            push 00
            push 01
            mov ecx, ebp
            mov byte ptr [ebp+0x00001F50], 01
            call edx
            xorps xmm0,xmm0

            pop edx
            pop ecx
            pop eax

        originalcode:
            mov eax,[ebp+0x000028A4]
            jmp dword ptr [TrackingJealousy::jmp_ret1]
    }
}

// void on_frame(fmilliseconds& dt) {}

void TrackingJealousy::on_gui_frame(int display) {
    ImGui::Checkbox(_("Tracking Jealousy"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Jealousy tracks horizontally"));
}

std::optional<std::string> TrackingJealousy::on_initialize() {
    if (!install_hook_offset(0x3DDF40, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init TrackingJealousy mod 1\n");
        return "Failed to init TrackingJealousy mod 1";
	}

    return Mod::on_initialize();
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void TrackingJealousy::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("TrackingJealousy").value_or(false);
}

void TrackingJealousy::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("TrackingJealousy", mod_enabled);
}
