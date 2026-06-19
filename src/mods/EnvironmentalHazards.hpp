#pragma once
#include "../mod.hpp"
#include "../sdk/uActor.hpp"

class laser : public uActor {
public:
    char pad_1370[0x14];
    float length; // 0x1384
};

class RotatingLaser {
public:
    static RotatingLaser* spawn(Vector3f pos, Vector3f rotXYZ, float length, float lifetime, int type);
    static void update_all(fmilliseconds& dt);
    void destroy();
    static void kill_all();
    RotatingLaser(Vector3f pos, float length, float lifetime, int type);

private:
    bool is_dead() const { return m_dead; }
    void update(fmilliseconds& dt);
    void spawn_actor();
    void rotate(fmilliseconds& dt);
    laser* m_actor = nullptr;
    Vector3f m_pos = {0.0f, 0.0f, 0.0f};
    MtVector4 rot = {0.0f, 0.0f, 0.0f, 0.0f};
    MtVector4 m_baseQuat = {0.0f, 0.0f, 0.0f, 1.0f};
    MtVector4 m_skewQuat = {1.0f, 0.0f, 0.0f, 0.0f};
    float m_length = 0.0f;
    float m_lifetime = 0.0f;
    int m_rotationType = 0;
    float m_spawnDelay = 2.0f; // probably best to not spawn an enemy, powerup, laser at the same time
    float m_spawnTimer = 0.0f;
    float m_aliveTimer = 0.0f;
    float m_angle      = 0.0f;
    bool m_spawned = false;
    bool m_dead    = false;
    static std::vector<std::unique_ptr<RotatingLaser>> s_lasers;
};

class EnvironmentalHazards : public Mod {
public:
    EnvironmentalHazards() = default;
    std::string get_mod_name() override { return "EnvironmentalHazards"; };
    // std::vector<std::string> get_search_terms() override { return {"mod sample"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;
    static bool laser_tracks_player;
    static uintptr_t jmp_ret1;

    // void toggle1(bool enable);

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_stage_start();
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1;
};
