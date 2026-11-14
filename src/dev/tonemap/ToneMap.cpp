#include "ToneMap.hpp"
#include "../../ModFramework.hpp"
#include "../ps_bind.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
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

namespace tone_map {

Settings settings;

namespace {

constexpr UINT kTuneRegister = 200;                // tonemap.inc gTmTune

constexpr uintptr_t kShaderInstance = 0x00E55700;  // sShader::mpInstance
constexpr uintptr_t kTechniqueNum   = 0x603C;
constexpr uintptr_t kTechniqueDesc  = 0x3038;      // 12 bytes each: +4 name
constexpr uintptr_t kTechniques     = 0x4838;      // mpTechnique[]
constexpr uintptr_t kTechInfo       = 0x10;
enum InfoField { kPsCount = 3, kPsRecords = 8 };
constexpr char kSourcePath[] = "nativePC\\shader\\tonemap\\tonemap.hlsl";
constexpr char kCacheDir[] = "nativePC\\shader\\tonemap\\cache";
constexpr char kCacheTag[] = "tm2 ps_3_0 O3";

const char kEmbedded[] =
#include "tonemap.inc"
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

// The instruction tokens (ps_3_0: bits 24-27 of an instruction token = its length), what the classifier needs (the same
// rules as scratch tonemap\classify2.py, which picks the 207 composites of the DX9 bfx). `sampler4` = PointSampler4's
// sampler number (-1 none): `ext_tex` = the temp a texld from it writes is read as .yyyy before being written again (TONE 2's
// extended Reinhard keeps its white in the adaptation texel's .y).
struct Scan {
    int  rcp = 0, texld = 0, defs = 0;
    bool other = false;                 // an instruction outside the composites' set
    bool f127 = false, f4 = false, ext_tex = false, ok = false;
};

Scan scan_code(const uint32_t* code, int sampler4) {
    Scan s;
    if (!code || (code[0] & 0xFFFF0000u) != 0xFFFF0000u) return s;
    constexpr uint32_t k127 = 0x3FA28F5C, k4 = 0x40800000;      // 1.26999998f, 4.0f
    auto reg_type = [](uint32_t t) { return ((t >> 28) & 7) | ((t >> 8) & 0x18); };
    int track = -1;                                              // the temp holding the adaptation texel
    for (size_t i = 1; i < 0x4000;) {
        const uint32_t tok = code[i];
        if (tok == 0x0000FFFF) { s.ok = true; break; }
        const uint32_t op = tok & 0xFFFF;
        if (op == 0xFFFE) { i += 1 + ((tok >> 16) & 0x7FFF); continue; }
        const uint32_t n = (tok >> 24) & 0xF;
        switch (op) {
        case 6: ++s.rcp; break;                                  // rcp
        case 66: ++s.texld; break;                               // texld
        case 81:                                                 // def: register, four floats
            ++s.defs;
            for (uint32_t k = 2; k < 6 && k <= n; ++k) {
                if (code[i + k] == k127) s.f127 = true;
                if (code[i + k] == k4) s.f4 = true;
            }
            break;
        case 1: case 2: case 4: case 5: case 8: case 11: case 18: case 31: break;  // mov add mad mul dp3 max lrp dcl
        default: s.other = true; break;
        }
        if (op != 31 && op != 81 && n >= 1) {
            const uint32_t dst = code[i + 1];
            if (track >= 0) {
                for (uint32_t k = 2; k <= n; ++k) {
                    const uint32_t src = code[i + k];
                    if (reg_type(src) == 0 && (int)(src & 0x7FF) == track && ((src >> 16) & 0xFF) == 0x55) s.ext_tex = true;
                }
                if (reg_type(dst) == 0 && (int)(dst & 0x7FF) == track) track = -1;
            }
            if (op == 66 && n >= 3 && sampler4 >= 0 && reg_type(code[i + 3]) == 10 &&
                (int)(code[i + 3] & 0x7FF) == sampler4 && reg_type(dst) == 0)
                track = (int)(dst & 0x7FF);
        }
        i += 1 + n;
    }
    return s;
}

// Guarded: a technique's records are read straight from the game's memory
Scan scan_guarded(const uint32_t* code, int sampler4) {
    __try {
        return scan_code(code, sampler4);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return Scan{};
    }
}

// the register a parameter is in ("s2" -> 2), -1 if the original hasn't it
int reg_number(const Defines& d, const char* name) {
    const std::string key = std::string("REG_") + name;
    for (const auto& [k, v] : d)
        if (k == key && v.size() > 1) return std::atoi(v.c_str() + 1);
    return -1;
}

// every parameter of the original is one a composite has
bool only_known(const Defines& d) {
    static const char* kKnown[] = { "PointSampler0", "PointSampler1", "PointSampler4", "LinearSampler1", "LinearSampler2",
                                    "LinearSampler3", "gXfScaleColor", "gXfMiddleGray", "gXfMaxLuminance" };
    for (const auto& [k, v] : d) {
        if (k.compare(0, 4, "HAS_") != 0) continue;
        bool ok = false;
        for (const char* n : kKnown) ok = ok || k.compare(4, std::string::npos, n) == 0;
        if (!ok) return false;
    }
    return true;
}

bool name_is_filter(const char* name) {
    __try {
        return name && std::strncmp(name, "tXfFilter", 9) == 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

struct Target {
    int       tech = -1, ps = -1;
    uintptr_t rec = 0;                      // +0 handle, +4 bytecode
    Defines   regs;                         // the original's registers and its two code features
    IDirect3DPixelShader9* orig_handle = nullptr;
    const void*            orig_code = nullptr;
    IDirect3DPixelShader9* ours = nullptr;
    std::vector<uint8_t>   code;
};
struct Result { size_t index; std::vector<uint8_t> code; std::string error; };

// render thread
std::vector<std::pair<int, uintptr_t>> g_techs;     // (slot, technique) the targets came from
std::vector<Target> g_targets;
bool                g_from_file = false;
int                 g_failed = 0;
std::string         g_error;
std::string         g_waiting = "not looked yet";

// read by the bind listener (sRender's thread)
constexpr int                   kMaxOurs = 128;
IDirect3DPixelShader9* volatile g_bind_ours[kMaxOurs] = {};
volatile int                    g_bind_count = 0;
float                           g_bind_regs[4] = {};

// worker <-> render thread
std::mutex          g_mutex;
std::vector<Result> g_results;
bool                g_results_ready = false;
int                 g_results_generation = 0;
int                 g_generation = 0;
std::atomic<bool>   g_compiling{ false };
std::atomic<bool>   g_want_compile{ true };

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
    g_techs.clear();
    ++g_generation;
}

uintptr_t shader_instance() { return *reinterpret_cast<uintptr_t*>(kShaderInstance); }

// the techniques the targets came from are still the loaded ones
bool techs_current() {
    const uintptr_t shader = shader_instance();
    if (!shader) return false;
    for (const auto& [slot, tech] : g_techs)
        if (at<uintptr_t>(shader, kTechniques + 4 * slot) != tech) return false;
    return true;
}

// The tone map pixel shaders of every tXfFilter* technique
bool locate() {
    const uintptr_t shader = shader_instance();
    if (!shader) { g_waiting = "no sShader yet"; return false; }
    const uint32_t num = at<uint32_t>(shader, kTechniqueNum);
    if (num == 0 || num > 512) { g_waiting = "no techniques yet"; return false; }
    std::vector<Target> targets;
    std::vector<std::pair<int, uintptr_t>> techs;
    for (uint32_t i = 0; i < num; ++i) {
        const uintptr_t tech = at<uintptr_t>(shader, kTechniques + 4 * i);
        if (!tech || !name_is_filter(at<const char*>(shader, kTechniqueDesc + 12 * i + 4))) continue;
        const auto* info = at<const uint32_t*>(tech, kTechInfo);
        if (!info || !info[kPsRecords] || info[kPsCount] == 0 || info[kPsCount] > 8192) continue;
        bool any = false;
        for (uint32_t ps = 0; ps < info[kPsCount]; ++ps) {
            const uintptr_t rec = at<uintptr_t>(info[kPsRecords], 4 * ps);
            if (!rec) continue;
            const auto* code = at<const uint32_t*>(rec, 4);
            Target t;
            if (!read_registers(code, t.regs) || !has(t.regs, "PointSampler0") || !only_known(t.regs)) continue;
            const bool mg = has(t.regs, "gXfMiddleGray"), ps4 = has(t.regs, "PointSampler4");
            if (mg && ps4) continue;
            const int tone = mg ? 1 : ps4 ? 2 : 0;
            const Scan s = scan_guarded(code, reg_number(t.regs, "PointSampler4"));
            if (!s.ok || s.other || s.texld > 6 || s.rcp != (tone == 0 ? 0 : 3)) continue;
            if (tone == 0 && (has(t.regs, "gXfMaxLuminance") || s.defs == 0 ||
                              !(has(t.regs, "PointSampler1") || has(t.regs, "LinearSampler1") ||
                                has(t.regs, "LinearSampler2") || has(t.regs, "LinearSampler3"))))
                continue;                        // a plain copy or some other filter
            t.regs.emplace_back("TONE", std::to_string(tone));
            t.regs.emplace_back("EXT_TEX", (tone == 2 && s.ext_tex) ? "1" : "0");
            t.regs.emplace_back("SCOTOPIC", s.f127 ? "1" : "0");
            t.regs.emplace_back("BLEND_ALPHA", (s.f4 && has(t.regs, "PointSampler1")) ? "1" : "0");
            t.tech = (int)i;
            t.ps = (int)ps;
            t.rec = rec;
            t.orig_handle = handle_of(t);
            t.orig_code = code_of(t);
            targets.push_back(std::move(t));
            any = true;
        }
        if (any) techs.emplace_back((int)i, tech);
    }
    if (targets.empty()) { g_waiting = "no final composite shaders found in the tXfFilter techniques"; return false; }
    g_targets = std::move(targets);
    g_techs = std::move(techs);
    g_waiting.clear();
    spdlog::info("[ToneMap] {} final composite pixel shaders to replace", g_targets.size());
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
    auto jobs = std::make_shared<std::vector<Defines>>();
    for (const Target& t : g_targets) jobs->push_back(t.regs);
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
                const HRESULT hr = fn(source->data(), source->size(), "tonemap.hlsl", macros.data(), nullptr, "main",
                                      "ps_3_0", kOptimize3, 0, &code, &errors);
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
    if (g_failed) spdlog::warn("[ToneMap] {} shaders failed: {}", g_failed, g_error);
    else spdlog::info("[ToneMap] compiled {} tone map shaders", results.size());
}

// sRender's thread, on every pixel shader change: ours get the settings
void __stdcall on_bind(IDirect3DDevice9* device, IDirect3DPixelShader9* ps) {
    const int n = g_bind_count;
    for (int i = 0; i < n; ++i)
        if (g_bind_ours[i] == ps) {
            device->SetPixelShaderConstantF(kTuneRegister, g_bind_regs, 1);
            return;
        }
}

void update(bool on) {
    const float ev = std::clamp(settings.ev, -4.0f, 4.0f);
    g_bind_regs[0] = (float)std::clamp(settings.curve, 0, 2);
    g_bind_regs[1] = std::exp2(ev);                 // a light factor
    g_bind_regs[2] = std::exp2(ev / 2.2f);          // the same in gamma (for the stock curve)
    g_bind_regs[3] = 1.0f;
    if (!g_targets.empty() && !techs_current()) forget();
    if (!on) {
        for (Target& t : g_targets) restore(t);
        g_bind_count = 0;
        return;
    }
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (!device) return;
    if (g_targets.empty()) {
        if (g_compiling || !locate()) return;
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

bool write_source() {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(kSourcePath).parent_path(), ec);
    std::ofstream f(kSourcePath, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(kEmbedded, (std::streamsize)std::strlen(kEmbedded));
    f.close();
    if (!f) return false;
    g_want_compile = true;
    return true;
}

} // namespace

Status status() {
    Status s;
    s.found = !g_targets.empty();
    s.waiting = g_waiting;
    s.targets = (int)g_targets.size();
    s.failed = g_failed;
    s.compiling = g_compiling;
    s.from_file = g_from_file;
    s.error = g_error;
    for (const Target& t : g_targets)
        if (t.ours && handle_of(t) == t.ours) ++s.replaced;
    return s;
}

} // namespace tone_map

using namespace tone_map;

std::optional<std::string> ToneMap::on_initialize() {
    ps_bind::add(&on_bind);
    return Mod::on_initialize();
}

void ToneMap::on_frame(fmilliseconds& dt) { update(settings.on); }

void ToneMap::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Tone map"), &settings.on);
    ImGui::SameLine();
    help_marker(_("Replaces the game's tone map (how the scene's light becomes screen colours) with a filmic curve: bright "
                  "colours roll off toward white instead of flattening and shifting hue. The game's exposure, bloom, "
                  "afterimage blend and night-time blue shift stay as they are. Best with PbrMaterials' Scene buffer at "
                  "16-bit float, which keeps light above white for the curve to work with."));
    if (!settings.on) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("ToneMap");
    const Status st = status();
    if (!st.found) {
        ImGui::TextDisabled(_("Waiting for the shaders: %s"), st.waiting.c_str());
    } else {
        ImGui::TextDisabled(_("Tone map shaders: %d of %d replaced%s%s"), st.replaced, st.targets,
                            st.compiling ? ", compiling..." : "", st.from_file ? " (from nativePC)" : "");
        if (st.failed) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), _("%d failed: %s"), st.failed, st.error.c_str());
    }
    static const char* kCurves[] = { "Game (Reinhard)", "ACES (filmic)", "AgX (filmic, truer colours)" };
    ImGui::PushItemWidth(200.0f);
    ImGui::Combo(_("Curve"), &settings.curve, kCurves, 3);
    ImGui::SameLine();
    help_marker(_("Game: the original curve (with Exposure applied). ACES: contrasty film look, bright colours saturate. AgX: "
                  "softer, bright colours desaturate toward white as on film. Applied live. Each keeps the game's mid "
                  "tones at Exposure 0."));
    ImGui::SliderFloat(_("Exposure"), &settings.ev, -3.0f, 3.0f, "%+.1f EV");
    ImGui::SameLine();
    help_marker(_("Brighter or darker than the game's own exposure, in stops (+1 = twice the light)."));
    ImGui::PopItemWidth();
    if (ImGui::Button(_("Write shader source"))) write_source();
    ImGui::SameLine();
    if (ImGui::Button(_("Reload"))) g_want_compile = true;
    ImGui::SameLine();
    help_marker(_("Write shader source puts the shader in nativePC\\shader\\tonemap\\tonemap.hlsl, which is used from then "
                  "on instead of the built-in one; Reload compiles it again after an edit (delete the file to go back)."));
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void ToneMap::on_config_load(const utility::Config& cfg) {
    const Settings d;
    settings.on = cfg.get<bool>("tonemap_on").value_or(d.on);
    settings.curve = std::clamp(cfg.get<int>("tonemap_curve").value_or(d.curve), 0, 2);
    settings.ev = std::clamp(cfg.get<float>("tonemap_ev").value_or(d.ev), -3.0f, 3.0f);
}

void ToneMap::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("tonemap_on", settings.on);
    cfg.set<int>("tonemap_curve", settings.curve);
    cfg.set<float>("tonemap_ev", settings.ev);
}
