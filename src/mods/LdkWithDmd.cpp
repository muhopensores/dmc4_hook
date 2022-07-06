
#include "LdkWithDmd.hpp"

#if 1

bool LdkWithDmd::mod_enabled{ false };
uintptr_t LdkWithDmd::ldk_with_dmd_continue{ NULL };


naked void ldk_with_dmd_proc(void) {
    _asm {
        cmp [LdkWithDmd::mod_enabled], 0
        je code

		add eax,[edi+00000140h]
		dec eax
        jmp dword ptr [LdkWithDmd::ldk_with_dmd_continue]

    code:
        add eax,[edi+00000140h]
		jmp dword ptr [LdkWithDmd::ldk_with_dmd_continue]
    }
}

std::optional<std::string> LdkWithDmd::on_initialize() {
    if (!install_hook_offset(0x94AED, hook, &ldk_with_dmd_proc, &LdkWithDmd::ldk_with_dmd_continue, 6)) {
        spdlog::error("Failed to init LdkWithDmd mod\n");
        return "Failed to init LdkWithDmd mod";
    }

    return Mod::on_initialize();
}

void LdkWithDmd::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3309A1, patch, "\x7d\x1f", 2);
    }
    else {
        patch.reset();
    }
}

void LdkWithDmd::on_gui_frame() {
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("LDK With DMD", &mod_enabled);
	ImGui::SameLine();
	help_marker("When playing a mission on Legendary Dark Knight, enemies will be on DMD Difficulty instead of SOS");
}

void LdkWithDmd::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("ldk_with_dmd").value_or(false);
}

void LdkWithDmd::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ldk_with_dmd", mod_enabled);
}

#endif
