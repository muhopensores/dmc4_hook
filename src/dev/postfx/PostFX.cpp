#include "PostFX.hpp"
#include "../../sdk/Selection.hpp"
#include "Cel.hpp"
#include "Tags.hpp"
#include "TexFilter.hpp"
#include "Remix.hpp"
#include "../shadows/Shadows.hpp"
#include "../../ModFramework.hpp"
#include "../shader_file.hpp"
#include "../../sdk/Cam.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <chrono>
#include <fstream>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

namespace post_fx {

Settings settings;

namespace {

constexpr uintptr_t kRenderInstance = 0x00E552D8;  // sRender::mpInstance
constexpr uintptr_t kCameraInstance = 0x00E552D4;  // sCamera::mpInstance
constexpr uintptr_t kDevice         = 0x34;        // sRender: IDirect3DDevice9*
constexpr uintptr_t kDepthTexture   = 0x1700C;     // sRender: the frame's depth texture (a cTrans texture)
constexpr uintptr_t kTexD3D         = 0x10;        // cTrans texture: IDirect3DTexture9*
constexpr uint32_t  kMainScene      = 0x1F;        // tag key bits 24-28 of the main scene (sub-scenes count down from 31)
constexpr uint32_t  kPassTransparent = 6;          // tag key bits 20-23: BEGIN 0, ZPASS 1, SOLID 2 ... OVERLAP 5, TRANSPARENT 6
constexpr char kSourcePath[] = "nativePC\\shader\\postfx\\postfx.hlsl";

const char kEmbedded[] =
#include "postfx.inc"
    ;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// d3dcompiler_47's D3DCompile, loaded at run time (as PbrMaterials / ToneMap)
struct Blob : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};
using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const void*, void*, const char*, const char*, UINT,
                                      UINT, Blob**, Blob**);
constexpr UINT kOptimize3 = 1u << 15;

D3DCompileFn compiler() {
    static D3DCompileFn fn = [] {
        HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
        return dll ? (D3DCompileFn)GetProcAddress(dll, "D3DCompile") : nullptr;
    }();
    return fn;
}

// ---- compile (worker thread) -> shaders (render thread) ----

std::mutex             g_mutex;
std::vector<uint8_t>   g_code_ao, g_code_blur, g_code_outline, g_code_contact, g_code_fxaa, g_code_highlight;   // guarded by g_mutex
std::string            g_error;                  // guarded by g_mutex
int                    g_code_generation = 0;    // guarded by g_mutex: bumped by each finished compile
bool                   g_from_file = false;      // guarded by g_mutex
std::atomic<bool>      g_want_compile{ true };
std::atomic<bool>      g_compiling{ false };

std::string read_source(bool& from_file) {
    std::ifstream f(kSourcePath, std::ios::binary);
    if (f) {
        std::stringstream ss;
        ss << f.rdbuf();
        std::string s = ss.str();
        if (!s.empty()) { from_file = true; return s; }
    }
    from_file = false;
    return kEmbedded;
}

bool compile_one(D3DCompileFn fn, const std::string& src, const char* entry, std::vector<uint8_t>& out, std::string& error) {
    Blob* code = nullptr;
    Blob* errors = nullptr;
    const HRESULT hr = fn(src.data(), src.size(), "postfx.hlsl", nullptr, nullptr, entry, "ps_3_0", kOptimize3, 0, &code, &errors);
    if (errors) {
        if (FAILED(hr)) error = std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
        errors->Release();
    }
    if (FAILED(hr) || !code) {
        if (error.empty()) error = std::string("D3DCompile failed for ") + entry;
        return false;
    }
    const auto* p = static_cast<const uint8_t*>(code->GetBufferPointer());
    out.assign(p, p + code->GetBufferSize());
    code->Release();
    return true;
}

void start_compile() {
    D3DCompileFn fn = compiler();
    g_want_compile = false;
    if (!fn) {
        std::lock_guard lock(g_mutex);
        g_error = "d3dcompiler_47.dll not found";
        return;
    }
    g_compiling = true;
    std::thread([fn] {
        bool from_file = false;
        const std::string src = read_source(from_file);
        std::vector<uint8_t> ao, blur, outline, contact, fxaa, highlight;
        std::string error;
        const bool ok = compile_one(fn, src, "ps_ao", ao, error) && compile_one(fn, src, "ps_blur", blur, error) &&
                        compile_one(fn, src, "ps_outline", outline, error) && compile_one(fn, src, "ps_contact", contact, error) &&
                        compile_one(fn, src, "ps_fxaa", fxaa, error) && compile_one(fn, src, "ps_highlight", highlight, error);
        {
            std::lock_guard lock(g_mutex);
            g_from_file = from_file;
            g_error = ok ? std::string() : error;
            if (ok) {
                g_code_ao = std::move(ao);
                g_code_blur = std::move(blur);
                g_code_outline = std::move(outline);
                g_code_contact = std::move(contact);
                g_code_fxaa = std::move(fxaa);
                g_code_highlight = std::move(highlight);
                ++g_code_generation;
            }
        }
        if (!ok) spdlog::error("PostFX: shaders: {}", error);
        g_compiling = false;
    }).detach();
}

// ---- render thread state ----

