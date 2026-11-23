#include "Remix.hpp"
#include "../../mod.hpp"
#include "../../sdk/Cam.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/Devil4.hpp"
#include "../debug_server/Memory.hpp"
#include <d3d9.h>
#include <atomic>
#include <cstring>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <deque>
#include <cstdio>
#include <cmath>

namespace remix {

Settings settings;

namespace {

constexpr uintptr_t kCameraInstance = 0x00E552D4;  // sCamera::mpInstance
constexpr uint32_t  kMainScene = 0x1F;              // tag key bits 24-28
constexpr uint32_t  kMainView = 0;                  // tag key bits 29-31 of the main scene's tags (checked live: g_main_view)
// sRender's passes (tag key bits 20-23; names from off_E16B40, Vibed\RE\sRender_struct.md): 0 BEGIN, 1 ZPASS, 2 SOLID,
// 3 NUKI, 4 OCCLUSION, 5 OVERLAP, 6 TRANSPARENT, 7 EFFECT, 8 TRANSPARENT_NZ, 9 FILTER, 10 SCREEN, 11 END
constexpr uint32_t  kZPass = 1, kOcclusion = 4, kOverlap = 5, kFilter = 9, kScreen = 10, kEnd = 11;

std::atomic<uint32_t> g_skipped{ 0 }, g_seen{ 0 }, g_frames{ 0 }, g_moved{ 0 };
uint32_t g_skipped_last = 0, g_seen_last = 0, g_moved_last = 0;
constexpr uintptr_t kAlbedoParam = 0x00E5540C;   // mhXfSamplerAlbedoMap: its sShader::mParameterDesc index
constexpr uintptr_t kCameraBit   = 0x00E55780;   // `Camera`: which of a texture's two handles is current
constexpr uintptr_t kTexBound    = 0x8F54A8;     // the loop body after SetTexture (the sampler state)
constexpr uintptr_t kShaderInstance = 0x00E55700; // sShader::mpInstance
constexpr uintptr_t kParamDescs  = 0x838;        // ParameterDesc[512], 20 bytes: +8 the name's ID
// the water technique's samplers (XFUNIQUE IDs, as TexFilter): a pass sampling one of them without an albedo map is water
constexpr uint32_t kWaterIds[] = { 0xC70A6632 /*gXfFlowingWaterSampler*/, 0x73A29EF8 /*XfWaterNormalMapSampler*/,
                                   0xDEE93D6B /*XfWaterCausticsSampler*/, 0xF186BC12 /*SamplerNormalMapFilter (XfFlatWater)*/,
                                   0x0FA373F0 /*SamplerReflectionMap*/, 0x46F9F252 /*SamplerRefractionMap*/ };
constexpr int kWaterCount = sizeof(kWaterIds) / sizeof(kWaterIds[0]);
int      g_water_param[kWaterCount] = { -1, -1, -1, -1, -1, -1 };   // their parameter indices, looked up while missing
constexpr uint32_t kShadowNearId = 0xFC61C500;   // XfSamplerShadowNear: on stage 0 of the shadow overlay pass
int      g_shadow_param = -1;
// an effect pass binding only the depth buffer (XfPrimDepthPointSampler) has no texture: its glow is shaped in the pixel
// shader, which Remix doesn't run, and the depth buffer on stage 0 came out as white diamonds (lantern halos)
constexpr uint32_t kPrimDepthId = 0xB27FF634;
int      g_prim_depth_param = -1;
constexpr uint32_t kWaterZBufferId = 0xAAB42A50;   // XfWaterZBufferSampler: the sea-type water's depth fade
int      g_water_zbuf_param = -1;
// effects that bend the screen image (XfPrimSceneSampler: heat haze, refracting sword trails) through a distortion map;
// Remix sees only the distortion map, as a purple colour
constexpr uint32_t kPrimSceneId = 0xA56452E1;
int      g_prim_scene_param = -1;
// water draws get a moving texture transform on stage 0 (Remix 1.6.1 reads D3DTS_TEXTURE0 + TEXTURETRANSFORMFLAGS for every
// draw, d3d9_rtx_utils.cpp setTextureStageState; D3D9 ignores them for shader draws, so the game's image is unchanged):
// the layered water normals (rtx.opaqueMaterial.layeredWaterNormalEnable) offset their second sample by its translation
bool     g_tt_on = false;
// the sky: SOLID (pass 2) draws whose only sampler is XfBaseSampler (the sky panels / dome shader). Remix counts a draw
// whose viewport MinZ is >= rtx.skyMinZThreshold (1.0) as sky (d3d9_rtx.cpp setupCategoriesForHeuristics): rasterised
// into the background and the sky probe, so it lights the scene instead of blocking the sun. The game's viewport comes
// back at the next material (or pass, or the injection). The game's own raster of the sky is covered by the traced image.
constexpr uint32_t kBaseSamplerId = 0x4C3FB8D3;   // XfBaseSampler
int      g_base_param = -1;
bool     g_sky_vp = false;
D3DVIEWPORT9 g_saved_vp{};
std::atomic<uint32_t> g_sky{ 0 };
uint32_t g_sky_last = 0;

void sky_viewport(IDirect3DDevice9* dev, bool on) {
    if (on && !g_sky_vp) {
        D3DVIEWPORT9 vp{};
        if (FAILED(dev->GetViewport(&vp)) || vp.Width == 0 || vp.Height == 0) return;
        g_saved_vp = vp;
        vp.MinZ = 1.0f;
        vp.MaxZ = 1.0f;
        dev->SetViewport(&vp);
        g_sky_vp = true;
    } else if (!on && g_sky_vp) {
        dev->SetViewport(&g_saved_vp);
        g_sky_vp = false;
    }
}
LARGE_INTEGER g_qpf{}, g_qpc0{};

void water_transform(IDirect3DDevice9* dev, bool on) {
    if (on) {
        if (!g_qpf.QuadPart) { QueryPerformanceFrequency(&g_qpf); QueryPerformanceCounter(&g_qpc0); }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        const double t = double(now.QuadPart - g_qpc0.QuadPart) / double(g_qpf.QuadPart);
        auto wrap = [](double x) { return float(x - std::floor(x)); };
        D3DMATRIX m{};
        m._11 = m._22 = settings.water_tiling;
        m._33 = m._44 = 1.0f;
        m._31 = wrap(t * settings.water_flow[0]);   // 2-element texture coordinates: the third row is the translation
        m._32 = wrap(t * settings.water_flow[1]);
        dev->SetTransform(D3DTS_TEXTURE0, &m);
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        g_tt_on = true;
    } else if (g_tt_on) {
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        g_tt_on = false;
    }
}
// Remix skips a draw whose stage 0 texture has no content hash ("Texture 0 without valid hash", d3d9_rtx.cpp): a
// default-pool texture that is never filled has none (d3d9_common_texture.cpp SetupForRtxFrom hashes the CPU copy; render
// targets get one from their size). Hidden draws get this one on stage 0. Released before device resets.
IDirect3DTexture9* g_hide = nullptr;
std::atomic<uint32_t> g_hidden{ 0 };
uint32_t g_hidden_last = 0;

// ---- the stage's lights ----
// Light units (sdk/uLight.hpp; values read live in room 400, 2026-11-24): uLight +0x18 attr, +0x1C light-group mask,
// +0x20 mColor (linear rgb, above 1 for the main light); uInfiniteLight +0x30 mDir = toward the light (the bright one has
// +y); MtVector3 is 16 bytes (x, y, z, pad): uPointLight +0x40 mWPos, +0x50 mStart, +0x54 mEnd; uSpotLight +0x60 mWPos,
// +0x84 mEnd (taken as a point light). Remix makes its lights of the enabled D3D9 lights; D3D9 allows 8 at once:
// directional ones first, then the point lights that matter most to the view (nearest to the camera relative to their range).
struct GameLight { int kind; float col[3]; float v[3]; float range; uint32_t group; };   // kind 1 directional (v = travel), 2 point (v = pos)
constexpr int kMaxLights = 8;              // D3D9 lights on at once
constexpr int kMaxAll = 72;               // lights handed over per frame, in batches of 8 (Remix keeps every light it
                                          // is given during a frame: LightManager::addLight, keyed by the light's hash)
GameLight g_lights[kMaxAll];
D3DLIGHT9 g_frame[kMaxAll + 2];
int       g_frame_n = 0, g_frame_next = 0;
int       g_nlights = 0, g_ndir = 0, g_npoint = 0, g_prev_enabled = 0;
// the stage's hemisphere (ambient) lights: their sky colours summed. Remix has no ambient term and DMC4's interiors get
// theirs from these and from baked lightmaps, so rooms went dark: the fill light (a wide point light at the camera)
float     g_hemi[3] = { 0, 0, 0 }, g_hemi_acc[3] = { 0, 0, 0 };
uintptr_t g_dti_hemi = 0;
// Light groups: a uLight lights a model only when its group mask (+0x1C) shares a bit with the model's mLightGroup
// (uModel +0x130; the .mod header's light group, 3 for characters by default). Stages keep alternative lights per group
// (room 307: three suns of 1.5 for characters, stage and breakables, five hemisphere colours); Remix has no groups and
// added them all up (blown out). The stage's dominant group = the most common mLightGroup among the models, leaving out
// the characters' 3; only lights sharing a bit with it go over (all of them if none does).
uintptr_t g_dti_model = 0;
uint32_t  g_stage_group = 0;
uintptr_t g_dti_inf = 0, g_dti_point = 0, g_dti_spot = 0;
bool      g_dti_looked = false;

bool dti_under(uintptr_t dti, uintptr_t target) {
    for (int i = 0; dti && target && i < 16; ++i) {
        if (dti == target) return true;
        dti = *reinterpret_cast<const uintptr_t*>(dti + 0x10);   // MtDTI::mp_parent
    }
    return false;
}

// walks every move line's units (render thread, while the game thread runs them: SEH-guarded, returns -1 on a fault)
int gather_lights_raw(GameLight* out, int max) {
    __try {
        const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
        if (!su) return 0;
        g_hemi_acc[0] = g_hemi_acc[1] = g_hemi_acc[2] = 0;
        constexpr int kMaxPoints = 64;
        GameLight dirs[kMaxLights], points[kMaxPoints];
        float score[kMaxPoints];
        int nd = 0, np = 0;
        struct Hemi { uint32_t group; float col[3]; } hemis[16];
        int nh = 0;
        struct Mask { uint32_t mask, n; } masks[16];
        int nm = 0;
        float cam[3] = { 0, 0, 0 };
        if (const uintptr_t camera = *reinterpret_cast<uintptr_t*>(0x00E552D4)) {
            D3DMATRIX cv;
            std::memcpy(&cv, &reinterpret_cast<const sCamera*>(camera)->viewports[0].mViewMat, sizeof cv);
            for (int i = 0; i < 3; ++i) cam[i] = -(cv.m[3][0] * cv.m[i][0] + cv.m[3][1] * cv.m[i][1] + cv.m[3][2] * cv.m[i][2]);
        }
        for (int line = 0; line < 32; ++line) {
            const auto* ml = reinterpret_cast<const MoveLine*>(su + 0x20 + line * sizeof(MoveLine));
            int guard = 0;
            for (uintptr_t u = reinterpret_cast<uintptr_t>(ml->mTop); u && guard < 20000; ++guard) {
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uintptr_t next = reinterpret_cast<uintptr_t>(cu->mp_next_unit);
                if (cu->flags.bits.mBeFlag) {
                    const uintptr_t dti = debug_server::mem::dti_of(u);
                    const float* col = reinterpret_cast<const float*>(u + 0x20);
                    const uint32_t group = *reinterpret_cast<const uint32_t*>(u + 0x1C);
                    if (dti && g_dti_model && dti_under(dti, g_dti_model)) {
                        const uint32_t m = *reinterpret_cast<const uint32_t*>(u + 0x130);
                        if (m && m != 3) {
                            int i = 0;
                            while (i < nm && masks[i].mask != m) ++i;
                            if (i < nm) ++masks[i].n;
                            else if (nm < 16) masks[nm++] = { m, 1 };
                        }
                    } else if (dti && g_dti_hemi && dti_under(dti, g_dti_hemi)) {
                        if (nh < 16) hemis[nh++] = { group, { col[0], col[1], col[2] } };
                    } else if (dti && dti_under(dti, g_dti_inf) && nd < kMaxLights) {
                        const float* d = reinterpret_cast<const float*>(u + 0x30);
                        dirs[nd++] = { 1, { col[0], col[1], col[2] }, { -d[0], -d[1], -d[2] }, 0.0f, group };
                    } else if (dti && (dti_under(dti, g_dti_point) || dti_under(dti, g_dti_spot)) && np < kMaxPoints) {
                        const bool spot = dti_under(dti, g_dti_spot);
                        const float* p = reinterpret_cast<const float*>(u + (spot ? 0x60 : 0x40));
                        const float end = *reinterpret_cast<const float*>(u + (spot ? 0x84 : 0x54));
                        const float dx = p[0] - cam[0], dy = p[1] - cam[1], dz = p[2] - cam[2];
                        score[np] = std::sqrt(dx * dx + dy * dy + dz * dz) / (end > 1.0f ? end : 1.0f);
                        points[np++] = { 2, { col[0], col[1], col[2] }, { p[0], p[1], p[2] }, end, group };
                    }
                }
                u = next;
            }
        }
        // the stage's dominant light group; the lights (and the ambient colour) for it
        uint32_t stage = 0, best_n = 0;
        for (int i = 0; i < nm; ++i) if (masks[i].n > best_n) { best_n = masks[i].n; stage = masks[i].mask; }
        bool any = false;
        for (int i = 0; i < nd; ++i) any |= (dirs[i].group & stage) != 0;
        for (int i = 0; i < np; ++i) any |= (points[i].group & stage) != 0;
        if (!any) stage = 0;                                  // nothing for it: everything (as before)
        g_stage_group = stage;
        auto keep = [stage](uint32_t g) { return stage == 0 || (g & stage) != 0; };
        {   // compact the points kept
            int k = 0;
            for (int i = 0; i < np; ++i) if (keep(points[i].group)) { points[k] = points[i]; score[k] = score[i]; ++k; }
            np = k;
        }
        for (int i = 0; i < nh; ++i)                          // the brightest ambient colour for the stage's group
            if (keep(hemis[i].group) && hemis[i].col[0] + hemis[i].col[1] + hemis[i].col[2] > g_hemi_acc[0] + g_hemi_acc[1] + g_hemi_acc[2])
                for (int c = 0; c < 3; ++c) g_hemi_acc[c] = hemis[i].col[c];
        int n = 0;
        for (int i = 0; i < nd && n < max; ++i) if (keep(dirs[i].group)) out[n++] = dirs[i];
        while (n < max && np > 0) {   // the most relevant point light left: nearest relative to its range
            int best = 0;
            for (int i = 1; i < np; ++i) if (score[i] < score[best]) best = i;
            out[n++] = points[best];
            points[best] = points[np - 1];
            score[best] = score[np - 1];
            --np;
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void gather_lights() {
    if (!g_dti_looked) {
        g_dti_inf = debug_server::mem::find_dti("uInfiniteLight");
        g_dti_point = debug_server::mem::find_dti("uPointLight");
        g_dti_spot = debug_server::mem::find_dti("uSpotLight");
        g_dti_hemi = debug_server::mem::find_dti("uHemiSphereLight");
        g_dti_model = debug_server::mem::find_dti("uModel");
        g_dti_looked = g_dti_inf != 0;
    }
    GameLight tmp[kMaxAll];
    const int n = gather_lights_raw(tmp, kMaxAll);
    if (n < 0) return;   // a unit went away mid-walk: keep last frame's
    for (int i = 0; i < 3; ++i) g_hemi[i] = g_hemi_acc[i];
    g_nlights = n;
    g_ndir = g_npoint = 0;
    for (int i = 0; i < n; ++i) {
        g_lights[i] = tmp[i];
        if (tmp[i].kind == 1) ++g_ndir; else ++g_npoint;
    }
}

// Remix rasterises every draw after the first one it takes for UI ("RTX injection", d3d9_rtx.cpp internalPrepareDraw /
// makeDrawCallType: a fixed-function draw to the screen-sized target with an orthographic projection, _44 == 1, and
// depth writes off). DMC4's HUD uses vertex shaders, so Remix traced it as world geometry (it slid around with the
// camera). One degenerate fixed-function triangle like that at the start of the HUD pass starts the injection there; the
// state block puts the device back as the game left it (its caches stay right).
// Remix's final output pass tone-maps its image and encodes it as sRGB (rtx_context.cpp: dispatchToneMapping,
// dispatchSRGBDither with the hard-wired g_allowSrgbConversionForOutput) before blitting it into the target, here DMC4's
// linear HDR scene buffer (A16B16G16R16F), which the game then tone-maps and blooms again: blown-out highlights. Right
// after the injection the buffer is copied out and drawn back through a small shader: sRGB -> linear, then the tone curve
// undone (inverse Reinhard, x / (1 - x), floored), so the game's tone map gets a scene-like HDR image.
const char* kLinearizeSrc =
    "sampler2D s0 : register(s0);\n"
    "float4 k : register(c0);\n"   // x = expand, y = floor of 1 - x, z = exposure
    "float3 to_linear(float3 c) { return c <= 0.04045 ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }\n"
    "float4 main(float2 uv : TEXCOORD0) : COLOR {\n"
    "    float3 l = to_linear(saturate(tex2D(s0, uv).rgb));\n"
    "    float3 h = l / max(1.0 - l, k.y);\n"
    "    return float4(lerp(l, h, k.x) * k.z, 1.0);\n"
    "}\n";
IDirect3DPixelShader9* g_ps_linearize = nullptr;
bool               g_lin_failed = false;
IDirect3DTexture9* g_lin_copy = nullptr;            // default pool: released before device resets
UINT g_lin_w = 0, g_lin_h = 0; D3DFORMAT g_lin_fmt = D3DFMT_UNKNOWN;

struct LinBlob : IUnknown {   // ID3DBlob, without the D3DCompiler headers (as PostFX)
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};

bool linearize_shader(IDirect3DDevice9* dev) {
    if (g_ps_linearize || g_lin_failed) return g_ps_linearize != nullptr;
    using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const void*, void*, const char*, const char*, UINT,
                                         UINT, LinBlob**, LinBlob**);
    HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
    auto compile = dll ? reinterpret_cast<D3DCompileFn>(GetProcAddress(dll, "D3DCompile")) : nullptr;
    LinBlob *code = nullptr, *err = nullptr;
    if (!compile || FAILED(compile(kLinearizeSrc, strlen(kLinearizeSrc), "remix_linearize", nullptr, nullptr, "main", "ps_3_0",
                                   0, 0, &code, &err)) || !code ||
        FAILED(dev->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()), &g_ps_linearize))) {
        g_lin_failed = true;
        g_ps_linearize = nullptr;
    }
    if (code) code->Release();
    if (err) err->Release();
    return g_ps_linearize != nullptr;
}

void linearize(IDirect3DDevice9* dev) {
    if (!settings.linearize || !linearize_shader(dev)) return;
    IDirect3DSurface9 *rt = nullptr, *copy = nullptr;
    if (FAILED(dev->GetRenderTarget(0, &rt)) || !rt) return;
    D3DSURFACE_DESC d{};
    rt->GetDesc(&d);
    if (!g_lin_copy || g_lin_w != d.Width || g_lin_h != d.Height || g_lin_fmt != d.Format) {
        if (g_lin_copy) { g_lin_copy->Release(); g_lin_copy = nullptr; }
        if (FAILED(dev->CreateTexture(d.Width, d.Height, 1, D3DUSAGE_RENDERTARGET, d.Format, D3DPOOL_DEFAULT, &g_lin_copy, nullptr)))
            g_lin_copy = nullptr;
        g_lin_w = d.Width; g_lin_h = d.Height; g_lin_fmt = d.Format;
    }
    IDirect3DStateBlock9* sb = nullptr;
    if (g_lin_copy && SUCCEEDED(g_lin_copy->GetSurfaceLevel(0, &copy)) &&
        SUCCEEDED(dev->StretchRect(rt, nullptr, copy, nullptr, D3DTEXF_NONE)) &&
        SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL, &sb)) && sb) {
        struct V { float x, y, z, rhw, u, v; };
        const float w = float(d.Width), h = float(d.Height);
        const V v[3] = { { -0.5f, -0.5f, 0, 1, 0, 0 }, { 2 * w - 0.5f, -0.5f, 0, 1, 2, 0 }, { -0.5f, 2 * h - 0.5f, 0, 1, 0, 2 } };
        const float k[4] = { settings.expand, 0.25f, settings.exposure, 0 };   // the inverse curve capped at 4x
        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(g_ps_linearize);
        dev->SetPixelShaderConstantF(0, k, 1);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        dev->SetTexture(0, g_lin_copy);
        dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, v, sizeof(V));
        sb->Apply();
    }
    if (sb) sb->Release();
    if (copy) copy->Release();
    rt->Release();
}

