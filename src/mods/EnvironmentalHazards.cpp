#include "EnvironmentalHazards.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
#include "BerialPillar.hpp"
#include "RotatingLaser.hpp"
#include "../sdk/sArea.hpp"
#include "../sdk/aGame.hpp"

bool EnvironmentalHazards::laser_enabled = false;
bool EnvironmentalHazards::laser_tracks_player = false;
static bool pending_laser_spawn = false;
static float laser_spawn_timer = 0.0f;
static float laser_spawn_delay = 0.0f;

bool EnvironmentalHazards::pillar_enabled = false;
static float pillar_spawn_timer = 0.0f;
static float pillar_spawn_interval = 0.0f;
static bool inCorrectRoom;

void EnvironmentalHazards::spawn_bp_laser() {
    if (laser_tracks_player)
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 3);
    else
        RotatingLaser::spawn({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 1.57f}, 1500.0f, 1.0f, 0.0f, 1);
}

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
        if (devil4_sdk::is_in_bp()) {
            laser_spawn_timer += dt_sec;
            if (pending_laser_spawn && (laser_spawn_timer >= laser_spawn_delay)) {
                pending_laser_spawn = false;
                spawn_bp_laser();
            }
        }
        RotatingLaser::update_all(dt);
    }

    if (EnvironmentalHazards::pillar_enabled && inCorrectRoom) {
        pillar_spawn_timer += dt_sec;
        if (pillar_spawn_timer >= pillar_spawn_interval) {
            pillar_spawn_timer -= pillar_spawn_interval;
            pillar_spawn_interval = random_pillar_interval(0.2f, 6.0f);
            BerialPillar::spawn(random_pos_in_radius(Vector3f(0.0f, 0.0f, 0.0f), 0.0f, 1300.0f), 0.2f, 0.0f);
        }
        BerialPillar::update_all(dt);
    }
}

void EnvironmentalHazards::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::SeparatorText(_("Environmental Hazards"));
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("BP Laser Hazard"), &laser_enabled)) {
            if (devil4_sdk::get_local_player()) {
                if (laser_enabled) {
                    RotatingLaser::kill_all();
                    pending_laser_spawn = true;
                    laser_spawn_timer = 0.0f;
                    devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\room\\st405", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                } else {
                    RotatingLaser::kill_all();
                    pending_laser_spawn = false;
                    laser_spawn_timer = 0.0f;
                }
            }
        }
        ImGui::SameLine();
        help_marker(_("Enable this with Survival for extra hazards"));
        if (laser_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::Checkbox(_("Track player height"), &laser_tracks_player)) {
                if (devil4_sdk::get_local_player()) {
                    if (laser_enabled) {
                        RotatingLaser::kill_all();
                        pending_laser_spawn = true;
                        laser_spawn_timer   = 0.0f;
                    }
                }
            }
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();

        ImGui::SameLine(sameLineWidth);

        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("BP Fire Pillar Hazards"), &pillar_enabled)) {
            if (devil4_sdk::get_local_player()) {
                if (pillar_enabled) {
                    BerialPillar::kill_all();
                    pillar_spawn_interval = random_pillar_interval(1.0f, 30.0f);
                    devil4_sdk::get_stuff_from_files(
                        (MtDTI*)0x00ead4a0, "rom\\enemy\\em018", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                } else {
                    BerialPillar::kill_all();
                }
            }
        }
        ImGui::SameLine();
        help_marker(_("Enable this with Survival for extra hazards"));
        ImGui::EndGroup();
    }
}

void EnvironmentalHazards::on_stage_start() {
    RotatingLaser::kill_all();
    pending_laser_spawn  = false;
    laser_spawn_timer = 0.0f;
    if (laser_enabled && !devil4_sdk::is_loading_arc()) {
        pending_laser_spawn = true;
        devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\room\\st405", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
    }

    BerialPillar::kill_all();
    pillar_spawn_timer = 0.0f;
    if (pillar_enabled) {
        sArea* sAreaPtr = devil4_sdk::get_sArea();
        if (sAreaPtr) {
            aGame* aGamePtr = sAreaPtr->aGamePtr;
            if (aGamePtr) {
                int roomID = devil4_sdk::get_sArea()->aGamePtr->room_id;
                inCorrectRoom = roomID == 705 || roomID == 704 || roomID == 703 || roomID == 701 || roomID == 702 || roomID == 700;
                if (inCorrectRoom) {
                    pillar_spawn_interval = random_pillar_interval(1.0f, 30.0f);
                    devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em018", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                }
            }
        }
    }
}

std::optional<std::string> EnvironmentalHazards::on_initialize() {
    return Mod::on_initialize();
}

void EnvironmentalHazards::on_config_load(const utility::Config& cfg) {
    laser_enabled       = cfg.get<bool>("EnvironmentalHazards.laser").value_or(false);
    laser_tracks_player = cfg.get<bool>("EnvironmentalHazards.laser_tracks_player").value_or(false);
    pillar_enabled      = cfg.get<bool>("EnvironmentalHazards.pillar").value_or(false);
}

void EnvironmentalHazards::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("EnvironmentalHazards.laser", laser_enabled);
    cfg.set<bool>("EnvironmentalHazards.laser_tracks_player", laser_tracks_player);
    cfg.set<bool>("EnvironmentalHazards.pillar", pillar_enabled);
}
