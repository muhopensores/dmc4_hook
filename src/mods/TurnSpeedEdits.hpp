#pragma once

#include "../mod.hpp"

class TurnSpeedEdits : public Mod {
public:
    TurnSpeedEdits() = default;

    static bool backslide_enable;

    static float turnSpeed;

    void backslide_toggle(bool enable);

    std::string get_mod_name() override { return "TurnSpeedEdits"; };
    std::vector<std::string> get_search_terms() override {
        return {
            "backslide", "turn speed",
        };
    }
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> backslide_patch;
};
