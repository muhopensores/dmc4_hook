#include "../mods.h"
#include "modInfDreadnought.hpp"

#if 1
bool InfDreadnought::modEnabled{ false };
uintptr_t InfDreadnought::_infDreadnoughtContinue{ NULL };


naked void infDreadnought_proc(void)
{
    _asm {
		cmp byte ptr [InfDreadnought::modEnabled], 0
        je code

        cmp byte ptr [edi+152A0h], 0
        je mov1
        jmp code

    mov1:
        mov byte ptr [edi+152A0h], 1
        jmp code

    code:
        mov eax, [edi+152A0h]
		jmp dword ptr [InfDreadnought::_infDreadnoughtContinue]
    }
}


std::optional<std::string> InfDreadnought::onInitialize()
{
    if (!install_hook_offset(0x3BDB76, hook, &infDreadnought_proc, &InfDreadnought::_infDreadnoughtContinue, 6))
    {
        HL_LOG_ERR("Failed to init InfDreadnought mod\n");
        return "Failed to init InfDreadnought mod";
    }

    return Mod::onInitialize();
}

void InfDreadnought::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3BDCE2, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else
    {
        patch.revert();
    }
}

void InfDreadnought::onGUIframe()
{
    // from main.cpp
    // line 905 -> main->getMods()->onDrawUI("InfDreadnought"_hash);
    if (ImGui::Checkbox("Inf Dreadnought", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void InfDreadnought::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("inf_dreadnought").value_or(false);
    toggle(modEnabled);
};

void InfDreadnought::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("inf_dreadnought", modEnabled);
};

#endif

/*
[ENABLE]
//aobscanmodule(Dreadnought2,DevilMayCry4_DX9.exe,8B 87 A0 52 01 00) // should be unique
define(Dreadnought2,DevilMayCry4_DX9.exe+3BDB76)
alloc(newmem,$1000)
label(code)
label(return)

newmem:
  cmp [edi+000152A0],0
  je mov1
  jmp code

mov1:
  mov [edi+000152A0],1
  jmp code

code:
  mov eax,[edi+000152A0]
  jmp return

Dreadnought2:
  jmp newmem
  nop
return:
registersymbol(Dreadnought2)

{DevilMayCry4_DX9.exe+3BDCE2:
nop
nop
nop
nop
nop
nop
}
[DISABLE]
Dreadnought2:
  db 8B 87 A0 52 01 00
unregistersymbol(Dreadnought2)
dealloc(newmem)

DevilMayCry4_DX9.exe+3BDCE2:
mov [edi+000152A0],eax
*/