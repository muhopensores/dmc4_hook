#include "EnvironmentalHazards.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
#include "BerialPillar.hpp"
#include "RotatingLaser.hpp"

// lots of stupid delays to try to minimize chances of things not loading correctly, both for arc loading and for making entitties
bool EnvironmentalHazards::laser_enabled       = false;
bool EnvironmentalHazards::laser_tracks_player = false;
static bool pending_laser_arc_load             = false;
static bool laser_arc_loaded                   = false;
static bool pending_laser_spawn                = false;
static float laser_first_spawn_timer           = 0.0f;
static float laser_first_spawn_delay           = 0.0f;

void EnvironmentalHazards::spawn_bp_laser() {
    if (laser_tracks_player)
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 3);
    else
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 1);
}

bool EnvironmentalHazards::pillar_enabled = false;
static bool pending_pillar_arc_load       = false;
static bool pillar_arc_loaded             = false;
static float pillar_first_spawn_timer     = 0.0f;
static float pillar_first_spawn_delay     = 0.0f;

static float pillar_spawn_timer    = 0.0f;
static float pillar_spawn_interval = 0.0f;

static float pillar_spawn_timer2    = 0.0f;
static float pillar_spawn_interval2 = 0.0f;

static float random_pillar_interval(float min, float max) {
    float t = (float)rand() / (float)RAND_MAX;
    return min + t * (max - min);
}

static Vector3f random_pos_in_radius(Vector3f origin, float radius_min, float radius_max) {
    float angle = (float)rand() / RAND_MAX * 2.0f * 3.14159265f;
    float t     = (float)rand() / RAND_MAX;
    float r0    = radius_min * radius_min;
    float r1    = radius_max * radius_max;
    float dist  = sqrtf(r0 + (r1 - r0) * t);
    return {origin.x + dist * cosf(angle), origin.y, origin.z + dist * sinf(angle)};
}

void EnvironmentalHazards::on_frame(fmilliseconds& dt) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (devil4_sdk::is_paused() || !player)
        return;

    const float dt_sec = dt.count() / 1000.0f;

    if (EnvironmentalHazards::laser_enabled) {
        if (pending_laser_arc_load && !devil4_sdk::is_loading_arc()) {
            pending_laser_arc_load = false;
            devil4_sdk::load_arc("rom\\room\\st405");
        }

        if (devil4_sdk::is_in_bp()) {
            if (!laser_arc_loaded && !devil4_sdk::is_loading_arc()) {
                laser_arc_loaded        = true;
                laser_first_spawn_timer = 0.0f;
                laser_first_spawn_delay = 1.0f;
                pending_laser_spawn     = true;
            }

            if (laser_arc_loaded && pending_laser_spawn) {
                laser_first_spawn_timer += dt_sec;
                if (laser_first_spawn_timer >= laser_first_spawn_delay) {
                    pending_laser_spawn = false;
                    spawn_bp_laser();
                }
            }
        }
        RotatingLaser::update_all(dt);
    }

    if (EnvironmentalHazards::pillar_enabled) {
        if (pending_pillar_arc_load && !devil4_sdk::is_loading_arc()) {
            pending_pillar_arc_load = false;
            devil4_sdk::load_arc("rom\\enemy\\em018");
        }

        if (!pillar_arc_loaded && !devil4_sdk::is_loading_arc()) {
            pillar_arc_loaded        = true;
            pillar_first_spawn_timer = 0.0f;
            pillar_first_spawn_delay = 1.0f;
            pillar_spawn_timer       = 0.0f;
            pillar_spawn_interval    = random_pillar_interval(1.0f, 30.0f);
        }

        if (pillar_arc_loaded) {
            pillar_first_spawn_timer += dt_sec;
            if (pillar_first_spawn_timer < pillar_first_spawn_delay) {
                BerialPillar::update_all(dt);
                return;
            }

            /*pillar_spawn_timer += dt_sec;

            if (pillar_spawn_timer >= pillar_spawn_interval) {
                pillar_spawn_timer -= pillar_spawn_interval;
                pillar_spawn_interval = random_pillar_interval();

                BerialPillar::spawn(player->mPos, 0.5f, 0.0f);
            }*/

            pillar_spawn_timer2 += dt_sec;

            if (pillar_spawn_timer2 >= pillar_spawn_interval2) {
                pillar_spawn_timer2 -= pillar_spawn_interval2;
                pillar_spawn_interval2 = random_pillar_interval(0.2f, 6.0f);
                BerialPillar::spawn(random_pos_in_radius(Vector3f(0.0f, 0.0f, 0.0f), 0.0f, 1300.0f), 0.2f, 0.0f);
            }
            BerialPillar::update_all(dt);
        }
    }
}

void EnvironmentalHazards::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("BP Laser Hazard"), &laser_enabled)) {
            if (laser_enabled) {
                RotatingLaser::kill_all();
                laser_arc_loaded        = false;
                pending_laser_spawn     = false;
                laser_first_spawn_timer = 0.0f;
                pending_laser_arc_load  = true;
            } else {
                RotatingLaser::kill_all();
                laser_arc_loaded        = false;
                pending_laser_arc_load  = false;
                pending_laser_spawn     = false;
                laser_first_spawn_timer = 0.0f;
            }
        }
        ImGui::SameLine();
        help_marker(_("Enable this before starting Survival for extra hazards"));
        if (laser_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::Checkbox(_("Track player height"), &laser_tracks_player)) {
                if (laser_enabled) {
                    RotatingLaser::kill_all();
                    laser_arc_loaded        = false;
                    pending_laser_spawn     = false;
                    laser_first_spawn_timer = 0.0f;
                    pending_laser_arc_load  = true;
                }
            }
            ImGui::Unindent(lineIndent);
        }

        if (ImGui::Checkbox(_("BP Pillar Hazard"), &pillar_enabled)) {
            if (pillar_enabled) {
                BerialPillar::kill_all();
                pillar_arc_loaded        = false;
                pillar_first_spawn_timer = 0.0f;
                pending_pillar_arc_load  = true;
            } else {
                BerialPillar::kill_all();
                pillar_arc_loaded        = false;
                pending_pillar_arc_load  = false;
                pillar_first_spawn_timer = 0.0f;
            }
        }
        ImGui::EndGroup();
    }
}

void EnvironmentalHazards::on_stage_start() {
    RotatingLaser::kill_all();
    laser_arc_loaded        = false;
    pending_laser_arc_load  = false;
    pending_laser_spawn     = false;
    laser_first_spawn_timer = 0.0f;
    if (laser_enabled) {
        pending_laser_arc_load = true;
    }

    BerialPillar::kill_all();
    pillar_arc_loaded        = false;
    pending_pillar_arc_load  = false;
    pillar_first_spawn_timer = 0.0f;
    pillar_spawn_timer       = 0.0f;
    if (pillar_enabled) {
        pending_pillar_arc_load = true;
    }
}

std::optional<std::string> EnvironmentalHazards::on_initialize() {
    return Mod::on_initialize();
}

void EnvironmentalHazards::on_config_load(const utility::Config& cfg) {
    laser_enabled       = cfg.get<bool>("EnvironmentalHazards_laser").value_or(false);
    laser_tracks_player = cfg.get<bool>("laser_tracks_player").value_or(false);
    pillar_enabled      = cfg.get<bool>("EnvironmentalHazards_pillar").value_or(false);
}

void EnvironmentalHazards::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("EnvironmentalHazards_laser", laser_enabled);
    cfg.set<bool>("laser_tracks_player", laser_tracks_player);
    cfg.set<bool>("EnvironmentalHazards_pillar", pillar_enabled);
}
