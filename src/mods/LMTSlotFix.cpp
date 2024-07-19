#include "LMTSlotFix.hpp"

bool LMTSlotFix::mod_enabled { true };

void LMTSlotFix::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x2B934, patch1, "\x90\x90", 2);//break infinite loop referencing cCharTblMgr
    }
    else {
        patch1.reset();
    }
}

std::optional<std::string> LMTSlotFix::on_initialize() {
    return Mod::on_initialize();
}

void LMTSlotFix::on_gui_frame() {
    if (ImGui::Checkbox(_("LMT slot fix"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Enable reusing zeroed out offsets in LMT files. DO NOT disable if you have custom motion mods."));
}

void LMTSlotFix::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("lmt_slot_fix").value_or(false);
    toggle(mod_enabled);
};

void LMTSlotFix::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("lmt_slot_fix", mod_enabled);
};
