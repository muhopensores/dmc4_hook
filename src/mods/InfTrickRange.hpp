#pragma once

#include "../mod.hpp"

class InfTrickRange : public Mod {
public:
    InfTrickRange() = default;

    static bool mod_enabled;
    static bool mod_enabled_height;

    void toggle(bool enable);
    void toggle2(bool enable);

    std::string get_mod_name() override { return "InfTrickRange"; };
    std::vector<std::string> get_search_terms() override {
        return {
            "infinite trick range",
        };
    }

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch, patch2;
};
