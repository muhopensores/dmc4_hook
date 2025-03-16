#include "uPlayerParamsEdit.hpp"

uintptr_t uPlayerParamsEdit::jmp_ret1{ NULL };
uintptr_t uPlayerParamsEdit::jmp_ret2{ NULL };
static bool fast_gilg_charge { false };

void __stdcall dante_param_edit(uintptr_t param_table) {
    if (fast_gilg_charge) {
        *(float*)(param_table+0x31C) = 15.0f; //lvl 1
        *(float*)(param_table+0x320) = 45.0f; //lvl 2
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

void uPlayerParamsEdit::on_gui_frame() {
    ImGui::Checkbox(_("Fast Gilg charge"), &fast_gilg_charge);
    ImGui::SameLine();
    help_marker(_("Reduce gilgamesh charge time"));
}

void uPlayerParamsEdit::on_config_load(const utility::Config& cfg) {
    fast_gilg_charge = cfg.get<bool>("fast_gilg_charge").value_or(false);
}

void uPlayerParamsEdit::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_gilg_charge", fast_gilg_charge);
}
