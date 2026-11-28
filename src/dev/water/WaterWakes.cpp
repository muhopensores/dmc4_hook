#include "WaterWakes.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include "../debug_server/Memory.hpp"
#include "../../sdk/TestSwitch.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

// Notes and addresses: WaterWakes.hpp (DX9 exe, IDA 2026-11-26).

bool WaterWakes::mod_enabled = true;

namespace water_wakes {

constexpr uintptr_t kRainCall = 0xAC19DC;   // uInteractiveWater::move: call uInteractiveWater::rain (5 bytes, esi = this)
constexpr uintptr_t kRain     = 0xAC4EB0;
uintptr_t jmp_ret_rain = 0;
uintptr_t rain_fn = kRain;

constexpr float kFootprint = 70.0f;   // cm: a wake's radius (40 until 2026-11-26: the user wanted them more visible)
// a wake's peak strength: kWakeBase + kWakePerCm x the smoothed cm a call, at most kWakeMax (0.05 / 0.04 / 0.6 before,
// as strong as a rain drop's 0.2 at a walk; round forces fall off from the centre, so the peak is higher)
constexpr float kWakeBase  = 0.3f;    // height map read back 2026-11-26: 0.15 / 0.08 / 1.2 left the wake at the rain's level
constexpr float kWakePerCm = 0.15f;
constexpr float kWakeMax   = 2.0f;
constexpr float kSplash    = 90.0f;   // cm: a splash's size
constexpr float kAbove     = 15.0f;   // cm above the surface still counted as in the water
constexpr float kBelow     = 150.0f;  // cm below it
// a splash grows with the speed it hits the water at (cm a frame): a jump lands at about kLandingSpeed (Nero's, 56: a 189 cm
// fall), a dive or slam faster (lock-on + back + sword in the air, Split: 76 after 267 cm); each kGrowSpeed past a jump's
// adds the plain splash's size and strength once more, up to kMaxGrow times (measured in st705, 2026-11-26)
constexpr float kLandingSpeed = 56.0f;
constexpr float kGrowSpeed    = 20.0f;
constexpr float kMaxGrow      = 2.5f;
// strength multiplier (height map read back in st705, 2026-11-26, pushed over kSplashCalls: x3 and x10 already reach
// the simulation's full height on a plain jump)
constexpr float kSplashGain   = 1.5f;   // 1.0 with square forces; the round ones fall off from the centre
// attack shapes in the water (push_attacks): a force per contact point, each the shape's size, strength 0.05 +
// kAttackPerCm x the cm it moved since last frame, at most kAttackMaxStrength (a plain landing's splash is 1.5, a rain drop
// 0.2); shapes moving under kAttackMinSpeed cm a frame are left out; at most kAttackPoints points per shape
constexpr float kAttackPerCm       = 0.02f;    // 0.01 / 1.0 with square forces (2026-11-26: more visible)
constexpr float kAttackMaxStrength = 2.0f;
constexpr float kAttackSize        = 1.3f;     // a cut's radius = the shape's x this
constexpr float kAttackMinSpeed    = 4.0f;
constexpr int   kAttackPoints      = 6;
uint32_t g_attack_logged = 0;   // g_call of the last attack log line (one every 120 calls at most)

// speed: a smoothed cm per call. rain is called twice a move (120 calls a second at 60 fps; the characters move on every other
// call) and each call hands the simulation its own list, so a wake is pushed on every call from the smoothed speed
// a splash is pushed on at least 2 calls in a row (one frame): rain is called twice a move and each pass replaces the list the
// draw hands to the simulation (+0x164 / +0x19C, built from the forces), so a force pushed on one call reached the water
// only half the time (a Split's splash changed nothing at 1x or 10x strength, height map read back 2026-11-26). Faster
// impacts push longer (kSplashCalls more calls per unit of grow past 1): pushed for 4 frames, a plain jump's splash already
// reached the simulation's full height (+-ripple_height), so a Split looked no higher
constexpr int kSplashCalls = 6;
struct Splash { uintptr_t water; float x, y, size, strength; int calls; };
std::vector<Splash> g_splashes;

// fall: the fastest descent (cm a call) since it was last in the water
struct Track { float pos[3]; float height; float speed; float peak_height; float fall; uint32_t seen; };
std::unordered_map<uint64_t, Track> g_tracks;   // (water, unit) -> last frame's position
uint32_t g_call = 0;                            // rain-call counter (one a frame per water)
int g_call_pushes = 0;                          // forces pushed in this call (push_round's budget)
std::atomic<uint32_t> g_pushed{0};
uint32_t g_shown_pushed = 0, g_shown_waters = 0;
std::atomic<uint32_t> g_waters{0};

uintptr_t g_dti_player = 0, g_dti_enemy = 0;
bool g_dti_looked = false;
void (*g_watch)(uintptr_t water) = nullptr;

bool dti_under(uintptr_t dti, uintptr_t target) {
    for (int i = 0; dti && target && i < 16; ++i) {
        if (dti == target) return true;
        dti = *reinterpret_cast<const uintptr_t*>(dti + 0x10);   // MtDTI::mp_parent
    }
    return false;
}

// a 3x3 inverse (rows r0..r2 of the world matrix): local = (world - t) x inv
bool invert3(const float* r, float* inv) {
    const float a = r[0], b = r[1], c = r[2], d = r[4], e = r[5], f = r[6], g = r[8], h = r[9], i = r[10];
    const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (std::fabs(det) < 1e-12f) return false;
    const float k = 1.0f / det;
    inv[0] = (e * i - f * h) * k; inv[1] = (c * h - b * i) * k; inv[2] = (b * f - c * e) * k;
    inv[3] = (f * g - d * i) * k; inv[4] = (a * i - c * g) * k; inv[5] = (c * d - a * f) * k;
    inv[6] = (d * h - e * g) * k; inv[7] = (b * g - a * h) * k; inv[8] = (a * e - b * d) * k;
    return true;
}

// forces a pass may hold: move's vertex scratch takes both passes' quads, 1015 forces a move (WaterWakes.hpp); rain and
// wave push after these, so they keep their share
constexpr uint32_t kPassForces = 507;

void push(uintptr_t water, float x, float y, float size, float strength) {
    auto* base = *reinterpret_cast<float**>(water + 0x104);
    auto*& ptr = *reinterpret_cast<float**>(water + 0x108);
    if (!base || !ptr) return;
    const uint32_t used = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(base)) / 16;
    const uint32_t rain = *reinterpret_cast<const uint32_t*>(water + 0x138);                // mDestinyOfRain
    const uint32_t wave = *reinterpret_cast<const float*>(water + 0x13C) > 0.0f ? 32u : 0u;   // mWaveHeight
    static const bool no_cap = test_switch("DMC4_TEST_WAKE_NO_CAP") != nullptr;   // the list's own 512 only (the bug)
    if (used >= 512 || (!no_cap && used + rain + wave >= kPassForces)) return;   // full
    ptr[0] = x; ptr[1] = y; ptr[2] = size; ptr[3] = strength;
    ptr += 4;
    g_pushed.fetch_add(1, std::memory_order_relaxed);
    ++g_call_pushes;
}

// The simulation draws each force as a quad of one strength: uInteractiveWater::move turns the list into two triangles
// a force (sub_AC4B20: x / y +- size, the strength in every corner; size is a half extent, a rain drop's 1/256 = one
// texel), so a force bigger than a texel or two left a flat square, and a strong one a square plateau (user, 2026-11-26:
// "the injected force forms a squarish patch and doesn't look natural"). push_round lays a disk of small quads instead,
// on the height map's texel grid (centres at -1 + k x kTexel), each with the strength of a smooth bump at its distance,
// (1 - (d / radius)^2)^2. Each quad is two cells wide (half extent one cell, a rain drop's at one texel), so neighbours
// overlap: one-texel quads left nothing in the height map (the force pass seems to fade a quad towards its UV edges,
// and a one-texel quad is sampled at its edge), and kRoundGain makes up for the overlap. Bigger disks use coarser cells
// (at most kRoundCells), and past kCallBudget forces in a call the rest go as single quads, so the rain keeps room in the
// list (kPassForces at most, with the rain and wave).
constexpr float kTexel      = 2.0f / 512.0f;   // map units per texel of the 512 x 512 height map (st705 / BP floors)
// GUI multipliers on every force's strength and radius (user, 2026-11-26: "force should be more exaggerated"; defaults
// 2 / 1.5 over the values above)
float g_strength = 2.0f;
float g_size     = 1.5f;
constexpr int   kRoundCells = 320;   // 80 until 2026-11-26: a Split / Helm Breaker splash (~8 texels) got 1.7-texel cells,
                                     // and the impact ripple looked grid-like (user)