bool g_ui_done = false;
bool g_solid_seen = false;   // this frame's main-scene SOLID (2) pass has started: the 3D is in
// the frame's tag order once a minute (every 3600th frame): the (view, sub-scene, pass) runs, to remix_passes.txt
uint32_t g_order[256]; int g_norder = 0; bool g_order_written = false;
uintptr_t g_order_rt[256]; uint32_t g_order_desc[256];   // the bound render target of each run, its w | h << 12 | fmt << 24
std::atomic<uint32_t> g_presents{ 0 };                  // PostFX::on_frame calls (one per present)
uint32_t g_disp_since_present = 0, g_disp_per_present_max = 0, g_ui_fired_pass = 0xFF; uintptr_t g_ui_fired_rt = 0;
std::atomic<uint32_t> g_ui_triggers{ 0 };

void trigger_ui(IDirect3DDevice9* dev) {
    IDirect3DStateBlock9* sb = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &sb)) || !sb) return;
    struct V { float x, y, z; DWORD c; } v[3] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
    D3DMATRIX ortho{};
    ortho._11 = ortho._22 = ortho._33 = ortho._44 = 1.0f;
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->SetTransform(D3DTS_PROJECTION, &ortho);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, v, sizeof(V));
    sb->Apply();
    sb->Release();
    ++g_ui_triggers;
}

