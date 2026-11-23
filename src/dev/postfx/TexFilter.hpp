#pragma once

namespace utility { class Config; }

// Better texture filtering (Graphics tab). The engine keeps one sampler record per shader parameter (sShader +0x5038,
// 16 bytes, indexed by the parameter's state handle; layout in Vibed\RE\texture_binding.md) that
// sRender::applySamplerState 0x8F4000 turns into SetSamplerState on every draw. The game's TextureFilter option reaches
// only the albedo (1 / 2) and normal map records (sShader::sampler_quality_override 0x8FDF10); the other material maps
// and the water stay trilinear without anisotropy, and effect textures (XfPrim*LinearSampler) are bilinear with POINT
// between mips (they pop as particles grow). With this on: the material maps and the water get the albedo record's
// anisotropy (the game's option still sets the level; 16 when the option isn't anisotropic), the effect samplers
// trilinear. Written every frame while on, the saved records put back when turned off. Samplers that read render
// targets, depth, shadow maps or tables (point on purpose) are never touched.
namespace tex_filter {

extern bool enabled;

void update();      // every frame (Present thread)
void draw_gui();
void load(const utility::Config& cfg);
void save(utility::Config& cfg);

} // namespace tex_filter
