#pragma once

#include "../mod.hpp"

class VisualizeHitbox : public Mod {
public:
    VisualizeHitbox() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    //static uintptr_t jmp_ret1;
    //static uintptr_t jmp_ret2;

    std::string get_mod_name() override { return "VisualizeHitbox"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook1;
};
