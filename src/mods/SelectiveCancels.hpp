#pragma once

#include "../mod.hpp"

// using powers of two here to avoid making alot of bool variables
enum CancelMoves {       // bits table
    ECSTASY         = 1,  // 0000 0001
    ARGUMENT        = 2,  // 0000 0010
    KICK13          = 4,  // 0000 0100
    SLASH_DIMENSION = 8,  // 0000 1000
    PROP            = 16, // 0001 0000
    SHOCK           = 32, // 0010 0000
    OMEN            = 64, // 0100 0000
    GUNSTINGER      = 128,
    EPIDEMIC        = 256,
    DT_PIN_UP_P2    = 512,
    SHOWDOWN        = 1024,
    DRAW            = 2048
    //BEAST_UPPERCUT  = 2048
};

class SelectiveCancels : public Mod {
public:
    SelectiveCancels() = default;

    static bool selective_cancels_enable;         // = false;
    static uint32_t cancels;                    // = 0;
    static uintptr_t selective_cancels_continue; //= 0x0080332F;

    std::string get_mod_name() override { return "SelectiveCancels"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

    inline void draw_checkbox_simple(const char* name, CancelMoves move);

private:
    std::unique_ptr<FunctionHook> detour;
};
