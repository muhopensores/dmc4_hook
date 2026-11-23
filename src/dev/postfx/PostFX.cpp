#include "PostFX.hpp"
#include "../../ModFramework.hpp"
#include "../shader_file.hpp"
#include "../../sdk/Cam.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <fstream>
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
constexpr char kSourcePath[] = "nativePC\\shader\\postfx\\ao.hlsl";

const char kEmbedded[] =
#include "ao.inc"
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
std::vector<uint8_t>   g_code_ao, g_code_blur;   // guarded by g_mutex
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
    const HRESULT hr = fn(src.data(), src.size(), "ao.hlsl", nullptr, nullptr, entry, "ps_3_0", kOptimize3, 0, &code, &errors);
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
        std::vector<uint8_t> ao, blur;
        std::string error;
        const bool ok = compile_one(fn, src, "ps_ao", ao, error) && compile_one(fn, src, "ps_blur", blur, error);
        {
            std::lock_guard lock(g_mutex);
            g_from_file = from_file;
            g_error = ok ? std::string() : error;
            if (ok) {
                g_code_ao = std::move(ao);
                g_code_blur = std::move(blur);
                ++g_code_generation;
            }
        }
        if (!ok) spdlog::error("PostFX: ao shaders: {}", error);
        g_compiling = false;
    }).detach();
}

// ---- render thread state ----

IDirect3DPixelShader9* g_ps_ao = nullptr;
IDirect3DPixelShader9* g_ps_blur = nullptr;
int                    g_ps_generation = 0;
IDirect3DTexture9*     g_ao_target = nullptr;   // D3DPOOL_DEFAULT: released on reset
UINT                   g_ao_w = 0, g_ao_h = 0;
IDirect3DStateBlock9*  g_state = nullptr;       // released on reset
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
    release(g_state);
    g_ao_w = g_ao_h = 0;
}

bool ensure_shaders(IDirect3DDevice9* dev) {
    std::lock_guard lock(g_mutex);
    if (g_code_generation == g_ps_generation) return g_ps_ao && g_ps_blur;
    release(g_ps_ao);
    release(g_ps_blur);
    g_ps_generation = g_code_generation;
    if (g_code_ao.empty() || g_code_blur.empty()) return false;
    if (FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_ao.data()), &g_ps_ao)) ||
        FAILED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(g_code_blur.data()), &g_ps_blur))) {
        release(g_ps_ao);
        release(g_ps_blur);
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

void draw_ao() {
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
    set_point_clamp(dev, 0, depth);

    // 1: occlusion into our target
    dev->SetRenderTarget(0, ao_surface);
    dev->SetDepthStencilSurface(nullptr);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    dev->SetPixelShader(g_ps_ao);
    draw_fullscreen(dev, desc.Width, desc.Height);

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
    dev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
    dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    dev->SetPixelShader(g_ps_blur);
    draw_fullscreen(dev, desc.Width, desc.Height);

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

void __stdcall on_tag(const uint32_t* tags, uint32_t index, uint32_t count) {
    if (index == 0) {
        g_done = false;
        ++g_diag.dispatches;
        if (g_diag.capture == 3) g_diag.capture = 2;
        if (g_diag.capture == 1) { g_diag.capture = 3; g_diag.nseq = 0; g_diag.count = count; }
    }
    if (g_diag.capture == 3 && g_diag.nseq < 64) {
        const uint32_t k = tags[index * 2 + 1] >> 20;
        if (g_diag.nseq == 0 || (g_diag.seq[g_diag.nseq - 1] & 0xFFF) != k) g_diag.seq[g_diag.nseq++] = (index << 12) | k;
    }
    ++g_diag.tags;
    if (g_done || !settings.ao) return;
    const uint32_t key = tags[index * 2 + 1];
    if (((key >> 24) & 0x1F) == kMainScene) {
        ++g_diag.main_tags;
        g_diag.max_pass_main = (std::max)(g_diag.max_pass_main, (key >> 20) & 0xF);
    }
    if (((key >> 24) & 0x1F) != kMainScene || ((key >> 20) & 0xF) < kPassTransparent) return;
    g_done = true;
    ++g_diag.triggers;
    g_diag.last_key = key;
    draw_ao();
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
        popad
        jmp dword ptr [jmp_ret_tag]
    }
}

bool g_hooked = false;
const bool g_listed = shader_file::add({ "Ambient occlusion", kSourcePath, kEmbedded, [] { g_want_compile = true; }, 3 });

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

} // namespace post_fx

using namespace post_fx;

std::optional<std::string> PostFX::on_initialize() {
    g_hooked = install_hook_absolute(0x8F4460, hook_tag, &detour_tag, &jmp_ret_tag, 11);
    if (!g_hooked) spdlog::error("PostFX: failed to hook sRender::dispatchCommands' tag loop (0x8F4460)");
    return Mod::on_initialize();
}

void PostFX::on_frame(fmilliseconds& dt) {
    if (settings.ao && g_want_compile && !g_compiling) start_compile();
}

void PostFX::on_reset() { release_device_objects(); }

void PostFX::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
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

void PostFX::on_config_load(const utility::Config& cfg) {
    const Settings d;
    settings.ao = cfg.get<bool>("postfx_ao").value_or(d.ao);
    settings.ao_strength = std::clamp(cfg.get<float>("postfx_ao_strength").value_or(d.ao_strength), 0.0f, 8.0f);
    settings.ao_radius = std::clamp(cfg.get<float>("postfx_ao_radius").value_or(d.ao_radius), 10.0f, 300.0f);
}

void PostFX::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("postfx_ao", settings.ao);
    cfg.set<float>("postfx_ao_strength", settings.ao_strength);
    cfg.set<float>("postfx_ao_radius", settings.ao_radius);
}
