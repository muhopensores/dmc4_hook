#pragma once

#include "../mod.hpp"

class Payline : public Mod {
public:
    Payline() = default;

    static bool mod_enabled;
    static bool helm_splitter_remap;

    static uintptr_t full_house;
    static uintptr_t nero_rave;
    static uintptr_t nero_streak_1;
    static uintptr_t helm_splitter_directional;
    static uintptr_t double_down_directional;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_jne5;
    static uintptr_t jmp_ret6;
    static uintptr_t jmp_ret7;
    static uintptr_t jmp_ret8;
    static uintptr_t jmp_ret9;
    static uintptr_t jmp_ret10;

    // void toggle(bool enable);
    static void Payline_Toggle();
    static void Helm_Splitter_Toggle();

    std::string get_mod_name() override { return "Payline"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void onUpdateInput(utility::Input& input) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5, hook6, hook7, hook8, hook9, hook10;
};