constexpr int   kCallBudget = 440;   // of the list's 512; the rain takes mDestinyOfRain a call (20 on BP stage 1)
// each cell's strength is squashed smoothly towards kCellMax (s -> kCellMax tanh(s / kCellMax)): a strong force saturated
// the simulation into a flat plateau with a steep rim, whose ring showed the grid
constexpr float kCellMax    = 1.5f;
constexpr float kRoundGain  = 1.0f;   // 0.5 left a splash at 0.13 (rain 0.07) in the height map
void push_round(uintptr_t water, float x, float y, float radius, float strength) {
    radius *= g_size;
    strength *= g_strength;
    if (radius <= kTexel || g_call_pushes > kCallBudget) {
        push(water, x, y, radius, strength);
        return;
    }
    float cell = kTexel;
    const float cells = 3.14159265f * radius * radius / (cell * cell);
    if (cells > kRoundCells) cell *= std::sqrt(cells / kRoundCells);
    const int n = static_cast<int>(std::ceil(radius / cell));
    const float cx = std::round((x + 1.0f) / kTexel) * kTexel - 1.0f;
    const float cy = std::round((y + 1.0f) / kTexel) * kTexel - 1.0f;
    for (int j = -n; j <= n; ++j) {
        for (int i = -n; i <= n; ++i) {
            const float px = cx + i * cell, py = cy + j * cell;
            const float q = ((px - x) * (px - x) + (py - y) * (py - y)) / (radius * radius);
            if (q >= 1.0f) continue;
            const float s = kRoundGain * strength * (1.0f - q) * (1.0f - q);
            push(water, px, py, cell, kCellMax * std::tanh(s / kCellMax));
        }
    }
}

// Footsteps (players; user, 2026-11-26: "should also reference the footprint collision system for the wake"): uPlayer has
// two cFootprintCtrl, +0x1BC0 / +0x1BE0, 0x20 bytes each. cFootprintCtrl::update (0x44F610) casts a 30 cm line down at the
// foot joint (sCollision::collNarrow_FootprintLine); +0x1C turns 1 on the frame it touches the floor (when the game spawns
// its footprint effect, a uEfctFootPrint) and stays 1 while the foot is down. +0x04 on, +0x08 the model, +0x0C the foot's
// joint number (model +744: number -> index table, model +740: the joints, 0x90 bytes each, the world matrix at +0x50
// (uModel::getJointWMat), so the foot at +0x80). Each touch-down in the water pushes a ripple at the foot, stronger the
// faster the character moves; the root's wake (wading) still runs for the trail
constexpr uintptr_t kFootCtrl[2] = { 0x1BC0, 0x1BE0 };
constexpr float kStep        = 45.0f;   // cm: a step's radius
constexpr float kStepBase    = 0.4f;    // strength: kStepBase + kStepPerCm x the smoothed cm a call, at most kStepMax
constexpr float kStepPerCm   = 0.12f;
constexpr float kStepMax     = 2.0f;
std::unordered_map<uintptr_t, uint8_t> g_feet;   // footprint controller -> its contact byte at the last call
uint32_t g_step_logged = 0;

