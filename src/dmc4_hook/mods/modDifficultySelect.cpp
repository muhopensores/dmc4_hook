#include "modDifficultySelect.hpp"

// bool DifficultySelect::modEnabled{ false };

int gameDifficulty = 0;

std::optional<std::string> DifficultySelect::onInitialize()
{
    return Mod::onInitialize();
}

void DifficultySelect::setDefault()
{
    install_patch_offset(0x004D6C7, patch1,  "\x87\x78\xc7", 3);
    install_patch_offset(0x0094AE7, patch2,  "\x14\x77", 2);
    install_patch_offset(0x0094B02, patch3,  "\x28\xad\xcb\x00\x8b\xc7", 6);
    install_patch_offset(0x00B5689, patch4,  "\x24\xf3\xc0\x00", 3);
    install_patch_offset(0x011B9B6, patch5,  "\x74\x0e", 2);
    install_patch_offset(0x011B9BE, patch6,  "\x14\x74\x05\xba\x02", 5);
    install_patch_offset(0x011B9DB, patch7,  "\xfc", 1);
    install_patch_offset(0x011B9E4, patch8,  "\xe8\xbe", 1);
    install_patch_offset(0x011BAFA, patch9,  "\x74", 1);
    install_patch_offset(0x011BB02, patch10, "\x14\x74\x05\xb9\x02", 5);
    install_patch_offset(0x011BB19, patch11, "\x24\xbf", 2);
    install_patch_offset(0x011BB21, patch12, "\x10\xbf", 2);
    install_patch_offset(0x0234D4C, patch13, "\x03\x75", 2);
    install_patch_offset(0x024D80D, patch14, "\x03\x75", 2);
    install_patch_offset(0x02892AB, patch15, "\x03\x75", 2);
    install_patch_offset(0x02ABB41, patch16, "\x03\x75", 2);
    install_patch_offset(0x02C0435, patch17, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x02DA9EB, patch18, "\x03\x75", 2);
    install_patch_offset(0x02FAD4E, patch19, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x0304E9E, patch20, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x03309A0, patch21, "\x03\x74", 2);
    install_patch_offset(0x0343B6A, patch22, "\x24\xf3\xc0", 3);
    install_patch_offset(0x04AB93C, patch23, "\x24\xf3\xc0", 3);
    install_patch_offset(0x0837A5F, patch24, "\x20\x1c", 2);
    install_patch_offset(0x09EC8B4, patch25, "\x00\x00\xe1\x45", 5);
    install_patch_offset(0x09EC9F4, patch26, "\x00\x00\xe1\x45", 5);
    install_patch_offset(0x09ECB34, patch27, "\x00\x00\xe1\x45", 5);
    install_patch_offset(0x09ECC74, patch28, "\x00\x00\xe1\x45", 5);
    install_patch_offset(0x09ECDB4, patch29, "\x00\x00\xe1\x45", 5);
    install_patch_offset(0x011BA85, patch30, "\xc4\xbf", 2);
    install_patch_offset(0x011BA90, patch31, "\xb0\xbf", 2);
    install_patch_offset(0x01D2197, patch32, "\xd0\x28", 2);
    install_patch_offset(0x0234D5B, patch33, "\x44", 1);
    install_patch_offset(0x024D6F6, patch34, "\x24\x5b", 2);
    install_patch_offset(0x024D81C, patch35, "\x44", 1);
    install_patch_offset(0x0254383, patch36, "\x24\x5b", 2);
    install_patch_offset(0x02892BA, patch37, "\x44", 1);
    install_patch_offset(0x02949A4, patch38, "\x02\x00", 2);
    install_patch_offset(0x02ABB50, patch39, "\x44", 1);
    install_patch_offset(0x02C0445, patch40, "\x14", 1);
    install_patch_offset(0x02DA9CA, patch41, "\x5c\xa0", 1);
    install_patch_offset(0x02FAD60, patch42, "\x44", 1);
    install_patch_offset(0x0304EB0, patch43, "\x44", 1);
    install_patch_offset(0x03BFD41, patch44, "\xa8\x27\xd6", 3);
    install_patch_offset(0x03BFD72, patch45, "\xf0\xf2", 2);
    install_patch_offset(0x03C0309, patch46, "\x88\x9f", 2);
    install_patch_offset(0x0378D60, patch47, "\x01", 1);
    install_patch_offset(0x03A7B09, patch48, "\x40", 1);
    install_patch_offset(0x09EC0E0, patch49,"\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\xc0\xa8\x00\x00\x61\x45\x00\x00"
                                            "\xe1\x45\x78\x30\x30\x00\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
                                            "\x00\x00\xe1\x44\x00\x00\x78\x30\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45", 72);

}

