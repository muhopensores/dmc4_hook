#include "InfFrostHeal.hpp"

bool InfFrostHeal::mod_enabled = false;
uintptr_t InfFrostHeal::jmp_ret1 = NULL;

void InfFrostHeal::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x1A5969, patch1, "\x90\x90\x90", 3);
    }
    else {
        patch1.reset(); // sete cl, after cmp dword ptr [esi+000098C0],05
    }
}
/*
naked void detour1() {
    _asm {
            cmp byte ptr [InfFrostHeal::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [InfFrostHeal::jmp_ret1]
    }
}
*/
// void on_frame(fmilliseconds& dt) {}

void InfFrostHeal::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Frost Heals"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("By default, Frosts are limited to successfully healing and rearming 5 times, making some attacks awkward to see in practice"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> InfFrostHeal::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init InfFrostHeal mod 1\n");
        return "Failed to init InfFrostHeal mod 1";
	}*/

    return Mod::on_initialize();
}

void InfFrostHeal::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("InfFrostHeal").value_or(false);
    toggle1(mod_enabled);
}

void InfFrostHeal::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("InfFrostHeal", mod_enabled);
}
