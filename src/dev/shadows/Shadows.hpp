#pragma once

#include "../../mod.hpp"
#include <cstdint>

// Shadows: the stage shadow's map size and live overrides of its settings. Plan and RE findings: Vibed\Shadows\README.md.
//
// How the game does it (DX9): a stage's .sdl places one uShadow unit (vtbl 0xC09880). Its move (vtbl slot 6, 0xA72B70)
// runs every frame and calls its own setters again with the stored values: mShadowType -> mode +0xE8 (0xA7D1A0; with the
// game's ShadowQuality HIGH a single light-space map becomes 3 cascades), mPCF -> +0xFC (0xA7D1F0), mMapSize -> +0x224
// (0xA7D390: HIGH raises it to at least 1024; a new size frees and creates the maps, R32F squares, 3 for cascades). Its
// draw (slot 9, 0xA72D00) renders the maps and sets up the receiver pass: tXfMaterialShadowReceiver draws the receivers
// again and blends XfShadowColor over the lit frame by shadow x mShadowColor.w (4 / 9 / 16 depth taps by mPCF; HIGH
// always 16).
//
// Here: a hook in the size setter (0xA7D3C9) gives it our size, so the move's next call recreates the maps; a hook on the
// move's entry (0xA72B70) writes the overridden fields before the move and the draw read them, and puts the stage's own
// values back when an override is turned off (a value the move finds changed since our last write came from the .sdl).
//
// Depth buffer: every offscreen pass (the shadow casts too) draws with sRender's shared D24S8 depth surface (+0x1B1E8,
// made by 0x8F7230 at max(screen, 2048) per side) unless the off-screen MSAA one fits. A 4096 map is bigger than it, D3D9
// refuses the draws, the map keeps its clear value and the whole scene reads as shadowed (2026-11-14, first build). So a
// hook where sRender::dispatchCommands picks that surface (0x8F5A8A) hands such targets a depth surface of our own, made
// at the map size and released before a device reset.
namespace shadows {

enum Field { SPLIT_NEAR, SPLIT_MIDDLE, VIEW_DISTANCE, DEPTH_BIAS, SLOPE_BIAS, DARKNESS, FIELD_COUNT };

struct Override {
    bool  on = false;
    float value = 0.0f;
};

extern bool     mod_enabled;
extern int      map_size;                  // 0 = the game's (1024 with ShadowQuality HIGH), else 2048 / 4096
extern Override overrides[FIELD_COUNT];

struct UnitInfo {
    uintptr_t unit = 0;
    uint32_t  mode = 0;                    // +0xE8: 0 cascades, 1 one light-space map, 2 uniform area, 3 spot
    uint32_t  pcf = 0;                     // +0xFC: 0 / 1 / 2 = 4 / 9 / 16 depth taps (HIGH: 16 whatever this says)
    uint32_t  size = 0;                    // +0x224
    float     stage[FIELD_COUNT] = {};     // the .sdl's values (what the overrides replace)
};

struct Status {
    int      quality = -1;                 // the game's ShadowQuality: 0 none, 1 default, 2 high (-1 = no sShader yet)
    int      units = 0;
    UnitInfo unit[4];
};

Status status();

} // namespace shadows

class Shadows : public Mod {
public:
    Shadows() = default;
    std::string get_mod_name() override { return "Shadows"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_reset() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    std::unique_ptr<FunctionHook> hook_move;
    std::unique_ptr<FunctionHook> hook_map_size;
    std::unique_ptr<FunctionHook> hook_depth;
};
