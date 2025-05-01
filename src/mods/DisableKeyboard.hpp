#pragma once

#include "../mod.hpp"

class DisableKeyboard : public Mod {
public:
    DisableKeyboard() = default;

    static bool always_block_inputs;
    static bool auto_block_inputs;
    static uintptr_t jmp_ret;

    std::string get_mod_name() override { return "DisableKeyboard"; };

    std::optional<std::string> on_initialize() override;

    void on_update_input(utility::Input& input) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_game_pause(bool toggle) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
    // std::unique_ptr<Patch> patch;
};
