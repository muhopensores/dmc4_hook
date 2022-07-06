#pragma once

#include "../mod.hpp"

class HeightRestrictionNero : public Mod {
public:
    HeightRestrictionNero() = default;

    static bool mod_enabled;

    void toggle(bool enable);

    std::string get_mod_name() override { return "HeightRestrictionNero"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch_buster;
    std::unique_ptr<Patch> patch_split;
    std::unique_ptr<Patch> patch_calibur;
    std::unique_ptr<Patch> patch_ex_calibur;
    std::unique_ptr<Patch> patch_snatch;
    std::unique_ptr<Patch> patch_rave;
    std::unique_ptr<Patch> patch_double_down;
    std::unique_ptr<Patch> patch_rev;
};
