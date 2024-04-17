#include "CharacterSwap.hpp"

bool CharacterSwap::nero_enabled{ false };
bool CharacterSwap::dante_enabled{ false };

std::optional<std::string> CharacterSwap::on_initialize()
{
    return Mod::on_initialize();
}
/*
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
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
        patch8.reset();
        patch9.reset();
        patch10.reset();
        patch11.reset();
        patch12.reset();
        patch13.reset();
        patch14.reset();
        patch15.reset();
        patch16.reset();
        patch17.reset();
        patch18.reset();
        patch19.reset();
        patch20.reset();
        patch21.reset();
    }
}
*/

void CharacterSwap::toggle_nero(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03790CF, patch1,  "\x74\x46", 2); // jmp char selection
        install_patch_offset(0x087825C, patch2,  "\x01", 1);  // mission 01
        install_patch_offset(0x04ABC19, patch3,  "\x01", 1);  // mission 02
        install_patch_offset(0x04ABE59, patch4,  "\x01", 1);  // mission 03
        install_patch_offset(0x04ABFB9, patch5,  "\x01", 1);  // mission 04
        install_patch_offset(0x04AC109, patch6,  "\x01", 1);  // mission 05
        install_patch_offset(0x04AC259, patch7,  "\x01", 1);  // mission 06
        install_patch_offset(0x04AC3A9, patch8,  "\x01", 1);  // mission 07
        install_patch_offset(0x04AC4F9, patch9,  "\x01", 1);  // mission 08
        install_patch_offset(0x04AC649, patch10, "\x01", 1);  // mission 09
        install_patch_offset(0x04AC769, patch11, "\x01", 1);  // mission 10
        install_patch_offset(0x04AC889, patch12, "\x01", 1);  // mission 11
        install_patch_offset(0x04AC999, patch13, "\x01", 1);  // mission 12
        install_patch_offset(0x04ACAE9, patch14, "\x01", 1);  // mission 13
        install_patch_offset(0x04ACC39, patch15, "\x01", 1);  // mission 14
        install_patch_offset(0x04ACD99, patch16, "\x01", 1);  // mission 15
        install_patch_offset(0x04ACF19, patch17, "\x01", 1);  // mission 16
        install_patch_offset(0x04AD069, patch18, "\x01", 1);  // mission 17
        install_patch_offset(0x04AD1D9, patch19, "\x01", 1);  // mission 18
        install_patch_offset(0x04AD339, patch20, "\x01", 1);  // mission 19
        install_patch_offset(0x04AD449, patch21, "\x01", 1);  // mission 20
    }
    else
    {
        patch1.reset();
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
        patch8.reset();
        patch9.reset();
        patch10.reset();
        patch11.reset();
        patch12.reset();
        patch13.reset();
        patch14.reset();
        patch15.reset();
        patch16.reset();
        patch17.reset();
        patch18.reset();
        patch19.reset();
        patch20.reset();
        patch21.reset();
    }
}

void CharacterSwap::toggle_dante(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03790CF, patch1,  "\x90\x90", 2); // nop 2 char selection
        install_patch_offset(0x087825C, patch2,  "\x00", 1); // mission 01 does not work :(
        install_patch_offset(0x04ABC19, patch3,  "\x00", 1); // mission 02
        install_patch_offset(0x04ABE59, patch4,  "\x00", 1); // mission 03
        install_patch_offset(0x04ABFB9, patch5,  "\x00", 1); // mission 04
        install_patch_offset(0x04AC109, patch6,  "\x00", 1); // mission 05
        install_patch_offset(0x04AC259, patch7,  "\x00", 1); // mission 06
        install_patch_offset(0x04AC3A9, patch8,  "\x00", 1); // mission 07
        install_patch_offset(0x04AC4F9, patch9,  "\x00", 1); // mission 08
        install_patch_offset(0x04AC649, patch10, "\x00", 1); // mission 09
        install_patch_offset(0x04AC769, patch11, "\x00", 1); // mission 10
        install_patch_offset(0x04AC889, patch12, "\x00", 1); // mission 11
        install_patch_offset(0x04AC999, patch13, "\x00", 1); // mission 12
        install_patch_offset(0x04ACAE9, patch14, "\x00", 1); // mission 13
        install_patch_offset(0x04ACC39, patch15, "\x00", 1); // mission 14
        install_patch_offset(0x04ACD99, patch16, "\x00", 1); // mission 15
        install_patch_offset(0x04ACF19, patch17, "\x00", 1); // mission 16
        install_patch_offset(0x04AD069, patch18, "\x00", 1); // mission 17
        install_patch_offset(0x04AD1D9, patch19, "\x00", 1); // mission 18
        install_patch_offset(0x04AD339, patch20, "\x00", 1); // mission 19
        install_patch_offset(0x04AD449, patch21, "\x00", 1); // mission 20
    }
    else
    {
        patch1.reset();
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
        patch8.reset();
        patch9.reset();
        patch10.reset();
        patch11.reset();
        patch12.reset();
        patch13.reset();
        patch14.reset();
        patch15.reset();
        patch16.reset();
        patch17.reset();
        patch18.reset();
        patch19.reset();
        patch20.reset();
        patch21.reset();
    }
}

void CharacterSwap::on_gui_frame()
{
    if (ImGui::Checkbox("Force Nero", &nero_enabled))
    {
        dante_enabled = false;
        toggle_dante(dante_enabled);
        toggle_nero(nero_enabled);
    }
    ImGui::SameLine();
    help_marker("At the start of a level, you will spawn as whichever character is checked. Does not work for M1");
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Force Dante", &dante_enabled))
    {
        nero_enabled = false;
        toggle_nero(nero_enabled);
        toggle_dante(dante_enabled);
    }
}

void CharacterSwap::on_config_load(const utility::Config& cfg)
{
    nero_enabled = cfg.get<bool>("force_nero").value_or(false);
    toggle_nero(nero_enabled);
    dante_enabled = cfg.get<bool>("force_dante").value_or(false);
    toggle_dante(dante_enabled);
};

void CharacterSwap::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("force_nero", nero_enabled);
    cfg.set<bool>("force_dante", dante_enabled);
};
