#include "uPlayerParamsEdit.hpp"

uintptr_t uPlayerParamsEdit::jmp_ret1{ NULL };
uintptr_t uPlayerParamsEdit::jmp_ret2{ NULL };
static bool faster_gilg_charge { false };

void __stdcall dante_param_edit(uintptr_t param_table) {
    if (faster_gilg_charge) {
        *(float*)(param_table+0x31C) = 15.0f;
        *(float*)(param_table+0x320) = 45.0f;
    }
}

naked void detour1() {//Dante params
    _asm {
        //Edit params here
        //mov [edx+whatever]
            pushad
            push edx
            call dante_param_edit
            popad
        //originalcode
            mov [ebx+0x1EA8],edx
            jmp [uPlayerParamsEdit::jmp_ret1];
    }
}

void __stdcall nero_param_edit(float* param_table) {

}

naked void detour2() {//Nero params
    _asm {
        //Edit params here
        //mov [ecx+whatever]
            pushad
            push ecx
            call nero_param_edit
            popad
        //originalcode
            mov [ebp+0x1EA8],ecx
            jmp [uPlayerParamsEdit::jmp_ret2];
    }
}

std::optional<std::string> uPlayerParamsEdit::on_initialize() {
    if (!install_hook_offset(0x3B4753, hook1, &detour1, &jmp_ret1, 6)) {//Dante params
        spdlog::error("Failed to init uPlayerParamsEdit mod1\n");
        return "Failed to init uPlayerParamsEdit mod1";
    }

    if (!install_hook_offset(0x3E3327, hook2, &detour2, &jmp_ret2, 6)) {//Nero params
        spdlog::error("Failed to init uPlayerParamsEdit mod2\n");
        return "Failed to init uPlayerParamsEdit mod2";
    }
    return Mod::on_initialize();
}