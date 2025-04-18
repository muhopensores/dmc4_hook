#pragma once

#include "../mod.hpp"

class DtKnuckle : public Mod {
public:
    DtKnuckle() = default;

    static bool mod_enabled;
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
    static uintptr_t jmp_ret11;
    static uintptr_t jmp_ret12;
    static uintptr_t jmp_ret13;
    static uintptr_t jmp_ret14;
    static uintptr_t jmp_ret15;
    static uintptr_t jmp_ret16;

    void toggle(bool enable);

    std::string get_mod_name() override { return "DtKnuckle"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch1, patch2, patch3, patch4, patch5, patch6, patch7, patch8, patch9, patch10, patch11, patch12;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5, hook6, hook7, hook8, hook9, hook10, hook11, hook12, hook13, hook14, hook15 \
        ,hook16;
};

enum DtStandModes {
    DT_ACTIVATION = 1,
    STAND_BUSTER,
    STAND_SNATCH,
    STAND_ATTACK,
    STAND_HOLD = 7
};