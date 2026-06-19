#pragma once
#include "../sdk/uActor.hpp"

class laser : public uActor {
public:
    char pad_1370[0x14];
    float length; // 0x1384
};

class RotatingLaser {
public:
    RotatingLaser(Vector3f pos, float length, float spawnDelay, float lifetime, int type);
    static RotatingLaser* spawn(Vector3f pos, Vector3f rotXYZ, float length, float spawnDelay, float lifetime, int type);
    static void update_all(fmilliseconds& dt);
    void destroy();
    static void kill_all();

private:
    bool is_dead() const { return m_dead; }
    void update(fmilliseconds& dt);
    void spawn_actor();
    void rotate(fmilliseconds& dt);
    laser* m_actor       = nullptr;
    Vector3f m_pos       = {0.0f, 0.0f, 0.0f};
    MtVector4 rot        = {0.0f, 0.0f, 0.0f, 0.0f};
    MtVector4 m_baseQuat = {0.0f, 0.0f, 0.0f, 1.0f};
    MtVector4 m_skewQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    float m_length       = 0.0f;
    float m_lifetime     = 0.0f;
    int m_rotationType   = 0;
    float m_spawnDelay   = 0.0f; // probably best to not spawn an enemy, powerup, laser at the same time
    float m_spawnTimer   = 0.0f;
    float m_aliveTimer   = 0.0f;
    float m_angle        = 0.0f;
    bool m_spawned       = false;
    bool m_dead          = false;
    static std::vector<std::unique_ptr<RotatingLaser>> s_lasers;
};
