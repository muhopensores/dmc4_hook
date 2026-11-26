#include "Remix.hpp"
#include "../../mod.hpp"
#include "../../sdk/Cam.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/Devil4.hpp"
#include "../debug_server/Memory.hpp"
#include "../tonemap/ToneMap.hpp"
#include "../../utility/Patch.hpp"
#include <d3d9.h>
#include <atomic>
#include <cstring>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <array>
#include <deque>
#include <cstdio>
#include <cmath>
#include <algorithm>
#define REMIX_ALLOW_X86                 // the API's types only: the bridge's 32-bit client takes the calls
#include "remix/remix_c.h"
#undef REMIX_ALLOW_X86

namespace remix {

Settings settings;

void count_view(uintptr_t vp);
void view_position(const D3DMATRIX& v, float* out);
extern int g_vp_transposed;     // -1 not decided, 1 = the game stores matrices transposed
bool g_main_uploaded = false;   // the main camera's block was uploaded in this main scene (its matrices are the D3D ones)
bool g_first_traced = false;    // the main scene's first draw Remix traces has gone by (its camera is Remix's for the frame)
int g_world_param = -1;         // gXfWorld's parameter (world positions)
uintptr_t g_cur_world = 0;      // its value now (a pointer to the matrix)
bool g_world_this_draw = false; // this draw's vertex parameters included gXfWorld (else it has none: skinned, sky ...)
int g_joints_param = -1;        // gXfJointWorld's parameter (skinned draws' bones, row-major 3x4 each)
uintptr_t g_cur_joints = 0;     // its value now (a pointer to the bone matrices)
bool g_joints_this_draw = false; // this draw's vertex parameters included gXfJointWorld
D3DMATRIX g_world_set{};        // the WORLD transform last given to D3D
// camera watch: the view / projection the frame's first traced draw carried (Remix's camera for the frame), the pass it
// came in and whether the main camera had been uploaded, and the last frame's main camera
D3DMATRIX g_first_view{}, g_first_proj{}, g_prev_main_view{};
bool g_first_valid = false, g_first_after_main = false, g_prev_main_valid = false;
uint32_t g_first_pass = 0, g_first_kind = 0, g_watch_lines = 0, g_watch_frame = 0;
float g_frame_vp[16];           // this main scene's camera (its first main upload), for the probe's copy check
bool g_have_frame_vp = false;   // the frame's camera (below): counts the draws per view block
namespace {

constexpr uintptr_t kCameraInstance = 0x00E552D4;  // sCamera::mpInstance
// sLight (the light manager): every uLight adds itself to its list when made (uLight::uLight 0xA6AB30 -> sub_A67080): the
// scheduler's units, effect particle lights (uSynchroPointLight / uSynchroSpotLight, not on any move line), enemies'
// (uEm019_00::setLight) and uScreenBreak's. The game lights models from this list (sub_A66180 / sub_A652B0): +0x20 the
// entries (1024), +0x2020 their count. Vibed/RE/light_units.md
constexpr uintptr_t kLightManager = 0x00E55788;
constexpr uint32_t  kMainScene = 0x1F;              // tag key bits 24-28
// tag key bits 29-31 = the drawing viewport's scene number (sCamera_ViewPort::mSceneNo: viewport 0 has 7, 1 has 6, ...)
uint32_t g_main_view = 7;                           // the view Remix traces (pick_traced_view)
// sRender's passes (tag key bits 20-23; names from off_E16B40, Vibed\RE\sRender_struct.md): 0 BEGIN, 1 ZPASS, 2 SOLID,
// 3 NUKI, 4 OCCLUSION, 5 OVERLAP, 6 TRANSPARENT, 7 EFFECT, 8 TRANSPARENT_NZ, 9 FILTER, 10 SCREEN, 11 END
constexpr uint32_t  kZPass = 1, kOcclusion = 4, kOverlap = 5, kFilter = 9, kScreen = 10, kEnd = 11;

std::atomic<uint32_t> g_skipped{ 0 }, g_seen{ 0 }, g_frames{ 0 }, g_moved{ 0 };
uint32_t g_skipped_last = 0, g_seen_last = 0, g_moved_last = 0;
constexpr uintptr_t kAlbedoParam = 0x00E5540C;   // mhXfSamplerAlbedoMap: its sShader::mParameterDesc index
constexpr uintptr_t kCameraBit   = 0x00E55780;   // `Camera`: which of a texture's two handles is current
constexpr uintptr_t kTexBound    = 0x8F54A8;     // the loop body after SetTexture (the sampler state)
constexpr uintptr_t kShaderInstance = 0x00E55700; // sShader::mpInstance
constexpr uintptr_t kParamDescs  = 0x838;        // ParameterDesc[512], 20 bytes: +8 the name's ID
// the water technique's samplers (XFUNIQUE IDs, as TexFilter): a pass sampling one of them without an albedo map is water
constexpr uint32_t kWaterIds[] = { 0xC70A6632 /*gXfFlowingWaterSampler*/, 0x73A29EF8 /*XfWaterNormalMapSampler*/,
                                   0xDEE93D6B /*XfWaterCausticsSampler*/,
                                   0xF186BC12 /*SamplerNormalMapFilter (XfInteractiveWater: the practice stage, the BP floor)*/,
                                   0x0FA373F0 /*SamplerReflectionMap*/, 0x46F9F252 /*SamplerRefractionMap*/,
                                   0x30386F08 /*XfFlatWaterNormalMapSampler*/ };
constexpr int kWaterCount = sizeof(kWaterIds) / sizeof(kWaterIds[0]);
int      g_water_param[kWaterCount] = { -1, -1, -1, -1, -1, -1, -1 };   // their parameter indices, looked up while missing
constexpr uint32_t kShadowNearId = 0xFC61C500;   // XfSamplerShadowNear: on stage 0 of the shadow overlay pass
int      g_shadow_param = -1;
// an effect pass binding only the depth buffer (XfPrimDepthPointSampler) has no texture: its glow is shaped in the pixel
// shader, which Remix doesn't run, and the depth buffer on stage 0 came out as white diamonds (lantern halos)
constexpr uint32_t kPrimDepthId = 0xB27FF634;
int      g_prim_depth_param = -1;
constexpr uint32_t kWaterZBufferId = 0xAAB42A50;   // XfWaterZBufferSampler: the sea-type water's depth fade
int      g_water_zbuf_param = -1;
// effects that bend the screen image (XfPrimSceneSampler: heat haze, refracting sword trails) through a distortion map;
// Remix sees only the distortion map, as a purple colour
constexpr uint32_t kPrimSceneId = 0xA56452E1;
int      g_prim_scene_param = -1;
// water draws get a moving texture transform on stage 0 (Remix 1.6.1 reads D3DTS_TEXTURE0 + TEXTURETRANSFORMFLAGS for every
// draw, d3d9_rtx_utils.cpp setTextureStageState; D3D9 ignores them for shader draws, so the game's image is unchanged):
// the layered water normals (rtx.opaqueMaterial.layeredWaterNormalEnable) offset their second sample by its translation
bool     g_tt_on = false;
// the sky: SOLID (pass 2) draws whose only sampler is XfBaseSampler (the sky panels / dome shader). Remix counts a draw
// whose viewport MinZ is >= rtx.skyMinZThreshold (1.0) as sky (d3d9_rtx.cpp setupCategoriesForHeuristics): rasterised
// into the background and the sky probe, so it lights the scene instead of blocking the sun. The game's viewport comes
// back at the next material (or pass, or the injection). The game's own raster of the sky is covered by the traced image.
constexpr uint32_t kBaseSamplerId = 0x4C3FB8D3;   // XfBaseSampler
int      g_base_param = -1;
bool     g_sky_vp = false;
D3DVIEWPORT9 g_saved_vp{};
std::atomic<uint32_t> g_sky{ 0 };
uint32_t g_sky_last = 0;

void sky_viewport(IDirect3DDevice9* dev, bool on) {
    if (on && !g_sky_vp) {
        D3DVIEWPORT9 vp{};
        if (FAILED(dev->GetViewport(&vp)) || vp.Width == 0 || vp.Height == 0) return;
        g_saved_vp = vp;
        vp.MinZ = 1.0f;
        vp.MaxZ = 1.0f;
        dev->SetViewport(&vp);
        g_sky_vp = true;
    } else if (!on && g_sky_vp) {
        dev->SetViewport(&g_saved_vp);
        g_sky_vp = false;
    }
}
LARGE_INTEGER g_qpf{}, g_qpc0{};

// tiling: the wave normal map's repeats per unit of the water's UVs (settings.water_tiling, or the interactive water's own)
void water_transform(IDirect3DDevice9* dev, bool on, float tiling = -1.0f) {
    if (on) {
        if (!g_qpf.QuadPart) { QueryPerformanceFrequency(&g_qpf); QueryPerformanceCounter(&g_qpc0); }
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        const double t = double(now.QuadPart - g_qpc0.QuadPart) / double(g_qpf.QuadPart);
        auto wrap = [](double x) { return float(x - std::floor(x)); };
        D3DMATRIX m{};
        m._11 = m._22 = tiling > 0.0f ? tiling : settings.water_tiling;
        m._33 = m._44 = 1.0f;
        m._31 = wrap(t * settings.water_flow[0]);   // 2-element texture coordinates: the third row is the translation
        m._32 = wrap(t * settings.water_flow[1]);
        dev->SetTransform(D3DTS_TEXTURE0, &m);
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
        g_tt_on = true;
    } else if (g_tt_on) {
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        g_tt_on = false;
    }
}
// Remix skips a draw whose stage 0 texture has no content hash ("Texture 0 without valid hash", d3d9_rtx.cpp): a
// default-pool texture that is never filled has none (d3d9_common_texture.cpp SetupForRtxFrom hashes the CPU copy; render
// targets get one from their size). Hidden draws get this one on stage 0. Released before device resets.
IDirect3DTexture9* g_hide = nullptr;
std::atomic<uint32_t> g_hidden{ 0 };
uint32_t g_hidden_last = 0;

// ---- the stage's lights ----
// Light units (sdk/uLight.hpp; values read live in room 400, 2026-11-24): uLight +0x18 attr, +0x1C light-group mask,
// +0x20 mColor (linear rgb, above 1 for the main light); uInfiniteLight +0x30 mDir = toward the light (the bright one has
// +y); MtVector3 is 16 bytes (x, y, z, pad): uPointLight +0x30 mPos, +0x40 mWPos, +0x50 mStart, +0x54 mEnd, +0x58 mpParent;
// uSpotLight +0x30 mPos, +0x60 mWPos, +0x84 mEnd, +0x94 mpParent (constructors 0xA6AEC0 / 0xA6B4D0; taken as a point
// light). A light without a parent is where its mPos says: mWPos is the game's copy, made when the light moves, so a
// light placed in photo mode (game paused) stayed at the origin. Remix makes its lights of the enabled D3D9 lights; D3D9 allows 8 at once:
// directional ones first, then the point lights that matter most to the view (nearest to the camera relative to their range).
struct GameLight { int kind; float col[3]; float v[3]; float range; uint32_t group; uintptr_t unit; };   // kind 1 directional (v = travel), 2 point (v = pos)
constexpr int kMaxLights = 8;              // D3D9 lights on at once
constexpr int kMaxAll = 72;               // lights handed over per frame, in batches of 8 (Remix keeps every light it
                                          // is given during a frame: LightManager::addLight, keyed by the light's hash)
GameLight g_lights[kMaxAll];
D3DLIGHT9 g_frame[kMaxAll + 2];
int       g_frame_n = 0, g_frame_next = 0;
int       g_nlights = 0, g_ndir = 0, g_npoint = 0, g_prev_enabled = 0;
// the stage's hemisphere (ambient) lights: their sky colours summed. Remix has no ambient term and DMC4's interiors get
// theirs from these and from baked lightmaps, so rooms went dark: the fill light (a wide point light at the camera)
float     g_hemi[3] = { 0, 0, 0 }, g_hemi_acc[3] = { 0, 0, 0 };
uintptr_t g_dti_hemi = 0;
// Light groups: a uLight lights a model only when its group mask (+0x1C) shares a bit with the model's mLightGroup
// (uModel +0x130; the .mod header's light group, 3 for characters by default). Stages keep alternative lights per group
// (room 307: three suns of 1.5 for characters, stage and breakables, five hemisphere colours); Remix has no groups and
// added them all up (blown out). The stage's dominant group = the most common mLightGroup among the models, leaving out
// the characters' 3; only lights sharing a bit with it go over (all of them if none does).
uintptr_t g_dti_model = 0;
uintptr_t g_dti_ocean = 0;
uintptr_t g_ocean_cube = 0;   // the live uOceanWater's mpCubemapTex (+0x1A4, an rTexture), gather_lights_raw
int       g_dup_skipped = 0;    // lights left out as copies of another (same place / direction, another light group)
int       g_unlit_skipped = 0;  // lights no model takes in the game (kept: the lightmaps hold their light)
bool      g_dir_kept[kMaxLights] = {};   // light watch: which of g_dirs_all went to Remix
uintptr_t g_dti_inf = 0, g_dti_point = 0, g_dti_spot = 0;
bool      g_dti_looked = false;
// light watch: every directional light the walk found (before the group filter) and the models' light groups the stage's
// group was picked from, kept by gather_lights_raw for light_watch
GameLight g_dirs_all[kMaxLights];
int       g_ndirs_all = 0;
int       g_attr_skipped = 0;   // light units left out by their mAttr this frame (game_uses)
int       g_off_skipped = 0;    // light units left out as switched off this frame (light_live)
// the dome's hemisphere lights (gather_lights_raw): uHemiSphereLight mColor +0x20 (towards mDir), mRevColor +0x30
// (away from it), mDir +0x40
struct HemiLight { uint32_t group; float sky[3], ground[3], dir[3]; };
HemiLight g_dome_hemis[16];
int       g_ndome_hemis = 0;
struct GroupCount { uint32_t mask, n; };
GroupCount g_group_counts[16];
int       g_ngroup_counts = 0;
uint32_t  g_light_lines = 0;
std::string g_light_sig;

// Whether the game's shaders get a light unit at all (Vibed/RE/light_units.md): mAttr (+0x18) 0x8 = per-pixel, any kind;
// 0x10 = simple, point lights only; 0x2 = folded into the SH ambient. Skinned models (characters) take every light with
// one of them (sub_A652B0, gate 0x1A), other models only 0x8 / 0x10 ones (sub_A66180, gate 0x18). Remix lights both alike,
// so the broader rule: a point light needs 0x2, 0x8 or 0x10, a sun, spot or hemisphere light 0x2 or 0x8 (st001's spots
// have 0x50 and light nothing; st300's lamps have 0x42, ambient for characters). 0x4 negates the colour (a darkening
// light), which Remix can't do, so those are left out too.
bool game_uses(uint8_t attr, int kind) {
    if (attr & 0x4) return false;
    return (attr & (kind == 2 ? 0x1A : 0x0A)) != 0;   // kind 2 = point
}

// Whether the game counts a light unit as on (the unit test of sub_A66180 / sub_A652B0): moving (mBeFlag 2), drawn and
// visible (mUnitAttr UATTR_DRAW | UATTR_VISIBLE), its move line drawn (MoveLine +8 bit 2, mTrans; line 63 = none) and in
// some view (mTransView; the game ANDs it with the view's bit). Rooms load alternative light sets and show one: st300 has
// ml00H / ml00S / ml01F / ml01H / ml01S, only ml01F's units are drawn and visible (2026-11-26), and the walk took them all.
bool light_live(const cUnit* cu, uintptr_t su) {
    const auto& b = cu->flags.bits;
    constexpr uint32_t shown = UATTR_DRAW | UATTR_VISIBLE;
    if (b.mBeFlag != BEFLAG_MOVE || (b.mUnitAttr & shown) != shown || b.mTransView == 0) return false;
    if (b.mMoveLine == 63) return true;
    if (b.mMoveLine >= 32) return false;
    return reinterpret_cast<const MoveLine*>(su + 0x20 + b.mMoveLine * sizeof(MoveLine))->mTrans != 0;
}

bool dti_under(uintptr_t dti, uintptr_t target) {
    for (int i = 0; dti && target && i < 16; ++i) {
        if (dti == target) return true;
        dti = *reinterpret_cast<const uintptr_t*>(dti + 0x10);   // MtDTI::mp_parent
    }
    return false;
}

// walks every move line's units (render thread, while the game thread runs them: SEH-guarded, returns -1 on a fault)
int gather_lights_raw(GameLight* out, int max) {
    __try {
        const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
        if (!su) return 0;
        g_hemi_acc[0] = g_hemi_acc[1] = g_hemi_acc[2] = 0;
        int skipped = 0, off = 0;
        uintptr_t ocean = 0;
        constexpr int kMaxPoints = 64;
        GameLight dirs[kMaxLights], points[kMaxPoints];
        float score[kMaxPoints];
        int nd = 0, np = 0;
        HemiLight hemis[16];
        int nh = 0;
        struct Mask { uint32_t mask, n; } masks[16];
        int nm = 0;
        float cam[3] = { 0, 0, 0 };
        if (const uintptr_t camera = *reinterpret_cast<uintptr_t*>(0x00E552D4)) {
            D3DMATRIX cv;
            std::memcpy(&cv, &reinterpret_cast<const sCamera*>(camera)->viewports[0].mViewMat, sizeof cv);
            for (int i = 0; i < 3; ++i) cam[i] = -(cv.m[3][0] * cv.m[i][0] + cv.m[3][1] * cv.m[i][1] + cv.m[3][2] * cv.m[i][2]);
        }
        // models (move lines): their light groups, for the stage's group below
        for (int line = 0; line < 32; ++line) {
            const auto* ml = reinterpret_cast<const MoveLine*>(su + 0x20 + line * sizeof(MoveLine));
            int guard = 0;
            for (uintptr_t u = reinterpret_cast<uintptr_t>(ml->mTop); u && guard < 20000; ++guard) {
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uintptr_t next = reinterpret_cast<uintptr_t>(cu->mp_next_unit);
                if (cu->flags.bits.mBeFlag && g_dti_ocean && !ocean && (cu->flags.bits.mUnitAttr & UATTR_DRAW) &&
                    dti_under(debug_server::mem::dti_of(u), g_dti_ocean))
                    ocean = *reinterpret_cast<const uintptr_t*>(u + 0x1A4);
                if (cu->flags.bits.mBeFlag && g_dti_model) {
                    const uintptr_t dti = debug_server::mem::dti_of(u);
                    if (dti && dti_under(dti, g_dti_model)) {
                        const uint32_t m = *reinterpret_cast<const uint32_t*>(u + 0x130);
                        if (m) {
                            int i = 0;
                            while (i < nm && masks[i].mask != m) ++i;
                            if (i < nm) ++masks[i].n;
                            else if (nm < 16) masks[nm++] = { m, 1 };
                        }
                    }
                }
                u = next;
            }
        }
        // the lights: the game's own list (kLightManager), in its order; kind from the type byte (+0x19: 1 directional,
        // 2 point, 3 spot, set by the class; effect lights are point / spot subclasses), 0 = hemisphere (no type)
        if (const uintptr_t mgr = *reinterpret_cast<const uintptr_t*>(kLightManager)) {
            const uint32_t count = std::min<uint32_t>(*reinterpret_cast<const uint32_t*>(mgr + 0x2020), 1024);
            const auto* list = reinterpret_cast<const uintptr_t*>(mgr + 0x20);
            for (uint32_t li = 0; li < count; ++li) {
                const uintptr_t u = list[li];
                if (!u) continue;
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uint8_t type = *reinterpret_cast<const uint8_t*>(u + 0x19);
                int kind = (type >= 1 && type <= 3) ? type : -1;
                if (kind < 0 && g_dti_hemi && dti_under(debug_server::mem::dti_of(u), g_dti_hemi)) kind = 0;
                if (kind < 0) continue;
                if (!light_live(cu, su)) { ++off; continue; }
                const uint8_t attr = *reinterpret_cast<const uint8_t*>(u + 0x18);
                if (!game_uses(attr, kind)) { ++skipped; continue; }
                const float* col = reinterpret_cast<const float*>(u + 0x20);
                const uint32_t group = *reinterpret_cast<const uint32_t*>(u + 0x1C);
                if (kind == 0) {
                    if (nh < 16) {
                        const float* rev = reinterpret_cast<const float*>(u + 0x30);
                        const float* d = reinterpret_cast<const float*>(u + 0x40);
                        hemis[nh++] = { group, { col[0], col[1], col[2] }, { rev[0], rev[1], rev[2] }, { d[0], d[1], d[2] } };
                    }
                } else if (kind == 1) {
                    if (nd < kMaxLights) {
                        const float* d = reinterpret_cast<const float*>(u + 0x30);
                        dirs[nd++] = { 1, { col[0], col[1], col[2] }, { -d[0], -d[1], -d[2] }, 0.0f, group };
                    }
                } else if (np < kMaxPoints) {
                    const bool spot = kind == 3;
                    const uintptr_t parent = *reinterpret_cast<const uintptr_t*>(u + (spot ? 0x94 : 0x58));
                    const float* p = reinterpret_cast<const float*>(u + (parent ? (spot ? 0x60 : 0x40) : 0x30));
                    const float end = *reinterpret_cast<const float*>(u + (spot ? 0x84 : 0x54));
                    const float dx = p[0] - cam[0], dy = p[1] - cam[1], dz = p[2] - cam[2];
                    score[np] = std::sqrt(dx * dx + dy * dy + dz * dz) / (end > 1.0f ? end : 1.0f);
                    points[np++] = { 2, { col[0], col[1], col[2] }, { p[0], p[1], p[2] }, end, group, u };
                }
            }
        }
        // Copies: stages repeat a light once per light group, each copy lighting its own models in the game (room 1: three
        // suns in one direction, groups 0x806 / 0x20 / 0x818; room 100: three lamps at one spot, 0x1 / 0x20 / 0x80;
        // room 307: suns for characters, stage and breakables). Remix has no groups and added them up (blown out). Of
        // lights in about one direction (suns: cos > 0.95; rooms 12 / 13 tune each group's copy 9-11 degrees apart) or one
        // place (points / spots: within 50 cm), the one lighting the most stage models (their mLightGroup shares a bit
        // with its group; characters' default 3 left out: every player, weapon and prop counts once while the stage is a
        // few big models, so counting them picked room 1's character sun) stays, then the one lighting the most models,
        // then the brightest. A light no model takes still goes over (counted only): fully lightmapped stages give their
        // models light group 0, so the game's own lights for them only live in the lightmaps, which Remix doesn't have
        // (BP stage 1, st705: its key lights are group 0x600, the brightest 7.4; dropping them left the stage unlit). Until 2026-11-26 the bridge kept only the lights of the stage's commonest model group, which also
        // dropped lights that had no copy (room 100's sun: group 0x1E against 0x2020).
        auto coverage = [&](uint32_t g, bool stage_only) {
            int c = 0;
            for (int i = 0; i < nm; ++i)
                if ((masks[i].mask & g) && !(stage_only && masks[i].mask == 3)) c += masks[i].n;
            return c;
        };
        // which of two copies stays: more stage models, then more models, then brighter
        auto wins = [&](const GameLight& a, const GameLight& b) {
            const int sa = coverage(a.group, true), sb = coverage(b.group, true);
            if (sa != sb) return sa > sb;
            const int ta = coverage(a.group, false), tb = coverage(b.group, false);
            if (ta != tb) return ta > tb;
            return a.col[0] + a.col[1] + a.col[2] > b.col[0] + b.col[1] + b.col[2];
        };
        int dup = 0, unlit = 0;
        bool drop_d[kMaxLights] = {};
        int cov_d[kMaxLights];
        for (int i = 0; i < nd; ++i) {
            cov_d[i] = coverage(dirs[i].group, false);
            if (nm > 0 && cov_d[i] == 0) ++unlit;
        }
        for (int i = 0; i < nd; ++i)
            for (int j = i + 1; j < nd && !drop_d[i]; ++j) {
                if (drop_d[j]) continue;
                const float dot = dirs[i].v[0] * dirs[j].v[0] + dirs[i].v[1] * dirs[j].v[1] + dirs[i].v[2] * dirs[j].v[2];
                if (dot < 0.95f) continue;
                drop_d[wins(dirs[j], dirs[i]) ? i : j] = true;
                ++dup;
            }
        bool drop_p[kMaxPoints] = {};
        int cov_p[kMaxPoints];
        for (int i = 0; i < np; ++i) {
            cov_p[i] = coverage(points[i].group, false);
            if (nm > 0 && cov_p[i] == 0) ++unlit;
        }
        for (int i = 0; i < np; ++i)
            for (int j = i + 1; j < np && !drop_p[i]; ++j) {
                if (drop_p[j]) continue;
                const float dx = points[i].v[0] - points[j].v[0], dy = points[i].v[1] - points[j].v[1],
                            dz = points[i].v[2] - points[j].v[2];
                if (dx * dx + dy * dy + dz * dz > 2500.0f) continue;
                drop_p[wins(points[j], points[i]) ? i : j] = true;
                ++dup;
            }
        {   // compact the points kept
            int k = 0;
            for (int i = 0; i < np; ++i) if (!drop_p[i]) { points[k] = points[i]; score[k] = score[i]; ++k; }
            np = k;
        }
        for (int i = 0; i < nh; ++i)                          // the fill's colour: the brightest hemisphere light used
            if ((nm == 0 || coverage(hemis[i].group, false) > 0) &&
                hemis[i].sky[0] + hemis[i].sky[1] + hemis[i].sky[2] > g_hemi_acc[0] + g_hemi_acc[1] + g_hemi_acc[2])
                for (int c = 0; c < 3; ++c) g_hemi_acc[c] = hemis[i].sky[c];
        g_dup_skipped = dup;
        g_unlit_skipped = unlit;
        int n = 0;
        for (int i = 0; i < nd && n < max; ++i) if (!drop_d[i]) out[n++] = dirs[i];
        while (n < max && np > 0) {   // the most relevant point light left: nearest relative to its range
            int best = 0;
            for (int i = 1; i < np; ++i) if (score[i] < score[best]) best = i;
            out[n++] = points[best];
            points[best] = points[np - 1];
            score[best] = score[np - 1];
            --np;
        }
        for (int i = 0; i < nd; ++i) { g_dirs_all[i] = dirs[i]; g_dir_kept[i] = !drop_d[i]; }
        g_ndirs_all = nd;
        for (int i = 0; i < nm; ++i) g_group_counts[i] = { masks[i].mask, masks[i].n };
        g_ngroup_counts = nm;
        g_attr_skipped = skipped;
        g_off_skipped = off;
        // the dome: the hemisphere lights that light characters (the default model light group 3: what the game's
        // hemisphere lights reach, sub_A652B0), else every one that's on
        int nk = 0;
        for (int i = 0; i < nh; ++i) if (hemis[i].group & 3) g_dome_hemis[nk++] = hemis[i];
        if (nk == 0) for (int i = 0; i < nh; ++i) g_dome_hemis[nk++] = hemis[i];
        g_ndome_hemis = nk;
        g_ocean_cube = ocean;
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

// light watch (always on, like the camera watch): a line in remix_probe.txt whenever the suns handed to Remix or the stage
// light group change (400 lines a session at most), with the models' light groups the group was picked from. A sun that
// flips between two directions moves far shadow edges a lot: a Fortuna wall's lower half went from lit to shadowed and
// back for a while, then stopped (2026-11-24), which this is meant to catch
void light_watch() {
    std::string sig;
    char buf[192];
    sprintf_s(buf, "%d lights off, %d left out by mAttr, %d copies, %d for no model (kept); %d hemisphere lights for the dome;",
              g_off_skipped, g_attr_skipped, g_dup_skipped, g_unlit_skipped, g_ndome_hemis);
    sig += buf;
    for (int i = 0; i < g_ndirs_all; ++i) {
        const GameLight& d = g_dirs_all[i];
        const bool kept = g_dir_kept[i];
        sprintf_s(buf, " sun %d%s: travels %.2f %.2f %.2f, colour %.2f %.2f %.2f, group %08X;", i, kept ? "" : " (left out)",
                  d.v[0], d.v[1], d.v[2], d.col[0], d.col[1], d.col[2], d.group);
        sig += buf;
    }
    if (sig == g_light_sig) return;
    g_light_sig = sig;
    if (g_light_lines >= 400) return;
    std::string groups;
    for (int i = 0; i < g_ngroup_counts; ++i) {
        sprintf_s(buf, " %08X x%u", g_group_counts[i].mask, g_group_counts[i].n);
        groups += buf;
    }
    FILE* f = nullptr;
    if (fopen_s(&f, "remix_probe.txt", "a") == 0 && f) {
        fprintf(f, "light watch, frame %u: %s %d point lights; models' light groups:%s\n", g_watch_frame + 1,
                sig.c_str(), g_npoint, groups.empty() ? " none" : groups.c_str());
        fclose(f);
        ++g_light_lines;
    }
}

void gather_lights() {
    if (!g_dti_looked) {
        g_dti_inf = debug_server::mem::find_dti("uInfiniteLight");
        g_dti_point = debug_server::mem::find_dti("uPointLight");
        g_dti_spot = debug_server::mem::find_dti("uSpotLight");
        g_dti_hemi = debug_server::mem::find_dti("uHemiSphereLight");
        g_dti_ocean = debug_server::mem::find_dti("uOceanWater");
        g_dti_model = debug_server::mem::find_dti("uModel");
        g_dti_looked = g_dti_inf != 0;
    }
    GameLight tmp[kMaxAll];
    const int n = gather_lights_raw(tmp, kMaxAll);
    if (n < 0) return;   // a unit went away mid-walk: keep last frame's
    for (int i = 0; i < 3; ++i) g_hemi[i] = g_hemi_acc[i];
    g_nlights = n;
    g_ndir = g_npoint = 0;
    for (int i = 0; i < n; ++i) {
        g_lights[i] = tmp[i];
        if (tmp[i].kind == 1) ++g_ndir; else ++g_npoint;
    }
    light_watch();
}

// Remix rasterises every draw after the first one it takes for UI ("RTX injection", d3d9_rtx.cpp internalPrepareDraw /
// makeDrawCallType: a fixed-function draw to the screen-sized target with an orthographic projection, _44 == 1, and
// depth writes off). DMC4's HUD uses vertex shaders, so Remix traced it as world geometry (it slid around with the
// camera). One degenerate fixed-function triangle like that at the start of the HUD pass starts the injection there; the
// state block puts the device back as the game left it (its caches stay right).
// Remix's final output pass tone-maps its image and encodes it for the display (rtx_context.cpp: dispatchToneMapping,
// dispatchSRGBDither with the hard-wired g_allowSrgbConversionForOutput) before blitting it into the target, here DMC4's
// scene buffer. DMC4 lights in gamma space and its final pass (XfFilter composites, Vibed\ToneMap\README.md) reads that
// buffer back x 2: no curve on most stages (TONE 0), Reinhard at an adapting exposure on a few, and ToneMap's ACES / AgX
// when that mod is on. Remix's image straight in came out blown out (x 2); converted to linear light (the old fix) it
// crushed everything below mid grey (sRGB 0.2 -> 0.033 -> 0.066 on screen) and the curves darkened it again. Now: right
// after the injection the buffer is copied out and drawn back at half (x exposure), and ToneMap's shaders take the final
// pass with no curve (tone_map::passthrough, set in on_tag), so the screen shows Remix's image with the game's bloom.
const char* kLinearizeSrc =
    "sampler2D s0 : register(s0);\n"
    "float4 k : register(c0);\n"   // x = 0.5 x exposure
    "float4 main(float2 uv : TEXCOORD0) : COLOR {\n"
    "    return float4(saturate(tex2D(s0, uv).rgb) * k.x, 1.0);\n"
    "}\n";
IDirect3DPixelShader9* g_ps_linearize = nullptr;
bool               g_lin_failed = false;
IDirect3DTexture9* g_lin_copy = nullptr;            // default pool: released before device resets
UINT g_lin_w = 0, g_lin_h = 0; D3DFORMAT g_lin_fmt = D3DFMT_UNKNOWN;

struct LinBlob : IUnknown {   // ID3DBlob, without the D3DCompiler headers (as PostFX)
    virtual void* STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual SIZE_T STDMETHODCALLTYPE GetBufferSize() = 0;
};

bool linearize_shader(IDirect3DDevice9* dev) {
    if (g_ps_linearize || g_lin_failed) return g_ps_linearize != nullptr;
    using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const void*, void*, const char*, const char*, UINT,
                                         UINT, LinBlob**, LinBlob**);
    HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
    auto compile = dll ? reinterpret_cast<D3DCompileFn>(GetProcAddress(dll, "D3DCompile")) : nullptr;
    LinBlob *code = nullptr, *err = nullptr;
    if (!compile || FAILED(compile(kLinearizeSrc, strlen(kLinearizeSrc), "remix_linearize", nullptr, nullptr, "main", "ps_3_0",
                                   0, 0, &code, &err)) || !code ||
        FAILED(dev->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()), &g_ps_linearize))) {
        g_lin_failed = true;
        g_ps_linearize = nullptr;
    }
    if (code) code->Release();
    if (err) err->Release();
    return g_ps_linearize != nullptr;
}

void linearize(IDirect3DDevice9* dev) {
    if (!settings.linearize || !linearize_shader(dev)) return;
    IDirect3DSurface9 *rt = nullptr, *copy = nullptr;
    if (FAILED(dev->GetRenderTarget(0, &rt)) || !rt) return;
    D3DSURFACE_DESC d{};
    rt->GetDesc(&d);
    if (!g_lin_copy || g_lin_w != d.Width || g_lin_h != d.Height || g_lin_fmt != d.Format) {
        if (g_lin_copy) { g_lin_copy->Release(); g_lin_copy = nullptr; }
        if (FAILED(dev->CreateTexture(d.Width, d.Height, 1, D3DUSAGE_RENDERTARGET, d.Format, D3DPOOL_DEFAULT, &g_lin_copy, nullptr)))
            g_lin_copy = nullptr;
        g_lin_w = d.Width; g_lin_h = d.Height; g_lin_fmt = d.Format;
    }
    IDirect3DStateBlock9* sb = nullptr;
    if (g_lin_copy && SUCCEEDED(g_lin_copy->GetSurfaceLevel(0, &copy)) &&
        SUCCEEDED(dev->StretchRect(rt, nullptr, copy, nullptr, D3DTEXF_NONE)) &&
        SUCCEEDED(dev->CreateStateBlock(D3DSBT_ALL, &sb)) && sb) {
        struct V { float x, y, z, rhw, u, v; };
        const float w = float(d.Width), h = float(d.Height);
        const V v[3] = { { -0.5f, -0.5f, 0, 1, 0, 0 }, { 2 * w - 0.5f, -0.5f, 0, 1, 2, 0 }, { -0.5f, 2 * h - 0.5f, 0, 1, 0, 2 } };
        const float k[4] = { 0.5f * settings.exposure, 0, 0, 0 };   // the final pass doubles it
        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(g_ps_linearize);
        dev->SetPixelShaderConstantF(0, k, 1);
        dev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
        dev->SetTexture(0, g_lin_copy);
        dev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
        dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        dev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        dev->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, v, sizeof(V));
        sb->Apply();
    }
    if (sb) sb->Release();
    if (copy) copy->Release();
    rt->Release();
}

bool g_ui_done = false;
bool g_solid_seen = false;   // this frame's main-scene SOLID (2) pass has started: the 3D is in
// the frame's tag order once a minute (every 3600th frame): the (view, sub-scene, pass) runs, to remix_passes.txt
uint32_t g_order[256]; int g_norder = 0; bool g_order_written = false;
uintptr_t g_order_rt[256]; uint32_t g_order_desc[256];   // the bound render target of each run, its w | h << 12 | fmt << 24
std::atomic<uint32_t> g_presents{ 0 };                  // PostFX::on_frame calls (one per present)
uint32_t g_disp_since_present = 0, g_disp_per_present_max = 0, g_ui_fired_pass = 0xFF; uintptr_t g_ui_fired_rt = 0;
std::atomic<uint32_t> g_ui_triggers{ 0 };