IDirect3DTexture9* hide_texture(IDirect3DDevice9* dev) {
    if (!g_hide && FAILED(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_hide, nullptr)))
        g_hide = nullptr;
    return g_hide;
}
// effects (sPrim) take their colour from these instead of the albedo map; stage 0 is often the depth buffer (soft
// particles, XfPrimDepthPointSampler), which Remix drew as solid white sprites
constexpr uint32_t kPrimBaseIds[] = { 0xBCB92646 /*XfPrimBaseLinearSampler*/, 0x1FE75530 /*XfPrimBasePointSampler*/,
                                      0xB5E3E840 /*XfPrimSampler*/ };
int      g_prim_param[3] = { -1, -1, -1 };
float    g_sun_dir[3] = { 0.0f, -1.0f, 0.0f };
IDirect3DTexture9* g_standin = nullptr;          // the water stand-in (managed: survives resets)
uint32_t g_standin_rgb = 0;                       // the colour it was filled with
std::atomic<uint32_t> g_water{ 0 };
uint32_t g_water_last = 0;

int param_of(uint32_t id) {
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) return -1;
    for (int i = 0; i < 512; ++i)
        if (*reinterpret_cast<const uint32_t*>(shader + kParamDescs + 20 * i + 8) == id) return i;
    return -1;
}

bool is_colour(uint32_t param, uint32_t albedo) {
    if (param == albedo) return true;
    for (int w : g_prim_param) if (w >= 0 && static_cast<uint32_t>(w) == param) return true;
    return false;
}

bool is_water(uint32_t param) {
    for (int w : g_water_param) if (w >= 0 && static_cast<uint32_t>(w) == param) return true;
    return false;
}

uint32_t rgb_of(const float* c) {
    auto b = [](float v) { return static_cast<uint32_t>((v < 0 ? 0 : v > 1 ? 1 : v) * 255.0f + 0.5f); };
    return 0xFF000000u | (b(c[0]) << 16) | (b(c[1]) << 8) | b(c[2]);
}

// a 4x4 solid texture of the water colour; refilled when the colour changes (Remix hashes the content, so one colour =
// one texture for a Remix material replacement)
IDirect3DTexture9* standin(IDirect3DDevice9* dev) {
    const uint32_t rgb = rgb_of(settings.water_color);
    if (!g_standin && FAILED(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &g_standin, nullptr)))
        g_standin = nullptr;
    if (g_standin && rgb != g_standin_rgb) {
        D3DLOCKED_RECT r;
        if (SUCCEEDED(g_standin->LockRect(0, &r, nullptr, 0))) {
            for (int y = 0; y < 4; ++y) {
                auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(r.pBits) + y * r.Pitch);
                for (int x = 0; x < 4; ++x) row[x] = rgb;
            }
            g_standin->UnlockRect(0);
            g_standin_rgb = rgb;
        }
    }
    return g_standin;
}
bool     g_in_main = false;

bool      g_have_main = false;          // the main camera's matrices of the last frame (vs_array)
D3DMATRIX g_main_v{}, g_main_p{};
// the next 8 of the frame's lights into the D3D9 slots (Remix adds them at the next draw); slots left over are switched off
void submit_lights(IDirect3DDevice9* dev) {
    int slot = 0;
    for (; slot < kMaxLights && g_frame_next < g_frame_n; ++slot, ++g_frame_next) {
        dev->SetLight(slot, &g_frame[g_frame_next]);
        dev->LightEnable(slot, TRUE);
    }
    for (int i = slot; i < g_prev_enabled; ++i) dev->LightEnable(i, FALSE);
    g_prev_enabled = slot;
}

void set_transforms(IDirect3DDevice9* dev) {
    const uintptr_t camera = *reinterpret_cast<uintptr_t*>(kCameraInstance);
    if (!camera || !dev) return;
    const auto* cam = reinterpret_cast<const sCamera*>(camera);
    D3DMATRIX view, proj, world;
    std::memcpy(&view, &cam->viewports[0].mViewMat, sizeof view);     // row vectors, as D3D
    std::memcpy(&proj, &cam->viewports[0].mProjMat, sizeof proj);
    if (g_have_main) { view = g_main_v; proj = g_main_p; }           // last frame's main camera (sCamera only until then)
    std::memset(&world, 0, sizeof world);
    world._11 = world._22 = world._33 = world._44 = 1.0f;
    dev->SetTransform(D3DTS_WORLD, &world);
    dev->SetTransform(D3DTS_VIEW, &view);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
    // a white fixed-function material: Remix colours the lights it adds to tagged textures ("Add Light to Texture",
    // scene manager createEffectLight) with its Diffuse times rtx.effectLightColor; DMC4 never sets one (black lights)
    {
        D3DMATERIAL9 mat{};
        mat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
        mat.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
        dev->SetMaterial(&mat);
    }
    // lights: Remix makes its lights of every enabled D3D9 light (d3d9_rtx.cpp, DirtyLights), shader draws too; the frame's
    // list goes over in batches of 8 (submit_lights: here, then at each new material until all are given)
    g_frame_n = g_frame_next = 0;
    if (settings.game_lights && g_nlights > 0) {
        for (int i = 0; i < g_nlights; ++i) {
            const GameLight& g = g_lights[i];
            const float k = g.kind == 1 ? settings.dir_strength : settings.point_strength;
            D3DLIGHT9 l{};
            l.Diffuse = { g.col[0] * k, g.col[1] * k, g.col[2] * k, 1.0f };
            if (g.kind == 1) {
                l.Type = D3DLIGHT_DIRECTIONAL;
                l.Direction = { g.v[0], g.v[1], g.v[2] };
            } else {
                l.Type = D3DLIGHT_POINT;
                l.Position = { g.v[0], g.v[1], g.v[2] };
                l.Range = g.range > 1.0f ? g.range : 1000.0f;
                l.Attenuation0 = 1.0f;
            }
            g_frame[g_frame_n++] = l;
        }
    } else if (settings.sun) {   // no light units found: the shadow light
        D3DLIGHT9 l{};
        l.Type = D3DLIGHT_DIRECTIONAL;
        l.Diffuse = { settings.sun_color[0] * settings.sun_strength, settings.sun_color[1] * settings.sun_strength,
                      settings.sun_color[2] * settings.sun_strength, 1.0f };
        l.Direction = { g_sun_dir[0], g_sun_dir[1], g_sun_dir[2] };
        g_frame[g_frame_n++] = l;
    }
    // the ambient fill: the stage's hemisphere colour as a wide point light at the camera
    if (settings.game_lights && settings.fill && (g_hemi[0] + g_hemi[1] + g_hemi[2]) > 0.0f) {
        float pos[3];   // the camera: -t * R^T of the row-vector view
        for (int i = 0; i < 3; ++i) pos[i] = -(view.m[3][0] * view.m[i][0] + view.m[3][1] * view.m[i][1] + view.m[3][2] * view.m[i][2]);
        D3DLIGHT9 l{};
        l.Type = D3DLIGHT_POINT;
        l.Diffuse = { g_hemi[0] * settings.fill_strength, g_hemi[1] * settings.fill_strength, g_hemi[2] * settings.fill_strength, 1.0f };
        l.Position = { pos[0], pos[1], pos[2] };
        l.Range = settings.fill_range;
        l.Attenuation0 = 1.0f;
        g_frame[g_frame_n++] = l;
    }
    submit_lights(dev);
}

// The pixel-shader texture loop of dispatchCommands, per sampler entry of the pass (`record` +0x0C low nibble = the
// count, +0x10 the entries: bits 0-15 the parameter index, 16-23 the D3D9 stage, 24-31 the sampler state; `slot` = the
// entry's resource (rTexture's D3D9 handles at +0x10 / +0x14, picked by +0x22 bit 0 & Camera); `cache` = the game's
// per-parameter "already bound" table). Returns true when it bound the texture itself.
// Remix (d3d9_rtx.cpp, internalPrepareDraw) takes a shader draw's colour texture from stage 0 and a second one from stage
// 1, whatever the shader samples there; DMC4's shaders put the albedo on any stage (the floor's normal map came out as the
// colour). So when the pass has an albedo map: the albedo goes to stage 0 and whatever the shader had on stage 0 to the
// albedo's stage. The game's cache is invalidated for every entry bound here, so later draws (other scenes, the bridge
// off) bind again instead of trusting stages this moved.
uint32_t g_pass = 0;                         // the current tag's pass (bits 20-23), for the pass log

