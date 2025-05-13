#include "MutatorHolyWater.hpp"
#include "../sdk/Devil4.hpp"

void MutatorHolyWater::use_hw_asm_call(){
	constexpr uintptr_t use_holy_water_fptr = 0x00827D10;

	uPlayer* u_plr = devil4_sdk::get_local_player();
	if (!u_plr) { return; }

	__asm {
		mov esi, u_plr
		call use_holy_water_fptr
	}
}

bool m_alow_cmd = true;

void on_cooldown_end() {
	m_alow_cmd = true;
}

std::optional<std::string> MutatorHolyWater::on_initialize() {
    MutatorRegistry::define("HolyWater")
        .description("Use a holy water even in places where its not allowed")
        .alias("hw")
        .on_init([] {
        use_hw_asm_call();
    });

	console->system().RegisterCommand("holywater", "Use a Holy Water", [/*this*/]() {
        use_hw_asm_call();
    });

	/*m_command = std::hash<std::string>{}("\\HolyWater");
	m_shorthand = std::hash<std::string>{}("\\hw");
	
	m_cooldown = new utility::Timer(30.0f, on_cooldown_end);*/

	return Mod::on_initialize();
}

void MutatorHolyWater::on_frame(fmilliseconds & dt) {
	if (!m_alow_cmd) { m_cooldown->tick(dt); }
}
#if 0
void TwCmdHolyWater::on_twitch_command(std::size_t hash) {
	if ((hash == m_command || hash == m_shorthand ) && m_alow_cmd) {
		use_hw_asm_call();
		m_cooldown->start();
		m_alow_cmd = false;
	}
}
#endif