void trigger_ui(IDirect3DDevice9* dev) {
    IDirect3DStateBlock9* sb = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &sb)) || !sb) return;
    struct V { float x, y, z; DWORD c; } v[3] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
    D3DMATRIX ortho{};
    ortho._11 = ortho._22 = ortho._33 = ortho._44 = 1.0f;
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->SetTransform(D3DTS_PROJECTION, &ortho);
    dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
    dev->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, v, sizeof(V));
    sb->Apply();
    sb->Release();
    ++g_ui_triggers;
}

// --- one-frame probe (settings.probe): what the main view's target holds at each pass, around the injection and after the
// present, read back through a 64 x 36 copy (GetRenderTargetData: the bridge sends the pixels back), to remix_probe.txt ---
int g_probe = 0;                       // 2 = this frame is probed (its samples come in until the next frame's first tag)
uintptr_t g_other_cam = 0;             // the camera block of the draws now under a passed-over camera (0 = the main one)
uintptr_t g_cur_vp = 0;                // the camera block the main scene's draws use now (perspective ones only)
bool g_other_ortho = false;            // ... and it's orthographic
std::atomic<uint32_t> g_other_draws{ 0 };
uint32_t g_other_draws_last = 0;
std::unordered_set<std::string> g_probe_layouts;
int g_block_kind = 0;                  // the view-projection block of the draws now: 0 the main one, 1 a copy of it, 2 another
std::unordered_set<std::string> g_probe_paths;               // texture lines already in this probe
std::unordered_set<std::string> g_probe_states;              // state lines already in this probe
std::unordered_map<uintptr_t, int> g_probe_tex_count;        // draws per albedo map so far in the probed frame
std::unordered_map<uintptr_t, std::string> g_tex_paths;      // cTrans::Texture -> its rTexture's path (built at probe start)

// every loaded texture with its path: rTextures in sResource's table (+0x1038, 8192 cResource*; rTexture vtable 0xC035A8,
// mPath +0x04, mpTexture +0xCC = the cTrans::Texture the bind reads)
template <class F> void for_each_texture(F&& f) {
    const uintptr_t res = *reinterpret_cast<const uintptr_t*>(0x00E552D0);   // sDevil4Resource::mpInstance
    if (!res) return;
    auto* cs = reinterpret_cast<LPCRITICAL_SECTION>(res + 0x04);
    EnterCriticalSection(cs);
    for (int i = 0; i < 8192; ++i) {
        const uintptr_t r = *reinterpret_cast<const uintptr_t*>(res + 0x1038 + 4 * i);
        if (!r || *reinterpret_cast<const uintptr_t*>(r) != 0xC035A8) continue;
        const uintptr_t t = *reinterpret_cast<const uintptr_t*>(r + 0xCC);
        char p[65];
        memcpy(p, reinterpret_cast<const char*>(r + 4), 64);
        p[64] = 0;
        if (t) f(t, static_cast<const char*>(p));
    }
    LeaveCriticalSection(cs);
}

void build_tex_paths() {   // the probe's texture names
    g_tex_paths.clear();
    for_each_texture([](uintptr_t t, const char* p) { g_tex_paths[t] = p; });
}

// settings.hide_textures: the textures whose path matches, rebuilt when the text changes and every second (textures load
// and unload)
std::unordered_set<uintptr_t> g_hide_tex;
char g_hide_built_for[64] = "";

bool path_matches(const char* path, const char* filter) {   // any ';'-separated word of filter in path, any case
    for (const char* f = filter; *f;) {
        const char* end = strchr(f, ';');
        const size_t len = end ? size_t(end - f) : strlen(f);
        if (len)
            for (const char* p = path; *p; ++p)
                if (_strnicmp(p, f, len) == 0) return true;
        if (!end) break;
        f = end + 1;
    }
    return false;
}

std::unordered_set<uintptr_t> g_hide_water_tex;   // settings.hide_water_textures' textures
char g_hide_water_built_for[64] = "";

std::unordered_set<uintptr_t> g_light_tex;        // settings.light_textures' textures
char g_light_built_for[128] = "";

std::unordered_set<uintptr_t> g_sky_tex;          // settings.sky_textures' textures
char g_sky_built_for[64] = "";
std::unordered_set<uintptr_t> g_terrain_tex;      // settings.terrain_textures' textures
char g_terrain_built_for[2048] = "";
std::unordered_set<uintptr_t> g_mask_tex;         // with terrain_textures: the blend masks (paths ending _AM)
std::atomic<uint32_t> g_sky_tex_frame{ 0 };       // g_frames when one was last drawn (0 = not since the start)
std::string g_sky_tex_path;                       // the first one's path (the GUI's)

void build_hide_textures() {
    g_hide_tex.clear();
    g_hide_water_tex.clear();
    g_light_tex.clear();
    g_sky_tex.clear();
    g_sky_tex_path.clear();
    memcpy(g_sky_built_for, settings.sky_textures, sizeof g_sky_built_for);
    g_sky_built_for[sizeof g_sky_built_for - 1] = 0;
    g_terrain_tex.clear();
    g_mask_tex.clear();
    memcpy(g_terrain_built_for, settings.terrain_textures, sizeof g_terrain_built_for);
    g_terrain_built_for[sizeof g_terrain_built_for - 1] = 0;
    memcpy(g_light_built_for, settings.light_textures, sizeof g_light_built_for);
    g_light_built_for[sizeof g_light_built_for - 1] = 0;
    memcpy(g_hide_built_for, settings.hide_textures, sizeof g_hide_built_for);
    g_hide_built_for[sizeof g_hide_built_for - 1] = 0;
    memcpy(g_hide_water_built_for, settings.hide_water_textures, sizeof g_hide_water_built_for);
    g_hide_water_built_for[sizeof g_hide_water_built_for - 1] = 0;
    if (!g_hide_built_for[0] && !g_hide_water_built_for[0] && !g_light_built_for[0] && !g_sky_built_for[0] &&
        !g_terrain_built_for[0])
        return;
    for_each_texture([](uintptr_t t, const char* p) {
        if (g_sky_built_for[0] && path_matches(p, g_sky_built_for)) {
            g_sky_tex.insert(t);
            if (g_sky_tex_path.empty()) g_sky_tex_path = p;
        }
        if (g_hide_built_for[0] && path_matches(p, g_hide_built_for)) g_hide_tex.insert(t);
        if (g_terrain_built_for[0] && path_matches(p, g_terrain_built_for)) g_terrain_tex.insert(t);
        if (g_terrain_built_for[0]) {   // DMC4's blend masks: "..._AM" (st307-t-alpha00_AM)
            const size_t len = strnlen(p, 64);
            if (len > 3 && p[len - 3] == '_' && (p[len - 2] | 0x20) == 'a' && (p[len - 1] | 0x20) == 'm') g_mask_tex.insert(t);
        }
        if (g_light_built_for[0] && path_matches(p, g_light_built_for)) g_light_tex.insert(t);
        if (g_hide_water_built_for[0] && path_matches(p, g_hide_water_built_for)) g_hide_water_tex.insert(t);
    });
}

// the probe's shader dumps: each draw's vertex / pixel shader bytecode (as Remix got it, the bridge's patches included)
// to remix_shaders\vs_<handle>.bin / ps_<handle>.bin, once per pair
std::unordered_set<uint64_t> g_dumped_pairs;
void dump_shader_pair(IDirect3DVertexShader9* vs, IDirect3DPixelShader9* ps) {
    const uint64_t key = (uint64_t(reinterpret_cast<uintptr_t>(vs)) << 32) | reinterpret_cast<uintptr_t>(ps);
    if (g_dumped_pairs.size() >= 300 || !g_dumped_pairs.insert(key).second) return;
    CreateDirectoryA("remix_shaders", nullptr);
    auto save = [](const char* name, auto* sh) {
        if (!sh) return;
        UINT size = 0;
        if (FAILED(sh->GetFunction(nullptr, &size)) || !size) return;
        std::vector<char> buf(size);
        if (FAILED(sh->GetFunction(buf.data(), &size))) return;
        FILE* f = nullptr;
        if (fopen_s(&f, name, "wb") == 0 && f) {
            fwrite(buf.data(), 1, size, f);
            fclose(f);
        }
    };
    char name[96];
    sprintf_s(name, "remix_shaders\\vs_%p.bin", static_cast<void*>(vs));
    save(name, vs);
    sprintf_s(name, "remix_shaders\\ps_%p.bin", static_cast<void*>(ps));
    save(name, ps);
}
uint32_t g_probe_pass = 0xFF;          // the main pass sampled last
IDirect3DSurface9* g_probe_scene = nullptr;   // the target the trigger drew into (sampled at every later tag too)
int g_probe_tags = 0;
std::string g_probe_text;

float half_float(uint16_t h) {
    const uint32_t e = (h >> 10) & 0x1F, m = h & 0x3FF;
    const float v = e == 0 ? std::ldexp(float(m), -24) : e == 31 ? 65504.0f : std::ldexp(float(m | 0x400), int(e) - 25);
    return (h & 0x8000) ? -v : v;
}

bool read_pixel(D3DFORMAT f, const uint8_t* row, UINT x, float* c) {
    switch (f) {
    case D3DFMT_A16B16G16R16F: { const auto* p = reinterpret_cast<const uint16_t*>(row) + 4 * x;
                                for (int i = 0; i < 3; ++i) c[i] = half_float(p[i]);
                                return true; }
    case D3DFMT_A32B32G32R32F: { const auto* p = reinterpret_cast<const float*>(row) + 4 * x;
                                for (int i = 0; i < 3; ++i) c[i] = p[i];
                                return true; }
    case D3DFMT_A8R8G8B8: case D3DFMT_X8R8G8B8: { const uint8_t* p = row + 4 * x;
                                c[0] = p[2] / 255.0f; c[1] = p[1] / 255.0f; c[2] = p[0] / 255.0f;
                                return true; }
    case D3DFMT_A8B8G8R8: case D3DFMT_X8B8G8R8: { const uint8_t* p = row + 4 * x;
                                c[0] = p[0] / 255.0f; c[1] = p[1] / 255.0f; c[2] = p[2] / 255.0f;
                                return true; }
    case D3DFMT_A2R10G10B10: { const uint32_t v = reinterpret_cast<const uint32_t*>(row)[x];
                                c[0] = ((v >> 20) & 0x3FF) / 1023.0f; c[1] = ((v >> 10) & 0x3FF) / 1023.0f; c[2] = (v & 0x3FF) / 1023.0f;
                                return true; }
    default: return false;
    }
}

// column 0 of the copy is the target's own first column (the one the user saw the traced image in), the rest a point sample
void probe(IDirect3DDevice9* dev, const char* label, IDirect3DSurface9* surface = nullptr, const char* dump = nullptr) {
    char buf[320];
    IDirect3DSurface9* rt = surface;
    if (rt) rt->AddRef();
    else if (FAILED(dev->GetRenderTarget(0, &rt)) || !rt) {
        sprintf_s(buf, "%-40s no render target\n", label);
        g_probe_text += buf;
        return;
    }
    D3DSURFACE_DESC d{};
    rt->GetDesc(&d);
    if (d.Format == D3DFMT_UNKNOWN) d.Format = D3DFMT_A8R8G8B8;
    constexpr UINT kW = 64, kH = 36;
    IDirect3DSurface9 *full = nullptr, *shrunk = nullptr, *sys = nullptr, *from = rt;
    HRESULT hr = S_OK;
    const char* step = "resolve";
    if (d.MultiSampleType != D3DMULTISAMPLE_NONE) {
        hr = dev->CreateRenderTarget(d.Width, d.Height, d.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &full, nullptr);
        if (SUCCEEDED(hr)) hr = dev->StretchRect(rt, nullptr, full, nullptr, D3DTEXF_NONE);
        if (SUCCEEDED(hr)) from = full;
    }
    if (SUCCEEDED(hr)) { step = "shrunk target"; hr = dev->CreateRenderTarget(kW, kH, d.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &shrunk, nullptr); }
    if (SUCCEEDED(hr)) { step = "shrink"; hr = dev->StretchRect(from, nullptr, shrunk, nullptr, D3DTEXF_POINT); }
    if (SUCCEEDED(hr)) {
        step = "first column";
        const RECT src{ 0, 0, 1, LONG(d.Height) }, dst{ 0, 0, 1, LONG(kH) };
        hr = dev->StretchRect(from, &src, shrunk, &dst, D3DTEXF_POINT);
    }
    if (SUCCEEDED(hr)) { step = "system copy"; hr = dev->CreateOffscreenPlainSurface(kW, kH, d.Format, D3DPOOL_SYSTEMMEM, &sys, nullptr); }
    if (SUCCEEDED(hr)) { step = "read back"; hr = dev->GetRenderTargetData(shrunk, sys); }
    D3DLOCKED_RECT lr{};
    if (SUCCEEDED(hr)) { step = "lock"; hr = sys->LockRect(&lr, nullptr, D3DLOCK_READONLY); }
    sprintf_s(buf, "%-40s rt %04X %ux%u f%u ms%u", label, unsigned(reinterpret_cast<uintptr_t>(rt) & 0xFFFF), d.Width, d.Height,
              unsigned(d.Format), unsigned(d.MultiSampleType));
    g_probe_text += buf;
    if (SUCCEEDED(hr)) {
        double sum[3] = { 0, 0, 0 }, col = 0;
        float top = 0;
        int lit = 0, n = 0;
        bool known = true;
        std::vector<uint8_t> ppm;
        if (dump) ppm.resize(kW * kH * 3);
        for (UINT y = 0; y < kH && known; ++y) {
            const auto* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < kW; ++x) {
                float c[3];
                if (!read_pixel(d.Format, row, x, c)) { known = false; break; }
                if (dump)
                    for (int i = 0; i < 3; ++i)
                        ppm[(y * kW + x) * 3 + i] = static_cast<uint8_t>(std::pow(std::clamp(c[i], 0.0f, 1.0f), 1.0f / 2.2f) * 255.0f + 0.5f);
                const float l = (std::max)(c[0], (std::max)(c[1], c[2]));
                if (x == 0) { col += l; continue; }
                for (int i = 0; i < 3; ++i) sum[i] += c[i];
                top = (std::max)(top, l);
                if (l > 0.004f) ++lit;
                ++n;
            }
        }
        sys->UnlockRect();
        if (dump && known) {
            FILE* f = nullptr;
            if (fopen_s(&f, dump, "wb") == 0 && f) {
                fprintf(f, "P6\n%u %u\n255\n", kW, kH);
                fwrite(ppm.data(), 1, ppm.size(), f);
                fclose(f);
            }
        }
        if (known && n)
            sprintf_s(buf, " | mean %.3f %.3f %.3f, max %.3f, lit %d of %d, first column %.3f\n", sum[0] / n, sum[1] / n,
                      sum[2] / n, top, lit, n, col / kH);
        else
            sprintf_s(buf, " | format not read\n");
    } else {
        sprintf_s(buf, " | %s failed %08X\n", step, unsigned(hr));
    }
    g_probe_text += buf;
    if (sys) sys->Release();
    if (shrunk) shrunk->Release();
    if (full) full->Release();
    rt->Release();
}

// what Remix's makeDrawCallType / isRenderingUI read for the trigger, as the game left it
void probe_state(IDirect3DDevice9* dev) {
    static const struct { D3DRENDERSTATETYPE s; const char* n; } kStates[] = {
        { D3DRS_ZENABLE, "z" }, { D3DRS_ZWRITEENABLE, "zwrite" }, { D3DRS_ZFUNC, "zfunc" }, { D3DRS_STENCILENABLE, "stencil" },
        { D3DRS_STENCILFUNC, "sfunc" }, { D3DRS_STENCILPASS, "spass" }, { D3DRS_STENCILZFAIL, "szfail" },
        { D3DRS_TWOSIDEDSTENCILMODE, "stencil2" }, { D3DRS_ALPHATESTENABLE, "atest" }, { D3DRS_ALPHABLENDENABLE, "blend" },
        { D3DRS_COLORWRITEENABLE, "cwrite" }, { D3DRS_SCISSORTESTENABLE, "scissor" }, { D3DRS_SRGBWRITEENABLE, "srgbw" },
        { D3DRS_CULLMODE, "cull" }, { D3DRS_FOGENABLE, "fog" }, { D3DRS_CLIPPING, "clip" } };
    std::string s = "  trigger state:";
    char buf[128];
    for (const auto& k : kStates) {
        DWORD v = 0;
        dev->GetRenderState(k.s, &v);
        sprintf_s(buf, " %s %lX", k.n, v);
        s += buf;
    }
    D3DVIEWPORT9 vp{};
    dev->GetViewport(&vp);
    sprintf_s(buf, "\n  viewport %lu %lu %lux%lu z %.2f..%.2f", vp.X, vp.Y, vp.Width, vp.Height, vp.MinZ, vp.MaxZ);
    s += buf;
    IDirect3DVertexShader9* vs = nullptr;
    IDirect3DPixelShader9* ps = nullptr;
    dev->GetVertexShader(&vs);
    dev->GetPixelShader(&ps);
    sprintf_s(buf, " | vs %d ps %d", vs != nullptr, ps != nullptr);
    s += buf;
    if (vs) vs->Release();
    if (ps) ps->Release();
    DWORD tex = 0, rts = 0;
    for (DWORD i = 0; i < 16; ++i) {
        IDirect3DBaseTexture9* t = nullptr;
        if (SUCCEEDED(dev->GetTexture(i, &t)) && t) { tex |= 1u << i; t->Release(); }
    }
    for (DWORD i = 1; i < 4; ++i) {
        IDirect3DSurface9* r = nullptr;
        if (SUCCEEDED(dev->GetRenderTarget(i, &r)) && r) { rts |= 1u << i; r->Release(); }
    }
    IDirect3DSurface9* ds = nullptr;
    D3DSURFACE_DESC dd{};
    if (SUCCEEDED(dev->GetDepthStencilSurface(&ds)) && ds) { ds->GetDesc(&dd); ds->Release(); }
    sprintf_s(buf, " | textures %04lX | targets 1-3 %lX | depth %ux%u f%u ms%u", tex, rts, dd.Width, dd.Height,
              unsigned(dd.Format), unsigned(dd.MultiSampleType));
    s += buf;
    D3DMATRIX p{};
    dev->GetTransform(D3DTS_PROJECTION, &p);
    sprintf_s(buf, " | projection _34 %.3f _44 %.3f\n", p._34, p._44);
    s += buf;
    g_probe_text += s;
}

void probe_frame_start(IDirect3DDevice9* dev) {
    if (g_probe == 2) {   // the probed frame has been presented: what's on its targets now
        probe(dev, "next frame, bound target");
        IDirect3DSurface9* bb = nullptr;
        if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)) && bb) {
            probe(dev, "next frame, backbuffer", bb);
            bb->Release();
        }
        FILE* f = nullptr;
        if (fopen_s(&f, "remix_probe.txt", "a") == 0 && f) {
            fputs(g_probe_text.c_str(), f);
            fputs("\n", f);
            fclose(f);
        }
        g_probe_text.clear();
        if (g_probe_scene) { g_probe_scene->Release(); g_probe_scene = nullptr; }
        g_probe = 0;
    }
    if (settings.probe && !g_probe) {
        settings.probe = 0;
        g_probe = 2;
        g_probe_pass = 0xFF;
        g_probe_tags = 0;
        g_probe_layouts.clear();
        g_probe_paths.clear();
        g_probe_states.clear();
        g_probe_tex_count.clear();
        g_dumped_pairs.clear();
        build_tex_paths();
        char buf[256];
        sprintf_s(buf, "probe: hand-over at pass %d, skip filters %d, skip HUD %d, skip depth prepass %d, hand-over shader %d, "
                       "exposure %.2f, traced view %u\n", settings.inject_pass, settings.skip_filters, settings.skip_hud,
                  settings.skip_zpass, settings.linearize, settings.exposure, g_main_view);
        g_probe_text = buf;
    }
}

IDirect3DTexture9* hide_texture(IDirect3DDevice9* dev) {
    if (!g_hide && FAILED(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_hide, nullptr)))
        g_hide = nullptr;
    return g_hide;
}
// effects (sPrim) take their colour from these instead of the albedo map; stage 0 is often the depth buffer (soft
// particles, XfPrimDepthPointSampler), which Remix drew as solid white sprites
constexpr uint32_t kPrimBaseIds[] = { 0xBCB92646 /*XfPrimBaseLinearSampler*/, 0x1FE75530 /*XfPrimBasePointSampler*/,
                                      0xB5E3E840 /*XfPrimSampler*/ };
int      g_prim_param[3] = { -1, -1, -1 };
float    g_sun_dir[3] = { 0.0f, -1.0f, 0.0f };
// the water stand-ins, one per kind of water shader, each its own texture and so its own Remix material (DMC4Water mod):
// 0 XfWater (its pixel shader without gXfViewProjI), 1 XfWater (with it), 2 XfInteractiveWater, 3 XfFlowingWater,
// 4 XfFlatWater, 5 puddles (XfWater draws narrower than settings.puddle_size, chosen per draw: puddle::on_draw). Kind k
// is the water colour with k added to its blue byte (kind 0 = the colour itself, the mod's first material). Managed:
// they survive resets
constexpr int kWaterKinds = 6;
IDirect3DTexture9* g_standin[kWaterKinds] = {};
uint32_t g_standin_rgb[kWaterKinds] = {};          // the colour each was filled with
std::atomic<uint32_t> g_water{ 0 };
uint32_t g_water_last = 0;

int param_of(uint32_t id) {
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader) return -1;
    for (int i = 0; i < 512; ++i)
        if (*reinterpret_cast<const uint32_t*>(shader + kParamDescs + 20 * i + 8) == id) return i;
    return -1;
}

bool is_colour(uint32_t param, uint32_t albedo) {
    if (param == albedo) return true;
    for (int w : g_prim_param) if (w >= 0 && static_cast<uint32_t>(w) == param) return true;
    return false;
}

bool is_water(uint32_t param) {
    for (int w : g_water_param) if (w >= 0 && static_cast<uint32_t>(w) == param) return true;
    return false;
}

uint32_t rgb_of(const float* c) {
    auto b = [](float v) { return static_cast<uint32_t>((v < 0 ? 0 : v > 1 ? 1 : v) * 255.0f + 0.5f); };
    return 0xFF000000u | (b(c[0]) << 16) | (b(c[1]) << 8) | b(c[2]);
}

// a 4x4 solid texture of the water colour; refilled when the colour changes (Remix hashes the content, so one colour =
// one texture for a Remix material replacement)
IDirect3DTexture9* standin(IDirect3DDevice9* dev, int kind) {
    if (kind < 0 || kind >= kWaterKinds) kind = 0;
    uint32_t rgb = rgb_of(settings.water_color);
    rgb = (rgb & 0xFFFFFF00u) | std::min<uint32_t>((rgb & 0xFFu) + static_cast<uint32_t>(kind), 0xFFu);
    IDirect3DTexture9*& t = g_standin[kind];
    if (!t && FAILED(dev->CreateTexture(4, 4, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &t, nullptr))) t = nullptr;
    if (t && rgb != g_standin_rgb[kind]) {
        D3DLOCKED_RECT r;
        if (SUCCEEDED(t->LockRect(0, &r, nullptr, 0))) {
            for (int y = 0; y < 4; ++y) {
                auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(r.pBits) + y * r.Pitch);
                for (int x = 0; x < 4; ++x) row[x] = rgb;
            }
            t->UnlockRect(0);
            g_standin_rgb[kind] = rgb;
        }
    }
    return t;
}

// ---- water shaders ----
// a string in a shader's comment blocks (its constant table: DMC4's shaders keep their names)
bool code_has(const uint32_t* code, const char* text) {
    const size_t len = strlen(text);
    const uint32_t* p = code + 1;
    for (int guard = 0; guard < 65536 && *p != 0x0000FFFFu; ++guard) {
        const uint32_t t = *p;
        if ((t & 0xFFFF) == 0xFFFE) {
            const uint32_t words = (t >> 16) & 0x7FFF;
            const char* b = reinterpret_cast<const char*>(p + 1);
            for (size_t i = 0; i + len <= words * 4u; ++i)
                if (memcmp(b + i, text, len) == 0) return true;
            p += 1 + words;
            continue;
        }
        p += 1 + ((t >> 24) & 0xF);
    }
    return false;
}

std::vector<const uint32_t*> g_water_b_ps;   // XfWater pixel shader objects of kind 1 ({handle, bytecode}: gXfViewProjI)

// the kind of water a draw is (see the stand-ins), from the samplers it binds and, for XfWater, its pixel shader
int water_kind(IDirect3DDevice9* dev, const uint32_t* entries, uint32_t n) {
    bool xfwater = false, flowing = false, inter = false, flat = false;
    for (uint32_t i = 0; i < n; ++i) {
        const int p = static_cast<int>(entries[i] & 0xFFFF);
        if (p == g_water_param[1] || p == g_water_param[2]) xfwater = true;
        else if (p == g_water_param[0]) flowing = true;
        else if (p == g_water_param[3] || p == g_water_param[4] || p == g_water_param[5]) inter = true;
        else if (p == g_water_param[6]) flat = true;
    }
    if (xfwater) {
        IDirect3DPixelShader9* ps = nullptr;
        dev->GetPixelShader(&ps);
        if (ps) ps->Release();
        for (const uint32_t* s : g_water_b_ps)
            if (s[0] == reinterpret_cast<uint32_t>(ps)) return 1;
        return 0;
    }
    if (flat) return 4;
    if (flowing) return 3;
    if (inter) return 2;
    return 0;
}

// the interactive water surface for Remix (replaces XfInteractiveWater's tXfInteractiveWaterGeometory vertex shader: same
// inputs, outputs and constants, plus the height map's lift and a NORMAL0 output). The simulation stores height as
// 0.5 + 0.5 h / MaxAmplitude in .x, MaxAmplitude = mScale.y x 512 (uInteractiveWater::draw). Lifting by that (as
// steep as the game's normal map draws the slopes) gave enormous waves: the lift is the world matrix's up row (which
// carries mScale.y) x ripple_height cm x (2 x stored - 1). The normal map is n * 0.5 + 0.5 with x along the world matrix's X row, y along Z,
// z along up, mixed with up by the vertex colour's x, as the pixel shader does. The grid has 128 cells over the 512-texel
// maps, so both are averaged over each vertex's 4 x 4 texels (a 3 x 3 tent, taps 2 texels apart): drops smaller than a
// cell came out as square spikes. The normal is built on the world matrix's rows normalised (the game's pixel shader works
// on unit axes): BP stage 1's water (Infinite Azure, st705) is scaled 10 x 1 x 10, and the raw rows tilted every ripple
// normal ten times too far, so the reflections fell below the horizon in hard black cells (2026-11-26)
// With the bridge's own grid (graded::, c251.z = 1) the vertices are camera-relative lines in the surface's local units,
// each with its line spacing (x, z) in TEXCOORD0: every vertex moves with the camera (c251.xy, local x / z) snapped to
// its own spacing, so the ripples are sampled at the same points as the camera moves, and the UV and the edge fade
// (the game's grid colour, sub_AC4C20: 1 inside half the square, 0 past 0.875 of it) come from the position. The taps
// widen with the distance from the camera, not with the line's own spacing: a jump at a zone edge drew a straight seam
// across the water (the normals' blur changing at once)
const char* kIwVsSrc = R"(
float4 gXfViewProj[4] : register(c0);
float4 gXfWorld[3] : register(c4);
float4 kRipple : register(c250);
float4 kGraded : register(c251);   // w: the ripple normals' strength (settings.ripple_normal)
sampler2D sHeight : register(s0);
sampler2D sNormal : register(s1);
struct VsIn { float4 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; };
struct VsOut {
    float4 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; float3 wpos : TEXCOORD1; float3 up : TEXCOORD2;
    float3 ax : TEXCOORD3; float3 az : TEXCOORD4; float4 cpos : TEXCOORD5; float3 nrm : NORMAL0;
};
// the simulation's height at a height-map point, -1..1 x settings.ripple_height (kRipple.x), without the ripple curve:
// the shading's slope (the curve keeps the geometry low, but flattened the rain's ripples out of the shading too)
float height_at(float2 t) {
    return (tex2Dlod(sHeight, float4(t, 0.0, 0.0)).x * 2.0 - 1.0) * kRipple.x;
}
VsOut main(VsIn i) {
    VsOut o;
    float3 lp = i.pos.xyz;
    float4 col = i.col;
    float2 uv = i.uv;
    float tap = kRipple.z;
    if (kGraded.z > 0.5) {
        float2 st = i.uv;
        float lim = kGraded.z > 1.5 ? 1.0e7 : 1000.0;   // 2 = on to the horizon (settings.water_to_horizon)
        float2 q = clamp(i.pos.xz + floor(kGraded.xy / st + 0.5) * st, -lim, lim);
        lp = float3(q.x, 0.0, q.y);
        uv = saturate(q / 2000.0 + 0.5);
        float2 f = saturate(1.0 - (abs(q / 1000.0) - 0.5) * 2.6666667);
        col = (f.x * f.y).xxxx;
        float2 dc = abs(q - kGraded.xy);
        tap = max(1.953125, max(dc.x, dc.y) / 25.0) / 4000.0;   // about half the spacing there, smooth across zones
    }
    float4 p = float4(lp, 1.0);
    float3 wp = float3(dot(gXfWorld[0], p), dot(gXfWorld[1], p), dot(gXfWorld[2], p));
    float3 up = float3(gXfWorld[0].y, gXfWorld[1].y, gXfWorld[2].y);
    float3 ax = float3(gXfWorld[0].x, gXfWorld[1].x, gXfWorld[2].x);
    float3 az = float3(gXfWorld[0].z, gXfWorld[1].z, gXfWorld[2].z);
    float3 upn = normalize(up);
    float3 axn = normalize(ax);
    float3 azn = normalize(az);
    float3 n = upn;
    if (kRipple.y > 0.5) {
        // the heights through a 3 x 3 tent at least a texel wide (1 / 512 of the square: 39 cm on BP stage 1), and the
        // normals from the lifted surface's own slope: with a quarter-texel tent near the camera and the simulation's
        // normal map, WaterWakes' bumps (2-3 texels) came out as faceted triangles under Remix (user, 2026-11-26: "the
        // deformation is too abrupt and leaves triangular patches", Remix only; vanilla reads the maps per pixel). The
        // slope normals also drop the normal map's strong rain distortion: the water reads crystal clear, which the user
        // picked over the normal map sampled the same way ("now it looks more like what I meant with crystal clear")
        tap = max(tap, 1.0 / 512.0);
        float h = 0.0;
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x) {
                const float w = (2.0 - abs(x)) * (2.0 - abs(y));
                h += w * tex2Dlod(sHeight, float4(uv + float2(x, y) * tap, 0.0, 0.0)).x;
            }
        h = h * (2.0 / 16.0) - 1.0;
        wp += up * (sign(h) * pow(abs(h), kRipple.w) * kRipple.x * col.x);   // kRipple.w: settings.ripple_curve
        // slope (cm per cm) of the simulation's height, uncurved, along local x / z: central differences a tap apart
        // each way; uv runs over 2000 local units, |ax| / |az| world units each (the square's scale). The geometry keeps
        // the curve (low, no bulk); the shading gets the rain's ripples at the simulation's own scale, as vanilla's
        // normal map distorts its reflection (user, 2026-11-26: "need the same level of wavy-ness" as vanilla).
        // kGraded.w = settings.ripple_normal scales it (live)
        const float s = tap;
        const float gx = (height_at(uv + float2(s, 0.0)) - height_at(uv - float2(s, 0.0))) / (2.0 * s * 2000.0 * length(ax));
        const float gz = (height_at(uv + float2(0.0, s)) - height_at(uv - float2(0.0, s))) / (2.0 * s * 2000.0 * length(az));
        n = normalize(upn - col.x * kGraded.w * (gx * axn + gz * azn));
    }
    float4 w4 = float4(wp, 1.0);
    float4 c = float4(dot(w4, gXfViewProj[0]), dot(w4, gXfViewProj[1]), dot(w4, gXfViewProj[2]), dot(w4, gXfViewProj[3]));
    o.pos = c; o.cpos = c; o.col = col; o.uv = uv; o.wpos = wp; o.up = up; o.ax = ax; o.az = az; o.nrm = n;
    return o;
}
)";
std::vector<uint32_t> g_iw_vs_code;   // kIwVsSrc compiled (vs_3_0), once
bool g_iw_vs_failed = false;
uint32_t g_iw_replaced = 0;          // surfaces given it (needs the bridge on when the stage's shaders are created)

bool iw_vs_code() {
    if (!g_iw_vs_code.empty() || g_iw_vs_failed) return !g_iw_vs_code.empty();
    using D3DCompileFn = HRESULT(WINAPI*)(const void*, SIZE_T, const char*, const void*, void*, const char*, const char*, UINT,
                                         UINT, LinBlob**, LinBlob**);
    HMODULE dll = LoadLibraryA("d3dcompiler_47.dll");
    auto compile = dll ? reinterpret_cast<D3DCompileFn>(GetProcAddress(dll, "D3DCompile")) : nullptr;
    LinBlob *code = nullptr, *err = nullptr;
    if (compile && SUCCEEDED(compile(kIwVsSrc, strlen(kIwVsSrc), "remix_iwater", nullptr, nullptr, "main", "vs_3_0", 0, 0,
                                     &code, &err)) && code) {
        const auto* w = static_cast<const uint32_t*>(code->GetBufferPointer());
        g_iw_vs_code.assign(w, w + code->GetBufferSize() / 4);
    } else {
        g_iw_vs_failed = true;
    }
    if (code) code->Release();
    if (err) err->Release();
    return !g_iw_vs_code.empty();
}

// the uInteractiveWater units (their maps: +0xC4 the height map, +0xD8 the normal map the surface samples; cTrans::Texture
// objects, the D3D9 texture at +0x10 / +0x14), found once a frame on the first surface draw
uintptr_t g_dti_iwater = 0;
uintptr_t g_iw_units[8];
int g_iw_nunits = 0;
uint32_t g_iw_units_frame = 0xFFFFFFFFu;
bool g_vtf_bound = false;            // the surface's maps are on the vertex samplers (unbound at the next draw)
volatile uint8_t g_iw_graded_next = 0;   // the next draws are the interactive surface: drawn with graded:: (water_draw)

