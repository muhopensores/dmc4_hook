#pragma once

#include "../mod.hpp"

class DmdLevelAi : public Mod {
public:
    DmdLevelAi() = default;

    static bool mod_enabled;
    static bool frenzied_enabled;

    void toggle(bool enable);
    void frenzied_toggle(bool enable);

    std::string get_mod_name() override { return "DmdLevelAi"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    // std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<Patch> patch, patch2, patch3, patch4, patch5, patch6, patch7, patch8, patch9;
};
