#include "EnvironmentalHazards.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
#include "RotatingLaser.hpp"

bool EnvironmentalHazards::mod_enabled = false;
bool EnvironmentalHazards::laser_tracks_player = false;

void EnvironmentalHazards::spawn_bp_laser() {
    if (laser_tracks_player)
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 3);
    else
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 1);
}

void EnvironmentalHazards::on_frame(fmilliseconds& dt) {
    if (EnvironmentalHazards::mod_enabled) {
        RotatingLaser::update_all(dt);
    }
}

void EnvironmentalHazards::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Tracking BP Laser Hazard"), &mod_enabled)) {
            if (mod_enabled && devil4_sdk::is_in_bp()) {
                RotatingLaser::kill_all();
                spawn_bp_laser();
            } else {
                RotatingLaser::kill_all();
            }
        }
        ImGui::SameLine();
        help_marker(_("Enable this before starting Survival for extra hazards"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::Checkbox(_("Track player height"), &laser_tracks_player)) {
                if (mod_enabled) {
                    RotatingLaser::kill_all();
                    spawn_bp_laser();
                }
            }
            ImGui::Unindent(lineIndent);
        }
    }
}

void EnvironmentalHazards::on_stage_start() {
    RotatingLaser::kill_all();
    if (mod_enabled && devil4_sdk::is_in_bp()) {
        spawn_bp_laser();
    }
}

std::optional<std::string> EnvironmentalHazards::on_initialize() {
    return Mod::on_initialize();
}

void EnvironmentalHazards::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("EnvironmentalHazards").value_or(false);
    laser_tracks_player = cfg.get<bool>("laser_tracks_player").value_or(false);
}

void EnvironmentalHazards::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("EnvironmentalHazards", mod_enabled);
    cfg.set<bool>("laser_tracks_player", laser_tracks_player);
}
