#pragma once

#include <cstdint>

namespace utility { class Config; }
class FunctionHook;
#include <memory>

// RTX Remix bridge (Vibed\Remix\README.md). Remix replaces d3d9.dll and path-traces what the game draws, but it rebuilds
// the scene from the fixed-function transforms (SetTransform), which DMC4 never sets: its vertex shaders get only
// gXfViewProj (c0-c3), gXfWorld (static meshes, c4-c6) or the joint palette (skinned, c23+). With vertex capture on
// (rtx.useVertexCapture), Remix reads each vertex's clip position and needs only the view and projection to put it back
// in the world. This gives Remix exactly those, from sCamera's main viewport, at the start of each dispatch and when the
// main scene starts, and can drop the draws Remix shouldn't see (they'd be duplicates or garbage): other views and
// sub-scenes (shadow maps, environment cubes, reflections), the main scene's depth prepass, and the filter pass.
// Render thread, from PostFX's tag hook (0x8F4460).
namespace remix {

struct Settings {
    bool on = false;
    bool skip_other_scenes = true;   // tags whose view / sub-scene isn't the main scene's
    bool skip_zpass = true;          // the main scene's depth prepass (pass 1 ZPASS)
    bool hide_occlusion = true;      // occlusion-query quads (pass 4 OCCLUSION: light halos' visibility tests)
    bool hide_shadow_overlays = true; // the stage drawn again with only its shadow maps (pass 5 OVERLAP)
    bool hide_glows = true;          // untextured effect sprites (only the depth buffer bound): glows shaped in the pixel shader
    bool skip_filters = true;        // screen filters (pass 9 FILTER)
    bool skip_hud = false;           // HUD, menus, 2D (passes 10 SCREEN / 11 END): the title and menus draw there too
    // where Remix's traced image goes into the game's frame (0 = at present, as Remix does by itself). From that pass on the
    // game draws with its own shaders over it: effects (refraction samples the traced scene), filters, tone map, HUD.
    // Remix writes into the target bound then (the HDR scene buffer) and rasterises every later draw (d3d9_rtx.cpp:
    // RTX injection). 6 = before TRANSPARENT.
    int inject_pass = 6;
    bool relabel = true;             // effect / flat-water shaders' TEXCOORD labels swapped for Remix (at creation: restart)
    bool linearize = true;           // Remix's injected image back to linear HDR before the game's tone map
    float expand = 0.0f;             // how far to undo Remix's tone curve (0 = only the sRGB encoding; 1 blew out bright
                                     // views: the sky stretched up to 32x drove the game's eye adaptation and bloom)
    float exposure = 1.0f;           // scale of the image handed to the game
    bool sky = true;                 // sky draws marked as sky for Remix (viewport depth 1..1, rtx.skyMinZThreshold)
    bool albedo_to_stage0 = true;    // main-scene draws: the albedo map on stage 0, where Remix takes the colour from
    bool game_lights = true;         // the stage's light units (directional, point) as D3D9 lights, which Remix traces
    float dir_strength = 1.0f;       // directional lights' colour multiplier (1: the game's values, as the image goes back
                                     // through the game's tone map linear; 2 overexposed sunlit rooms)
    float point_strength = 1.0f;     // point lights' colour multiplier
    bool fill = true;                // ambient fill: the stage's hemisphere (ambient) colour as a wide light at the camera
    float fill_strength = 1.0f;
    float fill_range = 4000.0f;      // cm
    bool sun = true;                 // without game lights: the shadow light (uShadow) as one directional light
    float sun_strength = 2.0f;
    float sun_color[3] = { 1.0f, 0.95f, 0.88f };
    bool water_standin = true;       // water passes (no albedo map, water samplers): a solid colour on stage 0
    float water_color[3] = { 0.03f, 0.05f, 0.06f };
    float water_flow[2] = { 0.02f, 0.012f };   // water texture offset per second (u, v): Remix's water normals move with it
    float water_tiling = 4.0f;       // wave normal map repeats per unit of the water's UVs (the texture transform's scale)
    bool hide_refraction = true;     // screen-refraction effects (heat haze, sword-trail distortion): not traceable
    // water layouts hidden from Remix instead of the stand-in (appended last: remix_live.py reads the fields above by offset)
    bool hide_water_depth = false;   // XfWater (normal + cube + depth buffer): the sea, puddles faded by depth
    bool hide_water_simple = false;  // water binding only its normal map
    bool hide_water_flat = false;    // XfFlatWater (the BP floor)
};
extern Settings settings;

bool loaded();                                  // a Remix d3d9.dll is the game's d3d9 (the bridge's or the runtime's)
// the tag hook: before tag `index` runs; true = skip it
bool on_tag(void* device, const uint32_t* list, uint32_t index, uint32_t count);
void draw_gui();
// sRender::dispatchCommands' pixel-shader texture loop (0x8F545E, 10 bytes): the albedo onto stage 0 while the bridge
// is on and the main scene draws. Installed by PostFX.
constexpr uintptr_t kTexLoop = 0x8F545E;
extern uintptr_t jmp_ret_tex;
void* tex_detour();   // the detour's address
// dispatchCommands' lazy shader creation (0x8F49BC, 7 bytes): relabels the effect shaders' outputs before they're created
constexpr uintptr_t kShaderCreate = 0x8F49BC;
extern uintptr_t jmp_ret_shader;
void* shader_detour();
// dispatchCommands' vertex-shader float4-array uploads (0x8F5380, 10 bytes): the frame's own camera for Remix
constexpr uintptr_t kVsArrays = 0x8F5380;
extern uintptr_t jmp_ret_vsarr;
void* vsarr_detour();
std::unique_ptr<FunctionHook>& vsarr_hook();
std::unique_ptr<FunctionHook>& shader_hook();
void set_sun(const float* dir);
void on_reset();   // releases the default-pool texture before a device reset   // the way the sun's light travels, game axes (PostFX's g_sun), each frame
void load(const utility::Config& cfg);
void save(utility::Config& cfg);

} // namespace remix
