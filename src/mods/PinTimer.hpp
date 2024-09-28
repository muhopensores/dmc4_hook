#pragma once

#include "../mod.hpp"

class PinTimer : public Mod {
public:
    PinTimer() = default;

    static bool mod_enabled;

    void custom_imgui_window();

    std::string get_mod_name() override { return "PinTimer"; };
    Mod::ModType get_mod_type() override { return SLOW; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
