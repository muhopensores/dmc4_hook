#include "MutatorSuperhot.hpp"
#include "../sdk/Devil4.hpp"
#include "glm/gtx/compatibility.hpp"

static bool mod_enabled_nero = false;
static bool mod_enabled_dante = false;

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
		.on_init([] { mod_enabled_nero = true; mod_enabled_dante = true; })
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
	if (mod_enabled_nero) {
		uPlayer* pl = devil4_sdk::get_local_player();
		sWorkRate* sw = devil4_sdk::get_work_rate();
		if (!pl || !sw) { return; }
		if (pl->controllerID == 1) {
			float result = exponential_ease_out(glm::smoothstep(0.0f, 6.0f, pl->m_d_vel_magnitude * 1.4f));
			float value = (result * tiny[0]) + (1.0f - tiny[0]) * previous;
			sw->room_speed = value;
			sw->enemy_speed = value;
			previous = value;
		}
	}
	if (mod_enabled_dante) {
		uPlayer* pl = devil4_sdk::get_local_player();
		sWorkRate* sw = devil4_sdk::get_work_rate();
		if (!pl || !sw) { return; }
		if (pl->controllerID == 0) {
			float result = exponential_ease_out(glm::smoothstep(0.0f, 6.0f, pl->m_d_vel_magnitude * 1.4f));
			float value = (result * tiny[1]) + (1.0f - tiny[1]) * previous;
			sw->room_speed = value;
			sw->enemy_speed = value;
			previous = value;
		}
	}
}

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.

static float speed_backup[2]{ 1.0f, 1.0f };
void MutatorSuperhot::on_gui_frame(int display) {
	if (display == 1) {
		ImGui::BeginGroup();
		if (ImGui::Checkbox(_("SUPERHOT Mode"), &mod_enabled_nero)) {
			sWorkRate* sw = devil4_sdk::get_work_rate();
			if (!sw) { return; }
			if (mod_enabled_nero) {
				speed_backup[0] = sw->room_speed;
				speed_backup[1] = sw->enemy_speed;
			}
			if (!mod_enabled_nero) {
				sw->room_speed = speed_backup[0];
				sw->enemy_speed = speed_backup[1];
			}
		}
		ImGui::SameLine();
		help_marker(_("Enemy speed scales with your movement speed"));
		if (mod_enabled_nero) {
			ImGui::Indent(lineIndent);
			ImGui::PushItemWidth(sameLineItemWidth);
			ImGui::InputFloat(_("Velocity Factor"), &tiny[0], 0.01f);
			ImGui::PopItemWidth();
			ImGui::Unindent(lineIndent);
		}
		ImGui::EndGroup();
	}
	else if (display == 2) {
		ImGui::BeginGroup();
		if (ImGui::Checkbox(_("SUPERHOT Mode"), &mod_enabled_dante)) {
			sWorkRate* sw = devil4_sdk::get_work_rate();
			if (!sw) { return; }
			if (mod_enabled_dante) {
				speed_backup[0] = sw->room_speed;
				speed_backup[1] = sw->enemy_speed;
			}
			if (!mod_enabled_dante) {
				sw->room_speed = speed_backup[0];
				sw->enemy_speed = speed_backup[1];
			}
		}
		ImGui::SameLine();
		help_marker(_("Enemy speed scales with your movement speed"));
		if (mod_enabled_dante) {
			ImGui::Indent(lineIndent);
			ImGui::PushItemWidth(sameLineItemWidth);
			ImGui::InputFloat(_("Velocity Factor"), &tiny[1], 0.01f);
			ImGui::PopItemWidth();
			ImGui::Unindent(lineIndent);
		}
		ImGui::EndGroup();
	}
}

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

// onConfigSave
// save your data into cfg structure.
//void ModTwCmdSuperhot::onConfigSave(utility::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void ModTwCmdSuperhot::onConfigLoad(const utility::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };