#include "DisableCameraEvents.hpp"

bool DisableCameraEvents::mod_enabled = false;
uintptr_t DisableCameraEvents::jmp_ret1 = NULL;

void DisableCameraEvents::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x000BEF1, patch1, "\xB8\x07\x00\x00\x00\x90\x90", 7); // skips to part x of cutscene
        //install_patch_offset(0x0093BE0, patch2, "\xEB\x13", 2); // disables fade to black flash but breaks move list + map, has ugly load into bp
        // install_patch_offset(0x000C6C1, patch2, "\xEB\x17", 2); // old version
        // try setting that one value in aGame to 4 when old version is called
        // DevilMayCry4_DX9.exe+C9DD starting first forest fight or castle demo
        // DevilMayCry4_DX9.exe+14C3A8 ending first forest fight
    }
    else {
        patch1.reset();
        //patch2.reset();
    }
}

uintptr_t sArea = 0x00E552C8;
uintptr_t jmpOutAddr = 0x00493BF5;
uintptr_t sMed = 0x0E558B8;

naked void detour1() {
    _asm {
        cmp byte ptr [DisableCameraEvents::mod_enabled], 0
        je originalcode

        push eax

        mov eax, [sMed]
        mov eax, [eax]
        test eax, eax
        je popcode
        mov eax, [eax+0x24]
        test eax, eax
        je popcode // no player, user is in a menu

        mov eax, [sArea] // sArea addr
        mov eax, [eax] // sArea
        test eax, eax
        je popcode // no change
        mov eax, [eax+0x3838] // currentRoom
        test eax, eax
        je popcode // no change
        mov eax, [eax+0x114] // pauseMenu
        test eax, eax
        je skipcode // pause menu not found, skip fade
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

std::optional<std::string> DisableCameraEvents::on_initialize() {
    if (!install_hook_offset(0x93BDE, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init DisableCameraEvents mod\n");
		return "Failed to init DisableCameraEvents mod";
	}
    return Mod::on_initialize();
}

void DisableCameraEvents::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Disable Camera Events"), &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void DisableCameraEvents::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_camera_events").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
}

void DisableCameraEvents::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_camera_events", mod_enabled);
}
