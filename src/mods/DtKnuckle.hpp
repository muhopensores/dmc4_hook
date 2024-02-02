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

    void toggle(bool enable);

    std::string get_mod_name() override { return "DtKnuckle"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1, patch2, patch3, patch4, patch5, patch6, patch7, patch8, patch9, patch10, patch11;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5, hook6, hook7, hook8, hook9, hook10, hook11, hook12, hook13;
};

static const std::vector<std::pair<uint16_t, const char*>> buttonPairs = {
    {0x00000400, "Left Trigger"},
    {0x00000800, "Right Trigger"},
    {0x00000100, "Left Shoulder"},
    {0x00000200, "Right Shoulder"},
    {0x00001000, "Y"},
    {0x00002000, "B"},
    {0x00004000, "A"},
    {0x00008000, "X"},
    {0x00000001, "Back"},
    {0x00000002, "Left Thumb"},
    {0x00000004, "Right Thumb"},
    {0x00000008, "Start"},
    {0x00000010, "Dpad Up"},
    {0x00000020, "Dpad Right"},
    {0x00000040, "Dpad Down"},
    {0x00000080, "Dpad Left"}
};
