#include "ShadowReceiver.hpp"
#include "../../ModFramework.hpp"
#include "../ps_bind.hpp"
#include "../shader_file.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace shadow_receiver {

Settings settings;
uintptr_t jmp_ret_capture = 0;

namespace {

constexpr UINT kTuneRegister = 210;                // shadow_receiver.inc: gShTune, gShFlags, gShMat[12]
constexpr UINT kTuneCount = 14;

constexpr uintptr_t kShaderInstance = 0x00E55700;  // sShader::mpInstance
constexpr uintptr_t kTechniques     = 0x4838;      // sShader: mpTechnique[]
constexpr uintptr_t kReceiverSlot   = 0x00E5561C;  // tXfMaterialShadowReceiver's slot (sShader::InitTechniques 0x8FFDB1)
constexpr uintptr_t kTechInfo       = 0x10;
enum InfoField { kComponents = 1, kPermutations = 2, kPsCount = 3, kPsRecords = 8 };
constexpr uintptr_t kCascadeCall    = 0xA74E61;    // return address of sub_A73E90's call (the cascade mode)
constexpr char kSourcePath[] = "nativePC\\shader\\shadows\\shadow_receiver.hlsl";
constexpr char kCacheDir[] = "nativePC\\shader\\shadows\\cache";
constexpr char kCacheTag[] = "shr1 ps_3_0 O3";

const char kEmbedded[] =
#include "shadow_receiver.inc"
    ;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// d3dcompiler_47's D3DCompile, loaded at run time (as PbrMaterials)
struct Blob : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};
struct Macro { const char* name; const char* definition; };
using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const Macro*, void*, const char*, const char*, UINT,
                                      UINT, Blob**, Blob**);
constexpr UINT kOptimize3 = 1u << 15;

D3DCompileFn compiler() {
    static D3DCompileFn fn = [] {
        HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
        return dll ? (D3DCompileFn)GetProcAddress(dll, "D3DCompile") : nullptr;
    }();
    return fn;
}

using Defines = std::vector<std::pair<std::string, std::string>>;

// The original's constant table: HAS_<name> / REG_<name> per parameter (same reader as PbrMaterials)
bool read_registers(const uint32_t* code, Defines& out) {
    if (!code || (code[0] & 0xFFFF0000u) != 0xFFFF0000u) return false;
    for (size_t i = 1; i < 0x4000;) {
        const uint32_t tok = code[i];
        if ((tok & 0xFFFF) != 0xFFFE) break;
        const uint32_t n = (tok >> 16) & 0x7FFF;
        if (n >= 8 && code[i + 1] == 0x42415443u) {                      // 'CTAB'
            const auto* base = reinterpret_cast<const uint8_t*>(&code[i + 2]);
            const uint32_t size = (n - 1) * 4;
            const uint32_t count = *reinterpret_cast<const uint32_t*>(base + 12);
            const uint32_t info = *reinterpret_cast<const uint32_t*>(base + 16);
            if (info + 20ull * count > size) return false;
            for (uint32_t k = 0; k < count; ++k) {
                const uint8_t* c = base + info + 20 * k;
                const uint32_t name = *reinterpret_cast<const uint32_t*>(c);
                const uint16_t set = *reinterpret_cast<const uint16_t*>(c + 4);
                const uint16_t index = *reinterpret_cast<const uint16_t*>(c + 6);
                if (name >= size || set > 3) return false;
                std::string s;
                for (uint32_t p = name; p < size && base[p]; ++p) {
                    const char ch = (char)base[p];
                    if (!(isalnum((unsigned char)ch) || ch == '_')) return false;
                    s += ch;
                }
                if (s.empty()) return false;
                out.emplace_back("HAS_" + s, "1");
                out.emplace_back("REG_" + s, std::string(1, "bics"[set]) + std::to_string(index));
            }
            return true;
        }
        i += 1 + n;
    }
    return false;
}

bool has(const Defines& d, const char* name) {
    const std::string key = std::string("HAS_") + name;
    for (const auto& [k, v] : d)
        if (k == key) return true;
    return false;
}

struct Target {
    int       ps = -1;
    uintptr_t rec = 0;                      // +0 handle, +4 bytecode
    Defines   regs;
    IDirect3DPixelShader9* orig_handle = nullptr;
    const void*            orig_code = nullptr;
    IDirect3DPixelShader9* ours = nullptr;
    std::vector<uint8_t>   code;
};
struct Result { size_t index; std::vector<uint8_t> code; std::string error; };

