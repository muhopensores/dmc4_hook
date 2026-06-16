#include "EnvironmentalHazards.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"

bool EnvironmentalHazards::mod_enabled = false;
uintptr_t EnvironmentalHazards::jmp_ret1 = NULL;
static uActor* laserAddr = NULL;
static bool arc_requested = false;
static bool laser_pending = false;
static float laser_timer  = 0.0f;
static float laser_angle = 0.0f;

static void rotate_laser(fmilliseconds& dt, uActor* laser, float height) {
    laser->mPos = {0.0f, height, 0.0f};
    laser_angle += dt.count() * 0.001f;
    MtVector4 base = {0.7071068f, 0.0f, 0.0f, 0.7071068f};
    MtVector4 spin = {0.0f, sinf(laser_angle * 0.5f), 0.0f, cosf(laser_angle * 0.5f)};
    MtVector4 q;
    q.w          = spin.w * base.w - spin.x * base.x - spin.y * base.y - spin.z * base.z;
    q.x          = spin.w * base.x + spin.x * base.w + spin.y * base.z - spin.z * base.y;
    q.y          = spin.w * base.y - spin.x * base.z + spin.y * base.w + spin.z * base.x;
    q.z          = spin.w * base.z + spin.x * base.y - spin.y * base.x + spin.z * base.w;
    laser->mQuat = q;
}

static uActor* spawn_laser(uActor* laserAddr) {
    if (!devil4_sdk::check_exists_in_moveline(laserAddr, 10))
        return devil4_sdk::easy_spawn(0x8825D0, 10);
    return laserAddr;
}

static void prepare_laser() {
    if (!devil4_sdk::is_in_bp()) return;
    devil4_sdk::load_arc("rom\\room\\st405");
    laser_pending = true;
    laser_timer   = 0.0f;
    laser_angle   = 0.0f;
}

static void kill_laser() {
    if (laserAddr) {
        uactor_sdk::despawn(laserAddr);
        laserAddr = nullptr;
    }
}

void EnvironmentalHazards::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (!devil4_sdk::is_playing()) return;
    if (!devil4_sdk::is_in_bp()) return;
    if (laser_pending) {
        laser_timer += dt.count() * 0.001f;
        if (laser_timer > 2.0f) {
            laserAddr = spawn_laser(laserAddr);
            laser_pending = false;
        }
        return;
    }

    if (!laserAddr) return;
    rotate_laser(dt, laserAddr, 100.0f);
}

void EnvironmentalHazards::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Environmental Hazards"), &mod_enabled)) {
            if (mod_enabled)
                prepare_laser();
            else
                kill_laser();
        }
        /*ImGui::SameLine();
        help_marker(_("Environmental Hazards Help Marker"));
        if (mod_enabled) {
            ImGui::InputScalar("[DEBUG] Laser Addr", ImGuiDataType_U32, &laserAddr, NULL, NULL, "%08x", ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);
        }*/
    }
}

void EnvironmentalHazards::on_stage_start() {
    if (mod_enabled) {
        prepare_laser();
    }
}

// void EnvironmentalHazards::on_game_pause(bool toggle) {}
// bool EnvironmentalHazards::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> EnvironmentalHazards::on_initialize() {
    return Mod::on_initialize();
}

void EnvironmentalHazards::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("EnvironmentalHazards").value_or(false);
}

void EnvironmentalHazards::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("EnvironmentalHazards", mod_enabled);
}
