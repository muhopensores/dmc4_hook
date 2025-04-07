#include "DisableCameraEvents.hpp"

bool DisableCameraEvents::mod_enabled{ false };
uintptr_t DisableCameraEvents::jmp_ret1{NULL};

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

naked void detour1() {
    _asm {
        cmp byte ptr [DisableCameraEvents::mod_enabled], 0
        je originalcode

        push eax

        mov eax, [sArea] // sArea addr
        test eax, eax
        je popcode // no change, not in a mission
        mov eax, [eax] // sArea
        test eax, eax
        je popcode // no change
        mov eax, [eax+0x3838] // currentRoom
        test eax, eax
        je popcode // no change
        mov eax, [eax+0x114] // pauseMenu
        test eax, eax
        je skipcode // pause menu not found, skip fade
        //cmp ebp,[eax+0x1a0] // uSkillListMgr // this cmp fails
        //jne skipcode
        // pause menu found
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

#if 0
#include "DisableCameraEvents.hpp"

bool DisableCameraEvents::mod_enabled{ false };
uintptr_t DisableCameraEvents::jmp_ret1{NULL};

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
uintptr_t jmpOutAddr = 0x493BCD;
uintptr_t jmpReallyOutAddr = 0x493C16;
naked void detour1() {
    _asm {
        cmp byte ptr [DisableCameraEvents::mod_enabled], 0
        je originalcode

        push eax

        mov eax, ds:[sArea] // sArea
        test eax, eax
        je popcode
        add eax, 0x3838 // currentRoom
        test eax, eax
        je popcode
        mov eax, [eax+0x114] // pauseMenu
        test eax, eax
        je jmpReallyOut
        cmp ebp,[eax+0x1a0] // uSkillListMgr
        je jmpReallyOut
        jmp popcode // it is a pause menu

        popcode:
        pop eax
        originalcode:
        test ecx,ecx
        push esi
        je jmpout
        jmp dword ptr [DisableCameraEvents::jmp_ret1]

        jmpout:
        jmp dword ptr [jmpOutAddr]

        jmpReallyOut:
        pop eax
        jmp dword ptr [jmpReallyOutAddr]
    }
}

std::optional<std::string> DisableCameraEvents::on_initialize() {
    if (!install_hook_offset(0x93BB7, hook1, &detour1, &jmp_ret1, 5)) {
		spdlog::error("Failed to init DisableCameraEvents mod\n");
		return "Failed to init DisableCameraEvents mod";
	}
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
#endif