// diagnostics: each distinct pass layout of the main scene once (the samplers' name IDs and stages, the tag's pass and
// what the bridge did), appended to remix_passes.txt in the game folder. Render thread only.
void log_layout(const uint32_t* entries, uint32_t n, const char* what) {
    static std::unordered_set<std::string> seen;
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader || seen.size() > 4000) return;
    std::string sig = "pass " + std::to_string(g_pass) + ":";
    char buf[32];
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t p = entries[i] & 0xFFFF, stage = (entries[i] >> 16) & 0xFF;
        const uint32_t id = p < 512 ? *reinterpret_cast<const uint32_t*>(shader + kParamDescs + 20 * p + 8) : 0;
        sprintf_s(buf, " s%u=%08X", stage, id);
        sig += buf;
    }
    if (!seen.insert(sig).second) return;
    FILE* f = nullptr;
    if (fopen_s(&f, "remix_passes.txt", "a") == 0 && f) {
        fprintf(f, "%s -> %s\n", sig.c_str(), what);
        fclose(f);
    }
}

int __stdcall bind_texture(IDirect3DDevice9* dev, const uint32_t* entry, const uint32_t* slot, const uint8_t* record,
                           uint32_t* cache) {
    if (!settings.on || !settings.albedo_to_stage0) return 0;
    const uint32_t albedo = *reinterpret_cast<const uint32_t*>(kAlbedoParam);
    const uint32_t n = *reinterpret_cast<const uint32_t*>(record + 0x0C) & 0xF;
    const auto* entries = reinterpret_cast<const uint32_t*>(record + 0x10);
    uint32_t albedo_stage = 0xFF;
    bool water = false;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t p = entries[i] & 0xFFFF;
        if (albedo_stage == 0xFF && is_colour(p, albedo)) albedo_stage = (entries[i] >> 16) & 0xFF;
        else if (is_water(p)) water = true;
    }
    if (entry == entries) {
        const bool hiding = albedo_stage == 0xFF && !water &&
            ((g_pass == kOcclusion && settings.hide_occlusion) ||
             (g_pass == kOverlap && settings.hide_shadow_overlays && g_shadow_param >= 0));
        log_layout(entries, n, albedo_stage != 0xFF ? "colour to stage 0" : water && settings.water_standin ? "water stand-in"
                                                                       : hiding ? "hidden (if shadow overlay)" : "left alone");
    }
    // draws Remix shouldn't trace: the occlusion-query quads (the depth texture on stage 0: white diamonds at the lights)
    // and the stage drawn again with only its shadow maps (duplicate surfaces with a shadow map as their colour)
    bool hide = false;
    if (settings.hide_refraction && g_prim_scene_param >= 0)
        for (uint32_t i = 0; i < n; ++i)
            if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_prim_scene_param)) hide = true;
    if (entry == entries) {
        if (g_frame_next < g_frame_n) submit_lights(dev);   // the next batch of the frame's lights
        water_transform(dev, !hide && albedo_stage == 0xFF && water && settings.water_standin);
        const bool sky = settings.sky && g_pass == 2 && n == 1 && g_base_param >= 0 &&
                         (entries[0] & 0xFFFF) == static_cast<uint32_t>(g_base_param);
        sky_viewport(dev, sky);
        if (sky) ++g_sky;
    }
    if (albedo_stage == 0xFF && !water) {
        if (g_pass == kOcclusion && settings.hide_occlusion) hide = true;
        if (g_pass == kOverlap && settings.hide_shadow_overlays && g_shadow_param >= 0)
            for (uint32_t i = 0; i < n; ++i)
                if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_shadow_param)) hide = true;
        if (settings.hide_glows && g_prim_depth_param >= 0)
            for (uint32_t i = 0; i < n; ++i)
                if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_prim_depth_param)) hide = true;
    }
    // water (no albedo map, its wave / flow maps on stage 0 came out teal / magenta): a stand-in colour on stage 0, the
    // shader's own stage 0 texture on stage 15
    // water layouts the user hides (puddles: square planes over dips that the game fades out by depth, which Remix can't)
    if (!hide && albedo_stage == 0xFF && water) {
        bool zbuf = false, flat = false;
        for (uint32_t i = 0; i < n; ++i) {
            const int p = static_cast<int>(entries[i] & 0xFFFF);
            if (p == g_water_zbuf_param) zbuf = true;
            if (p == g_water_param[3]) flat = true;   // SamplerNormalMapFilter: XfFlatWater
        }
        if ((zbuf && settings.hide_water_depth) || (flat && settings.hide_water_flat) || (n == 1 && settings.hide_water_simple))
            hide = true;
    }
    if (hide || (albedo_stage == 0xFF && water && settings.water_standin)) {
        if (entry == entries) {
            if (hide) { dev->SetTexture(0, hide_texture(dev)); ++g_hidden; }
            else { dev->SetTexture(0, standin(dev)); ++g_water; }
        }
        const uint32_t param = *entry & 0xFFFF, stage = (*entry >> 16) & 0xFF;
        const uintptr_t res = *slot;
        cache[param] = 0xFFFFFFFF;
        IDirect3DBaseTexture9* tex = nullptr;
        if (res) {
            const uint32_t which = *reinterpret_cast<const uint8_t*>(res + 0x22) & 1 & *reinterpret_cast<const uint32_t*>(kCameraBit);
            tex = *reinterpret_cast<IDirect3DBaseTexture9* const*>(res + 0x10 + 4 * which);
        }
        dev->SetTexture(stage == 0 ? 15 : stage, tex);
        return 1;
    }
    if (albedo_stage == 0xFF) return 0;                 // no albedo map: the game binds as usual
    const uint32_t param = *entry & 0xFFFF, stage = (*entry >> 16) & 0xFF;
    const uintptr_t res = *slot;
    cache[param] = 0xFFFFFFFF;
    IDirect3DBaseTexture9* tex = nullptr;
    if (res) {
        const uint32_t which = *reinterpret_cast<const uint8_t*>(res + 0x22) & 1 & *reinterpret_cast<const uint32_t*>(kCameraBit);
        tex = *reinterpret_cast<IDirect3DBaseTexture9* const*>(res + 0x10 + 4 * which);
    }
    uint32_t to = stage;
    if (stage == albedo_stage) to = 0;
    else if (stage == 0) to = albedo_stage;
    if (to != stage) ++g_moved;
    dev->SetTexture(to, tex);
    return 1;
}

} // namespace

