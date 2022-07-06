#include "TwCmdHolyWater.hpp"

// TODO(): move this into somewhere general
uPlayer* get_local_payer() {
	constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
	SMediator* s_med_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
	uPlayer* u_plr = s_med_ptr->player_ptr;
	return u_plr;
}

void use_hw_asm_call(){
	constexpr uintptr_t use_holy_water_fptr = 0x00827D10;

	uPlayer* u_plr = get_local_payer();
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

std::optional<std::string> TwCmdHolyWater::on_initialize() {
	m_command = std::hash<std::string>{}("\\HolyWater");
	m_shorthand = std::hash<std::string>{}("\\hw");
	
	m_cooldown = new utility::Timer(30.0f, on_cooldown_end);

	return Mod::on_initialize();
}

void TwCmdHolyWater::on_frame(fmilliseconds & dt) {
	if (!m_alow_cmd) { m_cooldown->tick(dt); }
}

void TwCmdHolyWater::on_twitch_command(std::size_t hash) {
	if ((hash == m_command || hash == m_shorthand ) && m_alow_cmd) {
		use_hw_asm_call();
		m_cooldown->start();
		m_alow_cmd = false;
	}
}
