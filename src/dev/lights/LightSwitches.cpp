#include "LightSwitches.hpp"
#include <spdlog/spdlog.h>
#include <d3d9.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>

// Notes and addresses: LightSwitches.hpp (DX9 exe; the bindings were logged live with cdb, 2026-11-15).

bool LightSwitches::mod_enabled = true;
bool LightSwitches::rank_enabled = true;

namespace light_switches {

constexpr uintptr_t kBoolSite  = 0x8F54FD;  // pixel shader bool binding loop body (10 bytes replaced)
constexpr uintptr_t kArraySite = 0x8F569B;  // pixel shader varying-length float array loop body (10 bytes)
constexpr uintptr_t kEndSite   = 0x8F56F8;  // after the pixel shader parameters (6 bytes)
constexpr uintptr_t kCacheCmp  = 0x8F56A5;  // cmp [esp + edx*4 + disp32], eax: the parameter cache the arrays use

// the parameter index globals: switches 0-3 spot lights, 4-7 point lights
constexpr uintptr_t kSwitchParams[8] = {0xE5571C, 0xE55720, 0xE55724, 0xE55728,
                                        0xE556A4, 0xE556A8, 0xE556AC, 0xE556B0};
constexpr uintptr_t kArrayParams[2]  = {0xE556F4, 0xE55494};  // gXfLightParam, gXfLightSimple
constexpr int kStockRegister[2] = {0, 23};  // their registers in the stock material shaders (a draw without the binding)
constexpr int kRegistersPerLight[2] = {4, 2};

uintptr_t jmp_ret_bool = 0, jmp_ret_array = 0, jmp_ret_end = 0, jmp_ret_rank = 0;

// ---- strongest point lights first (sub_A66180) ----
constexpr uintptr_t kRankSite = 0xA66448;   // mov [esp+10h], ecx (the list) / mov [esp+2Ch], edx (its count): 8 bytes
constexpr uintptr_t kShader   = 0xE55700;   // sShader::mpInstance; +0x68F8 = point light slots
constexpr uint32_t  kMaxLights = 1024;      // the list's room: (+0x1020 - +0x20) / 4

std::atomic<uint32_t> g_ranked{0};
uint32_t g_shown_ranked = 0;

struct RankScratch {
    void*    list[kMaxLights];
    uint32_t at[kMaxLights];     // the places point lights held
    void*    light[kMaxLights];
    float    score[kMaxLights];
};

// What a point light gives a model: colour (as the loop reads it) x its falloff at the model's box.
float light_score(const uint8_t* l, const float* box) {
    auto f = [l](int off) { return *reinterpret_cast<const float*>(l + off); };
    float d2 = 0.0f;
    for (int i = 0; i < 3; ++i) {
        const float p = f(0x40 + 4 * i), lo = box[i], hi = box[4 + i];
        const float e = p < lo ? lo - p : (p > hi ? p - hi : 0.0f);
        d2 += e * e;
    }
    const float d = d2 > 0.0f ? std::sqrt(d2) : 0.0f;
    const float start = f(0x50), end = f(0x54);
    float falloff;
    if (end > start) falloff = 1.0f - std::clamp((d - start) / (end - start), 0.0f, 1.0f);
    else falloff = d <= start ? 1.0f : 0.0f;
    const float colour = 0.3f * std::fabs(f(0x20)) + 0.59f * std::fabs(f(0x24)) + 0.11f * std::fabs(f(0x28));
    return colour * falloff;
}

// sub_A66180's light list for this model: the game's, or a copy with the point lights strongest first
void** __stdcall rank_lights(void** list, uint32_t count, const float* box, uint32_t group) {
    if (!LightSwitches::rank_enabled || !list || !box || count < 2 || count > kMaxLights) return list;
    const uintptr_t shader = *reinterpret_cast<const uintptr_t*>(kShader);
    if (!shader) return list;
    const uint32_t slots = *reinterpret_cast<const uint32_t*>(shader + 0x68F8);
    thread_local RankScratch s;
    uint32_t n = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* l = static_cast<const uint8_t*>(list[i]);
        if (!l || !(l[0x18] & 0x10) || l[0x19] != 2) continue;
        if (!(*reinterpret_cast<const uint32_t*>(l + 0x1C) & group)) continue;
        s.at[n] = i;
        s.light[n] = list[i];
        s.score[n] = light_score(l, box);
        ++n;
    }
    if (n <= slots) return list;  // they all fit: the game's order changes nothing
    // insertion sort, strongest first; equal scores keep the list order (stable)
    for (uint32_t i = 1; i < n; ++i) {
        void* l = s.light[i];
        const float sc = s.score[i];
        uint32_t j = i;
        while (j > 0 && s.score[j - 1] < sc) {
            s.light[j] = s.light[j - 1];
            s.score[j] = s.score[j - 1];
            --j;
        }
        s.light[j] = l;
        s.score[j] = sc;
    }
    std::memcpy(s.list, list, count * sizeof(void*));
    for (uint32_t k = 0; k < n; ++k) s.list[s.at[k]] = s.light[k];
    g_ranked.fetch_add(1, std::memory_order_relaxed);
    return s.list;
}

