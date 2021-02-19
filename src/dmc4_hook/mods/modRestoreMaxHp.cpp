#include "../mods.h"
#include "modRestoreMaxHp.hpp"
#include "modInputStates.hpp"

bool RestoreMaxHp::modEnabled{ false };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueScarecrow{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueFrost{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueArmour{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueGhosts{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueSwordFly{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueSwordFish{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueSeed{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueRiot{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueBlitz{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueDog{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueBerial{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueFrog{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueEchidna{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueCredo{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueAgnus{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueSanctus{ NULL };
uintptr_t RestoreMaxHp::_restoreMaxHpContinueDante{ NULL };

uintptr_t RestoreMaxHp::_resetTimerContinue{ NULL };

float twoMinutesTimer = 7200.0f;
bool resetTimer = false;

naked void restoreMaxHpScarecrow_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [ebx+1548h]               // Max HP
        fstp dword ptr [ebx+1544h]              // Current HP

    code:
        fld dword ptr [ebx+1548h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueScarecrow]
    }
}

naked void restoreMaxHpFrost_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h] // Max HP
        fstp dword ptr [esi+151Ch] // Current HP

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueFrost]
    }
}

naked void restoreMaxHpArmour_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
        test byte ptr [esi+0EA8h],-80h
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueArmour]
    }
}

naked void restoreMaxHpGhosts_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [edi+151Ch]
        fstp dword ptr [edi+1518h]

    code:
        fld dword ptr [edi+151Ch]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueGhosts]
    }
}

naked void restoreMaxHpSwordFly_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+151Ch]
        fstp dword ptr [esi+1518h]

    code:
        fld dword ptr [esi+151Ch]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueSwordFly]
    }
}

naked void restoreMaxHpSwordFish_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1524h]
        fstp dword ptr [esi+1520h]

    code:
        fld dword ptr [esi+1524h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueSwordFish]
    }
}

naked void restoreMaxHpSeed_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [ebx+32A0h]
        fstp dword ptr [ebx+329Ch]

    code:
        fld dword ptr [ebx+32A0h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueSeed]
    }
}

naked void restoreMaxHpRiot_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+151Ch]
        fstp dword ptr [esi+1518h]

    code:
        fld dword ptr [esi+151Ch]
        cmp byte ptr [esi+1438h],00
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueRiot]
    }
}

naked void restoreMaxHpBlitz_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueBlitz]
    }
}

naked void restoreMaxHpDog_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [edi+7FE0h]
        fstp dword ptr [edi+7FDCh]

    code:
        fld dword ptr [edi+7FE0h]
        test byte ptr [edi+0EA8h], -80h
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueDog]
    }
}

naked void restoreMaxHpBerial_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueBerial]
    }
}

naked void restoreMaxHpFrog_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueFrog]
    }
}

naked void restoreMaxHpEchidna_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueEchidna]
    }
}

naked void restoreMaxHpCredo_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueCredo]
    }
}

naked void restoreMaxHpAgnus_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1520h]
        fstp dword ptr [esi+151Ch]

    code:
        fld dword ptr [esi+1520h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueAgnus]
    }
}

naked void restoreMaxHpSanctus_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [esi+1D0Ch]
        fstp dword ptr [esi+1D08h]

    code:
        fld dword ptr [esi+1D0Ch]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueSanctus]
    }
}

naked void restoreMaxHpDante_proc(void)
{
    _asm {
        cmp byte ptr [RestoreMaxHp::modEnabled],0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        fld dword ptr [edi+15D0h]
        fstp dword ptr [edi+15CCh]

    code:
        fld dword ptr [edi+15D0h]
		jmp dword ptr [RestoreMaxHp::_restoreMaxHpContinueDante]
    }
}

