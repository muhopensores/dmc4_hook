#include "EnvironmentalHazards.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sMediator.hpp"
bool EnvironmentalHazards::laser_tracks_player = false;
static float stage_start_timer = 0.0f;
std::vector<std::unique_ptr<RotatingLaser>> RotatingLaser::s_lasers;

RotatingLaser::RotatingLaser(Vector3f pos, float length, float lifetime, int type)
    : m_pos(pos)
    , m_length(length)
    , m_lifetime(lifetime)
    , m_rotationType(type) {
    devil4_sdk::load_arc("rom\\room\\st405");
}

RotatingLaser* RotatingLaser::spawn(Vector3f pos, Vector3f rotXYZ, float length, float lifetime, int type) {
    auto laser = std::make_unique<RotatingLaser>(pos, length, lifetime, type);
    auto* ptr  = laser.get();
    s_lasers.emplace_back(std::move(laser));
    glm::quat q = glm::quat(glm::vec3(rotXYZ.x, rotXYZ.y, rotXYZ.z));
    ptr->m_baseQuat = {q.x, q.y, q.z, q.w};
    return ptr;
}

void RotatingLaser::spawn_actor() {
    m_actor = (laser*)devil4_sdk::easy_spawn(0x8825D0, 10);
    if (!m_actor) {
        m_dead = true;
        return;
    }
    m_actor->mPos   = m_pos;
    m_actor->length = m_length;
    m_actor->mQuat  = m_baseQuat;
    m_spawned = true;
}

void RotatingLaser::rotate(fmilliseconds& dt) {
    if (!m_actor) return;
    if (m_rotationType == 0) {
        m_actor->mQuat = m_skewQuat * m_baseQuat;
        m_actor->mPos  = m_pos;
        return;
    }
    m_angle += dt.count() * 0.001f;
    MtVector4 spin = {0.0f, sinf(m_angle * 0.5f), 0.0f, cosf(m_angle * 0.5f)};
    MtVector4 rotated = spin * m_baseQuat;
    m_actor->mQuat    = m_skewQuat * rotated;
    float halfLen = m_length * 0.5f;
    float s       = sinf(m_angle);
    float c       = cosf(m_angle);
    Vector3f dir = {s, 0.0f, c};
    if (m_rotationType == 1) {
        m_actor->mPos = m_pos;
    } else if (m_rotationType == 2) {
        m_actor->mPos.x = m_pos.x - dir.x * halfLen;
        m_actor->mPos.y = m_pos.y;
        m_actor->mPos.z = m_pos.z - dir.z * halfLen;
    }
}

void RotatingLaser::destroy() {
    if (m_actor && devil4_sdk::check_exists_in_moveline(m_actor, 10)) {
        uactor_sdk::despawn(m_actor);
    }
    m_actor = nullptr;
    m_dead = true;
}

void RotatingLaser::kill_all() {
    for (auto& laser : s_lasers) {
        laser->destroy();
    }
    s_lasers.clear();
}

void RotatingLaser::update(fmilliseconds& dt) {
    if (m_dead) return;
    float delta = dt.count() * 0.001f;
    if (!m_spawned) {
        m_spawnTimer += delta;
        if (m_spawnTimer >= m_spawnDelay) {
            spawn_actor();
        }
        return;
    }
    m_aliveTimer += delta;
    if (m_aliveTimer >= m_lifetime) {
        destroy();
        return;
    }
    rotate(dt);
}

void RotatingLaser::update_all(fmilliseconds& dt) {
    for (auto& laser : s_lasers) {
        laser->update(dt);
    }
    for (auto it = s_lasers.begin(); it != s_lasers.end();) {
        if ((*it)->is_dead()) {
            it = s_lasers.erase(it);
        } else {
            ++it;
        }
    }
}

// bp rotating laser mod
bool EnvironmentalHazards::mod_enabled = false;
uintptr_t EnvironmentalHazards::jmp_ret1 = NULL;
static laser* laserAddr = NULL;
static float laser_angle = 0.0f;

static void rotate_bp_laser(fmilliseconds& dt, laser* laser, float height) {
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

static void spawn_bp_laser() {
    if (!laserAddr || !devil4_sdk::check_exists_in_moveline(laserAddr, 10)) {
        laser* las = (laser*)devil4_sdk::easy_spawn(0x8825D0, 10);
        if (!las) {
            return;
        }
        laser_angle = 0.0f;
        las->length = 1500.0f;
        laserAddr   = las;
    }
}

static void prepare_bp_laser() {
    if (!laserAddr || !devil4_sdk::check_exists_in_moveline(laserAddr, 10)) {
        devil4_sdk::load_arc("rom\\room\\st405");
    }
}

static void kill_bp_laser() {
    if (laserAddr && devil4_sdk::check_exists_in_moveline(laserAddr, 10)) {
        uactor_sdk::despawn(laserAddr);
    }
    laserAddr = nullptr;
}

void EnvironmentalHazards::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (!devil4_sdk::is_playing()) return;
    if (devil4_sdk::is_paused()) return;
    if (stage_start_timer > 0.0f) {
        stage_start_timer += dt.count() * 0.001f;
        if (stage_start_timer > 1.0f) {
            if (mod_enabled && devil4_sdk::is_in_bp()) {
                kill_bp_laser();
                stage_start_timer = 0.0f;
                spawn_bp_laser();
            }
        }
    }

    RotatingLaser::update_all(dt);
    if (!devil4_sdk::is_in_bp()) return;

    if (!laserAddr || !devil4_sdk::check_exists_in_moveline(laserAddr, 10)) {
        laserAddr = nullptr;
        return;
    }
    static float laserHeight = 100.0f;
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        float deltaTime = dt.count() * 0.001f;
        float targetHeight = player->mPos.y + 100.0f;
        float speed        = 200.0f;
        laserHeight += std::clamp(targetHeight - laserHeight, -speed * deltaTime, speed * deltaTime);
        if (laser_tracks_player)
            rotate_bp_laser(dt, laserAddr, targetHeight);
        else
            rotate_bp_laser(dt, laserAddr, 100.0f);
    }
}

void EnvironmentalHazards::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Tracking BP Laser Hazard"), &mod_enabled)) {
            if (mod_enabled && devil4_sdk::is_in_bp()) {
                kill_bp_laser();
                prepare_bp_laser();
                spawn_bp_laser();
            } else {
                kill_bp_laser();
                RotatingLaser::kill_all();
            }
        }
        ImGui::SameLine();
        help_marker(_("Enable this before starting Survival for extra hazards"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            ImGui::Checkbox(_("Track player height"), &laser_tracks_player);
            ImGui::Unindent(lineIndent);
        }
    }
}

void EnvironmentalHazards::on_stage_start() {
    stage_start_timer = 0.001f;
    kill_bp_laser();
    RotatingLaser::kill_all();
    prepare_bp_laser();
}

// void EnvironmentalHazards::on_game_pause(bool toggle) {}
// bool EnvironmentalHazards::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

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
