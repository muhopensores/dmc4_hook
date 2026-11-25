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
    float water_tiling = 8.0f;       // wave normal map repeats per unit of the water's UVs (the texture transform's scale)
    bool hide_refraction = true;     // screen-refraction effects (heat haze, sword-trail distortion): not traceable
    // water layouts hidden from Remix instead of the stand-in (appended last: remix_live.py reads the fields above by offset)
    bool hide_water_depth = false;   // XfWater (normal + cube + depth buffer): the sea, puddles faded by depth
    bool hide_water_simple = false;  // water binding only its normal map
    bool hide_water_flat = false;    // XfFlatWater (the BP floor)
    // 1 = probe the next frame (the GUI's button, or a debugger): the main view's picture at each pass, around the injection
    // and after the present, plus the trigger's device state, to remix_probe.txt; back to 0 when it starts. Not saved
    int probe = 0;
    // the traced view's main scene draws every model, also those outside the camera's view (uModel::frustumCullAndLOD
    // answers "inside"): Remix only knows what's drawn, so shadow casters, bounce surfaces and walls behind the camera
    // came and went as it turned
    bool draw_all = false;
    // main-scene draws made with another camera than the one Remix gets (Fortuna's stage: a second view block in SOLID;
    // the shadow overlay's projector in OVERLAP): Remix rebuilds every vertex from its screen position with the main camera,
    // so they landed in the wrong place (floating stairs, shards, a refracting sheet). 0 = traced as before, 1 = hidden
    // from Remix, 2 = placed with their own projection (the main camera's view, P = V^-1 . VP: the block is the same
    // camera with another projection, used for objects that change from frame to frame)
    int other_cameras = 1;
    // debugging: draws using a texture whose path contains this text (any case; several separated by ';') are hidden from
    // Remix, to tell which object is which. Not saved
    char hide_textures[64] = "";
    // D3D WORLD = each draw's position (gXfWorld's translation). Remix matches a draw to last frame's object by its mesh
    // and the centre of its bounding box (input vertices times WORLD); with WORLD identity every copy of a prop had the
    // same centre (Fortuna's 9 trash cans), so each took another copy's history as the draw order shuffled: the copies
    // flickered between lit and dark and smeared. Only the translation: Remix turns captured normals by WORLD, and the
    // bridge's normal output is world space already
    bool world_positions = true;
    // water draws using a texture whose path contains this text (any case; several separated by ';') are hidden from
    // Remix: puddles share the sea's shader, so the stand-in gave them the sea's look
    char hide_water_textures[64] = "";
    // interactive water (uInteractiveWater: the practice stage, the BP floor): its ripples are simulated on the GPU in the
    // sub-scenes IWaterInit / IWaterUpdate (which "skip other views" skipped) and drawn only as a normal map on a flat grid.
    // On: the simulation runs, and the surface's vertex shader is replaced (at creation: restart) by one that lifts the
    // grid by the height map and outputs the normal map's normal, so Remix traces real ripples
    bool water_ripples = true;
    float ripple_height = 10.0f;   // cm the surface rises at the simulation's full amplitude, per unit of the water's Y scale
    // water meshes whose vertex shaders move their vertices (XfWater's waves, the interactive surface) drawn subdivided:
    // each level splits every triangle in four (a cached copy per mesh), so the waves get as many vertices. 0 = the game's
    int water_detail = 1;
    // skinned draws (bones in gXfJointWorld, no gXfWorld: characters) get WORLD = the translation of their first bone,
    // which moves with the character (within a metre or so of its position), instead of the origin for every body part.
    // Testing whether Remix then matches a body part to its own last-frame object more reliably (a character drawn
    // standing at the start spot for a second after a room change while the game had it running). Needs world_positions
    bool skinned_positions = true;
    // the stage's point and spot lights as sphere lights through the Remix API (needs exposeRemixApi = True in
    // .trex\bridge.conf). Remix makes its D3D9 point lights only as bright as their range (the colour is divided by
    // its largest component) and stops taking a still light's changes after 50 frames; through the API the radiance
    // is ours: the game's colour x point_strength reached at point_distance. Without the API: D3D9 lights as before
    bool api_lights = true;
    float point_radius = 5.0f;       // cm, the sphere lights' size
    float point_distance = 200.0f;   // cm: where an API light gives the game's colour (x point_strength)
    // draws using a texture whose path contains this text (any case, ';'-separated) get a sphere light at their middle
    // (Remix's own "Add Light to Texture" reads the draw's vertices on the CPU, and under vertex capture they are only
    // on the GPU: it makes no light for DMC4). Main-scene draws before the hand-over only: not effects drawn after it
    char light_textures[128] = "";
    float tex_light_color[3] = { 1.0f, 0.72f, 0.42f };
    float tex_light_strength = 1.0f;  // the light's colour at point_distance
    float tex_light_radius = 5.0f;    // cm
    // Remix options set live through the Remix API's SetConfigVariable (Remix's user layer, as its own menu does):
    // "key = value" items separated by ';' or new lines, applied when the text changes. For tests (performance A / B
    // without restarts, Vibed\Remix\remix_live.py); options Remix reads only at start still need rtx.conf
    char rtx_options[256] = "";
    // XfWater draws (the sea's shader) narrower than puddle_size get their own stand-in (kind 5), so their own Remix
    // material (DMC4Water: a thin clear film of water): Fortuna's puddles use the sea's shader and got the sea's look
    bool puddles = true;
    float puddle_size = 10000.0f;    // cm, the draw's widest extent across the ground in the world: st012's puddles
                                     // are one 59 m draw, st400's sea a 2 km plane, st001's water 105 m (not seen)
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
// dispatchCommands' third vertex-constant loop (0x8F53E5, the same 10 bytes as 0x8F5380): uploads through a descriptor
// {count | 0x80000000, data}, which is how skinned draws get their bones (gXfJointWorld, sScene's envelopes)
constexpr uintptr_t kVsDescs = 0x8F53E5;
extern uintptr_t jmp_ret_vsdesc;
void* vsdesc_detour();
std::unique_ptr<FunctionHook>& vsdesc_hook();
std::unique_ptr<FunctionHook>& shader_hook();
// uModel::frustumCullAndLOD (0x9EDBB0, 6 bytes: push ebp / mov ebp, esp / and esp, -16): settings.draw_all
// dispatchCommands' indexed draw (0x8F574E, 7 bytes: the command in eax, the device in esi): settings.water_detail
constexpr uintptr_t kDrawIndexed = 0x8F574E;
extern uintptr_t jmp_ret_draw;
void* draw_detour();
std::unique_ptr<FunctionHook>& draw_hook();
constexpr uintptr_t kCullAndLod = 0x9EDBB0;
extern uintptr_t jmp_ret_cull;
void* cull_detour();
std::unique_ptr<FunctionHook>& cull_hook();
void set_sun(const float* dir);
void on_reset();   // releases the default-pool texture before a device reset   // the way the sun's light travels, game axes (PostFX's g_sun), each frame
void load(const utility::Config& cfg);
void save(utility::Config& cfg);

} // namespace remix
