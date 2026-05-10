#include "CustomModelFix.hpp"

#if 1
uintptr_t CustomModelFix::jmp_ret1 = NULL;
uintptr_t CustomModelFix::jmp_jb1 = 0x009F46F6;

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

bool CustomModelFix::custom_nero_model_fix  = false;
bool CustomModelFix::custom_dante_model_fix = false;
bool CustomModelFix::force_default_faceplate = false;

naked void detour1(void) { // player in edx
    _asm {
            cmp byte ptr [CustomModelFix::custom_nero_model_fix], 0
            jne detourcode
            cmp byte ptr [CustomModelFix::custom_dante_model_fix], 0
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
            cmp byte ptr [CustomModelFix::custom_nero_model_fix], 1
            jne originalcode
            jmp jecode

        IsDante:
            cmp byte ptr [CustomModelFix::custom_dante_model_fix], 1
            jne originalcode
        jecode:
            jmp dword ptr [CustomModelFix::jmp_jb1]
    }
}

void CustomModelFix::toggle_force_faceplate(bool enable) {
    if (enable) {
        install_patch_offset(0x3B844F, patchFaceplate, "\xbd\x1f\x00\x00\xe0\x90\x90", 7); // mov ebp,E000001F nop nop
    } else {
        patchFaceplate.reset(); // and ebp,[edi+eax*4+000033AC]
    }
}

std::optional<std::string> CustomModelFix::on_initialize() {
    if (!install_hook_offset(0x05F30AD, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init CustomModelFix mod\n");
        return "Failed to init CustomModelFix mod";
    }
    return Mod::on_initialize();
}

void CustomModelFix::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Custom Nero Model Fix"), &custom_nero_model_fix);
        ImGui::SameLine();
        help_marker(_("Remove exploding shadows on Nero mods"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Custom Dante Model Fix"), &custom_dante_model_fix);
        ImGui::SameLine();
        help_marker(_("Remove exploding shadows on Dante mods"));

        if (ImGui::Checkbox(_("Force Default Face Plate"), &force_default_faceplate)) {
            toggle_force_faceplate(force_default_faceplate);
        }
        ImGui::SameLine();
        help_marker(_("Force the character face that shows when not attacking or getting hurt so cutscene face mods display correctly"));
    }
}

void CustomModelFix::on_config_load(const utility::Config& cfg) {
    custom_nero_model_fix = cfg.get<bool>("custom_nero_model_fix").value_or(false);
    custom_dante_model_fix = cfg.get<bool>("custom_dante_model_fix").value_or(false);
    force_default_faceplate = cfg.get<bool>("force_default_faceplate").value_or(false);
}

void CustomModelFix::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("custom_nero_model_fix", custom_nero_model_fix);
    cfg.set<bool>("custom_dante_model_fix", custom_dante_model_fix);
    cfg.set<bool>("force_default_faceplate", force_default_faceplate);
}

#endif
