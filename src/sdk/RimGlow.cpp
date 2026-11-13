#include "RimGlow.hpp"
#include "../mod.hpp"
#include "MtDTI.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>

namespace rim_glow {
namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

constexpr uintptr_t fn_buildRenderState  = 0x00A61D00;   // cMaterial::buildRenderState(this@ecx): mRenderState0 / 1 from mFlags
constexpr uintptr_t kMaterialStandardDTI = 0x00EAE1E4;   // cMaterialStandard::DTI
constexpr uintptr_t kShader              = 0x00E55700;   // sShader::mpInstance
constexpr uintptr_t kSysTextures         = 0x6868;       // sShader::mpSysTexture[6]: 0 NullWhite, 1 NullBlack, 2 NullNormal_NM, 3 DefaultCube_CM

FakeTexture g_black, g_white;
std::atomic<bool> g_base_ready{ false };

uint32_t argb(float r, float g, float b) {
    auto c = [](float v) { return (uint32_t)(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return 0xFF000000u | c(r) << 16 | c(g) << 8 | c(b);
}

void wrap(FakeTexture& f, uintptr_t source, IDirect3DBaseTexture9* d3d) {
    std::memcpy(f.res, (void*)source, kResSize);
    std::memcpy(f.tex, (void*)at<uintptr_t>(source, 0xCC), kTexSize);
    f.d3d = d3d;
    at<uint32_t>(f.ptr(), 0x44) = 0x40000000;               // mRefCount
    at<uintptr_t>(f.ptr(), 0xCC) = (uintptr_t)f.tex;
    const uintptr_t t = (uintptr_t)f.tex;
    at<IDirect3DBaseTexture9*>(t, 0x10) = d3d;
    at<uintptr_t>(t, 0x14) = 0;
    at<uint16_t>(t, 0x1C) = 1;
    at<uint16_t>(t, 0x1E) = 1;
    at<uint32_t>(t, 0x24) = D3DFMT_A8R8G8B8;
    for (int i = 0; i < 4; ++i) at<float>(t, 0x30 + 4 * i) = 1.0f;
}

bool fill_2d(IDirect3DTexture9* t, uint32_t color) {
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, nullptr, 0))) return false;
    *(uint32_t*)lr.pBits = color;
    t->UnlockRect(0);
    return true;
}

bool fill_cube(IDirect3DCubeTexture9* t, uint32_t color) {
    for (int face = 0; face < 6; ++face) {
        D3DLOCKED_RECT lr;
        if (FAILED(t->LockRect((D3DCUBEMAP_FACES)face, 0, &lr, nullptr, 0))) return false;
        *(uint32_t*)lr.pBits = color;
        t->UnlockRect((D3DCUBEMAP_FACES)face, 0);
    }
    return true;
}

// sShader's system textures, once they exist: NullWhite and DefaultCube_CM (what the fakes copy).
bool system_textures(uintptr_t& white, uintptr_t& cube) {
    const uintptr_t shader = at<uintptr_t>(kShader, 0);
    if (!shader) return false;
    white = at<uintptr_t>(shader, kSysTextures);
    cube = at<uintptr_t>(shader, kSysTextures + 3 * 4);
    return white && cube && at<uintptr_t>(white, 0xCC) && at<uintptr_t>(cube, 0xCC);
}

} // namespace

bool update_base() {
    if (g_base_ready) return true;
    uintptr_t white = 0, cube = 0;
    if (!system_textures(white, cube)) return false;
    IDirect3DDevice9* dev = g_framework->get_d3d9_device();
    if (!dev) return false;
    IDirect3DTexture9 *black_t = nullptr, *white_t = nullptr;
    if (FAILED(dev->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &black_t, nullptr)) ||
        FAILED(dev->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &white_t, nullptr)) ||
        !fill_2d(black_t, 0xFF000000u) || !fill_2d(white_t, 0xFFFFFFFFu)) {
        if (black_t) black_t->Release();
        if (white_t) white_t->Release();
        spdlog::error("RimGlow: couldn't make the glow's textures");
        return false;
    }
    wrap(g_black, white, black_t);
    wrap(g_white, white, white_t);
    g_base_ready = true;
    return true;
}

bool base_ready() { return g_base_ready; }
uintptr_t black() { return g_black.ptr(); }
uintptr_t white() { return g_white.ptr(); }

bool Cube::update(const float rgb[3]) {
    if (!made) {
        uintptr_t white = 0, cube = 0;
        if (!system_textures(white, cube)) return false;
        IDirect3DDevice9* dev = g_framework->get_d3d9_device();
        if (!dev) return false;
        IDirect3DCubeTexture9* t = nullptr;
        if (FAILED(dev->CreateCubeTexture(1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr))) {
            spdlog::error("RimGlow: couldn't make a glow cube");
            return false;
        }
        wrap(f, cube, t);
        shown[0] = -1.0f;
    }
    if (shown[0] != rgb[0] || shown[1] != rgb[1] || shown[2] != rgb[2]) {
        if (fill_cube((IDirect3DCubeTexture9*)f.d3d, argb(rgb[0], rgb[1], rgb[2])))
            std::memcpy(shown, rgb, sizeof shown);
    }
    made = true;
    return true;
}

