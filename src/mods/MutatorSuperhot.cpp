#include "MutatorSuperhot.hpp"
#include "../sdk/Devil4.hpp"

#include "glm/gtx/compatibility.hpp"

static bool mod_enabled{ false };

// Modeled after the exponential function y = 2^(10(x - 1))
float exponential_ease_in(float p) {
	return (p == 0.0f) ? p : pow(2.0f, 10.0f * (p - 1.0f));
}

// Modeled after the exponential function y = -2^(-10x) + 1
float exponential_ease_out(float p) {
	return (p == 1.0f) ? p : 1.0f - pow(2.0f, -10.0f * p);
}

// Modeled after the piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
float exponential_ease_in_out(float p) {
	if(p == 0.0f || p == 1.0f) return p;

	if(p < 0.5f) {
		return 0.5f * pow(2.0f, (20.0f * p) - 10.0f);
	}
	
		return -0.5f * pow(2.0f, (-20.0f * p) + 10.0f) + 1.0f;

}

std::optional<std::string> MutatorSuperhot::on_initialize() {
	MutatorRegistry::define("SuperHot")
		.description("Superhot mode")
		.alias("hw")
		.on_init([] { mod_enabled = true; })
		.set_timer(20.0f, [] {
		sWorkRate* sw = devil4_sdk::get_work_rate();
		if (!sw) { return; }
		sw->room_speed = 1.0f;
		sw->enemy_speed = 1.0f; });
	return Mod::on_initialize();
}

float previous{ 0.0f };
// onFrame()
// do something each frame example
void MutatorSuperhot::on_frame(fmilliseconds& dt) {
	
	if (!mod_enabled) { return; }

	uPlayer* pl = devil4_sdk::get_local_player();
	sWorkRate* sw = devil4_sdk::get_work_rate();

	if (!pl || !sw) { return; }

	float result = exponential_ease_out(glm::smoothstep(0.0f, 6.0f, pl->m_d_vel_magnitude*1.4f));

	float value = (result * tiny) + (1.0f - tiny)*previous;
	sw->room_speed = value;
	sw->enemy_speed = value;
	previous = value;
}

// onConfigSave
// save your data into cfg structure.
//void ModTwCmdSuperhot::onConfigSave(utility::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void ModTwCmdSuperhot::onConfigLoad(const utility::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void MutatorSuperhot::on_gui_frame() {
	ImGui::Checkbox("SUPERHOT Mode", &mod_enabled);
    if (mod_enabled) {
        ImGui::PushItemWidth(sameLineItemWidth);
		ImGui::InputFloat("Velocity Factor", &tiny, 0.01f);
        ImGui::PopItemWidth();
	}
    ImGui::SameLine();
    help_marker("Enemy speed scales with your movement speed");
}

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