// the interactive surface's own grid (kIwVsSrc's graded mode). The game's is 129 x 129 vertices over its square (1.56 m
// apart on BP stage 1, 0.78 m at water_detail 1), while its pixel shader reads the 512-texel ripple maps at every pixel:
// under Remix the ripples were only as fine as the vertices. This grid's lines are full length (no cracks), camera-relative
// in the surface's local units (the square is -1000..1000): every kStep (half a texel) within kFineLines x kStep of the
// camera, the spacing doubling after every kZoneLines lines further out, to 2048 steps (the whole square from any camera
// position; lines past its edge are clamped onto it by the shader), then four far lines out to 524288 steps (~100 km on BP
// stage 1) for settings.water_to_horizon. Built once; the shader moves it with the camera.
namespace graded {
constexpr float kStep = 2000.0f / 1024.0f;
constexpr int kFineLines = 40, kZoneLines = 20, kExtent = 2048;
IDirect3DVertexBuffer9* g_vb = nullptr;
IDirect3DIndexBuffer9* g_ib = nullptr;
uint32_t g_nv = 0, g_nt = 0;
bool g_failed = false;
float g_cam[2] = { 0, 0 };   // the camera in the surface's local x / z (iw_bind)

// the line positions of one axis and their spacings, in local units
void lines(std::vector<float>& pos, std::vector<float>& step) {
    std::vector<std::pair<int, int>> half;   // (position, spacing) in steps, 0 .. kExtent
    int x = 0, s = 1, in_zone = 0;
    while (true) {
        half.emplace_back(x, s);
        if (x >= kExtent) break;
        ++in_zone;
        if (in_zone >= (s == 1 ? kFineLines : kZoneLines) && x % (2 * s) == 0 && s < 64) { s *= 2; in_zone = 0; }
        x += s;
    }
    // past the square's reach: a few far lines out to the horizon (used with settings.water_to_horizon; otherwise
    // clamped onto the square's edge with the rest), each its own spacing so it never moves with the camera
    for (const int out : { 8192, 32768, 131072, 524288 }) half.emplace_back(out, out);
    pos.clear();
    step.clear();
    for (size_t i = half.size(); i-- > 1;) { pos.push_back(-half[i].first * kStep); step.push_back(half[i].second * kStep); }
    for (const auto& [p, sp] : half) { pos.push_back(p * kStep); step.push_back(sp * kStep); }
}

bool build(IDirect3DDevice9* dev) {
    std::vector<float> pos, step;
    lines(pos, step);
    const uint32_t n = static_cast<uint32_t>(pos.size());
    g_nv = n * n;
    g_nt = (n - 1) * (n - 1) * 2;
    if (FAILED(dev->CreateVertexBuffer(g_nv * 24, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &g_vb, nullptr)) ||
        FAILED(dev->CreateIndexBuffer(g_nt * 3 * 4, D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_MANAGED, &g_ib, nullptr)))
        return false;
    void* p = nullptr;
    if (FAILED(g_vb->Lock(0, 0, &p, 0)) || !p) return false;
    auto* v = static_cast<uint8_t*>(p);
    for (uint32_t j = 0; j < n; ++j)   // the game's layout: POSITION float3, COLOR, TEXCOORD0 float2 (here the spacings)
        for (uint32_t i = 0; i < n; ++i, v += 24) {
            const float xyz[3] = { pos[i], 0.0f, pos[j] }, st[2] = { step[i], step[j] };
            const uint32_t white = 0xFFFFFFFFu;
            std::memcpy(v, xyz, 12);
            std::memcpy(v + 12, &white, 4);
            std::memcpy(v + 16, st, 8);
        }
    g_vb->Unlock();
    if (FAILED(g_ib->Lock(0, 0, &p, 0)) || !p) return false;
    auto* t = static_cast<uint32_t*>(p);
    for (uint32_t j = 0; j + 1 < n; ++j)
        for (uint32_t i = 0; i + 1 < n; ++i) {   // the game's strip winding: (j, i), (j + 1, i), (j, i + 1), ...
            const uint32_t a = j * n + i, b = (j + 1) * n + i, c = a + 1, d = b + 1;
            *t++ = a; *t++ = b; *t++ = c;
            *t++ = c; *t++ = b; *t++ = d;
        }
    g_ib->Unlock();
    return true;
}

// the game's declaration is the layout build writes
bool decl_matches(IDirect3DVertexDeclaration9* decl, UINT stride) {
    D3DVERTEXELEMENT9 el[MAXD3DDECLLENGTH + 1];
    UINT nel = 0;
    if (stride != 24 || FAILED(decl->GetDeclaration(nullptr, &nel)) || nel > MAXD3DDECLLENGTH + 1 ||
        FAILED(decl->GetDeclaration(el, &nel)))
        return false;
    bool pos = false, col = false, uv = false;
    for (UINT i = 0; i < nel && el[i].Stream != 0xFF; ++i) {
        const D3DVERTEXELEMENT9& e = el[i];
        if (e.Stream != 0) return false;
        if (e.Usage == D3DDECLUSAGE_POSITION && e.UsageIndex == 0) pos = e.Offset == 0 && e.Type == D3DDECLTYPE_FLOAT3;
        else if (e.Usage == D3DDECLUSAGE_COLOR && e.UsageIndex == 0) col = e.Offset == 12 && e.Type == D3DDECLTYPE_D3DCOLOR;
        else if (e.Usage == D3DDECLUSAGE_TEXCOORD && e.UsageIndex == 0) uv = e.Offset == 16 && e.Type == D3DDECLTYPE_FLOAT2;
    }
    return pos && col && uv;
}

// draws it in place of the game's surface (water_draw); false = the game's draw goes ahead
bool draw(IDirect3DDevice9* dev) {
    if (g_failed) return false;
    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DIndexBuffer9* ib = nullptr;
    IDirect3DVertexDeclaration9* decl = nullptr;
    UINT off = 0, stride = 0;
    bool done = false;
    if (SUCCEEDED(dev->GetStreamSource(0, &vb, &off, &stride)) && vb && SUCCEEDED(dev->GetIndices(&ib)) && ib &&
        SUCCEEDED(dev->GetVertexDeclaration(&decl)) && decl && decl_matches(decl, stride)) {
        if (!g_vb) {
            g_failed = !build(dev);
            if (g_failed) {
                if (g_vb) { g_vb->Release(); g_vb = nullptr; }
                if (g_ib) { g_ib->Release(); g_ib = nullptr; }
            }
        }
        if (g_vb && g_ib) {
            dev->SetStreamSource(0, g_vb, 0, 24);
            dev->SetIndices(g_ib);
            dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, g_nv, 0, g_nt);
            dev->SetStreamSource(0, vb, off, stride);
            dev->SetIndices(ib);
            done = true;
        }
    }
    if (vb) vb->Release();
    if (ib) ib->Release();
    if (decl) decl->Release();
    return done;
}
} // namespace graded

