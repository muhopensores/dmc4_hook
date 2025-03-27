#pragma once

#include "../mod.hpp"

class CharacterSwap : public Mod {
public:
    CharacterSwap() = default;

    static bool mod_enabled;
    static bool prefer_dante;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_jne;

    void toggle(bool enable);
    void Prefer_Dante(bool enable);


    std::string get_mod_name() override { return "CharacterSwap"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<FunctionHook> hook2;
};
