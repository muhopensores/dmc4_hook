#include "../mods.h"
#include "modForceLucifer.hpp"

#if 1

bool ForceLucifer::modEnabled{ false };
uintptr_t ForceLucifer::_forceLuciferContinue{ NULL };

ForceLucifer::ForceLucifer()
{
    // onInitialize();
}

naked void forceLucifer_proc(void)
{
    _asm {
        cmp [ForceLucifer::modEnabled], 0
        je code

        cmp [ecx+1370h],6
        jne code
        mov [ecx+137Ch],1
        jmp code

    code:
        mov al,[ecx+137Ch]
		jmp dword ptr [ForceLucifer::_forceLuciferContinue]
    }
}

std::optional<std::string> ForceLucifer::onInitialize()
{
    if (!install_hook_offset(0x431CF0, hook, &forceLucifer_proc, &ForceLucifer::_forceLuciferContinue, 6))
    {
        HL_LOG_ERR("Failed to init ForceLucifer mod\n");
        return "Failed to init ForceLucifer mod";
    }

    return Mod::onInitialize();
}

void ForceLucifer::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("Force Lucifer", &modEnabled);
}

void ForceLucifer::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("force_lucifer").value_or(false);
};

void ForceLucifer::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("force_lucifer", modEnabled);
};

#endif