// render thread
uintptr_t           g_tech = 0;
std::vector<Target> g_targets;
bool                g_from_file = false;
unsigned            g_max_slots = 0;
int                 g_failed = 0;
std::string         g_error;
std::string         g_waiting = "not looked yet";

// the cascade matrices and the rest of the receiver's inputs (game thread writes, sRender's thread reads)
std::mutex g_mat_mutex;
Frame      g_frame;                         // its maps AddRef'd
bool       g_frame_held = false;            // g_frame is a capture (release_frame clears it)
bool       g_frame_released = false;        // a device reset is under way: don't take new references
uint32_t   g_mat_count = 0;                 // captures so far
uint32_t   g_mat_seen = 0;                  // g_mat_count at the last frame update
int        g_mat_stale = 1000;              // frames since the count last moved

// read by the bind listener (sRender's thread)
constexpr int                   kMaxOurs = 64;
IDirect3DPixelShader9* volatile g_bind_ours[kMaxOurs] = {};
volatile int                    g_bind_count = 0;
float                           g_bind_regs[4 * kTuneCount] = {};

// worker <-> render thread
std::mutex          g_mutex;
std::vector<Result> g_results;
bool                g_results_ready = false;
int                 g_results_generation = 0;
int                 g_generation = 0;
std::atomic<bool>   g_compiling{ false };
std::atomic<bool>   g_want_compile{ true };

// 0xA775E0 (receiver setup), from the cascade draw only: unit (1st argument), cTrans (2nd), the map range (3rd: float3),
// the matrices (6th)
void __stdcall on_capture(uintptr_t ret, uintptr_t unit, uintptr_t trans, const float* range, const float* mats) {
    if (ret != kCascadeCall || !mats || !unit) return;
    Frame f;
    std::memcpy(f.mats, mats, sizeof(f.mats));
    if (range) std::memcpy(f.range, range, 3 * sizeof(float));
    // the far fade: the cascade draw put it in the scene's parameter table (a pointer to its float4) before the call
    const uintptr_t table = trans ? at<uintptr_t>(trans, 0x2A94) : 0;
    const uint32_t grad = at<uint32_t>(unit, 0x270);
    if (table && grad < 0x10000) {
        const float* g = at<const float*>(table, 4 * grad);
        if (g) std::memcpy(f.grad, g, sizeof(f.grad));
    }
    std::memcpy(f.colour, reinterpret_cast<const float*>(unit + 0x60), sizeof(f.colour));
    std::memcpy(f.light_dir, reinterpret_cast<const float*>(unit + 0x40), 3 * sizeof(float));
    const float b = at<float>(unit, 0xF8);
    f.border = b > 1e-6f ? 1.0f / b : 0.0f;
    for (int i = 0; i < 3; ++i) {
        const uintptr_t tex = at<uintptr_t>(unit, 0x18 + 4 * i);
        f.maps[i] = tex ? at<IDirect3DTexture9*>(tex, 0x10) : nullptr;
        if (i == 0 && tex) f.size = at<uint16_t>(tex, 0x1C);
    }
    std::lock_guard<std::mutex> lock(g_mat_mutex);
    ++g_mat_count;
    if (g_frame_released) {         // a device reset: keep the matrices, no references to the maps
        for (IDirect3DTexture9*& m : f.maps) m = nullptr;
    } else {
        for (IDirect3DTexture9* m : f.maps)
            if (m) m->AddRef();
    }
    for (IDirect3DTexture9* m : g_frame.maps)
        if (m) m->Release();
    g_frame = f;
    g_frame_held = !g_frame_released;
}

IDirect3DPixelShader9*& handle_of(const Target& t) { return at<IDirect3DPixelShader9*>(t.rec, 0); }
const void*&            code_of(const Target& t) { return at<const void*>(t.rec, 4); }

void install(Target& t) {
    if (!t.ours) return;
    IDirect3DPixelShader9*& h = handle_of(t);
    const void*& c = code_of(t);
    if (h == t.ours && c == t.code.data()) return;
    if (h != t.ours) t.orig_handle = h;
    if (c != t.code.data()) t.orig_code = c;
    c = t.code.data();
    h = t.ours;
}

