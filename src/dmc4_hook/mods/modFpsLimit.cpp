#include "modFpsLimit.hpp"

#if 1
bool FpsLimit::modEnabled{ false };
uintptr_t FpsLimit::jmp_return{ NULL };
float FpsLimit::newfpslimit{ 120.0f };

FpsLimit::FpsLimit()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        cmp byte ptr [FpsLimit::modEnabled], 0
        je code

        divss xmm1, [FpsLimit::newfpslimit]
        jmp dword ptr [FpsLimit::jmp_return]

    code:
        divss xmm1, [esi+3Ch]
		jmp dword ptr [FpsLimit::jmp_return]
    }
}

std::optional<std::string> FpsLimit::onInitialize()
{
    if (!install_hook_offset(0x50D89C, hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init FpsLimit mod\n");
        return "Failed to init FpsLimit mod";
    }

    return Mod::onInitialize();
}

void FpsLimit::onGUIframe()
{
    // from main.cpp
    // line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
    ImGui::Checkbox("Override FPS Limit", &modEnabled);
    ImGui::PushItemWidth(217);
    ImGui::InputFloat("New FPS Limit", &newfpslimit, 1.0f, 1.0f, "%.0f");
    ImGui::PopItemWidth();
    ImGui::Spacing();
}

void FpsLimit::onConfigLoad(const utils::Config& cfg)
{
    modEnabled  = cfg.get<bool>("fps_limit").value_or(false);
	newfpslimit = cfg.get<float>("new_fps_limit").value_or(120.0f);
};

void FpsLimit::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("fps_limit", modEnabled);
    cfg.set<float>("new_fps_limit", newfpslimit);
};

#endif