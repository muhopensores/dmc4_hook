#include "Shadows.hpp"
#include "sdk/TestSwitch.hpp"
#include "ShadowReceiver.hpp"
#include "PbrShadow.hpp"
#include "../../ModFramework.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace shadows {

bool mod_enabled = true;
int  map_size = 2048;

namespace {

constexpr uintptr_t kShaderInstance = 0xE55700;    // sShader::mpInstance
constexpr uint32_t  kShadowQuality = 0x68E0;       // sShader: the PC settings' ShadowQuality (0 none, 1 default, 2 high)
constexpr uint32_t  kMode = 0xE8, kPcf = 0xFC, kSize = 0x224;
#ifndef NDEBUG
constexpr uint32_t  kLightDir = 0x40;              // mInfiniteLightDir, float3
constexpr uint32_t  kTargetLight = 0x10C, kTargetLightEnable = 0x110;   // mpTargetLight, mTargetLightEnable (move 0xA72BCA)
#endif

struct FieldDef {
    uint32_t    offset;
    const char* key;      // config
    const char* label;
    float       def, lo, hi;
    const char* format;
    const char* help;
};

// uShadow fields (createProperty 0xA714C0; Vibed\RE\cunit_map\properties.md)
const FieldDef kFields[FIELD_COUNT] = {
    { 0xA0, "near_split", "Near cascade ends at", 0.083f, 0.01f, 0.5f, "%.3f",
      "mThreshold0: where the sharpest of the 3 cascades ends, as a share of the shadow distance. Smaller = sharper shadows "
      "close to the camera, but the middle cascade starts nearer. With the game's Shadow Quality HIGH every stage uses "
      "cascades; most stages leave this at 0.083." },
    { 0xA4, "middle_split", "Middle cascade ends at", 0.25f, 0.05f, 0.9f, "%.3f",
      "mThreshold1: where the middle cascade ends (the far one covers the rest), as a share of the shadow distance." },
    { 0x80, "view_distance", "Shadow distance", 5000.0f, 1000.0f, 20000.0f, "%.0f cm",
      "mShadowViewDistance: how far from the camera shadows reach. With Shadow Quality HIGH the game uses at least 5000 "
      "(50 m), so lower values change nothing there. Longer = the cascades cover more, so each is blurrier." },
    { 0x50, "depth_bias", "Depth bias", 5.0f, 0.0f, 60.0f, "%.2f cm",
      "mShadowDepthBias: the cast pass pushes every shadow caster this many cm away from the sun, so a surface is only "
      "shadowed when it is more than this far behind what casts on it, along the sun's rays. Too low = stripes on lit "
      "surfaces (shadow acne); too high = shadows come loose from the feet, and nothing standing near a wall shadows it. "
      "Stages use 3 to 50." },
    { 0xAC, "slope_bias", "Slope bias", 0.0f, 0.0f, 60.0f, "%.2f",
      "mShadowSlopeDepthBias: extra bias on surfaces steep to the light, where acne shows first." },
    { 0x6C, "darkness", "Darkness", 0.6f, 0.0f, 1.0f, "%.2f",
      "mShadowColor alpha: how far a shadowed pixel is pulled toward the shadow colour (0 = no shadows, 1 = the colour "
      "itself, usually black). Stages use 0.19 to 0.82." },
};

} // namespace

Override overrides[FIELD_COUNT] = {
    { false, kFields[SPLIT_NEAR].def },  { false, kFields[SPLIT_MIDDLE].def }, { false, kFields[VIEW_DISTANCE].def },
    { false, kFields[DEPTH_BIAS].def },  { false, kFields[SLOPE_BIAS].def },   { false, kFields[DARKNESS].def },
};
#ifndef NDEBUG
SunOverride sun;

constexpr float kDeg = 3.14159265f / 180.0f;

void sun_angles(const float dir[3], float& elevation, float& azimuth) {
    const float len = std::sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
    if (len < 1e-6f) { elevation = 90.0f; azimuth = 0.0f; return; }
    elevation = std::asin(std::clamp(-dir[1] / len, -1.0f, 1.0f)) / kDeg;
    azimuth = std::atan2(dir[0], dir[2]) / kDeg;
    if (azimuth < 0.0f) azimuth += 360.0f;
}

