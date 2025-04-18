#pragma once

#include "../mod.hpp"

class TrackingFullHouse : public Mod {
public:
    TrackingFullHouse() = default;

    static bool tracking_full_house_nero;
    static bool tracking_full_house_dante;

    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    std::string get_mod_name() override { return "TrackingFullHouse"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
