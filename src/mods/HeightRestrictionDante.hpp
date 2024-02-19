#pragma once

#include "../mod.hpp"

class HeightRestrictionDante : public Mod {
public:
    HeightRestrictionDante() = default;

    static bool mod_enabled;
    static bool mod_airhike;
    void toggle(bool enable);
    void toggle2(bool enable);

    std::string get_mod_name() override { return "HeightRestrictionDante"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<Patch> patch_airhike;
};