void sun_direction(float elevation, float azimuth, float dir[3]) {
    const float e = elevation * kDeg, a = azimuth * kDeg;
    dir[0] = std::cos(e) * std::sin(a);
    dir[1] = -std::sin(e);
    dir[2] = std::cos(e) * std::cos(a);
}
#endif

namespace {

struct Slot {
    uintptr_t unit = 0;
    uint32_t  last_seen = 0;
    uint32_t  size_gen = 0;
    bool      have[FIELD_COUNT] = {};
    float     stage[FIELD_COUNT] = {};
    float     written[FIELD_COUNT] = {};
#ifndef NDEBUG
    bool      have_dir = false;
    float     stage_dir[3] = {};
    float     written_dir[3] = {};
    bool      target_light = false;
#endif
    uint32_t  mode = 0, pcf = 0, size = 0;
};

std::mutex            g_mutex;
Slot                  g_slots[4];
uint32_t              g_tick = 0;
std::atomic<uint32_t> g_size_gen{ 0 };     // bumped when the size option changes (to put the game's size back)

volatile uint32_t g_map_size = 0;          // what the size hook hands out (0 = the game's)
uintptr_t         jmp_ret_move = 0;
uintptr_t         jmp_ret_map_size = 0;    // 0xA7D3D1: store the size, free and create the maps
uintptr_t         jmp_same_size = 0xA7D3E3; // the size didn't change
uintptr_t         jmp_ret_depth = 0;       // 0x8F5A90: SetDepthStencilSurface(eax)

// our depth surface for shadow maps bigger than the game's shared one (render thread: on_frame / on_reset / the hook)
IDirect3DSurface9* volatile g_depth = nullptr;
volatile uint32_t           g_depth_size = 0;
IDirect3DSurface9*          g_shared = nullptr;     // the game's surface last measured, and its size
uint32_t                    g_shared_w = 0, g_shared_h = 0;
bool                        g_depth_failed = false; // creation failed for this size (shown in the panel)

int game_quality() {
    const uintptr_t shader = *(uintptr_t*)kShaderInstance;
    return shader ? *(int*)(shader + kShadowQuality) : -1;
}

void refresh_map_size() { g_map_size = mod_enabled ? (uint32_t)map_size : 0; }

// uShadow::move (game thread, every frame), before its body
void __stdcall on_move(uintptr_t unit) {
    std::lock_guard<std::mutex> lock(g_mutex);
    ++g_tick;
    Slot* slot = nullptr;
    for (Slot& s : g_slots)
        if (s.unit == unit) { slot = &s; break; }
    if (!slot) {    // a new unit: take the slot seen longest ago
        slot = &g_slots[0];
        for (Slot& s : g_slots)
            if (s.last_seen < slot->last_seen) slot = &s;
        *slot = Slot{};
        slot->unit = unit;
        slot->size_gen = g_size_gen.load();
    }
    slot->last_seen = g_tick;

    for (int i = 0; i < FIELD_COUNT; ++i) {
        float* field = (float*)(unit + kFields[i].offset);
        const float cur = *field;
        if (!slot->have[i] || std::memcmp(&cur, &slot->written[i], sizeof(float)) != 0) {
            slot->stage[i] = cur;       // the .sdl's (or its keys') value
            slot->have[i] = true;
        }
        const float target = (mod_enabled && overrides[i].on) ? overrides[i].value : slot->stage[i];
        *field = target;
        slot->written[i] = target;
    }

#ifndef NDEBUG
    // the sun's direction, the same way (3 floats; the .sdl's value kept while overridden)
    float* dir = (float*)(unit + kLightDir);
    if (!slot->have_dir || std::memcmp(dir, slot->written_dir, sizeof(slot->written_dir)) != 0) {
        std::memcpy(slot->stage_dir, dir, sizeof(slot->stage_dir));
        slot->have_dir = true;
    }
    if (mod_enabled && sun.on) sun_direction(sun.elevation, sun.azimuth, dir);
    else std::memcpy(dir, slot->stage_dir, sizeof(slot->stage_dir));
    std::memcpy(slot->written_dir, dir, sizeof(slot->written_dir));
    slot->target_light = *(uint8_t*)(unit + kTargetLightEnable) && *(uintptr_t*)(unit + kTargetLight);
#endif

    // the size option went back to the game's: make the setter (called by this move) run its own rule again
    const uint32_t gen = g_size_gen.load();
    if (slot->size_gen != gen) {
        slot->size_gen = gen;
        if (g_map_size == 0 && game_quality() == 2) *(uint32_t*)(unit + kSize) = 0;   // HIGH: becomes 1024
    }
    slot->mode = *(uint32_t*)(unit + kMode);
    slot->pcf = *(uint32_t*)(unit + kPcf);
    slot->size = *(uint32_t*)(unit + kSize);
}

// 0xA72B70 uShadow::move entry, 9 bytes: push ebp / mov ebp, esp / and esp, -16 / sub esp, 8 (ecx = the unit)
naked void detour_move() {
    _asm {
        pushad
        push ecx
        call on_move                // __stdcall
        popad
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        sub esp, 8
        jmp dword ptr [jmp_ret_move]
    }
}

// 0xA7D3C9 in the mMapSize setter, 8 bytes: cmp [esi+224h], eax / jz (eax = the size after the game's HIGH rule)
naked void detour_map_size() {
    _asm {
        cmp dword ptr [g_map_size], 0
        je compare
        mov eax, dword ptr [g_map_size]
    compare:
        cmp [esi + 0x224], eax
        je same
        jmp dword ptr [jmp_ret_map_size]
    same:
        jmp dword ptr [jmp_same_size]
    }
}

// sRender::dispatchCommands is about to bind `shared` (its shared depth surface) for a w x h render target
IDirect3DSurface9* __stdcall pick_depth(IDirect3DSurface9* shared, uint32_t w, uint32_t h) {
    IDirect3DSurface9* own = g_depth;
    if (!own || !shared) return shared;
    if (shared != g_shared) {
        D3DSURFACE_DESC desc{};
        if (FAILED(shared->GetDesc(&desc))) return shared;
        g_shared = shared;
        g_shared_w = desc.Width;
        g_shared_h = desc.Height;
    }
    if (w <= g_shared_w && h <= g_shared_h) return shared;
    if (w <= g_depth_size && h <= g_depth_size) return own;
    return shared;
}

// 0x8F5A8A, 6 bytes: mov eax, [edi+1B1E8h] (edi = sRender, ebx = the render-target command: +0x18 width, +0x1C height);
// ecx / edx are loaded again right after, at 0x8F5A90
naked void detour_depth() {
    _asm {
        mov eax, [edi + 0x1B1E8]
        push dword ptr [ebx + 0x1C]
        push dword ptr [ebx + 0x18]
        push eax
        call pick_depth             // __stdcall, returns the surface in eax
        jmp dword ptr [jmp_ret_depth]
    }
}

void release_depth() {
    IDirect3DSurface9* own = g_depth;
    g_depth = nullptr;
    g_shared = nullptr;
    if (own) own->Release();
}

// render thread (Present): keep our depth surface the size the maps need (none up to 2048: the game's fits)
void update_depth() {
    const uint32_t want = g_map_size > 2048 ? g_map_size : 0;
    if (want != g_depth_size) {
        release_depth();
        g_depth_failed = false;
        g_depth_size = want;
    }
    if (!want || g_depth || g_depth_failed) return;
    IDirect3DDevice9* device = g_framework->get_d3d9_device();
    if (!device) return;
    IDirect3DSurface9* own = nullptr;
    const HRESULT hr = device->CreateDepthStencilSurface(want, want, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, &own,
                                                         nullptr);
    if (FAILED(hr) || !own) {
        spdlog::error("Shadows: couldn't make a {0}x{0} depth surface (0x{1:08X}); {0} maps won't draw", want, (uint32_t)hr);
        g_depth_failed = true;
        return;
    }
    g_depth = own;
}

} // namespace

