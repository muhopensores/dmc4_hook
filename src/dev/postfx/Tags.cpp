#include "Tags.hpp"
#include "../../mod.hpp"
#include "../../ModFramework.hpp"
#include "../shader_file.hpp"
#include "../../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <atomic>
#include <cctype>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace tags {
namespace {

constexpr uintptr_t kModelDraw = 0x009F6CC0;      // uModel::draw(cTrans*), thiscall
constexpr uintptr_t kTransAlloc = 0x2AB0;         // cTrans: the command allocator (moves down as commands are made)
constexpr uintptr_t kTransContext = 0x2A90;       // cTrans: the current context; its +0x1FC byte = the render mode
constexpr uint8_t   kModeLit = 1;                 // uModel::draw's case 1: setupLitPass + transDefault (the views' draws)
constexpr uintptr_t kShaderInstance = 0x00E55700; // sShader::mpInstance
constexpr uintptr_t kTechniques = 0x4838;         // sShader: mpTechnique[]
constexpr uintptr_t kZPassSlot = 0x00E55734;      // tXfMaterialZPass's slot (sShader::InitTechniques 0x8FFA00)
constexpr uintptr_t kTechInfo = 0x10;             // technique: info; [3] pixel shader count, [8] the records
constexpr UINT      kTagRegister = 220;
constexpr char kSourcePath[] = "nativePC\\shader\\postfx\\tag.hlsl";

const char kEmbedded[] =
#include "tag.inc"
    ;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// ---- IDs and the command ranges ----
// Every unit keeps its ID while it's drawn (an ID unseen for a while is given to another unit). Each lit draw appends
// the range of command records it made to one ring; a depth-pass command's model is the smallest range among the latest
// entries that holds it. (Per-frame tables rotated at FRAME_END came out of step with the draws, which the game builds
// on job threads around that point: a table held two frames' halves and the IDs mapped to the wrong units.)

struct Entry {
    uintptr_t lo, hi;   // the command records the draw made: [lo, hi)
    uintptr_t unit;
    uint8_t id;
};
constexpr uint32_t kRing = 4096;
constexpr uint32_t kScan = 1024;           // the latest entries looked at (a few frames' draws)
Entry g_ring[kRing];
std::atomic<uint32_t> g_head{ 0 };         // entries written so far (the next slot = g_head % kRing)
std::mutex g_push;                         // draws may come from several threads

struct Slot {
    uintptr_t unit = 0;
    uint32_t last = 0;                     // g_draws when it last drew
};
Slot g_ids[256];                           // [id]; 0 unused (untagged)
uint32_t g_draws = 0;                      // lit draws so far (guarded by g_push)
constexpr uint32_t kForget = 20000;        // draws after which an unseen unit's ID is free again
constexpr uint32_t kLately = 4000;         // draws within which a unit counts as still drawn (a few frames)
std::atomic<int> g_live{ 0 };              // IDs in use (for the panel)

using DrawFn = void(__fastcall*)(void* self, void* edx, void* trans);
DrawFn g_draw = nullptr;
std::unique_ptr<FunctionHook> g_hook;
bool g_hooked = false;

// under g_push
uint8_t id_for(uintptr_t unit) {
    int free_id = 0, oldest = 0;
    for (int i = 1; i < 256; ++i) {
        if (g_ids[i].unit == unit) { g_ids[i].last = g_draws; return (uint8_t)i; }
        if (!g_ids[i].unit) { if (!free_id) free_id = i; }
        else if (!oldest || g_ids[i].last < g_ids[oldest].last) oldest = i;
    }
    int id = free_id;
    if (!id && oldest && g_draws - g_ids[oldest].last > kForget) id = oldest;
    if (!id) return 0;                     // all 255 in use: untagged
    g_ids[id] = { unit, g_draws };
    int live = 0;
    for (int i = 1; i < 256; ++i) live += g_ids[i].unit ? 1 : 0;
    g_live = live;
    return (uint8_t)id;
}

void __fastcall draw_detour(void* self, void* edx, void* trans) {
    const uintptr_t t = reinterpret_cast<uintptr_t>(trans);
    const uintptr_t ctx = t ? at<uintptr_t>(t, kTransContext) : 0;
    const uint8_t mode = ctx ? at<uint8_t>(ctx, 0x1FC) : 0;
    const uintptr_t before = t ? at<uintptr_t>(t, kTransAlloc) : 0;
    g_draw(self, edx, trans);
    if (mode != kModeLit || !t) return;
    const uintptr_t after = at<uintptr_t>(t, kTransAlloc);
    if (after >= before) return;   // nothing drawn
    std::lock_guard lock(g_push);
    ++g_draws;
    const uintptr_t unit = reinterpret_cast<uintptr_t>(self);
    const uint8_t id = id_for(unit);
    const uint32_t h = g_head.load(std::memory_order_relaxed);
    g_ring[h % kRing] = { after, before, unit, id };
    g_head.store(h + 1, std::memory_order_release);
}

// ---- render thread ----

uint32_t g_set = 0xFFFF;  // c220's ID now (0xFFFF = unknown)

uint8_t find(uintptr_t cmd) {
    const uint32_t h = g_head.load(std::memory_order_acquire);
    const uint32_t n = h < kScan ? h : kScan;
    // newest first, the first range holding it: older entries cover the same addresses from earlier frames (the command
    // buffers are reused) with other models, so "the smallest range" picked wrong ones. uModel::draw doesn't nest.
    for (uint32_t k = 1; k <= n; ++k) {
        const Entry& e = g_ring[(h - k) % kRing];
        if (cmd >= e.lo && cmd < e.hi) return e.id;
    }
    return 0;
}

// ---- the depth pass's shaders ----

struct Blob : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};
using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const void*, void*, const char*, const char*, UINT,
                                      UINT, Blob**, Blob**);

