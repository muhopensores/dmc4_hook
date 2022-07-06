#pragma once

#include "../mod.hpp"

class CharacterSwap : public Mod {
public:
    CharacterSwap() = default;

    static bool nero_enabled;
    static bool dante_enabled;

    void toggle_nero(bool enable);
    void toggle_dante(bool enable);

    std::string get_mod_name() override { return "CharacterSwap"; };
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
    std::unique_ptr<Patch> patch8;
    std::unique_ptr<Patch> patch9;
    std::unique_ptr<Patch> patch10;
    std::unique_ptr<Patch> patch11;
    std::unique_ptr<Patch> patch12;
    std::unique_ptr<Patch> patch13;
    std::unique_ptr<Patch> patch14;
    std::unique_ptr<Patch> patch15;
    std::unique_ptr<Patch> patch16;
    std::unique_ptr<Patch> patch17;
    std::unique_ptr<Patch> patch18;
    std::unique_ptr<Patch> patch19;
    std::unique_ptr<Patch> patch20;
    std::unique_ptr<Patch> patch21;
};
