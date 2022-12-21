#include "DifficultySelect.hpp"

// bool DifficultySelect::modEnabled{ false };

static int game_difficulty = 0;

std::optional<std::string> DifficultySelect::on_initialize() {
    MutatorRegistry::define("DanteMustDieDifficulty").alias("SetDMD")
        .description("hehe")
        .on_init([&] { set_dmd(); })
        .set_timer(35.0, [&] {set_default(); });

    MutatorRegistry::define("GodMustDie").alias("SetGMD")
        .description("hehe")
        .on_init([&] { set_gmd(); })
        .set_timer(35.0, [&] {set_default(); });
        
    return Mod::on_initialize();
}

void DifficultySelect::reset_patches() {
    patch1.reset(nullptr);
    patch2.reset(nullptr);
    patch3.reset(nullptr);
    patch4.reset(nullptr);
    patch5.reset(nullptr);
    patch6.reset(nullptr);
    patch7.reset(nullptr);
    patch8.reset(nullptr);
    patch9.reset(nullptr);
    patch10.reset(nullptr);
    patch11.reset(nullptr);
    patch12.reset(nullptr);
    patch13.reset(nullptr);
    patch14.reset(nullptr);
    patch15.reset(nullptr);
    patch16.reset(nullptr);
    patch17.reset(nullptr);
    patch18.reset(nullptr);
    patch19.reset(nullptr);
    patch20.reset(nullptr);
    patch21.reset(nullptr);
    patch22.reset(nullptr);
    patch23.reset(nullptr);
    patch24.reset(nullptr);
    patch25.reset(nullptr);
    patch26.reset(nullptr);
    patch27.reset(nullptr);
    patch28.reset(nullptr);
    patch29.reset(nullptr);
    patch30.reset(nullptr);
    patch31.reset(nullptr);
    patch32.reset(nullptr);
    patch33.reset(nullptr);
    patch34.reset(nullptr);
    patch35.reset(nullptr);
    patch36.reset(nullptr);
    patch37.reset(nullptr);
    patch38.reset(nullptr);
    patch39.reset(nullptr);
    patch40.reset(nullptr);
    patch41.reset(nullptr);
    patch42.reset(nullptr);
    patch43.reset(nullptr);
    patch44.reset(nullptr);
    patch45.reset(nullptr);
    patch46.reset(nullptr);
    patch47.reset(nullptr);
    patch48.reset(nullptr);
    patch49.reset(nullptr);
}