// 0xA66448, 8 bytes: mov [esp+10h], ecx / mov [esp+2Ch], edx (ecx = the light list, edx = its count; ebp = sub_A66180's
// frame: +10h the model's box, +14h its light group). The loop at 0xA66450 then reads the list from [esp+10h].
// xmm4 (0) and xmm5 (mSpeed, every light's strength: mSpeed - its shadow) are loaded at 0xA663DC / 0xA663E4 and live
// across the site, and rank_lights' code uses the SSE registers: all eight are saved, or every light a model gets is
// scaled by whatever the scoring left in xmm5 (stages went black in st700, lit up with each flash in st307).
naked void detour_rank() {
    _asm {
        pushad
        sub esp, 0x80
        movdqu [esp], xmm0
        movdqu [esp + 0x10], xmm1
        movdqu [esp + 0x20], xmm2
        movdqu [esp + 0x30], xmm3
        movdqu [esp + 0x40], xmm4
        movdqu [esp + 0x50], xmm5
        movdqu [esp + 0x60], xmm6
        movdqu [esp + 0x70], xmm7
        push dword ptr [ebp + 0x14]     // light group
        push dword ptr [ebp + 0x10]     // the model's box
        push edx                        // count
        push ecx                        // list
        call rank_lights                // __stdcall, the list to walk in eax
        mov [esp + 0x80 + 24], eax      // pushad's ecx
        movdqu xmm0, [esp]
        movdqu xmm1, [esp + 0x10]
        movdqu xmm2, [esp + 0x20]
        movdqu xmm3, [esp + 0x30]
        movdqu xmm4, [esp + 0x40]
        movdqu xmm5, [esp + 0x50]
        movdqu xmm6, [esp + 0x60]
        movdqu xmm7, [esp + 0x70]
        add esp, 0x80
        popad
        mov [esp + 0x10], ecx
        mov [esp + 0x2C], edx
        jmp dword ptr [jmp_ret_rank]
    }
}
uint32_t g_cache_disp = 0;  // the cache's offset from esp at the array site (0x1A0 in the DX9 exe)

// sRender's thread, one draw at a time (dispatchCommands)
struct Draw {
    uint8_t   seen = 0;                       // the switches this draw binds (bit k = switch k)
    uint8_t   on = 0;                         // the ones the game turned on
    int       reg[2] = {-1, -1};              // the arrays' first registers in this draw
    uint32_t* cache[2] = {nullptr, nullptr};  // and their cache entries
};
Draw g_draw;

std::atomic<uint32_t> g_lit{0}, g_blanked{0}, g_guessed{0};
uint32_t g_shown_lit = 0, g_shown_blanked = 0, g_shown_guessed = 0;

inline uint32_t global(uintptr_t at) { return *reinterpret_cast<const uint32_t*>(at); }

// a pixel shader bool binding: the value to send (light switches: TRUE)
uint32_t __stdcall on_bool(uint32_t param, uint32_t value) {
    if (!LightSwitches::mod_enabled) return value;
    for (int k = 0; k < 8; ++k) {
        const uint32_t p = global(kSwitchParams[k]);
        if (p == 0 || p != param) continue;
        g_draw.seen |= uint8_t(1u << k);
        if (value) g_draw.on |= uint8_t(1u << k);
        else g_draw.on &= uint8_t(~(1u << k));
        return 1;
    }
    return value;
}

