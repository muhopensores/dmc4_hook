#pragma once

#include "../mod.hpp"

class KnockbackEdits : public Mod {
public:
    KnockbackEdits() = default;

    static bool mod_enabled;
    static uintptr_t jmp_return;
    static uintptr_t jmp_return2;

    std::string get_mod_name() override { return "KnockbackEdits"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook, hook2;
};