int find_iw_units_raw(uintptr_t* out, int max) {   // render thread, units run on the game thread: SEH-guarded
    __try {
        const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
        if (!su || !g_dti_iwater) return 0;
        int n = 0;
        for (int line = 0; line < 32; ++line) {
            const auto* ml = reinterpret_cast<const MoveLine*>(su + 0x20 + line * sizeof(MoveLine));
            int guard = 0;
            for (uintptr_t u = reinterpret_cast<uintptr_t>(ml->mTop); u && guard < 20000; ++guard) {
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uintptr_t next = reinterpret_cast<uintptr_t>(cu->mp_next_unit);
                if (n < max && debug_server::mem::dti_of(u) == g_dti_iwater) out[n++] = u;
                u = next;
            }
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

IDirect3DBaseTexture9* trans_texture(uintptr_t obj) {   // a cTrans::Texture's current D3D9 texture
    if (!obj) return nullptr;
    const uint32_t which = *reinterpret_cast<const uint8_t*>(obj + 0x22) & 1 & *reinterpret_cast<const uint32_t*>(kCameraBit);
    return *reinterpret_cast<IDirect3DBaseTexture9* const*>(obj + 0x10 + 4 * which);
}

// the interactive surface's UVs run 0..1 over its whole square (2000 cm x mScale.x: 200 m on BP stage 1), so the stand-in's
// wave normal map (the mod's material, Remix's layered water normals) is tiled to this many cm, from its scale: at the
// sea's tiling it was one 25 m tile, and the calm water between the simulation's ripple rings drew as flat mirrors
constexpr float kIwWaveTileCm = 250.0f;

extern bool g_have_main;
extern D3DMATRIX g_main_v;
// the main camera (last frame's) in the surface's local units: mWmat (+0x50, rows with the scale) inverted
void iw_camera_local(uintptr_t unit) {
    float cam[3];
    if (!g_have_main) return;
    view_position(g_main_v, cam);
    const float* m = reinterpret_cast<const float*>(unit + 0x50);
    const float a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!std::isfinite(det) || std::fabs(det) < 1e-12f) return;
    const float w[3] = { cam[0] - m[12], cam[1] - m[13], cam[2] - m[14] };
    // local = w x M3^-1 (row vectors): the x and z columns of the inverse
    const float lx = (w[0] * (e * i - f * h) + w[1] * (f * g - d * i) + w[2] * (d * h - e * g)) / det;
    const float lz = (w[0] * (b * f - c * e) + w[1] * (c * d - a * f) + w[2] * (a * e - b * d)) / det;
    if (std::isfinite(lx) && std::isfinite(lz)) { graded::g_cam[0] = lx; graded::g_cam[1] = lz; }
}

// settings.iw_measure: the height map (h, the surface's current one) read back whole, the lift it gives in cm (kIwVsSrc:
// (2 x stored - 1) x ripple_height x the world matrix's up row) logged with where its highest point is (map -1..1)
void iw_measure_read(IDirect3DDevice9* dev, IDirect3DBaseTexture9* h, float lift_cm) {
    if (settings.iw_measure <= 0 || !h || h->GetType() != D3DRTYPE_TEXTURE) return;
    --settings.iw_measure;
    IDirect3DSurface9 *s = nullptr, *sys = nullptr;
    if (FAILED(static_cast<IDirect3DTexture9*>(h)->GetSurfaceLevel(0, &s)) || !s) return;
    D3DSURFACE_DESC d{};
    s->GetDesc(&d);
    char line[256] = "";
    D3DLOCKED_RECT lr{};
    if (SUCCEEDED(dev->CreateOffscreenPlainSurface(d.Width, d.Height, d.Format, D3DPOOL_SYSTEMMEM, &sys, nullptr)) &&
        SUCCEEDED(dev->GetRenderTargetData(s, sys)) && SUCCEEDED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) {
        float lo = 1e9f, hi = -1e9f;
        UINT hx = 0, hy = 0, lx = 0, ly = 0;
        double sum = 0;
        bool known = true;
        for (UINT y = 0; y < d.Height && known; ++y) {
            const auto* row = static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch;
            for (UINT x = 0; x < d.Width; ++x) {
                float v;
                switch (d.Format) {
                case D3DFMT_R32F: case D3DFMT_G32R32F: v = reinterpret_cast<const float*>(row)[x * (d.Format == D3DFMT_R32F ? 1 : 2)]; break;
                case D3DFMT_R16F: case D3DFMT_G16R16F: v = half_float(reinterpret_cast<const uint16_t*>(row)[x * (d.Format == D3DFMT_R16F ? 1 : 2)]); break;
                default: { float c[3]; if (!read_pixel(d.Format, row, x, c)) { known = false; v = 0; } else v = c[0]; }
                }
                if (!known) break;
                sum += v;
                if (v > hi) { hi = v; hx = x; hy = y; }
                if (v < lo) { lo = v; lx = x; ly = y; }
            }
        }
        if (known && settings.iw_dump > 0 && settings.iw_measure % 5 == 0 && d.Format == D3DFMT_R32F) {
            --settings.iw_dump;
            char name[64];
            sprintf_s(name, "remix_iw_%u.raw", g_frames.load());
            FILE* rf = nullptr;
            if (fopen_s(&rf, name, "wb") == 0 && rf) {
                for (UINT y = 0; y < d.Height; ++y) fwrite(static_cast<const uint8_t*>(lr.pBits) + y * lr.Pitch, 4, d.Width, rf);
                fclose(rf);
            }
        }
        sys->UnlockRect();
        auto map = [&](UINT t, UINT n) { return 2.0f * (t + 0.5f) / static_cast<float>(n) - 1.0f; };
        if (known)
            sprintf_s(line, "%u %ux%u f%u: lift %.2f .. %.2f cm, highest at map %.3f %.3f, lowest at %.3f %.3f, mean %.3f cm\n",
                      g_frames.load(), d.Width, d.Height, unsigned(d.Format), (2 * lo - 1) * lift_cm, (2 * hi - 1) * lift_cm,
                      map(hx, d.Width), map(hy, d.Height), map(lx, d.Width), map(ly, d.Height),
                      (2 * sum / (double(d.Width) * d.Height) - 1) * lift_cm);
        else
            sprintf_s(line, "%u %ux%u f%u: format not read\n", g_frames.load(), d.Width, d.Height, unsigned(d.Format));
    } else {
        sprintf_s(line, "%u read back failed\n", g_frames.load());
    }
    if (sys) sys->Release();
    s->Release();
    FILE* f = nullptr;
    if (fopen_s(&f, "remix_iw.txt", "a") == 0 && f) {
        fputs(line, f);
        fclose(f);
    }
}

// the interactive water surface's draw: its height map and normal map on vertex samplers 0 / 1 and the lift in c250 (see
// kIwVsSrc); any other draw: unbound again (the simulation renders into these maps next frame)
void iw_bind(IDirect3DDevice9* dev, const uint32_t* first, const uint32_t* entries, uint32_t n) {
    float k[4] = { 0, 0, 0, 0 };
    IDirect3DBaseTexture9 *h = nullptr, *nm = nullptr;
    bool horizon = false;
    if (first) {
        uintptr_t nobj = 0;
        for (uint32_t i = 0; i < n; ++i)
            if (static_cast<int>(entries[i] & 0xFFFF) == g_water_param[3]) nobj = first[i];
        if (!g_dti_iwater) g_dti_iwater = debug_server::mem::find_dti("uInteractiveWater");
        if (g_iw_units_frame != g_frames) {
            const int found = find_iw_units_raw(g_iw_units, 8);
            if (found >= 0) g_iw_nunits = found;
            g_iw_units_frame = g_frames;
        }
        for (int i = 0; i < g_iw_nunits && nobj; ++i)
            if (*reinterpret_cast<const uintptr_t*>(g_iw_units[i] + 0xD8) == nobj) {
                h = trans_texture(*reinterpret_cast<const uintptr_t*>(g_iw_units[i] + 0xC4));
                nm = trans_texture(nobj);
                const float sx = std::fabs(*reinterpret_cast<const float*>(g_iw_units[i] + 0x40));   // mScale.x
                horizon = settings.water_to_horizon && std::isfinite(sx) && sx >= 5.0f;   // a square of 100 m or more
                if (g_tt_on && std::isfinite(sx) && sx > 0.0f)
                    water_transform(dev, true, std::min(512.0f, 2000.0f * sx / kIwWaveTileCm));
                iw_camera_local(g_iw_units[i]);
                const float* um = reinterpret_cast<const float*>(g_iw_units[i] + 0x50);
                iw_measure_read(dev, h, settings.ripple_height * std::sqrt(um[4] * um[4] + um[5] * um[5] + um[6] * um[6]));
            }
        if (h && nm) {
            k[0] = settings.ripple_height;
            k[1] = 1.0f;
            k[3] = std::clamp(settings.ripple_curve, 0.25f, 8.0f);
            // taps half a grid cell apart: 4 texels a cell, halved per subdivision level
            k[2] = (2.0f / 512.0f) / static_cast<float>(1 << std::min(std::max(settings.water_detail, 0), 3));
            if (k[2] < 1.0f / 512.0f) k[2] = 1.0f / 512.0f;
        }
    }
    if (!h && !g_vtf_bound) return;
    dev->SetTexture(D3DVERTEXTEXTURESAMPLER0, h);
    dev->SetTexture(D3DVERTEXTEXTURESAMPLER1, nm);
    if (h) {
        for (DWORD s : { DWORD(D3DVERTEXTEXTURESAMPLER0), DWORD(D3DVERTEXTEXTURESAMPLER1) }) {
            dev->SetSamplerState(s, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            dev->SetSamplerState(s, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            dev->SetSamplerState(s, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
            dev->SetSamplerState(s, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            dev->SetSamplerState(s, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        }
    }
    dev->SetVertexShaderConstantF(250, k, 1);
    const float kg[4] = { graded::g_cam[0], graded::g_cam[1],
                          (h && g_iw_graded_next && !graded::g_failed) ? (horizon ? 2.0f : 1.0f) : 0.0f,
                          settings.ripple_normal };
    dev->SetVertexShaderConstantF(251, kg, 1);
    g_vtf_bound = h != nullptr;
}

// ---- water mesh subdivision (settings.water_detail) ----
// The waves of XfWater and the interactive surface are vertex positions, so Remix sees them only as finely as the mesh
// is cut. Their draws are made by the bridge instead (the indexed draw hook, below) from a cached copy of the mesh with
// every triangle split in four per level, each new vertex the mean of its edge's two (every declared element in its own
// format), through the game's shaders and state. Copies are keyed by the buffers and the range and checked against the
// first bytes of the game's data every few seconds (buffers are reused after a stage change).
namespace subdiv {
float half_to_float(uint16_t h) {
    const uint32_t s = (h >> 15) & 1, e = (h >> 10) & 0x1F, m = h & 0x3FF;
    float v;
    if (e == 0) v = std::ldexp(static_cast<float>(m), -24);
    else if (e == 31) v = m ? NAN : INFINITY;
    else v = std::ldexp(static_cast<float>(m | 0x400), static_cast<int>(e) - 25);
    return s ? -v : v;
}
uint16_t float_to_half(float f) {
    uint32_t x;
    std::memcpy(&x, &f, 4);
    const uint32_t s = (x >> 16) & 0x8000;
    int e = static_cast<int>((x >> 23) & 0xFF) - 127 + 15;
    uint32_t m = x & 0x7FFFFF;
    if (e >= 31) return static_cast<uint16_t>(s | 0x7C00);
    if (e <= 0) {
        if (e < -10) return static_cast<uint16_t>(s);
        m |= 0x800000;
        return static_cast<uint16_t>(s | ((m >> (1 - e + 13)) + ((m >> (1 - e + 12)) & 1)));
    }
    uint32_t r = s | (static_cast<uint32_t>(e) << 10) | (m >> 13);
    if (m & 0x1000) ++r;   // round
    return static_cast<uint16_t>(r);
}
// the midpoint of one declared element of two vertices, in its own format
void mid_element(BYTE type, const uint8_t* a, const uint8_t* b, uint8_t* o) {
    switch (type) {
    case D3DDECLTYPE_FLOAT1: case D3DDECLTYPE_FLOAT2: case D3DDECLTYPE_FLOAT3: case D3DDECLTYPE_FLOAT4: {
        const int n = type - D3DDECLTYPE_FLOAT1 + 1;
        for (int i = 0; i < n; ++i) {
            float x, y;
            std::memcpy(&x, a + 4 * i, 4);
            std::memcpy(&y, b + 4 * i, 4);
            const float r = 0.5f * (x + y);
            std::memcpy(o + 4 * i, &r, 4);
        }
        break;
    }
    case D3DDECLTYPE_D3DCOLOR: case D3DDECLTYPE_UBYTE4: case D3DDECLTYPE_UBYTE4N:
        for (int i = 0; i < 4; ++i) o[i] = static_cast<uint8_t>((a[i] + b[i] + 1) / 2);
        break;
    case D3DDECLTYPE_SHORT2: case D3DDECLTYPE_SHORT4: case D3DDECLTYPE_SHORT2N: case D3DDECLTYPE_SHORT4N: {
        const int n = (type == D3DDECLTYPE_SHORT2 || type == D3DDECLTYPE_SHORT2N) ? 2 : 4;
        for (int i = 0; i < n; ++i) {
            int16_t x, y;
            std::memcpy(&x, a + 2 * i, 2);
            std::memcpy(&y, b + 2 * i, 2);
            const int16_t r = static_cast<int16_t>((static_cast<int>(x) + y) / 2);
            std::memcpy(o + 2 * i, &r, 2);
        }
        break;
    }
    case D3DDECLTYPE_USHORT2N: case D3DDECLTYPE_USHORT4N: {
        const int n = type == D3DDECLTYPE_USHORT2N ? 2 : 4;
        for (int i = 0; i < n; ++i) {
            uint16_t x, y;
            std::memcpy(&x, a + 2 * i, 2);
            std::memcpy(&y, b + 2 * i, 2);
            const uint16_t r = static_cast<uint16_t>((static_cast<uint32_t>(x) + y + 1) / 2);
            std::memcpy(o + 2 * i, &r, 2);
        }
        break;
    }
    case D3DDECLTYPE_FLOAT16_2: case D3DDECLTYPE_FLOAT16_4: {
        const int n = type == D3DDECLTYPE_FLOAT16_2 ? 2 : 4;
        for (int i = 0; i < n; ++i) {
            uint16_t x, y;
            std::memcpy(&x, a + 2 * i, 2);
            std::memcpy(&y, b + 2 * i, 2);
            const uint16_t r = float_to_half(0.5f * (half_to_float(x) + half_to_float(y)));
            std::memcpy(o + 2 * i, &r, 2);
        }
        break;
    }
    case D3DDECLTYPE_UDEC3: case D3DDECLTYPE_DEC3N: {
        uint32_t x, y, r = 0;
        std::memcpy(&x, a, 4);
        std::memcpy(&y, b, 4);
        for (int i = 0; i < 3; ++i) {
            int p = static_cast<int>((x >> (10 * i)) & 0x3FF), q = static_cast<int>((y >> (10 * i)) & 0x3FF);
            if (type == D3DDECLTYPE_DEC3N) { if (p & 0x200) p -= 0x400; if (q & 0x200) q -= 0x400; }
            r |= (static_cast<uint32_t>((p + q) / 2) & 0x3FF) << (10 * i);
        }
        r |= x & 0xC0000000u;
        std::memcpy(o, &r, 4);
        break;
    }
    default:
        break;   // left as the first vertex's
    }
}
uint32_t decl_size(BYTE type) {
    switch (type) {
    case D3DDECLTYPE_FLOAT1: case D3DDECLTYPE_D3DCOLOR: case D3DDECLTYPE_UBYTE4: case D3DDECLTYPE_SHORT2:
    case D3DDECLTYPE_UBYTE4N: case D3DDECLTYPE_SHORT2N: case D3DDECLTYPE_USHORT2N: case D3DDECLTYPE_UDEC3:
    case D3DDECLTYPE_DEC3N: case D3DDECLTYPE_FLOAT16_2: return 4;
    case D3DDECLTYPE_FLOAT2: case D3DDECLTYPE_SHORT4: case D3DDECLTYPE_SHORT4N: case D3DDECLTYPE_USHORT4N:
    case D3DDECLTYPE_FLOAT16_4: return 8;
    case D3DDECLTYPE_FLOAT3: return 12;
    case D3DDECLTYPE_FLOAT4: return 16;
    default: return 0;
    }
}
uint32_t fnv(const uint8_t* p, size_t n, uint32_t h = 2166136261u) {
    for (size_t i = 0; i < n; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}

struct Key {
    uintptr_t vb, ib, decl;
    uint32_t off, stride, base, start, prims, type, minidx, nverts, level;
    bool operator<(const Key& k) const { return std::memcmp(this, &k, sizeof(Key)) < 0; }
};
struct Entry {
    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DIndexBuffer9* ib = nullptr;
    uint32_t nv = 0, nt = 0, check = 0, last = 0, verified = 0;
    bool failed = false;
};
std::map<Key, Entry> g_cache;
uint32_t g_drawn = 0, g_drawn_last = 0;

void release(Entry& e) {
    if (e.vb) e.vb->Release();
    if (e.ib) e.ib->Release();
    e.vb = nullptr;
    e.ib = nullptr;
}
void clear() {
    for (auto& [k, e] : g_cache) release(e);
    g_cache.clear();
}

// reads the draw's vertices and triangles (a list or a strip, degenerate triangles dropped), rebased to its range
// a copy out of a locked buffer that can't take the game down: the Remix bridge client's lock pointers aren't always
// backed for the whole range (BP's interactive water floor, 2026-11-26: a 191 KB vertex copy ran off the end of mapped
// memory, access violation in memcpy)
bool copy_guarded(void* dst, const void* src, size_t n) {
    __try {
        std::memcpy(dst, src, n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool read(const Key& k, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib, std::vector<uint8_t>& verts,
          std::vector<uint32_t>& tris, uint32_t* check) {
    D3DINDEXBUFFER_DESC id{};
    if (FAILED(ib->GetDesc(&id))) return false;
    const uint32_t isz = id.Format == D3DFMT_INDEX32 ? 4 : 2;
    const uint32_t nidx = k.type == D3DPT_TRIANGLELIST ? k.prims * 3 : k.prims + 2;
    if (nidx == 0 || (static_cast<uint64_t>(k.start) + nidx) * isz > id.Size) return false;   // the draw's range in the buffer
    void* p = nullptr;
    if (FAILED(ib->Lock(k.start * isz, nidx * isz, &p, D3DLOCK_READONLY)) || !p) return false;
    std::vector<uint8_t> raw(static_cast<size_t>(nidx) * isz);
    const bool got = copy_guarded(raw.data(), p, raw.size());
    ib->Unlock();
    if (!got) return false;
    std::vector<uint32_t> idx(nidx);
    for (uint32_t i = 0; i < nidx; ++i)
        idx[i] = isz == 4 ? reinterpret_cast<const uint32_t*>(raw.data())[i] : reinterpret_cast<const uint16_t*>(raw.data())[i];
    *check = fnv(raw.data(), std::min<size_t>(raw.size(), 256));
    D3DVERTEXBUFFER_DESC vd{};
    if (FAILED(vb->GetDesc(&vd))) return false;
    const uint64_t first = k.off + (static_cast<uint64_t>(k.base) + k.minidx) * k.stride;
    const uint64_t bytes = static_cast<uint64_t>(k.nverts) * k.stride;
    if (bytes == 0 || first + bytes > vd.Size) return false;
    if (FAILED(vb->Lock(static_cast<UINT>(first), static_cast<UINT>(bytes), &p, D3DLOCK_READONLY)) || !p) return false;
    verts.resize(static_cast<size_t>(bytes));
    const bool vgot = copy_guarded(verts.data(), p, verts.size());
    vb->Unlock();
    if (!vgot) return false;
    *check = fnv(verts.data(), std::min<size_t>(verts.size(), 256), *check);
    auto add = [&](uint32_t a, uint32_t b, uint32_t c) {
        if (a == b || b == c || a == c) return;
        a -= k.minidx; b -= k.minidx; c -= k.minidx;
        if (a >= k.nverts || b >= k.nverts || c >= k.nverts) return;
        tris.push_back(a); tris.push_back(b); tris.push_back(c);
    };
    if (k.type == D3DPT_TRIANGLELIST)
        for (uint32_t t = 0; t < k.prims; ++t) add(idx[3 * t], idx[3 * t + 1], idx[3 * t + 2]);
    else
        for (uint32_t t = 0; t < k.prims; ++t)
            if (t & 1) add(idx[t + 1], idx[t], idx[t + 2]); else add(idx[t], idx[t + 1], idx[t + 2]);
    return !tris.empty();
}

bool build(IDirect3DDevice9* dev, const Key& k, IDirect3DVertexBuffer9* vb, IDirect3DIndexBuffer9* ib,
           IDirect3DVertexDeclaration9* decl, Entry& e) {
    D3DVERTEXELEMENT9 el[MAXD3DDECLLENGTH + 1];
    UINT nel = 0;
    if (FAILED(decl->GetDeclaration(nullptr, &nel)) || nel > MAXD3DDECLLENGTH + 1 || FAILED(decl->GetDeclaration(el, &nel)))
        return false;
    for (UINT i = 0; i < nel && el[i].Stream != 0xFF; ++i)
        if (el[i].Stream != 0) return false;   // one stream only
    std::vector<uint8_t> verts;
    std::vector<uint32_t> tris;
    if (!read(k, vb, ib, verts, tris, &e.check)) return false;
    const uint32_t s = k.stride;
    for (uint32_t level = 0; level < k.level; ++level) {
        if (verts.size() / s + tris.size() > 4000000) break;   // a cap: 4 M vertices
        std::map<uint64_t, uint32_t> mids;
        auto mid = [&](uint32_t a, uint32_t b) {
            const uint64_t key = a < b ? (uint64_t(a) << 32) | b : (uint64_t(b) << 32) | a;
            const auto it = mids.find(key);
            if (it != mids.end()) return it->second;
            const uint32_t n = static_cast<uint32_t>(verts.size() / s);
            verts.resize(verts.size() + s);
            uint8_t* o = &verts[n * s];
            const uint8_t* pa = &verts[a * s];
            const uint8_t* pb = &verts[b * s];
            std::memcpy(o, pa, s);
            for (UINT i = 0; i < nel && el[i].Stream != 0xFF; ++i)
                if (el[i].Offset + decl_size(el[i].Type) <= s) mid_element(el[i].Type, pa + el[i].Offset, pb + el[i].Offset, o + el[i].Offset);
            mids.emplace(key, n);
            return n;
        };
        std::vector<uint32_t> out;
        out.reserve(tris.size() * 4);
        for (size_t t = 0; t < tris.size(); t += 3) {
            const uint32_t a = tris[t], b = tris[t + 1], c = tris[t + 2];
            const uint32_t ab = mid(a, b), bc = mid(b, c), ca = mid(c, a);
            const uint32_t q[12] = { a, ab, ca, ab, b, bc, ca, bc, c, ab, bc, ca };
            out.insert(out.end(), q, q + 12);
        }
        tris.swap(out);
    }
    e.nv = static_cast<uint32_t>(verts.size() / s);
    e.nt = static_cast<uint32_t>(tris.size() / 3);
    const bool i32 = e.nv > 0xFFFF;
    if (FAILED(dev->CreateVertexBuffer(static_cast<UINT>(verts.size()), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &e.vb, nullptr)) ||
        FAILED(dev->CreateIndexBuffer(static_cast<UINT>(tris.size() * (i32 ? 4 : 2)), D3DUSAGE_WRITEONLY,
                                      i32 ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_MANAGED, &e.ib, nullptr))) {
        release(e);
        return false;
    }
    void* p = nullptr;
    if (FAILED(e.vb->Lock(0, 0, &p, 0)) || !p) { release(e); return false; }
    std::memcpy(p, verts.data(), verts.size());
    e.vb->Unlock();
    if (FAILED(e.ib->Lock(0, 0, &p, 0)) || !p) { release(e); return false; }
    if (i32) std::memcpy(p, tris.data(), tris.size() * 4);
    else for (size_t i = 0; i < tris.size(); ++i) static_cast<uint16_t*>(p)[i] = static_cast<uint16_t>(tris[i]);
    e.ib->Unlock();
    return true;
}
} // namespace subdiv

// ---- lights through the Remix API (settings.api_lights, settings.light_textures) ----
// The bridge's 32-bit client takes the API's light calls (CreateLight / DrawLightInstance / DestroyLight) when
// exposeRemixApi = True in .trex\bridge.conf; it accepts API version 0.6.x (remixapi_InitializeLibrary). A light is
// drawn per frame; the API can't change a light, so one whose position or radiance changed is made again.
namespace api {
remixapi_Interface g{};
int g_state = 0;                  // 0 not tried, 1 ready, -1 not available
int g_error = 0;                  // remixapi_ErrorCode of the failed init
uint32_t g_drawn_last = 0, g_made = 0;
struct Live { remixapi_LightHandle h = nullptr; float pos[3]{}, rad[3]{}, radius = 0; uint32_t seen = 0; };
std::unordered_map<uint64_t, Live> g_live;
uint32_t g_drawn = 0;

void note(const char* what, int code) {   // a line in remix_probe.txt (the API's state is otherwise only in the GUI)
    FILE* f = nullptr;
    if (fopen_s(&f, "remix_probe.txt", "a") == 0 && f) {
        fprintf(f, "remix api: %s (%d), frame %u\n", what, code, g_frames.load());
        fclose(f);
    }
}

uint32_t g_retry_at = 0;          // NOT_INITIALIZED (the bridge server not up yet, as on the first frames): try again then

bool ready() {
    if (g_state > 0) return true;
    if (g_state < 0 && !(g_error == REMIXAPI_ERROR_CODE_NOT_INITIALIZED && g_frames.load() >= g_retry_at)) return false;
    const bool first = g_state == 0;
    g_state = -1;
    const HMODULE d3d9 = GetModuleHandleA("d3d9.dll");
    const auto init = d3d9 ? reinterpret_cast<PFN_remixapi_InitializeLibrary>(GetProcAddress(d3d9, "remixapi_InitializeLibrary"))
                           : nullptr;
    if (!init) { g_error = -1; note("no remixapi_InitializeLibrary in d3d9.dll", -1); return false; }
    remixapi_InitializeLibraryInfo info{};
    info.sType = REMIXAPI_STRUCT_TYPE_INITIALIZE_LIBRARY_INFO;
    info.version = REMIXAPI_VERSION_MAKE(REMIXAPI_VERSION_MAJOR, REMIXAPI_VERSION_MINOR, REMIXAPI_VERSION_PATCH);
    remixapi_Interface out{};
    const remixapi_ErrorCode e = init(&info, &out);
    if (e != REMIXAPI_ERROR_CODE_SUCCESS || !out.CreateLight || !out.DestroyLight || !out.DrawLightInstance) {
        g_error = static_cast<int>(e);
        if (first) note("remixapi_InitializeLibrary failed", g_error);
        if (e == REMIXAPI_ERROR_CODE_NOT_INITIALIZED) g_retry_at = g_frames.load() + 120;
        return false;
    }
    g = out;
    g_state = 1;
    note("ready", 0);
    return true;
}

bool differs(const float* a, const float* b, float eps) {
    return std::fabs(a[0] - b[0]) > eps || std::fabs(a[1] - b[1]) > eps || std::fabs(a[2] - b[2]) > eps;
}

// this frame's light for key: made (again) when new or changed, then drawn
void draw(uint64_t key, const float pos[3], float radius, const float rad[3]) {
    Live& l = g_live[key];
    if (l.h && (differs(pos, l.pos, 0.05f) || differs(rad, l.rad, 1e-4f * (1.0f + rad[0] + rad[1] + rad[2])) ||
                std::fabs(radius - l.radius) > 1e-3f)) {
        g.DestroyLight(l.h);
        l.h = nullptr;
    }
    if (!l.h) {
        remixapi_LightInfoSphereEXT s{};
        s.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO_SPHERE_EXT;
        s.position = { pos[0], pos[1], pos[2] };
        s.radius = radius;
        s.shaping_hasvalue = 0;
        s.volumetricRadianceScale = 1.0f;
        remixapi_LightInfo li{};
        li.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO;
        li.pNext = &s;
        li.hash = key;
        li.radiance = { rad[0], rad[1], rad[2] };
        const remixapi_ErrorCode e = g.CreateLight(&li, &l.h);
        if (e != REMIXAPI_ERROR_CODE_SUCCESS) {
            l.h = nullptr;
            static int fails = 0;
            if (fails++ < 3) note("CreateLight failed", static_cast<int>(e));
        } else if (g_made == 0) {
            note("first light made", 0);
        }
        if (l.h) ++g_made;
        for (int i = 0; i < 3; ++i) { l.pos[i] = pos[i]; l.rad[i] = rad[i]; }
        l.radius = radius;
    }
    if (l.h && g.DrawLightInstance(l.h) == REMIXAPI_ERROR_CODE_SUCCESS) ++g_drawn;
    l.seen = g_frames.load();
}

// The ambient dome (settings.dome): the room's hemisphere lights (g_dome_hemis) as one Remix dome light. A hemisphere
// light gives a surface facing mDir its sky colour (mColor) and one facing away its ground colour (mRevColor),
// lerp(ground, sky, 0.5 + 0.5 n.dir) between: what a dome of the sky colour above and the ground colour below (a hard
// horizon across mDir) gives an unoccluded Lambert surface. Remix's dome light is a lat-long DDS read as radiance
// (light_helper.slangh sampleDomeLightTexture: v = acos(dir.z) / pi, u = 0.5 + atan2(dir.x, dir.y) / 2pi, in the dome's
// space, the world here; DDS only: AssetDataManager::findAsset), seen by rays that leave the scene. DMC4 adds its
// ambient in gamma space (a white surface shows the colour), so the colours go over as c^2.2. The texture is written to
// <game>\remix_bridge\ambient_<hash>.dds when the lights change (at most every 15 frames: keyed fades) and the light is
// made again with it.
namespace dome {
constexpr int kW = 128, kH = 64;
uint64_t g_key = 0;                 // the texture's hash (0 = none yet)
std::wstring g_path;
uint32_t g_written_at = 0;
bool g_drawn = false;               // drawn this frame
bool g_drawn_last = false;          // ... last frame (the ambient fill steps aside)
float g_sky[3] = {}, g_ground[3] = {};   // the GUI's: the summed colours straight up / down

std::wstring folder() {
    static std::wstring dir;
    if (!dir.empty()) return dir;
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring d(exe);
    d = d.substr(0, d.find_last_of(L"\\/") + 1) + L"remix_bridge";
    CreateDirectoryW(d.c_str(), nullptr);
    WIN32_FIND_DATAW fd;   // last session's textures
    const HANDLE f = FindFirstFileW((d + L"\\ambient_*.dds").c_str(), &fd);
    if (f != INVALID_HANDLE_VALUE) {
        do DeleteFileW((d + L"\\" + fd.cFileName).c_str()); while (FindNextFileW(f, &fd));
        FindClose(f);
    }
    dir = d;
    return dir;
}

// The ocean's cube map (settings.dome_cube): read from the D3D9 texture once per texture (managed, so the bridge client
// answers the lock itself), mip level at most 128 wide, decoded to floats. DMC4's environment cubes keep HDR in alpha:
// XfWater's pixel shader reads rgb / a (Vibed/Remix/README.md, "Ocean tint"). Faces in D3D order (+X -X +Y -Y +Z -Z) in
// the game's world axes, which the shader samples with its world reflection vector.
struct Cube {
    uintptr_t rsrc = 0;            // the rTexture it was read from
    int n = 0;                     // face size
    std::vector<float> rgb[6];     // rgb / a, gamma space
    bool ok = false;
    char path[64] = "";
    float mean[6][3] = {};
};
Cube g_cube;

void decode_block(const uint8_t* blk, D3DFORMAT f, uint8_t px[16][4]) {
    uint8_t alpha[16];
    for (auto& a : alpha) a = 255;
    const uint8_t* cb = blk;
    if (f == D3DFMT_DXT5) {
        uint8_t pal[8] = { blk[0], blk[1] };
        if (pal[0] > pal[1]) for (int k = 1; k < 7; ++k) pal[k + 1] = static_cast<uint8_t>(((7 - k) * pal[0] + k * pal[1]) / 7);
        else { for (int k = 1; k < 5; ++k) pal[k + 1] = static_cast<uint8_t>(((5 - k) * pal[0] + k * pal[1]) / 5); pal[6] = 0; pal[7] = 255; }
        uint64_t bits = 0;
        for (int i = 0; i < 6; ++i) bits |= static_cast<uint64_t>(blk[2 + i]) << (8 * i);
        for (int i = 0; i < 16; ++i) alpha[i] = pal[(bits >> (3 * i)) & 7];
        cb = blk + 8;
    } else if (f == D3DFMT_DXT3) {
        for (int i = 0; i < 16; ++i) alpha[i] = static_cast<uint8_t>(((blk[i / 2] >> (4 * (i & 1))) & 15) * 17);
        cb = blk + 8;
    }
    const uint16_t c0 = static_cast<uint16_t>(cb[0] | cb[1] << 8), c1 = static_cast<uint16_t>(cb[2] | cb[3] << 8);
    int col[4][3];
    auto expand = [](uint16_t c, int* o) { o[0] = (c >> 11) * 255 / 31; o[1] = ((c >> 5) & 63) * 255 / 63; o[2] = (c & 31) * 255 / 31; };
    expand(c0, col[0]);
    expand(c1, col[1]);
    const bool four = c0 > c1 || f != D3DFMT_DXT1;
    for (int k = 0; k < 3; ++k) {
        col[2][k] = four ? (2 * col[0][k] + col[1][k]) / 3 : (col[0][k] + col[1][k]) / 2;
        col[3][k] = four ? (col[0][k] + 2 * col[1][k]) / 3 : 0;
    }
    const uint32_t idx = cb[4] | cb[5] << 8 | cb[6] << 16 | static_cast<uint32_t>(cb[7]) << 24;
    for (int i = 0; i < 16; ++i) {
        const int k = (idx >> (2 * i)) & 3;
        for (int c = 0; c < 3; ++c) px[i][c] = static_cast<uint8_t>(col[k][c]);
        px[i][3] = (!four && k == 3) ? 0 : alpha[i];
    }
}

bool read_face(IDirect3DCubeTexture9* cube, int face, UINT level, const D3DSURFACE_DESC& desc, std::vector<float>& out) {
    const int w = static_cast<int>(desc.Width), hgt = static_cast<int>(desc.Height);
    D3DLOCKED_RECT r;
    if (FAILED(cube->LockRect(static_cast<D3DCUBEMAP_FACES>(face), level, &r, nullptr, D3DLOCK_READONLY))) return false;
    out.assign(static_cast<size_t>(w) * hgt * 3, 0.0f);
    auto put = [&](int x, int y, const uint8_t* p, bool bgra) {
        if (x >= w || y >= hgt) return;
        const float a = std::max(p[3] / 255.0f, 1.0f / 255.0f);
        float* o = &out[(static_cast<size_t>(y) * w + x) * 3];
        o[0] = (bgra ? p[2] : p[0]) / 255.0f / a;
        o[1] = p[1] / 255.0f / a;
        o[2] = (bgra ? p[0] : p[2]) / 255.0f / a;
    };
    const auto* base = static_cast<const uint8_t*>(r.pBits);
    bool ok = true;
    if (desc.Format == D3DFMT_DXT1 || desc.Format == D3DFMT_DXT3 || desc.Format == D3DFMT_DXT5) {
        const int bs = desc.Format == D3DFMT_DXT1 ? 8 : 16;
        uint8_t px[16][4];
        for (int by = 0; by < (hgt + 3) / 4; ++by)
            for (int bx = 0; bx < (w + 3) / 4; ++bx) {
                decode_block(base + by * r.Pitch + bx * bs, desc.Format, px);
                for (int i = 0; i < 16; ++i) put(bx * 4 + (i & 3), by * 4 + (i >> 2), px[i], false);
            }
    } else if (desc.Format == D3DFMT_A8R8G8B8 || desc.Format == D3DFMT_X8R8G8B8) {
        for (int y = 0; y < hgt; ++y)
            for (int x = 0; x < w; ++x) {
                uint8_t p[4];
                std::memcpy(p, base + y * r.Pitch + x * 4, 4);
                if (desc.Format == D3DFMT_X8R8G8B8) p[3] = 255;
                put(x, y, p, true);
            }
    } else {
        ok = false;
    }
    cube->UnlockRect(static_cast<D3DCUBEMAP_FACES>(face), level);
    return ok;
}

// the D3D9 cube behind an rTexture (+0xCC cTrans::Texture, its texture at +0x10) and its path (+0x04); SEH-guarded
IDirect3DCubeTexture9* cube_of(uintptr_t rsrc, char* path, size_t n) {
    __try {
        std::memcpy(path, reinterpret_cast<const char*>(rsrc + 4), n - 1);
        path[n - 1] = 0;
        const uintptr_t tt = *reinterpret_cast<const uintptr_t*>(rsrc + 0xCC);
        auto* base = tt ? *reinterpret_cast<IDirect3DBaseTexture9* const*>(tt + 0x10) : nullptr;
        if (!base || base->GetType() != D3DRTYPE_CUBETEXTURE) return nullptr;
        return static_cast<IDirect3DCubeTexture9*>(base);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

// (re)reads g_cube from rsrc (an rTexture)
void load_cube(uintptr_t rsrc) {
    g_cube = Cube{};
    g_cube.rsrc = rsrc;
    if (!rsrc) return;
    IDirect3DCubeTexture9* cube = cube_of(rsrc, g_cube.path, sizeof g_cube.path);
    if (!cube) return;
    {
        UINT level = 0;
        D3DSURFACE_DESC desc;
        if (FAILED(cube->GetLevelDesc(0, &desc))) return;
        while (desc.Width > 128 && level + 1 < cube->GetLevelCount() && SUCCEEDED(cube->GetLevelDesc(level + 1, &desc))) ++level;
        for (int f = 0; f < 6; ++f)
            if (!read_face(cube, f, level, desc, g_cube.rgb[f])) return;
        g_cube.n = static_cast<int>(desc.Width);
        g_cube.ok = desc.Width == desc.Height;
        for (int f = 0; f < 6; ++f) {
            double m[3] = { 0, 0, 0 };
            for (size_t i = 0; i < g_cube.rgb[f].size(); i += 3) for (int c = 0; c < 3; ++c) m[c] += g_cube.rgb[f][i + c];
            for (int c = 0; c < 3; ++c) g_cube.mean[f][c] = static_cast<float>(m[c] / (g_cube.rgb[f].size() / 3));
        }
    }
}

// the cube's texel in world direction d (D3D cube face selection), gamma space
void sample_cube(const float d[3], float out[3]) {
    const float ax = std::fabs(d[0]), ay = std::fabs(d[1]), az = std::fabs(d[2]);
    int face;
    float sc, tc, ma;
    if (ax >= ay && ax >= az) { face = d[0] >= 0 ? 0 : 1; ma = ax; sc = d[0] >= 0 ? -d[2] : d[2]; tc = -d[1]; }
    else if (ay >= az)        { face = d[1] >= 0 ? 2 : 3; ma = ay; sc = d[0]; tc = d[1] >= 0 ? d[2] : -d[2]; }
    else                      { face = d[2] >= 0 ? 4 : 5; ma = az; sc = d[2] >= 0 ? d[0] : -d[0]; tc = -d[1]; }
    const int n = g_cube.n;
    const int x = std::min(n - 1, std::max(0, static_cast<int>((sc / ma + 1.0f) * 0.5f * n)));
    const int y = std::min(n - 1, std::max(0, static_cast<int>((tc / ma + 1.0f) * 0.5f * n)));
    const float* p = &g_cube.rgb[face][(static_cast<size_t>(y) * n + x) * 3];
    for (int c = 0; c < 3; ++c) out[c] = p[c];
}

bool use_cube() { return settings.dome_cube && g_cube.ok && g_cube.n > 0; }

// the radiance from direction d (world): the summed colours of the side of each light d is on, then to linear
void radiance_from(const float d[3], float strength, float out[3]) {
    float c[3] = { 0, 0, 0 };
    for (int i = 0; i < g_ndome_hemis; ++i) {
        const HemiLight& h = g_dome_hemis[i];
        const float* src = (d[0] * h.dir[0] + d[1] * h.dir[1] + d[2] * h.dir[2]) >= 0.0f ? h.sky : h.ground;
        for (int k = 0; k < 3; ++k) c[k] += src[k];
    }
    for (int k = 0; k < 3; ++k) out[k] = std::pow(std::max(c[k], 0.0f), 2.2f) * strength;
}

// With the cube: its colours (to linear), scaled so their mean over the sphere matches the hemisphere lights' (the room's
// ambient level stays the game's, its colours come from the sky the sea reflects); without hemisphere lights, as is.
float g_cube_scale = 1.0f;

bool write(const std::wstring& path, float strength) {
    std::vector<float> px(kW * kH * 4);
    const bool cube = use_cube();
    double hemi_sum = 0, cube_sum = 0, weight = 0;
    for (int y = 0; y < kH; ++y) {
        const float theta = (y + 0.5f) / kH * 3.14159265f;
        const double wgt = std::sin(theta);
        for (int x = 0; x < kW; ++x) {
            const float phi = ((x + 0.5f) / kW - 0.5f) * 6.28318531f;
            const float d[3] = { std::sin(theta) * std::sin(phi), std::sin(theta) * std::cos(phi), std::cos(theta) };
            float* p = &px[(y * kW + x) * 4];
            float hemi[3];
            radiance_from(d, strength, hemi);
            hemi_sum += wgt * (0.2126 * hemi[0] + 0.7152 * hemi[1] + 0.0722 * hemi[2]);
            if (cube) {
                sample_cube(d, p);
                for (int c = 0; c < 3; ++c) p[c] = std::pow(std::max(p[c], 0.0f), 2.2f);
                cube_sum += wgt * (0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]);
            } else {
                for (int c = 0; c < 3; ++c) p[c] = hemi[c];
            }
            p[3] = 1.0f;
            weight += wgt;
        }
    }
    if (cube) {
        g_cube_scale = (g_ndome_hemis > 0 && cube_sum > 1e-9) ? static_cast<float>(hemi_sum / cube_sum)
                                                               : strength;
        for (int i = 0; i < kW * kH; ++i) for (int c = 0; c < 3; ++c) px[i * 4 + c] *= g_cube_scale;
    }
    uint32_t head[32 + 5] = {};
    head[0] = 0x20534444;            // "DDS "
    head[1] = 124;                   // DDS_HEADER: size, flags (caps | height | width | pixel format), height, width,
    head[2] = 0x1007;                // pitch; ddspf at [19]: size 32, DDPF_FOURCC, "DX10"; caps TEXTURE
    head[3] = kH;
    head[4] = kW;
    head[5] = kW * 16;
    head[19] = 32;
    head[20] = 0x4;
    head[21] = 0x30315844;           // "DX10"
    head[27] = 0x1000;
    head[32] = 2;                    // DDS_HEADER_DXT10: DXGI_FORMAT_R32G32B32A32_FLOAT, TEXTURE2D, misc, array 1
    head[33] = 3;
    head[35] = 1;
    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return false;
    const bool ok = fwrite(head, sizeof head, 1, f) == 1 && fwrite(px.data(), px.size() * sizeof(float), 1, f) == 1;
    fclose(f);
    return ok;
}

uint64_t signature(float strength) {   // the lights' colours and directions, quantised (keyed fades: small steps)
    uint64_t h = 1469598103934665603ull;
    auto mix = [&h](int32_t v) { for (int i = 0; i < 4; ++i) { h ^= (v >> (i * 8)) & 0xFF; h *= 1099511628211ull; } };
    for (int i = 0; i < g_ndome_hemis; ++i) {
        const HemiLight& l = g_dome_hemis[i];
        for (int k = 0; k < 3; ++k) {
            mix(static_cast<int32_t>(std::lround(l.sky[k] * 256.0f)));
            mix(static_cast<int32_t>(std::lround(l.ground[k] * 256.0f)));
            mix(static_cast<int32_t>(std::lround(l.dir[k] * 64.0f)));
        }
    }
    mix(static_cast<int32_t>(std::lround(strength * 1000.0f)));
    mix(use_cube() ? static_cast<int32_t>(g_cube.rsrc) : 0);
    return h ? h : 1;
}
} // namespace dome

// a sky texture (settings.sky_textures) drawn in the last 5 seconds: Remix's sky probe has the stage's own sky
bool sky_shown() {
    const uint32_t at = g_sky_tex_frame.load();
    return at != 0 && g_frames.load() - at < 300;
}

// once a frame (api_frame): the dome light, made again when the texture changes
void dome_frame() {
    dome::g_drawn = false;
    if (!settings.game_lights || !settings.dome || sky_shown()) return;
    if (settings.dome_cube && g_ocean_cube != dome::g_cube.rsrc) {
        dome::load_cube(g_ocean_cube);
        if (g_ocean_cube) {
            char line[400];
            const auto& m = dome::g_cube.mean;
            sprintf_s(line, "dome sky from the ocean's cube map %s: %s, %d x %d per face; face means (rgb / a) +X %.2f %.2f %.2f, "
                      "-X %.2f %.2f %.2f, +Y %.2f %.2f %.2f, -Y %.2f %.2f %.2f, +Z %.2f %.2f %.2f, -Z %.2f %.2f %.2f",
                      dome::g_cube.path, dome::g_cube.ok ? "read" : "NOT read", dome::g_cube.n, dome::g_cube.n,
                      m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2],
                      m[3][0], m[3][1], m[3][2], m[4][0], m[4][1], m[4][2], m[5][0], m[5][1], m[5][2]);
            note(line, 0);
        }
    }
    if (g_ndome_hemis == 0 && !dome::use_cube()) return;
    const uint32_t now = g_frames.load();
    const uint64_t sig = dome::signature(settings.dome_strength);
    if (sig != dome::g_key && (dome::g_key == 0 || now - dome::g_written_at >= 15)) {
        wchar_t name[64];
        swprintf_s(name, L"\\ambient_%016llX.dds", static_cast<unsigned long long>(sig));
        const std::wstring path = dome::folder() + name;
        if (dome::write(path, settings.dome_strength)) {
            if (!dome::g_path.empty()) DeleteFileW(dome::g_path.c_str());   // Remix read it when its light was made
            dome::g_path = path;
            dome::g_key = sig;
            dome::g_written_at = now;
            for (int k = 0; k < 3; ++k) {
                const float up[3] = { 0, 1, 0 }, down[3] = { 0, -1, 0 };
                float a[3], b[3];
                dome::radiance_from(up, 1.0f, a);
                dome::radiance_from(down, 1.0f, b);
                dome::g_sky[k] = a[k];
                dome::g_ground[k] = b[k];
            }
        } else {
            static int fails = 0;
            if (fails++ < 3) note("dome texture not written", 0);
        }
    }
    if (!dome::g_key) return;
    const uint64_t key = (uint64_t(3) << 62) | (dome::g_key & 0x3FFFFFFFFFFFFFFFull);
    Live& l = g_live[key];
    if (!l.h) {
        remixapi_LightInfoDomeEXT d{};
        d.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO_DOME_EXT;
        d.transform.matrix[0][0] = d.transform.matrix[1][1] = d.transform.matrix[2][2] = 1.0f;
        d.colorTexture = dome::g_path.c_str();
        remixapi_LightInfo li{};
        li.sType = REMIXAPI_STRUCT_TYPE_LIGHT_INFO;
        li.pNext = &d;
        li.hash = key;
        li.radiance = { 1.0f, 1.0f, 1.0f };
        const remixapi_ErrorCode e = g.CreateLight(&li, &l.h);
        if (e != REMIXAPI_ERROR_CODE_SUCCESS) {
            l.h = nullptr;
            static int fails = 0;
            if (fails++ < 3) note("CreateLight (dome) failed", static_cast<int>(e));
        } else {
            note("dome light made", 0);
        }
    }
    if (l.h && g.DrawLightInstance(l.h) == REMIXAPI_ERROR_CODE_SUCCESS) dome::g_drawn = true;
    l.seen = now;
}

// settings.rtx_options, when it changed since the last frame: each "key = value" through SetConfigVariable (the
// results go to remix_probe.txt)
void apply_options() {
    static char applied[sizeof(settings.rtx_options)] = "";
    char now[sizeof(settings.rtx_options)];
    std::memcpy(now, settings.rtx_options, sizeof(now));
    now[sizeof(now) - 1] = 0;
    if (std::strcmp(applied, now) == 0 || !ready() || !g.SetConfigVariable) return;
    std::memcpy(applied, now, sizeof(applied));
    const std::string text(now);
    const auto trim = [](std::string s) {
        const size_t a = s.find_first_not_of(" \t\r");
        const size_t b = s.find_last_not_of(" \t\r");
        return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
    };
    size_t i = 0;
    while (i < text.size()) {
        size_t j = text.find_first_of(";\n", i);
        if (j == std::string::npos) j = text.size();
        const std::string item = text.substr(i, j - i);
        i = j + 1;
        const size_t eq = item.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(item.substr(0, eq)), value = trim(item.substr(eq + 1));
        if (key.empty()) continue;
        const remixapi_ErrorCode e = g.SetConfigVariable(key.c_str(), value.c_str());
        note(("SetConfigVariable " + key + " = " + value).c_str(), static_cast<int>(e));
    }
}

// the lights not drawn this frame go
void sweep() {
    const uint32_t now = g_frames.load();
    for (auto it = g_live.begin(); it != g_live.end();) {
        if (it->second.seen != now) {
            if (it->second.h) g.DestroyLight(it->second.h);
            it = g_live.erase(it);
        } else {
            ++it;
        }
    }
    g_drawn_last = g_drawn;
    g_drawn = 0;
    dome::g_drawn_last = dome::g_drawn;
}

void destroy_all() {
    for (auto& [k, l] : g_live) if (l.h) g.DestroyLight(l.h);
    g_live.clear();
    dome::g_drawn = dome::g_drawn_last = false;
}

// radiance of a sphere light of radius r that lights a surface facing it at distance d as much as colour c (irradiance
// = radiance x pi r^2 / d^2 for d >> r)
void radiance(const float c[3], float strength, float r, float d, float out[3]) {
    const float k = strength * (d * d) / (3.14159265f * r * r);
    for (int i = 0; i < 3; ++i) out[i] = std::max(0.0f, c[i]) * k;
}
} // namespace api

// lights at the middle of draws using settings.light_textures: found while the frame draws (the indexed-draw hook),
// handed to Remix at the next frame's start, so every pass's draws can take part
namespace texlight {
struct VsRegs { int scale = -1, offset = -1, world = -1, joints = -1, uvscroll = -1; };
std::unordered_map<uintptr_t, VsRegs> g_vs;   // vertex shader -> its registers (from its CTAB)
struct Found { uint64_t key; float pos[3]; };
std::vector<Found> g_found;
std::unordered_set<uint64_t> g_keys;            // this frame's, to take each draw once
uint32_t g_found_last = 0;
constexpr size_t kMax = 64;

// the float4 registers of the names that matter in a vertex shader's constant table (the CTAB comment block)
VsRegs regs_of(IDirect3DVertexShader9* vs) {
    const auto it = g_vs.find(reinterpret_cast<uintptr_t>(vs));
    if (it != g_vs.end()) return it->second;
    VsRegs r;
    UINT size = 0;
    if (vs && SUCCEEDED(vs->GetFunction(nullptr, &size)) && size >= 8 && size < (1u << 20)) {
        std::vector<uint32_t> code((size + 3) / 4);
        if (SUCCEEDED(vs->GetFunction(code.data(), &size))) {
            for (size_t i = 1; i < code.size();) {
                const uint32_t tok = code[i];
                if ((tok & 0xFFFF) != 0xFFFE) break;                       // the comments come before the code
                const uint32_t n = tok >> 16;
                if (n >= 8 && i + 1 + n <= code.size() && code[i + 1] == 0x42415443u) {   // 'CTAB'
                    const uint8_t* ct = reinterpret_cast<const uint8_t*>(&code[i + 2]);
                    const size_t len = (n - 1) * 4;
                    uint32_t count = 0, info = 0;
                    std::memcpy(&count, ct + 12, 4);
                    std::memcpy(&info, ct + 16, 4);
                    for (uint32_t c = 0; c < count && info + 20 * (c + 1) <= len; ++c) {
                        const uint8_t* ci = ct + info + 20 * c;
                        uint32_t name = 0;
                        uint16_t set = 0, reg = 0;
                        std::memcpy(&name, ci, 4);
                        std::memcpy(&set, ci + 4, 2);
                        std::memcpy(&reg, ci + 6, 2);
                        if (set != 2 || name >= len || !std::memchr(ct + name, 0, len - name)) continue;   // FLOAT4 only
                        const char* s = reinterpret_cast<const char*>(ct + name);
                        if (!std::strcmp(s, "gXfQuantPosScale")) r.scale = reg;
                        else if (!std::strcmp(s, "gXfQuantPosOffset")) r.offset = reg;
                        else if (!std::strcmp(s, "gXfWorld")) r.world = reg;
                        else if (!std::strcmp(s, "gXfJointWorld")) r.joints = reg;
                        else if (!std::strcmp(s, "gXfUVScroll")) r.uvscroll = reg;
                    }
                    break;
                }
                i += 1 + n;
            }
        }
    }
    g_vs[reinterpret_cast<uintptr_t>(vs)] = r;
    return r;
}

// one component of a declared position element as the input assembler gives it to the shader
bool decode(BYTE type, const uint8_t* p, float out[3]) {
    out[0] = out[1] = out[2] = 0.0f;
    switch (type) {
    case D3DDECLTYPE_FLOAT3: case D3DDECLTYPE_FLOAT4: std::memcpy(out, p, 12); return true;
    case D3DDECLTYPE_FLOAT16_4: for (int i = 0; i < 3; ++i) { uint16_t h; std::memcpy(&h, p + 2 * i, 2); out[i] = subdiv::half_to_float(h); } return true;
    case D3DDECLTYPE_SHORT4N: for (int i = 0; i < 3; ++i) { int16_t s; std::memcpy(&s, p + 2 * i, 2); out[i] = std::max(-1.0f, s / 32767.0f); } return true;
    case D3DDECLTYPE_SHORT4: for (int i = 0; i < 3; ++i) { int16_t s; std::memcpy(&s, p + 2 * i, 2); out[i] = s; } return true;
    case D3DDECLTYPE_USHORT4N: for (int i = 0; i < 3; ++i) { uint16_t s; std::memcpy(&s, p + 2 * i, 2); out[i] = s / 65535.0f; } return true;
    case D3DDECLTYPE_UBYTE4N: for (int i = 0; i < 3; ++i) out[i] = p[i] / 255.0f; return true;
    case D3DDECLTYPE_UBYTE4: for (int i = 0; i < 3; ++i) out[i] = p[i]; return true;
    default: return false;
    }
}

uint64_t fnv64(const void* data, size_t n, uint64_t h = 1469598103934665603ull) {
    const uint8_t* b = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= 1099511628211ull; }
    return h;
}

// the indexed draw about to run uses a light texture: the middle of its vertices, through the shader's own constants
void capture(IDirect3DDevice9* dev, const uint8_t* cmd) {
    if (g_found.size() >= kMax || !dev || !cmd) return;
    const uint32_t base = *reinterpret_cast<const uint32_t*>(cmd + 0x1C);
    const uint32_t nverts = *reinterpret_cast<const uint16_t*>(cmd + 0x24);
    const uint32_t minidx = *reinterpret_cast<const uint16_t*>(cmd + 0x26);
    if (!nverts) return;
    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DVertexDeclaration9* decl = nullptr;
    IDirect3DVertexShader9* vs = nullptr;
    UINT off = 0, stride = 0;
    float c[3] = { 0, 0, 0 };
    bool ok = false;
    if (SUCCEEDED(dev->GetStreamSource(0, &vb, &off, &stride)) && vb && stride && SUCCEEDED(dev->GetVertexDeclaration(&decl)) &&
        decl) {
        D3DVERTEXELEMENT9 el[MAXD3DDECLLENGTH + 1];
        UINT nel = 0;
        int pe = -1;
        if (SUCCEEDED(decl->GetDeclaration(nullptr, &nel)) && nel <= MAXD3DDECLLENGTH + 1 && SUCCEEDED(decl->GetDeclaration(el, &nel)))
            for (UINT i = 0; i < nel && el[i].Stream != 0xFF; ++i)
                if (el[i].Stream == 0 && el[i].Usage == D3DDECLUSAGE_POSITION && el[i].UsageIndex == 0) { pe = static_cast<int>(i); break; }
        void* p = nullptr;
        if (pe >= 0 && SUCCEEDED(vb->Lock(off + (base + minidx) * stride, nverts * stride, &p, D3DLOCK_READONLY)) && p) {
            float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
            const uint8_t* v = static_cast<const uint8_t*>(p);
            for (uint32_t i = 0; i < nverts; ++i) {
                float q[3];
                if (!decode(el[pe].Type, v + i * stride + el[pe].Offset, q)) break;
                for (int k = 0; k < 3; ++k) { lo[k] = std::min(lo[k], q[k]); hi[k] = std::max(hi[k], q[k]); }
                ok = true;
            }
            vb->Unlock();
            for (int k = 0; k < 3; ++k) c[k] = 0.5f * (lo[k] + hi[k]);
        }
    }
    if (ok && SUCCEEDED(dev->GetVertexShader(&vs)) && vs) {
        const VsRegs r = regs_of(vs);
        float f[12];
        if (r.scale >= 0 && r.offset >= 0 && SUCCEEDED(dev->GetVertexShaderConstantF(r.scale, f, 1)) &&
            SUCCEEDED(dev->GetVertexShaderConstantF(r.offset, f + 4, 1)))
            for (int k = 0; k < 3; ++k) c[k] = c[k] * f[k] + f[4 + k];
        const int rows = r.world >= 0 ? r.world : r.joints;   // a rigid model's world, or a skinned one's first bone
        if (rows >= 0 && SUCCEEDED(dev->GetVertexShaderConstantF(rows, f, 3))) {
            float w[3];
            for (int k = 0; k < 3; ++k) w[k] = f[4 * k] * c[0] + f[4 * k + 1] * c[1] + f[4 * k + 2] * c[2] + f[4 * k + 3];
            for (int k = 0; k < 3; ++k) c[k] = w[k];
        }
    }
    if (vs) vs->Release();
    if (decl) decl->Release();
    if (ok && std::isfinite(c[0]) && std::isfinite(c[1]) && std::isfinite(c[2])) {
        const int32_t cell[3] = { static_cast<int32_t>(std::floor(c[0])), static_cast<int32_t>(std::floor(c[1])),
                                  static_cast<int32_t>(std::floor(c[2])) };
        uint64_t key = fnv64(&vb, sizeof vb);
        key = fnv64(&base, 4, key);
        key = fnv64(&minidx, 4, key);
        key = fnv64(cell, sizeof cell, key);
        key |= 1ull << 63;
        if (g_keys.insert(key).second) g_found.push_back({ key, { c[0], c[1], c[2] } });
    }
    if (vb) vb->Release();
}
} // namespace texlight

volatile uint8_t g_puddle_next = 0;            // the draws after this bind are XfWater with the stand-in (puddle::on_draw)

// ---- puddles (settings.puddles): Fortuna's puddles draw with the sea's shader (XfWater, same samplers, the same cube
// map as st004's sea), so the stand-in made them the sea's material. What tells them apart is size: each draw's vertex
// range measured in the world (the shader's own quantisation and world constants), the puddles' stand-in when it's
// narrower than settings.puddle_size across the ground (game x / z)
namespace puddle {
struct Box { float lo[3], hi[3]; bool ok; };
std::unordered_map<uint64_t, Box> g_local;     // a vertex range's box in its own space, by buffer and range
int g_sea_kind = 0;                            // the bind's stand-in kind for its draws that aren't puddles
IDirect3DPixelShader9* g_ps = nullptr;         // ... drawn with this pixel shader (another one: not that water any more)
std::atomic<uint32_t> g_puddles{ 0 }, g_seas{ 0 };
uint32_t g_puddles_last = 0, g_seas_last = 0;

Box local_box(IDirect3DDevice9* dev, const uint8_t* cmd) {
    Box b{ { 0, 0, 0 }, { 0, 0, 0 }, false };
    const uint32_t base = *reinterpret_cast<const uint32_t*>(cmd + 0x1C);
    const uint32_t nverts = *reinterpret_cast<const uint16_t*>(cmd + 0x24);
    const uint32_t minidx = *reinterpret_cast<const uint16_t*>(cmd + 0x26);
    if (!nverts) return b;
    IDirect3DVertexBuffer9* vb = nullptr;
    UINT off = 0, stride = 0;
    if (FAILED(dev->GetStreamSource(0, &vb, &off, &stride)) || !vb) return b;
    uint64_t key = texlight::fnv64(&vb, sizeof vb);
    key = texlight::fnv64(&off, 4, key);
    key = texlight::fnv64(&stride, 4, key);
    key = texlight::fnv64(&base, 4, key);
    key = texlight::fnv64(&minidx, 4, key);
    key = texlight::fnv64(&nverts, 4, key);
    const auto it = g_local.find(key);
    if (it != g_local.end()) { vb->Release(); return it->second; }
    IDirect3DVertexDeclaration9* decl = nullptr;
    if (stride && SUCCEEDED(dev->GetVertexDeclaration(&decl)) && decl) {
        D3DVERTEXELEMENT9 el[MAXD3DDECLLENGTH + 1];
        UINT nel = 0;
        int pe = -1;
        if (SUCCEEDED(decl->GetDeclaration(nullptr, &nel)) && nel <= MAXD3DDECLLENGTH + 1 && SUCCEEDED(decl->GetDeclaration(el, &nel)))
            for (UINT i = 0; i < nel && el[i].Stream != 0xFF; ++i)
                if (el[i].Stream == 0 && el[i].Usage == D3DDECLUSAGE_POSITION && el[i].UsageIndex == 0) { pe = static_cast<int>(i); break; }
        void* p = nullptr;
        if (pe >= 0 && SUCCEEDED(vb->Lock(off + (base + minidx) * stride, nverts * stride, &p, D3DLOCK_READONLY)) && p) {
            for (int k = 0; k < 3; ++k) { b.lo[k] = 1e30f; b.hi[k] = -1e30f; }
            const uint8_t* v = static_cast<const uint8_t*>(p);
            for (uint32_t i = 0; i < nverts; ++i) {
                float q[3];
                if (!texlight::decode(el[pe].Type, v + i * stride + el[pe].Offset, q)) break;
                for (int k = 0; k < 3; ++k) { b.lo[k] = std::min(b.lo[k], q[k]); b.hi[k] = std::max(b.hi[k], q[k]); }
                b.ok = true;
            }
            vb->Unlock();
        }
        decl->Release();
    }
    vb->Release();
    if (g_local.size() > 4096) g_local.clear();
    g_local[key] = b;
    return b;
}

// the draw's widest extent across the ground in the world (cm), its middle in c; < 0 when it can't be read
float world_size(IDirect3DDevice9* dev, const uint8_t* cmd, float c[3]) {
    Box b = local_box(dev, cmd);
    if (!b.ok) return -1.0f;
    IDirect3DVertexShader9* vs = nullptr;
    if (FAILED(dev->GetVertexShader(&vs)) || !vs) return -1.0f;
    const texlight::VsRegs r = texlight::regs_of(vs);
    vs->Release();
    float f[12];
    if (r.scale >= 0 && r.offset >= 0 && SUCCEEDED(dev->GetVertexShaderConstantF(r.scale, f, 1)) &&
        SUCCEEDED(dev->GetVertexShaderConstantF(r.offset, f + 4, 1)))
        for (int k = 0; k < 3; ++k) {
            const float a = b.lo[k] * f[k] + f[4 + k], z = b.hi[k] * f[k] + f[4 + k];
            b.lo[k] = std::min(a, z);
            b.hi[k] = std::max(a, z);
        }
    const int rows = r.world >= 0 ? r.world : r.joints;
    if (rows >= 0 && SUCCEEDED(dev->GetVertexShaderConstantF(rows, f, 3))) {   // the box's corners through the world rows
        float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
        for (int i = 0; i < 8; ++i) {
            const float q[3] = { (i & 1) ? b.hi[0] : b.lo[0], (i & 2) ? b.hi[1] : b.lo[1], (i & 4) ? b.hi[2] : b.lo[2] };
            for (int k = 0; k < 3; ++k) {
                const float w = f[4 * k] * q[0] + f[4 * k + 1] * q[1] + f[4 * k + 2] * q[2] + f[4 * k + 3];
                lo[k] = std::min(lo[k], w);
                hi[k] = std::max(hi[k], w);
            }
        }
        for (int k = 0; k < 3; ++k) { b.lo[k] = lo[k]; b.hi[k] = hi[k]; }
    }
    for (int k = 0; k < 3; ++k) c[k] = 0.5f * (b.lo[k] + b.hi[k]);
    const float size = std::max(b.hi[0] - b.lo[0], b.hi[2] - b.lo[2]);
    return std::isfinite(size) ? size : -1.0f;
}

// an XfWater draw of a bind with the stand-in: the puddles' stand-in or the bind's own, by its size
void on_draw(IDirect3DDevice9* dev, const uint8_t* cmd) {
    IDirect3DPixelShader9* ps = nullptr;
    dev->GetPixelShader(&ps);
    if (ps) ps->Release();
    if (ps != g_ps) { g_puddle_next = 0; return; }
    float c[3] = { 0, 0, 0 };
    const float size = world_size(dev, cmd, c);
    const bool is_puddle = size >= 0.0f && size < settings.puddle_size;
    dev->SetTexture(0, standin(dev, is_puddle ? 5 : g_sea_kind));
    ++(is_puddle ? g_puddles : g_seas);
    if (g_probe == 2 && g_probe_states.size() < 900) {
        char line[200];
        sprintf_s(line, "  sea-shader water draw: %.0f cm across at %.0f %.0f %.0f -> %s\n", size, c[0], c[1], c[2],
                  is_puddle ? "puddle" : "sea");
        if (g_probe_states.insert(line).second) g_probe_text += line;
    }
}
} // namespace puddle

volatile uint8_t g_light_next = 0;             // the next indexed draw uses a light texture (texlight::capture)
volatile uint8_t g_sky_next = 0;               // the next indexed draw uses a sky texture (settings.sky_textures)
volatile uint8_t g_uvscroll_next = 0;          // this material's vertex shader scrolls its UVs (uv_scroll_draw)
bool g_uv_tt_on = false;                       // the stage-0 texture transform holds a UV scroll
volatile uint8_t g_terrain_next = 0;           // the next draw is a terrain layer (terrain_draw): 1 base, 2 blended
uint8_t g_terrain_mask_stage = 0xFF;           // ... and its blend mask's stage (0xFF: none found)
std::atomic<uint32_t> g_terrain_draws{ 0 };
uint32_t g_terrain_draws_last = 0;
volatile uint8_t g_subdiv_next = 0;            // the draws after this bind are water to subdivide
IDirect3DPixelShader9* g_subdiv_ps = nullptr;  // ... made with this pixel shader (another one: not water any more)

// a sky draw (settings.sky_textures; g_sky_next from on_tag), from either draw path of dispatchCommands: the indexed one
// (water_draw) or DrawPrimitive (0x8F5726, prim_draw: BP stage 1's sky draws this way). 1 = drawn here
int sky_draw(IDirect3DDevice9* dev, const uint8_t* cmd, bool indexed) {
    if (g_probe == 2 && dev) {   // its stage-shader constants (the LDR2 encoding, fog) into the probe
        float c[9][4] = {};
        dev->GetPixelShaderConstantF(32, &c[0][0], 9);
        IDirect3DPixelShader9* sps = nullptr;
        dev->GetPixelShader(&sps);
        if (sps) sps->Release();
        char tag[64];
        sprintf_s(tag, "  sky draw (ps %p) ", static_cast<void*>(sps));
        g_probe_text += tag;
        char line[400];
        sprintf_s(line, "constants: c32 LDR2Offset %.4f | c33 FogEnable %.2f | c34 FogFactor %.3f %.3f %.5f %.3f | "
                  "c35 FogPlane %.3f %.3f %.3f %.3f | c36 FogColor %.3f %.3f %.3f max %.3f | c37 BaseMapScale %.3f %.3f %.3f %.3f | "
                  "c38 Blend %.3f | c40 LightMapScale %.3f %.3f %.3f\n", c[0][0], c[1][0], c[2][0], c[2][1], c[2][2], c[2][3],
                  c[3][0], c[3][1], c[3][2], c[3][3], c[4][0], c[4][1], c[4][2], c[4][3], c[5][0], c[5][1], c[5][2], c[5][3],
                  c[6][0], c[8][0], c[8][1], c[8][2]);
        g_probe_text += line;
    }
    // the sky draws with DMC4's lit stage shader, which fogs toward gXfFogColor (c36, white up to 12.4 % on BP stage 1)
    // and writes x 0.5 + gXfLDR2Offset for the game's final pass to decode (ps 47CF0B20, 2026-11-26). Remix takes that
    // output as the sky's light: the fog, small in vanilla's gamma space, became a veil of ~70 / 255 once encoded
    // (the washed-out planet). So the bridge draws it here with the fog's cap (c36.w) at settings.sky_fog of the game's
    // (gXfFogEnable c33 off at 0; the user: some fog hides where the water ends) and puts it back after; the
    // x 0.5 is undone by rtx.skyBrightness = 2 (rtx.conf). The game's own raster of the sky is covered by Remix's image
    if (dev && cmd) {
        float fog[4] = {}, fog_col[4] = {};
        dev->GetPixelShaderConstantF(33, fog, 1);
        dev->GetPixelShaderConstantF(36, fog_col, 1);
        const float share = std::clamp(settings.sky_fog, 0.0f, 1.0f);
        const uint32_t w = *reinterpret_cast<const uint32_t*>(cmd + 0x18);
        const UINT type = w & 0xFF, prims = w >> 8;
        if (fog[0] != 0.0f && share < 1.0f && prims &&
            (type == D3DPT_TRIANGLELIST || type == D3DPT_TRIANGLESTRIP || type == D3DPT_TRIANGLEFAN)) {
            const float off[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            const float less[4] = { fog_col[0], fog_col[1], fog_col[2], fog_col[3] * share };   // .w: the fog's cap
            if (share <= 0.0f) dev->SetPixelShaderConstantF(33, off, 1);
            else dev->SetPixelShaderConstantF(36, less, 1);
            if (indexed)
                dev->DrawIndexedPrimitive(static_cast<D3DPRIMITIVETYPE>(type), *reinterpret_cast<const INT*>(cmd + 0x1C),
                                          *reinterpret_cast<const uint16_t*>(cmd + 0x26), *reinterpret_cast<const uint16_t*>(cmd + 0x24),
                                          *reinterpret_cast<const UINT*>(cmd + 0x20), prims);
            else
                dev->DrawPrimitive(static_cast<D3DPRIMITIVETYPE>(type), *reinterpret_cast<const UINT*>(cmd + 0x1C), prims);
            dev->SetPixelShaderConstantF(33, fog, 1);
            dev->SetPixelShaderConstantF(36, fog_col, 1);
            return 1;
        }
    }
    return 0;
}

// a terrain layer's draw (settings.terrain_textures; g_terrain_next from on_tag: 1 a base layer, 2 a blended layer of the
// overlap pass) with a plain ps_1_4 for Remix's terrain baker, which re-runs the draw's pixel shader from above: the game's
// shader writes lit, x 0.5-encoded colour (the bake came out grey), and Remix bakes replacement normal / roughness / height
// maps only through ps_1_x shaders (it samples them on stage 7, kTerrainBakerSecondaryTextureStage, kept empty here). The
// blended layers' alpha is their mask's red (st307's XfStage overlap shaders: alpha = mask.r, the mask on stage 1 read with
// TEXCOORD2, or on stage 4 with TEXCOORD3 when the layer has a normal map: found by path (*_AM, on_tag) and from the game
// shader's own texld; the masks are grey, so .b), moved to stage 2 so Remix doesn't take it as a second colour texture.
// The masks carry a per-vertex blend: alpha00_AM is a gradient across v (white, black at 0.5, white), so the layers fade
// out toward their outer edges (vanilla's soft grass / dirt borders). 1 = drawn here
namespace terrain {
const DWORD kBasePs[] = {   // ps_1_4: def c0, 0, 0, 0, 1; texld r0, t0; mov r0.a, c0.a
    0xFFFF0104, 0x00000051, 0xA00F0000, 0x00000000, 0x00000000, 0x00000000, 0x3F800000,
    0x00000042, 0x800F0000, 0xB0E40000, 0x00000001, 0x80080000, 0xA0FF0000, 0x0000FFFF };
// ps_1_4: texld r0, t0; texld r2, t<k>; mov r0.a, r2.b (token 6 = the mask's texture coordinate register, k 0-5)
const DWORD kBlendPs[] = {
    0xFFFF0104, 0x00000042, 0x800F0000, 0xB0E40000, 0x00000042, 0x800F0002, 0xB0E40002,
    0x00000001, 0x80080000, 0x80AA0002, 0x0000FFFF };
IDirect3DPixelShader9* g_ps[2] = {};
IDirect3DPixelShader9* g_mask_ps[6] = {};
std::unordered_map<IDirect3DPixelShader9*, std::array<int8_t, 16>> g_coords;   // game PS -> TEXCOORD index per sampler

// which TEXCOORD each sampler of a ps_2_0 / ps_3_0 is read with (texld v#, s#, the v#'s dcl_texcoord<n>); -1 = not by a
// plain texcoord. From the shader's own tokens (GetFunction), once per shader
const std::array<int8_t, 16>& coords_of(IDirect3DPixelShader9* ps) {
    auto it = g_coords.find(ps);
    if (it != g_coords.end()) return it->second;
    std::array<int8_t, 16> out;
    out.fill(-1);
    UINT size = 0;
    std::vector<DWORD> t;
    if (ps && SUCCEEDED(ps->GetFunction(nullptr, &size)) && size >= 8) {
        t.resize(size / 4);
        if (FAILED(ps->GetFunction(t.data(), &size))) t.clear();
    }
    if (t.size() > 1 && (t[0] & 0xFFFF0000) == 0xFFFF0000 && ((t[0] >> 8) & 0xFF) >= 2) {
        int8_t input_coord[32];
        std::fill(std::begin(input_coord), std::end(input_coord), int8_t(-1));
        const auto reg = [](DWORD tok, int& type) {
            type = static_cast<int>(((tok >> 28) & 7) | ((tok >> 8) & 0x18));
            return static_cast<int>(tok & 0x7FF);
        };
        for (size_t i = 1; i < t.size();) {
            const DWORD tok = t[i];
            if (tok == 0x0000FFFF) break;
            const DWORD op = tok & 0xFFFF;
            if (op == 0xFFFE) { i += 1 + ((tok >> 16) & 0x7FFF); continue; }   // a comment block
            const size_t len = (tok >> 24) & 0x0F;
            if (op == 0x1F && i + 2 < t.size()) {   // dcl: usage token, then the register
                int type = 0;
                const int r = reg(t[i + 2], type);
                if (type == 1 && r < 32 && (t[i + 1] & 0x1F) == 5) input_coord[r] = static_cast<int8_t>((t[i + 1] >> 16) & 0xF);
            } else if (op == 0x42 && i + 3 < t.size()) {   // texld dst, src (v#), sampler (s#)
                int st = 0, ss = 0;
                const int v = reg(t[i + 2], st), s = reg(t[i + 3], ss);
                if (st == 1 && ss == 10 && v < 32 && s < 16 && out[s] < 0) out[s] = input_coord[v];
            }
            i += 1 + len;
        }
    }
    return g_coords.emplace(ps, out).first->second;
}

IDirect3DPixelShader9* mask_ps(IDirect3DDevice9* dev, int k) {
    if (k < 0 || k > 5) return nullptr;
    if (!g_mask_ps[k]) {
        DWORD tokens[sizeof kBlendPs / sizeof kBlendPs[0]];
        memcpy(tokens, kBlendPs, sizeof tokens);
        tokens[6] = 0xB0E40000 | static_cast<DWORD>(k);
        if (FAILED(dev->CreatePixelShader(tokens, &g_mask_ps[k]))) g_mask_ps[k] = nullptr;
    }
    return g_mask_ps[k];
}
bool g_failed = false;
const D3DSAMPLERSTATETYPE kStates[] = { D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSV, D3DSAMP_MAGFILTER, D3DSAMP_MINFILTER,
                                        D3DSAMP_MIPFILTER, D3DSAMP_SRGBTEXTURE };

int draw(IDirect3DDevice9* dev, const uint8_t* cmd, bool indexed, int kind, int mask_stage) {
    if (!dev || !cmd || g_failed) return 0;
    if (!g_ps[0]) {
        if (FAILED(dev->CreatePixelShader(kBasePs, &g_ps[0])) || FAILED(dev->CreatePixelShader(kBlendPs, &g_ps[1]))) {
            for (auto& p : g_ps)
                if (p) { p->Release(); p = nullptr; }
            g_failed = true;
            return 0;
        }
    }
    const uint32_t w = *reinterpret_cast<const uint32_t*>(cmd + 0x18);
    const UINT type = w & 0xFF, prims = w >> 8;
    if (!prims || (type != D3DPT_TRIANGLELIST && type != D3DPT_TRIANGLESTRIP && type != D3DPT_TRIANGLEFAN)) return 0;
    IDirect3DPixelShader9* prev = nullptr;
    IDirect3DBaseTexture9 *t1 = nullptr, *t2 = nullptr, *t7 = nullptr, *mask = nullptr;
    dev->GetPixelShader(&prev);
    dev->GetTexture(1, &t1);
    dev->GetTexture(2, &t2);
    dev->GetTexture(7, &t7);
    DWORD s0_srgb = 0, s2[6] = {};
    dev->GetSamplerState(0, D3DSAMP_SRGBTEXTURE, &s0_srgb);
    for (int i = 0; i < 6; ++i) dev->GetSamplerState(2, kStates[i], &s2[i]);
    int coord = -1;
    IDirect3DPixelShader9* blend_ps = nullptr;
    if (kind == 2 && mask_stage >= 1 && mask_stage < 16) {
        dev->GetTexture(mask_stage, &mask);
        coord = coords_of(prev)[mask_stage];
        blend_ps = mask ? mask_ps(dev, coord) : nullptr;
    }
    const bool blend = blend_ps != nullptr;
    if (g_probe == 2 && g_probe_states.size() < 900) {   // which stages hold what at the bake draw
        IDirect3DBaseTexture9* t0 = nullptr;
        dev->GetTexture(0, &t0);
        DWORD blend_on = 0, src = 0, dst = 0, atest = 0, aref = 0;
        dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &blend_on);
        dev->GetRenderState(D3DRS_SRCBLEND, &src);
        dev->GetRenderState(D3DRS_DESTBLEND, &dst);
        dev->GetRenderState(D3DRS_ALPHATESTENABLE, &atest);
        dev->GetRenderState(D3DRS_ALPHAREF, &aref);
        char line[260];
        sprintf_s(line, "  terrain draw: kind %d, mask on stage %d read with TEXCOORD%d (ps %p), stage 0 %p -> %s | blend %lu "
                  "%lu %lu, atest %lu ref %lu\n", kind, mask_stage == 0xFF ? -1 : mask_stage, coord, static_cast<void*>(prev),
                  static_cast<void*>(t0), blend ? "mask shader" : "base shader", blend_on, src, dst, atest, aref);
        if (t0) t0->Release();
        if (g_probe_states.insert(line).second) g_probe_text += line;
    }
    if (blend) {
        for (int i = 0; i < 5; ++i) {
            DWORD v = 0;
            dev->GetSamplerState(mask_stage, kStates[i], &v);
            dev->SetSamplerState(2, kStates[i], v);
        }
        dev->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, FALSE);
        dev->SetTexture(2, mask);
    }
    dev->SetTexture(1, nullptr);
    dev->SetTexture(7, nullptr);
    if (settings.terrain_srgb) dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
    dev->SetPixelShader(blend ? blend_ps : g_ps[0]);
    if (indexed)
        dev->DrawIndexedPrimitive(static_cast<D3DPRIMITIVETYPE>(type), *reinterpret_cast<const INT*>(cmd + 0x1C),
                                  *reinterpret_cast<const uint16_t*>(cmd + 0x26), *reinterpret_cast<const uint16_t*>(cmd + 0x24),
                                  *reinterpret_cast<const UINT*>(cmd + 0x20), prims);
    else
        dev->DrawPrimitive(static_cast<D3DPRIMITIVETYPE>(type), *reinterpret_cast<const UINT*>(cmd + 0x1C), prims);
    dev->SetPixelShader(prev);
    dev->SetTexture(1, t1);
    dev->SetTexture(2, t2);
    dev->SetTexture(7, t7);
    dev->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, s0_srgb);
    for (int i = 0; i < 6; ++i) dev->SetSamplerState(2, kStates[i], s2[i]);
    for (IUnknown* u : { static_cast<IUnknown*>(prev), static_cast<IUnknown*>(t1), static_cast<IUnknown*>(t2),
                         static_cast<IUnknown*>(t7), static_cast<IUnknown*>(mask) })
        if (u) u->Release();
    ++g_terrain_draws;
    return 1;
}
} // namespace terrain

// UV scroll (2026-11-26): DMC4 scrolls textures in the vertex shader (TEXCOORD0 = UV0 + gXfUVScroll; per material, e.g.
// keyed by a .sdl's "Materials" UV0_U / UV0_V tracks, st305's Lapis River), and under Remix they stood still (the
// river, Dante's Pandora; user). For draws whose vertex shader has gXfUVScroll, its current value becomes the stage-0
// texture transform's translation (Remix 1.6.1 applies D3DTS_TEXTURE0 + TEXTURETRANSFORMFLAGS to a draw's texture
// coordinates; D3D9 ignores them for shader draws, so the game's image is unchanged). Read at the draw: the constant is
// the draw's own. Wrapped to 0..1 (the keys run to -14 and further)
void uv_scroll_off(IDirect3DDevice9* dev) {
    if (!g_uv_tt_on) return;
    dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    g_uv_tt_on = false;
}
void uv_scroll_draw(IDirect3DDevice9* dev) {
    IDirect3DVertexShader9* vs = nullptr;
    dev->GetVertexShader(&vs);
    if (vs) vs->Release();
    const int reg = vs ? texlight::regs_of(vs).uvscroll : -1;
    float c[4] = { 0, 0, 0, 0 };
    if (reg < 0 || FAILED(dev->GetVertexShaderConstantF(reg, c, 1)) || (c[0] == 0.0f && c[1] == 0.0f) ||
        !std::isfinite(c[0]) || !std::isfinite(c[1])) {
        uv_scroll_off(dev);
        return;
    }
    D3DMATRIX m{};
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    m._31 = c[0] - std::floor(c[0]);   // 2-element texture coordinates: the third row is the translation
    m._32 = c[1] - std::floor(c[1]);
    dev->SetTransform(D3DTS_TEXTURE0, &m);
    dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    g_uv_tt_on = true;
}

int __stdcall prim_draw(IDirect3DDevice9* dev, const uint8_t* cmd) {
    if (g_uvscroll_next && dev) uv_scroll_draw(dev);
    if (g_terrain_next) {
        const int kind = g_terrain_next;
        g_terrain_next = 0;
        if (terrain::draw(dev, cmd, false, kind, g_terrain_mask_stage)) return 1;
    }
    if (!g_sky_next) return 0;
    g_sky_next = 0;
    return sky_draw(dev, cmd, false);
}

// the indexed draw hook: 1 = drawn here (subdivided), 0 = the game draws it
int __stdcall water_draw(IDirect3DDevice9* dev, const uint8_t* cmd) {
    if (g_uvscroll_next && dev) uv_scroll_draw(dev);
    if (g_sky_next) {   // a sky draw (sky_draw)
        g_sky_next = 0;
        if (sky_draw(dev, cmd, true)) return 1;
    }
    if (g_terrain_next) {   // a terrain layer (terrain::draw)
        const int kind = g_terrain_next;
        g_terrain_next = 0;
        if (terrain::draw(dev, cmd, true, kind, g_terrain_mask_stage)) return 1;
    }
    if (g_light_next) {   // a light texture's draw: its middle for the next frame's light
        g_light_next = 0;
        texlight::capture(dev, cmd);
    }
    if (g_puddle_next && dev && cmd) puddle::on_draw(dev, cmd);   // before a subdivided draw: it takes the texture
    if (!g_subdiv_next || settings.water_detail <= 0 || !dev || !cmd) return 0;   // cleared outside the main scene (on_tag)
    IDirect3DPixelShader9* ps = nullptr;
    dev->GetPixelShader(&ps);
    if (ps) ps->Release();
    if (ps != g_subdiv_ps) { g_subdiv_next = 0; g_iw_graded_next = 0; return 0; }
    if (g_iw_graded_next && graded::draw(dev)) { ++subdiv::g_drawn; return 1; }
    subdiv::Key k{};
    const uint32_t w = *reinterpret_cast<const uint32_t*>(cmd + 0x18);
    k.type = w & 0xFF;
    k.prims = w >> 8;
    k.base = *reinterpret_cast<const uint32_t*>(cmd + 0x1C);
    k.start = *reinterpret_cast<const uint32_t*>(cmd + 0x20);
    k.nverts = *reinterpret_cast<const uint16_t*>(cmd + 0x24);
    k.minidx = *reinterpret_cast<const uint16_t*>(cmd + 0x26);
    k.level = static_cast<uint32_t>(std::min(settings.water_detail, 3));
    if ((k.type != D3DPT_TRIANGLELIST && k.type != D3DPT_TRIANGLESTRIP) || !k.prims || !k.nverts) return 0;
    IDirect3DVertexBuffer9* vb = nullptr;
    IDirect3DIndexBuffer9* ib = nullptr;
    IDirect3DVertexDeclaration9* decl = nullptr;
    UINT off = 0, stride = 0;
    int done = 0;
    if (SUCCEEDED(dev->GetStreamSource(0, &vb, &off, &stride)) && vb && SUCCEEDED(dev->GetIndices(&ib)) && ib &&
        SUCCEEDED(dev->GetVertexDeclaration(&decl)) && decl && stride) {
        k.vb = reinterpret_cast<uintptr_t>(vb);
        k.ib = reinterpret_cast<uintptr_t>(ib);
        k.decl = reinterpret_cast<uintptr_t>(decl);
        k.off = off;
        k.stride = stride;
        subdiv::Entry& e = subdiv::g_cache[k];
        if (e.vb && g_frames - e.verified > 300) {   // the game's buffer still holds that mesh?
            std::vector<uint8_t> v;
            std::vector<uint32_t> t;
            uint32_t check = 0;
            if (!subdiv::read(k, vb, ib, v, t, &check) || check != e.check) { subdiv::release(e); e.failed = false; }
            e.verified = g_frames;
        }
        if (!e.vb && !e.failed) {
            e.failed = !subdiv::build(dev, k, vb, ib, decl, e);
            e.verified = g_frames;
        }
        if (e.vb && e.ib) {
            dev->SetStreamSource(0, e.vb, 0, stride);
            dev->SetIndices(e.ib);
            dev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, e.nv, 0, e.nt);
            dev->SetStreamSource(0, vb, off, stride);
            dev->SetIndices(ib);
            e.last = g_frames;
            ++subdiv::g_drawn;
            done = 1;
        }
    }
    if (vb) vb->Release();
    if (ib) ib->Release();
    if (decl) decl->Release();
    return done;
}

bool     g_in_main = false;

bool      g_have_main = false;          // the main camera's matrices of the last frame (vs_array)
D3DMATRIX g_main_v{}, g_main_p{};
// the next 8 of the frame's lights into the D3D9 slots (Remix adds them at the next draw); slots left over are switched off
void submit_lights(IDirect3DDevice9* dev) {
    int slot = 0;
    for (; slot < kMaxLights && g_frame_next < g_frame_n; ++slot, ++g_frame_next) {
        dev->SetLight(slot, &g_frame[g_frame_next]);
        dev->LightEnable(slot, TRUE);
    }
    for (int i = slot; i < g_prev_enabled; ++i) dev->LightEnable(i, FALSE);
    g_prev_enabled = slot;
}

void set_transforms(IDirect3DDevice9* dev) {
    const uintptr_t camera = *reinterpret_cast<uintptr_t*>(kCameraInstance);
    if (!camera || !dev) return;
    const auto* cam = reinterpret_cast<const sCamera*>(camera);
    D3DMATRIX view, proj, world;
    std::memcpy(&view, &cam->viewports[0].mViewMat, sizeof view);     // row vectors, as D3D
    std::memcpy(&proj, &cam->viewports[0].mProjMat, sizeof proj);
    if (g_have_main) { view = g_main_v; proj = g_main_p; }           // last frame's main camera (sCamera only until then)
    std::memset(&world, 0, sizeof world);
    world._11 = world._22 = world._33 = world._44 = 1.0f;
    dev->SetTransform(D3DTS_WORLD, &world);
    g_world_set = world;
    dev->SetTransform(D3DTS_VIEW, &view);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
    // a white fixed-function material: Remix colours the lights it adds to tagged textures ("Add Light to Texture",
    // scene manager createEffectLight) with its Diffuse times rtx.effectLightColor; DMC4 never sets one (black lights)
    {
        D3DMATERIAL9 mat{};
        mat.Diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
        mat.Ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
        dev->SetMaterial(&mat);
    }
    // lights: Remix makes its lights of every enabled D3D9 light (d3d9_rtx.cpp, DirtyLights), shader draws too; the frame's
    // list goes over in batches of 8 (submit_lights: here, then at each new material until all are given)
    g_frame_n = g_frame_next = 0;
    const bool api_points = settings.api_lights && api::ready();   // the point lights go through the API (api_frame)
    if (settings.game_lights && g_nlights > 0) {
        for (int i = 0; i < g_nlights; ++i) {
            const GameLight& g = g_lights[i];
            if (g.kind != 1 && api_points) continue;
            const float k = g.kind == 1 ? settings.dir_strength : settings.point_strength;
            D3DLIGHT9 l{};
            l.Diffuse = { g.col[0] * k, g.col[1] * k, g.col[2] * k, 1.0f };
            if (g.kind == 1) {
                l.Type = D3DLIGHT_DIRECTIONAL;
                l.Direction = { g.v[0], g.v[1], g.v[2] };
            } else {
                l.Type = D3DLIGHT_POINT;
                l.Position = { g.v[0], g.v[1], g.v[2] };
                l.Range = g.range > 1.0f ? g.range : 1000.0f;
                l.Attenuation0 = 1.0f;
            }
            g_frame[g_frame_n++] = l;
        }
    } else if (settings.sun) {   // no light units found: the shadow light
        D3DLIGHT9 l{};
        l.Type = D3DLIGHT_DIRECTIONAL;
        l.Diffuse = { settings.sun_color[0] * settings.sun_strength, settings.sun_color[1] * settings.sun_strength,
                      settings.sun_color[2] * settings.sun_strength, 1.0f };
        l.Direction = { g_sun_dir[0], g_sun_dir[1], g_sun_dir[2] };
        g_frame[g_frame_n++] = l;
    }
    // the ambient fill: the stage's hemisphere colour as a wide point light at the camera
    if (settings.game_lights && settings.fill && !api::dome::g_drawn_last && (g_hemi[0] + g_hemi[1] + g_hemi[2]) > 0.0f) {
        float pos[3];   // the camera: -t * R^T of the row-vector view
        for (int i = 0; i < 3; ++i) pos[i] = -(view.m[3][0] * view.m[i][0] + view.m[3][1] * view.m[i][1] + view.m[3][2] * view.m[i][2]);
        D3DLIGHT9 l{};
        l.Type = D3DLIGHT_POINT;
        l.Diffuse = { g_hemi[0] * settings.fill_strength, g_hemi[1] * settings.fill_strength, g_hemi[2] * settings.fill_strength, 1.0f };
        l.Position = { pos[0], pos[1], pos[2] };
        l.Range = settings.fill_range;
        l.Attenuation0 = 1.0f;
        g_frame[g_frame_n++] = l;
    }
    submit_lights(dev);
}

// Lights that ride a moving model (2026-11-26). Remix gets only an object's translation (world_positions), so a light
// in a Remix mod (DMC4Lights) can't follow a model that turns. Room 200's two chandeliers (scr\st200\stage\st200-m90,
// units of st200-md90-denki.sdl) swing +-8 deg about a pivot ~21 m above their lamp (mAngle keys, 8 s), and their
// candle flames (es200_00v0, parented 20 m below the pivot) are effects, which Remix never sees: the user missed their
// light. Each frame the model's units are found on the move lines (a uModel: rModel* at +0xA30, its path at +0x04;
// mWmat at +0xA0, row vectors, translation in row 3), and every listed point, in the model's own frame, becomes an API
// sphere light at point x mWmat. Brightness as the stage's point lights and the Remix mods' lights: api::radiance at
// settings.point_distance (room_lights_mod.py: strength = irradiance at 2 m for colour x strength).
namespace unitlight {
struct Spec { const char* model; float pt[3]; float strength, radius; const float* col; };
constexpr float kWarm[3] = { 1.0f, 0.78f, 0.55f };   // room 200's lamps (room_lights_mod.py WARM)
// the 12 candles of effect\efs\scr\st200_00 (part 0: a 115 cm ring, 79 / 89 cm above the effect's origin) + the effect's
// mOfs (0, -2000, 0) on the model, 7 cm up to the flame; the ring (saku03 / 04) is at -2100 .. -1913
constexpr Spec kSpecs[] = {
    { "scr\\st200\\stage\\st200-m90", {  29.8f, -1904.0f,  111.1f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", {  81.3f, -1914.0f,   81.4f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { 111.1f, -1904.0f,   29.7f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { 111.1f, -1914.0f,  -29.8f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", {  81.4f, -1904.0f,  -81.3f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", {  29.7f, -1914.0f, -111.1f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { -29.8f, -1904.0f, -111.1f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { -81.3f, -1914.0f,  -81.4f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", {-111.1f, -1904.0f,  -29.7f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", {-111.1f, -1914.0f,   29.8f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { -81.4f, -1904.0f,   81.3f }, 0.8f, 2.0f, kWarm },
    { "scr\\st200\\stage\\st200-m90", { -29.7f, -1914.0f,  111.1f }, 0.8f, 2.0f, kWarm },
};
constexpr int kSpecCount = static_cast<int>(sizeof kSpecs / sizeof kSpecs[0]);
constexpr uintptr_t kModelRes = 0xA30, kWmat = 0xA0;
struct Light { uint64_t key; float pos[3]; int spec; };
constexpr int kMax = 128;
Light g_last[kMax];
int g_nlast = 0;
uint32_t g_count = 0;   // lights drawn last frame (the panel)

// render thread, while the game thread runs the units: SEH-guarded, -1 on a fault
int find_raw(Light* out, int max, uintptr_t dti_model) {
    __try {
        const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
        if (!su) return 0;
        int n = 0;
        for (int line = 0; line < 32; ++line) {
            const auto* ml = reinterpret_cast<const MoveLine*>(su + 0x20 + line * sizeof(MoveLine));
            int guard = 0;
            for (uintptr_t u = reinterpret_cast<uintptr_t>(ml->mTop); u && guard < 20000; ++guard) {
                const auto* cu = reinterpret_cast<const cUnit*>(u);
                const uintptr_t next = reinterpret_cast<uintptr_t>(cu->mp_next_unit);
                if (cu->flags.bits.mBeFlag && (cu->flags.bits.mUnitAttr & UATTR_DRAW) &&
                    dti_under(debug_server::mem::dti_of(u), dti_model)) {
                    const uintptr_t res = *reinterpret_cast<const uintptr_t*>(u + kModelRes);
                    const char* path = res ? reinterpret_cast<const char*>(res + 4) : nullptr;
                    const float* m = reinterpret_cast<const float*>(u + kWmat);
                    for (int s = 0; path && s < kSpecCount && n < max; ++s) {
                        if (_stricmp(path, kSpecs[s].model) != 0) continue;
                        const float* p = kSpecs[s].pt;
                        Light& l = out[n++];
                        l.key = (uint64_t(3) << 60) | (uint64_t(u) << 8) | uint64_t(s);
                        l.spec = s;
                        for (int i = 0; i < 3; ++i) l.pos[i] = p[0] * m[i] + p[1] * m[4 + i] + p[2] * m[8 + i] + m[12 + i];
                    }
                }
                u = next;
            }
        }
        return n;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void frame() {
    static uintptr_t dti_model = 0;
    if (!dti_model) dti_model = debug_server::mem::find_dti("uModel");
    if (!dti_model) return;
    Light found[kMax];
    const int n = find_raw(found, kMax, dti_model);
    if (n >= 0) {   // a fault mid-walk keeps last frame's
        std::memcpy(g_last, found, sizeof(Light) * n);
        g_nlast = n;
    }
    float rad[3];
    for (int i = 0; i < g_nlast; ++i) {
        const Spec& s = kSpecs[g_last[i].spec];
        if (!std::isfinite(g_last[i].pos[0]) || !std::isfinite(g_last[i].pos[1]) || !std::isfinite(g_last[i].pos[2])) continue;
        api::radiance(s.col, s.strength, s.radius, settings.point_distance, rad);
        api::draw(g_last[i].key, g_last[i].pos, s.radius, rad);
    }
    g_count = static_cast<uint32_t>(g_nlast);
}
} // namespace unitlight

// once a frame, after gather_lights: the stage's point lights and last frame's texture lights through the Remix API
void api_frame() {
    if (settings.on && settings.rtx_options[0]) api::apply_options();
    if (!settings.on || !settings.api_lights) {
        if (api::g_state > 0 && !api::g_live.empty()) api::destroy_all();
        texlight::g_found.clear();
        texlight::g_keys.clear();
        return;
    }
    if (!api::ready()) return;
    float rad[3];
    if (settings.game_lights)
        for (int i = 0; i < g_nlights; ++i) {
            const GameLight& g = g_lights[i];
            if (g.kind == 1 || !g.unit) continue;
            api::radiance(g.col, settings.point_strength, settings.point_radius, settings.point_distance, rad);
            api::draw((uint64_t(1) << 62) | g.unit, g.v, settings.point_radius, rad);
        }
    unitlight::frame();   // lights riding moving models (room 200's chandeliers)
    api::radiance(settings.tex_light_color, settings.tex_light_strength, settings.tex_light_radius, settings.point_distance, rad);
    for (const auto& f : texlight::g_found) api::draw(f.key, f.pos, settings.tex_light_radius, rad);
    api::dome_frame();
    texlight::g_found_last = static_cast<uint32_t>(texlight::g_found.size());
    texlight::g_found.clear();
    texlight::g_keys.clear();
    api::sweep();
}

// The pixel-shader texture loop of dispatchCommands, per sampler entry of the pass (`record` +0x0C low nibble = the
// count, +0x10 the entries: bits 0-15 the parameter index, 16-23 the D3D9 stage, 24-31 the sampler state; `slot` = the
// entry's resource (rTexture's D3D9 handles at +0x10 / +0x14, picked by +0x22 bit 0 & Camera); `cache` = the game's
// per-parameter "already bound" table). Returns true when it bound the texture itself.
// Remix (d3d9_rtx.cpp, internalPrepareDraw) takes a shader draw's colour texture from stage 0 and a second one from stage
// 1, whatever the shader samples there; DMC4's shaders put the albedo on any stage (the floor's normal map came out as the
// colour). So when the pass has an albedo map: the albedo goes to stage 0 and whatever the shader had on stage 0 to the
// albedo's stage. The game's cache is invalidated for every entry bound here, so later draws (other scenes, the bridge
// off) bind again instead of trusting stages this moved.
uint32_t g_pass = 0;                         // the current tag's pass (bits 20-23), for the pass log

// diagnostics: each distinct pass layout of the main scene once (the samplers' name IDs and stages, the tag's pass and
// what the bridge did), appended to remix_passes.txt in the game folder. Render thread only.
void log_layout(const uint32_t* entries, uint32_t n, const char* what) {
    static std::unordered_set<std::string> seen;
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
    if (!shader || seen.size() > 4000) return;
    std::string sig = "pass " + std::to_string(g_pass) + ":";
    char buf[32];
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t p = entries[i] & 0xFFFF, stage = (entries[i] >> 16) & 0xFF;
        const uint32_t id = p < 512 ? *reinterpret_cast<const uint32_t*>(shader + kParamDescs + 20 * p + 8) : 0;
        sprintf_s(buf, " s%u=%08X", stage, id);
        sig += buf;
    }
    if (!seen.insert(sig).second) return;
    FILE* f = nullptr;
    if (fopen_s(&f, "remix_passes.txt", "a") == 0 && f) {
        fprintf(f, "%s -> %s\n", sig.c_str(), what);
        fclose(f);
    }
}

int __stdcall bind_texture(IDirect3DDevice9* dev, const uint32_t* entry, const uint32_t* slot, const uint8_t* record,
                           uint32_t* cache) {
    if (!settings.on || !settings.albedo_to_stage0) return 0;
    const uint32_t albedo = *reinterpret_cast<const uint32_t*>(kAlbedoParam);
    const uint32_t n = *reinterpret_cast<const uint32_t*>(record + 0x0C) & 0xF;
    const auto* entries = reinterpret_cast<const uint32_t*>(record + 0x10);
    uint32_t albedo_stage = 0xFF, albedo_index = 0xFF;
    bool water = false;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t p = entries[i] & 0xFFFF;
        if (albedo_stage == 0xFF && is_colour(p, albedo)) { albedo_stage = (entries[i] >> 16) & 0xFF; albedo_index = i; }
        else if (is_water(p)) water = true;
    }
    // the probe: each textured draw's albedo map with the states Remix reads for it (alpha test, blending, depth write)
    if (g_probe == 2 && entry == entries && albedo_index != 0xFF && g_pass >= 2 && g_pass <= 5 && g_probe_states.size() < 900) {
        const uintptr_t tex = (slot - (entry - entries))[albedo_index];
        const auto it = g_tex_paths.find(tex);
        DWORD at = 0, af = 0, ar = 0, ab = 0, sb = 0, db = 0, op = 0, zw = 0, cull = 0, sep = 0;
        dev->GetRenderState(D3DRS_ALPHATESTENABLE, &at);
        dev->GetRenderState(D3DRS_ALPHAFUNC, &af);
        dev->GetRenderState(D3DRS_ALPHAREF, &ar);
        dev->GetRenderState(D3DRS_ALPHABLENDENABLE, &ab);
        dev->GetRenderState(D3DRS_SRCBLEND, &sb);
        dev->GetRenderState(D3DRS_DESTBLEND, &db);
        dev->GetRenderState(D3DRS_BLENDOP, &op);
        dev->GetRenderState(D3DRS_ZWRITEENABLE, &zw);
        dev->GetRenderState(D3DRS_CULLMODE, &cull);
        dev->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, &sep);
        const uint32_t fmt = tex ? *reinterpret_cast<const uint32_t*>(tex + 0x24) : 0;
        const int k = ++g_probe_tex_count[tex];
        IDirect3DVertexShader9* vs = nullptr;
        dev->GetVertexShader(&vs);
        if (vs) vs->Release();
        IDirect3DPixelShader9* ps = nullptr;
        dev->GetPixelShader(&ps);
        if (ps) ps->Release();
        dump_shader_pair(vs, ps);
        float wp[3] = { 0, 0, 0 };
        if (g_world_this_draw && g_cur_world && g_vp_transposed >= 0) {
            const float* w = reinterpret_cast<const float*>(g_cur_world);
            if (g_vp_transposed == 1) { wp[0] = w[3]; wp[1] = w[7]; wp[2] = w[11]; }
            else { wp[0] = w[12]; wp[1] = w[13]; wp[2] = w[14]; }
        } else if (g_joints_this_draw && g_cur_joints) {   // a skinned draw: its first bone's translation
            const float* j = reinterpret_cast<const float*>(g_cur_joints);
            wp[0] = j[3]; wp[1] = j[7]; wp[2] = j[11];
        }
        char line[400];
        sprintf_s(line, "  states pass %u, %s #%d (fmt %08X, %u maps): atest %lu func %lu ref %lu | blend %lu src %lu dst %lu op %lu "
                        "sep %lu | zwrite %lu cull %lu | block %d | vs %p ps %p | world at %.1f %.1f %.1f\n",
                  g_pass, it != g_tex_paths.end() ? it->second.c_str() : "(no path)", k, fmt, n, at, af, ar, ab, sb, db, op, sep,
                  zw, cull, g_block_kind, static_cast<void*>(vs), static_cast<void*>(ps), wp[0], wp[1], wp[2]);
        if (g_probe_states.insert(line).second) g_probe_text += line;
    }
    if (g_probe == 2 && g_block_kind != 0 && g_probe_paths.size() < 300) {
        const auto it = g_tex_paths.find(*slot);
        char line[200];
        sprintf_s(line, "      %s, pass %u, stage %u: %s\n", g_block_kind == 1 ? "copy of the main camera" : "another camera",
                  g_pass, (*entry >> 16) & 0xFF, !*slot ? "(none)" : it != g_tex_paths.end() ? it->second.c_str() : "(no path)");
        if (g_probe_paths.insert(line).second) g_probe_text += line;
    }
    if (entry == entries && g_probe == 2 && g_block_kind != 0 && g_probe_layouts.size() < 64) {
        const uintptr_t shader = *reinterpret_cast<uintptr_t*>(kShaderInstance);
        std::string sig = (g_block_kind == 1 ? std::string("  draw under a copy of the main camera")
                                             : "  draw under camera " + std::to_string(g_other_cam & 0xFFFF)) +
                          ", pass " + std::to_string(g_pass) + ":";
        char buf[40];
        for (uint32_t i = 0; i < n; ++i) {
            const uint32_t p = entries[i] & 0xFFFF, st = (entries[i] >> 16) & 0xFF;
            const uint32_t id = shader && p < 512 ? *reinterpret_cast<const uint32_t*>(shader + kParamDescs + 20 * p + 8) : 0;
            sprintf_s(buf, " s%u=%08X", st, id);
            sig += buf;
        }
        if (g_probe_layouts.insert(sig).second) {
            IDirect3DSurface9* rt = nullptr;
            D3DSURFACE_DESC d{};
            if (SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) { rt->GetDesc(&d); rt->Release(); }
            sprintf_s(buf, " | target %ux%u f%u", d.Width, d.Height, unsigned(d.Format));
            g_probe_text += sig + buf + "\n";
        }
    }
    if (entry == entries) {
        const bool hiding = albedo_stage == 0xFF && !water &&
            ((g_pass == kOcclusion && settings.hide_occlusion) ||
             (g_pass == kOverlap && settings.hide_shadow_overlays && g_shadow_param >= 0));
        log_layout(entries, n, albedo_stage != 0xFF ? "colour to stage 0" : water && settings.water_standin ? "water stand-in"
                                                                       : hiding ? "hidden (if shadow overlay)" : "left alone");
    }
    // draws Remix shouldn't trace: the occlusion-query quads (the depth texture on stage 0: white diamonds at the lights)
    // and the stage drawn again with only its shadow maps (duplicate surfaces with a shadow map as their colour)
    bool hide = false;
    if (!g_hide_tex.empty()) {
        const uint32_t* first = slot - (entry - entries);   // the draw's resources run beside its entries
        for (uint32_t i = 0; i < n; ++i)
            if (g_hide_tex.count(first[i])) hide = true;
    }
    if (settings.hide_refraction && g_prim_scene_param >= 0)
        for (uint32_t i = 0; i < n; ++i)
            if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_prim_scene_param)) hide = true;
    const bool other = g_other_cam != 0 && settings.other_cameras != 0;
    if (other && (settings.other_cameras == 1 || g_other_ortho)) hide = true;
    if (other && entry == entries) ++g_other_draws;
    if (entry == entries) {   // a light texture's draw: the indexed-draw hook takes its middle (texlight::capture)
        bool light = false;
        if (!hide && !g_light_tex.empty() && settings.api_lights) {
            const uint32_t* first = slot - (entry - entries);
            for (uint32_t i = 0; i < n; ++i)
                if (g_light_tex.count(first[i])) light = true;
        }
        g_light_next = light ? 1 : 0;
        g_terrain_next = 0;
        g_terrain_mask_stage = 0xFF;
        if (!hide && !water && !g_terrain_tex.empty()) {   // a terrain layer (settings.terrain_textures): terrain_draw
            const uint32_t* first = slot - (entry - entries);
            for (uint32_t i = 0; i < n; ++i)
                if (g_terrain_tex.count(first[i])) {
                    g_terrain_next = g_pass == kOverlap ? 2 : 1;
                    break;
                }
            // the mask's stage: XfStage's overlap shaders keep it on stage 1 (no normal map) or 4 (normal-mapped layers,
            // which have the normal map on stage 1: reading that as the mask gave them alpha 1, hard polygon edges)
            if (g_terrain_next == 2)
                for (uint32_t i = 0; i < n; ++i)
                    if (g_mask_tex.count(first[i])) {
                        g_terrain_mask_stage = static_cast<uint8_t>((entries[i] >> 16) & 0xFF);
                        break;
                    }
        }
        if (!g_sky_tex.empty()) {   // the stage's sky (settings.sky_textures): the dome steps aside
            const uint32_t* first = slot - (entry - entries);
            for (uint32_t i = 0; i < n; ++i)
                if (g_sky_tex.count(first[i])) {
                    g_sky_tex_frame = g_frames.load();
                    g_sky_next = 1;
                    break;
                }
        }
    }
    // the main view = the block most draws use (counted per draw: uploads were too few to tell the main camera from the
    // shadow overlay's projector, and with other cameras' draws hidden a wrong pick hid the whole scene for a frame)
    if (entry == entries && g_cur_vp) count_view(g_cur_vp);
    if (entry == entries) {
        if (g_frame_next < g_frame_n) submit_lights(dev);   // the next batch of the frame's lights
        const bool water_tt = !hide && albedo_stage == 0xFF && water && settings.water_standin;
        water_transform(dev, water_tt);
        {   // UV scroll: this material's vertex shader has gXfUVScroll (uv_scroll_draw sets it per draw); water has its own
            IDirect3DVertexShader9* vs = nullptr;
            dev->GetVertexShader(&vs);
            if (vs) vs->Release();
            const bool scroll = settings.uv_scroll && !hide && !water_tt && vs && texlight::regs_of(vs).uvscroll >= 0;
            g_uvscroll_next = scroll ? 1 : 0;
            if (!scroll) uv_scroll_off(dev);
        }
        const bool sky = (settings.sky && g_pass == 2 && n == 1 && g_base_param >= 0 &&
                          (entries[0] & 0xFFFF) == static_cast<uint32_t>(g_base_param));
        sky_viewport(dev, sky);
        if (sky) ++g_sky;
    }
    if (albedo_stage == 0xFF && !water) {
        if (g_pass == kOcclusion && settings.hide_occlusion) hide = true;
        if (g_pass == kOverlap && settings.hide_shadow_overlays && g_shadow_param >= 0)
            for (uint32_t i = 0; i < n; ++i)
                if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_shadow_param)) hide = true;
        if (settings.hide_glows && g_prim_depth_param >= 0)
            for (uint32_t i = 0; i < n; ++i)
                if ((entries[i] & 0xFFFF) == static_cast<uint32_t>(g_prim_depth_param)) hide = true;
    }
    // water (no albedo map, its wave / flow maps on stage 0 came out teal / magenta): a stand-in colour on stage 0, the
    // shader's own stage 0 texture on stage 15
    // water layouts the user hides (puddles: square planes over dips that the game fades out by depth, which Remix can't)
    if (!hide && albedo_stage == 0xFF && water) {
        bool zbuf = false, flat = false;
        for (uint32_t i = 0; i < n; ++i) {
            const int p = static_cast<int>(entries[i] & 0xFFFF);
            if (p == g_water_zbuf_param) zbuf = true;
            if (p == g_water_param[3]) flat = true;   // SamplerNormalMapFilter: XfFlatWater
        }
        if ((zbuf && settings.hide_water_depth) || (flat && settings.hide_water_flat) || (n == 1 && settings.hide_water_simple))
            hide = true;
        if (!g_hide_water_tex.empty()) {
            const uint32_t* first = slot - (entry - entries);
            for (uint32_t i = 0; i < n; ++i)
                if (g_hide_water_tex.count(first[i])) hide = true;
        }
        if (g_probe == 2 && entry == entries && g_probe_states.size() < 900) {
            const uint32_t* first = slot - (entry - entries);
            std::string line = "  water pass " + std::to_string(g_pass) + ", kind " + std::to_string(water_kind(dev, entries, n)) +
                               (hide ? " (hidden)" : "") + ":";
            for (uint32_t i = 0; i < n; ++i) {
                const auto wt = g_tex_paths.find(first[i]);
                line += " s" + std::to_string((entries[i] >> 16) & 0xFF) + "=" +
                        (!first[i] ? std::string("(none)") : wt != g_tex_paths.end() ? wt->second : std::string("(no path)"));
            }
            char buf[96] = "";
            if (g_cur_world && g_vp_transposed >= 0) {
                const float* w = reinterpret_cast<const float*>(g_cur_world);
                sprintf_s(buf, " | world at %.1f %.1f %.1f", g_vp_transposed == 1 ? w[3] : w[12], g_vp_transposed == 1 ? w[7] : w[13],
                          g_vp_transposed == 1 ? w[11] : w[14]);
            }
            IDirect3DVertexShader9* wvs = nullptr;
            IDirect3DPixelShader9* wps = nullptr;
            dev->GetVertexShader(&wvs);
            dev->GetPixelShader(&wps);
            if (wvs) wvs->Release();
            if (wps) wps->Release();
            dump_shader_pair(wvs, wps);
            char sh[64];
            sprintf_s(sh, " | vs %p ps %p", static_cast<void*>(wvs), static_cast<void*>(wps));
            line += std::string(buf) + sh + "\n";
            if (g_probe_states.insert(line).second) g_probe_text += line;
        }
    }
    if (entry == entries) {   // water whose vertex shader moves its vertices: its draws subdivided (water_draw)
        const int kind = (!hide && water) ? water_kind(dev, entries, n) : -1;
        g_subdiv_next = (settings.water_detail > 0 && (kind == 0 || kind == 1 || (kind == 2 && g_iw_replaced))) ? 1 : 0;
        g_iw_graded_next = (settings.water_detail > 0 && kind == 2 && g_iw_replaced && !graded::g_failed) ? 1 : 0;
        if (g_subdiv_next) {
            IDirect3DPixelShader9* ps = nullptr;
            dev->GetPixelShader(&ps);
            if (ps) ps->Release();
            g_subdiv_ps = ps;
        }
    }
    if (entry == entries) {   // XfWater with the stand-in: puddle or sea, per draw (puddle::on_draw)
        const int kind = (!hide && water && albedo_stage == 0xFF && settings.water_standin && settings.puddles)
                             ? water_kind(dev, entries, n) : -1;
        g_puddle_next = (kind == 0 || kind == 1) ? 1 : 0;
        if (g_puddle_next) {
            IDirect3DPixelShader9* ps = nullptr;
            dev->GetPixelShader(&ps);
            if (ps) ps->Release();
            puddle::g_ps = ps;
            puddle::g_sea_kind = kind;
        }
    }
    if (entry == entries) {   // the interactive water surface's maps (kIwVsSrc), or unbound again
        const bool iw = !hide && water && settings.water_ripples && g_iw_replaced && water_kind(dev, entries, n) == 2;
        iw_bind(dev, iw ? slot - (entry - entries) : nullptr, entries, n);
    }
    if (entry == entries) {   // WORLD = the draw's position (settings.world_positions)
        D3DMATRIX w{};
        w._11 = w._22 = w._33 = w._44 = 1.0f;
        if (settings.world_positions && g_world_this_draw && g_cur_world && g_vp_transposed >= 0) {
            const float* m = reinterpret_cast<const float*>(g_cur_world);
            if (g_vp_transposed == 1) { w._41 = m[3]; w._42 = m[7]; w._43 = m[11]; }
            else { w._41 = m[12]; w._42 = m[13]; w._43 = m[14]; }
            if (!std::isfinite(w._41) || !std::isfinite(w._42) || !std::isfinite(w._43)) w._41 = w._42 = w._43 = 0.0f;
        } else if (settings.world_positions && settings.skinned_positions && g_joints_this_draw && g_cur_joints) {
            // a skinned draw (no gXfWorld): its first bone's translation (row-major 3x4: the rows' w)
            const float* j = reinterpret_cast<const float*>(g_cur_joints);
            w._41 = j[3]; w._42 = j[7]; w._43 = j[11];
            if (!std::isfinite(w._41) || !std::isfinite(w._42) || !std::isfinite(w._43)) w._41 = w._42 = w._43 = 0.0f;
        }
        g_world_this_draw = false;
        g_joints_this_draw = false;
        if (std::memcmp(&w, &g_world_set, sizeof w) != 0) {
            dev->SetTransform(D3DTS_WORLD, &w);
            g_world_set = w;
        }
    }
    if (entry == entries && !hide && !g_first_traced) {
        g_first_traced = true;
        dev->GetTransform(D3DTS_VIEW, &g_first_view);
        dev->GetTransform(D3DTS_PROJECTION, &g_first_proj);
        g_first_valid = true;
        g_first_after_main = g_main_uploaded;
        g_first_pass = g_pass;
        g_first_kind = static_cast<uint32_t>(g_block_kind);
        if (g_probe == 2) {
            D3DMATRIX v{};
            dev->GetTransform(D3DTS_VIEW, &v);
            float pos[3];
            view_position(v, pos);
            char line[200];
            sprintf_s(line, "  first traced draw: pass %u, the main camera %s, D3D view at %.1f %.1f %.1f\n", g_pass,
                      g_main_uploaded ? "uploaded before it" : "NOT uploaded yet (last frame's)", pos[0], pos[1], pos[2]);
            g_probe_text += line;
        }
    }
    if (hide || (albedo_stage == 0xFF && water && settings.water_standin)) {
        if (entry == entries) {
            if (hide) { dev->SetTexture(0, hide_texture(dev)); ++g_hidden; }
            else { dev->SetTexture(0, standin(dev, water_kind(dev, entries, n))); ++g_water; }
        }
        const uint32_t param = *entry & 0xFFFF, stage = (*entry >> 16) & 0xFF;
        const uintptr_t res = *slot;
        cache[param] = 0xFFFFFFFF;
        IDirect3DBaseTexture9* tex = nullptr;
        if (res) {
            const uint32_t which = *reinterpret_cast<const uint8_t*>(res + 0x22) & 1 & *reinterpret_cast<const uint32_t*>(kCameraBit);
            tex = *reinterpret_cast<IDirect3DBaseTexture9* const*>(res + 0x10 + 4 * which);
        }
        dev->SetTexture(stage == 0 ? 15 : stage, tex);
        return 1;
    }
    if (albedo_stage == 0xFF) return 0;                 // no albedo map: the game binds as usual
    const uint32_t param = *entry & 0xFFFF, stage = (*entry >> 16) & 0xFF;
    const uintptr_t res = *slot;
    cache[param] = 0xFFFFFFFF;
    IDirect3DBaseTexture9* tex = nullptr;
    if (res) {
        const uint32_t which = *reinterpret_cast<const uint8_t*>(res + 0x22) & 1 & *reinterpret_cast<const uint32_t*>(kCameraBit);
        tex = *reinterpret_cast<IDirect3DBaseTexture9* const*>(res + 0x10 + 4 * which);
    }
    uint32_t to = stage;
    if (stage == albedo_stage) to = 0;
    else if (stage == 0) to = albedo_stage;
    if (to != stage) ++g_moved;
    dev->SetTexture(to, tex);
    return 1;
}

} // namespace

// ---- shader relabelling ----
// Remix's vertex capture (dxso_compiler.cpp, emitVertexCaptureInit / the capture write) takes a vertex's UV from the
// vertex shader's TEXCOORD0 output (.xy) and its colour from COLOR0. DMC4's effect shaders (XfPrim) write the vertex colour
// to TEXCOORD0 (`mul o1.xyz, r1.w, v1`, alpha in .w) and the UV to TEXCOORD1, so Remix drew every sprite with one texel
// of its texture (the lantern glows as white diamonds). D3D9 SM3 links vertex outputs to pixel inputs by these labels
// (dcl usage tokens), so relabelling both halves of a pass the same way leaves the game's own rendering as it was:
// TEXCOORD0 -> COLOR0 (or TEXCOORD1 when the shader already has a COLOR0 output) and TEXCOORD1 -> TEXCOORD0.
// A vertex shader qualifies when an instruction writes its COLOR input into its TEXCOORD0 output. Only while the bridge
// is on, and only for shaders created after it was turned on (they're created on first use).
// Normals: the capture takes a vertex's normal from the shader's NORMAL0 output, else from its NORMAL input
// (dxso_compiler.cpp m_vs.oNormal0, d3d9_rtx.cpp prepareVertexCapture). No DMC4 vertex shader outputs one, and the input
// is UBYTE4N 0..1 in the model's own space, which the shader decodes (mad r, v1, 2, -1) and turns with the world or bone
// matrices: Remix shaded every material draw with skewed, unskinned normals. Each material shader passes its world normal
// to the pixel shader in a TEXCOORD output; normal_output finds it by following which inputs each output component
// depends on (the NORMAL input and no POSITION / TANGENT / BINORMAL one, in .x .y .z of one register: all 149
// XfMaterialStandard vertex shaders, the 114 shadow receivers, 46 of XfMaterialBase's 87 with a normal input, whose
// others only output positions), and add_normal_output gives the shader a NORMAL0 output in a free register, written by
// copies (.xyz) of the instructions writing that one. The pixel shader is untouched (it doesn't read the new output). The
// bytecode grows, so the game gets a copy: it reads the pointer after this hook (0x8F49F4, CreateVertexShader).
namespace sm3 {
constexpr uint32_t kDcl = 0x1F, kComment = 0xFFFE, kEnd = 0x0000FFFF;
constexpr uint32_t kInput = 1, kOutput = 6, kTexcoord = 5, kColor = 10;   // D3DSPR_INPUT, D3DSPR_OUTPUT (vs_3_0)
uint32_t reg_type(uint32_t t) { return ((t >> 28) & 7) | ((t >> 8) & 0x18); }
uint32_t reg_num(uint32_t t) { return t & 0x7FF; }
uint32_t usage_token(uint32_t usage, uint32_t index) { return 0x80000000u | usage | (index << 16); }

struct Dcl { uint32_t* usage; uint32_t use, index, type, num; };

// walks a shader's tokens: its dcls and, for the others, calls f(opcode token pointer, length)
template <typename F>
bool walk(uint32_t* code, std::vector<Dcl>& dcls, F&& f) {
    uint32_t* p = code + 1;
    for (int guard = 0; guard < 65536; ++guard) {
        const uint32_t t = *p;
        if (t == kEnd) return true;
        if ((t & 0xFFFF) == kComment) { p += 1 + ((t >> 16) & 0x7FFF); continue; }
        const uint32_t len = (t >> 24) & 0xF;
        if ((t & 0xFFFF) == kDcl && len >= 2) {
            dcls.push_back({ p + 1, p[1] & 0x1F, (p[1] >> 16) & 0xF, reg_type(p[2]), reg_num(p[2]) });
        } else {
            f(p, len);
        }
        p += 1 + len;
    }
    return false;
}

const Dcl* find(const std::vector<Dcl>& d, uint32_t type, uint32_t use, uint32_t index) {
    for (auto& x : d) if (x.type == type && x.use == use && x.index == index) return &x;
    return nullptr;
}

// 0 = leave alone, 1 = TEXCOORD0 -> COLOR0, 2 = TEXCOORD0 -> TEXCOORD1; TEXCOORD1 -> TEXCOORD0 in both
int vertex_mode(uint32_t* code) {
    if ((code[0] & 0xFFFF0000u) != 0xFFFE0000u || (code[0] & 0xFFFF) < 0x0300) return 0;   // vs_3_0 only
    std::vector<Dcl> d;
    bool colour_in_t0 = false;
    std::vector<std::pair<uint32_t*, uint32_t>> ins;
    if (!walk(code, d, [&](uint32_t* p, uint32_t len) { ins.push_back({ p, len }); })) return 0;
    const Dcl* vcol = find(d, kInput, kColor, 0);
    const Dcl* t0 = find(d, kOutput, kTexcoord, 0);
    const Dcl* t1 = find(d, kOutput, kTexcoord, 1);
    if (!vcol || !t0 || !t1) return 0;
    for (auto& [p, len] : ins) {
        if (len < 2) continue;
        // only writes to .x / .y count: Remix reads TEXCOORD0.xy as the UV (XfMaterialStandard keeps its UV there and
        // vertex colour alpha in .z)
        if (reg_type(p[1]) != kOutput || reg_num(p[1]) != t0->num || !((p[1] >> 16) & 3)) continue;
        for (uint32_t k = 2; k <= len; ++k)
            if (reg_type(p[k]) == kInput && reg_num(p[k]) == vcol->num) colour_in_t0 = true;
    }
    if (colour_in_t0) return find(d, kOutput, kColor, 0) ? 2 : 1;
    // mode 3 | k << 4: TEXCOORD0.xy written by dp3 (a vector, not a UV), and output TEXCOORDk's .xy computed (mad / mul)
    // straight from the TEXCOORD0 input: swap TEXCOORD0 and TEXCOORDk
    const Dcl* vtex = find(d, kInput, kTexcoord, 0);
    if (!vtex || !t0) return 0;
    bool t0_vector = false;
    for (auto& [p, len] : ins)
        if (len >= 2 && (p[0] & 0xFFFF) == 8 /*dp3*/ && reg_type(p[1]) == kOutput && reg_num(p[1]) == t0->num &&
            ((p[1] >> 16) & 3)) t0_vector = true;
    if (!t0_vector) return 0;
    for (auto& [p, len] : ins) {
        const uint32_t op = p[0] & 0xFFFF;
        if (len < 3 || (op != 4 /*mad*/ && op != 5 /*mul*/) || reg_type(p[1]) != kOutput || ((p[1] >> 16) & 3) != 3) continue;
        bool from_uv = false;
        for (uint32_t k = 2; k <= len; ++k)
            if (reg_type(p[k]) == kInput && reg_num(p[k]) == vtex->num) from_uv = true;
        if (!from_uv) continue;
        for (auto& x : d)
            if (x.type == kOutput && x.use == kTexcoord && x.num == reg_num(p[1]) && x.index != 0 && x.index < 16)
                return 3 | static_cast<int>(x.index << 4);
    }
    return 0;
}

// relabels a shader's TEXCOORD0 / TEXCOORD1 dcls of register type `type` (vertex outputs or pixel inputs)
void relabel(uint32_t* code, uint32_t type, int mode) {
    std::vector<Dcl> d;
    if (!walk(code, d, [](uint32_t*, uint32_t) {})) return;
    const uint32_t k = (mode & 0xF) == 3 ? static_cast<uint32_t>(mode >> 4) : 1;
    uint32_t* t0 = nullptr; uint32_t* tk = nullptr;
    for (auto& x : d) {
        if (x.type != type || x.use != kTexcoord) continue;
        if (x.index == 0) t0 = x.usage;
        else if (x.index == k) tk = x.usage;
    }
    if (t0) *t0 = mode == 1 ? usage_token(kColor, 0) : usage_token(kTexcoord, k);
    if (tk) *tk = usage_token(kTexcoord, 0);
}

// ---- normals ----
constexpr uint32_t kNormalUsage = 3, kOutputs = 12;   // D3DDECLUSAGE_NORMAL; vs_3_0 has o0-o11
constexpr uint32_t kUsePosition = 1u << 0, kUseNormal = 1u << 3, kUseTangent = 1u << 6, kUseBinormal = 1u << 7;

// the components of source token `src` that instruction `op` reads to make destination component c (swizzle applied)
uint32_t read_comps(uint32_t op, uint32_t src, uint32_t c, uint32_t* out) {
    const auto sel = [src](uint32_t k) { return (src >> (16 + 2 * k)) & 3; };
    switch (op) {
    case 1: case 2: case 3: case 4: case 5: case 10: case 11: case 12: case 13: case 18: case 19: case 34: case 35: case 0x2E:
        out[0] = sel(c);                                    // component-wise: mov add sub mad mul min max slt sge lrp frc
        return 1;                                           // sgn abs mova
    case 6: case 7: case 14: case 15: case 32: case 37:
        out[0] = sel(0);                                    // scalar, replicate swizzle: rcp rsq exp log pow sincos
        return 1;
    case 8: case 33: case 0x24:
        for (uint32_t k = 0; k < 3; ++k) out[k] = sel(k);   // dp3 crs nrm
        return 3;
    case 9:
        for (uint32_t k = 0; k < 4; ++k) out[k] = sel(k);   // dp4
        return 4;
    default:
        for (uint32_t k = 0; k < 4; ++k) out[k] = k;        // anything else: all of it (the safe side)
        return 4;
    }
}

// the output register carrying the world normal, or -1 (none, more than one, or loops / calls / indexed outputs, which
// this doesn't follow). What each component depends on is a mask of D3DDECLUSAGE bits (bit 31: an undeclared input).
int normal_output(uint32_t* code) {
    if ((code[0] & 0xFFFF0000u) != 0xFFFE0000u || (code[0] & 0xFFFF) < 0x0300) return -1;   // vs_3_0 only
    std::vector<Dcl> d;
    std::vector<std::pair<uint32_t*, uint32_t>> ins;
    if (!walk(code, d, [&](uint32_t* p, uint32_t len) { ins.push_back({ p, len }); })) return -1;
    uint32_t in_use[16];
    for (auto& u : in_use) u = 1u << 31;
    bool has_normal = false;
    for (auto& x : d)
        if (x.type == kInput && x.num < 16) { in_use[x.num] = 1u << x.use; has_normal |= x.use == kNormalUsage; }
    if (!has_normal) return -1;
    uint32_t temp[32][4] = {}, out[kOutputs][4] = {};
    int depth = 0;
    for (auto& [p, len] : ins) {
        const uint32_t op = p[0] & 0xFFFF;
        switch (op) {
        case 0x28: case 0x29: ++depth; continue;            // if, ifc: writes inside add to what was there
        case 0x2A: continue;                                // else
        case 0x2B: --depth; continue;                       // endif
        case 0: case 0x2F: case 0x30: case 0x51: continue;  // nop defb defi def
        case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x26: case 0x27: case 0x2C: case 0x2D:
        case 0x61:
            return -1;                                      // call callnz loop ret endloop label rep endrep break breakc breakp
        }
        if (len < 1) continue;
        const uint32_t dst = p[1], mask = (dst >> 16) & 0xF;
        if (dst & 0x2000) return -1;                        // indexed destination
        const bool merge = depth > 0 || (p[0] & 0x10000000);   // conditional or predicated write
        uint32_t res[4] = {};
        for (uint32_t k = 2; k <= len; k += (p[k] & 0x2000) ? 2 : 1) {   // an indexed source is followed by its index
            const uint32_t s = p[k], t = reg_type(s), n = reg_num(s);
            for (uint32_t c = 0; c < 4; ++c) {
                if (!(mask >> c & 1)) continue;
                uint32_t cs[4];
                const uint32_t m = read_comps(op, s, c, cs);
                for (uint32_t i = 0; i < m; ++i) {
                    if (t == kInput && n < 16) res[c] |= in_use[n];
                    else if (t == 0 && n < 32) res[c] |= temp[n][cs[i]];
                }
            }
        }
        const uint32_t t = reg_type(dst), n = reg_num(dst);
        for (uint32_t c = 0; c < 4; ++c) {
            if (!(mask >> c & 1)) continue;
            if (t == 0 && n < 32) temp[n][c] = merge ? temp[n][c] | res[c] : res[c];
            else if (t == kOutput && n < kOutputs) out[n][c] |= res[c];
        }
    }
    int found = -1;
    for (uint32_t o = 0; o < kOutputs; ++o) {
        bool ok = true;
        for (uint32_t c = 0; c < 3; ++c)
            ok = ok && (out[o][c] & kUseNormal) && !(out[o][c] & (kUsePosition | kUseTangent | kUseBinormal));
        if (!ok) continue;
        if (found >= 0) return -1;                          // two: don't guess
        found = static_cast<int>(o);
    }
    return found;
}

// a copy of the shader with a NORMAL0 output in the first free output register, written by a copy (.xyz) of every
// instruction writing output `from`, right after it (so it reads the same values); empty when no register is free
std::vector<uint32_t> add_normal_output(uint32_t* code, uint32_t from) {
    std::vector<Dcl> d;
    if (!walk(code, d, [](uint32_t*, uint32_t) {})) return {};
    uint32_t used = 0;
    for (auto& x : d) if (x.type == kOutput && x.num < 32) used |= 1u << x.num;
    uint32_t k = 0;
    while (k < kOutputs && (used >> k & 1)) ++k;
    if (k == kOutputs || !(used >> from & 1)) return {};
    const auto retarget = [k](uint32_t tok, uint32_t mask) { return (tok & ~0x000F07FFu) | k | (mask << 16); };
    std::vector<uint32_t> out;
    const uint32_t* p = code;
    out.push_back(*p++);                                    // version
    for (int guard = 0; guard < 65536; ++guard) {
        const uint32_t t = *p;
        if (t == kEnd) { out.push_back(t); return out; }
        const uint32_t op = t & 0xFFFF;
        const uint32_t n = op == kComment ? 1 + ((t >> 16) & 0x7FFF) : 1 + ((t >> 24) & 0xF);
        out.insert(out.end(), p, p + n);
        if (op == kDcl && n >= 3 && reg_type(p[2]) == kOutput && reg_num(p[2]) == from) {
            out.push_back(t);
            out.push_back(usage_token(kNormalUsage, 0));
            out.push_back(retarget(p[2], 7));
        } else if (op != kComment && op != kDcl && n >= 2 && reg_type(p[1]) == kOutput && reg_num(p[1]) == from &&
                   ((p[1] >> 16) & 7)) {
            const size_t at = out.size();
            out.insert(out.end(), p, p + n);
            out[at + 1] = retarget(p[1], (p[1] >> 16) & 7);
        }
        p += n;
    }
    return {};
}
} // namespace sm3

std::unordered_map<const void*, int> g_vs_mode;      // vertex shader bytecode -> its relabelling
std::unordered_set<const void*> g_ps_done;            // pixel shader bytecode already relabelled
uint32_t g_relabelled = 0;
std::deque<std::vector<uint32_t>> g_vs_copies;        // vertex shaders with a NORMAL0 output (the game creates from these)
uint32_t g_normals = 0;

// pair = the pass's {pixel shader object, vertex shader object}, each {IDirect3D*Shader9* handle, bytecode}
void __stdcall patch_pair(uint32_t* pair) {
    if (!settings.on || !settings.relabel || !pair) return;
    auto* ps = reinterpret_cast<uint32_t*>(pair[0]);
    auto* vs = reinterpret_cast<uint32_t*>(pair[1]);
    if (!vs || !vs[1]) return;
    auto* vcode = reinterpret_cast<uint32_t*>(vs[1]);
    if (ps && ps[1]) {
        const auto* pcode = reinterpret_cast<const uint32_t*>(ps[1]);
        if (code_has(pcode, "XfWaterDeepColor") && code_has(pcode, "gXfViewProjI") &&
            std::find(g_water_b_ps.begin(), g_water_b_ps.end(), ps) == g_water_b_ps.end())
            g_water_b_ps.push_back(ps);
        // the interactive water surface (tXfInteractiveWaterGeometory, known by its pixel shader's constants): the game
        // creates its vertex shader from kIwVsSrc instead (same inputs / outputs, so the pixel shader links as before)
        if (!vs[0] && settings.water_ripples && vcode != g_iw_vs_code.data() &&
            code_has(pcode, "XfInteractiveWaterRefractScale") && iw_vs_code()) {
            vs[1] = reinterpret_cast<uint32_t>(g_iw_vs_code.data());
            g_vs_mode[g_iw_vs_code.data()] = 0;
            ++g_iw_replaced;
            return;
        }
    }
    int mode;
    auto it = g_vs_mode.find(vcode);
    if (it != g_vs_mode.end()) mode = it->second;
    else {
        mode = vs[0] ? 0 : sm3::vertex_mode(vcode);   // already created without us: leave its pixel shaders alone too
        // effects (passes 6-8) drawn after the injection are the game's own raster, not traced: relabelling them only risks
        // a pixel shader shared with a vertex shader the rule missed (black HUD / effect sheets); flat water (mode 3, pass 5)
        // is still traced
        if ((mode & 0xF) != 3 && settings.inject_pass > 0 && settings.inject_pass <= 6) mode = 0;
        if (mode) { sm3::relabel(vcode, sm3::kOutput, mode); ++g_relabelled; }
        g_vs_mode[vcode] = mode;
        // a NORMAL0 output (see "Normals" above): the game creates the shader from the copy
        const int nout = vs[0] ? -1 : sm3::normal_output(vcode);
        if (nout >= 0) {
            auto copy = sm3::add_normal_output(vcode, static_cast<uint32_t>(nout));
            if (!copy.empty()) {
                g_vs_copies.push_back(std::move(copy));
                uint32_t* ncode = g_vs_copies.back().data();
                vs[1] = reinterpret_cast<uint32_t>(ncode);
                g_vs_mode[ncode] = mode;
                ++g_normals;
            }
        }
    }
    if (!mode || !ps || ps[0] || !ps[1]) return;
    auto* pcode = reinterpret_cast<uint32_t*>(ps[1]);
    if (g_ps_done.insert(pcode).second) sm3::relabel(pcode, sm3::kInput, mode);
}

uintptr_t jmp_ret_shader = 0;
naked void detour_shader() {
    _asm {
        pushfd
        pushad
        push eax                     // the pass's shader pair
        call patch_pair              // __stdcall
        popad
        popfd
        mov ebx, [eax + 4]
        test ebx, ebx
        mov edi, [eax]
        jmp dword ptr [jmp_ret_shader]
    }
}

// ---- the frame's camera ----
// The view (sub_8E6830, a view's setup) keeps its matrices double-buffered per frame: block = view + parity * 0x40, gXfView
// at +0x270, gXfProj +0x2F0, gXfViewProj +0x370 (the parameter table gets pointers to them). The render thread draws a
// frame behind the game thread, so sCamera's matrices are the next frame's: geometry unprojected with them was a frame off
// (the player lagged behind). When the main scene uploads gXfViewProj (parameter index at 0xE55620) its pointer gives the
// View and Proj of the same block. The block may hold them transposed for the shaders: decided once against sCamera.
constexpr uintptr_t kViewProjParam = 0x00E55620;
uintptr_t g_last_vp = 0;
int       g_vp_transposed = -1;            // -1 not decided yet
std::atomic<uint32_t> g_vp_sets{ 0 };
uint32_t  g_vp_sets_last = 0;

void load_matrix(D3DMATRIX& m, const float* src, bool transposed) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) m.m[r][c] = transposed ? src[c * 4 + r] : src[r * 4 + c];
}

// the camera position of a row-vector view matrix (world -> view): -t * R^T
void view_position(const D3DMATRIX& v, float* out) {
    for (int i = 0; i < 3; ++i) out[i] = -(v.m[3][0] * v.m[i][0] + v.m[3][1] * v.m[i][1] + v.m[3][2] * v.m[i][2]);
}
std::atomic<uint32_t> g_vp_rejected{ 0 };
uint32_t g_vp_rejected_last = 0;

// The main view: the view block the most draws used last frame (blocks alternate between two buffers 0x40 apart, so this
// frame's is that pointer or its twin). Not sCamera's viewport 0: fixed-camera rooms draw through another camera, and
// every upload was turned away (the room went black). Orthographic (2D) views never count.
struct VpCount { uintptr_t vp; uint32_t n; };
VpCount   g_vpc[8];
int       g_nvpc = 0;
uintptr_t g_main_vp = 0;
uintptr_t g_last_copy = 0;   // the last copy block seen (the probe prints each once)

bool same_view(uintptr_t a, uintptr_t b) { return a == b || a + 0x40 == b || b + 0x40 == a; }

void count_view(uintptr_t vp) {
    for (int i = 0; i < g_nvpc; ++i) if (same_view(g_vpc[i].vp, vp)) { ++g_vpc[i].n; g_vpc[i].vp = vp; return; }
    if (g_nvpc < 8) g_vpc[g_nvpc++] = { vp, 1 };
}

// The view Remix traces: the gameplay viewport (0) while it's on and has a camera, else the first viewport that does (the
// free camera with Gameplay Cam off). Every view draws its own main scene, and Remix traces one camera: with a second
// one (photo mode / Free Camera leave a fullscreen free camera on viewport 1 when the gameplay camera comes back; co-op's
// split; extra cameras) the injection went into whichever view drew first and the one on screen stayed raster.
void pick_traced_view() {
    const uintptr_t camera = *reinterpret_cast<uintptr_t*>(kCameraInstance);
    if (!camera) return;
    const auto* cam = reinterpret_cast<const sCamera*>(camera);
    for (const auto& vp : cam->viewports)
        if (vp.mActive && vp.mpCamera) { g_main_view = vp.mSceneNo & 7u; return; }
}

void pick_main_view() {   // at each dispatch's start: last frame's most-drawn view
    uint32_t best = 0;
    for (int i = 0; i < g_nvpc; ++i) if (g_vpc[i].n > best) { best = g_vpc[i].n; g_main_vp = g_vpc[i].vp; }
    g_nvpc = 0;
}

// 4 x 4 inverse (Gauss-Jordan, doubles); false when singular
bool invert4(const D3DMATRIX& a, D3DMATRIX& out) {
    double m[4][8];
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 8; ++c) m[r][c] = c < 4 ? a.m[r][c] : (c - 4 == r ? 1.0 : 0.0);
    for (int c = 0; c < 4; ++c) {
        int piv = c;
        for (int r = c + 1; r < 4; ++r) if (std::fabs(m[r][c]) > std::fabs(m[piv][c])) piv = r;
        if (std::fabs(m[piv][c]) < 1e-12) return false;
        if (piv != c) for (int k = 0; k < 8; ++k) std::swap(m[c][k], m[piv][k]);
        const double d = m[c][c];
        for (int k = 0; k < 8; ++k) m[c][k] /= d;
        for (int r = 0; r < 4; ++r) {
            if (r == c) continue;
            const double f = m[r][c];
            if (f != 0.0) for (int k = 0; k < 8; ++k) m[r][k] -= f * m[c][k];
        }
    }
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) out.m[r][c] = static_cast<float>(m[r][c + 4]);
    return true;
}

// A draw under another view-projection block, with the main camera's view: its own projection P = V^-1 . VP (row
// vectors, VP = V . P). Fortuna's second block is the same camera with another projection for some objects (which ones
// changes from frame to frame: the bridge and a tree popped); Remix then unprojects them with that projection
bool own_projection(uintptr_t vp, D3DMATRIX& p) {
    if (!g_have_main || g_vp_transposed < 0) return false;
    D3DMATRIX m, inv;
    load_matrix(m, reinterpret_cast<const float*>(vp), g_vp_transposed == 1);
    if (!invert4(g_main_v, inv)) return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += inv.m[r][k] * m.m[k][c];
            if (!std::isfinite(s)) return false;
            p.m[r][c] = s;
        }
    return true;
}

