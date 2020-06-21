// include your mod header file
#include "modHeightRestrictionNero.hpp"

#if 1
// static variable defined in cpp file makes it local to current file
bool HeightRestrictionNero::modEnabled{ false };

// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> HeightRestrictionNero::onInitialize() {

    return Mod::onInitialize();
}

void HeightRestrictionNero::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3E614B, patchBuster, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5F8D, patchSplit, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E6248, patchCalibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E62B6, patchExCalibur, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E60E4, patchSnatch, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E603F, patchRave, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E5FE1, patchDoubleDown, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x3E4B12, patchRev, "\x90\x90", 2);
    }
    else
    {
        patchBuster.revert();
        patchSplit.revert();
        patchCalibur.revert();
        patchExCalibur.revert();
        patchSnatch.revert();
        patchRave.revert();
        patchDoubleDown.revert();
        patchRev.revert();
    }
}

void HeightRestrictionNero::onGUIframe()
{
    if (ImGui::Checkbox("Nero", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void HeightRestrictionNero::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("nero_height_restriction_removed").value_or(false);
    toggle(modEnabled);
};

void HeightRestrictionNero::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("nero_height_restriction_removed", modEnabled);
};

#endif