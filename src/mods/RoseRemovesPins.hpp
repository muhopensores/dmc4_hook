#pragma once

#include "../mod.hpp"

class RoseRemovesPins : public Mod {
public:
    RoseRemovesPins() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    std::string get_mod_name() override { return "RoseRemovesPins"; };
    void no_pin_timer(bool enable);

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<Patch> patch;
    std::unique_ptr<Patch> patch2;
};
