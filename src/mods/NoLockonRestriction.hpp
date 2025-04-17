#pragma once

#include "../mod.hpp"

class NoLockonRestriction : public Mod {
public:
    NoLockonRestriction() = default;

    static bool mod_enabled;

    void toggle(bool enable);
    std::string get_mod_name() override { return "NoLockonRestriction"; };
    //std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch1;
};