// ---- shader relabelling ----
// Remix's vertex capture (dxso_compiler.cpp, emitVertexCaptureInit / the capture write) takes a vertex's UV from the
// vertex shader's TEXCOORD0 output (.xy) and its colour from COLOR0. DMC4's effect shaders (XfPrim) write the vertex colour
// to TEXCOORD0 (`mul o1.xyz, r1.w, v1`, alpha in .w) and the UV to TEXCOORD1, so Remix drew every sprite with one texel
// of its texture (the lantern glows as white diamonds). D3D9 SM3 links vertex outputs to pixel inputs by these labels
// (dcl usage tokens), so relabelling both halves of a pass the same way leaves the game's own rendering as it was:
// TEXCOORD0 -> COLOR0 (or TEXCOORD1 when the shader already has a COLOR0 output) and TEXCOORD1 -> TEXCOORD0.
// A vertex shader qualifies when an instruction writes its COLOR input into its TEXCOORD0 output. Only while the bridge
// is on, and only for shaders created after it was turned on (they're created on first use).
// Normals: the capture takes a vertex's normal from the shader's NORMAL0 output, else from its NORMAL input
// (dxso_compiler.cpp m_vs.oNormal0, d3d9_rtx.cpp prepareVertexCapture). No DMC4 vertex shader outputs one, and the input
// is UBYTE4N 0..1 in the model's own space, which the shader decodes (mad r, v1, 2, -1) and turns with the world or bone
// matrices: Remix shaded every material draw with skewed, unskinned normals. Each material shader passes its world normal
// to the pixel shader in a TEXCOORD output; normal_output finds it by following which inputs each output component
// depends on (the NORMAL input and no POSITION / TANGENT / BINORMAL one, in .x .y .z of one register: all 149
// XfMaterialStandard vertex shaders, the 114 shadow receivers, 46 of XfMaterialBase's 87 with a normal input, whose
// others only output positions), and add_normal_output gives the shader a NORMAL0 output in a free register, written by
// copies (.xyz) of the instructions writing that one. The pixel shader is untouched (it doesn't read the new output). The
// bytecode grows, so the game gets a copy: it reads the pointer after this hook (0x8F49F4, CreateVertexShader).
namespace sm3 {
constexpr uint32_t kDcl = 0x1F, kComment = 0xFFFE, kEnd = 0x0000FFFF;
constexpr uint32_t kInput = 1, kOutput = 6, kTexcoord = 5, kColor = 10;   // D3DSPR_INPUT, D3DSPR_OUTPUT (vs_3_0)
uint32_t reg_type(uint32_t t) { return ((t >> 28) & 7) | ((t >> 8) & 0x18); }
uint32_t reg_num(uint32_t t) { return t & 0x7FF; }
uint32_t usage_token(uint32_t usage, uint32_t index) { return 0x80000000u | usage | (index << 16); }

struct Dcl { uint32_t* usage; uint32_t use, index, type, num; };

// walks a shader's tokens: its dcls and, for the others, calls f(opcode token pointer, length)
template <typename F>
bool walk(uint32_t* code, std::vector<Dcl>& dcls, F&& f) {
    uint32_t* p = code + 1;
    for (int guard = 0; guard < 65536; ++guard) {
        const uint32_t t = *p;
        if (t == kEnd) return true;
        if ((t & 0xFFFF) == kComment) { p += 1 + ((t >> 16) & 0x7FFF); continue; }
        const uint32_t len = (t >> 24) & 0xF;
        if ((t & 0xFFFF) == kDcl && len >= 2) {
            dcls.push_back({ p + 1, p[1] & 0x1F, (p[1] >> 16) & 0xF, reg_type(p[2]), reg_num(p[2]) });
        } else {
            f(p, len);
        }
        p += 1 + len;
    }
    return false;
}

const Dcl* find(const std::vector<Dcl>& d, uint32_t type, uint32_t use, uint32_t index) {
    for (auto& x : d) if (x.type == type && x.use == use && x.index == index) return &x;
    return nullptr;
}

// 0 = leave alone, 1 = TEXCOORD0 -> COLOR0, 2 = TEXCOORD0 -> TEXCOORD1; TEXCOORD1 -> TEXCOORD0 in both
int vertex_mode(uint32_t* code) {
    if ((code[0] & 0xFFFF0000u) != 0xFFFE0000u || (code[0] & 0xFFFF) < 0x0300) return 0;   // vs_3_0 only
    std::vector<Dcl> d;
    bool colour_in_t0 = false;
    std::vector<std::pair<uint32_t*, uint32_t>> ins;
    if (!walk(code, d, [&](uint32_t* p, uint32_t len) { ins.push_back({ p, len }); })) return 0;
    const Dcl* vcol = find(d, kInput, kColor, 0);
    const Dcl* t0 = find(d, kOutput, kTexcoord, 0);
    const Dcl* t1 = find(d, kOutput, kTexcoord, 1);
    if (!vcol || !t0 || !t1) return 0;
    for (auto& [p, len] : ins) {
        if (len < 2) continue;
        // only writes to .x / .y count: Remix reads TEXCOORD0.xy as the UV (XfMaterialStandard keeps its UV there and
        // vertex colour alpha in .z)
        if (reg_type(p[1]) != kOutput || reg_num(p[1]) != t0->num || !((p[1] >> 16) & 3)) continue;
        for (uint32_t k = 2; k <= len; ++k)
            if (reg_type(p[k]) == kInput && reg_num(p[k]) == vcol->num) colour_in_t0 = true;
    }
    if (colour_in_t0) return find(d, kOutput, kColor, 0) ? 2 : 1;
    // mode 3 | k << 4: TEXCOORD0.xy written by dp3 (a vector, not a UV), and output TEXCOORDk's .xy computed (mad / mul)
    // straight from the TEXCOORD0 input: swap TEXCOORD0 and TEXCOORDk
    const Dcl* vtex = find(d, kInput, kTexcoord, 0);
    if (!vtex || !t0) return 0;
    bool t0_vector = false;
    for (auto& [p, len] : ins)
        if (len >= 2 && (p[0] & 0xFFFF) == 8 /*dp3*/ && reg_type(p[1]) == kOutput && reg_num(p[1]) == t0->num &&
            ((p[1] >> 16) & 3)) t0_vector = true;
    if (!t0_vector) return 0;
    for (auto& [p, len] : ins) {
        const uint32_t op = p[0] & 0xFFFF;
        if (len < 3 || (op != 4 /*mad*/ && op != 5 /*mul*/) || reg_type(p[1]) != kOutput || ((p[1] >> 16) & 3) != 3) continue;
        bool from_uv = false;
        for (uint32_t k = 2; k <= len; ++k)
            if (reg_type(p[k]) == kInput && reg_num(p[k]) == vtex->num) from_uv = true;
        if (!from_uv) continue;
        for (auto& x : d)
            if (x.type == kOutput && x.use == kTexcoord && x.num == reg_num(p[1]) && x.index != 0 && x.index < 16)
                return 3 | static_cast<int>(x.index << 4);
    }
    return 0;
}

// relabels a shader's TEXCOORD0 / TEXCOORD1 dcls of register type `type` (vertex outputs or pixel inputs)
void relabel(uint32_t* code, uint32_t type, int mode) {
    std::vector<Dcl> d;
    if (!walk(code, d, [](uint32_t*, uint32_t) {})) return;
    const uint32_t k = (mode & 0xF) == 3 ? static_cast<uint32_t>(mode >> 4) : 1;
    uint32_t* t0 = nullptr; uint32_t* tk = nullptr;
    for (auto& x : d) {
        if (x.type != type || x.use != kTexcoord) continue;
        if (x.index == 0) t0 = x.usage;
        else if (x.index == k) tk = x.usage;
    }
    if (t0) *t0 = mode == 1 ? usage_token(kColor, 0) : usage_token(kTexcoord, k);
    if (tk) *tk = usage_token(kTexcoord, 0);
}

// ---- normals ----
constexpr uint32_t kNormalUsage = 3, kOutputs = 12;   // D3DDECLUSAGE_NORMAL; vs_3_0 has o0-o11
constexpr uint32_t kUsePosition = 1u << 0, kUseNormal = 1u << 3, kUseTangent = 1u << 6, kUseBinormal = 1u << 7;

// the components of source token `src` that instruction `op` reads to make destination component c (swizzle applied)
uint32_t read_comps(uint32_t op, uint32_t src, uint32_t c, uint32_t* out) {
    const auto sel = [src](uint32_t k) { return (src >> (16 + 2 * k)) & 3; };
    switch (op) {
    case 1: case 2: case 3: case 4: case 5: case 10: case 11: case 12: case 13: case 18: case 19: case 34: case 35: case 0x2E:
        out[0] = sel(c);                                    // component-wise: mov add sub mad mul min max slt sge lrp frc
        return 1;                                           // sgn abs mova
    case 6: case 7: case 14: case 15: case 32: case 37:
        out[0] = sel(0);                                    // scalar, replicate swizzle: rcp rsq exp log pow sincos
        return 1;
    case 8: case 33: case 0x24:
        for (uint32_t k = 0; k < 3; ++k) out[k] = sel(k);   // dp3 crs nrm
        return 3;
    case 9:
        for (uint32_t k = 0; k < 4; ++k) out[k] = sel(k);   // dp4
        return 4;
    default:
        for (uint32_t k = 0; k < 4; ++k) out[k] = k;        // anything else: all of it (the safe side)
        return 4;
    }
}

// the output register carrying the world normal, or -1 (none, more than one, or loops / calls / indexed outputs, which
// this doesn't follow). What each component depends on is a mask of D3DDECLUSAGE bits (bit 31: an undeclared input).
int normal_output(uint32_t* code) {
    if ((code[0] & 0xFFFF0000u) != 0xFFFE0000u || (code[0] & 0xFFFF) < 0x0300) return -1;   // vs_3_0 only
    std::vector<Dcl> d;
    std::vector<std::pair<uint32_t*, uint32_t>> ins;
    if (!walk(code, d, [&](uint32_t* p, uint32_t len) { ins.push_back({ p, len }); })) return -1;
    uint32_t in_use[16];
    for (auto& u : in_use) u = 1u << 31;
    bool has_normal = false;
    for (auto& x : d)
        if (x.type == kInput && x.num < 16) { in_use[x.num] = 1u << x.use; has_normal |= x.use == kNormalUsage; }
    if (!has_normal) return -1;
    uint32_t temp[32][4] = {}, out[kOutputs][4] = {};
    int depth = 0;
    for (auto& [p, len] : ins) {
        const uint32_t op = p[0] & 0xFFFF;
        switch (op) {
        case 0x28: case 0x29: ++depth; continue;            // if, ifc: writes inside add to what was there
        case 0x2A: continue;                                // else
        case 0x2B: --depth; continue;                       // endif
        case 0: case 0x2F: case 0x30: case 0x51: continue;  // nop defb defi def
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x26: case 0x27: case 0x2C: case 0x2D:
        case 0x61:
            return -1;                                      // call callnz loop ret endloop label rep endrep break breakc breakp
        }
        if (len < 1) continue;
        const uint32_t dst = p[1], mask = (dst >> 16) & 0xF;
        if (dst & 0x2000) return -1;                        // indexed destination
        const bool merge = depth > 0 || (p[0] & 0x10000000);   // conditional or predicated write
        uint32_t res[4] = {};
        for (uint32_t k = 2; k <= len; k += (p[k] & 0x2000) ? 2 : 1) {   // an indexed source is followed by its index
            const uint32_t s = p[k], t = reg_type(s), n = reg_num(s);
            for (uint32_t c = 0; c < 4; ++c) {
                if (!(mask >> c & 1)) continue;
                uint32_t cs[4];
                const uint32_t m = read_comps(op, s, c, cs);
                for (uint32_t i = 0; i < m; ++i) {
                    if (t == kInput && n < 16) res[c] |= in_use[n];
                    else if (t == 0 && n < 32) res[c] |= temp[n][cs[i]];
                }
            }
        }
        const uint32_t t = reg_type(dst), n = reg_num(dst);
        for (uint32_t c = 0; c < 4; ++c) {
            if (!(mask >> c & 1)) continue;
            if (t == 0 && n < 32) temp[n][c] = merge ? temp[n][c] | res[c] : res[c];
            else if (t == kOutput && n < kOutputs) out[n][c] |= res[c];
        }
    }
    int found = -1;
    for (uint32_t o = 0; o < kOutputs; ++o) {
        bool ok = true;
        for (uint32_t c = 0; c < 3; ++c)
            ok = ok && (out[o][c] & kUseNormal) && !(out[o][c] & (kUsePosition | kUseTangent | kUseBinormal));
        if (!ok) continue;
        if (found >= 0) return -1;                          // two: don't guess
        found = static_cast<int>(o);
    }
    return found;
}

// a copy of the shader with a NORMAL0 output in the first free output register, written by a copy (.xyz) of every
// instruction writing output `from`, right after it (so it reads the same values); empty when no register is free
std::vector<uint32_t> add_normal_output(uint32_t* code, uint32_t from) {
    std::vector<Dcl> d;
    if (!walk(code, d, [](uint32_t*, uint32_t) {})) return {};
    uint32_t used = 0;
    for (auto& x : d) if (x.type == kOutput && x.num < 32) used |= 1u << x.num;
    uint32_t k = 0;
    while (k < kOutputs && (used >> k & 1)) ++k;
    if (k == kOutputs || !(used >> from & 1)) return {};
    const auto retarget = [k](uint32_t tok, uint32_t mask) { return (tok & ~0x000F07FFu) | k | (mask << 16); };
    std::vector<uint32_t> out;
    const uint32_t* p = code;
    out.push_back(*p++);                                    // version
    for (int guard = 0; guard < 65536; ++guard) {
        const uint32_t t = *p;
        if (t == kEnd) { out.push_back(t); return out; }
        const uint32_t op = t & 0xFFFF;
        const uint32_t n = op == kComment ? 1 + ((t >> 16) & 0x7FFF) : 1 + ((t >> 24) & 0xF);
        out.insert(out.end(), p, p + n);
        if (op == kDcl && n >= 3 && reg_type(p[2]) == kOutput && reg_num(p[2]) == from) {
            out.push_back(t);
            out.push_back(usage_token(kNormalUsage, 0));
            out.push_back(retarget(p[2], 7));
        } else if (op != kComment && op != kDcl && n >= 2 && reg_type(p[1]) == kOutput && reg_num(p[1]) == from &&
                   ((p[1] >> 16) & 7)) {
            const size_t at = out.size();
            out.insert(out.end(), p, p + n);
            out[at + 1] = retarget(p[1], (p[1] >> 16) & 7);
        }
        p += n;
    }
    return {};
}
} // namespace sm3

