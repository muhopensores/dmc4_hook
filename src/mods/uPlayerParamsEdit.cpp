#include "uPlayerParamsEdit.hpp"

uintptr_t uPlayerParamsEdit::jmp_ret1{ NULL };
uintptr_t uPlayerParamsEdit::jmp_ret2{ NULL };
static bool fast_gilg_charge { false };

void __stdcall dante_param_edit(uintptr_t param_table) {
    if (fast_gilg_charge) { // I'd like to keep charge time edits standardized
        *(float*)(param_table+0x31C) = 15.0f; // gilg lvl 1 = 45.0f / 3.0f
        *(float*)(param_table+0x320) = 50.0f; // gilg lvl 2 = 150.0f / 3.0f
        // wtf where is lvl 3
        *(float*)(param_table+0x228) = 20.0f; // gilg lvl 1 rising = 60.0f / 3.0f
        *(float*)(param_table+0x234) = 50.0f; // gilg lvl 2 rising = 150.0f / 3.0f
        // wtf where is lvl 3
    }
}

naked void detour1() { // Dante params
    _asm {
            cmp byte ptr [fast_gilg_charge], 1
            jne originalcode
            // Edit params here
            // mov [edx+whatever]
            pushad
            push edx
            call dante_param_edit
            popad
        originalcode:
            mov [ebx+0x1EA8], edx
            jmp [uPlayerParamsEdit::jmp_ret1]
    }
}

void __stdcall nero_param_edit(float* param_table) {

}

// commented out, nothing uses it yet
naked void detour2() {//Nero params
    _asm {
            // cmp byte ptr [fast_?_charge], 1
            // jne originalcode
            // Edit params here
            // mov [ecx+whatever]
            pushad
            push ecx
            call nero_param_edit
            popad
        // originalcode:
            mov [ebp+0x1EA8], ecx
            jmp [uPlayerParamsEdit::jmp_ret2]
    }
}

std::optional<std::string> uPlayerParamsEdit::on_initialize() {
    if (!install_hook_offset(0x3B4753, hook1, &detour1, &jmp_ret1, 6)) {//Dante params
        spdlog::error("Failed to init uPlayerParamsEdit mod1\n");
        return "Failed to init uPlayerParamsEdit mod1";
    }

    /*if (!install_hook_offset(0x3E3327, hook2, &detour2, &jmp_ret2, 6)) {//Nero params
        spdlog::error("Failed to init uPlayerParamsEdit mod2\n");
        return "Failed to init uPlayerParamsEdit mod2";
    }*/
    return Mod::on_initialize();
}

void uPlayerParamsEdit::on_gui_frame() {
    ImGui::Checkbox(_("Faster Gilgamesh Charges"), &fast_gilg_charge);
    ImGui::SameLine();
    help_marker(_("3x faster Gilgamesh charges\nApplies when loading into a stage"));
}

void uPlayerParamsEdit::on_config_load(const utility::Config& cfg) {
    fast_gilg_charge = cfg.get<bool>("fast_gilg_charge").value_or(false);
}

void uPlayerParamsEdit::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_gilg_charge", fast_gilg_charge);
}
