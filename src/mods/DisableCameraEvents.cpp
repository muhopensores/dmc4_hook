#include "DisableCameraEvents.hpp"

bool DisableCameraEvents::mod_enabled{ false };
uintptr_t DisableCameraEvents::jmp_ret1{NULL};

void DisableCameraEvents::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x000BEF1, patch1, "\xB8\x06\x00\x00\x00\x90\x90", 7); // skips to part x of cutscene
        install_patch_offset(0x0093BE0, patch2, "\xEB\x13", 2); // disables fade to black flash but breaks move list + map
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}
# if 0
uintptr_t sArea = 0x00E552C8;
uintptr_t jmpOutAddr = 0x00493BF5;
naked void detour1() {
    _asm {
        cmp byte ptr [DisableCameraEvents::mod_enabled], 0
        je originalcode

        push eax

        mov eax, [sArea] // sArea
        mov eax, [eax]
        test eax, eax
        je popcode
        mov eax, [eax+0x3830] // aGame
        test eax, eax
        je popcode
        cmp byte ptr [eax+0x1d1], 1
        je popcode
        jmp skipcode

        popcode:
        pop eax
        originalcode:
        test eax, eax
        je jmpout
        fld dword ptr [esp+0x0C]
        jmp dword ptr [DisableCameraEvents::jmp_ret1]

        skipcode:
        pop eax
        jmpout:
        jmp dword ptr [jmpOutAddr]
    }
}
#endif
std::optional<std::string> DisableCameraEvents::on_initialize() {
    /*if (!install_hook_offset(0x93BDE, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init DisableCameraEvents mod\n");
		return "Failed to init DisableCameraEvents mod";
	}*/
    return Mod::on_initialize();
}

void DisableCameraEvents::on_gui_frame() {
    if (ImGui::Checkbox(_("Disable Camera Events"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("This now breaks the move list and map rather than your gameplay!"));
}

void DisableCameraEvents::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_camera_events").value_or(false);
    toggle(mod_enabled);
}

void DisableCameraEvents::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_camera_events", mod_enabled);
}