void push_footsteps(uintptr_t water, uintptr_t player, const float* m, const float* inv, float up_len, float per_cm,
                    float speed) {
    for (const uintptr_t off : kFootCtrl) {
        const uintptr_t c = player + off;
        const uint8_t contact = *reinterpret_cast<const uint8_t*>(c + 0x1C);
        uint8_t& last = g_feet[c];
        const bool touched = contact && !last && *reinterpret_cast<const uint8_t*>(c + 4) == 1;
        last = contact;
        if (!touched) continue;
        const uintptr_t model = *reinterpret_cast<const uintptr_t*>(c + 8);
        const uintptr_t table = model ? *reinterpret_cast<const uintptr_t*>(model + 744) : 0;
        const uintptr_t joints = model ? *reinterpret_cast<const uintptr_t*>(model + 740) : 0;
        if (!table || !joints) continue;
        const uint8_t index = *reinterpret_cast<const uint8_t*>(table + *reinterpret_cast<const uint8_t*>(c + 0x0C));
        if (index == 255) continue;
        const float* f = reinterpret_cast<const float*>(joints + 0x90 * index + 0x80);
        const float d[3] = { f[0] - m[12], f[1] - m[13], f[2] - m[14] };
        const float mx = (d[0] * inv[0] + d[1] * inv[3] + d[2] * inv[6]) / 1000.0f;
        const float my = -(d[0] * inv[2] + d[1] * inv[5] + d[2] * inv[8]) / 1000.0f;   // map y = -local z
        const float height = (d[0] * m[4] + d[1] * m[5] + d[2] * m[6]) / up_len;
        if (std::fabs(mx) > 1.0f || std::fabs(my) > 1.0f || height > kAbove || height < -kBelow) continue;
        const float strength = std::min(kStepMax, kStepBase + kStepPerCm * speed);
        push_round(water, mx, my, kStep * per_cm, strength);
        if (g_call - g_step_logged > 120) {
            g_step_logged = g_call;
            spdlog::info("WaterWakes: footstep at {:.0f} cm below the surface, moving {:.1f} cm a call: strength {:.2f}",
                         -height, speed, strength);
        }
    }
}

// an actor's collision manager: the player's mCollMgr pointer (uPlayer +0x1E8C), an enemy's embedded one (+0x1600);
// either only if its report actor (+0xD8) is the actor
uintptr_t collision_mgr(uintptr_t actor, bool player) {
    const uintptr_t mgr = player ? *reinterpret_cast<const uintptr_t*>(actor + 0x1E8C) : actor + 0x1600;
    if (!mgr || *reinterpret_cast<const uintptr_t*>(mgr + 0xD8) != actor) return 0;
    return mgr;
}

// attack shapes cutting the water: the manager's groups (mppCollisionGroup +0x358, 32 slots) with mAttr (+8) bit 0 set
// (1 attack, 2 damage, 4 push, 8 grab: uCollisionMgr::setVsAttr), each cCollision (group +0x94 mNum, +0x9C mppCollision)
// as a segment: a capsule (kCollPrim type 3, cylinder 2) between mMat0's and mMat1's translations (+0x70, +0xB0), a
// sphere from last frame's centre (mOldMat0 +0xF0) to this one (the game sweeps attack spheres the same way,
// cCollisionGroup::updateAABB); radius = kCollPrim radius (+0xC) x mScale (+8). Where the segment is within its radius
// of the surface, forces of its size along that stretch, stronger the further the shape moved since last frame
int push_attacks(uintptr_t water, uintptr_t mgr, const float* m, const float* inv, float up_len, float per_cm) {
    int pushed = 0;
    const auto* slots = reinterpret_cast<const uintptr_t*>(mgr + 0x358);
    for (int s = 0; s < 32; ++s) {
        const uintptr_t g = slots[s];
        if (!g || (*reinterpret_cast<const uint32_t*>(g + 8) & 1) == 0) continue;
        const int num = *reinterpret_cast<const int*>(g + 0x94);
        const auto* cols = *reinterpret_cast<const uintptr_t* const*>(g + 0x9C);
        if (!cols || num <= 0 || num > 64) continue;
        for (int i = 0; i < num; ++i) {
            const uintptr_t c = cols[i];
            const uintptr_t prim = c ? *reinterpret_cast<const uintptr_t*>(c + 4) : 0;
            if (!prim) continue;
            const int type = *reinterpret_cast<const int*>(prim);
            if (type == 1) continue;   // boxes: none seen on attacks
            const float r = *reinterpret_cast<const float*>(prim + 0xC) * *reinterpret_cast<const float*>(c + 8);
            const float* now0 = reinterpret_cast<const float*>(c + 0x70);
            const float* old0 = reinterpret_cast<const float*>(c + 0xF0);
            const float* a = (type == 0) ? old0 : now0;
            const float* b = (type == 0) ? now0 : reinterpret_cast<const float*>(c + 0xB0);
            if (!(r > 0.0f && r < 1000.0f)) continue;
            const float moved[3] = { now0[0] - old0[0], now0[1] - old0[1], now0[2] - old0[2] };
            const float speed = std::sqrt(moved[0] * moved[0] + moved[1] * moved[1] + moved[2] * moved[2]);   // cm a frame
            if (!(speed >= kAttackMinSpeed && speed < 2000.0f)) continue;
            auto height = [&](const float* p) {
                return ((p[0] - m[12]) * m[4] + (p[1] - m[13]) * m[5] + (p[2] - m[14]) * m[6]) / up_len;
            };
            const float ha = height(a), hb = height(b);
            // the stretch of a..b within r of the surface: |ha + t (hb - ha)| <= r, t in 0..1
            float t0 = 0.0f, t1 = 1.0f;
            const float dh = hb - ha;
            if (std::fabs(dh) < 1e-4f) {
                if (std::fabs(ha) > r) continue;
            } else {
                float lo = (-r - ha) / dh, hi = (r - ha) / dh;
                if (lo > hi) std::swap(lo, hi);
                t0 = std::max(t0, lo);
                t1 = std::min(t1, hi);
                if (t0 > t1) continue;
            }
            const float seg[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] };
            const float len = (t1 - t0) * std::sqrt(seg[0] * seg[0] + seg[1] * seg[1] + seg[2] * seg[2]);
            const int n = std::clamp(static_cast<int>(len / std::max(r, 1.0f)) + 1, 1, kAttackPoints);
            const float strength = std::min(kAttackMaxStrength, 0.05f + kAttackPerCm * speed);
            for (int k = 0; k < n; ++k) {
                const float t = n == 1 ? 0.5f * (t0 + t1) : t0 + (t1 - t0) * k / (n - 1);
                const float d[3] = { a[0] + seg[0] * t - m[12], a[1] + seg[1] * t - m[13], a[2] + seg[2] * t - m[14] };
                const float mx = (d[0] * inv[0] + d[1] * inv[3] + d[2] * inv[6]) / 1000.0f;
                const float my = -(d[0] * inv[2] + d[1] * inv[5] + d[2] * inv[8]) / 1000.0f;   // map y = -local z
                if (std::fabs(mx) > 1.0f || std::fabs(my) > 1.0f) continue;
                push_round(water, mx, my, kAttackSize * r * per_cm, strength);
                ++pushed;
            }
            if (g_call - g_attack_logged > 120) {
                g_attack_logged = g_call;
                spdlog::info("WaterWakes: attack shape (type {}, r {:.0f} cm) cut the water at {:.0f} cm a frame: {} forces, "
                             "strength {:.2f}", type, r, speed, n, strength);
            }
        }
    }
    return pushed;
}