// Another block holding the main camera's view-projection: the main block (or its twin, the other frame's) with the same
// matrix, else 0. Fortuna's shadow overlays, its sea and some stage objects (the bridge, a tree) draw with per-draw copies
// whose neighbours aren't a view and a projection, so they passed for another camera: hidden, the objects popped in and
// out as they moved between the copies and the main block from frame to frame
uintptr_t main_copy_of(uintptr_t vp) {
    const float* a = reinterpret_cast<const float*>(vp);
    for (const intptr_t d : { intptr_t(0), intptr_t(0x40), intptr_t(-0x40) }) {
        const uintptr_t c = g_main_vp + d;
        const float* b = reinterpret_cast<const float*>(c);
        bool same = true;
        for (int i = 0; i < 16 && same; ++i) same = std::fabs(a[i] - b[i]) <= 1e-4f * std::max(1.0f, std::fabs(b[i]));
        if (same) return c;
    }
    return 0;
}

void probe_camera(uintptr_t vp, const float* view, const float* proj, const char* what) {
    if (g_probe != 2) return;
    D3DMATRIX v;
    load_matrix(v, view, g_vp_transposed == 1);
    float pos[3];
    view_position(v, pos);
    const float* m = reinterpret_cast<const float*>(vp);
    float err = 0;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float s = 0;
            for (int k = 0; k < 4; ++k)
                s += g_vp_transposed == 1 ? proj[r * 4 + k] * view[k * 4 + c] : view[r * 4 + k] * proj[k * 4 + c];
            err += std::fabs(s - m[r * 4 + c]);
        }
    char buf[240];
    sprintf_s(buf, "  camera upload: block %04X (main %04X) at %.1f %.1f %.1f, proj _11 %.3f _22 %.3f _44 %.1f, view x proj "
                   "off by %.4f: %s\n",
              unsigned(vp & 0xFFFF), unsigned(g_main_vp & 0xFFFF), pos[0], pos[1], pos[2],
              proj[0], proj[5], proj[15], err, what);
    g_probe_text += buf;
    D3DMATRIX p;
    const bool other = g_main_vp && !same_view(vp, g_main_vp) && proj[15] != 1.0f;
    if (other && own_projection(vp, p)) {
        char row[400];
        sprintf_s(row, "    own projection (main view):  %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | %.4f %.4f "
                       "%.4f %.4f\n    main projection:              %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | %.4f %.4f %.4f %.4f | "
                       "%.4f %.4f %.4f %.4f\n",
                  p._11, p._12, p._13, p._14, p._21, p._22, p._23, p._24, p._31, p._32, p._33, p._34, p._41, p._42, p._43, p._44,
                  g_main_p._11, g_main_p._12, g_main_p._13, g_main_p._14, g_main_p._21, g_main_p._22, g_main_p._23, g_main_p._24,
                  g_main_p._31, g_main_p._32, g_main_p._33, g_main_p._34, g_main_p._41, g_main_p._42, g_main_p._43, g_main_p._44);
        g_probe_text += row;
    }
}

