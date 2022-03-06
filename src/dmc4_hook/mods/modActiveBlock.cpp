#include "modActiveBlock.hpp"
#include "modInputStates.hpp"

uintptr_t ActiveBlock::jmp_return{ NULL };
bool ActiveBlock::modEnabled{ false };
uintptr_t ActiveBlock::alt_ret{ 0x007BBE76 };

uintptr_t ActiveBlock::jmp_return2{ NULL };
// int currentStyle = 0;

uintptr_t ActiveBlock::jmp_return3{ NULL };
uintptr_t ActiveBlock::alt_ret3{ 0x007BBAC1 };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

ActiveBlock::ActiveBlock()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        cmp [ActiveBlock::modEnabled], 0
        jmp code

        cmp dword ptr [InputStates::inputTimer], 0x40a00000 // 0x41200000 = 10 // 0x40a00000 = 5
        ja code
        cmp dword ptr [InputStates::inputTimer], 0x00000000
        je code
        // cmp dword ptr [currentStyle], 3
        jne code
        push edi
        jmp dword ptr [ActiveBlock::alt_ret]

    code:
        cmp byte ptr [esi+0x00014CD4], bl
		jmp dword ptr [ActiveBlock::jmp_return]
    }
}

naked void detour2()
{
    _asm {
        mov eax,[esi+0x00014D98] // set up a PlayerTracker already
        cmp [ActiveBlock::modEnabled], 0
        je retcode

        // mov dword ptr [currentStyle], eax

    retcode:
		jmp dword ptr [ActiveBlock::jmp_return2]
    }
}

naked void detour3()
{
    _asm {
        cmp [ActiveBlock::modEnabled], 0
        je code

        push ecx
        mov ecx, [staticMediatorPtr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp esi, ecx
        pop ecx
        jne code

        cmp dword ptr [InputStates::inputTimer], 0x40a00000
        ja code
        cmp dword ptr [InputStates::inputTimer], 0x00000000
        je code
        cmp dword ptr [esi+0x00014D98], 3 //[currentStyle], 3
        jne code
        jmp dword ptr [ActiveBlock::alt_ret3]

    code:
        cmp dword ptr [esi+0x000152A0],00
		jmp dword ptr [ActiveBlock::jmp_return3]
    }
}

std::optional<std::string> ActiveBlock::onInitialize()
{
    /* if (!install_hook_offset(0x3BBE36, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init ActiveBlock mod\n");
        return "Failed to init ActiveBlock mod";
    }
    if (!install_hook_offset(0x3B6DCD, hook2, &detour2, &jmp_return2, 6))
    {
        HL_LOG_ERR("Failed to init ActiveBlock2 mod\n");
        return "Failed to init ActiveBlock2 mod";
    }*/
    if (!install_hook_offset(0x3BBAAE, hook3, &detour3, &jmp_return3, 7))
    {
        HL_LOG_ERR("Failed to init ActiveBlock3 mod\n");
        return "Failed to init ActiveBlock3 mod";
    }
    return Mod::onInitialize();
}

void ActiveBlock::onGUIframe()
{
    ImGui::Checkbox("Active Block", &modEnabled);
    ImGui::SameLine(0, 1);
    HelpMarker("Block during uncancellable frames");
}

void ActiveBlock::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("active_block").value_or(false);
};

void ActiveBlock::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("active_block", modEnabled);
};