// the forces of the characters in this water (game thread, inside uInteractiveWater::move); push_wakes_raw guards it
void push_wakes_body(uintptr_t water) {
    {
        const float* m = reinterpret_cast<const float*>(water + 0x50);   // mWmat: rows x, y, z, translation
        float inv[9];
        if (!invert3(m, inv)) return;
        const float up_len = std::sqrt(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]);
        const float x_len = std::sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
        if (up_len < 1e-6f || x_len < 1e-6f) return;
        const float per_cm = 1.0f / (1000.0f * x_len);   // map units per cm along the surface
        const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
        if (!su) return;
        ++g_call;
        g_call_pushes = 0;
        for (size_t k = 0; k < g_splashes.size();) {   // splashes still being pushed
            Splash& sp = g_splashes[k];
            if (sp.water == water) {
                push_round(water, sp.x, sp.y, sp.size, sp.strength);
                if (--sp.calls <= 0) { g_splashes.erase(g_splashes.begin() + k); continue; }
            }
            ++k;
        }
        for (int line = 0; line < 32; ++line) {
            const auto* ml = reinterpret_cast<const MoveLine*>(su + 0x20 + line * sizeof(MoveLine));
            int guard = 0;
            for (uintptr_t u = reinterpret_cast<uintptr_t>(ml->mTop); u && guard < 20000; ++guard) {
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uintptr_t next = reinterpret_cast<uintptr_t>(cu->mp_next_unit);
                if (cu->flags.bits.mBeFlag == BEFLAG_MOVE) {
                    const uintptr_t dti = debug_server::mem::dti_of(u);
                    const bool player = dti && dti_under(dti, g_dti_player);
                    if (dti && (player || dti_under(dti, g_dti_enemy))) {
                        if (const uintptr_t mgr = collision_mgr(u, player))
                            push_attacks(water, mgr, m, inv, up_len, per_cm);
                        const float* p = reinterpret_cast<const float*>(u + 0x30);
                        const float d[3] = { p[0] - m[12], p[1] - m[13], p[2] - m[14] };
                        const float lx = d[0] * inv[0] + d[1] * inv[3] + d[2] * inv[6];
                        const float lz = d[0] * inv[2] + d[1] * inv[5] + d[2] * inv[8];
                        const float height = (d[0] * m[4] + d[1] * m[5] + d[2] * m[6]) / up_len;   // cm above the surface
                        const float mx = lx / 1000.0f, my = -lz / 1000.0f;   // map y = -local z (WaterWakes.hpp)
                        const uint64_t key = (static_cast<uint64_t>(water) << 32) | u;
                        auto it = g_tracks.find(key);
                        const bool known = it != g_tracks.end() && it->second.seen + 1 == g_call;
                        const bool inside = std::fabs(mx) <= 1.0f && std::fabs(my) <= 1.0f && height <= kAbove &&
                                            height >= -kBelow;
                        float speed = 0.0f, peak = height, fall = 0.0f;
                        if (known) {
                            const Track& t = it->second;
                            const float dx = p[0] - t.pos[0], dz = p[2] - t.pos[2];
                            const float step = std::sqrt(dx * dx + dz * dz);
                            speed = step < 200.0f ? 0.5f * t.speed + 0.5f * step : 0.0f;   // a warp resets it
                            peak = height > kAbove ? std::max(t.peak_height, height) : t.peak_height;
                            const float down = t.height - height;                          // cm down this call
                            if (height > kAbove) fall = std::max(t.fall, down);
                            if (inside && t.height > kAbove && height <= kAbove) {          // came down into it
                                const float drop = std::max(0.0f, t.peak_height - height);
                                const float hit = 2.0f * std::max(t.fall, down);            // cm a frame
                                const float grow = std::clamp(1.0f + (hit - kLandingSpeed) / kGrowSpeed, 1.0f, kMaxGrow);
                                const float strength = std::min(1.5f, 0.2f + 0.01f * drop) * grow * kSplashGain;
                                const int calls = 2 + static_cast<int>(std::lround(kSplashCalls * (grow - 1.0f)));
                                g_splashes.push_back({ water, mx, my, kSplash * grow * per_cm, strength, calls });
                                spdlog::info("WaterWakes: splash after a {:.0f} cm fall at {:.0f} cm a frame: {:.0f} cm, "
                                             "strength {:.2f}", drop, hit, kSplash * grow, strength);
                                peak = height;
                            } else if (inside && speed > 0.25f) {                           // wading
                                push_round(water, mx, my, kFootprint * per_cm, std::min(kWakeMax, kWakeBase + kWakePerCm * speed));
                            }
                            if (height <= kAbove) peak = height;
                        }
                        if (player)
                            push_footsteps(water, u, m, inv, up_len, per_cm, speed);
                        Track& t = g_tracks[key];
                        t.pos[0] = p[0]; t.pos[1] = p[1]; t.pos[2] = p[2];
                        t.height = height;
                        t.speed = speed;
                        t.peak_height = peak;
                        t.fall = fall;
                        t.seen = g_call;
                    }
                }
                u = next;
            }
        }
    }
}