Status status() {
    Status st;
    st.quality = game_quality();
    std::lock_guard<std::mutex> lock(g_mutex);
    for (const Slot& s : g_slots) {
        // units not moved for ~2 s (another area, a menu) aren't listed
        if (!s.unit || g_tick - s.last_seen > 120 * 4) continue;
        UnitInfo& u = st.unit[st.units++];
        u.unit = s.unit;
        u.mode = s.mode;
        u.pcf = s.pcf;
        u.size = s.size;
        std::memcpy(u.stage, s.stage, sizeof(u.stage));
#ifndef NDEBUG
        std::memcpy(u.stage_dir, s.stage_dir, sizeof(u.stage_dir));
        u.target_light = s.target_light;
#endif
    }
    return st;
}

} // namespace shadows

using namespace shadows;

std::optional<std::string> Shadows::on_initialize() {
    if (!install_hook_absolute(0xA72B70, hook_move, &detour_move, &jmp_ret_move, 9))
        spdlog::error("Shadows: failed to hook uShadow::move (0xA72B70)");
    if (!install_hook_absolute(0xA7D3C9, hook_map_size, &detour_map_size, &jmp_ret_map_size, 8))
        spdlog::error("Shadows: failed to hook the shadow map size setter (0xA7D3C9)");
    if (!install_hook_absolute(0x8F5A8A, hook_depth, &detour_depth, &jmp_ret_depth, 6))
        spdlog::error("Shadows: failed to hook the depth surface pick (0x8F5A8A); maps over 2048 won't draw");
    if (!install_hook_absolute(shadow_receiver::kCaptureSite, hook_capture, &shadow_receiver::detour_capture,
                               &shadow_receiver::jmp_ret_capture, 6))
        spdlog::error("Shadows: failed to hook the receiver setup (0xA775E0); soft shadows fall back to a fixed radius");
    shadow_receiver::init();
    if (!install_hook_absolute(pbr_shadow::kModelGate, hook_model_gate, &pbr_shadow::detour_model_gate,
                               &pbr_shadow::jmp_ret_model_gate, 8) ||
        !install_hook_absolute(pbr_shadow::kDevilGate, hook_devil_gate, &pbr_shadow::detour_devil_gate,
                               &pbr_shadow::jmp_ret_devil_gate, 8)) {
        spdlog::error("Shadows: failed to hook the receiver pass's material gates; PBR materials keep the stock shadow");
        pbr_shadow::settings.on = false;
    }
    pbr_shadow::init();
    refresh_map_size();
    return Mod::on_initialize();
}

