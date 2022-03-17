#include "../mods.h"
#include "modOneHitKill.hpp"
#include "../utils/MessageDisplay.hpp" // TODO(): DISPLAY_MESSAGE should probably be included in mod.hpp or something
#include "modEnemySpawn.hpp"
#if 1
bool OneHitKill::cantDie{ false };
bool OneHitKill::oneHitKill{ false };
int OneHitKill::hotkey{ NULL };
uintptr_t OneHitKill::jmp_ret{ NULL };
uintptr_t OneHitKill::jmp_out{ 0x0051C129 };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

int oneHitKillHotkey;

void OneHitKill::NoDeathToggle(bool enable){ // no death 
    if (enable)
    {
        //install_patch_offset(0x11C11B, patchhp, "\xEB", 1); // used for both
        install_patch_offset(0x11C694, patchomen, "\xEB", 1);
    }
    else
    {
        //patchhp.revert();
        patchomen.revert();
    }
}

naked void detour(void)
{
    _asm {
            cmp byte ptr [OneHitKill::oneHitKill], 1
            je playercheck
			cmp byte ptr [OneHitKill::cantDie], 1
			je jmpout
        code:
			comiss xmm2, [esi+0x18]
			jb jmpout
        jmpret:
			jmp dword ptr [OneHitKill::jmp_ret]

		jmpout:
			jmp dword ptr [OneHitKill::jmp_out]

        playercheck:
            push ecx
            push edx
            mov ecx, [staticMediatorPtr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            mov edx, [esi+0xC0]
            cmp edx, ecx
            pop edx
            pop ecx
            je code
            jmp jmpret
    }
}

std::optional<std::string> OneHitKill::onInitialize()
{
    if (!install_hook_offset(0x011C117, hook, &detour, &jmp_ret, 6))
    {
        HL_LOG_ERR("Failed to init OneHitKill mod\n");
        return "Failed to init OneHitKill mod";
    }

    return Mod::onInitialize();
}

void OneHitKill::onGUIframe()
{
    if (ImGui::Checkbox("No Death (All)", &cantDie))
    {
        oneHitKill = false;
        //toggle2(oneHitKill);
        NoDeathToggle(cantDie);
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Disables dying while still allowing hp to drop. Also stops Omen killing enemies.");
    ImGui::SameLine(205);
    if (ImGui::Checkbox("One Hit Kill", &oneHitKill))
    {
        cantDie = false;
        NoDeathToggle(cantDie);
        //toggle2(oneHitKill);
    }
}

void OneHitKill::onConfigLoad(const utils::Config& cfg)
{
    cantDie = cfg.get<bool>("no_death").value_or(false);
    oneHitKill = cfg.get<bool>("one_hit_kill").value_or(false);
    hotkey = cfg.get<int>("one_hit_kill_hotkey").value_or(0x72);
    NoDeathToggle(cantDie);
};

void OneHitKill::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("no_death", cantDie);
    cfg.set<bool>("one_hit_kill", oneHitKill);
    cfg.set<int>("one_hit_kill_hotkey", hotkey);
};

void OneHitKill::onUpdateInput(hl::Input& input)
{
    if (!input.isDown(EnemySpawn::hotkeySpawnModifier))
    {
        if (input.wentDown(hotkey))
        {
            if (oneHitKill)
            {
                DISPLAY_MESSAGE("One Hit Kill Off");
            }
            else
            {
                DISPLAY_MESSAGE("One Hit Kill On");
                cantDie = false;
                NoDeathToggle(cantDie);
            }
            oneHitKill = !oneHitKill;
            // toggle2(oneHitKill);
        }
    }
}
#endif