void DifficultySelect::setDMD()
{
    install_patch_offset(0x004D6C7, patch1,  "\x05\xa8\xcd\xde\x00", 3);
    install_patch_offset(0x0094AE7, patch2,  "\x00\x77\x14", 2);
    install_patch_offset(0x0094B02, patch3,  "\x50\xb9\xcb\x00\x8b\xc7", 6);
    install_patch_offset(0x00B5689, patch4,  "\x70\x01\xcc\x00", 3);
    install_patch_offset(0x011B9B6, patch5,  "\x72\x0e", 2);
    install_patch_offset(0x011B9BE, patch6,  "\x00\x74\x05\xba\x00", 5);
    install_patch_offset(0x011B9DB, patch7,  "\xf4", 1);
    install_patch_offset(0x011B9E4, patch8,  "\xf4\xbe", 1);
    install_patch_offset(0x011BAFA, patch9,  "\x72", 1);
    install_patch_offset(0x011BB02, patch10, "\x00\x74\x05\xb9\x00", 5);
    install_patch_offset(0x011BB19, patch11, "\x40\xbd", 2);
    install_patch_offset(0x011BB21, patch12, "\x40\xbd", 2);
    install_patch_offset(0x0234D4C, patch13, "\x05\x74", 2);
    install_patch_offset(0x024D80D, patch14, "\x05\x74", 2);
    install_patch_offset(0x02892AB, patch15, "\x05\x74", 2);
    install_patch_offset(0x02ABB41, patch16, "\x05\x74", 2);
    install_patch_offset(0x02C0435, patch17, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02DA9EB, patch18, "\x05\x74", 2);
    install_patch_offset(0x02FAD4E, patch19, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x0304E9E, patch20, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x03309A0, patch21, "\x05\x75", 2);
    install_patch_offset(0x0343B6A, patch22, "\x70\x01\xcc", 3);
    install_patch_offset(0x04AB93C, patch23, "\x70\x01\xcc", 3);
    install_patch_offset(0x0837A5F, patch24, "\x30\x2a", 2);
    install_patch_offset(0x09EC8B4, patch25, "\x00\xc0\x28\x46", 5);
    install_patch_offset(0x09EC9F4, patch26, "\x00\xc0\x28\x46", 5);
    install_patch_offset(0x09ECB34, patch27, "\x00\xc0\x28\x46", 5);
    install_patch_offset(0x09ECC74, patch28, "\x00\xc0\x28\x46", 5);
    install_patch_offset(0x09ECDB4, patch29, "\x00\xc0\x28\x46", 5);
    install_patch_offset(0x011BA85, patch30, "\xc4\xbf", 2);
    install_patch_offset(0x011BA90, patch31, "\xb0\xbf", 2);
    install_patch_offset(0x01D2197, patch32, "\xd0\x28", 2);
    install_patch_offset(0x0234D5B, patch33, "\x44", 1);
    install_patch_offset(0x024D6F6, patch34, "\x24\x5b", 2);
    install_patch_offset(0x024D81C, patch35, "\x44", 1);
    install_patch_offset(0x0254383, patch36, "\x24\x5b", 2);
    install_patch_offset(0x02892BA, patch37, "\x44", 1);
    install_patch_offset(0x02949A4, patch38, "\x02\x00", 2);
    install_patch_offset(0x02ABB50, patch39, "\x44", 1);
    install_patch_offset(0x02C0445, patch40, "\x14", 1);
    install_patch_offset(0x02DA9CA, patch41, "\x5c\xa0", 1);
    install_patch_offset(0x02FAD60, patch42, "\x44", 1);
    install_patch_offset(0x0304EB0, patch43, "\x44", 1);
    install_patch_offset(0x03BFD41, patch44, "\xa8\x27\xd6", 3);
    install_patch_offset(0x03BFD72, patch45, "\xf0\xf2", 2);
    install_patch_offset(0x03C0309, patch46, "\x88\x9f", 2);
    install_patch_offset(0x0378D60, patch47, "\x01", 1);
    install_patch_offset(0x03A7B09, patch48, "\x40", 1);
    install_patch_offset(0x09EC0E0, patch49, "\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\xc0\xa8\x00\x00\x61\x45\x00\x00"
                                             "\xe1\x45\x78\x30\x30\x00\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
                                             "\x00\x00\xe1\x44\x00\x00\x78\x30\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45", 72);
}

