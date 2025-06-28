#pragma once
#include "../mod.hpp"

class MultiCharFixes : public Mod {
public:
    MultiCharFixes() = default;

    static bool targeting_fix;

    void toggle_targeting_fix(bool enabled);

    std::string get_mod_name() override { return "MultiCharFixes"; };

    //std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2;
    std::unique_ptr<Patch> patch1, patch2;
};
