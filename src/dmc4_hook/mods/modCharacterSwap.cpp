#include "modCharacterSwap.hpp"

bool CharacterSwap::modEnabled{ false };

std::optional<std::string> CharacterSwap::onInitialize()
{
    return Mod::onInitialize();
}

void CharacterSwap::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3790CF, patch1,  "\x74\x46", 2);
        install_patch_offset(0x87825C, patch2,  "\x01\x00", 2);
        install_patch_offset(0x4ABC19, patch3,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ABE59, patch4,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ABFB9, patch5,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC109, patch6,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC259, patch7,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC3A9, patch8,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC4F9, patch9,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC649, patch10, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC769, patch11, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC889, patch12, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AC999, patch13, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ACAE9, patch14, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ACC39, patch15, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ACD99, patch16, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4ACF19, patch17, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AD069, patch18, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AD1D9, patch19, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AD339, patch20, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x4AD449, patch21, "\x00\x8b\xc6\x5e\xc3\x33", 6);
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
        patch8.revert();
        patch9.revert();
        patch10.revert();
        patch11.revert();
        patch12.revert();
        patch13.revert();
        patch14.revert();
        patch15.revert();
        patch16.revert();
        patch17.revert();
        patch18.revert();
        patch19.revert();
        patch20.revert();
        patch21.revert();
    }
}

void CharacterSwap::onGUIframe()
{
    if (ImGui::Checkbox("Character Swap", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void CharacterSwap::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("character_swap").value_or(false);
    toggle(modEnabled);
};

void CharacterSwap::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("character_swap", modEnabled);
};
