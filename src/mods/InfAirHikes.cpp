#include "InfAirHikes.hpp"

bool InfAirHikes::mod_enabled_nero = false;
bool InfAirHikes::mod_enabled_dante = false;
uintptr_t InfAirHikes::jmp_ret1 = NULL;
uintptr_t InfAirHikes::jmp_ret2 = NULL;

naked void detour1(void) { // player in ecx
    _asm {
        cmp byte ptr [ecx+0x1494], 1 // controller id nero
        je CheckNero
		cmp byte ptr [InfAirHikes::mod_enabled_dante], 0
        je originalcode
        jmp newcode

        CheckNero:
        cmp byte ptr [InfAirHikes::mod_enabled_nero], 0
        je originalcode
    newcode:
        mov [ecx+0x00001E7E],dl
        jmp retcode
    originalcode:
        mov [ecx+0x00001E7F],dl
    retcode:
		jmp dword ptr [InfAirHikes::jmp_ret1]
    }
}

naked void detour2(void) { // player in ecx
    _asm {
        cmp byte ptr [ecx+0x1494], 1 // controller id nero
        je CheckNero
		cmp byte ptr [InfAirHikes::mod_enabled_dante], 0
        je originalcode
        jmp retcode

        CheckNero:
        cmp byte ptr [InfAirHikes::mod_enabled_nero], 1
        je retcode
    originalcode:
        add byte ptr [ecx+0x00001E7F], -01
        add byte ptr [ecx+0x00001E7E], 01
    retcode:
		jmp dword ptr [InfAirHikes::jmp_ret1]
    }
}

std::optional<std::string> InfAirHikes::on_initialize() {
    if (!install_hook_offset(0x3ACA3A, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init InfAirHikes mod 1\n");
		return "Failed to init InfAirHikes mod 1";
	}
    if (!install_hook_offset(0x3ACA4A, hook2, &detour2, &jmp_ret2, 14)) {
		spdlog::error("Failed to init InfAirHikes mod 2\n");
		return "Failed to init InfAirHikes mod 2";
	}

    MutatorRegistry::define("InfiniteAirHikes")
        .description("hehe")
        .on_init([&] { mod_enabled_nero = true; mod_enabled_dante = true; })
        .set_timer(15.0, [&] { mod_enabled_nero = false; mod_enabled_dante = false; });

    console->system().RegisterCommand("infjumps", "Toggle infinite Air Hikes", [this]() {
        InfAirHikes::mod_enabled_nero = !InfAirHikes::mod_enabled_nero;
        InfAirHikes::mod_enabled_dante = !InfAirHikes::mod_enabled_dante;
    });

    return Mod::on_initialize();
}


void InfAirHikes::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Infinite Air Hikes"), &mod_enabled_nero);
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Infinite Air Hikes"), &mod_enabled_dante);
    }
}

void InfAirHikes::on_config_load(const utility::Config& cfg) {
    mod_enabled_nero = cfg.get<bool>("inf_air_hikes_nero").value_or(false);
    mod_enabled_dante = cfg.get<bool>("inf_air_hikes_dante").value_or(false);
};

void InfAirHikes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_air_hikes_nero", mod_enabled_nero);
    cfg.set<bool>("inf_air_hikes_dante", mod_enabled_dante);
};
