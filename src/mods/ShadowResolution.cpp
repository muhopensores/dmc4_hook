#include "ShadowResolution.hpp"
#include "../sdk/Devil4.hpp"

bool ShadowResolution::mod_enabled = false;
uintptr_t ShadowResolution::jmp_ret1 = NULL;

void ShadowResolution::toggle1(bool enable) {
    if (enable) { // reset mid gameplay
        install_patch_offset(0x67D3B9, patch1, "\x08", 1); // patch cmp eax, 400 to cmp eax, 800
        install_patch_offset(0x67D3C0, patch2, "\x08", 1); // patch mov eax, 400 to mov eax, 800
    }
    else {
        patch1.reset();
        patch2.reset();
        sUnit* sUnit = devil4_sdk::get_sUnit();
        if (!sUnit) { return; };
        if (!sUnit->shadow) { return; };
        sUnit->shadow->resolution = 0;
    }
}
/*
static uintptr_t detour1_jmp_je = 0xA7D3E3;
naked void detour1() {
    _asm {
            cmp byte ptr [ShadowResolution::mod_enabled], 1
            jne originalcode

            mov eax, 2048
        originalcode:
            cmp [esi+0x00000224], eax
            je jmp_je
            jmp dword ptr [ShadowResolution::jmp_ret1]
        jmp_je:
            jmp [detour1_jmp_je]
    }
}
*/
// void on_frame(fmilliseconds& dt) {}

void ShadowResolution::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Increased Shadow Resolution"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ShadowResolution::on_initialize() {
    /*if (!install_hook_offset(0x67D3C9, hook1, &detour1, &jmp_ret1, 8)) {
        spdlog::error("Failed to init ShadowResolution mod 1\n");
        return "Failed to init ShadowResolution mod 1";
	}*/

    return Mod::on_initialize();
}

void ShadowResolution::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ShadowResolution").value_or(false);
}

void ShadowResolution::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ShadowResolution", mod_enabled);
}
