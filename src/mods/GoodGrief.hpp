#pragma once

#include "../mod.hpp"

class GoodGrief : public Mod {
public:
    GoodGrief() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    void toggle(bool enable);

    std::string get_mod_name() override { return "GoodGrief"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1, patch2;
    std::unique_ptr<FunctionHook> hook1, hook2;
};