#pragma once
#include "../mod.hpp"

class Survival : public Mod {
public:
    Survival() = default;
    std::string get_mod_name() override { return "Survival"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    struct EnemyInfo {
        int enemies_alive;
        int side_enemies_alive;
        int bosses_alive;
    };

    static bool mod_enabled;
    static bool survival_active;
    static uintptr_t jmp_return_hp;
    static uintptr_t jmp_return_combat;
    static uintptr_t jmp_return_red_timer;
    static bool meme_effects;
    static ImVec2 window_pos;
    static int wave;
    static utility::Timer* Survival::timer;
    static utility::Timer* Survival::meme_timer;
    static float survivedTimer;
    static std::random_device rd;
    static std::mt19937 rng;
    static bool player_existed_last_frame;
    static int currentRoomIndex;

    static EnemyInfo get_enemy_info(uEnemy* enemy);
    static void reset_wave();
    static void on_timer_trigger();
    static void on_meme_timer_trigger();
    static void toggle(bool enable);
    static void meme_toggle(bool enable);

    static int get_random_int(int min, int max);
    static float get_random_float(float min, float max);
    static void spawn_standard_enemy();
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
    std::unique_ptr<FunctionHook> hook_hp, hook_combat, hook_red_timer;
};
