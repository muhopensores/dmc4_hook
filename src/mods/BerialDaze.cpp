#include "BerialDaze.hpp"

#if 1
bool BerialDaze::mod_enabled{ false };
uintptr_t BerialDaze::jmp_ret{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [BerialDaze::mod_enabled], 0
			je originalcode

			movss xmm0, [esi+0x00001B70]
			addss xmm0, [esi+0x00001B70]
			movss [esi+0x00001B80], xmm0
			jmp dword ptr [BerialDaze::jmp_ret]
		
		originalcode:
            movss xmm0, [esi+0x00001B80]
			jmp dword ptr [BerialDaze::jmp_ret]
	}
}

void BerialDaze::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x23515B, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(0x234FE2, patch2, "\xEB\x7D", 2); // jmp
    }
    else {
        patch.reset();
        patch2.reset(); // jna
    }
}

std::optional<std::string> BerialDaze::on_initialize() {
	if (!install_hook_offset(0x024271A, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init BerialDaze mod\n");
		return "Failed to init BerialDaze mod";
	}
	return Mod::on_initialize();
}

void BerialDaze::on_gui_frame() {
    if (ImGui::Checkbox("Berial Daze", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Sets Berial to his fireless Dazed state");
}

void BerialDaze::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("berial_practice").value_or(false);
    toggle(mod_enabled);
}

void BerialDaze::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("berial_practice", mod_enabled);
}
#endif
