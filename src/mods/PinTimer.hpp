#pragma once

#include "../mod.hpp"

class PinTimer : public Mod {
public:
    PinTimer() = default;

    static bool mod_enabled;

    std::string get_mod_name() override { return "PinTimer"; };
    Mod::ModType get_mod_type() override { return SLOW; };
    void on_reset();
    void after_reset();

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
