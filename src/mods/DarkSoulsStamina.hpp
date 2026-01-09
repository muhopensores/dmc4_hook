#pragma once
#include "../mod.hpp"

class uPlayer;

class DarkSoulsStamina : public Mod {
public:
    DarkSoulsStamina() = default;
    std::string get_mod_name() override { return "DarkSoulsStamina"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    static float stamina_regen_rate;
    static float stamina_max;
    static float stamina_delay_max;

    static bool stamina_enabled;
    static float stamina;
    static float stamina_delay_current;

    bool stamina_use(int moveID);
    float get_move_cost(int moveID);
    void stamina_regen(float seconds);
    void check_for_stamina_use(uPlayer* player);

    static float stamina_spent_visual;
    static float stamina_ghost_delay_current;
    static float stamina_ghost_delay_max;
    static float stamina_ghost_rate;

    static uintptr_t jmp_ret1;

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    // std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1;
};
