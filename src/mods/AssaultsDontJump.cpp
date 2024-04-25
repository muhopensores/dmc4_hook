#include "AssaultsDontJump.hpp"

#if 1
uintptr_t AssaultsDontJump::jmp_ret{NULL};
uintptr_t AssaultsDontJump::jmp_out{0x005CC590};

bool AssaultsDontJump::mod_enabled{false};

naked void detour(void) {
    _asm {
			cmp byte ptr [AssaultsDontJump::mod_enabled], 0
			je originalcode

            cmp byte ptr [esi+0x148C], 0 // dt
            je newcode

        originalcode:
            mov eax, [esi+0x00001F70]
			jmp dword ptr [AssaultsDontJump::jmp_ret]

        newcode:
            mov eax, [esi+0x0000B9E0]
            jmp dword ptr [AssaultsDontJump::jmp_out]
    }
}

std::optional<std::string> AssaultsDontJump::on_initialize() {
    if (!install_hook_offset(0x1CC53F, hook, &detour, &jmp_ret, 6)) {
        spdlog::error("Failed to init AssaultsDontJump mod\n");
        return "Failed to init AssaultsDontJump mod";
    }

    return Mod::on_initialize();
}

// Completely remove assault jump
// Found accessing DT value at enemy+148C. Difficulty or something at assault+1F70
/*void AssaultsDontJump::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x1CC552, patch1, "\xEB\x3C", 2);
    } else {
        patch1.reset();
    }
}*/

void AssaultsDontJump::on_gui_frame() {

    ImGui::Checkbox(_("No DTless Assault Escape"), &mod_enabled);
    //toggle(mod_enabled);
    ImGui::SameLine();
    help_marker(_("Disable Assaults jumping out of combos until they DT"));
}

void AssaultsDontJump::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_assault_jumpout").value_or(false);
    //toggle(mod_enabled);
}

void AssaultsDontJump::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_assault_jumpout", mod_enabled);
}

#endif
