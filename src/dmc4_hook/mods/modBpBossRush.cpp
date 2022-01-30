#include "modBpBossRush.hpp"
#include "modBpJumpHook.hpp"
// THIS ISN'T USED SO DON'T EDIT IT THEN WONDER WHY IT'S NOT CHANGING ANYTHING
// Look in BpJumpHook instead
bool BpBossRush::modEnabled{ false };

std::optional<std::string> BpBossRush::onInitialize()
{
    return Mod::onInitialize();
}

void BpBossRush::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x0101AA4, patch1, "\x72\x06", 2);
        install_patch_offset(0x04AB8E2, patch2, "\x68\x65\xE5\x00\x8B", 5); // makes bp load stage 20 rather than 1
        install_patch_offset(0x04AB8FD, patch3, "\x14\x00", 2);
        install_patch_offset(0x0837BAC, patch4, "\x28\x00\xf8\x01", 5);
        install_patch_offset(0x0837CCA, patch5, "\x3C\x00\xf9\x01", 5);
        install_patch_offset(0x0837DE8, patch6, "\x50\x00\xfb\x01", 5);
        install_patch_offset(0x0837F07, patch7, "\x64\x00\xfa\x01", 5);
    }
    else
    {
        patch1.revert();
        patch2.revert();
        patch3.revert();
        patch4.revert();
        patch5.revert();
        patch6.revert();
        patch7.revert();
    }
}

void BpBossRush::onGUIframe()
{
    if (ImGui::Checkbox("BP Boss Rush", &modEnabled))
    {
        toggle(modEnabled);
        BpJumpHook::modEnabled = 0;
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Activate before starting BP to be teleported only to boss stages");
}

void BpBossRush::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("bp_boss_rush").value_or(false);
    toggle(modEnabled);
    if (modEnabled)
    {
        BpJumpHook::modEnabled = false;
    }
}

void BpBossRush::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("bp_boss_rush", modEnabled);
}