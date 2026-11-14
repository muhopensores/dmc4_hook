#include "Shadows.hpp"
#include "../../ModFramework.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <algorithm>
#include <atomic>
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
    { 0x50, "depth_bias", "Depth bias", 5.0f, 0.0f, 60.0f, "%.2f",
      "mShadowDepthBias: how far a surface may be behind the depth the shadow map stored and still count as lit. Too low = "
      "stripes on lit surfaces (shadow acne); too high = shadows come loose from the feet. Stages use 0.1 to 50." },
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

namespace {

struct Slot {
    uintptr_t unit = 0;
    uint32_t  last_seen = 0;
    uint32_t  size_gen = 0;
    bool      have[FIELD_COUNT] = {};
    float     stage[FIELD_COUNT] = {};
    float     written[FIELD_COUNT] = {};
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
    refresh_map_size();
    return Mod::on_initialize();
}

void Shadows::on_frame(fmilliseconds& dt) { update_depth(); }

// before IDirect3DDevice9::Reset: default-pool surfaces must go (made again on the next frame)
void Shadows::on_reset() {
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
    if (ImGui::Button(_("Defaults"))) {
        for (int i = 0; i < FIELD_COUNT; ++i) overrides[i] = Override{ false, kFields[i].def };
    }
    ImGui::PopItemWidth();

    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void Shadows::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("shadows_enabled").value_or(true);
    map_size = cfg.get<int>("shadows_map_size").value_or(2048);
    if (map_size != 0 && map_size != 2048 && map_size != 4096) map_size = 2048;
    for (int i = 0; i < FIELD_COUNT; ++i) {
        const FieldDef& f = kFields[i];
        const std::string key = std::string("shadows_") + f.key;
        overrides[i].on = cfg.get<bool>(key + "_on").value_or(false);
        overrides[i].value = std::clamp(cfg.get<float>(key).value_or(f.def), f.lo, f.hi);
    }
    refresh_map_size();
    g_size_gen.fetch_add(1);
}

void Shadows::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("shadows_enabled", mod_enabled);
    cfg.set<int>("shadows_map_size", map_size);
    for (int i = 0; i < FIELD_COUNT; ++i) {
        const std::string key = std::string("shadows_") + kFields[i].key;
        cfg.set<bool>(key + "_on", overrides[i].on);
        cfg.set<float>(key, overrides[i].value);
    }
}