// a pixel shader float array binding; esp = the stack pointer at the site (the cache is esp-relative)
void __stdcall on_array(uint32_t binding, uintptr_t esp) {
    if (!LightSwitches::mod_enabled) return;
    const uint32_t param = binding & 0xFFFF;
    for (int g = 0; g < 2; ++g) {
        const uint32_t p = global(kArrayParams[g]);
        if (p == 0 || p != param) continue;
        g_draw.reg[g] = int((binding >> 16) & 0xFF);
        g_draw.cache[g] = reinterpret_cast<uint32_t*>(esp + g_cache_disp + param * 4);
    }
}

// after the draw's pixel shader parameters: zero the registers of the lights the game had off
void __stdcall on_end(IDirect3DDevice9* device) {
    const Draw d = g_draw;
    g_draw = Draw{};
    if (!d.seen || !device) return;  // seen is only set while the mod is on: a draw whose switches were sent as TRUE
    g_lit.fetch_add(1, std::memory_order_relaxed);
    static const float zeros[16 * 4] = {};
    for (int g = 0; g < 2; ++g) {
        const unsigned off = unsigned((d.seen & ~d.on) >> (4 * g)) & 0xFu;
        if (!off) continue;
        int base = d.reg[g];
        if (base < 0) {
            base = kStockRegister[g];
            g_guessed.fetch_add(1, std::memory_order_relaxed);
        }
        const int n = kRegistersPerLight[g];
        for (int k = 0; k < 4;) {  // one call per run of off lights
            if (!(off & (1u << k))) { ++k; continue; }
            int e = k;
            while (e < 4 && (off & (1u << e))) ++e;
            device->SetPixelShaderConstantF(UINT(base + k * n), zeros, UINT((e - k) * n));
            g_blanked.fetch_add(uint32_t(e - k), std::memory_order_relaxed);
            k = e;
        }
        // the array's registers no longer hold what the engine sent: make the next draw send it again (the cache holds
        // the value pointer; the engine fills it with 0xFFFFFFFF at the start of each call)
        if (d.cache[g]) *d.cache[g] = 0xFFFFFFFFu;
    }
}

// 0x8F54FD, 10 bytes: mov edx, [ebx] / mov eax, [edi] / and edx, 0FFFFh (ebx = the binding, edi = its value). The cache
// compare at 0x8F5507 then uses eax (the value) and edx (the index); ecx (the binding list) is read after the loop.
naked void detour_bool() {
    _asm {
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        push ecx
        push edx
        push eax                    // value
        push edx                    // param
        call on_bool                // __stdcall, the value to send in eax
        pop edx
        pop ecx
        jmp dword ptr [jmp_ret_bool]
    }
}

// 0x8F569B, 10 bytes: the same three instructions (edi = the array's {count | flags, data} pointer); 0x8F56A5 compares
// eax with the cache entry [esp + edx*4 + 0x1A0].
naked void detour_array() {
    _asm {
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        push eax
        push ecx
        push edx
        lea eax, [esp + 12]         // esp at the site
        push eax
        push dword ptr [ebx]        // the binding
        call on_array               // __stdcall
        pop edx
        pop ecx
        pop eax
        jmp dword ptr [jmp_ret_array]
    }
}

// 0x8F56F8, 6 bytes: mov eax, [esp+50h] / mov eax, [eax] (esi = the device). Flags aren't live (sub eax, 3 follows).
naked void detour_end() {
    _asm {
        pushad
        push esi
        call on_end                 // __stdcall
        popad
        mov eax, [esp + 0x50]
        mov eax, [eax]
        jmp dword ptr [jmp_ret_end]
    }
}

bool sites_match() {
    static const uint8_t loop[10] = {0x8B, 0x13, 0x8B, 0x07, 0x81, 0xE2, 0xFF, 0xFF, 0x00, 0x00};
    static const uint8_t end[6] = {0x8B, 0x44, 0x24, 0x50, 0x8B, 0x00};
    static const uint8_t cmp[3] = {0x39, 0x84, 0x94};
    if (std::memcmp(reinterpret_cast<const void*>(kBoolSite), loop, sizeof loop) != 0) return false;
    if (std::memcmp(reinterpret_cast<const void*>(kArraySite), loop, sizeof loop) != 0) return false;
    if (std::memcmp(reinterpret_cast<const void*>(kEndSite), end, sizeof end) != 0) return false;
    if (std::memcmp(reinterpret_cast<const void*>(kCacheCmp), cmp, sizeof cmp) != 0) return false;
    std::memcpy(&g_cache_disp, reinterpret_cast<const void*>(kCacheCmp + 3), sizeof g_cache_disp);
    return true;
}