IDirect3DPixelShader9* g_ps_ao = nullptr;
IDirect3DPixelShader9* g_ps_blur = nullptr;
IDirect3DPixelShader9* g_ps_outline = nullptr;
IDirect3DPixelShader9* g_ps_contact = nullptr;
IDirect3DPixelShader9* g_ps_fxaa = nullptr;
IDirect3DPixelShader9* g_ps_highlight = nullptr;
IDirect3DTexture9*     g_scene_copy = nullptr;  // D3DPOOL_DEFAULT: released on reset
UINT                   g_copy_w = 0, g_copy_h = 0;
D3DFORMAT              g_copy_fmt = D3DFMT_UNKNOWN;
float                  g_sun[3] = { 0.0f, -1.0f, 0.0f };   // the way the sun's light travels (game axes), from on_frame
int                    g_ps_generation = 0;
IDirect3DTexture9*     g_ao_target = nullptr;   // D3DPOOL_DEFAULT: released on reset
UINT                   g_ao_w = 0, g_ao_h = 0;
IDirect3DStateBlock9*  g_state = nullptr;       // released on reset

// the object picker's depth sample (sample_world)
struct PickRequest {
    int x = 0, y = 0;
    std::promise<DepthSample> done;
    std::shared_future<DepthSample> future;
};
std::mutex                   g_pick_mutex;
std::shared_ptr<PickRequest> g_pick;              // guarded by g_pick_mutex
std::atomic<bool>            g_pick_pending{ false };
IDirect3DTexture9*           g_pick_rt = nullptr;  // 1 x 1, D3DPOOL_DEFAULT: released on reset
IDirect3DSurface9*           g_pick_sys = nullptr; // 1 x 1 system memory copy
bool                   g_done = false;          // AO drawn in this dispatch

// Diagnostics, found in memory by their magic (the debug server's read; tests without the panel)
struct Diag {
    uint32_t magic = 0x44584650;   // 'PFXD'
    uint32_t tags = 0;             // tags seen
    uint32_t dispatches = 0;       // tag loops started (index 0)
    uint32_t main_tags = 0;        // tags of the main scene (sub-scene 0x1F)
    uint32_t triggers = 0;         // draw_ao calls
    uint32_t stage = 0;            // where the last draw_ao stopped (0 = drew)
    uint32_t last_key = 0;         // the key that triggered
    uint32_t max_pass_main = 0;    // the highest pass seen in the main scene
    float    proj[5] = {};         // P00 P11 P22 P32 P23 used
    uint32_t capture = 0;          // set 1: the next dispatch's key changes go into seq (then 2)
    uint32_t count = 0;            // that dispatch's tag count
    uint32_t nseq = 0;
    uint32_t seq[64] = {};         // (index << 12) | key >> 20 at each change of view / sub-scene / pass
};
Diag g_diag;
std::atomic<int>       g_frames{ 0 };
std::atomic<int>       g_last_w{ 0 }, g_last_h{ 0 };

template <class T> void release(T*& p) {
    if (p) { p->Release(); p = nullptr; }
}

void release_device_objects() {
    release(g_ao_target);
    release(g_scene_copy);
    release(g_pick_rt);
    release(g_pick_sys);
    release(g_state);
    g_ao_w = g_ao_h = 0;
}

bool ensure_shaders(IDirect3DDevice9* dev) {
    std::lock_guard lock(g_mutex);
    if (g_code_generation == g_ps_generation) return g_ps_ao && g_ps_blur && g_ps_outline && g_ps_contact && g_ps_fxaa &&
                                                       g_ps_highlight;
    release(g_ps_ao);
    release(g_ps_blur);
    release(g_ps_outline);
    release(g_ps_contact);
    release(g_ps_fxaa);
    release(g_ps_highlight);
    g_ps_generation = g_code_generation;
    if (g_code_ao.empty() || g_code_blur.empty() || g_code_outline.empty() || g_code_contact.empty() ||
        g_code_fxaa.empty() || g_code_highlight.empty())
        return false;
    if (FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_ao.data()), &g_ps_ao)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_blur.data()), &g_ps_blur)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_outline.data()), &g_ps_outline)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_contact.data()), &g_ps_contact)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_fxaa.data()), &g_ps_fxaa)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_highlight.data()), &g_ps_highlight))) {
        release(g_ps_ao);
        release(g_ps_blur);
        release(g_ps_outline);
        release(g_ps_contact);
        release(g_ps_fxaa);
        release(g_ps_highlight);
        g_error = "CreatePixelShader failed";
        return false;
    }
    return true;
}

bool ensure_target(IDirect3DDevice9* dev, UINT w, UINT h) {
    if (g_ao_target && g_ao_w == w && g_ao_h == h) return true;
    release(g_ao_target);
    if (FAILED(dev->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_ao_target, nullptr)))
        return false;
    g_ao_w = w;
    g_ao_h = h;
    return true;
}

// a texture to copy the scene into (its size and format), for FXAA
bool ensure_scene_copy(IDirect3DDevice9* dev, const D3DSURFACE_DESC& d) {
    if (g_scene_copy && g_copy_w == d.Width && g_copy_h == d.Height && g_copy_fmt == d.Format) return true;
    release(g_scene_copy);
    if (FAILED(dev->CreateTexture(d.Width, d.Height, 1, D3DUSAGE_RENDERTARGET, d.Format, D3DPOOL_DEFAULT, &g_scene_copy,
                                  nullptr)))
        return false;
    g_copy_w = d.Width;
    g_copy_h = d.Height;
    g_copy_fmt = d.Format;
    return true;
}

struct Vertex {
    float x, y, z, rhw;
    float u, v;
};

