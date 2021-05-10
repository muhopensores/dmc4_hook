#include "modTwCmdHolyWater.hpp"

// TODO(): move this into somewhere general
uPlayer* getLocalPayer() {
	constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
	sMediator* sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
	uPlayer* uPlr = sMedPtr->playerPtr;
	return uPlr;
}

void useHwAsmCall(){

	constexpr uintptr_t useHolyWaterFptr = 0x00827D10;

	uPlayer* uPlr = getLocalPayer();
	if (!uPlr) { return; }

	__asm {
		mov esi, uPlr
		call useHolyWaterFptr
	}
}

bool m_alowCmd = true;

void onCooldownEnd() {
	m_alowCmd = true;
}

std::optional<std::string> TwCmdHolyWater::onInitialize()
{
	
	m_command = std::hash<std::string>{}("\\HolyWater");
	m_shorthand = std::hash<std::string>{}("\\hw");
	
	m_cooldown = new utils::Timer(30.0f, onCooldownEnd);

	return Mod::onInitialize();
}

void TwCmdHolyWater::onFrame(fmilliseconds & dt)
{
	if (!m_alowCmd) { m_cooldown->tick(dt); }
}

/*void TwCmdHolyWater::onGUIframe() {
}*/

void TwCmdHolyWater::onTwitchCommand(std::size_t hash)
{
	if ((hash == m_command || hash == m_shorthand ) && m_alowCmd) {
		useHwAsmCall();
		m_cooldown->start();
		m_alowCmd = false;
	}
}
