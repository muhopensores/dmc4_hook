#pragma once

#include "../mod.hpp"

class SkipWeapons : public Mod {
public:
    SkipWeapons() = default;

    static bool skip_pandora;
    static bool skip_lucifer;
    static uintptr_t skip_dante_gun_continue;
    static uintptr_t skip_dante_sword_continue;

    std::string get_mod_name() override { return "SkipWeapons"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<FunctionHook> hook2;
};