// one triangle over the whole target (pixel centres on texel centres: D3D9's half-pixel offset)
void draw_fullscreen(IDirect3DDevice9* dev, UINT w, UINT h) {
    const float fw = static_cast<float>(w), fh = static_cast<float>(h);
    const Vertex v[3] = {
        { -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
        { 2.0f * fw - 0.5f, -0.5f, 0.0f, 1.0f, 2.0f, 0.0f },
        { -0.5f, 2.0f * fh - 0.5f, 0.0f, 1.0f, 0.0f, 2.0f },
    };
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, v, sizeof(Vertex));
}

void set_point_clamp(IDirect3DDevice9* dev, DWORD stage, IDirect3DTexture9* tex) {
    dev->SetTexture(stage, tex);
    dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    dev->SetSamplerState(stage, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    dev->SetSamplerState(stage, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(stage, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    dev->SetSamplerState(stage, D3DSAMP_SRGBTEXTURE, FALSE);
}

// Debug: a render target's pixels to <game>\dmc4_hook_screenshots\<name>.raw (BGRA, rows top down; diag.capture 5 -> 6)
void dump_target(IDirect3DDevice9* dev, IDirect3DSurface9* rt, UINT w, UINT h, const char* name) {
    IDirect3DSurface9* sys = nullptr;
    if (FAILED(dev->CreateOffscreenPlainSurface(w, h, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &sys, nullptr))) return;
    D3DLOCKED_RECT lr;
    if (SUCCEEDED(dev->GetRenderTargetData(rt, sys)) && SUCCEEDED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) {
        std::ofstream f(std::string("dmc4_hook_screenshots\\") + name + ".raw", std::ios::binary);
        for (UINT y = 0; y < h; ++y) f.write(static_cast<const char*>(lr.pBits) + y * lr.Pitch, w * 4);
        sys->UnlockRect();
    }
    sys->Release();
}

// The object picker's depth sample: the requested pixel's depth texel (1 x 1 StretchRect, read back), decoded and turned
// into a world point with this frame's camera (viewport 0; view rows are row vectors, the view is rigid).
void service_pick(IDirect3DDevice9* dev, IDirect3DTexture9* depth, const D3DSURFACE_DESC& desc, const sCamera* cam) {
    std::shared_ptr<PickRequest> req;
    {
        std::lock_guard lock(g_pick_mutex);
        req = std::move(g_pick);
        g_pick.reset();
        g_pick_pending = false;
    }
    if (!req) return;
    DepthSample s;
    IDirect3DSurface9* bb = nullptr;
    IDirect3DSurface9* src = nullptr;
    IDirect3DSurface9* dst = nullptr;
    D3DSURFACE_DESC bd{};
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)) || FAILED(bb->GetDesc(&bd))) bd = desc;
    release(bb);
    const int tx = std::clamp((int)((long long)req->x * desc.Width / (std::max)(bd.Width, 1u)), 0, (int)desc.Width - 1);
    const int ty = std::clamp((int)((long long)req->y * desc.Height / (std::max)(bd.Height, 1u)), 0, (int)desc.Height - 1);
    s.texel[0] = tx;
    s.texel[1] = ty;
    if (!g_pick_rt && FAILED(dev->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_pick_rt,
                                                 nullptr)))
        s.error = "can't make the 1 x 1 target";
    if (s.error.empty() && !g_pick_sys &&
        FAILED(dev->CreateOffscreenPlainSurface(1, 1, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &g_pick_sys, nullptr)))
        s.error = "can't make the 1 x 1 copy";
    D3DLOCKED_RECT lr{};
    const RECT r{ tx, ty, tx + 1, ty + 1 };
    if (s.error.empty() && (FAILED(depth->GetSurfaceLevel(0, &src)) || FAILED(g_pick_rt->GetSurfaceLevel(0, &dst)) ||
                            FAILED(dev->StretchRect(src, &r, dst, nullptr, D3DTEXF_POINT)) ||
                            FAILED(dev->GetRenderTargetData(dst, g_pick_sys)) ||
                            FAILED(g_pick_sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))))
        s.error = "can't read the depth texture";
    release(src);
    release(dst);
    if (s.error.empty()) {
        const uint8_t* p = static_cast<const uint8_t*>(lr.pBits);     // B G R A
        s.id = p[0];                                                  // the object ID (tag.inc)
        s.unit = tags::unit_of(p[0]);
        const float z = p[2] / 255.0f * 0.00389099144f + p[1] / 255.0f * 1.51991853e-5f + p[3] / 255.0f * 0.99609381f;
        g_pick_sys->UnlockRect();
        const MtMatrix& P = cam->viewports[0].mProjMat;
        const MtMatrix& V = cam->viewports[0].mViewMat;
        if (z >= 0.99999f) {
            s.error = "nothing there (sky: no depth)";
        } else if (P.m1.x == 0.0f || P.m2.y == 0.0f || P.m3.w == 0.0f) {
            s.error = "no camera projection";
        } else {
            const float d = std::fabs(P.m4.z / (z * P.m3.w - P.m3.z));
            const float nx = (tx + 0.5f) / desc.Width * 2.0f - 1.0f, ny = 1.0f - (ty + 0.5f) / desc.Height * 2.0f;
            const float v[3] = { nx * d / P.m1.x, ny * d / P.m2.y, d / P.m3.w };   // view space (w = P23 x z = d)
            const float t[3] = { v[0] - V.m4.x, v[1] - V.m4.y, v[2] - V.m4.z };
            s.world[0] = t[0] * V.m1.x + t[1] * V.m1.y + t[2] * V.m1.z;
            s.world[1] = t[0] * V.m2.x + t[1] * V.m2.y + t[2] * V.m2.z;
            s.world[2] = t[0] * V.m3.x + t[1] * V.m3.y + t[2] * V.m3.z;
            s.distance = d;
            s.ok = true;
        }
    }
    req->done.set_value(s);
}