void Shadows::on_frame(fmilliseconds& dt) {
    update_depth();
    shadow_receiver::update(mod_enabled);
    pbr_shadow::update(mod_enabled);
}

// before IDirect3DDevice9::Reset: default-pool surfaces must go (made again on the next frame)
void Shadows::on_reset() {
    pbr_shadow::on_reset();
    shadow_receiver::release_frame();
    release_depth();
    g_depth_failed = false;
}

void Shadows::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (ImGui::Checkbox(_("Shadow tweaks"), &mod_enabled)) {
        refresh_map_size();
        g_size_gen.fetch_add(1);
    }
    ImGui::SameLine();
    help_marker(_("The stage shadow's map size and its settings, live. Off = every stage as the game sets it. The game's own "
                  "Shadow Quality (PC settings) should be HIGH: that turns on the 3 cascades and the smoothest filter."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("Shadows");

    const Status st = status();
    static const char* kQuality[] = { "NONE", "DEFAULT", "HIGH" };
    if (st.quality >= 0 && st.quality <= 2) {
        ImGui::Text(_("Game Shadow Quality: %s"), kQuality[st.quality]);
        if (st.quality != 2) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), _("(set it to HIGH in the PC settings)"));
        }
    }

    static const int   kSizes[] = { 0, 2048, 4096 };
    static const char* kSizeNames[] = { "Game (1024 on HIGH)", "2048", "4096" };
    int pick = 0;
    for (int i = 0; i < 3; ++i)
        if (kSizes[i] == map_size) pick = i;
    ImGui::PushItemWidth(180.0f);
    if (ImGui::Combo(_("Map size"), &pick, kSizeNames, 3)) {
        map_size = kSizes[pick];
        refresh_map_size();
        g_size_gen.fetch_add(1);
    }
    ImGui::SameLine();
    help_marker(_("Pixels per side of each shadow map (3 with cascades). Bigger = sharper shadow edges and fewer jaggies. "
                  "4096 uses 64 MB of video memory per map plus a 64 MB depth buffer. Applied live. Graphics Settings' "
                  "Increased Shadow Resolution is the same thing at 2048; this one wins when set."));
    if (g_depth_failed)
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), _("Couldn't make the depth buffer for this size (see the log)."));

    if (st.units == 0) ImGui::TextDisabled(_("No stage shadow running."));
    static const char* kModes[] = { "3 cascades", "one light-space map", "uniform area", "spot" };
    static const int   kTaps[] = { 4, 9, 16 };
    for (int k = 0; k < st.units; ++k) {
        const UnitInfo& u = st.unit[k];
        const uint32_t pcf = st.quality == 2 ? 2 : u.pcf;     // HIGH hands the shader the 16-tap filter (0xA7D710)
        ImGui::TextDisabled(_("Stage shadow: %s, %u px, %d-tap filter"), u.mode < 4 ? kModes[u.mode] : "?", u.size,
                            pcf < 3 ? kTaps[pcf] : 0);
    }

    ImGui::TextDisabled(_("Overrides (tick one to replace the stage's value)"));
    for (int i = 0; i < FIELD_COUNT; ++i) {
        const FieldDef& f = kFields[i];
        ImGui::PushID(i);
        ImGui::Checkbox("##on", &overrides[i].on);
        ImGui::SameLine();
        ImGui::BeginDisabled(!overrides[i].on);
        ImGui::SliderFloat(f.label, &overrides[i].value, f.lo, f.hi, f.format);
        ImGui::EndDisabled();
        ImGui::SameLine();
        help_marker(f.help);
        if (st.units > 0) {
            ImGui::SameLine();
            char text[48];
            std::snprintf(text, sizeof(text), f.format, st.unit[0].stage[i]);
            ImGui::TextDisabled(_("(stage: %s)"), text);
        }
        ImGui::PopID();
    }