void DifficultySelect::set_default() {
    install_patch_offset(0x004D6C7, patch1, "\x87\x78\xC7", 3);
    install_patch_offset(0x0094AE7, patch2, "\x14", 1);
    install_patch_offset(0x0094B02, patch3, "\x28\xAD", 2);
    install_patch_offset(0x00B5689, patch4, "\x24\xF3\xC0\x00", 4);
    install_patch_offset(0x011B9B6, patch5, "\x74", 1);
    install_patch_offset(0x011B9BE, patch6, "\x14\x74\x05\xBA\x02", 5);
    install_patch_offset(0x011B9DB, patch7, "\xFC\xBE", 2);
    install_patch_offset(0x011B9E4, patch8, "\xE8\xBE", 2);
    install_patch_offset(0x011BA85, patch9, "\xC4\xBF", 2);
    install_patch_offset(0x011BA90, patch10, "\xB0\xBF", 2);
    install_patch_offset(0x011BAFA, patch11, "\x74", 1);
    install_patch_offset(0x011BB02, patch12, "\x14\x74\x05\xB9\x02", 5);
    install_patch_offset(0x011BB19, patch13, "\x24\xBF", 2);
    install_patch_offset(0x011BB21, patch14, "\x10\xBF", 2);
    install_patch_offset(0x01D2197, patch15, "\xD0\x28", 2);
    install_patch_offset(0x0234D4C, patch16, "\x03\x75", 2);
    install_patch_offset(0x0234D5B, patch17, "\x44", 1);
    install_patch_offset(0x024D6F6, patch18, "\x24\x5B", 2);
    install_patch_offset(0x024D80D, patch19, "\x03\x75", 2);
    install_patch_offset(0x024D81C, patch20, "\x44", 1);
    install_patch_offset(0x0254383, patch21, "\x24\x5b", 2);
    install_patch_offset(0x02892AB, patch22, "\x03\x75", 2);
    install_patch_offset(0x02892BA, patch23, "\x44", 1);
    install_patch_offset(0x02949A4, patch24, "\x02", 1);
    install_patch_offset(0x02ABB41, patch25, "\x03\x75", 2);
    install_patch_offset(0x02ABB50, patch26, "\x44", 1);
    install_patch_offset(0x02C0435, patch27, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x02C0445, patch28, "\x14", 1);
    install_patch_offset(0x02DA9CA, patch29, "\x5C\xA0", 2);
    install_patch_offset(0x02DA9EB, patch30, "\x03\x75", 2);
    install_patch_offset(0x02FAD4E, patch31, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x02FAD60, patch32, "\x44", 1);
    install_patch_offset(0x0304E9E, patch33, "\x40\x01\x00\x00\x75", 5);
    install_patch_offset(0x0304EB0, patch34, "\x44", 1);
    install_patch_offset(0x03309A0, patch35, "\x03\x74", 2);
    install_patch_offset(0x0343B6A, patch36, "\x24\xF3\xC0\x00", 4);
    install_patch_offset(0x03BFD41, patch37, "\xA8\x27\xD6", 3);
    install_patch_offset(0x03BFD72, patch38, "\xF0\xF2", 2);
    install_patch_offset(0x03C0309, patch39, "\x88\x9F", 2);
    install_patch_offset(0x0378D60, patch40, "\x01", 1);
    install_patch_offset(0x03A7B09, patch41, "\x40", 1);
    install_patch_offset(0x04AB93C, patch42, "\x24\xF3\xC0\x00", 4);
    install_patch_offset(0x0837A5F, patch43, "\x20\x1C", 2);
    install_patch_offset(0x09EC0E0, patch44,
        "\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\x80\xBF\x00\x00\x61\x45\x00\x00"
        "\xe1\x45\x00\x00\x80\xBF\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
        "\x00\x00\xe1\x44\x00\x00\x80\xBF\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45",
        72);
    install_patch_offset(0x09EC8B4, patch45, "\x00\x00\xe1\x45", 4);
    install_patch_offset(0x09EC9F4, patch46, "\x00\x00\xe1\x45", 4);
    install_patch_offset(0x09ECB34, patch47, "\x00\x00\xe1\x45", 4);
    install_patch_offset(0x09ECC74, patch48, "\x00\x00\xe1\x45", 4);
    install_patch_offset(0x09ECDB4, patch49, "\x00\x00\xe1\x45", 4);
}

