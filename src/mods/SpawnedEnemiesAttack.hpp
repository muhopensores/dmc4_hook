#pragma once
#include "../mod.hpp"

class SpawnedEnemiesAttack : public Mod {
public:
    SpawnedEnemiesAttack() = default;
    std::string get_mod_name() override { return "SpawnedEnemiesAttack"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t detour1_je;

    // void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook1;
};
