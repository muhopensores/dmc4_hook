
#include "FreeJc.hpp"
#include "EasyJc.hpp"

#if 1
bool FreeJc::mod_enabled{ false };

std::optional<std::string> FreeJc::on_initialize() {
    return Mod::on_initialize();
}

void FreeJc::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x404A26, patch1, "\x90\x90", 2);
        install_patch_offset(0x427999, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.reset();
        install_patch_offset(0x427999, patch2, "\xBF\x02\x00\x00\x00", 5);
    }
}

void FreeJc::on_gui_frame() {
    if (ImGui::Checkbox("Free Enemy Step", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Enemy Step anywhere in the room with an enemy");
}

void FreeJc::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("free_jc").value_or(false);
    toggle(mod_enabled);
}

void FreeJc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("free_jc", mod_enabled);
}

#endif
