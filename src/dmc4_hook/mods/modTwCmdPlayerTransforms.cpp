#include "modTwCmdPlayerTransforms.hpp"

static uintptr_t PlayerTransformsDetourContinue = NULL;

bool  g_modEnable = false;
float tWorkrate   = 1.2f;

naked void PlayerTransformsDetour() {
	__asm {
		cmp g_modEnable, 1
		jne originalCode
		movss xmm2, [tWorkrate]
		jmp dword ptr [PlayerTransformsDetourContinue]
	originalCode:
		movss xmm2,[eax+10h]
		jmp dword ptr [PlayerTransformsDetourContinue]
	}
}

void onTimerCallback() {
	g_modEnable = false;
}

std::optional<std::string> TwCmdPlayerTransforms::onInitialize()
{
	// DevilMayCry4_DX9.exe+3ACEF0 
	if (!install_hook_offset(0x3ACEF0 , m_hook, &PlayerTransformsDetour, &PlayerTransformsDetourContinue, 5))
	{
		HL_LOG_ERR("Failed to init TwCmdPlayerTransforms mod\n");
		return "Failed to init TwCmdPlayerTransforms mod";
	}

	m_command_more_gravity = std::hash<std::string>{}("\\MoreGravity");
	m_command_less_gravity = std::hash<std::string>{}("\\LessGravity");

	m_timer = new utils::Timer(30.0f, onTimerCallback);

	return Mod::onInitialize();
}

void TwCmdPlayerTransforms::onFrame(fmilliseconds & dt)
{
	m_timer->tick(dt);
}

void TwCmdPlayerTransforms::onTwitchCommand(std::size_t hash)
{
	// TODO(): send a message if shit is already active?

	if (m_timer->m_active) { return; } // lmao forgot about this

	if (hash == m_command_less_gravity) {
		m_timer->start();
		g_modEnable = true;
		tWorkrate = 0.88f;
	}
	if (hash == m_command_more_gravity) {
		m_timer->start();
		g_modEnable = true;
		tWorkrate = 1.4f;
	}
}
