#include "FastPandora.hpp"

bool FastPandora::mod_enabled = false;
bool FastPandora::mod2_enabled = false;

uintptr_t FastPandora::jmp_ret1 = NULL;
    float funshipSpeedMultiplier = 3.0f;
uintptr_t FastPandora::jmp_ret2 = NULL;;
uintptr_t FastPandora::jmp_ret3 = NULL;;

// Grounded transformation
void FastPandora::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3BCE96, patch1, "\x90\x90", 2); // first transform
        install_patch_offset(0x3BD366, patch2, "\x90\x90", 2); // second transform
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

// Funship startup 1
naked void detour1() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [funshipSpeedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret1]

        handler:
            fld1
            jmp originalcode
    }
}

// Funship startup 2
naked void detour2() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [funshipSpeedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret2]
            
        handler:
            fld1
            jmp originalcode
    }
}

// Funship recovery
naked void detour3() {
    _asm {
            cmp byte ptr [FastPandora::mod2_enabled],1
            jne handler

            fld [funshipSpeedMultiplier]

        originalcode:
            sub esp,0x0C
            jmp [FastPandora::jmp_ret3]
            
        handler:
            fld1
            jmp originalcode
    }
}


std::optional<std::string> FastPandora::on_initialize() {
    if (!install_hook_offset(0x3DE1CA, hook1, &detour1, &jmp_ret1, 5)) { // Funship startup 1
        spdlog::error("Failed to init FastPandora mod1\n");
        return "Failed to init FastPandora mod 1";
	}

    if (!install_hook_offset(0x3DE4C9, hook2, &detour2, &jmp_ret2, 5)) { // Funship startup 2
        spdlog::error("Failed to init FastPandora mod2\n");
        return "Failed to init FastPandora mod 2";
	}

    if (!install_hook_offset(0x3DEF6C, hook3, &detour3, &jmp_ret3, 5)) { // Funship recovery
        spdlog::error("Failed to init FastPandora mod3\n");
        return "Failed to init FastPandora mod 3";
	}

    return Mod::on_initialize();
}

void FastPandora::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Fast Pandora"), &mod_enabled)) { // Grounded transformation shots
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Cycle to grounded Pandora moves in DT speed outside of DT"));
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Fast Gunship"), &mod2_enabled); // Funship
    ImGui::SameLine();
    help_marker(_("Significantly speed up Argument startup and recovery"));
}

void FastPandora::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("fast_pandora").value_or(false);
    toggle(mod_enabled);
    mod2_enabled = cfg.get<bool>("fast_funship").value_or(false);
}

void FastPandora::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_pandora", mod_enabled);
    cfg.set<bool>("fast_funship", mod2_enabled);
}