D3DCompileFn compiler() {
    static D3DCompileFn fn = [] {
        HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
        return dll ? (D3DCompileFn)GetProcAddress(dll, "D3DCompile") : nullptr;
    }();
    return fn;
}

struct Target {
    uintptr_t rec = 0;                       // +0 handle, +4 bytecode
    bool alpha = false;                      // the alpha-tested one (lists gXfBlendFactor)
    IDirect3DPixelShader9* orig_handle = nullptr;
    const void* orig_code = nullptr;
    IDirect3DPixelShader9* ours = nullptr;
    std::vector<uint8_t> code;
};
std::vector<Target> g_targets;
uintptr_t g_tech = 0;
bool g_want_compile = true;
std::string g_error;

IDirect3DPixelShader9*& handle_of(const Target& t) { return at<IDirect3DPixelShader9*>(t.rec, 0); }
const void*& code_of(const Target& t) { return at<const void*>(t.rec, 4); }

// does the shader's constant table name `what`? (a plain scan of the comment block is enough here)
bool lists(const void* code, const char* what) {
    const auto* w = static_cast<const uint32_t*>(code);
    if (!w || (w[0] & 0xFFFF0000u) != 0xFFFF0000u) return false;
    if ((w[1] & 0xFFFF) != 0xFFFE) return false;
    const size_t n = (w[1] >> 16) & 0x7FFF;
    const char* b = reinterpret_cast<const char*>(&w[2]);
    return std::string(b, n * 4).find(what) != std::string::npos;
}

bool locate() {
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) return false;
    const uint32_t slot = *reinterpret_cast<uint32_t*>(kZPassSlot);
    if (slot > 255) return false;
    const uintptr_t tech = at<uintptr_t>(shader, kTechniques + 4 * slot);
    if (!tech) return false;
    const auto* info = at<const uint32_t*>(tech, kTechInfo);
    if (!info || info[3] == 0 || info[3] > 8 || !info[8]) { g_error = "tXfMaterialZPass isn't the expected one"; return false; }
    std::vector<Target> targets;
    for (uint32_t i = 0; i < info[3]; ++i) {
        Target t;
        t.rec = at<uintptr_t>(info[8], 4 * i);
        if (!t.rec || !code_of(t)) continue;
        t.alpha = lists(code_of(t), "gXfBlendFactor");
        t.orig_handle = handle_of(t);
        t.orig_code = code_of(t);
        targets.push_back(std::move(t));
    }
    g_targets = std::move(targets);
    g_tech = tech;
    g_want_compile = true;
    spdlog::info("[PostFX] object IDs: {} depth pass pixel shaders", g_targets.size());
    return true;
}

std::string read_source() {
    std::ifstream f(kSourcePath, std::ios::binary);
    if (f) {
        std::stringstream ss;
        ss << f.rdbuf();
        std::string s = ss.str();
        if (!s.empty()) return s;
    }
    return kEmbedded;
}