void DifficultySelect::setGMD()
{
    install_patch_offset(0x004D6C7, patch1,  "\x05\xa8\xcd\xde\x00", 3);
    install_patch_offset(0x0094AE7, patch2,  "\x00\x77\x14", 2);
    install_patch_offset(0x0094B02, patch3,  "\x50\xb9\xcb\x00\x8b\xc7", 6);
    install_patch_offset(0x00B5689, patch4,  "\xc4\xcd\xde\x00", 3);
    install_patch_offset(0x011B9B6, patch5,  "\x72\x0e", 2);
    install_patch_offset(0x011B9BE, patch6,  "\x00\x74\x05\xba\x00", 5);
    install_patch_offset(0x011B9DB, patch7,  "\x10\xbf", 2);
    install_patch_offset(0x011B9E4, patch8,  "\x10\xbf", 2);
    install_patch_offset(0x011BAFA, patch9,  "\x72", 1);
    install_patch_offset(0x011BB02, patch10, "\x00\x74\x05\xb9\x00", 5);
    install_patch_offset(0x011BB19, patch11, "\x48\xbd", 2);
    install_patch_offset(0x011BB21, patch12, "\x48\xbd", 2);
    install_patch_offset(0x0234D4C, patch13, "\x05\x74", 2);
    install_patch_offset(0x024D80D, patch14, "\x05\x74", 2);
    install_patch_offset(0x02892AB, patch15, "\x05\x74", 2);
    install_patch_offset(0x02ABB41, patch16, "\x05\x74", 2);
    install_patch_offset(0x02C0435, patch17, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02DA9EB, patch18, "\x05\x74", 2);
    install_patch_offset(0x02FAD4E, patch19, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x0304E9E, patch20, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x03309A0, patch21, "\x05\x75", 2);
    install_patch_offset(0x0343B6A, patch22, "\xc4\xcd\xde", 3);
    install_patch_offset(0x04AB93C, patch23, "\xc4\xcd\xde", 3);
    install_patch_offset(0x0837A5F, patch24, "\x50\x46", 2);
    install_patch_offset(0x09EC8B4, patch25, "\x00\xa0\x8c\x46", 5);
    install_patch_offset(0x09EC9F4, patch26, "\x00\xa0\x8c\x46", 5);
    install_patch_offset(0x09ECB34, patch27, "\x00\xa0\x8c\x46", 5);
    install_patch_offset(0x09ECC74, patch28, "\x00\xa0\x8c\x46", 5);
    install_patch_offset(0x09ECDB4, patch29, "\x00\xa0\x8c\x46", 5);
    install_patch_offset(0x011BA85, patch30, "\x04\xc0", 2);
    install_patch_offset(0x011BA90, patch31, "\x04\xc0", 2);
    install_patch_offset(0x01D2197, patch32, "\x14\x2e", 2);
    install_patch_offset(0x0234D5B, patch33, "\x48", 1);
    install_patch_offset(0x024D6F6, patch34, "\xa4\x3e", 2);
    install_patch_offset(0x024D81C, patch35, "\x48", 1);
    install_patch_offset(0x0254383, patch36, "\xa4\x3e", 2);
    install_patch_offset(0x02892BA, patch37, "\x48", 1);
    install_patch_offset(0x02949A4, patch38, "\x01\x00", 2);
    install_patch_offset(0x02ABB50, patch39, "\x48", 1);
    install_patch_offset(0x02C0445, patch40, "\x5a", 1);
    install_patch_offset(0x02DA9CA, patch41, "\xa8\x9e", 2);
    install_patch_offset(0x02FAD60, patch42, "\x48", 1);
    install_patch_offset(0x0304EB0, patch43, "\x48", 1);
    install_patch_offset(0x03BFD41, patch44, "\xf0\xf2\xc0", 3);
    install_patch_offset(0x03BFD72, patch45, "\xfc\xee", 2);
    install_patch_offset(0x03C0309, patch46, "\xff\x87", 2);
    install_patch_offset(0x0378D60, patch47, "\x00", 1);
    install_patch_offset(0x03A7B09, patch48, "\x3c", 1);
    install_patch_offset(0x09EC0E0, patch49, "\x00\xc0\xa8\x44\x00\xc0\xa8\x44\x00\x00\x80\xbf\x00\xc0\x28\x45\x00\x00\xc0\xa8\x00\x00\xe1\x44\x00\x00"
                                             "\x61\x45\x78\x30\x30\x00\x00\xc0\xa8\x44\x00\xa0\x8c\x45\x00\xa0\x0c\x45\x00\x00\xe1\x44\x00\xa0\x8c\x45"
                                             "\x00\x00\x61\x44\x00\x00\x78\x30\x00\x00\xe1\x44\x00\xc0\xa8\x44\x00\xc0\xa8\x44", 72);
}

