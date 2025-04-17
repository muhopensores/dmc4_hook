#include "FpsLimit.hpp"

#if 1
bool FpsLimit::mod_enabled{ false };
uintptr_t FpsLimit::jmp_return{ NULL };
float FpsLimit::newfpslimit{ 120.0f };

naked void detour() {
    _asm {
        cmp byte ptr [FpsLimit::mod_enabled], 0
        je code

        divss xmm1, [FpsLimit::newfpslimit]
        jmp dword ptr [FpsLimit::jmp_return]

    code:
        divss xmm1, [esi+0x3C]
		jmp dword ptr [FpsLimit::jmp_return]
    }
}

std::optional<std::string> FpsLimit::on_initialize() {
    if (!install_hook_offset(0x50D89C, hook, &detour, &jmp_return, 5)) {
        spdlog::error("Failed to init FpsLimit mod\n");
        return "Failed to init FpsLimit mod";
    }

    return Mod::on_initialize();
}

void FpsLimit::on_gui_frame(int display) {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Override FPS Limit"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("If your FPS is set to unlocked, this will allow you to change the cap. Some things work differently at certain framerates"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::InputFloat(_("FPS Limit"), &newfpslimit, 1.0f, 1.0f, "%.0f");
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
}

void FpsLimit::on_config_load(const utility::Config& cfg) {
    mod_enabled  = cfg.get<bool>("fps_limit").value_or(false);
	newfpslimit = cfg.get<float>("new_fps_limit").value_or(120.0f);
}

void FpsLimit::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fps_limit", mod_enabled);
    cfg.set<float>("new_fps_limit", newfpslimit);
}

#endif
