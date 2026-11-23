#include "TexFilter.hpp"
#include "../../mod.hpp"
#include <cstdint>

namespace tex_filter {

bool enabled = true;

namespace {

constexpr uintptr_t kShaderInstance = 0x00E55700;   // sShader::mpInstance
constexpr uintptr_t kParamDescs = 0x838;             // ParameterDesc[512], 20 bytes: +8 the name's ID, +14 the state handle
constexpr uintptr_t kSamplerStates = 0x5038;         // SamplerState[256], 16 bytes
constexpr int kParams = 512;

enum Kind { ANISO, TRILINEAR };
struct Target {
    uint32_t    id;          // XFUNIQUE name hash (Vibed\RE\sShader_hash_ids.json)
    Kind        kind;
    const char* name;
};
constexpr Target kTargets[] = {
    { 0x8ABFFE3E, ANISO, "XfSamplerAmbientOccMap" },
    { 0x7222DCE7, ANISO, "XfSamplerDetailMap" },
    { 0x85F3C771, ANISO, "XfSamplerGlossMap" },
    { 0xB3CA6E13, ANISO, "XfSamplerMaskMap" },
    { 0x10CD25EE, ANISO, "XfSamplerLightMap" },
    { 0x5DEBB18E, ANISO, "XfSamplerLightMap2" },
    { 0x83109A3C, ANISO, "XfSamplerShadowMap" },
    { 0xD02F25DC, ANISO, "XfSamplerShadowMap2" },
    { 0x1A84B4C4, ANISO, "XfSamplerAdditionalMap" },
    { 0xC70A6632, ANISO, "gXfFlowingWaterSampler" },
    { 0x73A29EF8, ANISO, "XfWaterNormalMapSampler" },
    { 0xDEE93D6B, ANISO, "XfWaterCausticsSampler" },
    { 0xBCB92646, TRILINEAR, "XfPrimBaseLinearSampler" },
    { 0x9E52B9A6, TRILINEAR, "XfPrimNormalLinearSampler" },
    { 0x667CF66E, TRILINEAR, "XfPrimMaskLinearSampler" },
};
constexpr uint32_t kAlbedoId = 0x3101C8F5;            // XfSamplerAlbedoMap: the game's option sets it
constexpr int kCount = sizeof(kTargets) / sizeof(kTargets[0]);

constexpr uint32_t kMagShift = 24, kMinShift = 27, kAnisoShift = 20;
constexpr uint32_t kLinear = 2, kAnisotropic = 3;

uintptr_t g_shader = 0;                // the sShader the handles were found in
int       g_handle[kCount];            // -1: the parameter isn't registered
int       g_albedo = -1;
uint32_t  g_saved[kCount][2];          // the record as the game had it
uint32_t  g_written[kCount][2];        // what we last wrote: a record that differs was set again by the game
bool      g_have_saved[kCount] = {};
int       g_applied = 0;               // records ours this frame (the panel)

uint32_t* record(int handle) {
    return reinterpret_cast<uint32_t*>(g_shader + kSamplerStates + 16 * (uintptr_t)handle);
}

int handle_of(uint32_t id) {
    for (int p = 0; p < kParams; ++p) {
        const uintptr_t d = g_shader + kParamDescs + 20 * (uintptr_t)p;
        if (*reinterpret_cast<const uint32_t*>(d + 8) == id) return *reinterpret_cast<const uint8_t*>(d + 14);
    }
    return -1;
}

void find(uintptr_t shader) {
    g_shader = shader;
    for (int i = 0; i < kCount; ++i) {
        g_handle[i] = handle_of(kTargets[i].id);
        g_have_saved[i] = false;
    }
    g_albedo = handle_of(kAlbedoId);
}

void restore() {
    for (int i = 0; i < kCount; ++i)
        if (g_have_saved[i] && g_handle[i] >= 0) {
            uint32_t* r = record(g_handle[i]);
            r[0] = g_saved[i][0];
            r[1] = g_saved[i][1];
            g_have_saved[i] = false;
        }
}

} // namespace

void update() {
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    g_applied = 0;
    if (!shader) return;
    if (shader != g_shader) find(shader);
    // parameters register when their technique loads (the water's after the materials'): look again for missing ones
    for (int i = 0; i < kCount; ++i)
        if (g_handle[i] < 0) g_handle[i] = handle_of(kTargets[i].id);
    if (g_albedo < 0) g_albedo = handle_of(kAlbedoId);
    if (!enabled) {
        restore();
        return;
    }
    // the level the game's option gave the albedo map (16 when the option isn't anisotropic)
    uint32_t aniso_field = 15;
    if (g_albedo >= 0) {
        const uint32_t a = record(g_albedo)[0];
        if (((a >> kMinShift) & 7) == kAnisotropic) aniso_field = (a >> kAnisoShift) & 0xF;
    }
    for (int i = 0; i < kCount; ++i) {
        if (g_handle[i] < 0) continue;
        uint32_t* r = record(g_handle[i]);
        // a technique may set its records after we first saw them (the water's start as the engine's POINT default)
        if (!g_have_saved[i] || r[0] != g_written[i][0] || r[1] != g_written[i][1]) {
            g_saved[i][0] = r[0];
            g_saved[i][1] = r[1];
            g_have_saved[i] = true;
        }
        uint32_t d0 = g_saved[i][0], d1 = g_saved[i][1];
        g_written[i][0] = d0;
        g_written[i][1] = d1;
        // only records that filter at all (a sampler set to point is reading data, not a picture)
        if (((d0 >> kMinShift) & 7) < kLinear) continue;
        d0 &= ~((7u << kMagShift) | (7u << kMinShift));
        d0 |= kLinear << kMagShift;
        if (kTargets[i].kind == ANISO)
            d0 = (d0 & ~(0xFu << kAnisoShift)) | (aniso_field << kAnisoShift) | (kAnisotropic << kMinShift);
        else
            d0 |= kLinear << kMinShift;
        d1 = (d1 & ~7u) | kLinear;                     // between mips too
        r[0] = g_written[i][0] = d0;
        r[1] = g_written[i][1] = d1;
        ++g_applied;
    }
}

void draw_gui() {
    ImGui::Checkbox(_("Better texture filtering"), &enabled);
    ImGui::SameLine();
    Mod::help_marker(_("The game's Texture Filter option only sharpens colour and normal maps. This gives the rest of the "
                       "materials' maps (lightmaps, detail, gloss, mask, occlusion) and the water the same anisotropic "
                       "filtering, so floors and walls seen at an angle stay sharp, and blends effect textures smoothly "
                       "between their sizes instead of popping. The level follows the game's option (16x if it's off)."));
    if (enabled) {
        ImGui::SameLine();
        ImGui::TextDisabled(_("(%d samplers)"), g_applied);
    }
}

void load(const utility::Config& cfg) { enabled = cfg.get<bool>("postfx_texture_filter").value_or(true); }
void save(utility::Config& cfg) { cfg.set<bool>("postfx_texture_filter", enabled); }

} // namespace tex_filter