// outlines and cel shading go together (the user, 2026-11-23): the cel shading switch turns both on
bool outlines_on() { return cel::settings.on && settings.outline; }
bool highlight_on() { return settings.highlight && selection::unit() != 0; }

void draw_post() {
    g_diag.stage = 1;
    const uintptr_t render = *reinterpret_cast<uintptr_t*>(kRenderInstance);
    const uintptr_t camera = *reinterpret_cast<uintptr_t*>(kCameraInstance);
    if (!render || !camera) return;
    IDirect3DDevice9* dev = at<IDirect3DDevice9*>(render, kDevice);
    const uintptr_t depth_obj = at<uintptr_t>(render, kDepthTexture);
    g_diag.stage = 2;
    if (!dev || !depth_obj) return;
    g_diag.stage = 3;
    IDirect3DTexture9* depth = at<IDirect3DTexture9*>(depth_obj, kTexD3D);
    if (!depth) return;
    if (g_pick_pending) {
        D3DSURFACE_DESC pd;
        if (SUCCEEDED(depth->GetLevelDesc(0, &pd))) service_pick(dev, depth, pd, reinterpret_cast<const sCamera*>(camera));
    }
    if (!(settings.ao || outlines_on() || settings.contact || settings.aa || highlight_on())) return;   // only a pick this frame
    g_diag.stage = 4;
    if (!ensure_shaders(dev)) return;
    g_diag.stage = 5;
    D3DSURFACE_DESC desc;
    if (FAILED(depth->GetLevelDesc(0, &desc)) || !ensure_target(dev, desc.Width, desc.Height)) return;
    g_diag.stage = 6;
    if (!g_state && FAILED(dev->CreateStateBlock(D3DSBT_ALL, &g_state))) return;
    g_diag.stage = 7;

    // the main view's projection (sCamera viewport 0; MtMatrix rows m1..m4, row vectors)
    const auto* cam = reinterpret_cast<const sCamera*>(camera);
    const MtMatrix& proj = cam->viewports[0].mProjMat;
    const float c0[4] = { proj.m1.x, proj.m2.y, proj.m3.z, proj.m4.z };
    const float c1[4] = { proj.m3.w, 0.0f, 0.0f, 0.0f };
    std::memcpy(g_diag.proj, c0, sizeof c0);
    g_diag.proj[4] = c1[0];
    if (c0[0] == 0.0f || c0[1] == 0.0f || c0[3] == 0.0f) return;
    g_diag.stage = 8;
    const float fw = static_cast<float>(desc.Width), fh = static_cast<float>(desc.Height);
    const float c2[4] = { 1.0f / fw, 1.0f / fh, fw, fh };
    const float c3[4] = { settings.ao_radius, settings.ao_strength, 0.03f, 4000.0f };  // radius, strength, bias, fade start (cm)
    const float c4[4] = { 6000.0f, 0.08f, static_cast<float>(settings.ao_view), 0.0f };  // fade end (cm), max screen radius (uv), view
    // outline: the neighbour offset in px (the width is per 1080 lines), silhouette / crease thresholds from Detail
    const float detail = std::clamp(settings.outline_detail, 0.0f, 1.0f);
    const float c5[4] = { (std::max)(1.0f, settings.outline_width * fh / 1080.0f), 0.05f,
                          0.02f * std::pow(10.0f, (0.5f - detail) * 2.0f),   // Detail 0.5 = 0.02, 1 = 0.002, 0 = 0.2
                          settings.outline_opacity };
    const float c6[4] = { settings.outline_color[0], settings.outline_color[1], settings.outline_color[2],
                          settings.outline_distance * 100.0f };
    // contact shadows: toward the sun in the shader's camera space (x = view x * sign(P00), y = view y * sign(P11),
    // z = view z * P23 = the distance ahead); the view matrix's rows turn a world direction (row vectors)
    const MtMatrix& view = cam->viewports[0].mViewMat;
    const float tx = -g_sun[0], ty = -g_sun[1], tz = -g_sun[2];
    float lx = (tx * view.m1.x + ty * view.m2.x + tz * view.m3.x) * (proj.m1.x < 0.0f ? -1.0f : 1.0f);
    float ly = (tx * view.m1.y + ty * view.m2.y + tz * view.m3.y) * (proj.m2.y < 0.0f ? -1.0f : 1.0f);
    float lz = (tx * view.m1.z + ty * view.m2.z + tz * view.m3.z) * (proj.m3.w < 0.0f ? -1.0f : 1.0f);
    const float ll = std::sqrt(lx * lx + ly * ly + lz * lz);
    if (ll > 1e-6f) { lx /= ll; ly /= ll; lz /= ll; }
    const float c7[4] = { lx, ly, lz, settings.contact_length };
    const float c8[4] = { 25.0f, settings.contact_darkness, 3000.0f, 0.0f };   // thickness (cm), darkness, gone at (cm)
    const float c9[4] = { settings.aa_softness, 0.125f, 0.0312f, 0.0f };          // FXAA: subpixel, edge thresholds

    g_state->Capture();
    IDirect3DSurface9* scene = nullptr;
    IDirect3DSurface9* scene_depth = nullptr;
    IDirect3DSurface9* ao_surface = nullptr;
    dev->GetRenderTarget(0, &scene);
    dev->GetDepthStencilSurface(&scene_depth);
    g_ao_target->GetSurfaceLevel(0, &ao_surface);

    dev->SetVertexShader(nullptr);
    dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
    dev->SetRenderState(D3DRS_ZENABLE, FALSE);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    dev->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
    dev->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    dev->SetPixelShaderConstantF(0, c0, 1);
    dev->SetPixelShaderConstantF(1, c1, 1);
    dev->SetPixelShaderConstantF(2, c2, 1);
    dev->SetPixelShaderConstantF(3, c3, 1);
    dev->SetPixelShaderConstantF(4, c4, 1);
    dev->SetPixelShaderConstantF(5, c5, 1);
    dev->SetPixelShaderConstantF(6, c6, 1);
    dev->SetPixelShaderConstantF(7, c7, 1);
    dev->SetPixelShaderConstantF(8, c8, 1);
    dev->SetPixelShaderConstantF(9, c9, 1);
    set_point_clamp(dev, 0, depth);

    // the passes below multiply onto the scene (dest x src; the scene's alpha kept)
    dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
    if (settings.ao || settings.contact) {
        // 1: occlusion into our target: AO, then contact shadows multiplied into it (both dithered in 4 x 4 cells)
        dev->SetRenderTarget(0, ao_surface);
        dev->SetDepthStencilSurface(nullptr);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
        if (settings.ao) {
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            dev->SetPixelShader(g_ps_ao);
            draw_fullscreen(dev, desc.Width, desc.Height);
        }
        if (settings.contact) {
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, settings.ao ? TRUE : FALSE);
            dev->SetPixelShader(g_ps_contact);
            draw_fullscreen(dev, desc.Width, desc.Height);
        }

        if (g_diag.capture == 5) {
            IDirect3DSurface9* depth_surface = nullptr;
            depth->GetSurfaceLevel(0, &depth_surface);
            dump_target(dev, depth_surface, desc.Width, desc.Height, "pfx_depth");
            dump_target(dev, ao_surface, desc.Width, desc.Height, "pfx_ao");
            release(depth_surface);
            g_diag.capture = 6;
        }

        // 2: blurred, multiplied onto the scene (dest x src; the scene's alpha kept)
        dev->SetRenderTarget(0, scene);
        set_point_clamp(dev, 1, g_ao_target);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, settings.ao_view ? FALSE : TRUE);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        dev->SetPixelShader(g_ps_blur);
        draw_fullscreen(dev, desc.Width, desc.Height);
    }

    // 3: ink lines, multiplied onto the scene (part of the cel look: only with cel shading on)
    if (outlines_on()) {
        dev->SetRenderTarget(0, scene);
        dev->SetDepthStencilSurface(nullptr);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        dev->SetPixelShader(g_ps_outline);
        draw_fullscreen(dev, desc.Width, desc.Height);
    }

    // the selected object's outline (its ID in the depth texture's blue byte), alpha blended
    const uint8_t sel_id = settings.highlight ? tags::id_of(selection::unit()) : 0;
    if (sel_id) {
        const float c10[4] = { sel_id / 255.0f, (std::max)(1.0f, 2.0f * fh / 1080.0f), 0.0f, 0.0f };
        const float c11[4] = { settings.highlight_color[0], settings.highlight_color[1], settings.highlight_color[2], 1.0f };
        dev->SetPixelShaderConstantF(10, c10, 1);
        dev->SetPixelShaderConstantF(11, c11, 1);
        dev->SetRenderTarget(0, scene);
        dev->SetDepthStencilSurface(nullptr);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        dev->SetPixelShader(g_ps_highlight);
        draw_fullscreen(dev, desc.Width, desc.Height);
    }

    // anti-aliasing, last: the scene (with everything above) copied out, FXAA'd back over it
    if (settings.aa) {
        D3DSURFACE_DESC sd;
        IDirect3DSurface9* copy = nullptr;
        if (SUCCEEDED(scene->GetDesc(&sd)) && ensure_scene_copy(dev, sd) &&
            SUCCEEDED(g_scene_copy->GetSurfaceLevel(0, &copy)) &&
            SUCCEEDED(dev->StretchRect(scene, nullptr, copy, nullptr, D3DTEXF_NONE))) {
            dev->SetRenderTarget(0, scene);
            dev->SetDepthStencilSurface(nullptr);
            dev->SetTexture(2, g_scene_copy);
            dev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            dev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            dev->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            dev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            dev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            dev->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, FALSE);
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
            dev->SetPixelShader(g_ps_fxaa);
            draw_fullscreen(dev, sd.Width, sd.Height);
            dev->SetTexture(2, nullptr);
        }
        release(copy);
    }

    // back as the game had it (SetRenderTarget resets the viewport, so the state block goes last)
    dev->SetTexture(1, nullptr);
    dev->SetRenderTarget(0, scene);
    dev->SetDepthStencilSurface(scene_depth);
    g_state->Apply();
    release(ao_surface);
    release(scene_depth);
    release(scene);
    ++g_frames;
    g_diag.stage = 0;
    g_last_w = static_cast<int>(desc.Width);
    g_last_h = static_cast<int>(desc.Height);
}