std::unordered_map<const void*, int> g_vs_mode;      // vertex shader bytecode -> its relabelling
std::unordered_set<const void*> g_ps_done;            // pixel shader bytecode already relabelled
uint32_t g_relabelled = 0;
std::deque<std::vector<uint32_t>> g_vs_copies;        // vertex shaders with a NORMAL0 output (the game creates from these)
uint32_t g_normals = 0;

// pair = the pass's {pixel shader object, vertex shader object}, each {IDirect3D*Shader9* handle, bytecode}
void __stdcall patch_pair(uint32_t* pair) {
    if (!settings.on || !settings.relabel || !pair) return;
    auto* ps = reinterpret_cast<uint32_t*>(pair[0]);
    auto* vs = reinterpret_cast<uint32_t*>(pair[1]);
    if (!vs || !vs[1]) return;
    auto* vcode = reinterpret_cast<uint32_t*>(vs[1]);
    int mode;
    auto it = g_vs_mode.find(vcode);
    if (it != g_vs_mode.end()) mode = it->second;
    else {
        mode = vs[0] ? 0 : sm3::vertex_mode(vcode);   // already created without us: leave its pixel shaders alone too
        // effects (passes 6-8) drawn after the injection are the game's own raster, not traced: relabelling them only risks
        // a pixel shader shared with a vertex shader the rule missed (black HUD / effect sheets); flat water (mode 3, pass 5)
        // is still traced
        if ((mode & 0xF) != 3 && settings.inject_pass > 0 && settings.inject_pass <= 6) mode = 0;
        if (mode) { sm3::relabel(vcode, sm3::kOutput, mode); ++g_relabelled; }
        g_vs_mode[vcode] = mode;
        // a NORMAL0 output (see "Normals" above): the game creates the shader from the copy
        const int nout = vs[0] ? -1 : sm3::normal_output(vcode);
        if (nout >= 0) {
            auto copy = sm3::add_normal_output(vcode, static_cast<uint32_t>(nout));
            if (!copy.empty()) {
                g_vs_copies.push_back(std::move(copy));
                uint32_t* ncode = g_vs_copies.back().data();
                vs[1] = reinterpret_cast<uint32_t>(ncode);
                g_vs_mode[ncode] = mode;
                ++g_normals;
            }
        }
    }
    if (!mode || !ps || ps[0] || !ps[1]) return;
    auto* pcode = reinterpret_cast<uint32_t*>(ps[1]);
    if (g_ps_done.insert(pcode).second) sm3::relabel(pcode, sm3::kInput, mode);
}

uintptr_t jmp_ret_shader = 0;
naked void detour_shader() {
    _asm {
        pushfd
        pushad
        push eax                     // the pass's shader pair
        call patch_pair              // __stdcall
        popad
        popfd
        mov ebx, [eax + 4]
        test ebx, ebx
        mov edi, [eax]
        jmp dword ptr [jmp_ret_shader]
    }
}

// ---- the frame's camera ----
// The view (sub_8E6830, a view's setup) keeps its matrices double-buffered per frame: block = view + parity * 0x40, gXfView
// at +0x270, gXfProj +0x2F0, gXfViewProj +0x370 (the parameter table gets pointers to them). The render thread draws a
// frame behind the game thread, so sCamera's matrices are the next frame's: geometry unprojected with them was a frame off
// (the player lagged behind). When the main scene uploads gXfViewProj (parameter index at 0xE55620) its pointer gives the
// View and Proj of the same block. The block may hold them transposed for the shaders: decided once against sCamera.
constexpr uintptr_t kViewProjParam = 0x00E55620;
uintptr_t g_last_vp = 0;
int       g_vp_transposed = -1;            // -1 not decided yet
std::atomic<uint32_t> g_vp_sets{ 0 };
uint32_t  g_vp_sets_last = 0;

void load_matrix(D3DMATRIX& m, const float* src, bool transposed) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) m.m[r][c] = transposed ? src[c * 4 + r] : src[r * 4 + c];
}

// the camera position of a row-vector view matrix (world -> view): -t * R^T
void view_position(const D3DMATRIX& v, float* out) {
    for (int i = 0; i < 3; ++i) out[i] = -(v.m[3][0] * v.m[i][0] + v.m[3][1] * v.m[i][1] + v.m[3][2] * v.m[i][2]);
}
std::atomic<uint32_t> g_vp_rejected{ 0 };
uint32_t g_vp_rejected_last = 0;

// The main view: the view block the most draws used last frame (blocks alternate between two buffers 0x40 apart, so this
// frame's is that pointer or its twin). Not sCamera's viewport 0: fixed-camera rooms draw through another camera, and
// every upload was turned away (the room went black). Orthographic (2D) views never count.
struct VpCount { uintptr_t vp; uint32_t n; };
VpCount   g_vpc[8];
int       g_nvpc = 0;
uintptr_t g_main_vp = 0;

bool same_view(uintptr_t a, uintptr_t b) { return a == b || a + 0x40 == b || b + 0x40 == a; }

void count_view(uintptr_t vp) {
    for (int i = 0; i < g_nvpc; ++i) if (same_view(g_vpc[i].vp, vp)) { ++g_vpc[i].n; g_vpc[i].vp = vp; return; }
    if (g_nvpc < 8) g_vpc[g_nvpc++] = { vp, 1 };
}

void pick_main_view() {   // at each dispatch's start: last frame's most-drawn view
    uint32_t best = 0;
    for (int i = 0; i < g_nvpc; ++i) if (g_vpc[i].n > best) { best = g_vpc[i].n; g_main_vp = g_vpc[i].vp; }
    g_nvpc = 0;
}

void __stdcall vs_array(IDirect3DDevice9* dev, const uint32_t* entry, const uintptr_t* value) {
    if (!settings.on || !g_in_main || !dev) return;
    if ((*entry & 0xFFFF) != *reinterpret_cast<const uint32_t*>(kViewProjParam)) return;
    const uintptr_t vp = *value;
    if (!vp) return;
    const float* view = reinterpret_cast<const float*>(vp - 0x100);
    const float* proj = reinterpret_cast<const float*>(vp - 0x80);
    if (proj[15] == 1.0f) { if (vp != g_last_vp) { g_last_vp = vp; ++g_vp_rejected; } return; }   // orthographic: 2D
    count_view(vp);
    if (vp == g_last_vp) return;
    if (g_vp_transposed < 0) {
        // stored as D3D rows, ViewProj = View * Proj; stored transposed (for the shaders' dp4), ViewProj = Proj * View
        const float* m = reinterpret_cast<const float*>(vp);
        float e0 = 0, e1 = 0;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) {
                float s0 = 0, s1 = 0;
                for (int k = 0; k < 4; ++k) {
                    s0 += view[r * 4 + k] * proj[k * 4 + c];
                    s1 += proj[r * 4 + k] * view[k * 4 + c];
                }
                e0 += std::fabs(s0 - m[r * 4 + c]);
                e1 += std::fabs(s1 - m[r * 4 + c]);
            }
        g_vp_transposed = e1 < e0 ? 1 : 0;
    }
    g_last_vp = vp;
    if (g_main_vp && !same_view(vp, g_main_vp)) { ++g_vp_rejected; return; }   // another view (the sky's, ...)
    D3DMATRIX v, p;
    load_matrix(v, view, g_vp_transposed == 1);
    load_matrix(p, proj, g_vp_transposed == 1);
    dev->SetTransform(D3DTS_VIEW, &v);
    dev->SetTransform(D3DTS_PROJECTION, &p);
    g_main_v = v;
    g_main_p = p;
    g_have_main = true;
    ++g_vp_sets;
}

uintptr_t jmp_ret_vsarr = 0;
naked void detour_vsarr() {
    _asm {
        pushad
        push edi                     // the binding's value (a pointer to the matrix)
        push ebx                     // the binding
        push esi                     // the device
        call vs_array                // __stdcall
        popad
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        jmp dword ptr [jmp_ret_vsarr]
    }
}

uintptr_t jmp_ret_tex = 0;
uintptr_t kTexBoundAddr = kTexBound;
volatile uint8_t g_remap = 0;   // the bridge's albedo move is on and the main scene is drawing: the detour's fast check

naked void detour_tex() {
    _asm {
        cmp byte ptr [g_remap], 0
        je original
        pushad
        lea eax, [esp + 0x20 + 0x1A0]   // the game's per-parameter cache (esp + 0x1A0 before pushad)
        push eax
        push ecx                        // the pass record
        push edi                        // the entry's resource
        push ebx                        // the entry
        push esi                        // the device
        call bind_texture               // __stdcall
        test eax, eax
        jnz handled
        popad
    original:
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        jmp dword ptr [jmp_ret_tex]
    handled:
        popad
        jmp dword ptr [kTexBoundAddr]
    }
}

void* vsarr_detour() { return reinterpret_cast<void*>(&detour_vsarr); }
std::unique_ptr<FunctionHook>& vsarr_hook() { static std::unique_ptr<FunctionHook> h; return h; }

void* shader_detour() { return reinterpret_cast<void*>(&detour_shader); }
std::unique_ptr<FunctionHook>& shader_hook() { static std::unique_ptr<FunctionHook> h; return h; }

void* tex_detour() { return reinterpret_cast<void*>(&detour_tex); }

void set_sun(const float* dir) {
    for (int i = 0; i < 3; ++i) g_sun_dir[i] = dir[i];
    ++g_presents;
    if (g_disp_since_present > g_disp_per_present_max) g_disp_per_present_max = g_disp_since_present;
    g_disp_since_present = 0;
}

void on_reset() {
    if (g_hide) { g_hide->Release(); g_hide = nullptr; }
    if (g_lin_copy) { g_lin_copy->Release(); g_lin_copy = nullptr; }
}

