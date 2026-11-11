#pragma once
#include "../mod.hpp"

class uPlayer;

class DarkSoulsStamina : public Mod {
public:
    DarkSoulsStamina() = default;
    std::string get_mod_name() override { return "DarkSoulsStamina"; };
    std::vector<std::string> get_search_terms() override { return {"dark souls stamina"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    // settings, shared by every player
    static float stamina_regen_rate;
    static float stamina_max;
    static float stamina_delay_max;
    static float stamina_ghost_delay_max;
    static float stamina_ghost_rate;

    static bool stamina_enabled;
    static bool meme_stamina_enabled;

    // Each player has his own gauge (by player_info's index). The input hooks (detour1 here, InputStates' keyboard
    // detour2) look the hooked player up in gauge_player and drop his input while exhausted[slot] is set; on_frame
    // fills both.
    static constexpr int kSlots = 4;
    static uintptr_t gauge_player[kSlots];
    static uint8_t exhausted[kSlots];

    static float get_move_cost(uPlayer* player, int moveID);

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
