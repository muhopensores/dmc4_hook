#pragma once
#include "../mod.hpp"

class Survival : public Mod {
public:
    Survival() = default;
    std::string get_mod_name() override { return "Survival"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    struct EnemyInfo {
        int enemies_alive;
        bool is_boss_spawned;
    };

    static bool mod_enabled;
    static bool meme_effects;
    static int wave;
    static std::shared_ptr<utility::Timer> timer;
    static std::random_device rd;
    static std::mt19937 rng;
    static bool player_existed_last_frame;

    static EnemyInfo get_enemy_info(uEnemy* enemy);
    static bool can_spawn_enemy(EnemyInfo enemy_info, SMediator* sMed);
    static void create_timer();
    static void reset_wave();
    static void on_timer_trigger();
    static void toggle(bool enable);
    static void toggle_basic_powerups(bool toggle);
    static void toggle_meme_powerups(bool toggle);

    static int get_random_int(int min, int max);
    static float get_random_float(float min, float max);
    static void spawn_kinda_random_enemy();
    static void spawn_boss_enemy();
    static void spawn_side_enemy();

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
};
