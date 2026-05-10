#pragma once

#include "../mod.hpp"

class CustomModelFix : public Mod {
public:
    CustomModelFix() = default;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_jb1;

    static bool custom_nero_model_fix;
    static bool custom_dante_model_fix;

    static bool force_default_faceplate;
    void toggle_force_faceplate(bool enable);

    std::string get_mod_name() override { return "CustomModelFix"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<Patch> patchFaceplate;
};
