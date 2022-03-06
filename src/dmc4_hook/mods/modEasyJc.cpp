#include "../mods.h"
#include "modEasyJc.hpp"
#include "modFreeJc.hpp"

#if 1
bool EasyJc::modEnabled{ false };
uintptr_t EasyJc::_easyJcContinue{ NULL };
float jcScale{1.5f};

EasyJc::EasyJc()
{
    // onInitialize();
}

naked void easyJc_proc(void)
{
    _asm {
		cmp byte ptr [EasyJc::modEnabled], 0
        je originalcode

        movss xmm3, dword ptr [esi-0x10]
        mulss xmm3, dword ptr [jcScale]
        jmp dword ptr [EasyJc::_easyJcContinue]

		originalcode:
        movss xmm3, dword ptr [esi-0x10]
		jmp dword ptr [EasyJc::_easyJcContinue]
    }
}

std::optional<std::string> EasyJc::onInitialize()
{
    if (!install_hook_offset(0xAB336, hook, &easyJc_proc, &EasyJc::_easyJcContinue, 5))
    {
        HL_LOG_ERR("Failed to init EasyJc mod\n");
        return "Failed to init EasyJc mod";
    }

    return Mod::onInitialize();
}

void EasyJc::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x427999, patch, "\x90\x90\x90\x90\x90", 5);
    }
    else
    {
        install_patch_offset(0x427999, patch, "\xBF\x02\x00\x00\x00", 5);
    }
}

void EasyJc::onGUIframe()
{
    // from main.cpp
    // line 899 -> if (ImGui::Checkbox("Easy JCs", &checkEasyJc))
    if (ImGui::Checkbox("Easy JC", &modEnabled))
    {
        toggle(modEnabled);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("50% bigger Enemy Step hitspheres");
}

void EasyJc::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("easy_jc").value_or(false);
    toggle(modEnabled);
};

void EasyJc::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("easy_jc", modEnabled);
};

#endif