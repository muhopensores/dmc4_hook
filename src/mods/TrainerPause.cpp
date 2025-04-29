#include "TrainerPause.hpp"
#include "WorkRate.hpp"
#include "../sdk/Devil4.hpp"
bool TrainerPause::mod_enabled = true;

std::optional<std::string> TrainerPause::on_initialize() {
	return Mod::on_initialize();
}

void TrainerPause::on_gui_frame(int display) {
	if (ImGui::Checkbox(_("Trainer Pause"), &mod_enabled)) {
		sWorkRate* wr = devil4_sdk::get_work_rate();
		if (!wr) { return; }
		if (mod_enabled)
			wr->global_speed = 0.0f;
		else
			if (!WorkRate::hotkey_paused) {
				wr->global_speed = 1.0f;
			}
	}
	ImGui::SameLine();
	help_marker(_("Pause the game when opening the trainer"));
}

void TrainerPause::on_config_load(const utility::Config & cfg) {
    mod_enabled = cfg.get<bool>("trainer_pause").value_or(true);
}

void TrainerPause::on_config_save(utility::Config & cfg) {
    cfg.set<bool>("trainer_pause", mod_enabled);
}