bool loaded() {
    static int cached = -1;
    if (cached < 0) {
        cached = 0;
        HMODULE d3d9 = GetModuleHandleA("d3d9.dll");
        // the bridge client and the runtime both export the Remix API's entry point
        if (d3d9 && (GetProcAddress(d3d9, "remixapi_InitializeLibrary") || GetProcAddress(d3d9, "remixapi_initializeLibrary")))
            cached = 1;
        if (!cached && d3d9) {
            char path[MAX_PATH] = {};
            GetModuleFileNameA(d3d9, path, MAX_PATH);
            std::string p(path);
            for (auto& c : p) c = (char)tolower((unsigned char)c);
            if (p.find("system32") == std::string::npos && p.find("syswow64") == std::string::npos) cached = 1;
        }
    }
    return cached == 1;
}

bool on_tag(void* device, const uint32_t* list, uint32_t index, uint32_t count) {
    if (!settings.on) { g_remap = 0; return false; }
    auto* dev = static_cast<IDirect3DDevice9*>(device);
    const uint32_t key = list[index * 2 + 1];
    const uint32_t scene = (key >> 24) & 0x1F, pass = (key >> 20) & 0xF;
    const bool main = scene == kMainScene;
    if (index == 0) {
        g_skipped_last = g_skipped.exchange(0);
        g_seen_last = g_seen.exchange(0);
        g_moved_last = g_moved.exchange(0);
        g_water_last = g_water.exchange(0);
        for (int i = 0; i < kWaterCount; ++i)
            if (g_water_param[i] < 0) g_water_param[i] = param_of(kWaterIds[i]);
        if (g_shadow_param < 0) g_shadow_param = param_of(kShadowNearId);
        if (g_prim_depth_param < 0) g_prim_depth_param = param_of(kPrimDepthId);
        if (g_water_zbuf_param < 0) g_water_zbuf_param = param_of(kWaterZBufferId);
        if (g_prim_scene_param < 0) g_prim_scene_param = param_of(kPrimSceneId);
        if (g_base_param < 0) g_base_param = param_of(kBaseSamplerId);
        g_sky_last = g_sky.exchange(0);
        g_hidden_last = g_hidden.exchange(0);
        for (int i = 0; i < 3; ++i) {
            if (g_prim_param[i] < 0) g_prim_param[i] = param_of(kPrimBaseIds[i]);
        }
        ++g_frames;
        g_in_main = false;
        g_last_vp = 0;
        pick_main_view();
        g_vp_sets_last = g_vp_sets.exchange(0);
        g_vp_rejected_last = g_vp_rejected.exchange(0);
        ++g_disp_since_present;
        g_ui_done = false;
        g_solid_seen = false;
        if (g_frames % 3600 == 1 && g_norder > 0) {
            FILE* f = nullptr;
            if (fopen_s(&f, "remix_passes.txt", "a") == 0 && f) {
                fprintf(f, "tag order (view.scene.pass x tags):");
                for (int i = 0; i < g_norder; ++i)
                    fprintf(f, " %u.%02X.%u%s{rt %04X %ux%u f%u}", (g_order[i] >> 9) & 7, (g_order[i] >> 4) & 0x1F,
                            g_order[i] & 0xF, (g_order[i] & 0x8000) ? "[bb]" : "", unsigned(g_order_rt[i] & 0xFFFF),
                            g_order_desc[i] & 0xFFF, (g_order_desc[i] >> 12) & 0xFFF, g_order_desc[i] >> 24);
                fprintf(f, " | presents %u, most dispatches between two presents %u, ui trigger at pass %u rt %04X",
                        g_presents.load(), g_disp_per_present_max, g_ui_fired_pass, unsigned(g_ui_fired_rt & 0xFFFF));
                fprintf(f, "\n");
                fclose(f);
            }
        }
        g_norder = 0;
        if (settings.game_lights) gather_lights();
        set_transforms(dev);
    }
    ++g_seen;
    if (main && !g_in_main) {           // the main scene starts: its camera again (another view may have set its own)
        g_in_main = true;
        g_last_vp = 0;
        set_transforms(dev);
    } else if (!main) {
        g_in_main = false;
    }
    // after the injection the game's own shaders draw: no stage moves, stand-ins or hidden draws there
    const bool after_inject = settings.inject_pass > 0 && pass >= static_cast<uint32_t>(settings.inject_pass);
    g_remap = (main && settings.albedo_to_stage0 && !after_inject) ? 1 : 0;
    if (g_sky_vp && (pass != g_pass || !main)) sky_viewport(dev, false);   // the sky stays within SOLID
    g_pass = pass;
    bool skip = false;
    if (!main && settings.skip_other_scenes) skip = true;
    if (main && pass == kZPass && settings.skip_zpass) skip = true;
    // the filters tone-map the HDR scene buffer into the target the HUD and the end of the frame draw on: needed once the
    // traced image goes into that buffer
    if (main && pass == kFilter && settings.skip_filters && !after_inject) skip = true;
    if (main && (pass == kScreen || pass == kEnd) && settings.skip_hud) skip = true;
    if (g_frames % 3600 == 0) {
        const uint32_t k = ((key >> 29) << 9) | (scene << 4) | pass;
        if (g_norder == 0 || (g_order[g_norder - 1] & 0x7FFF) != k) {
            uint32_t bb = 0, desc = 0;
            uintptr_t rtp = 0;
            IDirect3DSurface9 *rt = nullptr, *back = nullptr;
            if (SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) {
                rtp = reinterpret_cast<uintptr_t>(rt);
                D3DSURFACE_DESC d{};
                if (SUCCEEDED(rt->GetDesc(&d))) desc = (d.Width & 0xFFF) | ((d.Height & 0xFFF) << 12) | ((uint32_t(d.Format) & 0xFF) << 24);
                if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) && back) {
                    bb = rt == back ? 0x8000u : 0;
                    back->Release();
                }
                rt->Release();
            }
            if (g_norder < 256) { g_order_rt[g_norder] = rtp; g_order_desc[g_norder] = desc; g_order[g_norder++] = k | bb; }
        }
    }
    if (main && pass == 2) g_solid_seen = true;
    if (main && !skip && after_inject && !g_ui_done && g_solid_seen) {
        g_ui_done = true;
        water_transform(dev, false);
        sky_viewport(dev, false);
        g_ui_fired_pass = pass;
        IDirect3DSurface9* rt = nullptr;
        if (SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) { g_ui_fired_rt = reinterpret_cast<uintptr_t>(rt); rt->Release(); }
        trigger_ui(dev);
        linearize(dev);
    }
    if (skip) ++g_skipped;
    return skip;
}

