#include "PlayerRotation.hpp"

#if 1
bool PlayerRotation::mod_enabled{false};

uintptr_t PlayerRotation::player_rotation_continue{NULL};

naked void player_rotation_proc(void) {
    _asm {
        // originalcode:
        movsd [edx+0x0000141C],xmm0 // movq

		cmp byte ptr [PlayerRotation::mod_enabled], 0
        je retcode
        cmp dword ptr [edx+0x00001494], 1 // controller id nero
        ja retcode
        //cmp dword ptr [edx+0x00002998], 0x132
        //jne retcode
        cmp dword ptr [edx+0x0000141C], 0 // stick threshold
        je retcode

        movss xmm0,[edx+0x00001420] // stick direction

        movss [edx+0x000016C0],xmm0 // overwrite air inertia
        movss [edx+0x00001710],xmm0 // overwrite rotation

        retcode:
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

void PlayerRotation::on_gui_frame() {
    ImGui::Checkbox("Unlock rotation", &mod_enabled);
    ImGui::SameLine();
    help_marker("Meme mode that locks player rotation to stick direction");
}

void PlayerRotation::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("player_rotation_enabled").value_or(false);
}

void PlayerRotation::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("player_rotation_enabled", mod_enabled);
}

#endif
