#include "GhostWalls.hpp"

bool GhostWalls::mod_enabled = false;
uintptr_t GhostWalls::jmp_ret1 = NULL;

static constexpr uintptr_t detour1_jmp_je = 0x583E8D;
naked void detour1() {
    _asm {
            cmp byte ptr [GhostWalls::mod_enabled], 1
            je newcode
        originalcode:
            cmp byte ptr [ebx+0x000020B8],00
            je je_code
        retcode:
            jmp dword ptr [GhostWalls::jmp_ret1]

        je_code:
            jmp dword ptr [detour1_jmp_je]

        newcode:
            cmp byte ptr [ebx+0x1410], 8 // id, mephisto
            je CheckMephisto
            //cmp byte ptr [ebx+0x1410], 9 // id, faust
            //je CheckFaust
            jmp originalcode

        CheckMephisto:
            cmp byte ptr [ebx+0x14], 14 // moveid, mephisto divekick
            je originalcode
            jmp retcode

        /*CheckFaust :
            cmp byte ptr [ebx+0x14], 14 // moveid, faust divekick
            je originalcode
            jmp retcode*/
    }
}

void GhostWalls::on_gui_frame(int display) {
    ImGui::Checkbox(_("Mephisto/Faust Collision"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Mephisto and Faust get caught on stage collision"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GhostWalls::on_initialize() {
    if (!install_hook_offset(0x183E51, GhostWalls::hook1, &detour1, &GhostWalls::jmp_ret1, 7)) {
        spdlog::error("Failed to init GhostWalls mod 1\n");
        return "Failed to init GhostWalls mod 1";
	}
    return Mod::on_initialize();
}

void GhostWalls::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("GhostWalls").value_or(false);
}

void GhostWalls::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("GhostWalls", mod_enabled);
}
