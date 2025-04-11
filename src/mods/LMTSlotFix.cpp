#include "LMTSlotFix.hpp"
#include "MoveTable.hpp"
bool LMTSlotFix::mod_enabled { true };

uintptr_t LMTSlotFix::jmp_ret1{ NULL };
    constexpr uintptr_t detour1_default_jmp = 0x42B936;
    constexpr uintptr_t detour1_jmp = 0x42B936;

/*void LMTSlotFix::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x2B934, patch1, "\x90\x90", 2);//break infinite loop referencing cCharTblMgr
    }
    else {
        patch1.reset();
    }
}*/

naked void detour1() {
    _asm {
            cmp byte ptr [MoveTable::mod_enabled], 1
            jne originalcode
            cmp eax,3
            ja detour1_ja
        originalcode:
            cmp eax,3
            ja detour1_default_ja
            jmp [LMTSlotFix::jmp_ret1]
        detour1_ja:
            jmp detour1_jmp
        detour1_default_ja:
            jmp detour1_default_jmp
    }
}

std::optional<std::string> LMTSlotFix::on_initialize() {
    if (!install_hook_offset(0x2B8FF, hook1, &detour1, &jmp_ret1, 5))
    {
        spdlog::error("Failed to init LMTSlotFix mod1\n");
        return "Failed to init LMTSlotFix mod1";
    }
    return Mod::on_initialize();
}

void LMTSlotFix::on_gui_frame() {
    /*if (ImGui::Checkbox(_("LMT slot fix"), &mod_enabled)) {
        //toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Enable reusing zeroed out offsets in LMT files. DO NOT disable if you have custom motion mods."));*/
}

void LMTSlotFix::on_config_load(const utility::Config& cfg) {
    // mod_enabled = cfg.get<bool>("lmt_slot_fix").value_or(false);
    // toggle(mod_enabled);
};

void LMTSlotFix::on_config_save(utility::Config& cfg) {
    // cfg.set<bool>("lmt_slot_fix", mod_enabled);
};