// 0x8F4460, 11 bytes replaced: `mov ecx, [esp+0BCh]` (the tag array) / `mov eax, [esp+60h]` (the tag index); the tags
// are {command, key} pairs sorted by key: bits 29-31 the view, 24-28 the sub-scene (0x1F = the main scene), 20-23 the
// pass, 0-19 priority / depth (Vibed\Shadows\README.md). The loop's back-edge (0x8F5EBD) jumps here, so every tag passes.
uintptr_t jmp_ret_tag = 0;
uintptr_t kTagContinue = 0x8F5EAB;   // dispatchCommands: ++index, back to 0x8F4460 while tags remain

// returns non-zero to skip the tag (the RTX Remix bridge's filtering): the detour then jumps to the loop's continue
int __stdcall on_tag(const uint32_t* list, uint32_t index, uint32_t count) {
    if (index == 0) {
        g_done = false;
        ++g_diag.dispatches;
        if (g_diag.capture == 3) g_diag.capture = 2;
        if (g_diag.capture == 1) { g_diag.capture = 3; g_diag.nseq = 0; g_diag.count = count; }
        tags::on_dispatch_start();
    }
    // object IDs: before each depth-pass command (any view), c220 = the ID of the model that made it
    if (tags::enabled && ((list[index * 2 + 1] >> 20) & 0xF) == 1) {
        const uintptr_t render = *reinterpret_cast<uintptr_t*>(kRenderInstance);
        if (render) tags::on_depth_tag(at<void*>(render, kDevice), list[index * 2]);
    }
    if (g_diag.capture == 3 && g_diag.nseq < 64) {
        const uint32_t k = list[index * 2 + 1] >> 20;
        if (g_diag.nseq == 0 || (g_diag.seq[g_diag.nseq - 1] & 0xFFF) != k) g_diag.seq[g_diag.nseq++] = (index << 12) | k;
    }
    ++g_diag.tags;
    if (remix::settings.on) {
        const uintptr_t render = *reinterpret_cast<uintptr_t*>(kRenderInstance);
        if (render && remix::on_tag(at<void*>(render, kDevice), list, index, count)) return 1;
    }
    if (g_done || !(settings.ao || outlines_on() || settings.contact || settings.aa || highlight_on() || g_pick_pending)) return 0;
    const uint32_t key = list[index * 2 + 1];
    if (((key >> 24) & 0x1F) == kMainScene) {
        ++g_diag.main_tags;
        g_diag.max_pass_main = (std::max)(g_diag.max_pass_main, (key >> 20) & 0xF);
    }
    if (((key >> 24) & 0x1F) != kMainScene || ((key >> 20) & 0xF) < kPassTransparent) return 0;
    g_done = true;
    ++g_diag.triggers;
    g_diag.last_key = key;
    draw_post();
    return 0;
}

