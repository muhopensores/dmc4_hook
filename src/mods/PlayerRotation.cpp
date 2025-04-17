#include "PlayerRotation.hpp"

#if 1
bool PlayerRotation::mod_enabled_nero = false;
bool PlayerRotation::mod_enabled_dante = false;

uintptr_t PlayerRotation::player_rotation_continue{NULL};

naked void player_rotation_proc(void) {
    _asm {
        // originalcode:
        movsd [edx+0x0000141C],xmm0 // movq
        cmp byte ptr [edx+0x1494], 1 // controller id nero
        je NeroCheck
        // DanteCheck:
        cmp byte ptr [PlayerRotation::mod_enabled_dante], 1
        je CheatCode
        jmp RetCode

        NeroCheck:
        cmp byte ptr [PlayerRotation::mod_enabled_nero], 1
        jne RetCode
        CheatCode:
        //cmp dword ptr [edx+0x00002998], 0x132
        //jne RetCode
        cmp dword ptr [edx+0x0000141C], 0 // stick threshold
        je RetCode
        movss xmm0,[edx+0x00001420] // stick direction
        movss [edx+0x000016C0],xmm0 // overwrite air inertia
        movss [edx+0x00001710],xmm0 // overwrite rotation
        jmp RetCode

        RetCode:
		jmp dword ptr [PlayerRotation::player_rotation_continue]
    }
}

std::optional<std::string> PlayerRotation::on_initialize() {
    if (!install_hook_offset(0x3A96A1, hook, &player_rotation_proc, &PlayerRotation::player_rotation_continue, 8)) {
        spdlog::error("Failed to init PlayerRotation mod\n");
        return "Failed to init PlayerRotation mod";
    }
    return Mod::on_initialize();
}

void PlayerRotation::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Unlock rotation"), &mod_enabled_nero);
        ImGui::SameLine();
        help_marker(_("Lock player rotation to stick direction"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Unlock rotation"), &mod_enabled_dante);
        ImGui::SameLine();
        help_marker(_("Lock player rotation to stick direction"));
    }
}

void PlayerRotation::on_config_load(const utility::Config& cfg) {
    mod_enabled_nero = cfg.get<bool>("player_rotation_enabled_nero").value_or(false);
    mod_enabled_dante = cfg.get<bool>("player_rotation_enabled_dante").value_or(false);
}

void PlayerRotation::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("player_rotation_enabled_nero", mod_enabled_nero);
    cfg.set<bool>("player_rotation_enabled_dante", mod_enabled_dante);
}

#endif