bool rank_site_matches() {
    static const uint8_t site[8] = {0x89, 0x4C, 0x24, 0x10, 0x89, 0x54, 0x24, 0x2C};
    return std::memcmp(reinterpret_cast<const void*>(kRankSite), site, sizeof site) == 0;
}

} // namespace light_switches

using namespace light_switches;

std::optional<std::string> LightSwitches::on_initialize() {
    if (!rank_site_matches() || !install_hook_absolute(kRankSite, hook_rank, &detour_rank, &jmp_ret_rank, 8)) {
        spdlog::error("LightSwitches: failed to hook the light list walk (0xA66448); point lights keep the game's order");
        hook_rank.reset();
        rank_enabled = false;
    }
    if (!sites_match()) {
        spdlog::error("LightSwitches: sRender::dispatchCommands isn't the expected code (0x8F54FD / 0x8F569B / 0x8F56F8); "
                      "not installed");
        mod_enabled = false;
        return Mod::on_initialize();
    }
    // all three or none: the switches sent as TRUE need the zeroing after them
    if (!install_hook_absolute(kBoolSite, hook_bool, &detour_bool, &jmp_ret_bool, 10) ||
        !install_hook_absolute(kArraySite, hook_array, &detour_array, &jmp_ret_array, 10) ||
        !install_hook_absolute(kEndSite, hook_end, &detour_end, &jmp_ret_end, 6)) {
        spdlog::error("LightSwitches: failed to hook sRender::dispatchCommands; light switches stay as the game sets them");
        hook_bool.reset();
        hook_array.reset();
        hook_end.reset();
        mod_enabled = false;
    }
    return Mod::on_initialize();
}

void LightSwitches::on_frame(fmilliseconds& dt) {
    (void)dt;
    g_shown_lit = g_lit.exchange(0, std::memory_order_relaxed);
    g_shown_blanked = g_blanked.exchange(0, std::memory_order_relaxed);
    g_shown_guessed = g_guessed.exchange(0, std::memory_order_relaxed);
    g_shown_ranked = g_ranked.exchange(0, std::memory_order_relaxed);
}

void LightSwitches::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (hook_rank) {
        ImGui::Checkbox(_("Strongest lights first"), &rank_enabled);
        ImGui::SameLine();
        help_marker(_("Stops the flicker when more than four effect lights are alive at once, such as rapid muzzle "
                      "flashes.\n"
                      "A model can take four point lights. The game gives those places to the oldest lights, so new "
                      "flashes are left out while old ones fade, then pop in when one dies. With this on, each model "
                      "takes the four that light it most."));
        if (rank_enabled)
            ImGui::TextDisabled(_("Last frame: %u models had more point lights than places"), g_shown_ranked);
    } else {
        ImGui::TextDisabled(_("Strongest lights first: not installed (see the log)"));
    }
    if (!hook_end) {
        ImGui::TextDisabled(_("Fixed light switches: not installed (see the log)"));
        return;
    }
    ImGui::Checkbox(_("Fixed light switches"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Stops the freeze when several effect lights appear at once, such as Rainstorm's or Pandora's muzzle "
                  "flashes, worst on the first burst after starting the game.\n"
                  "The game turns each light on and off with a switch in the shader, and the graphics driver builds a "
                  "new copy of every lit shader for each mix of switches it sees, while the game waits. With this on, "
                  "every switch stays on and a light the game has off is given no colour instead, so each shader is "
                  "built once. The picture is the same."));
    if (mod_enabled) {
        ImGui::TextDisabled(_("Last frame: %u lit draws, %u unused lights blanked"), g_shown_lit, g_shown_blanked);
        if (g_shown_guessed)
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f),
                               _("%u draws had light switches without the light list (the usual registers were used)"),
                               g_shown_guessed);
    }
}

void LightSwitches::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("light_switches_fixed").value_or(true);  // harmless without the hooks: nothing reads it
    rank_enabled = cfg.get<bool>("light_switches_rank").value_or(true);
}

void LightSwitches::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("light_switches_fixed", mod_enabled);
    cfg.set<bool>("light_switches_rank", rank_enabled);
}
