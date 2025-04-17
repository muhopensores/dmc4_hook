
#include "FrostsDontJump.hpp"

#if 1
bool FrostsDontJump::mod_enabled{ false };

std::optional<std::string> FrostsDontJump::on_initialize() {
    return Mod::on_initialize();
}

// Enemies access a float on spawn, that float seems to control I assume something like aggression until they DT
void FrostsDontJump::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x01A46F3, patch1, "\x0F\x57\xC0\x90\x90\x90\x90\x90", 8);
        // xorps xmm0, xmm0
        // nop 5
    }
    else {
        patch1.reset();
        // movss xmm0,[eax+00000134]
    }
}

void FrostsDontJump::on_gui_frame(int display) {

    if (ImGui::Checkbox(_("No DTless Frost Escape"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Disable Frosts jumping out of combos until they DT"));
}

void FrostsDontJump::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_frost_jumpout").value_or(false);
    toggle(mod_enabled);
}

void FrostsDontJump::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_frost_jumpout", mod_enabled);
}

#endif