void DifficultySelect::onConfigLoad(const utils::Config& cfg)
{
    gameDifficulty = cfg.get<int>("game_difficulty").value_or(0);
    if (gameDifficulty)
    {
        switch (gameDifficulty)
        {
        case 0:
            setDefault();
            break;
        case 1:
            setDMD();
            break;
        case 2:
            setGMD();
            break;
        }
    }
}

void DifficultySelect::onConfigSave(utils::Config& cfg)
{
    cfg.set<int>("game_difficulty", gameDifficulty);
}

void DifficultySelect::onGUIframe()
{
    ImGui::PushItemWidth(216);
    if (ImGui::Combo("Game Mode", &gameDifficulty, "Default\0Dante Must Die\0God Must Die\0"))
    {
        switch (gameDifficulty)
        {
        case 0:
            setDefault();
            break;
        case 1:
            setDMD();
            break;
        case 2:
            setGMD();
            break;
        }
    }
    ImGui::PopItemWidth();
    ImGui::Spacing();
    // if (ImGui::Checkbox("Default", &modEnabledDefault))
    // {
    //     toggleDefault(modEnabled);
    // }
    // ImGui::SameLine(205);
}
/*
void DifficultySelect::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("x_x").value_or(false);
    toggle(modEnabled);
};

void DifficultySelect::onConfigSave(utils::Config& cfg)
{
    //cfg.set<bool>("x_x", modEnabled);
};
*/
/*
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
        patch22.revert();
        patch23.revert();
        patch24.revert();
        patch25.revert();
        patch26.revert();
        patch27.revert();
        patch28.revert();
        patch29.revert();
        patch30.revert();
        patch31.revert();
        patch32.revert();
        patch33.revert();
        patch34.revert();
        patch35.revert();
        patch36.revert();
        patch37.revert();
        patch38.revert();
        patch39.revert();
        patch40.revert();
        patch41.revert();
        patch42.revert();
        patch43.revert();
        patch44.revert();
        patch45.revert();
        patch46.revert();
        patch47.revert();
        patch48.revert();
        patch49.revert();
*/