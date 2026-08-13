#include "../sdk/Devil4.hpp"
#include "../sdk/sWorkRate.hpp"
#include "DisappearingPlatform.hpp"

std::vector<std::unique_ptr<DisappearingPlatform>> DisappearingPlatform::s_platforms;
DisappearingPlatform::DisappearingPlatform(Vector3f pos, Vector3f scale, float speed, float spawnDelay, float lifetime)
    : m_pos(pos)
    , m_scale(scale)
    , m_speed(speed)
    , m_spawnDelay(spawnDelay)
    , m_lifetime(lifetime) {
}

DisappearingPlatform* DisappearingPlatform::spawn(Vector3f pos, Vector3f scale, float speed, float spawnDelay, float lifetime) {
    auto platform = std::make_unique<DisappearingPlatform>(pos, scale, speed, spawnDelay, lifetime);
    auto* ptr     = platform.get();
    s_platforms.emplace_back(std::move(platform));
    return ptr;
}

void DisappearingPlatform::spawn_actor() {
    m_actor = (platform*)devil4_sdk::easy_spawn(0x881950, 7);
    if (!m_actor) {
        m_dead = true;
        return;
    }
    m_actor->mPos                = m_pos;
    m_actor->mWorkRate.mWorkRate = m_speed;
    m_actor->mScale              = m_scale;
    m_actor->mPlatformType       = 1;
    m_spawned                    = true;
}

void DisappearingPlatform::destroy() {
    if (m_actor && devil4_sdk::check_exists_in_moveline(m_actor, 7)) {
        uactor_sdk::despawn(m_actor);
    }
    m_actor = nullptr;
    m_dead  = true;
}

void DisappearingPlatform::kill_all() {
    for (auto& platform : s_platforms) {
        platform->destroy();
    }
    s_platforms.clear();
}

void DisappearingPlatform::update(fmilliseconds& dt) {
    if (m_dead) {
        return;
    }
    if (devil4_sdk::is_paused())
        return;
    float delta = dt.count() * 0.001f;

    if (auto* work_rate = devil4_sdk::get_work_rate()) {
        delta *= work_rate->global_speed;
    }

    if (!m_spawned) {
        m_spawnTimer += delta;
        if (m_spawnTimer >= m_spawnDelay) {
            spawn_actor();
        }
        return;
    }

    m_lifeTimer += delta;
    if (m_lifeTimer >= m_lifetime) {
        destroy();
    }
}

void DisappearingPlatform::update_all(fmilliseconds& dt) {
    for (auto& platform : s_platforms) {
        platform->update(dt);
    }
    for (auto it = s_platforms.begin(); it != s_platforms.end();) {
        if ((*it)->is_dead()) {
            it = s_platforms.erase(it);
        } else {
            ++it;
        }
    }
}
