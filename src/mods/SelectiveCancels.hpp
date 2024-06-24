#pragma once

#include "../mod.hpp"

// using powers of two here to avoid making a lot of bool variables
enum CancelMoves {                              // bits table
    ECSTASY         = 0x1,        // 0000 0000 0000 0000 0000 0000 0000 0001
    ARGUMENT        = 0x2,        // 0000 0000 0000 0000 0000 0000 0000 0010
    KICK13          = 0x4,        // 0000 0000 0000 0000 0000 0000 0000 0100
    SLASH_DIMENSION = 0x8,        // 0000 0000 0000 0000 0000 0000 0000 1000
    PROP            = 0x10,       // 0000 0000 0000 0000 0000 0000 0001 0000
    SHOCK           = 0x20,       // 0000 0000 0000 0000 0000 0000 0010 0000
    OMEN            = 0x40,       // 0000 0000 0000 0000 0000 0000 0100 0000
    GUNSTINGER      = 0x80,       // 0000 0000 0000 0000 0000 0000 1000 0000
    EPIDEMIC        = 0x100,      // 0000 0000 0000 0000 0000 0001 0000 0000
    DT_PIN_UP_P2    = 0x200,      // 0000 0000 0000 0000 0000 0010 0000 0000
    SHOWDOWN        = 0x400,      // 0000 0000 0000 0000 0000 0100 0000 0000
    DRAW            = 0x800,      // 0000 0000 0000 0000 0000 1000 0000 0000
    DTGROUND        = 0x1000,     // 0000 0000 0000 0000 0001 0000 0000 0000
    ROLL            = 0x2000,     // 0000 0000 0000 0000 0010 0000 0000 0000
    STINGER         = 0x4000,     // 0000 0000 0000 0000 0100 0000 0000 0000
    REAL_IMPACT     = 0x8000,     // 0000 0000 0000 0000 1000 0000 0000 0000
    // ?            = 0x10000,    // 0000 0000 0000 0001 0000 0000 0000 0000
    // ?            = 0x20000,    // 0000 0000 0000 0010 0000 0000 0000 0000
    // ?            = 0x40000,    // 0000 0000 0000 0100 0000 0000 0000 0000
    // ?            = 0x80000,    // 0000 0000 0000 1000 0000 0000 0000 0000
    // ?            = 0x100000,   // 0000 0000 0001 0000 0000 0000 0000 0000
    // ?            = 0x200000,   // 0000 0000 0010 0000 0000 0000 0000 0000
    // ?            = 0x400000,   // 0000 0000 0100 0000 0000 0000 0000 0000
    // ?            = 0x800000,   // 0000 0000 1000 0000 0000 0000 0000 0000
    // ?            = 0x1000000,  // 0000 0001 0000 0000 0000 0000 0000 0000
    // ?            = 0x2000000,  // 0000 0010 0000 0000 0000 0000 0000 0000
    // ?            = 0x4000000,  // 0000 0100 0000 0000 0000 0000 0000 0000
    // ?            = 0x8000000,  // 0000 1000 0000 0000 0000 0000 0000 0000
    // ?            = 0x10000000, // 0001 0000 0000 0000 0000 0000 0000 0000
    // ?            = 0x20000000, // 0010 0000 0000 0000 0000 0000 0000 0000
    // ?            = 0x40000000, // 0100 0000 0000 0000 0000 0000 0000 0000
    // ?            = 0x80000000, // 1000 0000 0000 0000 0000 0000 0000 0000
};

class SelectiveCancels : public Mod {
public:
    SelectiveCancels() = default;

    static bool mod_enabled;
    static uint32_t cancels;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t grief_jmp_ret1;
    static uintptr_t grief_jmp_ret2;

    std::string get_mod_name() override { return "SelectiveCancels"; };

    std::optional<std::string> on_initialize() override;

    void griefToggle(bool enable);

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

    inline void draw_checkbox_simple(const char* name, CancelMoves move);

private:
    std::unique_ptr<FunctionHook> hook1, hook2;
    std::unique_ptr<FunctionHook> hookGrief, hookGrief2;
    std::unique_ptr<Patch> patch1, patch2;
};
