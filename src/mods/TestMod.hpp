#pragma once
#include "../mod.hpp"

class TestMod : public Mod {
public:
    TestMod() = default;
    std::string get_mod_name() override { return "TestMod"; };

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
};
