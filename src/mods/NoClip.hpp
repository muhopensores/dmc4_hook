#pragma once

#include "../mod.hpp"

#include "utility/Timer.hpp"

class NoClip : public Mod {
public:
    NoClip() = default;

    static bool mod_enabled;

    void toggle(bool enable);

    std::string get_mod_name() override { return "NoClip"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;
    void on_frame(fmilliseconds& dt) override;
    void on_update_input(utility::Input& input) override;

private:
    utility::Timer* m_timer;

    std::size_t m_command;
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
