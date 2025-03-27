#include "CharacterSwap.hpp"
#include "NoAutomaticCharacters.hpp"

bool CharacterSwap::mod_enabled{ false };
bool CharacterSwap::prefer_dante{ false };
uintptr_t CharacterSwap::jmp_ret{NULL};
uintptr_t CharacterSwap::jmp_jne{ 0x00405F62 };

void CharacterSwap::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x378AE9,  patch1, "\xB8\x01\x00\x00\x00\x90\x90", 7); // this stage is actually bp
        install_patch_offset(0x378ECB,  patch2, "\x90\x90\x90\x90\x90", 5); // leave whatever was left in ecx so it clears all difficulties and bp
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

void CharacterSwap::Prefer_Dante(bool enable) {
    if (enable) {
        // replace 4 (nero, no special costume, no auto) with 0
        install_patch_offset(0x378EF2, patch3,  "\x00", 1);
    }
    else {
        patch3.reset();
    }
}

naked void detourCharSwap(void) { // force which character is picked
    _asm {
        //
            cmp byte ptr [CharacterSwap::mod_enabled], 0
            je originalcode

            // bruh this gets destroyed before i can grab it
            // [0x00E552CC]+44]+8C

            // this is disgusting but I'm not sure how else to get portrait picked
            cmp dword ptr [NoAutomaticCharacters::lastPickedCharacter], 4
            jb originalcode

        // playNero:
            mov byte ptr [edi+0x28], 01
        originalcode:
            cmp byte ptr [edi+0x28], 01
            jne jnecode
            jmp dword ptr [CharacterSwap::jmp_ret]
        jnecode:
            jmp dword ptr [CharacterSwap::jmp_jne]
    }
}

std::optional<std::string> CharacterSwap::on_initialize() {
    if (!install_hook_offset(0x5F28, hook, &detourCharSwap, &jmp_ret, 6)) {
        spdlog::error("Failed to init AssaultsDontJump mod\n");
        return "Failed to init AssaultsDontJump mod";
    }
    return Mod::on_initialize();
}

void CharacterSwap::on_gui_frame() {
    if (ImGui::Checkbox(_("Character Select in any mission"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Does not work for M1"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Prefer Dante"), &prefer_dante)) {
        Prefer_Dante(prefer_dante);
    }
    ImGui::SameLine();
    help_marker(_("Also works if Character Select isn't enabled, for BP"));
}

void CharacterSwap::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("character_select").value_or(false);
    toggle(mod_enabled);
    prefer_dante = cfg.get<bool>("prefer_dante").value_or(false);
    Prefer_Dante(prefer_dante);
};

void CharacterSwap::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("character_select", mod_enabled);
    cfg.set<bool>("prefer_dante", prefer_dante);
};
