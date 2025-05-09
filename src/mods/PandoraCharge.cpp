#include "PandoraCharge.hpp"

bool PandoraCharge::mod_enabled = false;
uintptr_t PandoraCharge::jmp_ret1 = NULL;
uintptr_t PandoraCharge::jmp_ret2 = NULL;
uintptr_t PandoraCharge::jmp_ret3 = NULL;

// Charge & Uncharge with Pandora equipped
void PandoraCharge::toggle1(bool enable) {
    if (enable) {
        // make any gun jump to the selector
        install_patch_offset(0x3BB4C3, patch1, "\xEB", 1); // replaces je with jmp
    }
    else {
        patch1.reset();
    }
}

static constexpr uintptr_t detour1_call1       = 0x7ACE10;
static constexpr uintptr_t detour1_jne_custom  = 0x7BB8C4;
static constexpr uintptr_t detour1_jmp_custom  = 0x7BB58F;
static constexpr uintptr_t detour1_jne_default = 0x7BB5E6;
// Add Pandora Charge Effect
naked void detour1() {
    _asm {
            cmp byte ptr [PandoraCharge::mod_enabled], 1
            jne originalcode

            cmp eax, 07
            jne newcode // weapon id also isn't shotgun, check for pandora
            jmp dword ptr [PandoraCharge::jmp_ret1]

        newcode:
            mov esi, 0x3D
            mov ecx, edi
            call dword ptr [detour1_call1]
            cmp al, 0x01
            jne jmp_jne_custom
            mov edx, 0x78
            jmp dword ptr [detour1_jmp_custom]

        originalcode:
            cmp eax, 07
            jne jmp_jne_default
            jmp dword ptr [PandoraCharge::jmp_ret1]

        jmp_jne_custom:
            jmp dword ptr [detour1_jne_custom]

        jmp_jne_default:
            jmp dword ptr [detour1_jne_default]
    }
}

static constexpr uintptr_t detour2_call1       = 0x7ACE10;
static constexpr uintptr_t detour2_je_custom   = 0x7BB8C4;
static constexpr uintptr_t detour2_jmp_custom  = 0x7BB63D;
static constexpr uintptr_t detour2_jne_default = 0x7BB5E6;
// Add Pandora Charged Effect
naked void detour2() {
    _asm {
            cmp byte ptr [PandoraCharge::mod_enabled], 1
            jne originalcode

            cmp eax, 07
            jne newcode // weapon id also isn't shotgun, check for pandora
            jmp dword ptr [PandoraCharge::jmp_ret2]

        newcode:
            mov esi, 0x48
            mov ecx, edi
            call dword ptr [detour2_call1]
            test al, al
            je jmp_je_custom
            mov edx, 0x79
            jmp dword ptr [detour2_jmp_custom]

        originalcode:
            cmp eax, 07
            jne jmp_jne_default
            jmp dword ptr [PandoraCharge::jmp_ret2]

        jmp_je_custom:
            jmp dword ptr [detour2_je_custom]

        jmp_jne_default:
            jmp dword ptr [detour2_jne_default]
    }
}

 // Dante DT Check Func, this should probably be moved to its own file, it's very useful
naked void detour3() {
    _asm {
            cmp byte ptr [PandoraCharge::mod_enabled], 1
            jne originalcode

            // compare return address to get ID
            cmp dword ptr [esp], 0x7BCE94 // fast pandora 1
            je CheckGunCharge
            cmp dword ptr [esp], 0x7BD364 // fast pandora 2
            je CheckGunCharge
            cmp dword ptr [esp], 0x81B906 // jealousy
            je CheckGunCharge
            cmp dword ptr [esp], 0x808D1D // rainstorm projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7D9716 // rainstorm sounds
            je CheckGunCharge
            cmp dword ptr [esp], 0x7D996F // rainstorm sounds 2
            je CheckGunCharge
            cmp dword ptr [esp], 0x8095FD // twosome time projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7D8DDC // twosome time sounds
            je CheckGunCharge
            cmp dword ptr [esp], 0x80FD66 // honeycomb projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7D9228 // honeycomb sounds
            je CheckGunCharge
            cmp dword ptr [esp], 0x80F0EC // gun stinger projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7DB3AE // gun stinger sounds
            je CheckGunCharge
            cmp dword ptr [esp], 0x80D15C // fireworks projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7BA0BD // fireworks sounds
            je CheckGunCharge
            cmp dword ptr [esp], 0x80E2EC // backslide projectiles
            je CheckGunCharge
            cmp dword ptr [esp], 0x7DAE8B // backslide sounds
            je CheckGunCharge
            jmp originalcode

        CheckGunCharge:
            cmp [ecx+0x14DA0], 3 // is gun charged
            jne originalcode
            mov al, 1 // make this func think DT is active
            ret

        originalcode:
            cmp dword ptr [ecx+0x00014D38], 00
            setne al
            ret
            jmp dword ptr [PandoraCharge::jmp_ret3]
    }
}

std::optional<std::string> PandoraCharge::on_initialize() {
    // Add Pandora Charge Effect
    if (!install_hook_offset(0x3BB575, hook1, &detour1, &jmp_ret1, 5)) {
        spdlog::error("Failed to init PandoraCharge mod 1\n");
        return "Failed to init PandoraCharge mod 1";
	}
    // Add Pandora Charged Effect
    if (!install_hook_offset(0x3BB623, hook2, &detour2, &jmp_ret2, 5)) {
        spdlog::error("Failed to init PandoraCharge mod 3\2");
        return "Failed to init PandoraCharge mod 2";
	}
    // Dante DT Check Func, this should probably be moved to its own file, it's very useful
    if (!install_hook_offset(0x3B2080, hook3, &detour3, &jmp_ret3, 11)) {
        spdlog::error("Failed to init PandoraCharge mod 3\n");
        return "Failed to init PandoraCharge mod 3";
	}

    return Mod::on_initialize();
}

void PandoraCharge::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Pandora Charge"), &mod_enabled)) {
        toggle1(mod_enabled); // Charge & Uncharge with Pandora equipped
    }
    ImGui::SameLine();
    help_marker(_("Charge Pandora just like the other guns"));
}

// void on_frame(fmilliseconds& dt) {}

// void on_game_pause(bool toggle) {}

// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

void PandoraCharge::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("PandoraCharge").value_or(false);
    if (mod_enabled) toggle1(mod_enabled); // Charge & Uncharge with Pandora equipped
}

void PandoraCharge::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("PandoraCharge", mod_enabled);
}