// test switch DMC4_TEST_WAKE_FLOOD: as many single-texel forces as the list takes, on every pass, spread over the map
void push_flood(uintptr_t water) {
    for (int k = 0; k < 600; ++k) {
        auto* const before = *reinterpret_cast<float**>(water + 0x108);
        push(water, -0.9f + 0.075f * (k % 25), -0.9f + 0.075f * (k / 25 % 25), kTexel, 0.05f);
        if (*reinterpret_cast<float**>(water + 0x108) == before) break;
    }
}

// the walk under SEH (units run on this thread, but a character can go mid-walk). The body is its own function: a
// function with __try can't hold objects that need unwinding, and in Debug builds the standard library's iterators do
// (C2712; Release's are trivial, so only Debug failed)
void push_wakes_raw(uintptr_t water) {
    __try {
        push_wakes_body(water);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void __stdcall push_wakes(uintptr_t water) {
    if (!water) return;
    if (g_watch) g_watch(water);
    static const bool test_off = test_switch("DMC4_TEST_NO_WAKES") != nullptr;   // a run without the forces, setting kept
    if (!WaterWakes::mod_enabled || test_off) return;
    static const bool flood = test_switch("DMC4_TEST_WAKE_FLOOD") != nullptr;
    if (flood) {
        push_flood(water);
        return;
    }
    if (!g_dti_looked) {
        g_dti_player = debug_server::mem::find_dti("uPlayer");
        g_dti_enemy = debug_server::mem::find_dti("uEnemy");
        g_dti_looked = true;
    }
    g_waters.fetch_add(1, std::memory_order_relaxed);
    push_wakes_raw(water);
    if (g_splashes.size() > 64) g_splashes.clear();   // a water gone with its room: its splashes are never pushed
    if (g_feet.size() > 64) g_feet.clear();   // players gone with their room
    if (g_tracks.size() > 256) {   // characters gone: forget what wasn't seen lately
        for (auto it = g_tracks.begin(); it != g_tracks.end();)
            it = g_call - it->second.seen > 600 ? g_tracks.erase(it) : std::next(it);
    }
}

// 0xAC19DC, 5 bytes: call uInteractiveWater::rain (esi = the water). The forces go first, then the original call;
// rain and wave take their arguments in esi / non-volatile registers, which pushad / popad keep.
naked void detour_rain() {
    _asm {
        pushad
        push esi
        call push_wakes
        popad
        call dword ptr [rain_fn]
        jmp dword ptr [jmp_ret_rain]
    }
}

bool site_matches() {
    const auto* b = reinterpret_cast<const uint8_t*>(kRainCall);
    if (b[0] != 0xE8) return false;
    int32_t rel;
    std::memcpy(&rel, b + 1, 4);
    return kRainCall + 5 + rel == kRain;
}

void set_watch(void (*watch)(uintptr_t water)) {
    g_watch = watch;
}

} // namespace water_wakes

using namespace water_wakes;

std::optional<std::string> WaterWakes::on_initialize() {
    if (!site_matches() || !install_hook_absolute(kRainCall, hook_rain, &detour_rain, &jmp_ret_rain, 5)) {
        spdlog::error("WaterWakes: uInteractiveWater::move isn't the expected code (0xAC19DC); not installed");
        hook_rain.reset();
        mod_enabled = false;
    }
    return Mod::on_initialize();
}

void WaterWakes::on_frame(fmilliseconds& dt) {
    (void)dt;
    g_shown_pushed = g_pushed.exchange(0, std::memory_order_relaxed);
    g_shown_waters = g_waters.exchange(0, std::memory_order_relaxed);
}

void WaterWakes::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (!hook_rain) {
        ImGui::TextDisabled(_("Character wakes in water: not installed (see the log)"));
        return;
    }
    ImGui::Checkbox(_("Character wakes in water"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Players and enemies make ripples in interactive water (the practice stage's floor, water floors in "
                  "Bloody Palace mods): a wake while they move through it, a splash when they land in it, bigger for "
                  "dives and slams such as Split or Helm Breaker, and attacks that sweep through it cut it. The game itself only "
                  "ripples it with rain."));
    if (mod_enabled) {
        ImGui::SliderFloat(_("Wake strength"), &g_strength, 0.0f, 5.0f, "%.2f");
        ImGui::SameLine();
        help_marker(_("How hard wakes, splashes and attack cuts push the water. Past about 3 the middle of each one is at "
                      "the water's full height and only its edges grow."));
        ImGui::SliderFloat(_("Wake size"), &g_size, 0.5f, 3.0f, "%.2f");
        ImGui::SameLine();
        help_marker(_("How wide wakes, splashes and attack cuts are."));
        ImGui::TextDisabled(_("Last frame: %u ripples pushed into %u water surfaces"), g_shown_pushed, g_shown_waters);
    }
}

void WaterWakes::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("water_wakes").value_or(true);
    g_strength = std::clamp(cfg.get<float>("water_wakes_strength").value_or(2.0f), 0.0f, 5.0f);
    g_size = std::clamp(cfg.get<float>("water_wakes_size").value_or(1.5f), 0.5f, 3.0f);
}

void WaterWakes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("water_wakes", mod_enabled);
    cfg.set<float>("water_wakes_strength", g_strength);
    cfg.set<float>("water_wakes_size", g_size);
}
