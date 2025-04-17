#pragma once

#include "../mod.hpp"

class HeightRestriction : public Mod {
public:
    HeightRestriction() = default;

    static bool toggle_nero;
    void ToggleNero(bool enable);

    static bool toggle_dante;
    void ToggleDante(bool enable);

    static uintptr_t air_hike_jmp_ret;
    static bool air_hike_nero;
    static bool air_hike_dante;

    std::string get_mod_name() override { return "HeightRestriction"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch_buster;
    std::unique_ptr<Patch> patch_split;
    std::unique_ptr<Patch> patch_calibur;
    std::unique_ptr<Patch> patch_ex_calibur;
    std::unique_ptr<Patch> patch_snatch;
    std::unique_ptr<Patch> patch_rave;
    std::unique_ptr<Patch> patch_double_down;
    std::unique_ptr<Patch> patch_rev;

    std::unique_ptr<Patch> patch_dante;

    std::unique_ptr<FunctionHook> hook_air_hike;
};
