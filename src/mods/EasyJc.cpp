#include "EasyJc.hpp"
#include "FreeJc.hpp"

#if 1
bool EasyJc::mod_enabled{false};

uintptr_t EasyJc::easy_jc_continue{ NULL };
static float jc_scale{1.5f};

naked void easy_jc_proc(void) {
    _asm {
        movss xmm3, dword ptr [esi-0x10]
		cmp byte ptr [EasyJc::mod_enabled], 0
        je originalcode

        mulss xmm3, dword ptr [jc_scale]
		originalcode:
		jmp dword ptr [EasyJc::easy_jc_continue]
    }
}

std::optional<std::string> EasyJc::on_initialize() {
    if (!install_hook_offset(0xAB336, hook, &easy_jc_proc, &EasyJc::easy_jc_continue, 5)) {
        spdlog::error("Failed to init EasyJc mod\n");
        return "Failed to init EasyJc mod";
    }
    return Mod::on_initialize();
}

void EasyJc::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x427999, patch, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        install_patch_offset(0x427999, patch, "\xBF\x02\x00\x00\x00", 5); // because FreeJC uses same addr
    }
}

void EasyJc::on_gui_frame() {
    if (ImGui::Checkbox(_("Easy Enemy Step"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("50% bigger Enemy Step hitspheres"));
}

void EasyJc::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("easy_jc").value_or(false);
    toggle(mod_enabled);
}

void EasyJc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("easy_jc", mod_enabled);
}

#endif
