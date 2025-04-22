#include "FreeJc.hpp"
#include "EasyJc.hpp"

#if 1
bool FreeJc::mod_enabled_nero = false;
bool FreeJc::mod_enabled_dante = false;

uintptr_t FreeJc::jmp_ret2;

uintptr_t detour1_je1 = 0x804A37;
uintptr_t detour1_jmp1 = 0x804A10;
uintptr_t detour1_skip = 0x804A88;

naked void freeJcDetour1(void) { // player in edi
    _asm {
        cmp byte ptr [edi+0x1494], 1 // controller id nero
        je CheckNero
		cmp byte ptr [FreeJc::mod_enabled_dante], 1
        je jmp_skip
        jmp originalcode

        CheckNero:
        cmp byte ptr [FreeJc::mod_enabled_nero], 1
        je jmp_skip

    originalcode:
        test esi,esi
        je jmp_je
        jmp dword ptr [detour1_jmp1]
    jmp_je:
        jmp dword ptr [detour1_je1]
    jmp_skip:
        jmp dword ptr [detour1_skip]
    }
}

naked void freeJcDetour2(void) { // player in ecx
    _asm {
        cmp byte ptr [ecx+0x1494], 1 // controller id nero
        je CheckNero
		cmp byte ptr [FreeJc::mod_enabled_dante], 1
        je retcode
        cmp byte ptr [EasyJc::mod_enabled_dante], 1
        je retcode
        jmp originalcode

        CheckNero:
        cmp byte ptr [FreeJc::mod_enabled_nero], 1
        je retcode
        cmp byte ptr [EasyJc::mod_enabled_nero], 1
        je retcode
    originalcode:
        mov edi,00000002
    retcode:
		jmp dword ptr [FreeJc::jmp_ret2]
    }
}

std::optional<std::string> FreeJc::on_initialize() {
    if (!install_hook_offset(0x404A04, hook1, &freeJcDetour1, 0, 6)) {
		spdlog::error("Failed to init InfAirHikes mod 1\n");
		return "Failed to init InfAirHikes mod 1";
	}
    if (!install_hook_offset(0x427999, hook2, &freeJcDetour2, &jmp_ret2, 5)) {
		spdlog::error("Failed to init InfAirHikes mod 2\n");
		return "Failed to init InfAirHikes mod 2";
	}

    console->system().RegisterCommand("freejc", "Enemy Step anywhere", [/*this*/]() {
        FreeJc::mod_enabled_nero = !FreeJc::mod_enabled_nero;
        FreeJc::mod_enabled_dante = !FreeJc::mod_enabled_dante;
    });

    return Mod::on_initialize();
}

void FreeJc::on_gui_frame(int display) {
    if (display == 0) {
        if (ImGui::Checkbox(_("Free Enemy Step"), &FreeJc::mod_enabled_nero)) {
            mod_enabled_dante = mod_enabled_nero;
        }
        ImGui::SameLine();
        help_marker(_("Enemy Step anywhere (this enables/disables the cheat for both characters)"));
    }
    else if (display == 1) {
        ImGui::Checkbox(_("Free Enemy Step"), &FreeJc::mod_enabled_nero);
        ImGui::SameLine();
        help_marker(_("Enemy Step anywhere"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Free Enemy Step"), &FreeJc::mod_enabled_dante);
        ImGui::SameLine();
        help_marker(_("Enemy Step anywhere"));
    }
}

void FreeJc::on_config_load(const utility::Config& cfg) {
    FreeJc::mod_enabled_nero = cfg.get<bool>("free_jc_nero").value_or(false);
    FreeJc::mod_enabled_dante = cfg.get<bool>("free_jc_dante").value_or(false);
}

void FreeJc::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("free_jc_nero", FreeJc::mod_enabled_nero);
    cfg.set<bool>("free_jc_dante", FreeJc::mod_enabled_dante);
}

#endif