void draw_gui() {
    ImGui::Checkbox(_("RTX Remix bridge"), &settings.on);
    ImGui::SameLine();
    Mod::help_marker(_("For playing through NVIDIA RTX Remix (its d3d9.dll in the game folder): gives Remix the game's camera "
                       "every frame, which DMC4's shaders keep to themselves, and leaves out the draws Remix would show as "
                       "duplicates or garbage. Without Remix it does nothing useful: leave it off."));
    if (!settings.on) return;
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID("PostFX_Remix");
    ImGui::TextDisabled(loaded() ? _("Remix's d3d9.dll is loaded") : _("The system's d3d9.dll is loaded (no Remix)"));
    ImGui::Checkbox(_("Skip other views (shadow maps, reflections)"), &settings.skip_other_scenes);
    ImGui::Checkbox(_("Skip the depth prepass"), &settings.skip_zpass);
    ImGui::Checkbox(_("Hide occlusion tests (white diamonds at lights)"), &settings.hide_occlusion);
    ImGui::Checkbox(_("Hide shadow overlays"), &settings.hide_shadow_overlays);
    ImGui::Checkbox(_("Hide untextured glows"), &settings.hide_glows);
    ImGui::SameLine();
    Mod::help_marker(_("Some effect sprites (lamp halos) have no texture: the game shapes their glow in a shader Remix "
                       "doesn't run, so Remix showed them as white diamonds."));
    ImGui::Checkbox(_("Skip screen filters"), &settings.skip_filters);
    {
        static const int kPasses[] = { 0, 6, 7, 9, 10 };
        static const char* kNames[] = { "At present (Remix's default)", "Before transparent things (6)", "Before effects (7)",
                                        "Before screen filters (9)", "Before the HUD (10)" };
        int cur = 0;
        for (int i = 0; i < 5; ++i) if (kPasses[i] == settings.inject_pass) cur = i;
        if (ImGui::Combo(_("Hand the image to the game"), &cur, kNames, 5)) settings.inject_pass = kPasses[cur];
        ImGui::SameLine();
        Mod::help_marker(_("Where Remix's path-traced image goes into the game's frame. Everything the game draws after that "
                           "point uses its own shaders on top of it: refracting effects bend the traced image, the game's "
                           "filters and tone map apply, the HUD stays flat. At present (Remix's default) Remix traces "
                           "everything, the HUD included, and covers the game's image at the end."));
    }
    ImGui::Checkbox(_("Skip the HUD and menus"), &settings.skip_hud);
    ImGui::SameLine();
    Mod::help_marker(_("Remix traces the HUD as if it were in the world. Skipping it also hides the title screen and menus; "
                       "tagging the HUD's textures as UI in Remix's own menu (Alt+X) keeps them as a flat overlay instead."));
    ImGui::Checkbox(_("Albedo on stage 0"), &settings.albedo_to_stage0);
    ImGui::SameLine();
    Mod::help_marker(_("Remix takes each object's colour from texture stage 0; DMC4's shaders put the colour map on any "
                       "stage, so without this the floor shows its normal map. Moves the colour map there for Remix."));
    ImGui::Checkbox(_("Game lights"), &settings.game_lights);
    ImGui::SameLine();
    Mod::help_marker(_("Gives Remix the stage's own lights (its sun and fill lights, and point lights such as lamps and "
                       "muzzle flashes) as real lights. Remix can't see DMC4's lighting, which happens in its shaders."));
    if (settings.game_lights) {
        ImGui::SliderFloat(_("Directional strength"), &settings.dir_strength, 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat(_("Point light strength"), &settings.point_strength, 0.0f, 50.0f, "%.2f");
        ImGui::TextDisabled(_("%d directional, %d point lights (stage light group %08X)"), g_ndir, g_npoint, g_stage_group);
        ImGui::Checkbox(_("Ambient fill"), &settings.fill);
        ImGui::SameLine();
        Mod::help_marker(_("Remix has no ambient light and can't use DMC4's baked lighting, so interiors go dark. This turns the "
                           "stage's ambient colour into a soft light at the camera."));
        if (settings.fill) {
            ImGui::SliderFloat(_("Fill strength"), &settings.fill_strength, 0.0f, 20.0f, "%.2f");
            ImGui::SliderFloat(_("Fill range"), &settings.fill_range, 200.0f, 20000.0f, "%.0f cm", ImGuiSliderFlags_Logarithmic);
            ImGui::TextDisabled(_("Ambient colour %.2f %.2f %.2f"), g_hemi[0], g_hemi[1], g_hemi[2]);
        }
    }
    ImGui::Checkbox(_("Sun (without game lights)"), &settings.sun);
    ImGui::SameLine();
    Mod::help_marker(_("Gives Remix the stage's sun (the light its shadows come from) as a real light. Remix can't see "
                       "DMC4's own lighting, which happens in its shaders."));
    if (settings.sun) {
        ImGui::SliderFloat(_("Sun strength"), &settings.sun_strength, 0.0f, 20.0f, "%.2f");
        ImGui::ColorEdit3(_("Sun colour"), settings.sun_color);
    }
    ImGui::Checkbox(_("Water stand-in"), &settings.water_standin);
    ImGui::SameLine();
    Mod::help_marker(_("DMC4's water has no colour map, so Remix showed its wave maps (teal / magenta). This puts one "
                       "solid colour there instead; every water surface then shares one texture, which a Remix material "
                       "replacement can turn into real water."));
    if (settings.water_standin) {
        ImGui::ColorEdit3(_("Water colour"), settings.water_color);
        ImGui::SliderFloat2(_("Water flow"), settings.water_flow, -0.2f, 0.2f, "%.3f");
        ImGui::SliderFloat(_("Water tiling"), &settings.water_tiling, 0.25f, 64.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SameLine();
        Mod::help_marker(_("How many times the wave pattern repeats across the water: higher = smaller waves."));
    }
    if (settings.inject_pass > 0) {
        ImGui::Checkbox(_("Linear image for the game"), &settings.linearize);
        ImGui::SameLine();
        Mod::help_marker(_("Remix hands over a tone-mapped, display-encoded image; the game then tone-maps it again, which blows "
                           "out bright areas. This turns it back into a linear HDR image first."));
        if (settings.linearize) {
            ImGui::SliderFloat(_("Expand highlights"), &settings.expand, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat(_("Exposure"), &settings.exposure, 0.1f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        }
    }
    ImGui::Checkbox(_("Patch shaders for Remix (restart)"), &settings.relabel);
    ImGui::SameLine();
    Mod::help_marker(_("Swaps texture-coordinate labels in effect and flat-water shaders so Remix finds their UVs and colours, "
                       "and gives material shaders a normal output so Remix shades with the game's world-space normals "
                       "(needs rtx.useVertexCapturedNormals = True in rtx.conf). Applied as the game creates its shaders: "
                       "change it, save the config, restart."));
    ImGui::Checkbox(_("Sky as sky"), &settings.sky);
    ImGui::SameLine();
    Mod::help_marker(_("Tells Remix which draws are the sky, so it draws them as the background and lights the scene with "
                       "them, instead of treating the sky panels as walls that cast shadows."));
    ImGui::Checkbox(_("Hide refraction effects"), &settings.hide_refraction);
    ImGui::TextDisabled(_("Hide water (instead of the stand-in):"));
    ImGui::Checkbox(_("Sea type (depth-faded)"), &settings.hide_water_depth);
    ImGui::SameLine();
    ImGui::Checkbox(_("Normal map only"), &settings.hide_water_simple);
    ImGui::SameLine();
    ImGui::Checkbox(_("Flat (BP floor)"), &settings.hide_water_flat);
    ImGui::SameLine();
    Mod::help_marker(_("Heat haze and refracting sword trails bend the screen image in a shader; Remix only sees their "
                       "distortion texture, which shows as a purple colour."));
    ImGui::TextDisabled(_("Last frame: %u of %u draws skipped, %u textures moved, %u water passes"), g_skipped_last,
                        g_seen_last, g_moved_last, g_water_last);
    ImGui::TextDisabled(_("Shaders patched (needs the bridge on at startup): %u relabelled, %u with a normal output; "
                          "%u draws hidden, %u sky"), g_relabelled, g_normals, g_hidden_last, g_sky_last);
    ImGui::TextDisabled(_("Camera from the drawn frame: %u updates, %u other views passed over (%s layout)"), g_vp_sets_last,
                        g_vp_rejected_last, g_vp_transposed < 0 ? "?" : g_vp_transposed ? "transposed" : "row");
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
}

void load(const utility::Config& cfg) {
    settings.on = cfg.get<bool>("remix_bridge").value_or(false);
    settings.skip_other_scenes = cfg.get<bool>("remix_skip_other_scenes").value_or(true);
    settings.skip_zpass = cfg.get<bool>("remix_skip_zpass").value_or(true);
    settings.skip_filters = cfg.get<bool>("remix_skip_filters").value_or(true);
    settings.hide_occlusion = cfg.get<bool>("remix_hide_occlusion").value_or(true);
    settings.hide_shadow_overlays = cfg.get<bool>("remix_hide_shadow_overlays").value_or(true);
    settings.hide_glows = cfg.get<bool>("remix_hide_glows").value_or(true);
    settings.hide_refraction = cfg.get<bool>("remix_hide_refraction").value_or(true);
    settings.hide_water_depth = cfg.get<bool>("remix_hide_water_depth").value_or(false);
    settings.hide_water_simple = cfg.get<bool>("remix_hide_water_simple").value_or(false);
    settings.hide_water_flat = cfg.get<bool>("remix_hide_water_flat").value_or(false);
    settings.water_tiling = cfg.get<float>("remix_water_tiling").value_or(4.0f);
    for (int i = 0; i < 2; ++i)
        settings.water_flow[i] = cfg.get<float>("remix_water_flow_" + std::to_string(i)).value_or(settings.water_flow[i]);
    settings.skip_hud = cfg.get<bool>("remix_skip_hud").value_or(false);
    settings.inject_pass = cfg.get<int>("remix_inject_pass").value_or(6);
    settings.sky = cfg.get<bool>("remix_sky").value_or(true);
    settings.relabel = cfg.get<bool>("remix_relabel").value_or(true);
    settings.linearize = cfg.get<bool>("remix_linearize").value_or(true);
    settings.expand = cfg.get<float>("remix_expand").value_or(0.0f);
    settings.exposure = cfg.get<float>("remix_exposure").value_or(1.0f);
    settings.albedo_to_stage0 = cfg.get<bool>("remix_albedo_stage0").value_or(true);
    settings.sun = cfg.get<bool>("remix_sun").value_or(true);
    settings.game_lights = cfg.get<bool>("remix_game_lights").value_or(true);
    settings.dir_strength = cfg.get<float>("remix_dir_strength").value_or(1.0f);
    settings.point_strength = cfg.get<float>("remix_point_strength").value_or(1.0f);
    settings.fill = cfg.get<bool>("remix_fill").value_or(true);
    settings.fill_strength = cfg.get<float>("remix_fill_strength").value_or(1.0f);
    settings.fill_range = cfg.get<float>("remix_fill_range").value_or(4000.0f);
    settings.sun_strength = cfg.get<float>("remix_sun_strength").value_or(2.0f);
    settings.water_standin = cfg.get<bool>("remix_water_standin").value_or(true);
    for (int i = 0; i < 3; ++i) {
        settings.sun_color[i] = cfg.get<float>("remix_sun_color_" + std::to_string(i)).value_or(settings.sun_color[i]);
        settings.water_color[i] = cfg.get<float>("remix_water_color_" + std::to_string(i)).value_or(settings.water_color[i]);
    }
}

void save(utility::Config& cfg) {
    cfg.set<bool>("remix_bridge", settings.on);
    cfg.set<bool>("remix_skip_other_scenes", settings.skip_other_scenes);
    cfg.set<bool>("remix_skip_zpass", settings.skip_zpass);
    cfg.set<bool>("remix_skip_filters", settings.skip_filters);
    cfg.set<bool>("remix_hide_occlusion", settings.hide_occlusion);
    cfg.set<bool>("remix_hide_shadow_overlays", settings.hide_shadow_overlays);
    cfg.set<bool>("remix_hide_glows", settings.hide_glows);
    cfg.set<bool>("remix_hide_refraction", settings.hide_refraction);
    cfg.set<bool>("remix_hide_water_depth", settings.hide_water_depth);
    cfg.set<bool>("remix_hide_water_simple", settings.hide_water_simple);
    cfg.set<bool>("remix_hide_water_flat", settings.hide_water_flat);
    cfg.set<float>("remix_water_tiling", settings.water_tiling);
    for (int i = 0; i < 2; ++i) cfg.set<float>("remix_water_flow_" + std::to_string(i), settings.water_flow[i]);
    cfg.set<bool>("remix_skip_hud", settings.skip_hud);
    cfg.set<int>("remix_inject_pass", settings.inject_pass);
    cfg.set<bool>("remix_sky", settings.sky);
    cfg.set<bool>("remix_relabel", settings.relabel);
    cfg.set<bool>("remix_linearize", settings.linearize);
    cfg.set<float>("remix_expand", settings.expand);
    cfg.set<float>("remix_exposure", settings.exposure);
    cfg.set<bool>("remix_albedo_stage0", settings.albedo_to_stage0);
    cfg.set<bool>("remix_sun", settings.sun);
    cfg.set<bool>("remix_game_lights", settings.game_lights);
    cfg.set<float>("remix_dir_strength", settings.dir_strength);
    cfg.set<float>("remix_point_strength", settings.point_strength);
    cfg.set<bool>("remix_fill", settings.fill);
    cfg.set<float>("remix_fill_strength", settings.fill_strength);
    cfg.set<float>("remix_fill_range", settings.fill_range);
    cfg.set<float>("remix_sun_strength", settings.sun_strength);
    cfg.set<bool>("remix_water_standin", settings.water_standin);
    for (int i = 0; i < 3; ++i) {
        cfg.set<float>("remix_sun_color_" + std::to_string(i), settings.sun_color[i]);
        cfg.set<float>("remix_water_color_" + std::to_string(i), settings.water_color[i]);
    }
}

} // namespace remix
