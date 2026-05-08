#include "FpsFixes.hpp"

bool FpsFixes::mod_enabled = false;
uintptr_t FpsFixes::jmp_ret1 = NULL;

void FpsFixes::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x3CB345, patch1, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 24); // trick inertia
    }
    else {
        patch1.reset(); // trick inertia // writes to inertia xyz // for some reason noping this fixes the issue
        // movss [esi+00001E1C],xmm1
        // movss [esi+00001E20],xmm0
        // movss [esi+0000177C],xmm0
    }
}

/*naked void detour1() {
    _asm {
            cmp byte ptr [FpsFixes::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [FpsFixes::jmp_ret1]
    }
}*/

// void FpsFixes::on_frame(fmilliseconds& dt) {}

void FpsFixes::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("High FPS Fixes"), &mod_enabled)) {
            toggle1(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("The goal of this mod is to make variable fps play more like 60fps.\nCurrently only inertia after trick is affected."));
    }
}

// void FpsFixes::on_game_pause(bool toggle) {}
// bool FpsFixes::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FpsFixes::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init FpsFixes mod 1\n");
        return "Failed to init FpsFixes mod 1";
	}*/

    return Mod::on_initialize();
}

void FpsFixes::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FpsFixes").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void FpsFixes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FpsFixes", mod_enabled);
}
