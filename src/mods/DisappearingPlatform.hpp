#pragma once
#include "../sdk/uActor.hpp"

class platform : public uActor {
public:
    char pad_1370[0x10];
    char pad_1380[0xC];
    int mPlatformType; // 0x138C
};

class DisappearingPlatform {
public:
    DisappearingPlatform(Vector3f pos, Vector3f scale = {1.0f, 1.0f, 1.0f}, float speed = 1.0f, float spawnDelay = 0.0f, float lifetime = 3.3f);
    static DisappearingPlatform* spawn(Vector3f pos, Vector3f scale = {1.0f, 1.0f, 1.0f}, float speed = 1.0f, float spawnDelay = 0.0f, float lifetime = 3.3f);
    static void update_all(fmilliseconds& dt);
    void destroy();
    static void kill_all();

private:
    bool is_dead() const { return m_dead; }
    void update(fmilliseconds& dt);
    void spawn_actor();
    platform* m_actor    = nullptr;
    Vector3f m_pos     = {0.0f, 0.0f, 0.0f};
    Vector3f m_scale   = {1.0f, 1.0f, 1.0f};
    float m_speed      = 0.0f;
    float m_spawnDelay = 0.0f;
    float m_spawnTimer = 0.0f;
    float m_lifetime  = 3.3f;
    float m_lifeTimer = 0.0f;
    bool m_spawned     = false;
    bool m_dead        = false;
    static std::vector<std::unique_ptr<DisappearingPlatform>> s_platforms;
};
