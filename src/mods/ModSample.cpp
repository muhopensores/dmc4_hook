#include "ModSample.hpp"

#if 0
bool ModSample::mod_enabled = false;
uintptr_t ModSample::jmp_ret1 = NULL;

void ModSample::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x0, patch1, "\x90", 1);
    }
    else {
        patch1.reset();
    }
}

naked void detour1() {
    _asm {
            cmp byte ptr [ModSample::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [ModSample::jmp_ret1]
    }
}

// void on_frame(fmilliseconds& dt) {}

void ModSample::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("ModSample"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_(""));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ModSample::on_initialize() {
    if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init ModSample mod 1\n");
        return "Failed to init ModSample mod 1";
	}

    return Mod::on_initialize();
}

void ModSample::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ModSample").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void ModSample::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ModSample", mod_enabled);
}
#endif
