#pragma once

#include "../mod.hpp"

class StylePoints : public Mod {
public:
    StylePoints() = default;

    static bool mod_enabled;
    static bool originalNames;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static bool tonyHawk;
    static bool moreGrouping;
    static bool showAirTimeDisplay;
    static bool showHeightChart;
    static bool showInertiaChart;

    std::string get_mod_name() override { return "StylePoints"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4;
};
