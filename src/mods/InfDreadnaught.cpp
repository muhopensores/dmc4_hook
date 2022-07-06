
#include "InfDreadnaught.hpp"

#if 1
bool InfDreadnaught::mod_enabled{ false };
uintptr_t InfDreadnaught::inf_dreadnaught_continue{ NULL };

naked void inf_dreadnaught_proc(void) {
    _asm {
		cmp byte ptr [InfDreadnaught::mod_enabled], 0
        je code

        cmp byte ptr [edi+152A0h], 0
        je mov1
        jmp code

    mov1:
        mov byte ptr [edi+152A0h], 1
        jmp code

    code:
        mov eax, [edi+152A0h]
		jmp dword ptr [InfDreadnaught::inf_dreadnaught_continue]
    }
}

std::optional<std::string> InfDreadnaught::on_initialize() {
    if (!install_hook_offset(0x3BDB76, hook, &inf_dreadnaught_proc, &InfDreadnaught::inf_dreadnaught_continue, 6)) {
        spdlog::error("Failed to init InfDreadnaught mod\n");
        return "Failed to init InfDreadnaught mod";
    }

    return Mod::on_initialize();
}

void InfDreadnaught::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3BDCE2, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch.reset();
    }
}

void InfDreadnaught::on_gui_frame() {
    if (ImGui::Checkbox("Infinite Dreadnaught", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfDreadnaught::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_dreadnaught").value_or(false);
    toggle(mod_enabled);
}

void InfDreadnaught::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_Dreadnaught", mod_enabled);
}

#endif

/*
[ENABLE]
//aobscanmodule(Dreadnaught2,DevilMayCry4_DX9.exe,8B 87 A0 52 01 00) // should be unique
define(Dreadnaught2,DevilMayCry4_DX9.exe+3BDB76)
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

Dreadnaught2:
  jmp newmem
  nop
return:
registersymbol(Dreadnaught2)

{DevilMayCry4_DX9.exe+3BDCE2:
nop
nop
nop
nop
nop
nop
}
[DISABLE]
Dreadnaught2:
  db 8B 87 A0 52 01 00
unregistersymbol(Dreadnaught2)
dealloc(newmem)

DevilMayCry4_DX9.exe+3BDCE2:
mov [edi+000152A0],eax
*/
