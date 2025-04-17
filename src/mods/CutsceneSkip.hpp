#pragma once

#include "../mod.hpp"

class CutsceneSkip : public Mod {
public:
    CutsceneSkip() = default;

    static bool mod_enabled_intro;
    static bool mod_enabled_outro;

    void toggle_intro(bool enable);
    void toggle_outro(bool enable);

    std::string get_mod_name() override { return "CutsceneSkip"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
