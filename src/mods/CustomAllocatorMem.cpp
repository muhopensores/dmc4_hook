#include "CustomAllocatorMem.hpp"

bool CustomAllocatorMem::mod_enabled{ true };
MtAllocator* MtAllocatorList = (MtAllocator*)0x00E47D50;
void CustomAllocatorMem::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x4AFB3B, patch1, "\x00\x00\x00\x09",4); //Global - default \x00\x00\x00\x06
        install_patch_offset(0x4AFB73, patch2, "\x00\x00\x00\x03",4); //Temp - default \x00\x00\x00\x02
        install_patch_offset(0x4AFBAB, patch3, "\x00\x00\x00\x09",4); //Resource - default \x00\x00\x00\x06
        install_patch_offset(0x4AFBE3, patch4, "\x00\x00\x40\x00",4); //Material - default \x00\x00\x40\x00
        install_patch_offset(0x4AFC1B, patch5, "\x00\x00\xE0\x02",4); //System - default \x00\x00\xE0\x02
        install_patch_offset(0x4AFC53, patch6, "\x00\x00\x40\x02",4); //Unit - default \x00\x00\x40\x02
        install_patch_offset(0x4AFC8B, patch7, "\x00\x00\x10\x00",4); //Area - default \x00\x00\x10\x00
        install_patch_offset(0x4AFCC3, patch8, "\x00\x00\x20\x00",4); //Trans - default \x00\x00\x20\x00
        install_patch_offset(0x4AFCFB, patch9, "\x00\x00\x80\x00",4); //Array/String - default \x00\x00\x80\x00
        install_patch_offset(0x4AFD38, patch10, "\x00\x00\x00\x01",4); //Sound - default \x00\x00\x00\x01
    }
    else {
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
    }
}

std::optional<std::string> CustomAllocatorMem::on_initialize() {
    toggle(1);
    return Mod::on_initialize();
}