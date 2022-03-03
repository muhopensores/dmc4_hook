// include your mod header file
#include "modTwCmdSuperhot.hpp"
#include "../sdk/Devil4.hpp"

#include "glm/gtx/compatibility.hpp"

static bool mod_enabled{ false };

// Modeled after the exponential function y = 2^(10(x - 1))
float ExponentialEaseIn(float p)
{
	return (p == 0.0f) ? p : pow(2.0f, 10.0f * (p - 1.0f));
}

// Modeled after the exponential function y = -2^(-10x) + 1
float ExponentialEaseOut(float p)
{
	return (p == 1.0f) ? p : 1.0f - pow(2.0f, -10.0f * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
float ExponentialEaseInOut(float p)
{
	if(p == 0.0f || p == 1.0f) return p;

	if(p < 0.5f)
	{
		return 0.5f * pow(2.0f, (20.0f * p) - 10.0f);
	}
	else
	{
		return -0.5f * pow(2.0f, (-20.0f * p) + 10.0f) + 1.0f;
	}
}

std::optional<std::string> ModTwCmdSuperhot::onInitialize() {
	return Mod::onInitialize();
}

float previous{ 0.0f };
// onFrame()
// do something each frame example
void ModTwCmdSuperhot::onFrame(fmilliseconds& dt) {
	
	if (!mod_enabled) { return; }

	uPlayer* pl = Devil4SDK::getLocalPlayer();
	sWorkRate* sw = Devil4SDK::getWorkRate();

	if (!pl || !sw) { return; }

	float result = ExponentialEaseOut(glm::smoothstep(0.0f, 6.0f, pl->m_dVelMagnitude*1.4f));

	float value = (result * tiny) + (1.0f - tiny)*previous;
	sw->roomSpeed = value;
	sw->enemySpeed = value;
	previous = value;
}

// onConfigSave
// save your data into cfg structure.
//void ModTwCmdSuperhot::onConfigSave(utils::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void ModTwCmdSuperhot::onConfigLoad(const utils::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void ModTwCmdSuperhot::onGUIframe() { 
	ImGui::Checkbox("SUPERHOT Mode", &mod_enabled);
	ImGui::InputFloat("Velocity Factor", &tiny, 0.01f);
};

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

