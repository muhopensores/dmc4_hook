#include "modHideHud.hpp"

bool HideHud::modEnabledHealth{ false };
bool HideHud::modEnabledOrbs{ false };
bool HideHud::modEnabledStyle{ false };
bool HideHud::modEnabledTimer{ false };

std::optional<std::string> HideHud::onInitialize()
{
    return Mod::onInitialize();
}

void HideHud::toggleHealth(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x00FEFE5, patch1, "\x73\x2E", 2); // Hides health
    }
    else
    {
        patch1.revert();
    }
}
void HideHud::toggleOrbs(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x00FF1F0, patch2, "\x73\x76", 2); // Hide Orbs
    }
    else
    {
        patch2.revert();
    }
}
void HideHud::toggleStyle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x01017F8, patch3, "\x0F\x83\x5B\x03\x00\x00", 6); // Hide Style
    }
    else
    {
        patch3.revert();
    }
}
void HideHud::toggleTimer(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x0FDE2A, patchbp, "\x00", 1);
        install_patch_offset(0x0FE298, patchm12, "\x00", 1);
        // install_patch_offset(0x50A8E6, patchm12flash, "\x90\x90\x90\x90\x90", 5);
        install_patch_offset(0x0FE088, patchbpplus, "\x90\x90\x90\x90", 4);
    }
    else
    {
        patchbp.revert();
        patchm12.revert();
        // patchm12flash.revert();
        patchbpplus.revert();
    }
}

void HideHud::onGUIframe()
{
    if (ImGui::Checkbox("Hide HP HUD", &modEnabledHealth))
    {
        toggleHealth(modEnabledHealth);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Hide Orbs HUD", &modEnabledOrbs))
    {
        toggleOrbs(modEnabledOrbs);
    }
    if (ImGui::Checkbox("Hide Style HUD", &modEnabledStyle))
    {
        toggleStyle(modEnabledStyle);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Hide Timer", &modEnabledTimer))
    {
        toggleTimer(modEnabledTimer);
    }
}

void HideHud::onConfigLoad(const utils::Config& cfg)
{
    modEnabledHealth = cfg.get<bool>("hide_hp_hud").value_or(false);
    toggleHealth(modEnabledHealth);
    modEnabledOrbs = cfg.get<bool>("hide_orbs_hud").value_or(false);
    toggleOrbs(modEnabledOrbs);
    modEnabledStyle = cfg.get<bool>("hide_style_hud").value_or(false);
    toggleStyle(modEnabledStyle);
    modEnabledTimer = cfg.get<bool>("hide_timer_hud").value_or(false);
    toggleTimer(modEnabledTimer);
};

void HideHud::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("hide_hp_hud", modEnabledHealth);
    cfg.set<bool>("hide_orbs_hud", modEnabledOrbs);
    cfg.set<bool>("hide_style_hud", modEnabledStyle);
    cfg.set<bool>("hide_timer_hud", modEnabledTimer);
};

/*
void hlMain::ToggleHideHUD(bool toggle)
{
    if (hideHUDOne)
    {
        if (toggle)
            hideHUDOne_patch.apply(hideHUDOne, "\x73\x2E", 2);
        else
            hideHUDOne_patch.revert();
    }

    if (hideHUDTwo)
    {
        if (toggle)
            hideHUDTwo_patch.apply(hideHUDTwo, "\x73\x76", 2);
        if (toggle)
            hideOrbs_patch.apply(hideOrbs, "\x80\xbf\xfe\x00\x00\x00\x00", 7);
        else
            hideHUDTwo_patch.revert();
    }

    if (hideHUDThree)
    {
        if (toggle)
            hideHUDThree_patch.apply(hideHUDThree, "\x0F\x83\x5B\x03\x00\x00", 6);
        else
            hideHUDThree_patch.apply(hideHUDThree, "\x0F\x84\x5B\x03\x00\x00", 6);
    }
}
*/