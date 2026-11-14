#include "PbrMaterials.hpp"
#include "../../ModFramework.hpp"
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

namespace pbr_materials {

bool mod_enabled = true;
bool linear_light = true;
bool simple_specular = true;
int  reflect_mode = 0;
int  debug_view = 0;

namespace {

constexpr uintptr_t kShaderInstance = 0x00E55700;  // sShader::mpInstance
constexpr uintptr_t kTechniques     = 0x4838;      // sShader: mpTechnique[]
constexpr uintptr_t kStandardSlot   = 0x00E55678;  // tXfMaterialStandard's slot (sShader::InitTechniques 0x8FFA00)
constexpr uintptr_t kTechInfo       = 0x10;        // technique: its permutation info
enum InfoField { kComponents = 1, kPermutations = 2, kPsCount = 3, kComponentRecs = 5, kTable = 6, kPsRecords = 8 };
// skin, lighting, normal map, specular, lightmap, multitexture. A component record (12 bytes) holds its place value in
// bits 0-23 of +4 (decodeStateKey divides the key by it, sub_A61E40 sums selector x it) and its value count in the
// byte at +7 (sub_A62570 matches that many names): XfMaterialStandard.bfx 0x1F8
constexpr uint32_t kRadix[6] = { 7, 5, 4, 5, 8, 7 };
constexpr uint32_t kStride[6] = { 1, 7, 35, 140, 700, 5600 };
constexpr uint32_t kRim = 4;
constexpr char kSourcePath[] = "nativePC\\shader\\pbr\\material_pbr.hlsl";
constexpr char kCacheDir[] = "nativePC\\shader\\pbr\\cache";   // compiled shaders, by a hash of source + defines
constexpr char kCacheTag[] = "pbr1 ps_3_0 O3";                    // part of the hash: bump when the compile changes

const char kEmbedded[] =
#include "material_pbr.inc"
    ;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// d3dcompiler_47's D3DCompile, loaded at run time (no link dependency): ID3DBlob and D3D_SHADER_MACRO as it declares them
struct Blob : IUnknown {
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};
struct Macro { const char* name; const char* definition; };
using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const Macro*, void*, const char*, const char*, UINT,
                                      UINT, Blob**, Blob**);
constexpr UINT kOptimize3 = 1u << 15;               // D3DCOMPILE_OPTIMIZATION_LEVEL3

D3DCompileFn compiler() {
    static D3DCompileFn fn = [] {
        HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
        return dll ? (D3DCompileFn)GetProcAddress(dll, "D3DCompile") : nullptr;
    }();
    return fn;
}

using Defines = std::vector<std::pair<std::string, std::string>>;

// The original's constant table (the 'CTAB' comment after the version token): HAS_<name> / REG_<name> per parameter
bool read_registers(const uint32_t* code, Defines& out) {
    if (!code || (code[0] & 0xFFFF0000u) != 0xFFFF0000u) return false;   // a pixel shader version token
    for (size_t i = 1; i < 0x4000;) {
        const uint32_t tok = code[i];
        if ((tok & 0xFFFF) != 0xFFFE) break;                             // past the comments
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

struct Target {
    int       ps = -1;
    uintptr_t rec = 0;                      // the pixel shader record: +0 handle, +4 bytecode
    uint8_t   sel[6] = {};                  // the permutation it was found under
    Defines   regs;
    IDirect3DPixelShader9* orig_handle = nullptr;
    const void*            orig_code = nullptr;
    IDirect3DPixelShader9* ours = nullptr;
    std::vector<uint8_t>   code;            // ours, alive while the record points at it
};

struct Result { size_t index; std::vector<uint8_t> code; std::string error; };


// render thread
uintptr_t           g_tech = 0;
std::vector<Target> g_targets;
int                 g_unsupported = 0;
bool                g_from_file = false;
unsigned            g_max_slots = 0;
int                 g_failed = 0;
std::string         g_error;
std::string         g_waiting = "not looked yet";   // why standard_info() fails, for the panel

// worker <-> render thread
std::mutex          g_mutex;
std::vector<Result> g_results;
bool                g_results_ready = false;
int                 g_results_generation = 0;
int                 g_generation = 0;   // render thread: bumped when the records go away
std::atomic<bool>   g_compiling{ false };
std::atomic<bool>   g_want_compile{ true };

IDirect3DPixelShader9*& handle_of(const Target& t) { return at<IDirect3DPixelShader9*>(t.rec, 0); }
const void*&            code_of(const Target& t) { return at<const void*>(t.rec, 4); }

void install(Target& t) {
    if (!t.ours) return;
    IDirect3DPixelShader9*& h = handle_of(t);
    const void*& c = code_of(t);
    if (h == t.ours && c == t.code.data()) return;
    if (h != t.ours) t.orig_handle = h;         // the game's (0 until it first drew with it; it may have made one since)
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
    for (Target& t : g_targets) {
        if (t.ours) t.ours->Release();
    }
    g_targets.clear();
    g_unsupported = 0;
    g_tech = 0;
    ++g_generation;                             // a compile still running was for the old records
}

// The technique's permutation info, checked against the radices XfMaterialStandard.bfx has
const uint32_t* standard_info(uintptr_t& tech) {
    char why[160];
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) { g_waiting = "no sShader yet"; return nullptr; }
    const uint32_t slot = *reinterpret_cast<uint32_t*>(kStandardSlot);
    if (slot > 255) {
        snprintf(why, sizeof why, "technique slot %u (0xE55678) not set", slot);
        g_waiting = why;
        return nullptr;
    }
    tech = at<uintptr_t>(shader, kTechniques + 4 * slot);
    if (!tech) {
        snprintf(why, sizeof why, "tXfMaterialStandard (slot %u) not loaded", slot);
        g_waiting = why;
        return nullptr;
    }
    const auto* info = at<const uint32_t*>(tech, kTechInfo);
    if (!info || info[kComponents] != 6 || info[kPermutations] != 39200 || !info[kTable] || !info[kPsRecords] ||
        !info[kComponentRecs]) {
        snprintf(why, sizeof why, "slot %u: technique info %p: components %u, permutations %u (want 6, 39200)", slot,
                 (const void*)info, info ? info[kComponents] : 0, info ? info[kPermutations] : 0);
        g_waiting = why;
        return nullptr;
    }
    for (int i = 0; i < 6; ++i) {
        const uint32_t word = at<uint32_t>(info[kComponentRecs], 12 * i + 4);
        if ((word & 0xFFFFFF) != kStride[i] || (word >> 24) != kRadix[i]) {
            snprintf(why, sizeof why, "component %d: place value %u radix %u (want %u, %u)", i, word & 0xFFFFFF, word >> 24,
                     kStride[i], kRadix[i]);
            g_waiting = why;
            return nullptr;
        }
    }
    g_waiting.clear();
    return info;
}

bool supported(const uint8_t* sel) {
    const uint8_t lightmap = sel[4];
    return sel[2] <= 1 && (lightmap == 0 || lightmap == 1 || lightmap == 2 || lightmap == 5) && sel[5] == 0;
}

// Collect the pixel shaders only lit RIM keys use (the others are shared with vanilla permutations: never touched)
bool locate() {
    uintptr_t tech = 0;
    const uint32_t* info = standard_info(tech);
    if (!info) return false;
    const uint32_t perms = info[kPermutations], ps_count = info[kPsCount];
    const auto* table = reinterpret_cast<const uint32_t*>(info[kTable]);
    std::vector<int> owner(ps_count, -1);       // the first lit RIM key per pixel shader
    std::vector<bool> other(ps_count, false);   // used by any other key
    for (uint32_t key = 0; key < perms; ++key) {
        const uint32_t ps = table[key] & 0xFFFF;
        if (ps >= ps_count) continue;
        const uint32_t specular = key / 140 % 5, lighting = key / 7 % 5;
        if (specular == kRim && lighting != 0) {
            if (owner[ps] < 0) owner[ps] = (int)key;
        } else {
            other[ps] = true;
        }
    }
    std::vector<Target> targets;
    int unsupported = 0;
    for (uint32_t ps = 0; ps < ps_count; ++ps) {
        if (owner[ps] < 0 || other[ps]) continue;
        Target t;
        t.ps = (int)ps;
        uint32_t k = (uint32_t)owner[ps];
        for (int i = 0; i < 6; ++i) { t.sel[i] = (uint8_t)(k % kRadix[i]); k /= kRadix[i]; }
        t.rec = at<uintptr_t>(info[kPsRecords], 4 * ps);
        if (!t.rec || !supported(t.sel) || !read_registers(at<const uint32_t*>(t.rec, 4), t.regs)) { ++unsupported; continue; }
        t.orig_handle = handle_of(t);
        t.orig_code = code_of(t);
        targets.push_back(std::move(t));
    }
    g_targets = std::move(targets);
    g_unsupported = unsupported;
    g_tech = tech;
    spdlog::info("[PbrMaterials] {} lit RIM pixel shaders to replace, {} left stock (unsupported)", g_targets.size(), unsupported);
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

// Worker thread: compile every target's permutation of the source
void start_compile() {
    D3DCompileFn fn = compiler();
    if (!fn) {
        g_error = "d3dcompiler_47.dll not found";
        g_want_compile = false;
        return;
    }
    auto source = std::make_shared<std::string>(read_source(g_from_file));
    Defines common = {
        { "PBR_LINEAR", linear_light ? "1" : "0" },
        { "PBR_SIMPLE_SPEC", simple_specular ? "1" : "0" },
        { "PBR_REFLECT", std::to_string(reflect_mode) },
        { "PBR_DEBUG", std::to_string(debug_view) },
    };
    auto jobs = std::make_shared<std::vector<Defines>>();
    for (const Target& t : g_targets) {
        Defines d = common;
        d.emplace_back("LIGHTING", std::to_string(t.sel[1]));
        d.emplace_back("NORMALMAP", std::to_string(t.sel[2]));
        d.emplace_back("LIGHTMAP", std::to_string(t.sel[4]));
        d.insert(d.end(), t.regs.begin(), t.regs.end());
        jobs->push_back(std::move(d));
    }
    g_want_compile = false;
    g_compiling = true;
    const int generation = g_generation;
    std::thread([fn, source, jobs, generation] {
        // Each variant (about 0.25 s at O3, 28 of them): from the cache when its source + defines were compiled before,
        // else compiled on one of several threads (D3DCompile calls are independent) and written to the cache.
        std::vector<Result> out(jobs->size());
        std::error_code ec;
        std::filesystem::create_directories(kCacheDir, ec);
        std::atomic<size_t> next{ 0 };
        auto work = [&] {
            for (size_t i; (i = next++) < jobs->size();) {
                Result& r = out[i];
                r.index = i;
                // FNV-1a 64 over the tag, the source and the defines
                uint64_t h = 1469598103934665603ull;
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
                        r.code.clear();         // damaged: compile again
                    }
                }
                std::vector<Macro> macros;
                for (const auto& [name, value] : (*jobs)[i]) macros.push_back({ name.c_str(), value.c_str() });
                macros.push_back({ nullptr, nullptr });
                Blob* code = nullptr;
                Blob* errors = nullptr;
                const HRESULT hr = fn(source->data(), source->size(), "material_pbr.hlsl", macros.data(), nullptr, "main",
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

// Render thread: create the compiled shaders and put them in place of the old ones
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
            continue;                           // the previous version (or stock) stays
        }
        const bool installed = handle_of(t) == t.ours && t.ours;
        IDirect3DPixelShader9* old = t.ours;
        if (installed) {
            code_of(t) = r.code.data();
            handle_of(t) = shader;
        }
        t.ours = shader;
        t.code = std::move(r.code);             // the buffer moves with it: the record's pointer stays valid
        if (old) old->Release();
    }
    if (g_failed) spdlog::warn("[PbrMaterials] {} shaders failed: {}", g_failed, g_error);
    else spdlog::info("[PbrMaterials] compiled {} shaders", results.size());
}

} // namespace

Status status() {
    Status s;
    s.found = g_tech != 0;
    s.waiting = g_waiting;
    s.targets = (int)g_targets.size();
    s.unsupported = g_unsupported;
    s.failed = g_failed;
    s.compiling = g_compiling;
    s.from_file = g_from_file;
    s.max_slots = g_max_slots;
    s.error = g_error;
    for (const Target& t : g_targets)
        if (t.ours && handle_of(t) == t.ours) ++s.replaced;
    return s;
}

void request_compile() { g_want_compile = true; }

bool write_source() {
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(kSourcePath).parent_path(), ec);
    std::ofstream f(kSourcePath, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(kEmbedded, (std::streamsize)std::strlen(kEmbedded));
    return (bool)f;
}

void update() {
    if (!mod_enabled) {
        for (Target& t : g_targets) restore(t);
        return;
    }
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (!device) return;
    if (g_tech) {                               // the technique reloaded: its records are new
        uintptr_t tech = 0;
        if (!standard_info(tech) || tech != g_tech) forget();
    }
    if (!g_tech) {
        if (g_compiling || !locate()) return;
        D3DCAPS9 caps{};
        if (SUCCEEDED(device->GetDeviceCaps(&caps))) g_max_slots = caps.MaxPixelShader30InstructionSlots;
        g_want_compile = true;
    }
    take_results(device);
    if (g_want_compile && !g_compiling && !g_targets.empty()) start_compile();
    for (Target& t : g_targets) install(t);
}

} // namespace pbr_materials

using namespace pbr_materials;

std::optional<std::string> PbrMaterials::on_initialize() { return Mod::on_initialize(); }

void PbrMaterials::on_frame(fmilliseconds& dt) { update(); }

void PbrMaterials::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("PBR materials"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Metal / roughness shading (GGX) for models Albam exports with PBR materials. Their materials use a shader "
                  "permutation no game file uses, so vanilla models look exactly as before. Off = those materials draw with the "
                  "game's own shader for that permutation (a rim light)."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("PbrMaterials");

    const Status st = status();
    if (!st.found) {
        ImGui::TextUnformatted(_("Waiting for the game's material shaders."));
        if (!st.waiting.empty()) ImGui::TextDisabled("(%s)", st.waiting.c_str());
    }
    else if (st.compiling) ImGui::Text(_("Compiling %d shaders..."), st.targets);
    else ImGui::Text(_("%d of %d shaders replaced, %d permutations left stock"), st.replaced, st.targets, st.unsupported);
    ImGui::SameLine();
    help_marker(_("Left stock: permutations the PBR shader doesn't cover (detail or parallax normal maps, HDR vertex colour, "
                  "texture colour lightmaps, multitexture). Albam doesn't export PBR materials with them."));

    if (ImGui::Checkbox(_("Linear lighting"), &linear_light)) request_compile();
    ImGui::SameLine();
    help_marker(_("Light in linear space, as physically based shading expects (textures and light colours converted on the way "
                  "in, back to the game's gamma on the way out). Off = light in gamma space like the game's own shaders, for "
                  "comparing how PBR models sit next to vanilla ones."));
    if (ImGui::Checkbox(_("Point lights get specular"), &simple_specular)) request_compile();
    ImGui::SameLine();
    help_marker(_("The game's 4 point lights (fill lights) only light diffuse in its own shaders. On = they make highlights on "
                  "PBR materials too."));
    const char* reflects[] = { _("Scene light"), _("Cube, scene brightness"), _("Cube as is") };
    ImGui::PushItemWidth(180.0f);
    if (ImGui::Combo(_("Reflections"), &reflect_mode, reflects, IM_ARRAYSIZE(reflects))) request_compile();
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("What PBR materials reflect. Most have no environment cube of their own and get the game's default one, a "
                  "small daylight sky, which made models look like chrome. Scene light: the stage's own ambient light, soft "
                  "and in its colours (best without a cube). Cube, scene brightness: the cube's pattern at the stage's "
                  "brightness. Cube as is: for materials with a cube made for where they're used."));
    const char* views[] = { _("Off"), _("Albedo"), _("Normal"), _("Roughness"), _("Metal"), _("Occlusion"), _("Diffuse light"),
                            _("Specular") };
    ImGui::PushItemWidth(180.0f);
    if (ImGui::Combo(_("Debug view"), &debug_view, views, IM_ARRAYSIZE(views))) request_compile();
    ImGui::PopItemWidth();
    ImGui::SameLine();
    help_marker(_("Show one input or term of the PBR materials instead of the shaded result, for checking textures from Albam."));

    if (ImGui::Button(_("Reload shader"))) request_compile();
    ImGui::SameLine();
    if (ImGui::Button(_("Write shader source"))) write_source();
    ImGui::SameLine();
    help_marker(_("Write Source puts the built-in shader in nativePC\\shader\\pbr\\material_pbr.hlsl. While that file exists it "
                  "is used instead of the built-in one; Reload compiles it again after an edit (a failed compile keeps the "
                  "previous shaders and shows the error below). Delete the file to go back to the built-in shader. Compiled "
                  "shaders are kept in nativePC\\shader\\pbr\\cache, so only a changed source or a new "
                  "combination of settings compiles again; the folder can be deleted at any time."));
    ImGui::Text(_("Source: %s"), st.from_file ? "nativePC\\shader\\pbr\\material_pbr.hlsl" : _("built in"));
    if (st.max_slots) {
        ImGui::SameLine(0.0f, 24.0f);
        ImGui::Text(_("Device: %u ps_3_0 instruction slots"), st.max_slots);
    }
    if (!st.error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), _("%d failed:"), st.failed);
        ImGui::BeginChild("errors", ImVec2(0.0f, 120.0f), true);
        ImGui::TextWrapped("%s", st.error.c_str());
        ImGui::EndChild();
    }

    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void PbrMaterials::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("pbr_materials").value_or(true);
    linear_light = cfg.get<bool>("pbr_linear_light").value_or(true);
    simple_specular = cfg.get<bool>("pbr_simple_specular").value_or(true);
    reflect_mode = std::clamp(cfg.get<int>("pbr_reflect").value_or(0), 0, 2);
}

void PbrMaterials::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pbr_materials", mod_enabled);
    cfg.set<bool>("pbr_linear_light", linear_light);
    cfg.set<bool>("pbr_simple_specular", simple_specular);
    cfg.set<int>("pbr_reflect", reflect_mode);
}
