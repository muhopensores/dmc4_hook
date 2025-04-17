#pragma once

#include "../mod.hpp"

class RevFlying : public Mod {
public:
    RevFlying() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    void toggle(bool enable);
    void toggle2(bool enable);

    std::string get_mod_name() override { return "RevFlying"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
    std::unique_ptr<FunctionHook> hook1, hook2;
};
