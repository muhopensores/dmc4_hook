#pragma once

#include "../mod.hpp"

class LdkWithDmd : public Mod {
public:
    LdkWithDmd() = default;

    static bool mod_enabled;
    // static bool modFrostsCantEscapeEnabled;
    static uintptr_t ldk_with_dmd_continue;
    void toggle(bool enable);

    std::string get_mod_name() override { return "LdkWithDmd"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<Patch> patch;
};