void DifficultySelect::set_dmd() {
    install_patch_offset(0x004D6C7, patch1, "\x05\xA8\xCD", 3);
    install_patch_offset(0x0094AE7, patch2, "\x00", 1);
    install_patch_offset(0x0094B02, patch3, "\x50\xB9", 2);
    install_patch_offset(0x00B5689, patch4, "\x70\x01\xCC\x00", 4);
    install_patch_offset(0x011B9B6, patch5, "\x72", 1);
    install_patch_offset(0x011B9BE, patch6, "\x00\x74\x05\xBA\x00", 5);
    install_patch_offset(0x011B9DB, patch7, "\xF4\xBE", 2);
    install_patch_offset(0x011B9E4, patch8, "\xF4\xBE", 2);
    install_patch_offset(0x011BA85, patch9, "\xC4\xBF", 2);
    install_patch_offset(0x011BA90, patch10, "\xB0\xBF", 2);
    install_patch_offset(0x011BAFA, patch11, "\x72", 1);
    install_patch_offset(0x011BB02, patch12, "\x00\x74\x05\xB9\x00", 5);
    install_patch_offset(0x011BB19, patch13, "\x40\xBD", 2);
    install_patch_offset(0x011BB21, patch14, "\x40\xBD", 2);
    install_patch_offset(0x01D2197, patch15, "\xD0\x28", 2);
    install_patch_offset(0x0234D4C, patch16, "\x05\x74", 2);
    install_patch_offset(0x0234D5B, patch17, "\x44", 1);
    install_patch_offset(0x024D6F6, patch18, "\x24\x5B", 2);
    install_patch_offset(0x024D80D, patch19, "\x05\x74", 2);
    install_patch_offset(0x024D81C, patch20, "\x44", 1);
    install_patch_offset(0x0254383, patch21, "\x24\x5b", 2);
    install_patch_offset(0x02892AB, patch22, "\x05\x74", 2);
    install_patch_offset(0x02892BA, patch23, "\x44", 1);
    install_patch_offset(0x02949A4, patch24, "\x02", 1);
    install_patch_offset(0x02ABB41, patch25, "\x05\x74", 2);
    install_patch_offset(0x02ABB50, patch26, "\x44", 1);
    install_patch_offset(0x02C0435, patch27, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02C0445, patch28, "\x14", 1);
    install_patch_offset(0x02DA9CA, patch29, "\x5C\xA0", 2);
    install_patch_offset(0x02DA9EB, patch30, "\x05\x74", 2);
    install_patch_offset(0x02FAD4E, patch31, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02FAD60, patch32, "\x44", 1);
    install_patch_offset(0x0304E9E, patch33, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x0304EB0, patch34, "\x44", 1);
    install_patch_offset(0x03309A0, patch35, "\x05\x75", 2);
    install_patch_offset(0x0343B6A, patch36, "\x70\x01\xCC\x00", 4);
    install_patch_offset(0x03BFD41, patch37, "\xA8\x27\xD6", 3);
    install_patch_offset(0x03BFD72, patch38, "\xF0\xF2", 2);
    install_patch_offset(0x03C0309, patch39, "\x88\x9F", 2);
    install_patch_offset(0x0378D60, patch40, "\x01", 1);
    install_patch_offset(0x03A7B09, patch41, "\x40", 1);
    install_patch_offset(0x04AB93C, patch42, "\x70\x01\xCC\x00", 4);
    install_patch_offset(0x0837A5F, patch43, "\x30\x2A", 2);
    install_patch_offset(0x09EC0E0, patch44,
        "\x00\xc0\x28\x45\x00\xc0\x28\x45\x00\x00\x80\xbf\x00\xc0\xa8\x45\x00\x00\x80\xBF\x00\x00\x61\x45\x00\x00"
        "\xe1\x45\x00\x00\x80\xBF\x00\xc0\x28\x45\x00\xa0\x0c\x46\x00\xa0\x8c\x45\x00\x00\x61\x45\x00\xa0\x0c\x46"
        "\x00\x00\xe1\x44\x00\x00\x80\xBF\x00\x00\x61\x45\x00\xc0\x28\x45\x00\xc0\x28\x45",
        72);
    install_patch_offset(0x09EC8B4, patch45, "\x00\xC0\x28\x46", 4);
    install_patch_offset(0x09EC9F4, patch46, "\x00\xC0\x28\x46", 4);
    install_patch_offset(0x09ECB34, patch47, "\x00\xC0\x28\x46", 4);
    install_patch_offset(0x09ECC74, patch48, "\x00\xC0\x28\x46", 4);
    install_patch_offset(0x09ECDB4, patch49, "\x00\xC0\x28\x46", 4);
}

