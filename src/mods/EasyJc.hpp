#pragma once

#include "../mod.hpp"

class EasyJc : public Mod {
public:
    EasyJc() = default;

    static bool mod_enabled_nero;
    static bool mod_enabled_dante;
    static uintptr_t easy_jc_continue;

    std::string get_mod_name() override { return "EasyJc"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
