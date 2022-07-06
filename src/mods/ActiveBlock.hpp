#pragma once

#include "../mod.hpp"

class ActiveBlock : public Mod {
public:
    ActiveBlock() = default;
    static bool mod_enabled;
    static uint32_t inputpressed;

    static uintptr_t jmp_return;
    static uintptr_t alt_ret;

    std::string get_mod_name() override { return "ActiveBlock"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
