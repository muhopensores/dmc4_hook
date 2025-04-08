#include "CutscenePause.hpp"

bool  CutscenePause::mod_enabled{ false };
uintptr_t CutscenePause::jmp_ret{ NULL };
float CutscenePause::cutsceneProgress = 0.0f;
float CutscenePause::cutsceneLength = 10000.0f;
bool wasToggled = false;

naked void detour1(void) {
    _asm {
        cmp byte ptr [CutscenePause::mod_enabled], 0
        je code

        cmp byte ptr [wasToggled], 1
        jne writeFloat
        mov byte ptr [wasToggled], 0
        movss [CutscenePause::cutsceneProgress], xmm0 // backup current pause time
        writeFloat:
        movss xmm0, [CutscenePause::cutsceneProgress]

        code:
        movss [esi+0x1C], xmm0
        jmp dword ptr [CutscenePause::jmp_ret]
    }
}

void CutscenePause::on_gui_frame() {
    if (ImGui::Checkbox(_("Pause Cutscene"), &mod_enabled)) {
        wasToggled = true;
    }
    ImGui::SameLine();
    help_marker(_("The hotkey is PAUSE BREAK by default"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::InputFloat(_("Cutscene Length"), &cutsceneLength, 0.0f, 10.0f, "%.2f");
        ImGui::SameLine();
        help_marker(_("This controls the extent of the next slider"));
        ImGui::SliderFloat(_("Cutscene Progress"), &cutsceneProgress, 0.0f, cutsceneLength, "%.2f");
        ImGui::SameLine();
        help_marker(_("This doesn't seem to like time being reversed"));
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }
}

std::optional<std::string> CutscenePause::on_initialize() {
    if (!install_hook_offset(0x50A8AB, hook1, &detour1, &CutscenePause::jmp_ret, 5)) {
        spdlog::error("Failed to init CutscenePause mod\n");
        return "Failed to init CutscenePause mod";
    }
    utility::create_keyboard_hotkey(m_hotkeys, {VK_PAUSE}, "Pause Cutscene", "pause_cutscene");
    return Mod::on_initialize();
}

void CutscenePause::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        CutscenePause::mod_enabled = !CutscenePause::mod_enabled;
        wasToggled = true;
    }
}

/*void CutscenePause::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("cutscene_pause").value_or(false);
}

void CutscenePause::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("cutscene_pause", mod_enabled);
}*/
