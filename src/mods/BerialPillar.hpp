#pragma once
#include "../sdk/uActor.hpp"

class pillar : public uActor {
public:
    char pad_1370[0x10];
};

class BerialPillar {
public:
    BerialPillar(Vector3f pos, float speed, float spawnDelay);
    static BerialPillar* spawn(Vector3f pos, float speed, float spawnDelay);
    static void update_all(fmilliseconds& dt);
    void destroy();
    static void kill_all();

private:
    bool is_dead() const { return m_dead; }
    void update(fmilliseconds& dt);
    void spawn_actor();
    pillar* m_actor      = nullptr;
    Vector3f m_pos       = {0.0f, 0.0f, 0.0f};
    float m_speed        = 0.0f;
    float m_spawnDelay   = 0.0f;
    float m_spawnTimer   = 0.0f;
    bool m_spawned       = false;
    bool m_dead          = false;
    static std::vector<std::unique_ptr<BerialPillar>> s_pillars;
};
