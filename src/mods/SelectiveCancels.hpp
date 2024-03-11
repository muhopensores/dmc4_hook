#pragma once

#include "../mod.hpp"

// using powers of two here to avoid making a lot of bool variables
enum CancelMoves {                // bits table
    ECSTASY         = 1,          // 0000 0000 0000 0000 0000 0000 0000 0001
    ARGUMENT        = 2,          // 0000 0000 0000 0000 0000 0000 0000 0010
    KICK13          = 4,          // 0000 0000 0000 0000 0000 0000 0000 0100
    SLASH_DIMENSION = 8,          // 0000 0000 0000 0000 0000 0000 0000 1000
    PROP            = 16,         // 0000 0000 0000 0000 0000 0000 0001 0000
    SHOCK           = 32,         // 0000 0000 0000 0000 0000 0000 0010 0000
    OMEN            = 64,         // 0000 0000 0000 0000 0000 0000 0100 0000
    GUNSTINGER      = 128,        // 0000 0000 0000 0000 0000 0000 1000 0000
    EPIDEMIC        = 256,        // 0000 0000 0000 0000 0000 0001 0000 0000
    DT_PIN_UP_P2    = 512,        // 0000 0000 0000 0000 0000 0010 0000 0000
    SHOWDOWN        = 1024,       // 0000 0000 0000 0000 0000 0100 0000 0000
    DRAW            = 2048,       // 0000 0000 0000 0000 0000 1000 0000 0000
    DTGROUND        = 4096,       // 0000 0000 0000 0000 0001 0000 0000 0000
    ROLL            = 8192,       // 0000 0000 0000 0000 0010 0000 0000 0000
    STINGER         = 16384,      // 0000 0000 0000 0000 0100 0000 0000 0000
    REAL_IMPACT     = 32768,      // 0000 0000 0000 0000 1000 0000 0000 0000
    // ?            = 65536,      // 0000 0000 0000 0001 0000 0000 0000 0000
    // ?            = 131072,     // 0000 0000 0000 0010 0000 0000 0000 0000
    // ?            = 262144,     // 0000 0000 0000 0100 0000 0000 0000 0000
    // ?            = 524288,     // 0000 0000 0000 1000 0000 0000 0000 0000
    // ?            = 1048576,    // 0000 0000 0001 0000 0000 0000 0000 0000
    // ?            = 2097152,    // 0000 0000 0010 0000 0000 0000 0000 0000
    // ?            = 4194304,    // 0000 0000 0100 0000 0000 0000 0000 0000
    // ?            = 8388608,    // 0000 0000 1000 0000 0000 0000 0000 0000
    // ?            = 16777216,   // 0000 0001 0000 0000 0000 0000 0000 0000
    // ?            = 33554432,   // 0000 0010 0000 0000 0000 0000 0000 0000
    // ?            = 67108864,   // 0000 0100 0000 0000 0000 0000 0000 0000
    // ?            = 134217728,  // 0000 1000 0000 0000 0000 0000 0000 0000
    // ?            = 268435456,  // 0001 0000 0000 0000 0000 0000 0000 0000
    // ?            = 536870912,  // 0010 0000 0000 0000 0000 0000 0000 0000
    // ?            = 1073741824, // 0100 0000 0000 0000 0000 0000 0000 0000
    // ?            = 2147483648, // 1000 0000 0000 0000 0000 0000 0000 0000
};

class SelectiveCancels : public Mod {
public:
    SelectiveCancels() = default;

    static bool mod_enabled;
    static uint32_t cancels;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    std::string get_mod_name() override { return "SelectiveCancels"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

    inline void draw_checkbox_simple(const char* name, CancelMoves move);

private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};
