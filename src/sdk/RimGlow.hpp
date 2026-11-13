#pragma once
#include <cstdint>
#include <cstddef>

struct IDirect3DBaseTexture9;

// RimGlow: a model's standard materials turned into an edge glow (AfterImage's copies, EnemyAI's Provoke aura).
//
// DX9 code and XfMaterialStandard.bfx's shaders:
//   - Blending: cMaterial::buildRenderState 0xA61D00 gives a material of mode 4 / 8 (attr 0x20 / 0x40) the additive
//     state 0x2A20225 when mFlags bits 24-26 = 1; cMaterial::beginDraw 0xA61A70 uses it only while mAlpha (+0x20) >= 1
//     and otherwise forces alpha blending (0x2A20625). So mAlpha stays 1 and the strength goes through the blend factor:
//     gXfBlendFactor = mDiffuseColor.w (+0x3C) * mAlpha, which the pixel shader multiplies into the output alpha.
//   - The edge: the standard pixel shader ends with spec = (envCube * EnvMapPower + light specular) * MaskMap.rgb *
//     (1 - sat(FresnelFactor * (1 + cos)^5 + FresnelBias)) (gXfFresnelFactor = (FresnelFactor, FresnelBias, SpecularPow,
//     EnvMapPower * mColor2.w), cMaterialStandard +0x90..+0x9C), cos = the surface's facing toward the camera ("1 - N.V"
//     is 1 at the silhouette and 2 face-on). FresnelFactor f = 1 / ((1 + reach)^5 - 1) and FresnelBias -f give weight 1
//     at the silhouette, 0 from cos = reach inward.
//   - Materials without the specular term (NONE / MIRROR / POWMAP) take their SPECULAR_STANDARD twin through the key
//     (+0x10, decoded at every draw; skinned materials only).
//   - Clear inside: the albedo is a black texture, so diffuse and ambient add nothing; additive blending makes black
//     invisible. The colour is the environment cube's: a 1x1 cube of the look's colour. The mask is white unless kept.
//   - The textures are Direct3D textures made on the render thread behind copies of the game's system textures'
//     rTexture / cTrans::Texture (sShader +0x6868: NullWhite, DefaultCube_CM), put into the materials (+0x1C base, +0xD4
//     mask, +0xE8 env) and taken out again (restore) before the materials are destroyed.
namespace rim_glow {

// cMaterial / cMaterialStandard
constexpr uintptr_t mFlags    = 0x04;   // byte0 draw mode, byte1 alpha ref, byte2 material id, bits 24-26 blend pick, 27 zwrite, 28 zthrough
constexpr uintptr_t mStateKey = 0x10;   // the shader permutation key
constexpr uintptr_t mpBaseTex = 0x1C;   // rTexture* (AlbedoMap)
constexpr uintptr_t mAlpha    = 0x20;   // < 1 forces alpha blending (cMaterial::beginDraw)
constexpr uintptr_t mDiffuseW = 0x3C;   // mDiffuseColor.w: times mAlpha = gXfBlendFactor (the output alpha)
constexpr uintptr_t mColor2W  = 0x4C;   // times EnvMapPower in gXfFresnelFactor.w
constexpr uintptr_t mFresnel  = 0x90;   // FresnelFactor, FresnelBias, SpecularPow, EnvMapPower
constexpr uintptr_t mpMaskTex = 0xD4;   // MaskMap
constexpr uintptr_t mpEnvTex  = 0xE8;   // EnvironmentMap (cube)
constexpr uint32_t  MODE_MASK = 0xFF, ALPHA_REF_MASK = 0xFF00, BLEND_PICK_MASK = 0x07000000;
constexpr uint32_t  MODE_TRANSPARENT = 8, BLEND_ADDITIVE = 0x01000000, ZWRITE = 0x08000000, ZTHROUGH = 0x10000000;

// rTexture (0xE8): +0x44 mRefCount, +0xCC mpTexture (cTrans::Texture, 0x60: +0x10 the Direct3D texture, +0x14 its SLI
// copy, +0x1C / +0x1E width / height, +0x24 format, +0x30 mRange).
constexpr size_t kResSize = 0xE8, kTexSize = 0x60;

// A copy of a system texture's rTexture and cTrans::Texture around a Direct3D texture of ours: the materials and
// beginDraw read it like any other, and the huge reference count keeps a stray release from ever freeing it.
struct alignas(16) FakeTexture {
    alignas(16) uint8_t res[kResSize];
    alignas(16) uint8_t tex[kTexSize];
    IDirect3DBaseTexture9* d3d = nullptr;
    uintptr_t ptr() { return (uintptr_t)res; }
};

// Render thread: the shared black albedo and white mask, once the game's system textures exist. True when they do.
bool update_base();
bool base_ready();
uintptr_t black();
uintptr_t white();

// A 1x1 cube of one colour (each look keeps its own). update() on the render thread, after update_base().
struct Cube {
    FakeTexture f;
    float shown[3] = { -1.0f, -1.0f, -1.0f };
    bool made = false;
    bool update(const float rgb[3]);
    bool ready() const { return made; }
    uintptr_t ptr() { return f.ptr(); }
};

void build_render_state(uintptr_t mat);
bool is_standard(uintptr_t mat);
uint32_t glow_key(uint32_t key);

// A material as it was before a look was put on it.
struct Saved { uintptr_t mat; uint32_t flags, state_key; uintptr_t base, mask, env; float alpha, diffuse_w, color2_w; float fresnel[4]; };

Saved save(uintptr_t mat);
void restore(const Saved& v);
// The look on a saved material: glow (additive, black albedo, `cube`, white mask unless keep_mask) or plain (alpha
// blended); no depth write, z-through when asked.
void put_look(const Saved& v, bool glow, Cube* cube, bool keep_mask, bool through_walls);
// The look's live values: strength 0-1 (the plain look's alpha, the glow's blend factor), the glow's brightness and
// reach (the facing cosine where the edge ends).
void set_strength(uintptr_t mat, bool glow, float strength, float brightness, float reach);

} // namespace rim_glow
