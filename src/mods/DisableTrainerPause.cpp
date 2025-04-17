#include "DisableTrainerPause.hpp"
#include "../sdk/Devil4.hpp"

bool DisableTrainerPause::mod_enabled = false;

std::optional<std::string> DisableTrainerPause::on_initialize() {
	return Mod::on_initialize();
}

void DisableTrainerPause::on_gui_frame(int display) {
	ImGui::Checkbox(_("Disable Game Pause when opening the trainer"), &mod_enabled);
}

void DisableTrainerPause::on_config_load(const utility::Config & cfg) {
    mod_enabled = cfg.get<bool>("disable_trainer_pause").value_or(false);
}

void DisableTrainerPause::on_config_save(utility::Config & cfg) {
    cfg.set<bool>("disable_trainer_pause", mod_enabled);
}
