#pragma once

#include "../mod.hpp"

class InfRevive : public Mod {
public:
    InfRevive() = default;

    static bool mod_enabled;

    void toggle(bool enable);

    std::string get_mod_name() override { return "InfRevive"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
    std::unique_ptr<Patch> patch4;
    std::unique_ptr<Patch> patch5;
    std::unique_ptr<Patch> patch6;
    std::unique_ptr<Patch> patch7;
};
