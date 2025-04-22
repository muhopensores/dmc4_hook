#pragma once

#include "../mod.hpp"

class FastRoundTrip : public Mod {
public:
    FastRoundTrip() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static float start_frame;

    std::string get_mod_name() override { return "FastRoundTrip"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
