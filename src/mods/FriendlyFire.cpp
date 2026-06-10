#include "FriendlyFire.hpp"
#include "../sdk/Devil4.hpp"
bool FriendlyFire::mod_enabled = false;
uintptr_t FriendlyFire::jmp_ret1 = NULL;

void FriendlyFire::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x10C690, patch1, "\x8B\xB1\x00\x03\x00\x00", 6); // hit players,                                [playerCol+300],     mVsAttrEm2Atk,    (22020222)
        install_patch_offset(0x10C69B, patch2, "\x8B\x91\x10\x03\x00\x00", 6); // idk,                                        [playerCol+310],     mVsAttrEm2FrdAtk, (00202000)
        install_patch_offset(0x10C6AB, patch3, "\x8B\x91\x04\x03\x00\x00", 6); // not changing means I can't damage anything, [playerCol+304],     mVsAttrEm2Dmg,    (11111111)
        install_patch_offset(0x10C6E4, patch4, "\x8B\xB1\x00\x03\x00\x00", 6); // not changing means I can't shoot players,   [projectileCol+300], mVsAttrEm2Atk,    (22020222)
        install_patch_offset(0x10C6FF, patch5, "\x8B\x91\x04\x03\x00\x00", 6); // idk,                                        [playerCol+304],     mVsAttrEm2Dmg,    (11111111)
    }
    else {
        patch1.reset(); // hit players,                                                                                       [playerCol+2b0]      mVsAttrPlAtk,     0x22222200
        patch2.reset(); // idk,                                                                                               [playerCol+2c0],     mVsAttrPlFrdAtk,  0x00000022
        patch3.reset(); // not changing means I can't damage anything,                                                        [playerCol+2b4],     mVsAttrPlDmg,     0x11111100
        patch4.reset(); // not changing means I can't shoot players                                                           [projectileCol+2c4], mVsAttrPsAtk,     0x22222200
        patch5.reset(); // idk,                                                                                               [unkn+2c8],          mVsAttrPsDmg,     0x11111100
    }
}


/*naked void detour1() {
    _asm {
            cmp byte ptr [FriendlyFire::mod_enabled], 1
            jne originalcode

            

        originalcode:

            jmp dword ptr [FriendlyFire::jmp_ret1]
    }
}*/

// void FriendlyFire::on_frame(fmilliseconds& dt) {}

void FriendlyFire::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Friendly Fire"), &mod_enabled)) {
            toggle1(mod_enabled);
        }
    }
}

// void FriendlyFire::on_game_pause(bool toggle) {}
// bool FriendlyFire::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FriendlyFire::on_initialize() {
    /*if (!install_hook_offset(0x0, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init FriendlyFire mod 1\n");
        return "Failed to init FriendlyFire mod 1";
	}*/

    return Mod::on_initialize();
}

void FriendlyFire::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FriendlyFire").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void FriendlyFire::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FriendlyFire", mod_enabled);
}
