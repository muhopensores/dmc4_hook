#include "ReversableDraw.hpp"

bool ReversableDraw::mod_enabled = false;
uintptr_t ReversableDraw::jmp_ret1 = NULL;

naked void detour1() {
    _asm {
            cmp byte ptr [ReversableDraw::mod_enabled], 1
            jne originalcode

            push 00
            jmp cont

        originalcode:
            push ebx // holds 1
        cont:
            push ebx
            mov ecx,edi
            call eax
            jmp dword ptr [ReversableDraw::jmp_ret1]
    }
}

// void on_frame(fmilliseconds& dt) {}

void ReversableDraw::on_gui_frame(int display) {
    ImGui::Checkbox(_("Reversable Draw"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Enable reversals on draw"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ReversableDraw::on_initialize() {
    if (!install_hook_offset(0x3D3B7F, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init ReversableDraw mod 1\n");
        return "Failed to init ReversableDraw mod 1";
	}

    return Mod::on_initialize();
}

void ReversableDraw::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ReversableDraw").value_or(false);
}

void ReversableDraw::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ReversableDraw", mod_enabled);
}
