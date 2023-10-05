#include "CustomModelFix.hpp"

#if 1
uintptr_t CustomModelFix::jmp_ret1{NULL};
uintptr_t CustomModelFix::jmp_jb1{0x009F46F6};

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

bool CustomModelFix::char_nero{false};
bool CustomModelFix::char_dante{false};

naked void detour1(void) { // player in edx
    _asm {
            cmp byte ptr [CustomModelFix::char_nero], 0
            jne detourcode
            cmp byte ptr [CustomModelFix::char_dante], 0
            je originalcode

        detourcode:
            push ecx
            mov ecx, [static_mediator_ptr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            cmp edx, ecx
            pop ecx
            je charcheck // is player character

	    originalcode:
		    test [edx+0x00000138], eax
		    jmp dword ptr [CustomModelFix::jmp_ret1]

        charcheck:
            cmp dword ptr [edx+0x1494], 0 // controller id
            je IsDante
            cmp byte ptr [CustomModelFix::char_nero], 1
            jne originalcode
            jmp jecode
        IsDante:
            cmp byte ptr [CustomModelFix::char_dante], 1
            jne originalcode
            jmp jecode

        jecode:
            jmp dword ptr [CustomModelFix::jmp_jb1]
    }
}

std::optional<std::string> CustomModelFix::on_initialize() {
    if (!install_hook_offset(0x05F30AD, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init CustomModelFix mod\n");
        return "Failed to init CustomModelFix mod";
    }
    return Mod::on_initialize();
}

void CustomModelFix::on_gui_frame() {
    ImGui::Checkbox("Custom Nero Model Fix", &char_nero);
    ImGui::SameLine();
    help_marker("Remove exploding shadows on Nero mods");
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox("Custom Dante Model Fix", &char_dante);
    ImGui::SameLine();
    help_marker("Remove exploding shadows on Dante mods");
}

void CustomModelFix::on_config_load(const utility::Config& cfg) {
    char_nero = cfg.get<bool>("custom_nero_model_fix").value_or(false);
    char_dante = cfg.get<bool>("custom_dante_model_fix").value_or(false);
}

void CustomModelFix::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("custom_nero_model_fix", char_nero);
    cfg.set<bool>("custom_dante_model_fix", char_dante);
}

#endif