#ifndef NDEBUG
    // the sun's direction (debug builds)
    float stage_e = 0.0f, stage_a = 0.0f;
    if (st.units > 0) sun_angles(st.unit[0].stage_dir, stage_e, stage_a);
    ImGui::Checkbox("##sun_on", &sun.on);
    ImGui::SameLine();
    ImGui::BeginDisabled(!sun.on);
    ImGui::SliderFloat(_("Sun height"), &sun.elevation, 5.0f, 90.0f, "%.0f deg");
    ImGui::EndDisabled();
    ImGui::SameLine();
    help_marker(_("Where the shadows come from: the sun's height above the horizon. Stages put it 55 to 90 degrees high, so "
                  "shadows fall straight down onto floors and walls hardly ever catch one (walls turned from the sun never "
                  "do). Lower = longer shadows that climb walls. It moves only the shadows, not the stage's lighting: keep "
                  "it near where the stage's light seems to come from. Take from stage copies the stage's sun to start "
                  "from."));
    if (st.units > 0) {
        ImGui::SameLine();
        ImGui::TextDisabled(_("(stage: %.0f deg)"), stage_e);
    }
    ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0.0f));
    ImGui::SameLine();
    ImGui::BeginDisabled(!sun.on);
    ImGui::SliderFloat(_("Sun direction"), &sun.azimuth, 0.0f, 360.0f, "%.0f deg");
    ImGui::EndDisabled();
    ImGui::SameLine();
    help_marker(_("Which way the shadows point, around the up axis (0 = along the stage's +z, 90 = +x)."));
    if (st.units > 0) {
        ImGui::SameLine();
        ImGui::TextDisabled(_("(stage: %.0f deg)"), stage_a);
        ImGui::SameLine();
        if (ImGui::Button(_("Take from stage"))) {
            sun.elevation = std::clamp(stage_e, 5.0f, 90.0f);
            sun.azimuth = stage_a;
        }
        if (st.unit[0].target_light && sun.on)
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),
                               _("This stage ties the shadow to one of its lights, which replaces the sun override."));
    }