void restore(Target& t) {
    IDirect3DPixelShader9*& h = handle_of(t);
    if (h != t.ours || !t.ours) return;
    code_of(t) = t.orig_code;
    h = t.orig_handle;                          // 0: the game creates it from its bytecode again
}

void forget() {
    for (Target& t : g_targets)
        if (t.ours) t.ours->Release();
    g_targets.clear();
    g_tech = 0;
    ++g_generation;
}

const uint32_t* receiver_info(uintptr_t& tech) {
    char why[160];
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) { g_waiting = "no sShader yet"; return nullptr; }
    const uint32_t slot = *reinterpret_cast<uint32_t*>(kReceiverSlot);
    if (slot > 255) {
        snprintf(why, sizeof why, "technique slot %u (0xE5561C) not set", slot);
        g_waiting = why;
        return nullptr;
    }
    tech = at<uintptr_t>(shader, kTechniques + 4 * slot);
    if (!tech) {
        snprintf(why, sizeof why, "tXfMaterialShadowReceiver (slot %u) not loaded", slot);
        g_waiting = why;
        return nullptr;
    }
    const auto* info = at<const uint32_t*>(tech, kTechInfo);
    if (!info || info[kComponents] != 6 || info[kPermutations] != 672 || info[kPsCount] != 211 || !info[kPsRecords]) {
        snprintf(why, sizeof why, "slot %u: technique info %p: components %u, permutations %u, pixel shaders %u (want 6, "
                 "672, 211)", slot, (const void*)info, info ? info[kComponents] : 0, info ? info[kPermutations] : 0,
                 info ? info[kPsCount] : 0);
        g_waiting = why;
        return nullptr;
    }
    g_waiting.clear();
    return info;
}

// The cascade receivers: the pixel shaders whose constant table lists the Middle and Far maps
bool locate() {
    uintptr_t tech = 0;
    const uint32_t* info = receiver_info(tech);
    if (!info) return false;
    std::vector<Target> targets;
    for (uint32_t ps = 0; ps < info[kPsCount]; ++ps) {
        Target t;
        t.ps = (int)ps;
        t.rec = at<uintptr_t>(info[kPsRecords], 4 * ps);
        if (!t.rec || !read_registers(at<const uint32_t*>(t.rec, 4), t.regs)) continue;
        if (!has(t.regs, "XfSamplerShadowMiddle") || !has(t.regs, "XfSamplerShadowFar") ||
            !has(t.regs, "XfShadowPCFRadius") || !has(t.regs, "XfShadowMapRangeGrad"))
            continue;
        t.orig_handle = handle_of(t);
        t.orig_code = code_of(t);
        targets.push_back(std::move(t));
    }
    g_targets = std::move(targets);
    g_tech = tech;
    spdlog::info("[Shadows] {} cascade receiver pixel shaders to replace", g_targets.size());
    return true;
}

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

