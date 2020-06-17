#include "../mods.h"
#include "modLdkWithDmd.hpp"

#if 1

bool LdkWithDmd::modEnabled{ false };
bool LdkWithDmd::modFrostsCantEscapeEnabled{ false };
uintptr_t LdkWithDmd::_ldkWithDmdContinue{ NULL };


naked void ldkWithDmd_proc(void)
{
    _asm {
        cmp [LdkWithDmd::modFrostsCantEscapeEnabled],1
        je frosts
        cmp [LdkWithDmd::modEnabled], 0
        je code

    ldkdmd:
		add eax,[edi+00000140h]
		dec eax
        jmp dword ptr [LdkWithDmd::_ldkWithDmdContinue]

    frosts:
		add eax,[edi+00000140h]
        cmp [LdkWithDmd::modEnabled], 1
        je ldkdmd

    code:
        add eax,[edi+00000140h]
		jmp dword ptr [LdkWithDmd::_ldkWithDmdContinue]
    }
}

std::optional<std::string> LdkWithDmd::onInitialize()
{
    if (!install_hook_offset(0x94AED, hook, &ldkWithDmd_proc, &LdkWithDmd::_ldkWithDmdContinue, 6))
    {
        HL_LOG_ERR("Failed to init LdkWithDmd mod\n");
        return "Failed to init LdkWithDmd mod";
    }

    return Mod::onInitialize();
}

void LdkWithDmd::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3309A1, patch, "\x7d\x1f", 2);
    }
    else
    {
        patch.revert();
    }
}

void LdkWithDmd::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("LDK With DMD", &modEnabled);
	ImGui::SameLine(0, 1);
	HelpMarker("Start a Mission on Legendary Dark Knight. Enemies are on DMD Difficulty, instead of SOS");
    ImGui::Checkbox("Frosts Can't Escape", &modFrostsCantEscapeEnabled);
}

void LdkWithDmd::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("ldk_with_dmd").value_or(false);
    modFrostsCantEscapeEnabled = cfg.get<bool>("frosts_cant_escape").value_or(false);
};

void LdkWithDmd::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("ldk_with_dmd", modEnabled);
    cfg.set<bool>("frosts_cant_escape", modFrostsCantEscapeEnabled);
};

#endif