#endif

    if (ImGui::Button(_("Defaults"))) {
        for (int i = 0; i < FIELD_COUNT; ++i) overrides[i] = Override{ false, kFields[i].def };
#ifndef NDEBUG
        sun = SunOverride{};
#endif
    }

    // ---- soft shadows (step 2) ----
    shadow_receiver::Settings& soft = shadow_receiver::settings;
    ImGui::Checkbox(_("Soft shadows"), &soft.soft);
    ImGui::SameLine();
    help_marker(_("Replaces the game's shadow edge filter (16 depth taps in a box) with a contact-hardening one: shadows "
                  "are sharp where the object touches the ground and soften with the distance between them, like real "
                  "light. Applies to the stage shadow's 3 cascades (Shadow Quality HIGH)."));
    if (soft.soft) {
        const shadow_receiver::Status rs = shadow_receiver::status();
        if (!rs.found) {
            ImGui::TextDisabled(_("Waiting for the shader: %s"), rs.waiting.c_str());
        } else {
            ImGui::TextDisabled(_("Receiver shaders: %d of %d replaced%s%s"), rs.replaced, rs.targets,
                                rs.compiling ? ", compiling..." : "", rs.from_file ? " (from nativePC)" : "");
            if (rs.failed)
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), _("%d failed: %s"), rs.failed, rs.error.c_str());
            if (!rs.matrices && st.units > 0)
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),
                                   _("No cascade matrices seen: softness uses a fixed radius (not in cascade mode?)."));
        }
        static const char* kQualityNames[] = { "16 taps", "24 taps", "32 taps" };
        if (ImGui::Combo(_("Filter quality"), &soft.quality, kQualityNames, 3)) shadow_receiver::request_compile();
        ImGui::SameLine();
        help_marker(_("Depth samples per shadowed pixel. More = smoother wide penumbras, slower."));
        ImGui::SliderFloat(_("Light size"), &soft.light_size, 0.0f, 0.15f, "%.3f");
        ImGui::SameLine();
        help_marker(_("How fast shadows soften with distance: penumbra width per cm between the object and the surface "
                      "(the light's apparent size). The sun is about 0.01; 0.03 reads as a slightly hazy sun. 0 = always "
                      "sharp."));
        ImGui::SliderFloat(_("Widest penumbra"), &soft.max_soft, 5.0f, 150.0f, "%.0f cm");
        ImGui::SameLine();
        help_marker(_("The softest a shadow edge gets, in cm. Also how far around each pixel the filter looks for what "
                      "casts the shadow (at most 48 map pixels)."));
        ImGui::SliderFloat(_("Sharpest edge"), &soft.min_radius, 0.5f, 3.0f, "%.2f px");
        ImGui::SameLine();
        help_marker(_("The narrowest filter, in shadow map pixels: where the object touches the surface. Lower = crisper "
                      "contact shadows, but the map's pixels can show."));
        static const char* kDebugNames[] = { "Off", "Cascades", "Penumbra" };
        if (ImGui::Combo(_("Debug view"), &soft.debug, kDebugNames, 3)) shadow_receiver::request_compile();
        ImGui::SameLine();
        help_marker(_("Cascades: near red, middle green, far blue over every receiving surface. Penumbra: the filter "
                      "width where something shadows, green = sharp .. red = 16 map pixels and up."));
        if (ImGui::Button(_("Soft defaults"))) {
            const shadow_receiver::Settings d;
            if (soft.quality != d.quality || soft.debug != d.debug) shadow_receiver::request_compile();
            soft = d;
        }
    }

    // ---- shadows in PBR materials (step 3) ----
    ImGui::Checkbox(_("Shadows in PBR materials"), &pbr_shadow::settings.on);
    ImGui::SameLine();
    help_marker(_("PBR materials (PbrMaterials) shade the stage shadow themselves instead of the game's shadow pass darkening "
                  "them afterwards: no highlights or light through thin parts inside shadows, and on skin the shadow's edge "
                  "softens with the scattering. The darkness, colour and softness are the stage's and the settings above. "
                  "Needs Shadow Quality HIGH (3 cascades) and PBR materials on."));
    if (pbr_shadow::settings.on) {
        const pbr_shadow::Status ps = pbr_shadow::status();
        if (ps.active)
            ImGui::TextDisabled(_("In use: %d PBR mesh draws took the shadow themselves last frame."), ps.skipped);
        else if (!ps.pbr)
            ImGui::TextDisabled(_("Off: PBR materials are off."));
        else if (!ps.shader)
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),
                               _("The PBR shader has no shadow code (an older nativePC\\shader\\pbr\\material_pbr.hlsl? Shader "
                                 "files, bottom of this tab: PBR materials, Write file): PBR materials keep the game's "
                                 "shadow pass."));
        else if (!ps.data)
            ImGui::TextDisabled(_("Waiting for the stage shadow's 3 cascades (Shadow Quality HIGH)."));
        else
            ImGui::TextDisabled(_("Waiting for the shadow maps."));
    }
    ImGui::PopItemWidth();

    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

