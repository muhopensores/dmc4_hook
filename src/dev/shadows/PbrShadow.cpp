#include "PbrShadow.hpp"
#include "ShadowReceiver.hpp"
#include "../pbr/PbrMaterials.hpp"
#include "../ps_bind.hpp"
#include "../../ModFramework.hpp"
#include <d3d9.h>
#include <atomic>
#include <cstring>

namespace pbr_shadow {

Settings  settings;
uintptr_t jmp_ret_model_gate = 0;
uintptr_t jmp_ret_devil_gate = 0;

namespace {

constexpr UINT      kRegister = 202;                // material_pbr.inc gPbrShadow[18]
constexpr UINT      kCount = 18;
constexpr DWORD     kSampler = 13;                  // gPbrShadowNear / Middle / Far: s13-s15
constexpr uintptr_t kMaterialStandardVtbl = 0xC08934;

// the receiver gates' continuations
uintptr_t g_model_go = 0x9F33C4, g_model_skip = 0x9F3A6D;
uintptr_t g_devil_go = 0x5287D0, g_devil_skip = 0x528E8B;

// read by the gates (game thread)
volatile bool    g_active = false;
std::atomic<int> g_skipped{ 0 };

// render thread
float              g_regs[4 * kCount] = {};
IDirect3DTexture9* g_maps[3] = {};              // AddRef'd
bool               g_have = false;              // g_regs / g_maps are current
Status             g_status;

// what s13-s15 held before ours (sRender's thread)
constexpr D3DSAMPLERSTATETYPE kStates[6] = { D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_MAGFILTER, D3DSAMP_MINFILTER,
                                             D3DSAMP_MIPFILTER, D3DSAMP_SRGBTEXTURE };
constexpr DWORD kValues[6] = { D3DTADDRESS_CLAMP, D3DTADDRESS_CLAMP, D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_NONE, FALSE };
bool                   g_bound = false;
IDirect3DBaseTexture9* g_saved_tex[3] = {};
DWORD                  g_saved_state[3][6] = {};

void save(IDirect3DDevice9* device) {
    for (DWORD i = 0; i < 3; ++i) {
        g_saved_tex[i] = nullptr;
        device->GetTexture(kSampler + i, &g_saved_tex[i]);         // AddRef'd
        for (int k = 0; k < 6; ++k) device->GetSamplerState(kSampler + i, kStates[k], &g_saved_state[i][k]);
    }
}

void restore(IDirect3DDevice9* device) {
    for (DWORD i = 0; i < 3; ++i) {
        device->SetTexture(kSampler + i, g_saved_tex[i]);
        if (g_saved_tex[i]) g_saved_tex[i]->Release();
        g_saved_tex[i] = nullptr;
        for (int k = 0; k < 6; ++k) device->SetSamplerState(kSampler + i, kStates[k], g_saved_state[i][k]);
    }
    g_bound = false;
}

// sRender's thread, on every pixel shader change: a PBR shader gets the shadow's registers and maps; any other shader gets
// s13-s15 back as they were
void __stdcall on_bind(IDirect3DDevice9* device, IDirect3DPixelShader9* ps) {
    if (pbr_materials::is_ours(ps)) {
        device->SetPixelShaderConstantF(kRegister, g_regs, kCount);     // c203.x = 0 when not current: the shader skips
        if (!g_have) return;
        if (!g_bound) save(device);
        for (DWORD i = 0; i < 3; ++i) {
            device->SetTexture(kSampler + i, g_maps[i]);
            for (int k = 0; k < 6; ++k) device->SetSamplerState(kSampler + i, kStates[k], kValues[k]);
        }
        g_bound = true;
    } else if (g_bound) {
        restore(device);
    }
}

// game thread, at a receiver gate whose draw mode receives: true = this mesh is drawn by our PBR shader, which shades the
// shadow itself
bool __stdcall skip_receiver(uintptr_t mat) {
    if (!g_active || !mat) return false;
    if (*reinterpret_cast<const uintptr_t*>(mat) != kMaterialStandardVtbl) return false;
    if (!pbr_materials::draws_key(*reinterpret_cast<const uint32_t*>(mat + 0x10))) return false;
    g_skipped.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void release_maps() {
    for (IDirect3DTexture9*& m : g_maps) {
        if (m) m->Release();
        m = nullptr;
    }
    g_have = false;
}

} // namespace

#pragma push_macro("naked")
#undef naked                    // mod.hpp's `naked` is static; these are declared in the header

// 0x9F33B6 (uModel's receiver pass), 8 bytes: test byte ptr [esi+4], 0Bh / mov [esp+2Ch], esi; then jz 0x9F3A6D (skip)
__declspec(naked) void detour_model_gate() {   // not the `naked` macro: that one is static
    _asm {
        mov [esp + 0x2C], esi
        test byte ptr [esi + 4], 0x0B
        jz model_skip
        pushad
        push esi
        call skip_receiver          // __stdcall
        test al, al
        popad                       // keeps the flags
        jnz model_skip
        jmp dword ptr [g_model_go]
    model_skip:
        jmp dword ptr [g_model_skip]
    }
}

// 0x5287C2 (uDevil4Model's receiver pass), 8 bytes: test byte ptr [ecx+4], 0Bh / mov [esp+0Ch], ecx; then jz 0x528E8B
__declspec(naked) void detour_devil_gate() {
    _asm {
        mov [esp + 0x0C], ecx
        test byte ptr [ecx + 4], 0x0B
        jz devil_skip
        pushad
        push ecx
        call skip_receiver          // __stdcall
        test al, al
        popad
        jnz devil_skip
        jmp dword ptr [g_devil_go]
    devil_skip:
        jmp dword ptr [g_devil_skip]
    }
}

#pragma pop_macro("naked")

void init() { ps_bind::add(&on_bind); }

void update(bool mod_enabled) {
    shadow_receiver::Frame f;
    const bool data = mod_enabled && settings.on && shadow_receiver::take_frame(f);    // f.maps AddRef'd when true
    const bool shader = pbr_materials::shader_has_shadows();
    const bool pbr = pbr_materials::mod_enabled;
    const bool active = data && shader && pbr && f.maps[0] && f.maps[1] && f.maps[2] && f.size > 0;

    release_maps();
    if (active) {
        for (int i = 0; i < 3; ++i) g_maps[i] = f.maps[i];
    } else if (data) {
        for (IDirect3DTexture9* m : f.maps)
            if (m) m->Release();
    }

    float* r = g_regs;
    std::memset(r, 0, sizeof(g_regs));
    if (active) {
        const shadow_receiver::Settings& rs = shadow_receiver::settings;
        const bool soft = rs.soft;
        r[0] = soft ? rs.light_size : 0.0f;         // without soft shadows: a fixed narrow filter, about the stock one
        r[1] = soft ? rs.max_soft : 5.0f;
        r[2] = soft ? rs.min_radius : 1.5f;
        r[3] = 1.0f;
        r[4] = 1.0f;
        r[5] = (float)f.size;
        r[6] = f.border;
        r[7] = f.colour[3];
        std::memcpy(r + 8, f.range, 3 * sizeof(float));
        std::memcpy(r + 12, f.grad, 4 * sizeof(float));
        std::memcpy(r + 16, f.colour, 4 * sizeof(float));
        std::memcpy(r + 20, f.light_dir, 3 * sizeof(float));
        for (int c = 0; c < 3; ++c)                 // rows u, v, depth, w = the game matrix's columns
            for (int k = 0; k < 4; ++k)
                for (int j = 0; j < 4; ++j) r[24 + 16 * c + 4 * k + j] = f.mats[16 * c + 4 * j + k];
    }
    g_have = active;
    g_active = active;

    g_status.active = active;
    g_status.data = data;
    g_status.shader = shader;
    g_status.pbr = pbr;
    g_status.skipped = g_skipped.exchange(0, std::memory_order_relaxed);
}

void on_reset() {
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (device) {
        if (g_bound) restore(device);
        for (DWORD i = 0; i < 3; ++i) device->SetTexture(kSampler + i, nullptr);
    }
    g_active = false;
    release_maps();
}

Status status() { return g_status; }

} // namespace pbr_shadow
