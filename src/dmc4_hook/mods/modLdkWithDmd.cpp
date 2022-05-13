#include "../mods.h"
#include "modLdkWithDmd.hpp"

#if 1

bool LdkWithDmd::modEnabled{ false };
uintptr_t LdkWithDmd::_ldkWithDmdContinue{ NULL };


naked void ldkWithDmd_proc(void) {
    _asm {
        cmp [LdkWithDmd::modEnabled], 0
        je code

		add eax,[edi+00000140h]
		dec eax
        jmp dword ptr [LdkWithDmd::_ldkWithDmdContinue]

    code:
        add eax,[edi+00000140h]
		jmp dword ptr [LdkWithDmd::_ldkWithDmdContinue]
    }
}

std::optional<std::string> LdkWithDmd::onInitialize() {
    if (!install_hook_offset(0x94AED, hook, &ldkWithDmd_proc, &LdkWithDmd::_ldkWithDmdContinue, 6)) {
        HL_LOG_ERR("Failed to init LdkWithDmd mod\n");
        return "Failed to init LdkWithDmd mod";
    }

    return Mod::onInitialize();
}

void LdkWithDmd::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3309A1, patch, "\x7d\x1f", 2);
    }
    else {
        patch.revert();
    }
}

void LdkWithDmd::onGUIframe() {
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("LDK With DMD", &modEnabled);
	ImGui::SameLine();
	HelpMarker("When playing a mission on Legendary Dark Knight, enemies will be on DMD Difficulty instead of SOS");
}

void LdkWithDmd::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("ldk_with_dmd").value_or(false);
}

void LdkWithDmd::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("ldk_with_dmd", modEnabled);
}

#endif