void DifficultySelect::set_gmd() {
    install_patch_offset(0x004D6C7, patch1, "\x05\xA8\xCD", 3);
    install_patch_offset(0x0094AE7, patch2, "\x00", 1);
    install_patch_offset(0x0094B02, patch3, "\x50\xB9", 2);
    install_patch_offset(0x00B5689, patch4, "\xC4\xCD\xDE\x00", 4);
    install_patch_offset(0x011B9B6, patch5, "\x72", 1);
    install_patch_offset(0x011B9BE, patch6, "\x00\x74\x05\xBA\x00", 5);
    install_patch_offset(0x011B9DB, patch7, "\x10\xBF", 2);
    install_patch_offset(0x011B9E4, patch8, "\x10\xBF", 2);
    install_patch_offset(0x011BA85, patch9, "\x04\xC0", 2);
    install_patch_offset(0x011BA90, patch10, "\x04\xC0", 2);
    install_patch_offset(0x011BAFA, patch11, "\x72", 1);
    install_patch_offset(0x011BB02, patch12, "\x00\x74\x05\xB9\x00", 5);
    install_patch_offset(0x011BB19, patch13, "\x48\xBD", 2);
    install_patch_offset(0x011BB21, patch14, "\x48\xBD", 2);
    install_patch_offset(0x01D2197, patch15, "\x14\x2E", 2);
    install_patch_offset(0x0234D4C, patch16, "\x05\x74", 2);
    install_patch_offset(0x0234D5B, patch17, "\x48", 1);
    install_patch_offset(0x024D6F6, patch18, "\xA4\x3E", 2);
    install_patch_offset(0x024D80D, patch19, "\x05\x74", 2);
    install_patch_offset(0x024D81C, patch20, "\x48", 1);
    install_patch_offset(0x0254383, patch21, "\xA4\x3E", 2);
    install_patch_offset(0x02892AB, patch22, "\x05\x74", 2);
    install_patch_offset(0x02892BA, patch23, "\x48", 1);
    install_patch_offset(0x02949A4, patch24, "\x01", 1);
    install_patch_offset(0x02ABB41, patch25, "\x05\x74", 2);
    install_patch_offset(0x02ABB50, patch26, "\x48", 1);
    install_patch_offset(0x02C0435, patch27, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02C0445, patch28, "\x5A", 1);
    install_patch_offset(0x02DA9CA, patch29, "\xA8\x9E", 2);
    install_patch_offset(0x02DA9EB, patch30, "\x05\x74", 2);
    install_patch_offset(0x02FAD4E, patch31, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x02FAD60, patch32, "\x48", 1);
    install_patch_offset(0x0304E9E, patch33, "\x44\x01\x00\x00\x77", 5);
    install_patch_offset(0x0304EB0, patch34, "\x48", 1);
    install_patch_offset(0x03309A0, patch35, "\x05\x75", 2);
    install_patch_offset(0x0343B6A, patch36, "\xC4\xCD\xDE\x00", 4);
    install_patch_offset(0x03BFD41, patch37, "\xF0\xF2\xC0", 3);
    install_patch_offset(0x03BFD72, patch38, "\xFC\xEE", 2);
    install_patch_offset(0x03C0309, patch39, "\xFF\x87", 2);
    install_patch_offset(0x0378D60, patch40, "\x00", 1);
    install_patch_offset(0x03A7B09, patch41, "\x3C", 1);
    install_patch_offset(0x04AB93C, patch42, "\xC4\xCD\xDE\x00", 4);
    install_patch_offset(0x0837A5F, patch43, "\x50\x46", 2);
    install_patch_offset(0x09EC0E0, patch44,
        "\x00\xC0\xA8\x44\x00\xC0\xA8\x44\x00\x00\x80\xbf\x00\xC0\x28\x45\x00\x00\x80\xBF\x00\x00\xE1\x44\x00\x00"
        "\x61\x45\x00\x00\x80\xBF\x00\xc0\xA8\x44\x00\xA0\x8C\x45\x00\xA0\x0C\x45\x00\x00\xE1\x44\x00\xA0\x8C\x45"
        "\x00\x00\x61\x44\x00\x00\x80\xBF\x00\x00\xE1\x44\x00\xC0\xA8\x44\x00\xC0\xA8\x44",
        72);
    install_patch_offset(0x09EC8B4, patch45, "\x00\xA0\x8C\x46", 4);
    install_patch_offset(0x09EC9F4, patch46, "\x00\xA0\x8C\x46", 4);
    install_patch_offset(0x09ECB34, patch47, "\x00\xA0\x8C\x46", 4);
    install_patch_offset(0x09ECC74, patch48, "\x00\xA0\x8C\x46", 4);
    install_patch_offset(0x09ECDB4, patch49, "\x00\xA0\x8C\x46", 4);
    //reset_patches();
    /*
    // DMD & GMD share most of these, default does not:
    install_patch_offset(0x004D6C7, patch1, "\x05\xa8\xcd\xde\x00", 3);      //  frosts jump out of combos outside of DT // `fld dword ptr [edi+DevilMayCry4_DX9.exe+9EC778]` to `fld dword ptr [DevilMayCry4_DX9.exe+9ECDA8]` (1000.0f)
    install_patch_offset(0x0094AE7, patch2,  "\x00\x77\x14", 2);             // `cmp eax, 14` to `cmp eax, 00`
    install_patch_offset(0x0094B02, patch3,  "\x50\xb9\xcb\x00\x8b\xc7", 6); // `movss xmm0,[DevilMayCry4_DX9.exe+8BAD28]` (500.0f) to `movss xmm0,[DevilMayCry4_DX9.exe+8BB950]` (1000.0f)
    install_patch_offset(0x00B5689, patch4,  "\xc4\xcd\xde\x00", 3);         // `subss xmm0,[DevilMayCry4_DX9.exe+80F324]` (7200.0f) to `subss xmm0,[DevilMayCry4_DX9.exe+8C0170]` (10800.0f)
    install_patch_offset(0x011B9B6, patch5,  "\x72\x0e", 2);                 // `je DevilMayCry4_DX9.exe+11B9C6` to `jb DevilMayCry4_DX9.exe+11B9C6`
    install_patch_offset(0x011B9BE, patch6,  "\x00\x74\x05\xba\x00", 5);     // `cmp dword ptr [ecx+000000E0],14` to `cmp dword ptr [ecx+000000E0],00`
    install_patch_offset(0x011B9DB, patch7,  "\x10\xbf", 2);                 // `fld dword ptr [edx*4+DevilMayCry4_DX9.exe+9EBEFC]` changes per difficulty, GMD's is `fld dword ptr [edx*4+DevilMayCry4_DX9.exe+9EBF10]`
    install_patch_offset(0x011B9E4, patch8,  "\x10\xbf", 2);                 // `fld dword ptr [edx*4+DevilMayCry4_DX9.exe+9EBEE8]` changes per difficulty, GMD's is `fld dword ptr [edx*4+DevilMayCry4_DX9.exe+9EBF10]`
    install_patch_offset(0x011BAFA, patch9,  "\x72", 1);                     // `je DevilMayCry4_DX9.exe+11BB0A` to `jb DevilMayCry4_DX9.exe+11BB0A`
    install_patch_offset(0x011BB02, patch10, "\x00\x74\x05\xb9\x00", 5);     // `cmp dword ptr [eax+000000E0],14` to `cmp dword ptr [eax+000000E0],00`
    install_patch_offset(0x011BB19, patch11, "\x48\xbd", 2);                 // `fld dword ptr [eax+DevilMayCry4_DX9.exe+9EBF24]` to `fld dword ptr [eax+DevilMayCry4_DX9.exe+9EBD48]`
    install_patch_offset(0x011BB21, patch12, "\x48\xbd", 2);                 // `fld dword ptr [eax+DevilMayCry4_DX9.exe+9EBF24]` to `fld dword ptr [eax+DevilMayCry4_DX9.exe+9EBD48]`
    install_patch_offset(0x0234D4C, patch13, "\x05\x74", 2);                 // `cmp dword ptr [edi+00000140],03` & `jne DevilMayCry4_DX9.exe+234D7F` to `cmp dword ptr [edi+00000140],05` & `je DevilMayCry4_DX9.exe+234D7F`
    install_patch_offset(0x024D80D, patch14, "\x05\x74", 2);                 // `cmp dword ptr [eax+00000140],03` & `jne DevilMayCry4_DX9.exe+24D840` to `cmp dword ptr [eax+00000140],05` & `je DevilMayCry4_DX9.exe+24D840`
    install_patch_offset(0x02892AB, patch15, "\x05\x74", 2);                 // `cmp dword ptr [ecx+00000140],03` & `jne DevilMayCry4_DX9.exe+2892DE` to `cmp dword ptr [ecx+00000140],05` & `je DevilMayCry4_DX9.exe+2892DE`
    install_patch_offset(0x02ABB41, patch16, "\x05\x74", 2);                 // `cmp dword ptr [ecx+00000140],03` & `jne DevilMayCry4_DX9.exe+2ABB74` to `cmp dword ptr [ecx+00000140],05` & `je DevilMayCry4_DX9.exe+2ABB74`
    install_patch_offset(0x02C0435, patch17, "\x44\x01\x00\x00\x77", 5);     // `cmp [esi+00000140],edx` & `jne DevilMayCry4_DX9.exe+2C045F` to `cmp [esi+00000144],edx` & `ja DevilMayCry4_DX9.exe+2C045F`
    install_patch_offset(0x02DA9EB, patch18, "\x05\x74", 2);                 // `jne DevilMayCry4_DX9.exe+2DAA12` to `je DevilMayCry4_DX9.exe+2DAA12`
    install_patch_offset(0x02FAD4E, patch19, "\x44\x01\x00\x00\x77", 5);     // `cmp [eax+00000140],edx` & `jne DevilMayCry4_DX9.exe+2FAD84` to `cmp [eax+00000144],edx` & `ja DevilMayCry4_DX9.exe+2FAD84`
    install_patch_offset(0x0304E9E, patch20, "\x44\x01\x00\x00\x77", 5);     // `cmp [eax+00000140],edx` & `jne DevilMayCry4_DX9.exe+304ED4` to `cmp [eax+0000014],edx` & `ja DevilMayCry4_DX9.exe+304ED4`
    install_patch_offset(0x03309A0, patch21, "\x05\xEB\x1F", 3);             // jmp instead of jne // turns on dt // `cmp dword ptr [eax+00000140],03` & `je DevilMayCry4_DX9.exe+3309C2` to `cmp dword ptr [eax+00000140],05` & `jne DevilMayCry4_DX9.exe+3309C2`
    install_patch_offset(0x0343B6A, patch22, "\xc4\xcd\xde", 3);             // `subss xmm0,[DevilMayCry4_DX9.exe+80F324]` (7200.0f) changes per difficulty, GMD's is `subss xmm0,[DevilMayCry4_DX9.exe+9ECDC4]` (18000.0f)
    install_patch_offset(0x04AB93C, patch23, "\xc4\xcd\xde", 3);             // `movss xmm0,[DevilMayCry4_DX9.exe+80F324]` (7200.0f) changes per difficulty, GMD's is `movss xmm0,[DevilMayCry4_DX9.exe+9ECDC4]` (18000.0f)
    install_patch_offset(0x0837A5F, patch24, "\x50\x46", 2);                 // `7200.0f` changes per difficulty, GMD's is `18000.0f` idk why its 2 bytes and not aligned
    install_patch_offset(0x09EC8B4, patch25, "\x00\xa0\x8c\x46", 5);         // `7200.0f` changes per difficulty, GMD's is `18000.0f`
    install_patch_offset(0x09EC9F4, patch26, "\x00\xa0\x8c\x46", 5);         // `7200.0f` changes per difficulty, GMD's is `18000.0f`
    install_patch_offset(0x09ECB34, patch27, "\x00\xa0\x8c\x46", 5);         // `7200.0f` changes per difficulty, GMD's is `18000.0f`
    install_patch_offset(0x09ECC74, patch28, "\x00\xa0\x8c\x46", 5);         // `7200.0f` changes per difficulty, GMD's is `18000.0f`
    install_patch_offset(0x09ECDB4, patch29, "\x00\xa0\x8c\x46", 5);         // `7200.0f` changes per difficulty, GMD's is `18000.0f`
    // different only on GMD:
    install_patch_offset(0x011BA85, patch30, "\x04\xc0", 2);                 // `movups xmm0,[edx*4+DevilMayCry4_DX9.exe+9EBFC4]` to `movups xmm0,[edx*4+DevilMayCry4_DX9.exe+9EC004]`
    install_patch_offset(0x011BA90, patch31, "\x04\xc0", 2);                 // `movss xmm0,[edx*4+DevilMayCry4_DX9.exe+9EBFB0]` to `movss xmm0,[edx*4+DevilMayCry4_DX9.exe+9EC004]`
    install_patch_offset(0x01D2197, patch32, "\x14\x2e", 2);                 // `fld dword ptr [eax*4+DevilMayCry4_DX9.exe+9528D0]` to `fld dword ptr [eax*4+DevilMayCry4_DX9.exe+952E14]`
    install_patch_offset(0x0234D5B, patch33, "\x48", 1);                     // `jne DevilMayCry4_DX9.exe+234D7F` to `je DevilMayCry4_DX9.exe+234D7F`
    install_patch_offset(0x024D6F6, patch34, "\xa4\x3e", 2);                 // `movss xmm1,[DevilMayCry4_DX9.exe+A15B24]` (0.30f) to `movss xmm1,[DevilMayCry4_DX9.exe+A13EA4]` (0.44f)
    install_patch_offset(0x024D81C, patch35, "\x48", 1);                     // `jne DevilMayCry4_DX9.exe+24D840` to `je DevilMayCry4_DX9.exe+24D840`
    install_patch_offset(0x0254383, patch36, "\xa4\x3e", 2);                 // `movss xmm0,[DevilMayCry4_DX9.exe+A15B24]` (0.30f) to `movss xmm0,[DevilMayCry4_DX9.exe+A13EA4]` (0.44f)
    install_patch_offset(0x02892BA, patch37, "\x48", 1);                     // `mulss xmm0,[DevilMayCry4_DX9.exe+79A244]` (0.20f) to `mulss xmm0,[DevilMayCry4_DX9.exe+79A248]` (0.90f)
    install_patch_offset(0x02949A4, patch38, "\x01\x00", 2);                 // `mov [edi+0000201C],00000002` to `mov [edi+0000201C],00000001`
    install_patch_offset(0x02ABB50, patch39, "\x48", 1);                     // `mulss xmm0,[DevilMayCry4_DX9.exe+79A244]` (0.20f) to `mulss xmm0,[DevilMayCry4_DX9.exe+79A248]` (0.90f)
    install_patch_offset(0x02C0445, patch40, "\x5a", 1);                     // `cmp ecx,14` to `cmp ecx,5A`
    install_patch_offset(0x02DA9CA, patch41, "\xa8\x9e", 2);                 // `movss xmm0,[DevilMayCry4_DX9.exe+8AA05C]` (1200.0f) to `movss xmm0,[DevilMayCry4_DX9.exe+8A9EA8]` (6000.0f)
    install_patch_offset(0x02FAD60, patch42, "\x48", 1);                     // `mulss xmm0,[DevilMayCry4_DX9.exe+79A244]` (0.20f) to `mulss xmm0,[DevilMayCry4_DX9.exe+79A248]` (0.90f)
    install_patch_offset(0x0304EB0, patch43, "\x48", 1);                     // `mulss xmm0,[DevilMayCry4_DX9.exe+79A244]` (0.20f) to `mulss xmm0,[DevilMayCry4_DX9.exe+79A248]` (0.90f)
    install_patch_offset(0x03BFD41, patch44, "\xf0\xf2\xc0", 3);             // `movss xmm0,[DevilMayCry4_DX9.exe+9627A8]` (3000.0f) to `movss xmm0,[DevilMayCry4_DX9.exe+80F2F0]` (7000.0f)
    install_patch_offset(0x03BFD72, patch45, "\xfc\xee", 2);                 // `movss xmm0,[DevilMayCry4_DX9.exe+80F2F0]` (7000.0f) to `movss xmm0,[DevilMayCry4_DX9.exe+80EEFC]` (10000.0f)
    install_patch_offset(0x03C0309, patch46, "\xff\x87", 2);                 // `mov [edi+00003234],bl` to `inc [edi+00003234],bl`
    install_patch_offset(0x0378D60, patch47, "\x00", 1);                     // `push 01` to `push 00`
    install_patch_offset(0x03A7B09, patch48, "\x3c", 1);                     // `fld dword ptr [edi+40]` to `fld dword ptr [edi+3C]`
    install_patch_offset(0x09EC0E0, patch49, "\x00\xc0\xa8\x44\x00\xc0\xa8\x44\x00\x00\x80\xbf\x00\xc0\x28\x45\x00\x00\xc0\xa8\x00\x00\xe1\x44\x00\x00"
                                             "\x61\x45\x78\x30\x30\x00\x00\xc0\xa8\x44\x00\xa0\x8c\x45\x00\xa0\x0c\x45\x00\x00\xe1\x44\x00\xa0\x8c\x45"
                                             "\x00\x00\x61\x44\x00\x00\x78\x30\x00\x00\xe1\x44\x00\xc0\xa8\x44\x00\xc0\xa8\x44", 72);
    */
    /*       0x09EC0E0:
             DEFAULT / DMD & GMD
    00 float // 2700 / 1350
    04 float // 2700 / 1350
    08 float //   -1 /   -1
    0C float // 5400 / 2700
    10 int   // A8C00000
    14 float // 3600 / 1800
    18 float // 7200 / 3600
    1C int   // 00303078
    20 float // 2700 / 1350
    24 float // 9000 / 4500
    28 float // 4500 / 2250
    2C float // 3600 / 1800
    30 float // 9000 / 4500
    34 float // 1800 / 900
    38 int   // 30780000
    3C float // 3600 / 1800
    40 float // 2700 / 1350
    44 float // 2700 / 1350
    */
}

void DifficultySelect::on_config_load(const utility::Config& cfg) {
    game_difficulty = cfg.get<int>("game_difficulty").value_or(0);
    if (game_difficulty) {
        switch (game_difficulty) {
        case 0:
            // setDefault(); // no need to set default on load
            break;
        case 1:
            set_dmd();
            break;
        case 2:
            set_gmd();
            break;
        }
    }
}

void DifficultySelect::on_config_save(utility::Config& cfg) {
    cfg.set<int>("game_difficulty", game_difficulty);
}

void DifficultySelect::on_gui_frame() {
    ImGui::PushItemWidth(224);
    if (ImGui::Combo("Game Mode", &game_difficulty, "Default\0Dante Must Die\0God Must Die\0")) {
        switch (game_difficulty)
        {
        case 0:
            set_default();
            break;
        case 1:
            set_dmd();
            break;
        case 2:
            set_gmd();
            break;
        }
    }
    ImGui::PopItemWidth();
    ImGui::Spacing();
}
