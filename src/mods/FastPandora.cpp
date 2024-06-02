#include "FastPandora.hpp"

bool FastPandora::mod_enabled{ false };
bool FastPandora::mod2_enabled{ false };

uintptr_t FastPandora::jmp_ret1{NULL};
    float speedMultiplier = 3.0f;
uintptr_t FastPandora::jmp_ret2{NULL};
uintptr_t FastPandora::jmp_ret3{NULL};

//Startup 1
naked void detour1() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [speedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret1]

        handler:
            fld1
            jmp originalcode
    }
}
//Startup 2
naked void detour2() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [speedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret2]
            
        handler:
            fld1
            jmp originalcode
    }
}
//Recovery
naked void detour3() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [speedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret3]
            
        handler:
            fld1
            jmp originalcode
    }
}


std::optional<std::string> FastPandora::on_initialize() {
    if (!install_hook_offset(0x3DE1CA, hook1, &detour1, &jmp_ret1, 5)) {
    spdlog::error("Failed to init RisingSun mod\n");
    return "Failed to init RisingSun mod";
	}

    if (!install_hook_offset(0x3DE4C9, hook2, &detour2, &jmp_ret2, 5)) {
    spdlog::error("Failed to init RisingSun mod\n");
    return "Failed to init RisingSun mod";
	}

    if (!install_hook_offset(0x3DEF6C, hook3, &detour3, &jmp_ret3, 5)) {
    spdlog::error("Failed to init RisingSun mod\n");
    return "Failed to init RisingSun mod";
	}

    return Mod::on_initialize();
}

void FastPandora::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03BCE7E, patch, "\x66\x83\xBF\x72\x17\x00\x00\x01", 8);
    }
    else {
        patch.reset();
    }
}

void FastPandora::on_gui_frame() {
    if (ImGui::Checkbox(_("Fast Pandora"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Cycle to grounded Pandora moves in DT speed outside of DT"));
    if (mod_enabled) {
        ImGui::Checkbox(_("Faster Pandora"), &mod2_enabled);
        ImGui::SameLine();
        help_marker(_("Significantly speed up Argument startup and recovery"));
    }
}

void FastPandora::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("fast_pandora").value_or(false);
    mod2_enabled = cfg.get<bool>("faster_pandora").value_or(false);
    toggle(mod_enabled);
}

void FastPandora::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("fast_pandora", mod_enabled);
    cfg.set<bool>("faster_pandora", mod2_enabled);
}
