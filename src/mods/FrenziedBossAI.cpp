#include "FrenziedBossAI.hpp"

bool FrenziedBossAI::mod_enabled = false;
// uintptr_t FrenziedBossAI::jmp_ret1 = NULL;

void FrenziedBossAI::toggle1(bool enable) {
    if (enable) {
        install_patch_absolute(0x634805, patch1, "\x74\x76", 2);
        install_patch_absolute(0x64d6fd, patch2, "\x74\x72", 2);
        install_patch_absolute(0x6544a8, patch3, "\x74\x72", 2);
        install_patch_absolute(0x688c9a, patch4, "\x74\x72", 2);
        install_patch_absolute(0x6aba60, patch5, "\x74\x72", 2);
        install_patch_absolute(0x6c03dc, patch6, "\x14\x1C", 2);
        install_patch_absolute(0x6fadad, patch7, "\x74\x76", 2);
        install_patch_absolute(0x704efd, patch8, "\x74\x76", 2);
    } else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
        patch8.reset();
    }
}

/*naked void detour1() {
    _asm {
            cmp byte ptr [FrenziedBossAI::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [FrenziedBossAI::jmp_ret1]
    }
}*/

// void on_frame(fmilliseconds& dt) {}

void FrenziedBossAI::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Frenzied Boss AI"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Force bosses to use the AI they usually use when low HP"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FrenziedBossAI::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init FrenziedBossAI mod 1\n");
        return "Failed to init FrenziedBossAI mod 1";
	}*/

    return Mod::on_initialize();
}

void FrenziedBossAI::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FrenziedBossAI").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void FrenziedBossAI::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FrenziedBossAI", mod_enabled);
}