naked void detour_tag() {
    _asm {
        mov ecx, [esp + 0xBC]
        mov eax, [esp + 0x60]
        pushad
        push dword ptr [esp + 0x84 + 0x20]  // the tag count (var_11B4; pushad moved esp by 0x20)
        push eax
        push ecx
        call on_tag                 // __stdcall: pops its arguments
        test eax, eax
        jnz skip
        popad
        jmp dword ptr [jmp_ret_tag]
    skip:
        popad
        jmp dword ptr [kTagContinue]  // the loop's index increment (0x8F5EAB)
    }
}

// uShadow +0x40 mInfiniteLightDir. The unit may have just gone with its room (Shadows lists units for a few seconds after
// their last move), so the read is guarded.
void read_sun(uintptr_t unit, float* d) {
    __try {
        const float* p = reinterpret_cast<const float*>(unit + 0x40);
        d[0] = p[0];
        d[1] = p[1];
        d[2] = p[2];
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

bool g_hooked = false;
const bool g_listed = shader_file::add({ "Post effects (AO, outlines)", kSourcePath, kEmbedded, [] { g_want_compile = true; }, 3 });

} // namespace

Status status() {
    Status s;
    s.hooked = g_hooked;
    s.frames = g_frames;
    s.width = g_last_w;
    s.height = g_last_h;
    std::lock_guard lock(g_mutex);
    s.compiled = !g_code_ao.empty();
    s.from_file = g_from_file;
    s.error = g_error;
    return s;
}

namespace {
// queues a depth sample (nullptr when one is already waiting)
std::shared_ptr<PickRequest> queue_pick(int x, int y) {
    auto req = std::make_shared<PickRequest>();
    req->x = x;
    req->y = y;
    req->future = req->done.get_future().share();
    std::lock_guard lock(g_pick_mutex);
    if (g_pick) return nullptr;
    g_pick = req;
    g_pick_pending = true;
    return req;
}
} // namespace

std::shared_future<DepthSample> request_sample(int x, int y) {
    const std::shared_ptr<PickRequest> req = queue_pick(x, y);
    return req ? req->future : std::shared_future<DepthSample>();
}

DepthSample sample_world(int x, int y, int timeout_ms) {
    const std::shared_ptr<PickRequest> req = queue_pick(x, y);
    if (!req) {
        DepthSample s;
        s.error = "busy: another pick is waiting";
        return s;
    }
    if (req->future.wait_for(std::chrono::milliseconds(timeout_ms)) != std::future_status::ready) {
        std::lock_guard lock(g_pick_mutex);
        if (g_pick == req) {
            g_pick.reset();
            g_pick_pending = false;
        }
        DepthSample s;
        s.error = "no frame drew the main scene in time (loading, a menu over it, or minimised)";
        return s;
    }
    return req->future.get();
}

} // namespace post_fx

using namespace post_fx;