// Worker thread: compile every target (from the cache when its source + defines were compiled before)
void start_compile() {
    D3DCompileFn fn = compiler();
    if (!fn) {
        g_error = "d3dcompiler_47.dll not found";
        g_want_compile = false;
        return;
    }
    auto source = std::make_shared<std::string>(read_source(g_from_file));
    static const char* kTaps[] = { "16", "24", "32" };
    Defines common = {
        { "SHADOW_TAPS", kTaps[std::clamp(settings.quality, 0, 2)] },
        { "SHADOW_DEBUG", std::to_string(settings.debug) },
    };
    auto jobs = std::make_shared<std::vector<Defines>>();
    for (const Target& t : g_targets) {
        Defines d = common;
        d.insert(d.end(), t.regs.begin(), t.regs.end());
        jobs->push_back(std::move(d));
    }
    g_want_compile = false;
    g_compiling = true;
    const int generation = g_generation;
    std::thread([fn, source, jobs, generation] {
        std::vector<Result> out(jobs->size());
        std::error_code ec;
        std::filesystem::create_directories(kCacheDir, ec);
        std::atomic<size_t> next{ 0 };
        auto work = [&] {
            for (size_t i; (i = next++) < jobs->size();) {
                Result& r = out[i];
                r.index = i;
                uint64_t h = 1469598103934665603ull;     // FNV-1a 64 over the tag, the source and the defines
                auto mix = [&h](const std::string& s) {
                    for (unsigned char ch : s) { h ^= ch; h *= 1099511628211ull; }
                    h ^= 0xFF;
                    h *= 1099511628211ull;
                };
                mix(kCacheTag);
                mix(*source);
                for (const auto& [name, value] : (*jobs)[i]) { mix(name); mix(value); }
                char file[160];
                snprintf(file, sizeof file, "%s\\%016llx.cso", kCacheDir, (unsigned long long)h);
                {
                    std::ifstream f(file, std::ios::binary);
                    if (f) {
                        r.code.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
                        if (r.code.size() >= 8 && r.code.size() % 4 == 0 && r.code[2] == 0xFF && r.code[3] == 0xFF) continue;
                        r.code.clear();
                    }
                }
                std::vector<Macro> macros;
                for (const auto& [name, value] : (*jobs)[i]) macros.push_back({ name.c_str(), value.c_str() });
                macros.push_back({ nullptr, nullptr });
                Blob* code = nullptr;
                Blob* errors = nullptr;
                const HRESULT hr = fn(source->data(), source->size(), "shadow_receiver.hlsl", macros.data(), nullptr,
                                      "main", "ps_3_0", kOptimize3, 0, &code, &errors);
                if (SUCCEEDED(hr) && code) {
                    const auto* p = static_cast<const uint8_t*>(code->GetBufferPointer());
                    r.code.assign(p, p + code->GetBufferSize());
                }
                if (errors) {
                    if (FAILED(hr)) r.error.assign(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
                    errors->Release();
                }
                if (code) code->Release();
                if (r.code.empty() && r.error.empty()) r.error = "compile failed";
                if (!r.code.empty()) {
                    std::ofstream f(file, std::ios::binary | std::ios::trunc);
                    f.write(reinterpret_cast<const char*>(r.code.data()), (std::streamsize)r.code.size());
                }
            }
        };
        const unsigned threads = std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
        std::vector<std::thread> pool;
        for (unsigned k = 1; k < threads && k < jobs->size(); ++k) pool.emplace_back(work);
        work();
        for (std::thread& th : pool) th.join();
        std::lock_guard<std::mutex> lock(g_mutex);
        g_results = std::move(out);
        g_results_generation = generation;
        g_results_ready = true;
        g_compiling = false;
    }).detach();
}

void take_results(IDirect3DDevice9* device) {
    std::vector<Result> results;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (!g_results_ready) return;
        results = std::move(g_results);
        g_results_ready = false;
        if (g_results_generation != g_generation) return;
    }
    g_failed = 0;
    g_error.clear();
    for (Result& r : results) {
        if (r.index >= g_targets.size()) continue;
        Target& t = g_targets[r.index];
        IDirect3DPixelShader9* shader = nullptr;
        if (!r.code.empty() &&
            FAILED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(r.code.data()), &shader))) {
            r.error = "CreatePixelShader failed (" + std::to_string(r.code.size() / 4) +
                      " tokens; the device allows " + std::to_string(g_max_slots) + " ps_3_0 instruction slots)";
            shader = nullptr;
        }
        if (!shader) {
            ++g_failed;
            if (g_error.empty()) g_error = "pixel shader " + std::to_string(t.ps) + ": " + r.error;
            continue;
        }
        const bool installed = handle_of(t) == t.ours && t.ours;
        IDirect3DPixelShader9* old = t.ours;
        if (installed) {
            code_of(t) = r.code.data();
            handle_of(t) = shader;
        }
        t.ours = shader;
        t.code = std::move(r.code);
        if (old) old->Release();
    }
    if (g_failed) spdlog::warn("[Shadows] {} receiver shaders failed: {}", g_failed, g_error);
    else spdlog::info("[Shadows] compiled {} receiver shaders", results.size());
}

// sRender's thread, on every pixel shader change: ours get the tuning values and the cascade matrices
void __stdcall on_bind(IDirect3DDevice9* device, IDirect3DPixelShader9* ps) {
    const int n = g_bind_count;
    for (int i = 0; i < n; ++i)
        if (g_bind_ours[i] == ps) {
            device->SetPixelShaderConstantF(kTuneRegister, g_bind_regs, kTuneCount);
            return;
        }
}

