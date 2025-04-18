#pragma once

#include "../mod.hpp"

extern uintptr_t HookDanteKADTbl;
extern uintptr_t HookNeroKADTbl;

class MoveTable: public Mod {
public:
    // static bool mod_enabled;
    static uintptr_t jmp_ret0;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;

    MoveTable() = default;

    // void toggle(bool enable);

    std::string get_mod_name() override { return "MoveTable"; };
    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
private:
    std::unique_ptr<FunctionHook> hook0, hook1, hook2, hook3;
    // std::unique_ptr<Patch> patch1;
};
