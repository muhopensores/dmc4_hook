#include "BerialPillar.hpp"
#include "../sdk/Devil4.hpp"

std::vector<std::unique_ptr<BerialPillar>> BerialPillar::s_pillars;
BerialPillar::BerialPillar(Vector3f pos, float speed, float spawnDelay)
    : m_pos(pos)
    , m_speed(speed)
    , m_spawnDelay(spawnDelay) {
}

BerialPillar* BerialPillar::spawn(Vector3f pos, float speed, float spawnDelay) {
    auto pillar = std::make_unique<BerialPillar>(pos, speed, spawnDelay);
    auto* ptr  = pillar.get();
    s_pillars.emplace_back(std::move(pillar));
    return ptr;
}

void BerialPillar::spawn_actor() {
    m_actor = (pillar*)devil4_sdk::easy_spawn(0x649200, 10);
    if (!m_actor) {
        m_dead = true;
        return;
    }
    m_actor->mPos = m_pos;
    m_actor->mWorkRate.mWorkRate = m_speed;
    m_spawned = true;
}

void BerialPillar::destroy() {
    if (m_actor && devil4_sdk::check_exists_in_moveline(m_actor, 10)) {
        uactor_sdk::despawn(m_actor);
    }
    m_actor = nullptr;
    m_dead  = true;
}

void BerialPillar::kill_all() {
    for (auto& pillar : s_pillars) {
        pillar->destroy();
    }
    s_pillars.clear();
}

void BerialPillar::update(fmilliseconds& dt) {
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
}

void BerialPillar::update_all(fmilliseconds& dt) {
    for (auto& pillar : s_pillars) {
        pillar->update(dt);
    }
    for (auto it = s_pillars.begin(); it != s_pillars.end();) {
        if ((*it)->is_dead()) {
            it = s_pillars.erase(it);
        } else {
            ++it;
        }
    }
}
