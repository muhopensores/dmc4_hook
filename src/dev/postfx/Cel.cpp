#include "Cel.hpp"
#include "../../mod.hpp"
#include "../../ModFramework.hpp"
#include "../ps_bind.hpp"
#include "../shader_file.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace cel {

Settings settings;

namespace {

constexpr UINT kTuneRegister = 210;                // cel.inc gCelTune0 / gCelTune1
constexpr uintptr_t kShaderInstance = 0x00E55700;  // sShader::mpInstance
constexpr uintptr_t kTechniques     = 0x4838;      // sShader: mpTechnique[]
constexpr uintptr_t kStandardSlot   = 0x00E55678;  // tXfMaterialStandard's slot (sShader::InitTechniques 0x8FFA00)
constexpr uintptr_t kTechInfo       = 0x10;
enum InfoField { kComponents = 1, kPermutations = 2, kPsCount = 3, kComponentRecs = 5, kTable = 6, kPsRecords = 8 };
constexpr uint32_t kRadix[6]  = { 7, 5, 4, 5, 8, 7 };          // skin, lighting, normal map, specular, lightmap, multitexture
constexpr uint32_t kStride[6] = { 1, 7, 35, 140, 700, 5600 };
constexpr uint32_t kPowmap = 3;
constexpr char kSourcePath[] = "nativePC\\shader\\postfx\\cel.hlsl";

const char kEmbedded[] =
#include "cel.inc"
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

// The original's constant table: HAS_<name> / REG_<name> per parameter (the reader PbrMaterials / ToneMap use)
bool read_registers(const uint32_t* code, Defines& out) {
    if (!code || (code[0] & 0xFFFF0000u) != 0xFFFF0000u) return false;
    for (size_t i = 1; i < 0x4000;) {
        const uint32_t tok = code[i];
        if ((tok & 0xFFFF) != 0xFFFE) break;
        const uint32_t n = (tok >> 16) & 0x7FFF;
        if (n >= 8 && code[i + 1] == 0x42415443u) {                      // 'CTAB'
            const auto* base = reinterpret_cast<const uint8_t*>(&code[i + 2]);
            const uint32_t count = *reinterpret_cast<const uint32_t*>(base + 12);
            const uint32_t info = *reinterpret_cast<const uint32_t*>(base + 16);
            for (uint32_t k = 0; k < count && k < 64; ++k) {
                const uint8_t* c = base + info + 20 * k;
                const uint32_t name = *reinterpret_cast<const uint32_t*>(c);
                const uint16_t set = *reinterpret_cast<const uint16_t*>(c + 4);
                const uint16_t index = *reinterpret_cast<const uint16_t*>(c + 6);
                if (set > 3) return false;
                std::string s;
                for (uint32_t p = name; base[p] && s.size() < 64; ++p) {
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

struct Target {
    int       ps = -1;
    uintptr_t rec = 0;                      // the pixel shader record: +0 handle, +4 bytecode
    uint8_t   lighting = 0, normalmap = 0, lightmap = 0;
    Defines   regs;
    IDirect3DPixelShader9* orig_handle = nullptr;
    const void*            orig_code = nullptr;
    IDirect3DPixelShader9* ours = nullptr;
    std::vector<uint8_t>   code;            // ours, alive while the record points at it
};

struct Route {
    uint32_t key;
    uint32_t orig;                          // the table entry as the game had it
    uint32_t patched;                       // the twin's pixel shader, the key's own vertex shader
};

struct Result { size_t index; std::vector<uint8_t> code; std::string error; };

// render / frame thread
uintptr_t           g_tech = 0;
uint32_t*           g_table = nullptr;
std::vector<Target> g_targets;
std::vector<Route>  g_routes;
bool                g_routed = false;
bool                g_from_file = false;
int                 g_failed = 0;
std::string         g_error;
std::string         g_waiting = "not looked yet";
int                 g_generation = 0;

// worker <-> frame thread
std::mutex          g_mutex;
std::vector<Result> g_results;
bool                g_results_ready = false;
int                 g_results_generation = 0;
std::atomic<bool>   g_compiling{ false };
std::atomic<bool>   g_want_compile{ true };

// read by the bind listener (sRender's thread)
constexpr int                   kMaxOurs = 32;
IDirect3DPixelShader9* volatile g_bind_ours[kMaxOurs] = {};
volatile int                    g_bind_count = 0;
float                           g_bind_tune[8] = {};

IDirect3DPixelShader9*& handle_of(const Target& t) { return at<IDirect3DPixelShader9*>(t.rec, 0); }
const void*&            code_of(const Target& t) { return at<const void*>(t.rec, 4); }

void __stdcall on_bind_ps(IDirect3DDevice9* device, IDirect3DPixelShader9* ps) {
    const int n = g_bind_count;
    for (int i = 0; i < n; ++i)
        if (g_bind_ours[i] == ps) {
            device->SetPixelShaderConstantF(kTuneRegister, g_bind_tune, 2);
            return;
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
    IDirect3DPixelShader9*& h = handle_of(t);
    if (h != t.ours || !t.ours) return;
    code_of(t) = t.orig_code;
    h = t.orig_handle;
}

void route(bool on) {
    if (!g_table || g_routed == on) return;
    for (const Route& r : g_routes) g_table[r.key] = on ? r.patched : r.orig;
    g_routed = on;
}

void forget() {
    route(false);
    for (Target& t : g_targets)
        if (t.ours) t.ours->Release();
    g_targets.clear();
    g_routes.clear();
    g_table = nullptr;
    g_tech = 0;
    g_bind_count = 0;
    ++g_generation;
}

const uint32_t* standard_info(uintptr_t& tech) {
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) { g_waiting = "no sShader yet"; return nullptr; }
    const uint32_t slot = *reinterpret_cast<uint32_t*>(kStandardSlot);
    if (slot > 255) { g_waiting = "tXfMaterialStandard's slot not set"; return nullptr; }
    tech = at<uintptr_t>(shader, kTechniques + 4 * slot);
    if (!tech) { g_waiting = "tXfMaterialStandard not loaded"; return nullptr; }
    const auto* info = at<const uint32_t*>(tech, kTechInfo);
    if (!info || info[kComponents] != 6 || info[kPermutations] != 39200 || !info[kTable] || !info[kPsRecords] ||
        !info[kComponentRecs]) {
        g_waiting = "tXfMaterialStandard isn't the expected one";
        return nullptr;
    }
    for (int i = 0; i < 6; ++i) {
        const uint32_t word = at<uint32_t>(info[kComponentRecs], 12 * i + 4);
        if ((word & 0xFFFFFF) != kStride[i] || (word >> 24) != kRadix[i]) {
            g_waiting = "tXfMaterialStandard's components aren't the expected ones";
            return nullptr;
        }
    }
    g_waiting.clear();
    return info;
}

void decode(uint32_t key, uint32_t* sel) {
    for (int i = 0; i < 6; ++i) { sel[i] = key % kRadix[i]; key /= kRadix[i]; }
}

// The records and keys (the same rules as the scratch check postfx/cel_check.py, which compiled all 6 cleanly)
bool locate() {
    uintptr_t tech = 0;
    const uint32_t* info = standard_info(tech);
    if (!info) return false;
    const uint32_t perms = info[kPermutations], ps_count = info[kPsCount];
    auto* table = reinterpret_cast<uint32_t*>(info[kTable]);
    // pixel shaders that only skinned POWMAP keys use
    std::vector<uint8_t> powmap_only(ps_count, 1);
    for (uint32_t key = 0; key < perms; ++key) {
        const uint32_t ps = table[key] & 0xFFFF;
        if (ps >= ps_count) continue;
        uint32_t s[6];
        decode(key, s);
        if (s[3] != kPowmap || s[0] == 0) powmap_only[ps] = 0;
    }
    std::vector<int> target_of(ps_count, -1);
    std::vector<Target> targets;
    std::vector<Route> routes;
    for (uint32_t key = 0; key < perms; ++key) {
        uint32_t s[6];
        decode(key, s);
        if (s[0] == 0 || s[1] == 0 || s[3] > 1 || (table[key] & 0xFFFF) >= ps_count) continue;
        if (s[2] > 1 || (s[4] != 0 && s[4] != 5) || s[5] != 0) continue;    // what cel.inc reads
        const uint32_t twin = key + (kPowmap - s[3]) * kStride[3];
        const uint32_t tps = table[twin] & 0xFFFF;
        if (tps >= ps_count || !powmap_only[tps] || (table[twin] >> 16) != (table[key] >> 16)) continue;
        int& ti = target_of[tps];
        if (ti < 0) {
            Target t;
            t.ps = (int)tps;
            t.rec = at<uintptr_t>(info[kPsRecords], 4 * tps);
            t.lighting = (uint8_t)s[1];
            t.normalmap = (uint8_t)s[2];
            t.lightmap = (uint8_t)s[4];
            if (!t.rec || !read_registers(at<const uint32_t*>(t.rec, 4), t.regs)) continue;
            t.orig_handle = handle_of(t);
            t.orig_code = code_of(t);
            ti = (int)targets.size();
            targets.push_back(std::move(t));
        }
        const Target& t = targets[ti];
        if (t.lighting != s[1] || t.normalmap != s[2] || t.lightmap != s[4]) continue;   // compiled for another permutation
        routes.push_back({ key, table[key], (table[key] & 0xFFFF0000u) | tps });
    }
    g_targets = std::move(targets);
    g_routes = std::move(routes);
    g_table = table;
    g_tech = tech;
    g_routed = false;
    spdlog::info("[PostFX] cel shading: {} pixel shaders, {} character permutations", g_targets.size(), g_routes.size());
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

void start_compile() {
    D3DCompileFn fn = compiler();
    g_want_compile = false;
    if (!fn) { g_error = "d3dcompiler_47.dll not found"; return; }
    auto source = std::make_shared<std::string>(read_source(g_from_file));
    auto jobs = std::make_shared<std::vector<Defines>>();
    for (const Target& t : g_targets) {
        Defines d = { { "LIGHTING", std::to_string(t.lighting) }, { "NORMALMAP", std::to_string(t.normalmap) },
                      { "LIGHTMAP", std::to_string(t.lightmap) } };
        d.insert(d.end(), t.regs.begin(), t.regs.end());
        jobs->push_back(std::move(d));
    }
    g_compiling = true;
    const int generation = g_generation;
    std::thread([fn, source, jobs, generation] {
        std::vector<Result> out(jobs->size());
        for (size_t i = 0; i < jobs->size(); ++i) {
            Result& r = out[i];
            r.index = i;
            std::vector<Macro> macros;
            for (const auto& [name, value] : (*jobs)[i]) macros.push_back({ name.c_str(), value.c_str() });
            macros.push_back({ nullptr, nullptr });
            Blob* code = nullptr;
            Blob* errors = nullptr;
            const HRESULT hr = fn(source->data(), source->size(), "cel.hlsl", macros.data(), nullptr, "main", "ps_3_0",
                                  kOptimize3, 0, &code, &errors);
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
        }
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
        if (!r.code.empty() && FAILED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(r.code.data()), &shader))) {
            r.error = "CreatePixelShader failed";
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
    if (g_failed) spdlog::warn("[PostFX] cel shading: {} shaders failed: {}", g_failed, g_error);
}

bool g_init = false;
const bool g_source_listed = shader_file::add({ "Cel shading", kSourcePath, kEmbedded, [] { g_want_compile = true; }, 4 });

} // namespace

void init() {
    if (g_init) return;
    g_init = true;
    ps_bind::add(&on_bind_ps);
}

void update() {
    if (g_tech) {                                   // the technique reloaded: its records are new
        uintptr_t tech = 0;
        if (!standard_info(tech) || tech != g_tech) forget();
    }
    if (!settings.on) {
        route(false);
        for (Target& t : g_targets) restore(t);
        g_bind_count = 0;
        return;
    }
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (!device) return;
    const float tune[8] = { settings.edge, std::max(settings.soft, 0.001f), settings.flat, settings.highlight,
                            settings.shaded, settings.hl_size, settings.amb_shade, 1.0f };
    std::memcpy(g_bind_tune, tune, sizeof tune);
    device->SetPixelShaderConstantF(kTuneRegister, tune, 2);
    if (!g_tech) {
        if (g_compiling || !locate()) return;
        g_want_compile = true;
    }
    take_results(device);
    if (g_want_compile && !g_compiling && !g_targets.empty()) start_compile();
    int n = 0;
    bool all = !g_targets.empty();
    for (Target& t : g_targets) {
        install(t);
        if (t.ours && n < kMaxOurs) g_bind_ours[n++] = t.ours;
        if (!t.ours) all = false;
    }
    g_bind_count = n;
    route(all);                                     // only once every target has ours: else a route would draw stock POWMAP
}

Status status() {
    Status s;
    s.found = g_tech != 0;
    s.waiting = g_waiting;
    s.targets = (int)g_targets.size();
    s.routes = g_routed ? (int)g_routes.size() : 0;
    s.failed = g_failed;
    s.compiling = g_compiling;
    s.from_file = g_from_file;
    s.error = g_error;
    for (const Target& t : g_targets)
        if (t.ours && handle_of(t) == t.ours) ++s.replaced;
    return s;
}

void draw_gui() {
    ImGui::Checkbox(_("Cel shading (characters)"), &settings.on);
    ImGui::SameLine();
    Mod::help_marker(_("Players, enemies and their weapons lit like a cartoon: each light either lights a surface or doesn't, "
                       "with a soft edge between, the surroundings' light evened out, and a hard-edged highlight. Stages keep "
                       "their lighting. Goes well with Outlines."));
    if (!settings.on) return;
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID("PostFX_Cel");
    ImGui::SliderFloat(_("Shade line"), &settings.edge, -0.5f, 0.8f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("Where light turns to shade: higher = more of each character in shade."));
    ImGui::SliderFloat(_("Edge softness"), &settings.soft, 0.0f, 0.3f, "%.2f");
    ImGui::SliderFloat(_("Shade brightness"), &settings.shaded, 0.0f, 0.8f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("How much of the light the shaded side keeps (on top of the surroundings' light)."));
    ImGui::SliderFloat(_("Ambient shade"), &settings.amb_shade, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("The surroundings' light splits into two tones too: the side facing away from where most of it comes from "
                       "gets this much of it (1 = no split). DMC4 lights characters mostly this way, so this makes most of the "
                       "cel look."));
    ImGui::SliderFloat(_("Flat ambient"), &settings.flat, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("0 = the surroundings' light varies with the surface's direction (as the game), 1 = one even colour."));
    ImGui::SliderFloat(_("Highlight"), &settings.highlight, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat(_("Highlight size"), &settings.hl_size, 0.05f, 0.95f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("Higher = a smaller highlight."));
    const Status st = status();
    if (!st.found)
        ImGui::TextDisabled(_("Waiting: %s"), st.waiting.c_str());
    else if (!st.error.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", st.error.c_str());
    else if (st.compiling || st.replaced < st.targets)
        ImGui::TextDisabled(_("Compiling..."));
    else
        ImGui::TextDisabled(_("%d shaders, %d character permutations%s"), st.replaced, st.routes,
                            st.from_file ? _(", from the shader file") : "");
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
}

void load(const utility::Config& cfg) {
    const Settings d;
    settings.on = cfg.get<bool>("postfx_cel").value_or(d.on);
    settings.edge = std::clamp(cfg.get<float>("postfx_cel_edge").value_or(d.edge), -0.5f, 0.8f);
    settings.soft = std::clamp(cfg.get<float>("postfx_cel_soft").value_or(d.soft), 0.0f, 0.3f);
    settings.shaded = std::clamp(cfg.get<float>("postfx_cel_shaded").value_or(d.shaded), 0.0f, 0.8f);
    settings.flat = std::clamp(cfg.get<float>("postfx_cel_flat").value_or(d.flat), 0.0f, 1.0f);
    settings.highlight = std::clamp(cfg.get<float>("postfx_cel_highlight").value_or(d.highlight), 0.0f, 2.0f);
    settings.hl_size = std::clamp(cfg.get<float>("postfx_cel_hl_size").value_or(d.hl_size), 0.05f, 0.95f);
    settings.amb_shade = std::clamp(cfg.get<float>("postfx_cel_amb_shade").value_or(d.amb_shade), 0.0f, 1.0f);
}

void save(utility::Config& cfg) {
    cfg.set<bool>("postfx_cel", settings.on);
    cfg.set<float>("postfx_cel_edge", settings.edge);
    cfg.set<float>("postfx_cel_soft", settings.soft);
    cfg.set<float>("postfx_cel_shaded", settings.shaded);
    cfg.set<float>("postfx_cel_flat", settings.flat);
    cfg.set<float>("postfx_cel_highlight", settings.highlight);
    cfg.set<float>("postfx_cel_hl_size", settings.hl_size);
    cfg.set<float>("postfx_cel_amb_shade", settings.amb_shade);
}

} // namespace cel
