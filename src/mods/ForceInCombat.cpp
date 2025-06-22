#include "ForceInCombat.hpp"
#include "Survival.hpp"

bool ForceInCombat::mod_enabled = false;
uintptr_t ForceInCombat::jmp_ret1 = NULL;

naked void detour1() {
    _asm {
            cmp byte ptr [Survival::survival_active], 1
            je ForceCombat
            cmp byte ptr [ForceInCombat::mod_enabled], 1
            je ForceCombat

        // originalcode:
            pop esi
            mov al,bl
            pop ebx
            ret

        forcecombat:
            pop esi
            mov al, 1
            pop ebx
            ret
    }
}

// void ForceInCombat::on_frame(fmilliseconds& dt) {}

void ForceInCombat::on_gui_frame(int display) {
    ImGui::Checkbox(_("Force \"In Combat\""), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Useful for forcing last enemy cam, style point gain etc."));
}

// void ForceInCombat::on_game_pause(bool toggle) {}
// bool ForceInCombat::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ForceInCombat::on_initialize() {
    if (!install_hook_offset(0x936BC, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init ForceInCombat mod 1\n");
        return "Failed to init ForceInCombat mod 1";
	}

    return Mod::on_initialize();
}

void ForceInCombat::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ForceInCombat").value_or(false);
}

void ForceInCombat::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ForceInCombat", mod_enabled);
}
