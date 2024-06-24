
#include "InfDT.hpp"
#include "utility/MessageDisplay.hpp"
#include "EnemySpawn.hpp"

bool InfDT::mod_enabled{ false };
uintptr_t InfDT::jmp_ret{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [InfDT::mod_enabled], 0
			je originalcode

			mov dword ptr [ebp+0x00001F24], 0x461C4000
			jmp dword ptr [InfDT::jmp_ret]

			originalcode:
			movss xmm0, [ebp+0x00001F24]
			jmp dword ptr [InfDT::jmp_ret]
	}
}

std::optional<std::string> InfDT::on_initialize() {
	if (!install_hook_offset(0x00FF315, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init InfDT mod\n");
		return "Failed to init InfDT mod";
	}

    MutatorRegistry::define("InfiniteDT")
        .description("hehe")
        .on_init([&] { mod_enabled = true; })
        .set_timer(30.0, [&] { mod_enabled = false; });

    utility::create_keyboard_hotkey(m_hotkeys, {VK_F2}, "Inf DT", "inf_dt_key");

    console->system().RegisterCommand("infdt", "Infinite DT", [/*this*/]() {
        InfDT::mod_enabled = !InfDT::mod_enabled;
    });

	return Mod::on_initialize();
}

void InfDT::on_gui_frame() {
    ImGui::Checkbox(_("Infinite DT"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Fills your Devil Trigger Gauge and stops it from decreasing. By default this is hotkeyed to F2"));
}

void InfDT::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_dt").value_or(false);
}

void InfDT::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_dt", mod_enabled);
}

void InfDT::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        if (mod_enabled) {
            DISPLAY_MESSAGE("Infinite DT Off");
        }
        else {
            DISPLAY_MESSAGE("Infinite DT On");
        }
        mod_enabled = !mod_enabled;
    }
}