void build_render_state(uintptr_t mat) { ((void(__fastcall*)(uintptr_t, int))fn_buildRenderState)(mat, 0); }

bool is_standard(uintptr_t mat) {
    const auto* dti = ((MtDTI*(__fastcall*)(uintptr_t, int))(at<uintptr_t>(at<uintptr_t>(mat, 0), 0x10)))(mat, 0);
    for (; dti; dti = dti->mp_parent)
        if ((uintptr_t)dti == kMaterialStandardDTI) return true;
    return false;
}

// The glow is the shader's specular term, which the permutations without specular (NONE 0, MIRROR 2, POWMAP 3: models
// made without a specular map) don't have. Each has a STANDARD twin in XfMaterialStandard.bfx, and the material's key
// (cMaterial +0x10 = skin + 7 lighting + 35 normal map + 140 specular + 700 lightmap + 5600 multitexture) is decoded again
// at every draw (cMaterialStandard::beginDraw 0xA6387B), so the material takes the twin. Skinned materials only: some
// static twins read one UV set more than the mesh may have.
uint32_t glow_key(uint32_t key) {
    const uint32_t specular = key / 140 % 5, skin = key % 7;
    if (specular == 1 || specular == 4 || skin == 0) return key;
    return key - 140 * specular + 140;
}

Saved save(uintptr_t mat) {
    Saved v;
    v.mat = mat;
    v.flags = at<uint32_t>(mat, mFlags);
    v.state_key = at<uint32_t>(mat, mStateKey);
    v.base = at<uintptr_t>(mat, mpBaseTex);
    v.mask = at<uintptr_t>(mat, mpMaskTex);
    v.env = at<uintptr_t>(mat, mpEnvTex);
    v.alpha = at<float>(mat, mAlpha);
    v.diffuse_w = at<float>(mat, mDiffuseW);
    v.color2_w = at<float>(mat, mColor2W);
    std::memcpy(v.fresnel, &at<float>(mat, mFresnel), sizeof v.fresnel);
    return v;
}

void restore(const Saved& v) {
    at<uint32_t>(v.mat, mFlags) = v.flags;
    at<uint32_t>(v.mat, mStateKey) = v.state_key;
    at<uintptr_t>(v.mat, mpBaseTex) = v.base;
    at<uintptr_t>(v.mat, mpMaskTex) = v.mask;
    at<uintptr_t>(v.mat, mpEnvTex) = v.env;
    at<float>(v.mat, mColor2W) = v.color2_w;
    std::memcpy(&at<float>(v.mat, mFresnel), v.fresnel, sizeof v.fresnel);
    at<float>(v.mat, mAlpha) = v.alpha;
    at<float>(v.mat, mDiffuseW) = v.diffuse_w;
    build_render_state(v.mat);
}

void put_look(const Saved& v, bool glow, Cube* cube, bool keep_mask, bool through_walls) {
    const uintptr_t mat = v.mat;
    uint32_t f = v.flags & ~(ZWRITE | ZTHROUGH);
    if (through_walls) f |= ZTHROUGH;
    if (glow && cube) {
        f = (f & ~(MODE_MASK | ALPHA_REF_MASK | BLEND_PICK_MASK)) | MODE_TRANSPARENT | BLEND_ADDITIVE;
        at<uint32_t>(mat, mStateKey) = glow_key(v.state_key);
        at<uintptr_t>(mat, mpBaseTex) = black();
        if (!keep_mask) at<uintptr_t>(mat, mpMaskTex) = white();
        at<uintptr_t>(mat, mpEnvTex) = cube->ptr();
        at<float>(mat, mColor2W) = 1.0f;
    }
    at<uint32_t>(mat, mFlags) = f;
    build_render_state(mat);
}

void set_strength(uintptr_t mat, bool glow, float strength, float brightness, float reach) {
    const float a = std::clamp(strength, 0.0f, 1.0f);
    at<float>(mat, mAlpha) = glow ? 1.0f : a;
    at<float>(mat, mDiffuseW) = glow ? a : 1.0f;
    if (!glow) return;
    // the shader's term is (1 + cos)^5 (1 at the outline, 32 face-on): weight 1 at the outline down to 0 where cos
    // reaches `reach`
    const float x = 1.0f + std::clamp(reach, 0.02f, 1.0f), factor = 1.0f / (x * x * x * x * x - 1.0f);
    float* fr = &at<float>(mat, mFresnel);
    fr[0] = factor;
    fr[1] = -factor;
    fr[2] = 200.0f;                       // the lights' highlights shrink to points
    fr[3] = brightness;
}

} // namespace rim_glow
