#pragma once

#include <cstdint>

// Shadows in PBR materials (Shadows step 3, option B; Vibed\Shadows\README.md). The game shades the stage shadow as a pass of
// its own, tXfMaterialShadowReceiver, which draws receiving meshes again and pulls the finished pixels toward a flat colour:
// on PBR materials (PbrMaterials) the highlights and scattered light stayed visible inside shadows. Here:
//
// - The receiver pass skips meshes drawn with our PBR shader: hooks at its two material gates, `test byte [mat+4], 0Bh`
//   (the draw-mode byte: modes 1 / 2 / 8 receive), in uModel's receiver path 0x9F3060 (at 0x9F33B6, material in esi,
//   skip 0x9F3A6D, go on 0x9F33C4) and uDevil4Model's 0x5283E0 (at 0x5287C2, material in ecx, skip 0x528E8B, go on
//   0x5287D0). A material is ours when it is a cMaterialStandard (vtable 0xC08934) whose permutation key (+0x10) our
//   shader draws (pbr_materials::draws_key).
// - The PBR shader reads the shadow itself (material_pbr.inc "the stage shadow"): the cascade maps on s13-s15 (the material
//   shaders use s0-s7) and the receiver's inputs in c202-c219, from shadow_receiver's capture (ShadowReceiver.hpp: Frame),
//   set by a pixel shader bind listener (ps_bind) when one of the PBR shaders is bound. The maps and their sampler states
//   are put back when another shader is bound, so the game's own texture cache never sees them.
//
// Draw order: every sub-scene, the shadow casts included, sorts before the main scene's draws (the main scene is sub-scene
// 31, the counter 0xE5577C counts down from it each frame), so the maps are drawn when the PBR materials read them.
// Only while the stage shadow runs as 3 cascades (Shadow Quality HIGH) and the PBR shader has the shadow code; otherwise the
// receiver pass draws the PBR meshes as before.
namespace pbr_shadow {

struct Settings {
    bool on = true;
};
extern Settings settings;

struct Status {
    bool active = false;        // PBR materials shade the shadow; the receiver pass skips them
    bool data = false;          // the cascade receiver's inputs are being captured
    bool shader = false;        // the running PBR shader has the shadow code
    bool pbr = false;           // PbrMaterials is on
    int  skipped = 0;           // receiver draws skipped last frame (PBR meshes in range of the shadow)
};
Status status();

void init();                    // registers the bind listener
void update(bool mod_enabled);  // render thread, every frame
void on_reset();                // before a device reset: unbind and drop the maps

// the receiver pass's material gates (8 bytes each: the test and the mov after it); Shadows installs them
constexpr uintptr_t kModelGate = 0x9F33B6;
constexpr uintptr_t kDevilGate = 0x5287C2;
extern uintptr_t jmp_ret_model_gate;
extern uintptr_t jmp_ret_devil_gate;
void detour_model_gate();
void detour_devil_gate();

} // namespace pbr_shadow
