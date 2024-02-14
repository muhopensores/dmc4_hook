#pragma once

#include "../mod.hpp"

class CharSwitcher : public Mod {
public:
    CharSwitcher() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;

    std::string get_mod_name() override { return "CharSwitcher"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook1;
};