naked void resetTimer_proc(void)
{
    _asm {
        cmp byte ptr [resetTimer], 0
        je code
		cmp byte ptr [InputStates::inputpressed], 24
        jne code

        movss xmm3, [twoMinutesTimer]
        movss [esi+00000250h], xmm3

    code:
        movss xmm3, [esi+00000250h]
		jmp dword ptr [RestoreMaxHp::_resetTimerContinue]
    }
}

std::optional<std::string> RestoreMaxHp::onInitialize()
{
    if (!install_hook_offset(0x143BC1, hookScarecrow, &restoreMaxHpScarecrow_proc, &RestoreMaxHp::_restoreMaxHpContinueScarecrow, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1A6042, hookFrost, &restoreMaxHpFrost_proc, &RestoreMaxHp::_restoreMaxHpContinueFrost, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1653F5, hookArmour, &restoreMaxHpArmour_proc, &RestoreMaxHp::_restoreMaxHpContinueArmour, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x183ACF, hookGhosts, &restoreMaxHpGhosts_proc, &RestoreMaxHp::_restoreMaxHpContinueGhosts, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x20DFB6, hookSwordFly, &restoreMaxHpSwordFly_proc, &RestoreMaxHp::_restoreMaxHpContinueSwordFly, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1F902F, hookSwordFish, &restoreMaxHpSwordFish_proc, &RestoreMaxHp::_restoreMaxHpContinueSwordFish, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1E08D8, hookSeed, &restoreMaxHpSeed_proc, &RestoreMaxHp::_restoreMaxHpContinueSeed, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1B6FDC, hookRiot, &restoreMaxHpRiot_proc, &RestoreMaxHp::_restoreMaxHpContinueRiot, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x1D3603, hookBlitz, &restoreMaxHpBlitz_proc, &RestoreMaxHp::_restoreMaxHpContinueBlitz, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x21D971, hookDog, &restoreMaxHpDog_proc, &RestoreMaxHp::_restoreMaxHpContinueDog, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x235FF9, hookBerial, &restoreMaxHpBerial_proc, &RestoreMaxHp::_restoreMaxHpContinueBerial, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x24E6A3, hookFrog, &restoreMaxHpFrog_proc, &RestoreMaxHp::_restoreMaxHpContinueFrog, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x28A431, hookEchidna, &restoreMaxHpEchidna_proc, &RestoreMaxHp::_restoreMaxHpContinueEchidna, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x2ACE60, hookCredo, &restoreMaxHpCredo_proc, &RestoreMaxHp::_restoreMaxHpContinueCredo, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x2C1D77, hookAgnus, &restoreMaxHpAgnus_proc, &RestoreMaxHp::_restoreMaxHpContinueAgnus, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x305702, hookSanctus, &restoreMaxHpSanctus_proc, &RestoreMaxHp::_restoreMaxHpContinueSanctus, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0xBE16E, hookDante, &restoreMaxHpDante_proc, &RestoreMaxHp::_restoreMaxHpContinueDante, 6))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    if (!install_hook_offset(0x94D60, hookTimer, &resetTimer_proc, &RestoreMaxHp::_resetTimerContinue, 8))
    {
        HL_LOG_ERR("Failed to init RestoreMaxHp mod\n");
        return "Failed to init RestoreMaxHp mod";
    }

    return Mod::onInitialize();
}

void RestoreMaxHp::onGUIframe()
{
    // from main.cpp
    // line 2221 -> main->getMods()->onDrawUI("RestoreMaxHp"_hash);
    ImGui::Checkbox("Restore Max HP", &modEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("Press Lock On + Taunt to restore Max HP to enemies");
    ImGui::SameLine(205);
    ImGui::Checkbox("Reset Timer", &resetTimer);
    ImGui::SameLine(0, 1);
    HelpMarker("Press Lock On + Taunt to reset the BP timer");
}

void RestoreMaxHp::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("restore_max_hp").value_or(false);
    resetTimer = cfg.get<bool>("reset_bp_timer").value_or(false);
};

void RestoreMaxHp::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("restore_max_hp", modEnabled);
    cfg.set<bool>("reset_bp_timer", resetTimer);
};
