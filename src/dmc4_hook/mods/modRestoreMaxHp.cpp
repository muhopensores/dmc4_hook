#include "../mods.h"
#include "modRestoreMaxHp.hpp"
#include "modInputStates.hpp"

#if 1
bool RestoreMaxHp::modEnabled{ false };
uintptr_t RestoreMaxHp::_restoreMaxHpContinue{ NULL };

naked void restoreMaxHp_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fstp dword ptr [ebx+1DA8h]  // Originalcode

        fld dword ptr [ebx+1548h]   // Max HP
        fstp dword ptr [ebx+1544h]  // Current HP
        jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinue]

    code:
        fstp dword ptr [ebx+1DA8h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinue]
    }
}

std::optional<std::string> RestoreMaxHp::onInitialize()
{
    if (!install_hook_offset(0x143BD0, hook, &restoreMaxHp_proc, &RestoreMaxHp::_restoreMaxHpContinue, 6))
    {
        HL_LOG_ERR("Failed to init InfDreadnought mod\n");
        return "Failed to init InfDreadnought mod";
    }

    return Mod::onInitialize();
}

void RestoreMaxHp::onGUIframe()
{
    // from main.cpp
    // line 2221 -> main->getMods()->onDrawUI("RestoreMaxHp"_hash);
    ImGui::Checkbox("Restore Max HP", &modEnabled);
}

void RestoreMaxHp::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("restore_max_hp").value_or(false);
};

void RestoreMaxHp::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("restore_max_hp", modEnabled);
};

#endif
