#pragma once

#include <cstdint>
#include <string>

struct IDirect3DTexture9;

// Soft shadows (Shadows step 2, Vibed\Shadows\README.md): our pixel shader in place of tXfMaterialShadowReceiver's cascade
// ones. Contact-hardening filter: a blocker search, a penumbra that grows with the gap between occluder and receiver (in
// cm, so it doesn't jump at a cascade border), a rotated disc of bilinear depth compares. Source: shadow_receiver.inc,
// or nativePC\shader\shadows\shadow_receiver.hlsl when that file exists.
//
// The receiver technique: sShader::mpInstance +0x4838 = mpTechnique[], its slot at 0xE5561C (sShader::InitTechniques
// 0x8FFDB1); technique +0x10 = info ([1] 6 components: skin 7, albedo alpha 2, caster 2, mask 3, PCF 4 (2x2, 3x3, 4x4,
// RANDOM_32: an empty stub on PC, no shadow), blocker 2; [2] 672 permutations, [3] 211 pixel shaders, [8] the pixel
// shader records). Each permutation's pixel shader starts a group of 4, one per shadow mode (uShadow +0xE8): 3 cascades,
// one light-space map, uniform area, spot. Ours replaces the cascade ones: those whose constant table lists the Middle and
// Far maps (48: every PCF level, with or without each feature). Registers and features come from that table, as in
// PbrMaterials.
//
// The cascade matrices: sub_A73E90 (the cascade draw) hands its three 4x4s (rows = world x, y, z, 1; columns = u, v,
// depth, w) to the receiver setup 0xA775E0 as its 6th stack argument; a hook there (call from 0xA74E5C only) keeps them,
// and the bind listener (ps_bind, sRender's thread) writes them with the tuning values to c210-c223 when one of ours is
// bound. They feed only the scale (cm -> texels, depth -> cm), not the lookup, so a frame's lag doesn't show.
//
// The same hook keeps what the PBR materials need to shade the shadow themselves (Shadows step 3, PbrShadow.hpp): the map
// range (its 3rd argument, which it stores as XfShadowMapRange), the far fade (XfShadowMapRangeGrad, set by the cascade
// draw in the scene's parameter table before the call: [cTrans +0x2A94][handle at unit +0x270]), the unit's colour
// (+0x60 mShadowColor = XfShadowColorCaster), light direction (+0x40), border gradation (1 / +0xF8) and its three maps
// (+0x18 / +0x1C / +0x20: cTrans::Texture, the IDirect3DTexture9 at +0x10, the size at +0x1C), AddRef'd while held.
namespace shadow_receiver {

struct Settings {
    bool  soft = true;          // ours instead of the stock 16-tap filter
    int   quality = 1;          // filter taps: 0 16, 1 24, 2 32
    float light_size = 0.03f;   // penumbra width per cm of gap (the light's apparent size)
    float max_soft = 40.0f;     // the widest penumbra, cm
    float min_radius = 1.0f;    // the narrowest filter, texels
    int   debug = 0;            // 0 off, 1 cascades, 2 penumbra radius
};
extern Settings settings;

struct Status {
    bool        found = false;      // the technique's records exist
    std::string waiting;            // while not found: which check failed
    int         targets = 0;        // cascade receiver pixel shaders
    int         replaced = 0;       // running ours
    int         failed = 0;
    bool        compiling = false;
    bool        from_file = false;
    unsigned    max_slots = 0;
    std::string error;
    bool        matrices = false;   // the cascade matrices are being captured
};

Status status();

// The cascade receiver's inputs as the game set them up (captured with the matrices)
struct Frame {
    float    mats[48] = {};             // three 4 x 4: rows world x, y, z, 1; columns u, v, depth, w
    float    range[4] = {};             // XfShadowMapRange: where the near, middle and far cascades end (cm from the eye)
    float    grad[4] = {};              // XfShadowMapRangeGrad: the far cascade's fade, 1 - saturate(depth x .x + .y)
    float    colour[4] = {};            // mShadowColor (XfShadowColorCaster): rgb, w = darkness
    float    light_dir[4] = {};         // mInfiniteLightDir: the way the light travels
    float    border = 0.0f;             // XfShadowBorderGradation = 1 / mBorderGradation (0 = none)
    uint32_t size = 0;                  // the maps' size
    IDirect3DTexture9* maps[3] = {};    // near, middle, far
};
// render thread: the last capture if one came within the last 30 frames; out.maps are AddRef'd for the caller
bool take_frame(Frame& out);
// render thread, before a device reset (the maps are default-pool): drop the held maps until the next capture
void release_frame();

void request_compile();             // any thread
void update(bool mod_enabled);      // render thread, every frame
void init();                        // registers the bind listener
// the matrix capture hook: Shadows installs detour_capture at kCaptureSite (6 bytes) with jmp_ret_capture
constexpr uintptr_t kCaptureSite = 0xA775E0;
extern uintptr_t jmp_ret_capture;
void detour_capture();

} // namespace shadow_receiver
