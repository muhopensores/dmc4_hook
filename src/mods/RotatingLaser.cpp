#include "RotatingLaser.hpp"
#include "../sdk/Devil4.hpp"

std::vector<std::unique_ptr<RotatingLaser>> RotatingLaser::s_lasers;
RotatingLaser::RotatingLaser(Vector3f pos, float length, float spawnDelay, float lifetime, int type)
    : m_pos(pos)
    , m_length(length)
    , m_spawnDelay(spawnDelay)
    , m_lifetime(lifetime)
    , m_rotationType(type) {
}

RotatingLaser* RotatingLaser::spawn(Vector3f pos, Vector3f rotXYZ, float length, float spawnDelay, float lifetime, int type) {
    auto laser = std::make_unique<RotatingLaser>(pos, length, spawnDelay, lifetime, type);
    auto* ptr  = laser.get();
    s_lasers.emplace_back(std::move(laser));
    glm::quat q     = glm::quat(glm::vec3(rotXYZ.x, rotXYZ.y, rotXYZ.z));
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
    m_spawned       = true;
}

void RotatingLaser::rotate(fmilliseconds& dt) {
    if (!m_actor) { return; }
    if (m_rotationType == 0) {
        m_actor->mQuat = m_skewQuat * m_baseQuat;
        m_actor->mPos  = m_pos;
        return;
    }
    m_angle += dt.count() * 0.001f;
    MtVector4 spin    = {0.0f, sinf(m_angle * 0.5f), 0.0f, cosf(m_angle * 0.5f)};
    MtVector4 rotated = spin * m_baseQuat;
    m_actor->mQuat    = m_skewQuat * rotated;
    float halfLen     = m_length * 0.5f;
    float s           = sinf(m_angle);
    float c           = cosf(m_angle);
    Vector3f dir      = {s, 0.0f, c};
    if (m_rotationType == 1) {
        m_actor->mPos = m_pos;
    } else if (m_rotationType == 2) {
        m_actor->mPos.x = m_pos.x - dir.x * halfLen;
        m_actor->mPos.y = m_pos.y;
        m_actor->mPos.z = m_pos.z - dir.z * halfLen;
    } else if (m_rotationType == 3) {
        m_actor->mPos   = m_pos;
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            m_actor->mPos.y = player->mPos.y + 100.0f;
        }
    }
}

void RotatingLaser::destroy() {
    if (m_actor && devil4_sdk::check_exists_in_moveline(m_actor, 10)) {
        uactor_sdk::despawn(m_actor);
    }
    m_actor = nullptr;
    m_dead  = true;
}

void RotatingLaser::kill_all() {
    for (auto& laser : s_lasers) {
        laser->destroy();
    }
    s_lasers.clear();
}

void RotatingLaser::update(fmilliseconds& dt) {
    if (m_dead) { return; }
    if (devil4_sdk::is_paused()) return;
    float delta = dt.count() * 0.001f;
    if (!m_spawned) {
        m_spawnTimer += delta;
        if (m_spawnTimer >= m_spawnDelay) {
            spawn_actor();
        }
        return;
    }
    m_aliveTimer += delta;
    if (m_lifetime > 0.0f && m_aliveTimer >= m_lifetime) {
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
