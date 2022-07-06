#include "TwCmdPlayerTransforms.hpp"

static uintptr_t player_transforms_detour_continue = NULL;

static bool  g_mod_enable = false;
static float t_workrate   = 1.2f;
static float o_workrate   = 0.0f;
static uPlayer* plr = nullptr;

naked void player_transforms_detour() {
	__asm {
		mov [plr], eax
		cmp g_mod_enable, 1
		jne originalCode
		movss xmm2, [t_workrate]
		jmp dword ptr [player_transforms_detour_continue]
	originalCode:
		movss xmm2,[eax+10h]
		jmp dword ptr [player_transforms_detour_continue]
	}
}

void on_timer_callback() {
	g_mod_enable = false;
}

std::optional<std::string> TwCmdPlayerTransforms::on_initialize() {
	// DevilMayCry4_DX9.exe+3ACEF0 
	if (!install_hook_offset(0x3ACEF0 , m_hook, &player_transforms_detour, &player_transforms_detour_continue, 5))
	{
		spdlog::error("Failed to init TwCmdPlayerTransforms mod\n");
		return "Failed to init TwCmdPlayerTransforms mod";
	}

	m_command_more_gravity = std::hash<std::string>{}("\\MoreGravity");
	m_command_less_gravity = std::hash<std::string>{}("\\LessGravity");

	m_timer = new utility::Timer(30.0f, on_timer_callback);

	return Mod::on_initialize();
}

void TwCmdPlayerTransforms::on_frame(fmilliseconds & dt) {
	m_timer->tick(dt);
}

#if 0
void TwCmdPlayerTransforms::onGUIframe()
{
	if (ImGui::Button("LowGravityTest")) {
		m_timer->start();
		g_modEnable = true;
		tWorkrate = plr->mDeltaTime * 0.5f;
	}
	if (ImGui::Button("HighGravityTest")) {
		m_timer->start();
		g_modEnable = true;
		tWorkrate = plr->mDeltaTime * 1.27f;
	}
}
#endif

void TwCmdPlayerTransforms::on_twitch_command(std::size_t hash) {
	// TODO(): send a message if shit is already active?

	if (m_timer->m_active) { return; } // lmao forgot about this

	if (hash == m_command_less_gravity) {
		m_timer->start();
		g_mod_enable = true;
		t_workrate = plr->m_delta_time * 0.5f;
	}
	if (hash == m_command_more_gravity) {
		m_timer->start();
		g_mod_enable = true;
		t_workrate = plr->m_delta_time * 1.27f;
	}
}