static bool cfg_enabled = true;   // what the cfg says (DMC4_NO_GFX)

void Shadows::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg_enabled = cfg.get<bool>("shadows_enabled").value_or(true);
    if (test_switch("DMC4_NO_GFX")) mod_enabled = false;   // test runs (frame-rate checks): the cfg keeps its value
    map_size = cfg.get<int>("shadows_map_size").value_or(2048);
    if (map_size != 0 && map_size != 2048 && map_size != 4096) map_size = 2048;
    for (int i = 0; i < FIELD_COUNT; ++i) {
        const FieldDef& f = kFields[i];
        const std::string key = std::string("shadows_") + f.key;
        overrides[i].on = cfg.get<bool>(key + "_on").value_or(false);
        overrides[i].value = std::clamp(cfg.get<float>(key).value_or(f.def), f.lo, f.hi);
    }
#ifndef NDEBUG
    const SunOverride sd;
    sun.on = cfg.get<bool>("shadows_sun_on").value_or(sd.on);
    sun.elevation = std::clamp(cfg.get<float>("shadows_sun_height").value_or(sd.elevation), 5.0f, 90.0f);
    sun.azimuth = std::clamp(cfg.get<float>("shadows_sun_direction").value_or(sd.azimuth), 0.0f, 360.0f);
#endif
    shadow_receiver::Settings& soft = shadow_receiver::settings;
    const shadow_receiver::Settings d;
    soft.soft = cfg.get<bool>("shadows_soft").value_or(d.soft);
    soft.quality = std::clamp(cfg.get<int>("shadows_soft_quality").value_or(d.quality), 0, 2);
    soft.light_size = std::clamp(cfg.get<float>("shadows_light_size").value_or(d.light_size), 0.0f, 0.15f);
    soft.max_soft = std::clamp(cfg.get<float>("shadows_max_soft").value_or(d.max_soft), 5.0f, 150.0f);
    soft.min_radius = std::clamp(cfg.get<float>("shadows_min_radius").value_or(d.min_radius), 0.5f, 3.0f);
    pbr_shadow::settings.on = cfg.get<bool>("shadows_pbr").value_or(pbr_shadow::Settings{}.on);
    shadow_receiver::request_compile();
    refresh_map_size();
    g_size_gen.fetch_add(1);
}

void Shadows::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("shadows_enabled", test_switch("DMC4_NO_GFX") ? cfg_enabled : mod_enabled);
    cfg.set<int>("shadows_map_size", map_size);
    for (int i = 0; i < FIELD_COUNT; ++i) {
        const std::string key = std::string("shadows_") + kFields[i].key;
        cfg.set<bool>(key + "_on", overrides[i].on);
        cfg.set<float>(key, overrides[i].value);
    }
#ifndef NDEBUG
    cfg.set<bool>("shadows_sun_on", sun.on);
    cfg.set<float>("shadows_sun_height", sun.elevation);
    cfg.set<float>("shadows_sun_direction", sun.azimuth);
#endif
    const shadow_receiver::Settings& soft = shadow_receiver::settings;
    cfg.set<bool>("shadows_soft", soft.soft);
    cfg.set<int>("shadows_soft_quality", soft.quality);
    cfg.set<float>("shadows_light_size", soft.light_size);
    cfg.set<float>("shadows_max_soft", soft.max_soft);
    cfg.set<float>("shadows_min_radius", soft.min_radius);
    cfg.set<bool>("shadows_pbr", pbr_shadow::settings.on);
}