std::optional<std::string> PostFX::on_initialize() {
    g_hooked = install_hook_absolute(0x8F4460, hook_tag, &detour_tag, &jmp_ret_tag, 11);
    if (!g_hooked) spdlog::error("PostFX: failed to hook sRender::dispatchCommands' tag loop (0x8F4460)");
    if (!install_hook_absolute(remix::kTexLoop, hook_tex, remix::tex_detour(), &remix::jmp_ret_tex, 10))
        spdlog::error("PostFX: failed to hook sRender::dispatchCommands' texture loop (0x8F545E)");
    if (!install_hook_absolute(remix::kShaderCreate, remix::shader_hook(), remix::shader_detour(), &remix::jmp_ret_shader, 7))
        spdlog::error("PostFX: failed to hook sRender::dispatchCommands' shader creation (0x8F49BC)");
    if (!install_hook_absolute(remix::kVsArrays, remix::vsarr_hook(), remix::vsarr_detour(), &remix::jmp_ret_vsarr, 10))
        spdlog::error("PostFX: failed to hook sRender::dispatchCommands' vertex shader arrays (0x8F5380)");
    if (!install_hook_absolute(remix::kDrawIndexed, remix::draw_hook(), remix::draw_detour(), &remix::jmp_ret_draw, 7))
        spdlog::error("PostFX: failed to hook sRender::dispatchCommands' indexed draw (0x8F574E)");
    if (!install_hook_absolute(remix::kCullAndLod, remix::cull_hook(), remix::cull_detour(), &remix::jmp_ret_cull, 6))
        spdlog::error("PostFX: failed to hook uModel::frustumCullAndLOD (0x9EDBB0)");
    cel::init();
    tags::init();
    return Mod::on_initialize();
}

void PostFX::on_frame(fmilliseconds& dt) {
    if ((settings.ao || outlines_on() || settings.contact || settings.aa) && g_want_compile && !g_compiling) start_compile();
    // the sun for contact shadows: the stage's shadow light (uShadow mInfiniteLightDir +0x40), straight down without one
    {
        const shadows::Status sh = shadows::status();
        float d[3] = { 0.0f, -1.0f, 0.0f };
        if (sh.units > 0 && sh.unit[0].unit) read_sun(sh.unit[0].unit, d);
        const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        if (l > 1e-6f) for (int i = 0; i < 3; ++i) g_sun[i] = d[i] / l;
        remix::set_sun(g_sun);
    }
    cel::update();
    tex_filter::update();
    tags::enabled = true;   // object IDs are cheap (a lookup per depth-pass command): always on, for pick
    tags::update();
}

void PostFX::on_reset() { release_device_objects(); remix::on_reset(); }

static void draw_outline_gui();

static void draw_contact_gui() {
    ImGui::Checkbox(_("Contact shadows"), &settings.contact);
    ImGui::SameLine();
    Mod::help_marker(_("Small, sharp shadows where things meet (feet on the floor, a chin over the collar, coat folds), toward "
                       "the stage's sun, where the stage's shadow map is too coarse to show them."));
    if (!settings.contact) return;
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID("PostFX_Contact");
    ImGui::SliderFloat(_("Length"), &settings.contact_length, 5.0f, 150.0f, "%.0f cm");
    ImGui::SameLine();
    Mod::help_marker(_("How far from a surface an object still casts one."));
    ImGui::SliderFloat(_("Darkness"), &settings.contact_darkness, 0.0f, 1.0f, "%.2f");
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
}

