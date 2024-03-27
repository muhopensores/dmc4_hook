#pragma once

#include "../mod.hpp"

class CharSwitcher : public Mod {
public:
    CharSwitcher() = default;

    static bool mod_enabled;
    static bool inertia_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_ret6;
    static uintptr_t jmp_ret7;
    static uintptr_t jmp_ret8;
    static uintptr_t jmp_ret9;
    static uintptr_t jmp_ret10;
	
    void toggle(bool enable);
    void toggle2(bool enable);

    std::string get_mod_name() override { return "CharSwitcher"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1, patch2, patch3, patch4;
    std::unique_ptr<FunctionHook> hook2, hook3, hook4, hook5, hook6, hook7, hook8, hook9, hook10;
};
