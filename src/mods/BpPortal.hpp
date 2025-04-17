#pragma once

#include "../mod.hpp"

class BpPortal : public Mod {
public:
    BpPortal() = default;

    static bool mod_enabled;
    void toggle(bool enable);

    std::string get_mod_name() override { return "BpPortal"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
    void on_update_input(utility::Input& input) override;

private:
    utility::Timer* m_timer;
    std::unique_ptr<Patch> patch;
};