// The registers: c210 tuning, c211 flags, c212-c223 the cascades' u, v, z, w rows (column k of the game's matrix)
void fill_registers(float* r) {
    r[0] = settings.light_size;
    r[1] = settings.max_soft;
    r[2] = settings.min_radius;
    r[3] = 1.0f;
    r[4] = g_mat_stale < 30 ? 1.0f : 0.0f;
    r[5] = r[6] = r[7] = 0.0f;
    float m[48];
    {
        std::lock_guard<std::mutex> lock(g_mat_mutex);
        std::memcpy(m, g_frame.mats, sizeof(m));
    }
    for (int c = 0; c < 3; ++c)
        for (int k = 0; k < 4; ++k)
            for (int j = 0; j < 4; ++j) r[8 + 16 * c + 4 * k + j] = m[16 * c + 4 * j + k];
}

} // namespace

// 0xA775E0 entry, 6 bytes: push ebp / mov ebp, esp / and esp, -16. [esp] = the return address, [esp+4] the unit, +8 the
// cTrans, +0Ch the map range, +18h the matrices. After pushad they're 20h further; each push below moves them 4 more, so
// every argument is read at esp+30h in turn (last argument first).
__declspec(naked) void detour_capture() {   // not the `naked` macro: that one is static
    _asm {
        pushad
        push dword ptr [esp + 0x38]     // mats
        push dword ptr [esp + 0x30]     // range (0x2C + 4)
        push dword ptr [esp + 0x30]     // cTrans (0x28 + 8)
        push dword ptr [esp + 0x30]     // unit (0x24 + 12)
        push dword ptr [esp + 0x30]     // return address (0x20 + 16)
        call on_capture             // __stdcall
        popad
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [jmp_ret_capture]
    }
}

void init() { ps_bind::add(&on_bind); }

Status status() {
    Status s;
    s.found = g_tech != 0;
    s.waiting = g_waiting;
    s.targets = (int)g_targets.size();
    s.failed = g_failed;
    s.compiling = g_compiling;
    s.from_file = g_from_file;
    s.max_slots = g_max_slots;
    s.error = g_error;
    s.matrices = g_mat_stale < 30;
    for (const Target& t : g_targets)
        if (t.ours && handle_of(t) == t.ours) ++s.replaced;
    return s;
}

bool take_frame(Frame& out) {
    std::lock_guard<std::mutex> lock(g_mat_mutex);
    if (!g_frame_held || g_mat_stale >= 30) return false;
    out = g_frame;
    for (IDirect3DTexture9* m : out.maps)
        if (m) m->AddRef();
    return true;
}

void release_frame() {
    std::lock_guard<std::mutex> lock(g_mat_mutex);
    for (IDirect3DTexture9*& m : g_frame.maps) {
        if (m) m->Release();
        m = nullptr;
    }
    g_frame_held = false;
    g_frame_released = true;        // until update() sees the next frame
}

void request_compile() { g_want_compile = true; }

// Graphics tab > Shader files
const bool g_source_listed = shader_file::add({ "Soft shadows", kSourcePath, kEmbedded, &request_compile, 1 });

void update(bool mod_enabled) {
    {
        std::lock_guard<std::mutex> lock(g_mat_mutex);
        if (g_mat_count != g_mat_seen) { g_mat_seen = g_mat_count; g_mat_stale = 0; }
        else if (g_mat_stale < 1000) ++g_mat_stale;
        g_frame_released = false;   // the device is back (update runs from on_frame, after any reset)
    }
    if (!mod_enabled || !settings.soft) {
        for (Target& t : g_targets) restore(t);
        g_bind_count = 0;
        return;
    }
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (!device) return;
    float regs[4 * kTuneCount];
    fill_registers(regs);
    std::memcpy(g_bind_regs, regs, sizeof(regs));
    device->SetPixelShaderConstantF(kTuneRegister, regs, kTuneCount);   // also once a frame, in case the bind hook isn't there
    if (g_tech) {
        uintptr_t tech = 0;
        if (!receiver_info(tech) || tech != g_tech) forget();
    }
    if (!g_tech) {
        if (g_compiling || !locate()) return;
        D3DCAPS9 caps{};
        if (SUCCEEDED(device->GetDeviceCaps(&caps))) g_max_slots = caps.MaxPixelShader30InstructionSlots;
        g_want_compile = true;
    }
    take_results(device);
    if (g_want_compile && !g_compiling && !g_targets.empty()) start_compile();
    int n = 0;
    for (Target& t : g_targets) {
        install(t);
        if (t.ours && n < kMaxOurs) g_bind_ours[n++] = t.ours;
    }
    g_bind_count = n;
}

} // namespace shadow_receiver
