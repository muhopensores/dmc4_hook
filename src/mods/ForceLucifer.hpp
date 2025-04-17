#pragma once

#include "../mod.hpp"

class ForceLucifer : public Mod {
public:
    ForceLucifer() = default;

    static bool mod_enabled;
    static uintptr_t force_lucifer_continue;
    static bool enable_force_ecstasy_timer;

    std::string get_mod_name() override { return "ForceLucifer"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