void __stdcall vs_array(IDirect3DDevice9* dev, const uint32_t* entry, const uintptr_t* value) {
    if (!settings.on || !g_in_main || !dev) return;
    if (g_world_param >= 0 && (*entry & 0xFFFF) == static_cast<uint32_t>(g_world_param)) {
        g_cur_world = *value;
        g_world_this_draw = true;
        return;
    }
    if ((*entry & 0xFFFF) != *reinterpret_cast<const uint32_t*>(kViewProjParam)) return;
    uintptr_t vp = *value;
    if (!vp) return;
    bool copy = false;
    if (g_main_vp && !same_view(vp, g_main_vp)) {
        if (const uintptr_t m = main_copy_of(vp)) {
            if (g_probe == 2 && vp != g_last_copy) {
                float diff = -1;
                if (g_have_frame_vp) {
                    diff = 0;
                    for (int i = 0; i < 16; ++i)
                        diff = std::max(diff, std::fabs(reinterpret_cast<const float*>(vp)[i] - g_frame_vp[i]));
                }
                char buf[200];
                sprintf_s(buf, "  camera upload: block %04X is a copy of the main camera (block %04X, %+d); off this frame's camera "
                               "by %.6f\n", unsigned(vp & 0xFFFF), unsigned(m & 0xFFFF), int(intptr_t(m) - intptr_t(g_main_vp)), diff);
                g_probe_text += buf;
            }
            g_last_copy = vp;
            vp = m;
            copy = true;
        }
    }
    const float* view = reinterpret_cast<const float*>(vp - 0x100);
    const float* proj = reinterpret_cast<const float*>(vp - 0x80);
    if (g_probe == 2 && vp != g_last_vp)
        probe_camera(vp, view, proj, proj[15] == 1.0f ? "orthographic, passed over"
                                     : (g_main_vp && !same_view(vp, g_main_vp)) ? "another view, passed over"
                                     : "used");
    g_other_cam = (proj[15] == 1.0f || (g_main_vp && !same_view(vp, g_main_vp))) ? vp : 0;
    g_block_kind = copy ? 1 : g_other_cam ? 2 : 0;
    if (!g_other_cam) g_main_uploaded = true;
    if (!g_other_cam && !copy && !g_have_frame_vp) {
        std::memcpy(g_frame_vp, reinterpret_cast<const float*>(vp), sizeof g_frame_vp);
        g_have_frame_vp = true;
    }
    g_cur_vp = proj[15] == 1.0f ? 0 : vp;
    g_other_ortho = proj[15] == 1.0f;
    if (proj[15] == 1.0f) { if (vp != g_last_vp) { g_last_vp = vp; ++g_vp_rejected; } return; }   // orthographic: 2D
    if (vp == g_last_vp) return;
    if (g_vp_transposed < 0) {
        // stored as D3D rows, ViewProj = View * Proj; stored transposed (for the shaders' dp4), ViewProj = Proj * View
        const float* m = reinterpret_cast<const float*>(vp);
        float e0 = 0, e1 = 0;
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c) {
                float s0 = 0, s1 = 0;
                for (int k = 0; k < 4; ++k) {
                    s0 += view[r * 4 + k] * proj[k * 4 + c];
                    s1 += proj[r * 4 + k] * view[k * 4 + c];
                }
                e0 += std::fabs(s0 - m[r * 4 + c]);
                e1 += std::fabs(s1 - m[r * 4 + c]);
            }
        g_vp_transposed = e1 < e0 ? 1 : 0;
    }
    g_last_vp = vp;
    if (g_main_vp && !same_view(vp, g_main_vp)) {   // another view-projection block
        ++g_vp_rejected;
        D3DMATRIX p;
        if (settings.other_cameras == 2 && own_projection(vp, p)) {
            dev->SetTransform(D3DTS_VIEW, &g_main_v);
            dev->SetTransform(D3DTS_PROJECTION, &p);
        }
        return;
    }
    D3DMATRIX v, p;
    load_matrix(v, view, g_vp_transposed == 1);
    load_matrix(p, proj, g_vp_transposed == 1);
    dev->SetTransform(D3DTS_VIEW, &v);
    dev->SetTransform(D3DTS_PROJECTION, &p);
    g_main_v = v;
    g_main_p = p;
    g_have_main = true;
    ++g_vp_sets;
}