void compile(IDirect3DDevice9* dev) {
    g_want_compile = false;
    D3DCompileFn fn = compiler();
    if (!fn) { g_error = "d3dcompiler_47.dll not found"; return; }
    const std::string src = read_source();
    g_error.clear();
    for (Target& t : g_targets) {
        Blob* code = nullptr;
        Blob* errors = nullptr;
        const HRESULT hr = fn(src.data(), src.size(), "tag.hlsl", nullptr, nullptr, t.alpha ? "ps_alpha" : "ps_opaque", "ps_3_0",
                              1u << 15, 0, &code, &errors);
        if (FAILED(hr) || !code) {
            if (errors) g_error.assign(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());
            if (g_error.empty()) g_error = "tag.hlsl didn't compile";
        } else {
            const auto* p = static_cast<const uint8_t*>(code->GetBufferPointer());
            std::vector<uint8_t> bytes(p, p + code->GetBufferSize());
            IDirect3DPixelShader9* ps = nullptr;
            if (SUCCEEDED(dev->CreatePixelShader(reinterpret_cast<const DWORD*>(bytes.data()), &ps))) {
                const bool installed = t.ours && handle_of(t) == t.ours;
                if (installed) { code_of(t) = bytes.data(); handle_of(t) = ps; }
                if (t.ours) t.ours->Release();
                t.ours = ps;
                t.code = std::move(bytes);
            } else {
                g_error = "CreatePixelShader failed";
            }
        }
        if (errors) errors->Release();
        if (code) code->Release();
    }
}

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
    if (!t.ours || handle_of(t) != t.ours) return;
    code_of(t) = t.orig_code;
    handle_of(t) = t.orig_handle;
}

const bool g_listed = shader_file::add({ "Object IDs (depth pass)", kSourcePath, kEmbedded, [] { g_want_compile = true; }, 5 });

} // namespace

bool enabled = true;

void init() {
    if (g_hooked) return;
    g_hook = std::make_unique<FunctionHook>(kModelDraw, (uintptr_t)&draw_detour);
    if (!g_hook->create()) {
        g_error = "couldn't hook uModel::draw (0x9F6CC0)";
        spdlog::error("[PostFX] object IDs: {}", g_error);
        return;
    }
    g_draw = g_hook->get_original<std::remove_pointer_t<DrawFn>>();
    g_hooked = true;
}

void update() {
    if (g_tech) {
        const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
        const uint32_t slot = *reinterpret_cast<uint32_t*>(kZPassSlot);
        if (!shader || slot > 255 || at<uintptr_t>(shader, kTechniques + 4 * slot) != g_tech) {   // reloaded: new records
            for (Target& t : g_targets) if (t.ours) t.ours->Release();
            g_targets.clear();
            g_tech = 0;
        }
    }
    if (!enabled) {
        for (Target& t : g_targets) restore(t);
        return;
    }
    IDirect3DDevice9* dev = g_framework->get_d3d9_device();
    if (!dev) return;
    if (!g_tech && !locate()) return;
    if (g_want_compile) compile(dev);
    for (Target& t : g_targets) install(t);
}

void on_reset() {}

void on_dispatch_start() { g_set = 0xFFFF; }

void on_depth_tag(void* device, uintptr_t command) {
    if (!enabled) return;
    const uint8_t id = find(command);
    if (id == g_set) return;
    const float v[4] = { id / 255.0f, 0.0f, 0.0f, 0.0f };
    static_cast<IDirect3DDevice9*>(device)->SetPixelShaderConstantF(kTagRegister, v, 1);
    g_set = id;
}

uintptr_t unit_of(uint8_t id) {
    std::lock_guard lock(g_push);
    return id ? g_ids[id].unit : 0;
}

uint8_t id_of(uintptr_t unit) {
    if (!unit) return 0;
    std::lock_guard lock(g_push);
    for (int i = 1; i < 256; ++i)
        if (g_ids[i].unit == unit) return (uint8_t)i;
    return 0;
}

bool drawn_lately(uintptr_t unit) {
    if (!unit) return false;
    std::lock_guard lock(g_push);
    for (int i = 1; i < 256; ++i)
        if (g_ids[i].unit == unit) return g_draws - g_ids[i].last < kLately;
    return false;
}

Status status() {
    Status s;
    s.hooked = g_hooked;
    s.shaders = !g_targets.empty();
    for (const Target& t : g_targets) if (!t.ours || handle_of(t) != t.ours) s.shaders = false;
    s.last_frame_models = g_live;
    s.error = g_error;
    return s;
}

} // namespace tags
