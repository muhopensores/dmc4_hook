#include "ManualLuciferAim.hpp"

bool ManualLuciferAim::mod_enabled = false;
uintptr_t ManualLuciferAim::jmp_ret1 = NULL;
uintptr_t ManualLuciferAim::jmp_ret2 = NULL;
uintptr_t ManualLuciferAim::jmp_ret3 = NULL;
uintptr_t ManualLuciferAim::jmp_ret4 = NULL;

naked void detour1() {
    _asm {
            cmp byte ptr [ManualLuciferAim::mod_enabled], 1
            jne originalcode

            mov ebp,esi
            fld dword ptr ds:[0xC0F1C4]
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
            // mov ecx,esi
            jmp dword ptr [ManualLuciferAim::jmp_ret1]

        originalcode:
            push ebx
            mov ecx,esi
            call eax
            jmp dword ptr [ManualLuciferAim::jmp_ret1]
    }
}
naked void detour2() {
    _asm {
            cmp byte ptr [ManualLuciferAim::mod_enabled], 1
            jne originalcode

            mov ebp,esi
            fld dword ptr ds:[0xC0F1C4]
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
            // mov ecx, esi
            jmp dword ptr [ManualLuciferAim::jmp_ret2]

        originalcode:
            push ebx
            mov ecx, esi
            call eax
            jmp dword ptr [ManualLuciferAim::jmp_ret2]
    }
}
naked void detour3() {
    _asm {
            cmp byte ptr [ManualLuciferAim::mod_enabled], 1
            jne originalcode

            fld dword ptr ds:[0xC0F1C4]
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
            // mov ecx, ebp
            jmp dword ptr [ManualLuciferAim::jmp_ret3]

        originalcode:
            push ebx
            mov ecx, ebp
            call eax
            jmp dword ptr [ManualLuciferAim::jmp_ret3]
    }
}
naked void detour4() {
    _asm {
            cmp byte ptr [ManualLuciferAim::mod_enabled], 1
            jne originalcode

            fld dword ptr ds:[0xC0F1C4]
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
            // mov ecx, ebp
            jmp dword ptr [ManualLuciferAim::jmp_ret4]

        originalcode:
            push 01
            mov ecx, ebp
            call edx
            jmp dword ptr [ManualLuciferAim::jmp_ret4]
    }
}

// void on_frame(fmilliseconds& dt) {}

void ManualLuciferAim::on_gui_frame(int display) {
    ImGui::Checkbox(_("Manual Lucifer Aim"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Replace the auto aim on Lucifer A2, A4, E2, E3 with what most other moves use"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ManualLuciferAim::on_initialize() {
    if (!install_hook_offset(0x3D602D, hook1, &detour1, &jmp_ret1, 5)) { // A2
        spdlog::error("Failed to init ManualLuciferAim mod 1\n");
        return "Failed to init ManualLuciferAim mod 1";
	}
    if (!install_hook_offset(0x3D641A, hook2, &detour2, &jmp_ret2, 5)) { // A4
        spdlog::error("Failed to init ManualLuciferAim mod 2\n");
        return "Failed to init ManualLuciferAim mod 2";
	}
    if (!install_hook_offset(0x3D689B, hook3, &detour3, &jmp_ret3, 5)) { // E2
        spdlog::error("Failed to init ManualLuciferAim mod 3\n");
        return "Failed to init ManualLuciferAim mod 3";
	}
    if (!install_hook_offset(0x3D6BA7, hook4, &detour4, &jmp_ret4, 6)) { // E3
        spdlog::error("Failed to init ManualLuciferAim mod 4\n");
        return "Failed to init ManualLuciferAim mod 4";
	}

    return Mod::on_initialize();
}

void ManualLuciferAim::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ManualLuciferAim").value_or(false);
}

void ManualLuciferAim::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ManualLuciferAim", mod_enabled);
}