uintptr_t jmp_ret_vsarr = 0;
naked void detour_vsarr() {
    _asm {
        pushad
        push edi                     // the binding's value (a pointer to the matrix)
        push ebx                     // the binding
        push esi                     // the device
        call vs_array                // __stdcall
        popad
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        jmp dword ptr [jmp_ret_vsarr]
    }
}

uintptr_t jmp_ret_tex = 0;
uintptr_t kTexBoundAddr = kTexBound;
volatile uint8_t g_remap = 0;   // the bridge's albedo move is on and the main scene is drawing: the detour's fast check

uintptr_t jmp_ret_draw = 0;
uintptr_t kDrawDoneAddr = 0x8F5EAB;   // dispatchCommands' loop after a command
naked void detour_draw() {
    _asm {
        cmp byte ptr [g_sky_next], 0
        jne hooked
        cmp byte ptr [g_terrain_next], 0
        jne hooked
        cmp byte ptr [g_subdiv_next], 0
        jne hooked
        cmp byte ptr [g_puddle_next], 0
        jne hooked
        cmp byte ptr [g_uvscroll_next], 0
        jne hooked
        cmp byte ptr [g_light_next], 0
        je original
    hooked:
        pushad
        mov eax, [esp + 0x20 + 0x50]    // the command (esp + 0x50 before pushad)
        push eax
        push esi                        // the device
        call water_draw                 // __stdcall
        test eax, eax
        jnz drawn
        popad
    original:
        mov eax, [esp + 0x50]
        mov ecx, [eax + 0x18]
        jmp dword ptr [jmp_ret_draw]
    drawn:
        popad
        jmp dword ptr [kDrawDoneAddr]
    }
}

naked void detour_tex() {
    _asm {
        cmp byte ptr [g_remap], 0
        je original
        pushad
        lea eax, [esp + 0x20 + 0x1A0]   // the game's per-parameter cache (esp + 0x1A0 before pushad)
        push eax
        push ecx                        // the pass record
        push edi                        // the entry's resource
        push ebx                        // the entry
        push esi                        // the device
        call bind_texture               // __stdcall
        test eax, eax
        jnz handled
        popad
    original:
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        jmp dword ptr [jmp_ret_tex]
    handled:
        popad
        jmp dword ptr [kTexBoundAddr]
    }
}

void* vsarr_detour() { return reinterpret_cast<void*>(&detour_vsarr); }
std::unique_ptr<FunctionHook>& vsarr_hook() { static std::unique_ptr<FunctionHook> h; return h; }

// the descriptor uploads (kVsDescs): value = a pointer to {count | flag, data}; gXfJointWorld's data are the draw's bones
void __stdcall vs_desc(IDirect3DDevice9* dev, const uint32_t* entry, const uintptr_t* value) {
    if (!settings.on || !g_in_main || !dev) return;
    if (g_joints_param < 0 || (*entry & 0xFFFF) != static_cast<uint32_t>(g_joints_param)) return;
    const uintptr_t d = *value;
    if (!d || !(*reinterpret_cast<const uint32_t*>(d) & 0x7FFFFFFF)) return;
    g_cur_joints = *reinterpret_cast<const uintptr_t*>(d + 4);
    g_joints_this_draw = g_cur_joints != 0;
}

uintptr_t jmp_ret_vsdesc = 0;
naked void detour_vsdesc() {
    _asm {
        pushad
        push edi                     // the binding's value (a pointer to the descriptor)
        push ebx                     // the binding
        push esi                     // the device
        call vs_desc                 // __stdcall
        popad
        mov edx, [ebx]
        mov eax, [edi]
        and edx, 0xFFFF
        jmp dword ptr [jmp_ret_vsdesc]
    }
}
void* vsdesc_detour() { return reinterpret_cast<void*>(&detour_vsdesc); }
std::unique_ptr<FunctionHook>& vsdesc_hook() { static std::unique_ptr<FunctionHook> h; return h; }

void* shader_detour() { return reinterpret_cast<void*>(&detour_shader); }
std::unique_ptr<FunctionHook>& shader_hook() { static std::unique_ptr<FunctionHook> h; return h; }

void* tex_detour() { return reinterpret_cast<void*>(&detour_tex); }
void* draw_detour() { return reinterpret_cast<void*>(&detour_draw); }

// dispatchCommands' DrawPrimitive (0x8F5726, same 7 bytes as the indexed draw's: the command at esp + 0x50, the device in
// esi): only sky and terrain draws are taken (prim_draw)
uintptr_t jmp_ret_drawprim = 0;
naked void detour_drawprim() {
    _asm {
        cmp byte ptr [g_sky_next], 0
        jne hooked
        cmp byte ptr [g_uvscroll_next], 0
        jne hooked
        cmp byte ptr [g_terrain_next], 0
        je original
    hooked:
        pushad
        mov eax, [esp + 0x20 + 0x50]
        push eax
        push esi
        call prim_draw
        test eax, eax
        jnz drawn
        popad
    original:
        mov eax, [esp + 0x50]
        mov ecx, [eax + 0x18]
        jmp dword ptr [jmp_ret_drawprim]
    drawn:
        popad
        jmp dword ptr [kDrawDoneAddr]
    }
}
void* drawprim_detour() { return reinterpret_cast<void*>(&detour_drawprim); }
std::unique_ptr<FunctionHook>& drawprim_hook() { static std::unique_ptr<FunctionHook> h; return h; }
std::unique_ptr<FunctionHook>& draw_hook() { static std::unique_ptr<FunctionHook> h; return h; }

// Stage colour grading (uColorCorrectFilter, a pass-9 filter, priority 26000 by default; Fortuna has one, the BP stages
// don't): with tone curves set, it draws them into a 256 x 1 ramp in its own sub-scene ("ToneMapGenerate", sub_93E4D0), one
// draw per curve with the colour writes masked to R, G or B, then maps the frame through the ramp. Remix ignores every draw
// whose RGB writes aren't all on until its image is in (d3d9_rtx.cpp makeDrawCallType, "colour write disabled"), and the
// sub-scene always runs before the main scene, so under Remix the ramp stays black and the grading turned the whole frame
// black whenever the filters run after the hand-over (remix_probe.txt, 2026-11-24: the scene 0.62 before the grading draw,
// 0 after). While the bridge runs under Remix the curve check (sub_91B200, a compare with the default curve; only the
// grading calls it) answers "default": no ramp, the grading's colour matrix still applies.
constexpr uintptr_t kCurveIsCustom = 0x91B200;
std::unique_ptr<Patch> g_curve_patch;

void update_curve_patch() {
    const bool want = settings.on && loaded();
    if (want && !g_curve_patch) g_curve_patch = Patch::create(kCurveIsCustom, { 0x31, 0xC0, 0xC3 }, true);   // xor eax, eax; ret
    else if (!want && g_curve_patch) g_curve_patch.reset();   // the destructor puts the bytes back
}


// ---- no culling in the traced view ----
// Every model's draw (uModel::draw and the uDevil4Model / stage variants) asks uModel::frustumCullAndLOD (0x9EDBB0,
// __usercall: edx = the model, the view's cTrans on the stack, ret 4) whether its bounding sphere / box is in the view's
// six planes: -1 = culled, 63 = inside all of them, else a mask of the planes it's inside, for which
// uModel::cullPrimitivesAgainstFrustum then culls the model's parts one by one (63 = draw them all). DMC4 has no occlusion
// culling of models (pass 4's queries are the light halos'). With settings.draw_all, the traced view's main scene gets 63
// for every model: the key being built (cTrans +0x2A90 -> +0x234) says which view / scene the draw is for. Other views
// (shadow maps, reflections) keep their culling. Remix's anti-culling only keeps what it has seen, so this is the fix
// for lighting that changed with the view direction.
uintptr_t jmp_ret_cull = 0;
volatile uint8_t g_uncull = 0;
std::atomic<uint32_t> g_unculled{ 0 };   // models drawn this frame that the game would have culled
uint32_t g_unculled_last = 0;

int __stdcall cull_decide(uintptr_t ctx, int result) {
    if (result == 63 || !ctx) return result;
    const uintptr_t scene_ctx = *reinterpret_cast<const uintptr_t*>(ctx + 0x2A90);
    if (!scene_ctx) return result;
    const uint32_t key = *reinterpret_cast<const uint32_t*>(scene_ctx + 0x234);
    if (((key >> 24) & 0x1F) != kMainScene || (key >> 29) != g_main_view) return result;
    if (result < 0) ++g_unculled;
    return 63;
}

// the game's function as it is (its first 6 bytes, then the rest of it)
naked void cull_original() {
    _asm {
        push ebp
        mov ebp, esp
        and esp, 0FFFFFFF0h
        jmp dword ptr [jmp_ret_cull]
    }
}

naked void detour_cull() {
    _asm {
        cmp byte ptr [g_uncull], 0
        je original
        push dword ptr [esp + 4]     // the cTrans (edx = the model, untouched)
        call cull_original           // eax = the game's answer; it pops its argument
        push eax
        push dword ptr [esp + 8]     // the cTrans again: [esp] eax, [esp + 4] our return address, [esp + 8] cTrans
        call cull_decide             // __stdcall (cTrans, answer)
        ret 4
    original:
        push ebp
        mov ebp, esp
        and esp, 0FFFFFFF0h
        jmp dword ptr [jmp_ret_cull]
    }
}

void* cull_detour() { return reinterpret_cast<void*>(&detour_cull); }
std::unique_ptr<FunctionHook>& cull_hook() { static std::unique_ptr<FunctionHook> h; return h; }

void set_sun(const float* dir) {
    update_curve_patch();
    g_uncull = (settings.on && settings.draw_all && loaded()) ? 1 : 0;
    for (int i = 0; i < 3; ++i) g_sun_dir[i] = dir[i];
    ++g_presents;
    if (g_disp_since_present > g_disp_per_present_max) g_disp_per_present_max = g_disp_since_present;
    g_disp_since_present = 0;
}

void on_reset() {
    if (g_hide) { g_hide->Release(); g_hide = nullptr; }
    if (g_lin_copy) { g_lin_copy->Release(); g_lin_copy = nullptr; }
}

bool loaded() {
    static int cached = -1;
    if (cached < 0) {
        cached = 0;
        HMODULE d3d9 = GetModuleHandleA("d3d9.dll");
        // the bridge client and the runtime both export the Remix API's entry point
        if (d3d9 && (GetProcAddress(d3d9, "remixapi_InitializeLibrary") || GetProcAddress(d3d9, "remixapi_initializeLibrary")))
            cached = 1;
        if (!cached && d3d9) {
            char path[MAX_PATH] = {};
            GetModuleFileNameA(d3d9, path, MAX_PATH);
            std::string p(path);
            for (auto& c : p) c = (char)tolower((unsigned char)c);
            if (p.find("system32") == std::string::npos && p.find("syswow64") == std::string::npos) cached = 1;
        }
    }
    return cached == 1;
}

uint32_t g_sim_scenes[8];   // the frame's interactive water simulation sub-scenes (view + scene bits of their keys)
int g_nsim = 0;

// The game's depth texture (sRender +0x1700C, a cTrans texture, its IDirect3DTexture9 at +0x10: the A8R8G8B8 target the
// depth prepass packs depth into). With the prepass skipped (skip_zpass) nothing writes it, so it stays black and the
// depth-based filters of pass 8 do nothing. A frame drawn without the bridge (switched off and on, rarely otherwise)
// left its depth there, and those filters kept using that old frame's depth: a ghost fixed on the screen until a
// fullscreen switch made the targets again (2026-11-25; found with the probe's input thumbnails: the same pass-8 draws
// read it black when clean, an old depth when ghosted; its pointer matched). So it is cleared to black once a frame,
// where the prepass would have written it.
constexpr uintptr_t kRenderInstance = 0x00E552D8;   // sRender::mpInstance
uint32_t g_depth_cleared = ~0u;

void clear_depth_texture(IDirect3DDevice9* dev) {
    const uintptr_t render = *reinterpret_cast<const uintptr_t*>(kRenderInstance);
    if (!render) return;
    const uintptr_t obj = *reinterpret_cast<const uintptr_t*>(render + 0x1700C);
    if (!obj) return;
    auto* tex = *reinterpret_cast<IDirect3DTexture9* const*>(obj + 0x10);
    if (!tex) return;
    IDirect3DSurface9* s = nullptr;
    if (SUCCEEDED(tex->GetSurfaceLevel(0, &s)) && s) {
        dev->ColorFill(s, nullptr, 0);
        s->Release();
    }
}

bool on_tag(void* device, const uint32_t* list, uint32_t index, uint32_t count) {
    // the final pass without a curve while Remix's (already tone-mapped) image is handed over at half scale
    tone_map::passthrough = settings.on && settings.inject_pass > 0 && settings.linearize;
    if (!settings.on) { g_remap = 0; return false; }
    auto* dev = static_cast<IDirect3DDevice9*>(device);
    const uint32_t key = list[index * 2 + 1];
    const uint32_t scene = (key >> 24) & 0x1F, pass = (key >> 20) & 0xF;
    if (index == 0) pick_traced_view();
    const bool main = scene == kMainScene && (key >> 29) == g_main_view;
    if (index == 0) {
        g_nsim = 0;
        subdiv::g_drawn_last = subdiv::g_drawn;
        subdiv::g_drawn = 0;
        if (g_frames % 600 == 0)
            for (auto it = subdiv::g_cache.begin(); it != subdiv::g_cache.end();)
                if (g_frames - it->second.last > 1800) { subdiv::release(it->second); it = subdiv::g_cache.erase(it); }
                else ++it;
        g_skipped_last = g_skipped.exchange(0);
        g_unculled_last = g_unculled.exchange(0);
        g_other_draws_last = g_other_draws.exchange(0);
        g_seen_last = g_seen.exchange(0);
        g_moved_last = g_moved.exchange(0);
        g_water_last = g_water.exchange(0);
        puddle::g_puddles_last = puddle::g_puddles.exchange(0);
        puddle::g_seas_last = puddle::g_seas.exchange(0);
        for (int i = 0; i < kWaterCount; ++i)
            if (g_water_param[i] < 0) g_water_param[i] = param_of(kWaterIds[i]);
        if (g_shadow_param < 0) g_shadow_param = param_of(kShadowNearId);
        if (g_world_param < 0) g_world_param = param_of(0xAE179D10);   // gXfWorld
        if (g_joints_param < 0) g_joints_param = param_of(0x0EB7060F);   // gXfJointWorld
        if (g_prim_depth_param < 0) g_prim_depth_param = param_of(kPrimDepthId);
        if (g_water_zbuf_param < 0) g_water_zbuf_param = param_of(kWaterZBufferId);
        if (g_prim_scene_param < 0) g_prim_scene_param = param_of(kPrimSceneId);
        if (g_base_param < 0) g_base_param = param_of(kBaseSamplerId);
        g_sky_last = g_sky.exchange(0);
        g_hidden_last = g_hidden.exchange(0);
        for (int i = 0; i < 3; ++i) {
            if (g_prim_param[i] < 0) g_prim_param[i] = param_of(kPrimBaseIds[i]);
        }
        ++g_frames;
        if (strncmp(settings.hide_textures, g_hide_built_for, sizeof g_hide_built_for - 1) != 0 ||
            strncmp(settings.hide_water_textures, g_hide_water_built_for, sizeof g_hide_water_built_for - 1) != 0 ||
            strncmp(settings.light_textures, g_light_built_for, sizeof g_light_built_for - 1) != 0 ||
            strncmp(settings.sky_textures, g_sky_built_for, sizeof g_sky_built_for - 1) != 0 ||
            strncmp(settings.terrain_textures, g_terrain_built_for, sizeof g_terrain_built_for - 1) != 0 ||
            ((g_hide_built_for[0] || g_hide_water_built_for[0] || g_light_built_for[0] || g_sky_built_for[0] ||
              g_terrain_built_for[0]) &&
             g_frames % 60 == 0))
            build_hide_textures();
        g_in_main = false;
        g_last_vp = 0;
        pick_main_view();
        g_vp_sets_last = g_vp_sets.exchange(0);
        g_terrain_draws_last = g_terrain_draws.exchange(0);
        g_vp_rejected_last = g_vp_rejected.exchange(0);
        ++g_disp_since_present;
        g_ui_done = false;
        g_solid_seen = false;
        probe_frame_start(dev);
        if (g_frames % 3600 == 1 && g_norder > 0) {
            FILE* f = nullptr;
            if (fopen_s(&f, "remix_passes.txt", "a") == 0 && f) {
                fprintf(f, "tag order (view.scene.pass x tags):");
                for (int i = 0; i < g_norder; ++i)
                    fprintf(f, " %u.%02X.%u%s{rt %04X %ux%u f%u}", (g_order[i] >> 9) & 7, (g_order[i] >> 4) & 0x1F,
                            g_order[i] & 0xF, (g_order[i] & 0x8000) ? "[bb]" : "", unsigned(g_order_rt[i] & 0xFFFF),
                            g_order_desc[i] & 0xFFF, (g_order_desc[i] >> 12) & 0xFFF, g_order_desc[i] >> 24);
                fprintf(f, " | presents %u, most dispatches between two presents %u, ui trigger at pass %u rt %04X",
                        g_presents.load(), g_disp_per_present_max, g_ui_fired_pass, unsigned(g_ui_fired_rt & 0xFFFF));
                fprintf(f, "\n");
                fclose(f);
            }
        }
        g_norder = 0;
        if (settings.game_lights) gather_lights();
        set_transforms(dev);
        api_frame();
    }
    ++g_seen;
    if (main && !g_in_main) {           // the main scene starts: its camera again (another view may have set its own)
        g_in_main = true;
        g_other_cam = 0;
        g_block_kind = 0;
        g_cur_vp = 0;
        g_last_vp = 0;
        ++g_watch_frame;
        if (g_first_valid && g_have_main && g_watch_lines < 400) {
            auto diff = [](const D3DMATRIX& a, const D3DMATRIX& b, bool rot) {
                float d = 0;
                for (int r = 0; r < (rot ? 3 : 4); ++r)
                    for (int c = 0; c < (rot ? 3 : 4); ++c) d = std::max(d, std::fabs(a.m[r][c] - b.m[r][c]));
                return d;
            };
            float pf[3], pm[3], pp[3] = { 0, 0, 0 };
            view_position(g_first_view, pf);
            view_position(g_main_v, pm);
            if (g_prev_main_valid) view_position(g_prev_main_view, pp);
            const float first_off = std::sqrt((pf[0] - pm[0]) * (pf[0] - pm[0]) + (pf[1] - pm[1]) * (pf[1] - pm[1]) +
                                              (pf[2] - pm[2]) * (pf[2] - pm[2]));
            const float first_rot = diff(g_first_view, g_main_v, true), first_proj = diff(g_first_proj, g_main_p, false);
            const float jump = g_prev_main_valid ? std::sqrt((pm[0] - pp[0]) * (pm[0] - pp[0]) + (pm[1] - pp[1]) * (pm[1] - pp[1]) +
                                                             (pm[2] - pp[2]) * (pm[2] - pp[2])) : 0.0f;
            const float jump_rot = g_prev_main_valid ? diff(g_main_v, g_prev_main_view, true) : 0.0f;
            const bool wrong_first = first_off > 0.5f || first_rot > 0.002f || first_proj > 0.002f;
            const bool jumped = jump > 20.0f || jump_rot > 0.02f;
            if (wrong_first || jumped) {
                FILE* f = nullptr;
                if (fopen_s(&f, "remix_probe.txt", "a") == 0 && f) {
                    fprintf(f, "camera watch, frame %u: Remix's camera (first traced draw, pass %u, block kind %u, main camera %s) "
                               "off the frame's main camera by %.2f cm, rotation %.4f, projection %.4f; main camera moved %.2f cm, "
                               "rotation %.4f since the last frame\n",
                            g_watch_frame, g_first_pass, g_first_kind, g_first_after_main ? "uploaded before it" : "not uploaded yet",
                            first_off, first_rot, first_proj, jump, jump_rot);
                    fclose(f);
                    ++g_watch_lines;
                }
            }
        }
        if (g_have_main) { g_prev_main_view = g_main_v; g_prev_main_valid = true; }
        g_first_valid = false;
        g_main_uploaded = false;
        g_first_traced = false;
        g_have_frame_vp = false;
        g_cur_world = 0;
        g_cur_joints = 0;
        if (g_probe == 2 && g_main_vp && g_vp_transposed >= 0) {   // what the main blocks hold now, before any upload
            std::string s = "  main scene starts:";
            for (const intptr_t d : { intptr_t(0), intptr_t(0x40), intptr_t(-0x40) }) {
                D3DMATRIX v;
                load_matrix(v, reinterpret_cast<const float*>(g_main_vp + d - 0x100), g_vp_transposed == 1);
                float pos[3];
                view_position(v, pos);
                char b[96];
                sprintf_s(b, " block %04X camera at %.1f %.1f %.1f;", unsigned((g_main_vp + d) & 0xFFFF), pos[0], pos[1], pos[2]);
                s += b;
            }
            float pos[3];
            view_position(g_main_v, pos);
            char b[96];
            sprintf_s(b, " last frame's camera at %.1f %.1f %.1f\n", pos[0], pos[1], pos[2]);
            g_probe_text += s + b;
        }
        set_transforms(dev);
    } else if (!main) {
        g_in_main = false;
    }
    // after the injection the game's own shaders draw: no stage moves, stand-ins or hidden draws there
    const bool after_inject = settings.inject_pass > 0 && pass >= static_cast<uint32_t>(settings.inject_pass);
    g_remap = (main && settings.albedo_to_stage0 && !after_inject) ? 1 : 0;
    if (!g_remap) g_light_next = 0;   // the texture hook doesn't run there: no light flag left for a later draw
    if (!g_remap) g_subdiv_next = 0;   // water subdivision only where the bridge binds (the traced main scene)
    if (!g_remap) g_puddle_next = 0;
    if (g_sky_vp && (pass != g_pass || !main)) sky_viewport(dev, false);   // the sky stays within SOLID
    g_pass = pass;
    bool skip = false;
    // the interactive water's simulation (uInteractiveWater::draw: sub-scenes opened by a debug marker, command 16 with
    // the name at +0xC, then quads into its maps; Remix only rasterises draws into other targets) keeps running
    if (!main && settings.water_ripples) {
        const auto* cmd = reinterpret_cast<const uint32_t*>(list[index * 2]);
        const char* name = (cmd && cmd[0] == 16 && cmd[3]) ? reinterpret_cast<const char*>(cmd[3]) : nullptr;
        if (name && (strcmp(name, "IWaterInit") == 0 || strcmp(name, "IWaterUpdate") == 0) && g_nsim < 8)
            g_sim_scenes[g_nsim++] = key >> 24;
    }
    bool sim = false;
    for (int i = 0; i < g_nsim; ++i) sim |= g_sim_scenes[i] == (key >> 24);
    if (!main && settings.skip_other_scenes && !sim) skip = true;
    if (main && pass == kZPass && settings.skip_zpass) skip = true;
    if (skip && main && pass == kZPass && settings.clear_depth && g_depth_cleared != g_frames.load()) {
        clear_depth_texture(dev);   // stale depth would feed the pass-8 filters (see clear_depth_texture)
        g_depth_cleared = g_frames.load();
    }
    // the filters tone-map the HDR scene buffer into the target the HUD and the end of the frame draw on: needed once the
    // traced image goes into that buffer
    if (main && pass == kFilter && settings.skip_filters && !after_inject) skip = true;
    if (main && (pass == kScreen || pass == kEnd) && settings.skip_hud) skip = true;
    if (g_frames % 3600 == 0) {
        const uint32_t k = ((key >> 29) << 9) | (scene << 4) | pass;
        if (g_norder == 0 || (g_order[g_norder - 1] & 0x7FFF) != k) {
            uint32_t bb = 0, desc = 0;
            uintptr_t rtp = 0;
            IDirect3DSurface9 *rt = nullptr, *back = nullptr;
            if (SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) {
                rtp = reinterpret_cast<uintptr_t>(rt);
                D3DSURFACE_DESC d{};
                if (SUCCEEDED(rt->GetDesc(&d))) desc = (d.Width & 0xFFF) | ((d.Height & 0xFFF) << 12) | ((uint32_t(d.Format) & 0xFF) << 24);
                if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) && back) {
                    bb = rt == back ? 0x8000u : 0;
                    back->Release();
                }
                rt->Release();
            }
            if (g_norder < 256) { g_order_rt[g_norder] = rtp; g_order_desc[g_norder] = desc; g_order[g_norder++] = k | bb; }
        }
    }
    if (g_probe == 2 && main && g_ui_done && pass >= 8 && g_probe_tags < 120) {   // the filters: what each tag left
        ++g_probe_tags;
        g_probe_pass = pass;
        char label[128];
        // a filter starts with a debug marker (command 16, its name at +0xC: "ColorCorrectFilter", ...)
        const auto* cmd = reinterpret_cast<const uint32_t*>(list[index * 2]);
        const char* marker = (cmd && cmd[0] == 16 && cmd[3]) ? reinterpret_cast<const char*>(cmd[3]) : "";
        sprintf_s(label, "tag %u (pass %u, key %08X)%s %.40s", index, pass, key, skip ? " skipped" : "", marker);
        probe(dev, label);
        // the render-target textures the draw before this tag read (still bound): each probed and its 64 x 36 grid saved
        // as remix_probe_inputs\t<tag>_s<stage>.ppm, to find a post effect reading a stale frame (the "ghost", 2026-11-25)
        CreateDirectoryA("remix_probe_inputs", nullptr);
        for (DWORD s = 0; s < 8; ++s) {
            IDirect3DBaseTexture9* bt = nullptr;
            if (FAILED(dev->GetTexture(s, &bt)) || !bt) continue;
            if (bt->GetType() == D3DRTYPE_TEXTURE) {
                IDirect3DSurface9* lvl = nullptr;
                D3DSURFACE_DESC td{};
                if (SUCCEEDED(static_cast<IDirect3DTexture9*>(bt)->GetSurfaceLevel(0, &lvl)) && lvl) {
                    lvl->GetDesc(&td);
                    if (td.Usage & D3DUSAGE_RENDERTARGET) {
                        char in_label[96], path[96];
                        sprintf_s(in_label, "    before it, s%lu read tex %04X", s, unsigned(reinterpret_cast<uintptr_t>(bt) & 0xFFFF));
                        sprintf_s(path, "remix_probe_inputs\\t%u_s%lu.ppm", index, s);
                        probe(dev, in_label, lvl, path);
                    }
                    lvl->Release();
                }
            }
            bt->Release();
        }
        IDirect3DSurface9* rt = nullptr;
        if (g_probe_scene && SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) {
            if (rt != g_probe_scene) probe(dev, "  the trigger's target", g_probe_scene);
            rt->Release();
        }
    } else if (g_probe == 2 && main && pass != g_probe_pass) {
        g_probe_pass = pass;
        char label[64];
        sprintf_s(label, "main pass %u starts%s", pass, skip ? " (skipped)" : "");
        probe(dev, label);
    }
    if (main && pass == 2) g_solid_seen = true;
    if (main && !skip && after_inject && !g_ui_done && g_solid_seen) {
        g_ui_done = true;
        water_transform(dev, false);
        uv_scroll_off(dev);
        g_uvscroll_next = 0;
        sky_viewport(dev, false);
        g_ui_fired_pass = pass;
        IDirect3DSurface9* rt = nullptr;
        if (SUCCEEDED(dev->GetRenderTarget(0, &rt)) && rt) { g_ui_fired_rt = reinterpret_cast<uintptr_t>(rt); rt->Release(); }
        if (g_probe == 2) {
            probe_state(dev);
            if (!g_probe_scene) dev->GetRenderTarget(0, &g_probe_scene);
        }
        trigger_ui(dev);
        if (g_probe == 2) probe(dev, "after the trigger");
        linearize(dev);
        if (g_probe == 2) probe(dev, "after the hand-over");
    }
    if (skip) ++g_skipped;
    return skip;
}

