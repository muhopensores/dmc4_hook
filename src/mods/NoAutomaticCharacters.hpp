#pragma once

#include "../mod.hpp"

class NoAutomaticCharacters : public Mod {
public:
    NoAutomaticCharacters() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_ret2;
    static int lastPickedCharacter;

    std::string get_mod_name() override { return "NoAutomaticCharacters"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<FunctionHook> hook2;
};
