#include "EasyJc.hpp"

#if 1
bool EasyJc::mod_enabled_nero = false;
bool EasyJc::mod_enabled_dante = false;

uintptr_t EasyJc::easy_jc_continue{ NULL };
static float jc_scale{1.5f};

naked void easy_jc_proc(void) { // player in eax
    _asm {
        movss xmm3, dword ptr [esi-0x10]
        cmp byte ptr [eax+0x1494], 1 // controller id nero
        je CheckNero
		cmp byte ptr [EasyJc::mod_enabled_dante], 0
        je originalcode
        jmp newcode

        CheckNero:
        cmp byte ptr [EasyJc::mod_enabled_nero], 0
        je originalcode
    newcode:
        mulss xmm3, dword ptr [jc_scale]
    originalcode:
		jmp dword ptr [EasyJc::easy_jc_continue]
    }
}

std::optional<std::string> EasyJc::on_initialize() {
    if (!install_hook_offset(0xAB336, hook, &easy_jc_proc, &EasyJc::easy_jc_continue, 5)) {
        spdlog::error("Failed to init EasyJc mod 1\n");
        return "Failed to init EasyJc mod 1";
    }
    return Mod::on_initialize();
}

void EasyJc::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Easy Enemy Step"), &mod_enabled_nero);
        ImGui::SameLine();
        help_marker(_("50% bigger Enemy Step hitspheres"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Easy Enemy Step"), &mod_enabled_dante);
        ImGui::SameLine();
        help_marker(_("50% bigger Enemy Step hitspheres"));
    }
}

void EasyJc::on_config_load(const utility::Config& cfg) {
    EasyJc::mod_enabled_nero = cfg.get<bool>("easy_jc_nero").value_or(false);
    EasyJc::mod_enabled_dante = cfg.get<bool>("easy_jc_dante").value_or(false);
}

void EasyJc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("easy_jc_nero", EasyJc::mod_enabled_nero);
    cfg.set<bool>("easy_jc_dante", EasyJc::mod_enabled_dante);
}

#endif
