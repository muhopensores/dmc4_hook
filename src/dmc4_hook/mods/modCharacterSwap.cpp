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
        install_patch_offset(0x03790CF, patch1,  "\x74\x46", 2);
        install_patch_offset(0x087825C, patch2,  "\x01\x00", 2);
        install_patch_offset(0x04ABC19, patch3,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ABE59, patch4,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ABFB9, patch5,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC109, patch6,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC259, patch7,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC3A9, patch8,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC4F9, patch9,  "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC649, patch10, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC769, patch11, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC889, patch12, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AC999, patch13, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ACAE9, patch14, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ACC39, patch15, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ACD99, patch16, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04ACF19, patch17, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AD069, patch18, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AD1D9, patch19, "\x01\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AD339, patch20, "\x00\x8b\xc6\x5e\xc3\x33", 6);
        install_patch_offset(0x04AD449, patch21, "\x00\x8b\xc6\x5e\xc3\x33", 6);
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
