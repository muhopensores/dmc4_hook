#pragma once
#include "../mod.hpp"

class GermanWord : public Mod {
public:
    GermanWord() = default;
    std::string get_mod_name() override { return "GermanWord"; };
    // std::vector<std::string> get_search_terms() override { return {"mod sample"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;
    static bool survival_doppel_enabled;
    static bool spawn_queued;
    // static uintptr_t jmp_ret1;

    void toggle1(bool enable);
    void toggle2(bool enable);
    void toggle3(bool enable);
    void DoppelSpawnLogic(bool mod_enabled);

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_stage_start() override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<Patch> patch1, patch2, patch3;
    // std::unique_ptr<FunctionHook> hook1;
};