void draw_gui() {
    ImGui::Checkbox(_("RTX Remix bridge"), &settings.on);
    ImGui::SameLine();
    Mod::help_marker(_("For playing through NVIDIA RTX Remix (its d3d9.dll in the game folder): gives Remix the game's camera "
                       "every frame, which DMC4's shaders keep to themselves, and leaves out the draws Remix would show as "
                       "duplicates or garbage. Without Remix it does nothing useful: leave it off."));
    if (!settings.on) return;
    ImGui::Indent(Mod::lineIndent);
    ImGui::PushID("PostFX_Remix");
    ImGui::TextDisabled(loaded() ? _("Remix's d3d9.dll is loaded") : _("The system's d3d9.dll is loaded (no Remix)"));
    ImGui::Checkbox(_("Skip other views (shadow maps, reflections)"), &settings.skip_other_scenes);
    ImGui::SameLine();
    Mod::help_marker(_("Also every camera but the one in use: Remix traces one camera, so extra cameras (Free Camera's "
                       "split views, co-op's split screen) stay empty while the bridge is on."));
    ImGui::Checkbox(_("Draw what's off screen too"), &settings.draw_all);
    ImGui::SameLine();
    Mod::help_marker(_("DMC4 leaves out every model outside the camera's view. Remix only knows what the game draws, so "
                       "shadows, bounced light and reflections from things behind the camera came and went as it turned. "
                       "With this on, the game draws everything in the room for Remix (more work for the CPU and for "
                       "Remix). Only the view Remix traces; shadow maps and reflections keep culling."));
    {
        static const char* kOther[] = { "Placed with the main camera", "Hidden from Remix", "Placed with their own projection" };
        ImGui::Combo(_("Draws with another projection"), &settings.other_cameras, kOther, 3);
        ImGui::SameLine();
        Mod::help_marker(_("Some stages draw part of the scene with a second projection (Fortuna does, for objects that "
                           "change from frame to frame). Placed with the main camera, Remix put them at the wrong distance "
                           "(floating slabs, shards); hidden, they popped in and out. Own projection hands Remix the one "
                           "they were drawn with."));
    }
    ImGui::Checkbox(_("Scrolling textures"), &settings.uv_scroll);
    ImGui::SameLine();
    Mod::help_marker(_("Textures the game scrolls (rivers, flowing water, light running over Dante's Pandora) scroll under "
                       "Remix too. Remix doesn't follow the game's own scrolling, so the bridge hands it each draw's "
                       "offset."));
    ImGui::Checkbox(_("Interactive water ripples (restart)"), &settings.water_ripples);
    ImGui::SameLine();
    Mod::help_marker(_("The practice stage's and the Bloody Palace's water floors simulate their ripples on the graphics card "
                       "and draw them only as shading on a flat surface, which Remix can't see. With this on, the simulation "
                       "keeps running under the bridge and the surface is lifted by it, so Remix traces real ripples. Takes "
                       "effect for stages loaded after a restart with the bridge on."));
    if (settings.water_ripples) {
        ImGui::SliderFloat(_("Ripple height"), &settings.ripple_height, 0.0f, 100.0f, "%.1f cm");
        ImGui::SameLine();
        Mod::help_marker(_("How far the surface rises at the simulation's full amplitude, per unit of the water's Y scale "
                           "(the game sets the simulation's amplitude from it, so taller water makes taller ripples)."));
        ImGui::SliderFloat(_("Ripple waviness"), &settings.ripple_normal, 0.0f, 8.0f, "%.1f");
        ImGui::SameLine();
        Mod::help_marker(_("How much the ripples bend what the water reflects and shows through it, without making the "
                           "surface itself any taller. 0 = a flat mirror, 1 = calm, 4 and up = as busy as the game's own "
                           "water, with hard-edged patches close to the camera. Changes right away."));
        ImGui::Checkbox(_("Water to the horizon"), &settings.water_to_horizon);
        ImGui::SameLine();
        Mod::help_marker(_("Large water floors (100 m across or more, like the Bloody Palace's and the practice stage's) go "
                           "on as flat water to the horizon, so there is no edge where the water ends against the sky. Only "
                           "the picture: the water you can stand in is unchanged."));
        ImGui::TextDisabled(_("Water surfaces with ripples: %u"), g_iw_replaced);
        if (g_iw_nunits > 0) {
            const uintptr_t u = g_iw_units[0];
            const float* sc = reinterpret_cast<const float*>(u + 0x40);
            ImGui::TextDisabled(_("Interactive water: scale %.2f %.2f %.2f, spring %.3f, damping %.3f, rain %u a frame, "
                                  "edge waves %.3f"), sc[0], sc[1], sc[2], *reinterpret_cast<const float*>(u + 0x90),
                                *reinterpret_cast<const float*>(u + 0x94), *reinterpret_cast<const uint32_t*>(u + 0x138),
                                *reinterpret_cast<const float*>(u + 0x13C));
        }
    }
    ImGui::SliderInt(_("Water detail"), &settings.water_detail, 0, 3);
    ImGui::SameLine();
    Mod::help_marker(_("Waves are vertices to Remix, so they're only as fine as the water's mesh. Each level splits every "
                       "triangle of the sea's and the interactive water's meshes in four (the game's shaders then wave the "
                       "extra vertices). More detail costs Remix more work every frame. 0 = the game's meshes."));
    ImGui::TextDisabled(_("Water draws subdivided last frame: %u, meshes cached: %u"), subdiv::g_drawn_last,
                        static_cast<unsigned>(subdiv::g_cache.size()));
    {
    }
    ImGui::Checkbox(_("Give Remix each object's position"), &settings.world_positions);
    ImGui::SameLine();
    Mod::help_marker(_("Remix tells objects apart from frame to frame by their mesh and position. Without this every copy of "
                       "a prop (a street's trash cans) looked like it stood in the same spot, and the copies flickered between "
                       "lit and dark and smeared as they took each other's history."));
    ImGui::Checkbox(_("Characters too"), &settings.skinned_positions);
    ImGui::SameLine();
    Mod::help_marker(_("Characters and other skinned models: every body part gets the position of its first bone instead "
                       "of the world's origin, so Remix can tell them apart by where they are. Being tested against a body "
                       "that stays behind for a second after a room change."));
    ImGui::InputText(_("Hide water using textures containing"), settings.hide_water_textures, sizeof settings.hide_water_textures);
    ImGui::SameLine();
    Mod::help_marker(_("Water drawn with a texture whose path contains this text is left out of Remix (any case; several separated by ;). The "
                       "probe lists each water draw's textures."));
    ImGui::InputText(_("Hide textures containing"), settings.hide_textures, sizeof settings.hide_textures);
    ImGui::SameLine();
    Mod::help_marker(_("For telling which object is which: draws using a texture whose path contains this text are hidden "
                       "from Remix (any case; several separated by ;, e.g. wood00;grass). The probe lists the paths. Not "
                       "saved."));
    ImGui::Checkbox(_("Skip the depth prepass"), &settings.skip_zpass);
    ImGui::Checkbox(_("Hide occlusion tests (white diamonds at lights)"), &settings.hide_occlusion);
    ImGui::Checkbox(_("Hide shadow overlays"), &settings.hide_shadow_overlays);
    ImGui::Checkbox(_("Hide untextured glows"), &settings.hide_glows);
    ImGui::SameLine();
    Mod::help_marker(_("Some effect sprites (lamp halos) have no texture: the game shapes their glow in a shader Remix "
                       "doesn't run, so Remix showed them as white diamonds."));
    ImGui::Checkbox(_("Skip screen filters"), &settings.skip_filters);
    {
        static const int kPasses[] = { 0, 6, 7, 9, 10 };
        static const char* kNames[] = { "At present (Remix's default)", "Before transparent things (6)", "Before effects (7)",
                                        "Before screen filters (9)", "Before the HUD (10)" };
        int cur = 0;
        for (int i = 0; i < 5; ++i) if (kPasses[i] == settings.inject_pass) cur = i;
        if (ImGui::Combo(_("Hand the image to the game"), &cur, kNames, 5)) settings.inject_pass = kPasses[cur];
        ImGui::SameLine();
        Mod::help_marker(_("Where Remix's path-traced image goes into the game's frame. Everything the game draws after that "
                           "point uses its own shaders on top of it: refracting effects bend the traced image, the game's "
                           "filters and tone map apply, the HUD stays flat. At present (Remix's default) Remix traces "
                           "everything, the HUD included, and covers the game's image at the end."));
    }
    if (ImGui::Button(_("Probe one frame"))) settings.probe = 1;
    ImGui::SameLine();
    Mod::help_marker(_("For finding faults: writes remix_probe.txt in the game folder with how bright the picture is at "
                       "each stage of the next frame (before and after Remix's image goes in, each pass after it, the "
                       "final screen). Press it, then send the file."));
    ImGui::Checkbox(_("Skip the HUD and menus"), &settings.skip_hud);
    ImGui::SameLine();
    Mod::help_marker(_("Remix traces the HUD as if it were in the world. Skipping it also hides the title screen and menus; "
                       "tagging the HUD's textures as UI in Remix's own menu (Alt+X) keeps them as a flat overlay instead."));
    ImGui::Checkbox(_("Albedo on stage 0"), &settings.albedo_to_stage0);
    ImGui::SameLine();
    Mod::help_marker(_("Remix takes each object's colour from texture stage 0; DMC4's shaders put the colour map on any "
                       "stage, so without this the floor shows its normal map. Moves the colour map there for Remix."));
    ImGui::Checkbox(_("Game lights"), &settings.game_lights);
    ImGui::SameLine();
    Mod::help_marker(_("Gives Remix the stage's own lights (its sun and fill lights, and point lights such as lamps and "
                       "muzzle flashes) as real lights. Remix can't see DMC4's lighting, which happens in its shaders."));
    if (settings.game_lights) {
        ImGui::SliderFloat(_("Directional strength"), &settings.dir_strength, 0.0f, 20.0f, "%.2f");
        ImGui::SliderFloat(_("Point light strength"), &settings.point_strength, 0.0f, 50.0f, "%.2f");
        ImGui::TextDisabled(_("%d directional, %d point lights (%d copies of other lights left out; %d light no model in the game)"),
                            g_ndir, g_npoint, g_dup_skipped, g_unlit_skipped);
        if (g_off_skipped || g_attr_skipped)
            ImGui::TextDisabled(_("Left out as in the game: %d switched off (other light sets), %d by their mAttr"),
                                g_off_skipped, g_attr_skipped);
        ImGui::Checkbox(_("Point lights through the Remix API"), &settings.api_lights);
        ImGui::SameLine();
        Mod::help_marker(_("Remix makes the game's point lights only as bright as their range and stops taking their "
                           "changes after a moment, so lamps barely lit anything. Through the Remix API they light with "
                           "the game's colour (times Point light strength) at the distance below. Needs exposeRemixApi = "
                           "True in .trex\\bridge.conf (dmc4_hook's remix folder has it)."));
        if (settings.api_lights) {
            if (api::g_state > 0) ImGui::TextDisabled(_("Remix API ready: %u lights drawn last frame"), api::g_drawn_last);
            else if (api::g_state < 0) ImGui::TextDisabled(_("Remix API not available (error %d): lights go the old way"), api::g_error);
            ImGui::SliderFloat(_("Light size"), &settings.point_radius, 1.0f, 50.0f, "%.0f cm");
            ImGui::SliderFloat(_("Full colour at"), &settings.point_distance, 25.0f, 2000.0f, "%.0f cm", ImGuiSliderFlags_Logarithmic);
        }
        ImGui::Checkbox(_("Ambient fill"), &settings.fill);
        ImGui::SameLine();
        Mod::help_marker(_("Remix has no ambient light and can't use DMC4's baked lighting, so interiors go dark. This turns the "
                           "stage's ambient colour into a soft light at the camera."));
        if (settings.fill) {
            ImGui::SliderFloat(_("Fill strength"), &settings.fill_strength, 0.0f, 20.0f, "%.2f");
            ImGui::SliderFloat(_("Fill range"), &settings.fill_range, 200.0f, 20000.0f, "%.0f cm", ImGuiSliderFlags_Logarithmic);
            ImGui::TextDisabled(_("Ambient colour %.2f %.2f %.2f"), g_hemi[0], g_hemi[1], g_hemi[2]);
            if (api::dome::g_drawn_last) ImGui::TextDisabled(_("Off while the ambient dome is drawn"));
        }
        ImGui::Checkbox(_("Ambient dome"), &settings.dome);
        ImGui::SameLine();
        Mod::help_marker(_("The room's hemisphere lights (DMC4's ambient light: a sky colour from above, a ground colour from "
                           "below) as Remix's dome light, the light rays get when they leave the room. Needs Game lights through "
                           "the Remix API. The ambient fill steps aside while it's drawn."));
        if (settings.dome) {
            ImGui::SliderFloat(_("Dome strength"), &settings.dome_strength, 0.0f, 8.0f, "%.2f");
            ImGui::Checkbox(_("Sky from the sea's cube map"), &settings.dome_cube);
            ImGui::SameLine();
            Mod::help_marker(_("Where the room has an ocean, the dome shows the cube map the game's sea reflects (its sky and "
                               "horizon), at the hemisphere lights' brightness: the sea takes the game's tint and the room "
                               "its sky's colours. Without an ocean the dome keeps the hemisphere lights' two colours."));
            if (api::dome::use_cube())
                ImGui::TextDisabled(_("Sky: %s (%d px faces, x %.3f)"), api::dome::g_cube.path, api::dome::g_cube.n,
                                    api::dome::g_cube_scale);
            ImGui::TextDisabled(_("%d hemisphere lights; radiance from above %.2f %.2f %.2f, from below %.2f %.2f %.2f%s"),
                                g_ndome_hemis, api::dome::g_sky[0], api::dome::g_sky[1], api::dome::g_sky[2],
                                api::dome::g_ground[0], api::dome::g_ground[1], api::dome::g_ground[2],
                                api::dome::g_drawn_last ? "" : _(" (not drawn)"));
            ImGui::InputText(_("Sky textures containing"), settings.sky_textures, sizeof settings.sky_textures);
            ImGui::SameLine();
            Mod::help_marker(_("The stage's own sky: textures whose path contains this text (several separated by ';'). "
                               "Tag them as Sky Box Textures in Remix (rtx.conf's rtx.skyBoxTextures) and Remix draws them "
                               "into its sky, which reflections and the light from the sky then show. The dome would take "
                               "that sky's place, so it stays off while one of them is drawn."));
            if (api::sky_shown())
                ImGui::TextDisabled(_("Off: the stage's sky is drawn (%s)"), g_sky_tex_path.c_str());
            else if (settings.sky_textures[0])
                ImGui::TextDisabled(_("%zu sky textures loaded, none drawn lately"), g_sky_tex.size());
        }
        ImGui::InputText(_("Terrain textures containing"), settings.terrain_textures, sizeof settings.terrain_textures);
        ImGui::SameLine();
        Mod::help_marker(_("Blended ground (texture layers laid over each other and mixed by masks, like the forest's): "
                           "textures whose path contains this text (several separated by ';'). Tag them as Terrain Textures "
                           "in Remix (rtx.conf's rtx.terrainTextures) and Remix bakes them from above into one ground "
                           "texture with the game's mixing and their replacement materials' bump maps. Without it the "
                           "layers lie on each other and flicker."));
        if (settings.terrain_textures[0])
            ImGui::TextDisabled(_("%zu terrain textures loaded, %u draws last frame"), g_terrain_tex.size(),
                                g_terrain_draws_last);
    }
    ImGui::Checkbox(_("Sun (without game lights)"), &settings.sun);
    ImGui::SameLine();
    Mod::help_marker(_("Gives Remix the stage's sun (the light its shadows come from) as a real light. Remix can't see "
                       "DMC4's own lighting, which happens in its shaders."));
    if (settings.sun) {
        ImGui::SliderFloat(_("Sun strength"), &settings.sun_strength, 0.0f, 20.0f, "%.2f");
        ImGui::ColorEdit3(_("Sun colour"), settings.sun_color);
    }
    ImGui::InputText(_("Lights on textures containing"), settings.light_textures, sizeof settings.light_textures);
    ImGui::SameLine();
    Mod::help_marker(_("Puts a light at the middle of every object drawn with a texture whose path contains this text "
                       "(several separated by ';'). Remix's own Add Light to Texture can't place its lights in DMC4. Uses "
                       "the Remix API (Point lights through the Remix API on). Effects drawn after the hand-over aren't "
                       "covered."));
    if (settings.light_textures[0]) {
        ImGui::ColorEdit3(_("Texture light colour"), settings.tex_light_color);
        ImGui::SliderFloat(_("Texture light strength"), &settings.tex_light_strength, 0.0f, 50.0f, "%.2f");
        ImGui::SliderFloat(_("Texture light size"), &settings.tex_light_radius, 1.0f, 50.0f, "%.0f cm");
        ImGui::TextDisabled(_("%zu textures match, %u lights last frame"), g_light_tex.size(), texlight::g_found_last);
    }
    ImGui::Checkbox(_("Water stand-in"), &settings.water_standin);
    ImGui::SameLine();
    Mod::help_marker(_("DMC4's water has no colour map, so Remix showed its wave maps (teal / magenta). This puts one "
                       "solid colour there instead; every water surface then shares one texture, which a Remix material "
                       "replacement can turn into real water."));
    if (settings.water_standin) {
        ImGui::ColorEdit3(_("Water colour"), settings.water_color);
        ImGui::SliderFloat2(_("Water flow"), settings.water_flow, -0.2f, 0.2f, "%.3f");
        ImGui::SliderFloat(_("Water tiling"), &settings.water_tiling, 0.25f, 64.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SameLine();
        Mod::help_marker(_("How many times the wave pattern repeats across the water: higher = smaller waves."));
        ImGui::Checkbox(_("Puddles as puddles"), &settings.puddles);
        ImGui::SameLine();
        Mod::help_marker(_("Fortuna's puddles are drawn with the sea's shader, so they got the sea's dark, wavy look. "
                           "Sea-shader water narrower than the size below gets its own stand-in, which the DMC4Water Remix "
                           "mod draws as a thin film of clear water over the ground."));
        if (settings.puddles) {
            ImGui::SliderFloat(_("Puddles up to"), &settings.puddle_size, 100.0f, 20000.0f, "%.0f cm across",
                               ImGuiSliderFlags_Logarithmic);
            ImGui::TextDisabled(_("Last frame: %u puddle draws, %u sea draws"), puddle::g_puddles_last, puddle::g_seas_last);
        }
    }
    if (settings.inject_pass > 0) {
        ImGui::Checkbox(_("Remix's image as it is"), &settings.linearize);
        ImGui::SameLine();
        Mod::help_marker(_("Remix hands over an image it has already tone-mapped. The game's final pass doubles the scene "
                           "and may tone-map it again (a stage's own curve, or ToneMap's): with this on, the image goes in at "
                           "half and that pass runs without a curve, so the screen shows Remix's lighting plus the game's "
                           "bloom. Off: the game's final pass as it is (too bright or too dark)."));
        if (settings.linearize)
            ImGui::SliderFloat(_("Exposure"), &settings.exposure, 0.1f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    }
    ImGui::Checkbox(_("Patch shaders for Remix (restart)"), &settings.relabel);
    ImGui::SameLine();
    Mod::help_marker(_("Swaps texture-coordinate labels in effect and flat-water shaders so Remix finds their UVs and colours, "
                       "and gives material shaders a normal output so Remix shades with the game's world-space normals "
                       "(needs rtx.useVertexCapturedNormals = True in rtx.conf). Applied as the game creates its shaders: "
                       "change it, save the config, restart."));
    ImGui::Checkbox(_("Sky as sky"), &settings.sky);
    ImGui::SameLine();
    Mod::help_marker(_("Tells Remix which draws are the sky, so it draws them as the background and lights the scene with "
                       "them, instead of treating the sky panels as walls that cast shadows."));
    ImGui::Checkbox(_("Hide refraction effects"), &settings.hide_refraction);
    ImGui::TextDisabled(_("Hide water (instead of the stand-in):"));
    ImGui::Checkbox(_("Sea type (depth-faded)"), &settings.hide_water_depth);
    ImGui::SameLine();
    ImGui::Checkbox(_("Normal map only"), &settings.hide_water_simple);
    ImGui::SameLine();
    ImGui::Checkbox(_("Flat (BP floor)"), &settings.hide_water_flat);
    ImGui::SameLine();
    Mod::help_marker(_("Heat haze and refracting sword trails bend the screen image in a shader; Remix only sees their "
                       "distortion texture, which shows as a purple colour."));
    ImGui::TextDisabled(_("Last frame: %u of %u draws skipped, %u textures moved, %u water passes, %u models drawn "
                          "outside the view, %u draws with another camera"), g_skipped_last, g_seen_last, g_moved_last,
                        g_water_last, g_unculled_last, g_other_draws_last);
    ImGui::TextDisabled(_("Shaders patched (needs the bridge on at startup): %u relabelled, %u with a normal output; "
                          "%u draws hidden, %u sky"), g_relabelled, g_normals, g_hidden_last, g_sky_last);
    ImGui::TextDisabled(_("Camera from the drawn frame (view %u): %u updates, %u other views passed over (%s layout)"),
                        g_main_view, g_vp_sets_last, g_vp_rejected_last,
                        g_vp_transposed < 0 ? "?" : g_vp_transposed ? "transposed" : "row");
    ImGui::PopID();
    ImGui::Unindent(Mod::lineIndent);
}

void load(const utility::Config& cfg) {
    settings.on = cfg.get<bool>("remix_bridge").value_or(false);
    settings.skip_other_scenes = cfg.get<bool>("remix_skip_other_scenes").value_or(true);
    settings.skip_zpass = cfg.get<bool>("remix_skip_zpass").value_or(true);
    settings.skip_filters = cfg.get<bool>("remix_skip_filters").value_or(true);
    settings.hide_occlusion = cfg.get<bool>("remix_hide_occlusion").value_or(true);
    settings.hide_shadow_overlays = cfg.get<bool>("remix_hide_shadow_overlays").value_or(true);
    settings.hide_glows = cfg.get<bool>("remix_hide_glows").value_or(true);
    settings.hide_refraction = cfg.get<bool>("remix_hide_refraction").value_or(true);
    settings.hide_water_depth = cfg.get<bool>("remix_hide_water_depth").value_or(false);
    settings.hide_water_simple = cfg.get<bool>("remix_hide_water_simple").value_or(false);
    settings.hide_water_flat = cfg.get<bool>("remix_hide_water_flat").value_or(false);
    settings.draw_all = cfg.get<bool>("remix_draw_all").value_or(false);
    settings.other_cameras = cfg.get<int>("remix_other_cameras").value_or(1);
    settings.world_positions = cfg.get<bool>("remix_world_positions").value_or(true);
    settings.skinned_positions = cfg.get<bool>("remix_skinned_positions").value_or(true);
    settings.api_lights = cfg.get<bool>("remix_api_lights").value_or(true);
    settings.point_radius = cfg.get<float>("remix_point_radius").value_or(5.0f);
    settings.point_distance = cfg.get<float>("remix_point_distance").value_or(200.0f);
    {
        const std::string t = cfg.get("remix_light_textures").value_or("");
        strncpy_s(settings.light_textures, t.c_str(), _TRUNCATE);
    }
    for (int i = 0; i < 3; ++i)
        settings.tex_light_color[i] = cfg.get<float>("remix_tex_light_color_" + std::to_string(i)).value_or(settings.tex_light_color[i]);
    settings.tex_light_strength = cfg.get<float>("remix_tex_light_strength").value_or(1.0f);
    settings.tex_light_radius = cfg.get<float>("remix_tex_light_radius").value_or(5.0f);
    settings.water_ripples = cfg.get<bool>("remix_water_ripples").value_or(true);
    settings.ripple_height = cfg.get<float>("remix_ripple_height").value_or(25.0f);
    settings.ripple_normal = cfg.get<float>("remix_ripple_normal").value_or(2.0f);
    settings.water_to_horizon = cfg.get<bool>("remix_water_to_horizon").value_or(true);
    settings.water_detail = cfg.get<int>("remix_water_detail").value_or(1);
    {
        const std::string w = cfg.get("remix_hide_water_textures").value_or("");
        strncpy_s(settings.hide_water_textures, w.c_str(), _TRUNCATE);
    }
    settings.water_tiling = cfg.get<float>("remix_water_tiling").value_or(8.0f);
    settings.puddles = cfg.get<bool>("remix_puddles").value_or(true);
    settings.puddle_size = cfg.get<float>("remix_puddle_size").value_or(10000.0f);
    for (int i = 0; i < 2; ++i)
        settings.water_flow[i] = cfg.get<float>("remix_water_flow_" + std::to_string(i)).value_or(settings.water_flow[i]);
    settings.skip_hud = cfg.get<bool>("remix_skip_hud").value_or(false);
    settings.inject_pass = cfg.get<int>("remix_inject_pass").value_or(6);
    settings.sky = cfg.get<bool>("remix_sky").value_or(true);
    settings.relabel = cfg.get<bool>("remix_relabel").value_or(true);
    settings.linearize = cfg.get<bool>("remix_linearize").value_or(true);
    settings.expand = cfg.get<float>("remix_expand").value_or(0.0f);
    settings.exposure = cfg.get<float>("remix_exposure").value_or(1.0f);
    settings.albedo_to_stage0 = cfg.get<bool>("remix_albedo_stage0").value_or(true);
    settings.sun = cfg.get<bool>("remix_sun").value_or(true);
    settings.game_lights = cfg.get<bool>("remix_game_lights").value_or(true);
    settings.dir_strength = cfg.get<float>("remix_dir_strength").value_or(1.0f);
    settings.point_strength = cfg.get<float>("remix_point_strength").value_or(1.0f);
    settings.fill = cfg.get<bool>("remix_fill").value_or(true);
    settings.dome = cfg.get<bool>("remix_dome").value_or(true);
    settings.dome_strength = cfg.get<float>("remix_dome_strength").value_or(1.0f);
    settings.dome_cube = cfg.get<bool>("remix_dome_cube").value_or(true);
    {
        const std::string t = cfg.get("remix_sky_textures").value_or("-t-sora;-t-sky");
        strncpy_s(settings.sky_textures, t.c_str(), _TRUNCATE);
    }
    {
        const std::string t = cfg.get("remix_terrain_textures").value_or(kTerrainTextures);
        strncpy_s(settings.terrain_textures, t.c_str(), _TRUNCATE);
    }
    settings.terrain_srgb = cfg.get<bool>("remix_terrain_srgb").value_or(false);
    settings.uv_scroll = cfg.get<bool>("remix_uv_scroll").value_or(true);
    settings.fill_strength = cfg.get<float>("remix_fill_strength").value_or(1.0f);
    settings.fill_range = cfg.get<float>("remix_fill_range").value_or(4000.0f);
    settings.sun_strength = cfg.get<float>("remix_sun_strength").value_or(2.0f);
    settings.water_standin = cfg.get<bool>("remix_water_standin").value_or(true);
    for (int i = 0; i < 3; ++i) {
        settings.sun_color[i] = cfg.get<float>("remix_sun_color_" + std::to_string(i)).value_or(settings.sun_color[i]);
        settings.water_color[i] = cfg.get<float>("remix_water_color_" + std::to_string(i)).value_or(settings.water_color[i]);
    }
}

void save(utility::Config& cfg) {
    cfg.set<bool>("remix_bridge", settings.on);
    cfg.set<bool>("remix_skip_other_scenes", settings.skip_other_scenes);
    cfg.set<bool>("remix_skip_zpass", settings.skip_zpass);
    cfg.set<bool>("remix_skip_filters", settings.skip_filters);
    cfg.set<bool>("remix_hide_occlusion", settings.hide_occlusion);
    cfg.set<bool>("remix_hide_shadow_overlays", settings.hide_shadow_overlays);
    cfg.set<bool>("remix_hide_glows", settings.hide_glows);
    cfg.set<bool>("remix_hide_refraction", settings.hide_refraction);
    cfg.set<bool>("remix_hide_water_depth", settings.hide_water_depth);
    cfg.set<bool>("remix_hide_water_simple", settings.hide_water_simple);
    cfg.set<bool>("remix_hide_water_flat", settings.hide_water_flat);
    cfg.set<bool>("remix_draw_all", settings.draw_all);
    cfg.set<int>("remix_other_cameras", settings.other_cameras);
    cfg.set<bool>("remix_world_positions", settings.world_positions);
    cfg.set<bool>("remix_skinned_positions", settings.skinned_positions);
    cfg.set<bool>("remix_api_lights", settings.api_lights);
    cfg.set<float>("remix_point_radius", settings.point_radius);
    cfg.set<float>("remix_point_distance", settings.point_distance);
    cfg.set("remix_light_textures", std::string(settings.light_textures));
    for (int i = 0; i < 3; ++i) cfg.set<float>("remix_tex_light_color_" + std::to_string(i), settings.tex_light_color[i]);
    cfg.set<float>("remix_tex_light_strength", settings.tex_light_strength);
    cfg.set<float>("remix_tex_light_radius", settings.tex_light_radius);
    cfg.set<bool>("remix_water_ripples", settings.water_ripples);
    cfg.set<float>("remix_ripple_height", settings.ripple_height);
    cfg.set<float>("remix_ripple_normal", settings.ripple_normal);
    cfg.set<bool>("remix_water_to_horizon", settings.water_to_horizon);
    cfg.set<int>("remix_water_detail", settings.water_detail);
    cfg.set("remix_hide_water_textures", std::string(settings.hide_water_textures));
    cfg.set<float>("remix_water_tiling", settings.water_tiling);
    cfg.set<bool>("remix_puddles", settings.puddles);
    cfg.set<float>("remix_puddle_size", settings.puddle_size);
    for (int i = 0; i < 2; ++i) cfg.set<float>("remix_water_flow_" + std::to_string(i), settings.water_flow[i]);
    cfg.set<bool>("remix_skip_hud", settings.skip_hud);
    cfg.set<int>("remix_inject_pass", settings.inject_pass);
    cfg.set<bool>("remix_sky", settings.sky);
    cfg.set<bool>("remix_relabel", settings.relabel);
    cfg.set<bool>("remix_linearize", settings.linearize);
    cfg.set<float>("remix_expand", settings.expand);
    cfg.set<float>("remix_exposure", settings.exposure);
    cfg.set<bool>("remix_albedo_stage0", settings.albedo_to_stage0);
    cfg.set<bool>("remix_sun", settings.sun);
    cfg.set<bool>("remix_game_lights", settings.game_lights);
    cfg.set<float>("remix_dir_strength", settings.dir_strength);
    cfg.set<float>("remix_point_strength", settings.point_strength);
    cfg.set<bool>("remix_fill", settings.fill);
    cfg.set<bool>("remix_dome", settings.dome);
    cfg.set<float>("remix_dome_strength", settings.dome_strength);
    cfg.set<bool>("remix_dome_cube", settings.dome_cube);
    cfg.set("remix_sky_textures", std::string(settings.sky_textures));
    cfg.set("remix_terrain_textures", std::string(settings.terrain_textures));
    cfg.set<bool>("remix_terrain_srgb", settings.terrain_srgb);
    cfg.set<bool>("remix_uv_scroll", settings.uv_scroll);
    cfg.set<float>("remix_fill_strength", settings.fill_strength);
    cfg.set<float>("remix_fill_range", settings.fill_range);
    cfg.set<float>("remix_sun_strength", settings.sun_strength);
    cfg.set<bool>("remix_water_standin", settings.water_standin);
    for (int i = 0; i < 3; ++i) {
        cfg.set<float>("remix_sun_color_" + std::to_string(i), settings.sun_color[i]);
        cfg.set<float>("remix_water_color_" + std::to_string(i), settings.water_color[i]);
    }
}

} // namespace remix