void PostFX::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_B) {
        ImGui::Checkbox(_("Anti-aliasing (FXAA)"), &settings.aa);
        ImGui::SameLine();
        help_marker(_("Smooths jagged edges on models, the stage and outlines. Runs before effects and the HUD, so those stay "
                      "as sharp as they were."));
        if (settings.aa) {
            ImGui::Indent(lineIndent);
            ImGui::SliderFloat(_("Softness"), &settings.aa_softness, 0.0f, 1.0f, "%.2f");
            ImGui::SameLine();
            help_marker(_("How much fine detail inside surfaces is smoothed too (0 = only edges, sharpest)."));
            ImGui::Unindent(lineIndent);
        }
        ImGui::Checkbox(_("Outline the picked object"), &settings.highlight);
        ImGui::SameLine();
        help_marker(_("An outline around the object picked (photo mode, the debug server's pick): every model draws its "
                      "own ID into the frame's depth, so the outline follows its exact shape."));
        if (settings.highlight) {
            ImGui::Indent(lineIndent);
            ImGui::ColorEdit3(_("Outline colour"), settings.highlight_color);
            if (selection::unit() && ImGui::Button(_("Clear the pick"))) selection::clear();
            ImGui::Unindent(lineIndent);
        }
        return;
    }
    if (display == DISPLAY_SYSTEM_C) {     // the cel look: cel shading, and outlines with it
        cel::draw_gui();
        if (cel::settings.on) {
            ImGui::Indent(lineIndent);
            draw_outline_gui();
            ImGui::Unindent(lineIndent);
        }
        return;
    }
    if (display != DISPLAY_SYSTEM_A) return;
    tex_filter::draw_gui();
    remix::draw_gui();
    draw_contact_gui();
    ImGui::Checkbox(_("Ambient occlusion"), &settings.ao);
    ImGui::SameLine();
    help_marker(_("Darkens creases, corners and the ground around characters, where light from the surroundings can't "
                  "reach. Worked out from the frame's depth every frame, after the solid scene and before effects, so "
                  "glows and particles aren't darkened."));
    if (!settings.ao) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("PostFX_AO");
    ImGui::SliderFloat(_("Strength"), &settings.ao_strength, 0.0f, 8.0f, "%.1f");
    ImGui::SameLine();
    help_marker(_("How dark fully hidden corners get. 0 = off."));
    ImGui::SliderFloat(_("Radius"), &settings.ao_radius, 10.0f, 300.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("How far away a surface still shades another: small = only tight creases, large = broad shading under "
                  "and around objects."));
    const Status st = status();
    if (!st.hooked)
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), _("Not hooked (see the log)."));
    else if (!st.error.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", st.error.c_str());
    else if (!st.compiled)
        ImGui::TextDisabled(_("Compiling..."));
    else
        ImGui::TextDisabled(_("Drawn in %d frames (%d x %d)%s"), st.frames, st.width, st.height,
                            st.from_file ? _(", from the shader file") : "");
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

static void draw_outline_gui() {
    ImGui::Checkbox(_("Outlines"), &settings.outline);
    ImGui::SameLine();
    Mod::help_marker(_("Ink lines around objects and along their edges, part of the cel look (they show while cel shading is "
                       "on). Found from the frame's depth after the solid scene, so effects and glows draw over them."));
    if (!settings.outline) return;
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID("PostFX_Outline");
    ImGui::SliderFloat(_("Width"), &settings.outline_width, 0.5f, 4.0f, "%.1f px");
    ImGui::SameLine();
    Mod::help_marker(_("Line width at 1080p (wider screens scale it up)."));
    ImGui::SliderFloat(_("Detail"), &settings.outline_detail, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("0 = only outer silhouettes, 1 = also every fold and corner inside an object."));
    ImGui::SliderFloat(_("Opacity"), &settings.outline_opacity, 0.0f, 1.0f, "%.2f");
    ImGui::ColorEdit3(_("Colour"), settings.outline_color);
    ImGui::SameLine();
    Mod::help_marker(_("The lines multiply the picture: black = ink, a colour tints the edges."));
    ImGui::SliderFloat(_("Distance"), &settings.outline_distance, 5.0f, 100.0f, "%.0f m");
    ImGui::SameLine();
    Mod::help_marker(_("Lines fade out toward this distance, so far-off detail doesn't turn into noise."));
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
}

void PostFX::on_config_load(const utility::Config& cfg) {
    const Settings d;
    settings.ao = cfg.get<bool>("postfx_ao").value_or(d.ao);
    settings.ao_strength = std::clamp(cfg.get<float>("postfx_ao_strength").value_or(d.ao_strength), 0.0f, 8.0f);
    settings.ao_radius = std::clamp(cfg.get<float>("postfx_ao_radius").value_or(d.ao_radius), 10.0f, 300.0f);
    cel::load(cfg);
    tex_filter::load(cfg);
    remix::load(cfg);
    settings.contact = cfg.get<bool>("postfx_contact").value_or(d.contact);
    settings.aa = cfg.get<bool>("postfx_aa").value_or(d.aa);
    settings.highlight = cfg.get<bool>("postfx_highlight").value_or(d.highlight);
    settings.aa_softness = std::clamp(cfg.get<float>("postfx_aa_softness").value_or(d.aa_softness), 0.0f, 1.0f);
    settings.contact_length = std::clamp(cfg.get<float>("postfx_contact_length").value_or(d.contact_length), 5.0f, 150.0f);
    settings.contact_darkness = std::clamp(cfg.get<float>("postfx_contact_darkness").value_or(d.contact_darkness), 0.0f, 1.0f);
    settings.outline = cfg.get<bool>("postfx_outline").value_or(d.outline);
    settings.outline_width = std::clamp(cfg.get<float>("postfx_outline_width").value_or(d.outline_width), 0.5f, 4.0f);
    settings.outline_detail = std::clamp(cfg.get<float>("postfx_outline_detail").value_or(d.outline_detail), 0.0f, 1.0f);
    settings.outline_opacity = std::clamp(cfg.get<float>("postfx_outline_opacity").value_or(d.outline_opacity), 0.0f, 1.0f);
    settings.outline_distance = std::clamp(cfg.get<float>("postfx_outline_distance").value_or(d.outline_distance), 5.0f, 100.0f);
    settings.outline_color[0] = std::clamp(cfg.get<float>("postfx_outline_r").value_or(d.outline_color[0]), 0.0f, 1.0f);
    settings.outline_color[1] = std::clamp(cfg.get<float>("postfx_outline_g").value_or(d.outline_color[1]), 0.0f, 1.0f);
    settings.outline_color[2] = std::clamp(cfg.get<float>("postfx_outline_b").value_or(d.outline_color[2]), 0.0f, 1.0f);
}

void PostFX::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("postfx_ao", settings.ao);
    cfg.set<float>("postfx_ao_strength", settings.ao_strength);
    cfg.set<float>("postfx_ao_radius", settings.ao_radius);
    cel::save(cfg);
    tex_filter::save(cfg);
    remix::save(cfg);
    cfg.set<bool>("postfx_contact", settings.contact);
    cfg.set<bool>("postfx_aa", settings.aa);
    cfg.set<bool>("postfx_highlight", settings.highlight);
    cfg.set<float>("postfx_aa_softness", settings.aa_softness);
    cfg.set<float>("postfx_contact_length", settings.contact_length);
    cfg.set<float>("postfx_contact_darkness", settings.contact_darkness);
    cfg.set<bool>("postfx_outline", settings.outline);
    cfg.set<float>("postfx_outline_width", settings.outline_width);
    cfg.set<float>("postfx_outline_detail", settings.outline_detail);
    cfg.set<float>("postfx_outline_opacity", settings.outline_opacity);
    cfg.set<float>("postfx_outline_distance", settings.outline_distance);
    cfg.set<float>("postfx_outline_r", settings.outline_color[0]);
    cfg.set<float>("postfx_outline_g", settings.outline_color[1]);
    cfg.set<float>("postfx_outline_b", settings.outline_color[2]);
}
