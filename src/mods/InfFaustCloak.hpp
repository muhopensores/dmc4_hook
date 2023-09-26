#pragma once

#include "../mod.hpp"

class InfFaustCloak : public Mod {
public:
    InfFaustCloak() = default;

    static bool always_cloak;
    static bool no_cloak;

    void toggle1(bool enable);
    void toggle2(bool enable);

    std::string get_mod_name() override { return "InfFaustCloak"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
