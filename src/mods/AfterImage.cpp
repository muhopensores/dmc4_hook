#include "AfterImage.hpp"
#include "../sdk/Custom.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MtDTI.hpp"
#include "../sdk/Players.hpp"
#include "../sdk/sUnit.hpp"
#include "../dev/model_ext/ModelExtension.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <vector>

namespace after_image {

bool  trail          = false;
bool  gate_inertia   = true;
float trail_speed    = 5.0f;
bool  gate_velocity  = false;
float velocity_speed = 8.0f;
bool  gate_dodge     = false;
bool  dodge_jumps    = false;
bool  gate_mods      = true;
float trail_interval = 5.0f;
float hold_frames    = 20.0f;
float fade_frames    = 10.0f;
int   pool_size      = 8;

bool  glow           = true;
float rim_color[3]   = { 0.35f, 0.6f, 1.0f };
float rim_power      = 1.0f;
float rim_reach      = 0.35f;
float opacity        = 1.0f;
bool  keep_mask      = false;
bool  through_walls  = false;

bool  use_model_paths = false;
char  model_paths[2][4][128] = {
    { "model\\game\\pl006\\pl006_afimg", "model\\game\\pl006\\pl006_01_afimg", "model\\game\\pl006\\pl006_02_afimg",
      "model\\game\\pl006\\pl006_03_afimg" },
    { "", "", "", "" },
};

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// ---- Game addresses ----------------------------------------------------------------------------------------------
constexpr uintptr_t fn_buildRenderState = 0x00A61D00;   // cMaterial::buildRenderState(this@ecx): mRenderState0 / 1 from mFlags
constexpr uintptr_t kMaterialStandardDTI = 0x00EAE1E4;  // cMaterialStandard::DTI
constexpr uintptr_t kShader         = 0x00E55700;       // sShader::mpInstance
constexpr uintptr_t kSysTextures    = 0x6868;           // sShader::mpSysTexture[6]: 0 NullWhite, 1 NullBlack, 2 NullNormal_NM, 3 DefaultCube_CM
constexpr uintptr_t kPlayerId       = 0x1494;           // uPlayer::mPlayerID: 0 Dante, 1 Nero
constexpr uintptr_t kMoveVel        = 0x1E1C;           // uPlayer::mMoveVel
constexpr uintptr_t kActionNo       = 0x14F0;           // uPlayer::mActionNo
constexpr uintptr_t kCmnShlVtbl     = 0x00BE6720;       // uPlCmnShl000::vftable (the evade sensor)
constexpr uintptr_t kCmnShlOwner    = 0x17B0;           // its owner (uPlayer*)
constexpr int       kCmnShlLine     = 0x12;             // the unit line uPlCmnShl000::create puts it on
constexpr uintptr_t kPartModels[3]  = { 0x3270, 0x4340, 0x52C0 };   // uPlayerNero / uPlayerDante mHead, mHair, mCoat (a uDevil4Model first)
constexpr int       kPartJoint[3]   = { 4, 4, 2 };      // the copy's joint each part hangs from

// cMaterial / cMaterialStandard
constexpr uintptr_t mFlags        = 0x04;   // byte0 draw mode, byte1 alpha ref, byte2 material id, bits 24-26 blend pick, 27 zwrite, 28 zthrough
constexpr uintptr_t mStateKey     = 0x10;   // the shader permutation key (decoded at every draw)
constexpr uintptr_t mpBaseTex     = 0x1C;   // rTexture* (AlbedoMap)
constexpr uintptr_t mAlpha        = 0x20;   // < 1 forces alpha blending (cMaterial::beginDraw)
constexpr uintptr_t mDiffuseW     = 0x3C;   // mDiffuseColor.w: times mAlpha = gXfBlendFactor (the output alpha)
constexpr uintptr_t mColor2W      = 0x4C;   // times EnvMapPower in gXfFresnelFactor.w
constexpr uintptr_t mFresnel      = 0x90;   // FresnelFactor, FresnelBias, SpecularPow, EnvMapPower
constexpr uintptr_t mpMaskTex     = 0xD4;   // MaskMap
constexpr uintptr_t mpEnvTex      = 0xE8;   // EnvironmentMap (cube)
constexpr uint32_t  MODE_MASK     = 0xFF, ALPHA_REF_MASK = 0xFF00, BLEND_PICK_MASK = 0x07000000;
constexpr uint32_t  MODE_TRANSPARENT = 8, BLEND_ADDITIVE = 0x01000000, ZWRITE = 0x08000000, ZTHROUGH = 0x10000000;

// rTexture (0xE8): +0x44 mRefCount, +0xCC mpTexture (cTrans::Texture, 0x60: +0x10 the Direct3D texture, +0x14 its SLI
// copy, +0x1C / +0x1E width / height, +0x24 format, +0x30 mRange = gXfBaseMapScale for an albedo map).
constexpr size_t kResSize = 0xE8, kTexSize = 0x60;

void build_render_state(uintptr_t mat) { ((void(__fastcall*)(uintptr_t, int))fn_buildRenderState)(mat, 0); }

bool is_standard(uintptr_t mat) {
    const auto* dti = ((MtDTI*(__fastcall*)(uintptr_t, int))(at<uintptr_t>(at<uintptr_t>(mat, 0), 0x10)))(mat, 0);
    for (; dti; dti = dti->mp_parent)
        if ((uintptr_t)dti == kMaterialStandardDTI) return true;
    return false;
}

// ---- The glow's textures ------------------------------------------------------------------------------------------
// A copy of a system texture's rTexture and cTrans::Texture around a Direct3D texture of the mod's: the materials and
// beginDraw read them like any other, and the huge reference count keeps a stray release from ever freeing them.
struct alignas(16) FakeTexture {
    alignas(16) uint8_t res[kResSize];
    alignas(16) uint8_t tex[kTexSize];
    IDirect3DBaseTexture9* d3d = nullptr;
    uintptr_t ptr() { return (uintptr_t)res; }
};
FakeTexture g_black, g_white, g_cube;
std::atomic<bool> g_textures_ready{ false };
float g_cube_color[3] = { -1.0f, -1.0f, -1.0f };   // what the cube holds now (render thread)

uint32_t argb(float r, float g, float b) {
    auto c = [](float v) { return (uint32_t)(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return 0xFF000000u | c(r) << 16 | c(g) << 8 | c(b);
}

void wrap(FakeTexture& f, uintptr_t source, IDirect3DBaseTexture9* d3d) {
    std::memcpy(f.res, (void*)source, kResSize);
    std::memcpy(f.tex, (void*)at<uintptr_t>(source, 0xCC), kTexSize);
    f.d3d = d3d;
    at<uint32_t>(f.ptr(), 0x44) = 0x40000000;               // mRefCount
    at<uintptr_t>(f.ptr(), 0xCC) = (uintptr_t)f.tex;
    const uintptr_t t = (uintptr_t)f.tex;
    at<IDirect3DBaseTexture9*>(t, 0x10) = d3d;
    at<uintptr_t>(t, 0x14) = 0;
    at<uint16_t>(t, 0x1C) = 1;
    at<uint16_t>(t, 0x1E) = 1;
    at<uint32_t>(t, 0x24) = D3DFMT_A8R8G8B8;
    for (int i = 0; i < 4; ++i) at<float>(t, 0x30 + 4 * i) = 1.0f;
}

bool fill_2d(IDirect3DTexture9* t, uint32_t color) {
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, nullptr, 0))) return false;
    *(uint32_t*)lr.pBits = color;
    t->UnlockRect(0);
    return true;
}

bool fill_cube(IDirect3DCubeTexture9* t, uint32_t color) {
    for (int face = 0; face < 6; ++face) {
        D3DLOCKED_RECT lr;
        if (FAILED(t->LockRect((D3DCUBEMAP_FACES)face, 0, &lr, nullptr, 0))) return false;
        *(uint32_t*)lr.pBits = color;
        t->UnlockRect((D3DCUBEMAP_FACES)face, 0);
    }
    return true;
}

// Render thread (on_frame): the textures once the game's system textures exist, then the cube's colour as it changes.
void update_textures() {
    if (!g_textures_ready) {
        const uintptr_t shader = at<uintptr_t>(kShader, 0);
        if (!shader) return;
        const uintptr_t white = at<uintptr_t>(shader, kSysTextures), cube = at<uintptr_t>(shader, kSysTextures + 3 * 4);
        if (!white || !cube || !at<uintptr_t>(white, 0xCC) || !at<uintptr_t>(cube, 0xCC)) return;
        IDirect3DDevice9* dev = g_framework->get_d3d9_device();
        if (!dev) return;
        IDirect3DTexture9 *black_t = nullptr, *white_t = nullptr;
        IDirect3DCubeTexture9* cube_t = nullptr;
        if (FAILED(dev->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &black_t, nullptr)) ||
            FAILED(dev->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &white_t, nullptr)) ||
            FAILED(dev->CreateCubeTexture(1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &cube_t, nullptr)) ||
            !fill_2d(black_t, 0xFF000000u) || !fill_2d(white_t, 0xFFFFFFFFu)) {
            if (black_t) black_t->Release();
            if (white_t) white_t->Release();
            if (cube_t) cube_t->Release();
            spdlog::error("AfterImage: couldn't make the glow's textures");
            return;
        }
        wrap(g_black, white, black_t);
        wrap(g_white, white, white_t);
        wrap(g_cube, cube, cube_t);
        g_cube_color[0] = -1.0f;
    }
    if (g_cube_color[0] != rim_color[0] || g_cube_color[1] != rim_color[1] || g_cube_color[2] != rim_color[2]) {
        if (fill_cube((IDirect3DCubeTexture9*)g_cube.d3d, argb(rim_color[0], rim_color[1], rim_color[2])))
            std::memcpy(g_cube_color, rim_color, sizeof g_cube_color);
    }
    g_textures_ready = true;
}

// ---- Replacement models -------------------------------------------------------------------------------------------
struct Loaded { char path[128]; void* res; };
Loaded g_loaded[2][4] = {};
bool g_reload = false;

void release_loaded() {
    for (auto& per : g_loaded)
        for (Loaded& l : per) {
            if (l.res) devil4_sdk::release_resource((cResource*)l.res);
            l = Loaded{};
        }
}

// The replacement rModel for a part, loaded once per path (game thread), or 0.
uintptr_t replacement(int character, int part) {
    if (!use_model_paths) return 0;
    const char* path = model_paths[character][part];
    Loaded& l = g_loaded[character][part];
    if (std::strcmp(l.path, path) != 0) {
        if (l.res) devil4_sdk::release_resource((cResource*)l.res);
        strcpy_s(l.path, path);
        l.res = path[0] ? devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, l.path, 1) : nullptr;
    }
    return (uintptr_t)l.res;
}

// ---- The copy --------------------------------------------------------------------------------------------------------
constexpr int kParts = 3;           // head, hair, coat
constexpr int kFixed = 1 + kParts;  // the body (the unit itself) and the parts; the attachments' copies follow
constexpr uint32_t ATTR_VISIBLE = 0x4000;   // cUnit flags: an attachment follows its owner's visibility through it
enum Look : uint8_t { LOOK_NONE, LOOK_PLAIN, LOOK_GLOW };
// what a model's materials were set up for: the look and the options it reads at set-up
uint8_t setup_key(Look look) { return (uint8_t)(look | (keep_mask ? 4 : 0) | (through_walls ? 8 : 0)); }

// The glow is the shader's specular term, which the permutations without specular (NONE 0, MIRROR 2, POWMAP 3: models
// made without a specular map) don't have. Each has a STANDARD twin in XfMaterialStandard.bfx, and the material's key
// (cMaterial +0x10 = skin + 7 lighting + 35 normal map + 140 specular + 700 lightmap + 5600 multitexture) is decoded again
// at every draw (cMaterialStandard::beginDraw 0xA6387B), so the copy's material takes the twin. Skinned materials only:
// some static twins read one UV set more than the mesh may have.
uint32_t glow_key(uint32_t key) {
    const uint32_t specular = key / 140 % 5, skin = key % 7;
    if (specular == 1 || specular == 4 || skin == 0) return key;
    return key - 140 * specular + 140;
}

struct Saved { uintptr_t mat; uint32_t flags, state_key; uintptr_t base, mask, env; float alpha, diffuse_w, color2_w; float fresnel[4]; };

// One model of a copy and what was done to its materials. Its memory is the copy's to free (the game deletes a unit
// without running C++ destructors).
struct ModelSlot {
    uDevil4Model* m;
    uintptr_t built;        // the rModel it was set up with
    bool on;                // in this copy (a model the player has and shows)
    Look look;              // what its materials were turned into
    uint8_t key;            // and with which options (setup_key)
    Saved* saved;
    uint32_t saved_num, saved_cap;
};

struct Clone;
Clone* g_pool[kPoolMax] = {};
uint32_t g_spawns = 0, g_made = 0, g_rebuilds = 0, g_reused_busy = 0;
int g_attachments_copied = 0;      // on the last copy

struct Clone : CustomUnit<Clone> {
    uDevil4Model parts[kParts];
    ModelSlot* slots = nullptr;     // kFixed, then one per attachment copied so far
    int slot_num = 0, slot_cap = 0;
    int pool_index = 0;
    bool active = false;
    float age = 0.0f;

    explicit Clone(int index) : pool_index(index) {
        for (int i = 0; i < kParts; ++i) {
            uactor_sdk::uDevil4ModelCons(&parts[i]);
            parts[i].mpParent = this;
            parts[i].ParentJoint = kPartJoint[i];
        }
        grow(kFixed);
        mActorType = 5;
        mWorkRate.mType = 9;
        hide();
    }

    // The slots up to `n`: the body and parts, then a model of its own per attachment (made here, freed by destructor).
    // ModelExtension leaves them all alone: their .mod paths are the player's, and its lists would hang chains and
    // attachments on them and hide them.
    bool grow(int n) {
        if (n > slot_cap) {
            const int cap = std::max({ n, slot_cap * 2, 8 });
            auto* s = (ModelSlot*)std::realloc(slots, cap * sizeof(ModelSlot));
            if (!s) return false;
            std::memset(s + slot_cap, 0, (cap - slot_cap) * sizeof(ModelSlot));
            slots = s;
            slot_cap = cap;
        }
        while (slot_num < n) {
            ModelSlot& s = slots[slot_num];
            if (slot_num == 0) s.m = (uDevil4Model*)this;
            else if (slot_num <= kParts) s.m = &parts[slot_num - 1];
            else {
                void* mem = _aligned_malloc(sizeof(uDevil4Model), 16);
                if (!mem) return false;
                std::memset(mem, 0, sizeof(uDevil4Model));
                uactor_sdk::uDevil4ModelCons(mem);
                s.m = (uDevil4Model*)mem;
                s.m->mTransparency = 0.0f;
            }
            model_extension::exclude((uintptr_t)s.m);
            ++slot_num;
        }
        return true;
    }

    void hide() {
        for (int k = 0; k < slot_num; ++k) {
            slots[k].m->mTransparency = 0.0f;
            slots[k].on = false;
        }
        active = false;
    }

    // a model of the player's the copy leaves out: hidden by ModelExtension's <hide> (or otherwise), or an attachment
    // whose owner isn't shown
    static bool left_out(uDevil4Model* src, bool attachment) {
        if (src->mTransparency <= 0.0f || model_extension::hidden((uintptr_t)src)) return true;
        return attachment && !(at<uint32_t>((uintptr_t)src, 0x04) & ATTR_VISIBLE);
    }

    // The model's materials back to what setMaterials made (before they are rebuilt or destroyed: the textures put
    // in are the mod's).
    static void restore(ModelSlot& s) {
        for (uint32_t i = 0; i < s.saved_num; ++i) {
            const Saved& v = s.saved[i];
            at<uint32_t>(v.mat, mFlags) = v.flags;
            at<uint32_t>(v.mat, mStateKey) = v.state_key;
            at<uintptr_t>(v.mat, mpBaseTex) = v.base;
            at<uintptr_t>(v.mat, mpMaskTex) = v.mask;
            at<uintptr_t>(v.mat, mpEnvTex) = v.env;
            at<float>(v.mat, mColor2W) = v.color2_w;
            std::memcpy(&at<float>(v.mat, mFresnel), v.fresnel, sizeof v.fresnel);
            at<float>(v.mat, mAlpha) = v.alpha;
            at<float>(v.mat, mDiffuseW) = v.diffuse_w;
            build_render_state(v.mat);
        }
        s.saved_num = 0;
        s.look = LOOK_NONE;
        s.key = 0;
    }

    // Plain: alpha blended, no depth write. Glow: additive transparent mode, black albedo, the rim cube (and a white
    // mask), a permutation with the specular term, the fresnel turned into a rim.
    static void set_look(ModelSlot& s, Look want) {
        if (s.look == want && s.key == setup_key(want)) return;
        restore(s);
        const uDevil4Model* m = s.m;
        const uintptr_t* mats = (const uintptr_t*)m->mpMaterial;
        const uint32_t num = mats ? m->MaterialNum : 0;
        if (num > s.saved_cap) {
            auto* p = (Saved*)std::realloc(s.saved, num * sizeof(Saved));
            if (!p) return;
            s.saved = p;
            s.saved_cap = num;
        }
        for (uint32_t i = 0; i < num; ++i) {
            const uintptr_t mat = mats[i];
            if (!mat || !is_standard(mat)) continue;
            Saved& v = s.saved[s.saved_num++];
            v.mat = mat;
            v.flags = at<uint32_t>(mat, mFlags);
            v.state_key = at<uint32_t>(mat, mStateKey);
            v.base = at<uintptr_t>(mat, mpBaseTex);
            v.mask = at<uintptr_t>(mat, mpMaskTex);
            v.env = at<uintptr_t>(mat, mpEnvTex);
            v.alpha = at<float>(mat, mAlpha);
            v.diffuse_w = at<float>(mat, mDiffuseW);
            v.color2_w = at<float>(mat, mColor2W);
            std::memcpy(v.fresnel, &at<float>(mat, mFresnel), sizeof v.fresnel);
            uint32_t f = v.flags & ~(ZWRITE | ZTHROUGH);
            if (through_walls) f |= ZTHROUGH;
            if (want == LOOK_GLOW) {
                f = (f & ~(MODE_MASK | ALPHA_REF_MASK | BLEND_PICK_MASK)) | MODE_TRANSPARENT | BLEND_ADDITIVE;
                at<uint32_t>(mat, mStateKey) = glow_key(v.state_key);
                at<uintptr_t>(mat, mpBaseTex) = g_black.ptr();
                if (!keep_mask) at<uintptr_t>(mat, mpMaskTex) = g_white.ptr();
                at<uintptr_t>(mat, mpEnvTex) = g_cube.ptr();
                at<float>(mat, mColor2W) = 1.0f;
            }
            at<uint32_t>(mat, mFlags) = f;
            build_render_state(mat);
        }
        s.look = want;
        s.key = setup_key(want);
    }

    // Slot k set up as `src` (the player's model, its replacement, or an attachment) once, then posed as it is now.
    bool take(int k, uDevil4Model* src, uintptr_t rmodel, Look want) {
        ModelSlot& s = slots[k];
        uDevil4Model* m = s.m;
        s.on = false;
        m->mTransparency = 0.0f;
        if (!src || !rmodel || left_out(src, k >= kFixed)) return false;
        if (s.built != rmodel) {
            restore(s);
            custom_utils::get_model(m, (void*)rmodel);
            s.built = rmodel;
            ++g_rebuilds;
            m->mCameraTransparencyEnable = false;
            m->Render.mVFCullLevel = 0;
            m->Render.mRenderMode = 0;
            m->mShadowCastGroup = 0;
            m->mShadowRecvGroup = 0;
            if (k == 0) {
                m->Render.mZPrepassDist = 2000;
                m->Render.mPriorityBias = -100;
            }
        }
        set_look(s, want);
        const uint32_t n = std::min(src->mJointNum, m->mJointNum);
        for (uint32_t i = 0; i < n; ++i) m->mpJoint[i].mWmat = src->mpJoint[i].mWmat;
        if (k >= kFixed) {                  // an attachment's copy hangs from nothing: its place as the source's now
            m->mWmat = src->mWmat;
            m->mLightGroup = src->mLightGroup;
        }
        s.on = true;
        return true;
    }

    void activate(uintptr_t pl) {
        const int character = at<uint32_t>(pl, kPlayerId) == 1 ? 1 : 0;
        const Look want = (glow && g_textures_ready) ? LOOK_GLOW : LOOK_PLAIN;
        auto* body = (uDevil4Model*)pl;
        mPos = body->mPos;
        mQuat = body->mQuat;
        // the body and parts, then what ModelExtension attaches to them and to those attachments
        std::vector<uintptr_t> sources;
        for (int k = 0; k < kFixed; ++k) {
            auto* src = k == 0 ? body : (uDevil4Model*)(pl + kPartModels[k - 1]);
            sources.push_back((uintptr_t)src);
            const uintptr_t rep = replacement(character, k);
            take(k, src, rep ? rep : src->mpRModel, want);
        }
        int k = kFixed;
        for (size_t o = 0; o < sources.size(); ++o)
            for (uintptr_t a : model_extension::attachments_of(sources[o])) {
                if (std::find(sources.begin(), sources.end(), a) != sources.end() || !grow(k + 1)) continue;
                sources.push_back(a);
                auto* src = (uDevil4Model*)a;
                take(k++, src, src->mpRModel, want);
            }
        for (int j = k; j < slot_num; ++j) take(j, nullptr, 0, want);   // spares from a copy with more attachments
        g_attachments_copied = k - kFixed;
        age = 0.0f;
        active = true;
        apply(1.0f);
    }

    // The look's live values (the GUI can change them while copies are out) at `strength` (1 full, 0 gone).
    void apply(float strength) {
        const float a = std::clamp(opacity * strength, 0.0f, 1.0f);
        // the shader's term is (1 + cos)^5 (1 at the outline, 32 face-on): weight 1 at the outline down to 0 where cos
        // reaches rim_reach
        const float x = 1.0f + std::clamp(rim_reach, 0.02f, 1.0f), factor = 1.0f / (x * x * x * x * x - 1.0f);
        for (int k = 0; k < slot_num; ++k) {
            const ModelSlot& s = slots[k];
            if (!s.on) continue;
            const bool lit = s.look == LOOK_GLOW;
            s.m->mTransparency = lit ? 1.0f : a;     // the glow keeps mAlpha at 1 (uDevil4Model::updateMaterial copies this)
            for (uint32_t i = 0; i < s.saved_num; ++i) {
                const uintptr_t mat = s.saved[i].mat;
                at<float>(mat, mAlpha) = lit ? 1.0f : a;
                at<float>(mat, mDiffuseW) = lit ? a : 1.0f;
                if (lit) {
                    float* fr = &at<float>(mat, mFresnel);
                    fr[0] = factor;
                    fr[1] = -factor;
                    fr[2] = 200.0f;                       // the lights' highlights shrink to points
                    fr[3] = rim_power;
                }
            }
        }
    }

    void tick() {
        if (!active) return;
        age += m_delta_time;
        const float hold = std::max(hold_frames, 0.0f), fade = std::max(fade_frames, 0.0f);
        if (age >= hold + fade) { hide(); return; }
        apply(age <= hold || fade <= 0.0f ? 1.0f : 1.0f - (age - hold) / fade);
        if (slots[kParts].on) custom_utils::updateWmat(&parts[kParts - 1]);
    }

    void render(void* trans) {
        if (!active) return;
        const uintptr_t context = at<uintptr_t>((uintptr_t)trans, 0x2A90);
        if (!context || !at<uint8_t>(context, 0x1FC)) return;
        for (int k = 0; k < slot_num; ++k) {
            uDevil4Model* m = slots[k].m;
            if (slots[k].on && m->mpRModel && m->flags.bits.mTransMode) uactor_sdk::render_call(m, trans);
        }
    }

    void destructor(uint32_t flag) {
        for (int k = 0; k < slot_num; ++k) {
            restore(slots[k]);
            std::free(slots[k].saved);
        }
        if (pool_index >= 0 && pool_index < kPoolMax && g_pool[pool_index] == this) g_pool[pool_index] = nullptr;
        for (auto& p : parts) uactor_sdk::uDevil4ModelDest(&p);
        for (int k = kFixed; k < slot_num; ++k) {
            uactor_sdk::uDevil4ModelDest(slots[k].m);
            _aligned_free(slots[k].m);
        }
        std::free(slots);
        CustomActor::destructor(flag);
    }
};

void despawn(int slot) {
    if (Clone* c = g_pool[slot]) {
        g_pool[slot] = nullptr;          // the game deletes it later; its destructor puts the materials back
        c->hide();
        custom_utils::despawn(c);
    }
}

std::atomic<bool> g_spawn_request{ false };
int g_active = 0;
// player 1's gate readings at his last tick (readout)
float g_inertia = 0.0f, g_speed = 0.0f;
bool g_dodging = false, g_requested = false;

// Each player's trail (player_info's index): his copy timer and his last position (for the speed gate). The pool of
// copies is shared: a copy takes the pose of whoever asked for it.
constexpr int kPlayers = 4;
struct Trail {
    uintptr_t player = 0;
    float timer = 0.0f;
    float last_pos[3] = {};
};
Trail g_trails[kPlayers];
std::atomic<bool> g_trail_request[kPlayers] = {};   // a mod's move asked for this player's trail since his last tick

// The player has a live uPlCmnShl000: the game's own dodge detector (the shell that pays the last-moment dodge bonus),
// which the start of every evasive move spawns (the lock-on side / back rolls, enemy steps, Trickster dashes, air
// tricks, Gilgamesh rolls, Nero's evade...; also plain jumps; player_move_pipeline.md). It lives up to 20 frames while
// he stays in that move. No action flag says "evasive" (ACT_STAT is stand / jump / attack / guard / run...), so plain
// jumps are told apart by action: JumpBegin 6, Jump 7. The rolls are actions 18 / 19 (uPlayer::moveActPoseJumpBegin
// 0x8233B0 picks the roll's motion from the stick, +0x16B8, then action 19 and createCmnShl000, vtable slot 112).
bool dodging(uintptr_t pl) {
    if (!dodge_jumps) {
        const uint32_t act = at<uint32_t>(pl, kActionNo);
        if (act == 6 || act == 7) return false;
    }
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return false;
    for (uintptr_t u = (uintptr_t)su->mMoveLine[kCmnShlLine].mTop; u; u = at<uintptr_t>(u, 0x08)) {
        const uint32_t state = at<uint32_t>(u, 0x04) & 7;
        if (at<uintptr_t>(u, 0) == kCmnShlVtbl && at<uintptr_t>(u, kCmnShlOwner) == pl && (state == 1 || state == 2))
            return true;
    }
    return false;
}

void tick(uintptr_t pl) {
    const int idx = player_info::index_of(pl);
    if (idx < 0 || idx >= kPlayers) return;
    if (idx == 0) {                          // the pool's upkeep and the panel's Spawn one: once a frame, in player 1's tick
        if (g_reload) {
            g_reload = false;
            release_loaded();
        }
        const int size = std::clamp(pool_size, 1, kPoolMax);
        for (int i = size; i < kPoolMax; ++i) despawn(i);
        g_active = 0;
        for (int i = 0; i < size; ++i) if (g_pool[i] && g_pool[i]->active) ++g_active;
        if (g_spawn_request.exchange(false)) spawn(pl);
    }

    // the gates: any one that is on and open leaves copies
    Trail& t = g_trails[idx];
    const float dt = at<float>(pl, 0x10);   // cUnit::m_delta_time
    const float* pos = &at<float>(pl, 0x30);   // uCoord::mPos
    const float d[3] = { pos[0] - t.last_pos[0], pos[1] - t.last_pos[1], pos[2] - t.last_pos[2] };
    const float speed = (t.player == pl && dt > 0.0f) ? std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) / dt : 0.0f;
    std::memcpy(t.last_pos, pos, sizeof t.last_pos);
    t.player = pl;
    const float inertia = at<float>(pl, kMoveVel);
    const bool dodge = gate_dodge && dodging(pl);
    const bool requested = g_trail_request[idx].exchange(false) && gate_mods;
    if (idx == 0) {
        g_speed = speed;
        g_inertia = inertia;
        g_dodging = dodge;
        g_requested = requested;
    }
    if (!trail) return;
    t.timer -= dt;
    const bool open = (gate_inertia && inertia > trail_speed) || (gate_velocity && speed > velocity_speed) || dodge || requested;
    if (open && t.timer <= 0.0f) {
        spawn(pl);
        t.timer = trail_interval;
    }
}

} // namespace

void request_spawn() { g_spawn_request = true; }

void trail_request(uintptr_t pl) {
    const int idx = player_info::index_of(pl);
    if (idx >= 0 && idx < kPlayers) g_trail_request[idx] = true;
}

bool spawn(uintptr_t pl) {
    if (!pl) return false;
    const int size = std::clamp(pool_size, 1, kPoolMax);
    Clone* pick = nullptr;
    for (int i = 0; i < size && !pick; ++i)
        if (g_pool[i] && !g_pool[i]->active) pick = g_pool[i];
    for (int i = 0; i < size && !pick; ++i)
        if (!g_pool[i]) {
            pick = g_pool[i] = Clone::spawn(1, i);
            ++g_made;
        }
    if (!pick) {                          // all out: the oldest goes
        for (int i = 0; i < size; ++i)
            if (g_pool[i] && (!pick || g_pool[i]->age > pick->age)) pick = g_pool[i];
        if (!pick) return false;
        ++g_reused_busy;
    }
    pick->activate(pl);
    ++g_spawns;
    return true;
}

} // namespace after_image

// ---- The mod ---------------------------------------------------------------------------------------------------------------
using namespace after_image;

std::optional<std::string> AfterImage::on_initialize() {
    input_commands::add_tick(&after_image::tick);
    if (!input_commands::hook_installed) {
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("AfterImage: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init AfterImage (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void AfterImage::on_frame(fmilliseconds& dt) {
    update_textures();
}

void AfterImage::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("AfterImage");
    ImGui::Checkbox(_("After image trail"), &trail);
    ImGui::SameLine();
    help_marker(_("Leave glowing copies of your pose behind. When: the checks below; any one that is on and met leaves "
                  "copies."));
    ImGui::SameLine();
    if (ImGui::Button(_("Spawn one"))) request_spawn();
    ImGui::SameLine();
    help_marker(_("Leave one copy of your pose where you stand now."));

    ImGui::Indent();
    ImGui::Checkbox(_("Inertia"), &gate_inertia);
    ImGui::SameLine();
    help_marker(_("Leave copies while the game's move speed (inertia: runs, dashes, momentum carried into the air) is "
                  "above Inertia above. Moves that carry you with their animation don't count."));
    ImGui::SliderFloat(_("Inertia above"), &trail_speed, 0.0f, 30.0f, "%.1f");
    ImGui::SameLine();
    help_marker(_("The move speed the trail needs (cm per frame)."));
    ImGui::Checkbox(_("Speed"), &gate_velocity);
    ImGui::SameLine();
    help_marker(_("Leave copies while you actually move faster than Speed above, in any direction and for any reason: "
                  "animations that carry you, falls, launches, teleports."));
    ImGui::SliderFloat(_("Speed above"), &velocity_speed, 0.0f, 60.0f, "%.1f");
    ImGui::SameLine();
    help_marker(_("How far you have to move in one frame for the trail (cm per frame; walking is about 5, a run about 10)."));
    ImGui::Checkbox(_("Dodges"), &gate_dodge);
    ImGui::SameLine();
    help_marker(_("Leave copies at the start of evasive moves: the ones the game watches for a last-moment dodge (side and "
                  "back rolls, enemy steps, Trickster dashes and air tricks, Gilgamesh rolls, Nero's evade...), for their "
                  "first 20 frames at most. Moves added by mods don't count unless they start the game's dodge check."));
    ImGui::SameLine();
    ImGui::Checkbox(_("Jumps too"), &dodge_jumps);
    ImGui::SameLine();
    help_marker(_("Count plain jumps as dodges (the game checks them for last-moment dodges as well)."));
    ImGui::Checkbox(_("Mod moves"), &gate_mods);
    ImGui::SameLine();
    help_marker(_("Leave copies while a move added by another mod asks for them (the Gunslinger moves with After images on, "
                  "set per move in the Gunslinger settings)."));
    ImGui::Text(_("Now: inertia %.1f, speed %.1f, %s%s"), g_inertia, g_speed, g_dodging ? _("dodging") : _("not dodging"),
                g_requested ? _(", a mod move asks") : "");
    ImGui::SliderFloat(_("Trail interval"), &trail_interval, 1.0f, 30.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Frames between two copies of the trail."));
    ImGui::SliderFloat(_("Hold"), &hold_frames, 0.0f, 120.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Frames a copy stays at full strength before it fades."));
    ImGui::SliderFloat(_("Fade"), &fade_frames, 0.0f, 120.0f, "%.0f");
    ImGui::SameLine();
    help_marker(_("Frames a copy takes to fade out."));
    ImGui::SliderInt(_("Copies"), &pool_size, 1, kPoolMax);
    ImGui::SameLine();
    help_marker(_("How many copies are kept for reuse. When all are out, the oldest is taken for the next one."));

    ImGui::Checkbox(_("Glowing edges"), &glow);
    ImGui::SameLine();
    help_marker(_("The copy's edges glow and its inside stays see-through. Off: a plain see-through copy."));
    ImGui::ColorEdit3(_("Glow colour"), rim_color, ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    help_marker(_("The colour of the glowing edges."));
    ImGui::SliderFloat(_("Glow brightness"), &rim_power, 0.0f, 4.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("How bright the edges glow. 1 = the glow colour itself at the outline."));
    ImGui::SliderFloat(_("Glow width"), &rim_reach, 0.02f, 1.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("How far the glow reaches in from the outline: higher fills more of the copy, 1 reaches the parts "
                  "facing the camera."));
    ImGui::SliderFloat(_("Strength"), &opacity, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("How strong a copy is before it fades (its opacity)."));
    ImGui::Checkbox(_("Keep surface detail"), &keep_mask);
    ImGui::SameLine();
    help_marker(_("The glow follows the model's shine map, so it shows the clothes' and skin's detail instead of an even "
                  "colour."));
    ImGui::Checkbox(_("Draw through walls"), &through_walls);
    ImGui::SameLine();
    help_marker(_("Copies are drawn over whatever stands in front of them."));

    if (ImGui::TreeNode(_("Copy models"))) {
        ImGui::Checkbox(_("Use these models"), &use_model_paths);
        ImGui::SameLine();
        help_marker(_("Copies use these models instead of the character's own, when they load (game paths without .mod; "
                      "empty = the character's own part)."));
        static const char* parts[4] = { "Body", "Head", "Hair", "Coat" };
        static const char* chars[2] = { "Dante", "Nero" };
        for (int c = 0; c < 2; ++c) {
            ImGui::TextUnformatted(chars[c]);
            for (int p = 0; p < 4; ++p) {
                ImGui::PushID(c * 4 + p);
                ImGui::InputText(parts[p], model_paths[c][p], sizeof model_paths[c][p]);
                ImGui::PopID();
            }
        }
        if (ImGui::Button(_("Reload models"))) g_reload = true;
        ImGui::SameLine();
        help_marker(_("Load the models again (after changing the files on disk)."));
        ImGui::TreePop();
    }
    ImGui::Text("copies %u made, %d out | %u spawned, %u taken while out | %u model set-ups | %d attachment(s) on the last "
                "| glow textures %s", g_made, g_active, g_spawns, g_reused_busy, g_rebuilds, g_attachments_copied,
                g_textures_ready ? "ready" : "not yet");
    ImGui::Unindent();
    ImGui::PopID();
}

void AfterImage::on_config_load(const utility::Config& cfg) {
    trail          = cfg.get<bool>("after_image_trail").value_or(trail);
    gate_inertia   = cfg.get<bool>("after_image_gate_inertia").value_or(gate_inertia);
    trail_speed    = cfg.get<float>("after_image_trail_speed").value_or(trail_speed);
    gate_velocity  = cfg.get<bool>("after_image_gate_speed").value_or(gate_velocity);
    velocity_speed = cfg.get<float>("after_image_speed_above").value_or(velocity_speed);
    gate_dodge     = cfg.get<bool>("after_image_gate_dodge").value_or(gate_dodge);
    dodge_jumps    = cfg.get<bool>("after_image_dodge_jumps").value_or(dodge_jumps);
    gate_mods      = cfg.get<bool>("after_image_gate_mods").value_or(gate_mods);
    trail_interval = cfg.get<float>("after_image_trail_interval").value_or(trail_interval);
    hold_frames    = cfg.get<float>("after_image_hold").value_or(hold_frames);
    fade_frames    = cfg.get<float>("after_image_fade").value_or(fade_frames);
    pool_size      = std::clamp(cfg.get<int>("after_image_copies").value_or(pool_size), 1, kPoolMax);
    glow           = cfg.get<bool>("after_image_glow").value_or(glow);
    rim_color[0]   = cfg.get<float>("after_image_glow_r").value_or(rim_color[0]);
    rim_color[1]   = cfg.get<float>("after_image_glow_g").value_or(rim_color[1]);
    rim_color[2]   = cfg.get<float>("after_image_glow_b").value_or(rim_color[2]);
    rim_power      = cfg.get<float>("after_image_glow_brightness").value_or(rim_power);
    rim_reach      = cfg.get<float>("after_image_glow_reach").value_or(rim_reach);
    opacity        = cfg.get<float>("after_image_strength").value_or(opacity);
    keep_mask      = cfg.get<bool>("after_image_keep_detail").value_or(keep_mask);
    through_walls  = cfg.get<bool>("after_image_through_walls").value_or(through_walls);
    use_model_paths = cfg.get<bool>("after_image_use_models").value_or(use_model_paths);
    static const char* chars[2] = { "dante", "nero" };
    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 4; ++p)
            if (auto v = cfg.get("after_image_model_" + std::string(chars[c]) + "_" + std::to_string(p))) strcpy_s(model_paths[c][p], v->c_str());
}

void AfterImage::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("after_image_trail", trail);
    cfg.set<bool>("after_image_gate_inertia", gate_inertia);
    cfg.set<float>("after_image_trail_speed", trail_speed);
    cfg.set<bool>("after_image_gate_speed", gate_velocity);
    cfg.set<float>("after_image_speed_above", velocity_speed);
    cfg.set<bool>("after_image_gate_dodge", gate_dodge);
    cfg.set<bool>("after_image_dodge_jumps", dodge_jumps);
    cfg.set<bool>("after_image_gate_mods", gate_mods);
    cfg.set<float>("after_image_trail_interval", trail_interval);
    cfg.set<float>("after_image_hold", hold_frames);
    cfg.set<float>("after_image_fade", fade_frames);
    cfg.set<int>("after_image_copies", pool_size);
    cfg.set<bool>("after_image_glow", glow);
    cfg.set<float>("after_image_glow_r", rim_color[0]);
    cfg.set<float>("after_image_glow_g", rim_color[1]);
    cfg.set<float>("after_image_glow_b", rim_color[2]);
    cfg.set<float>("after_image_glow_brightness", rim_power);
    cfg.set<float>("after_image_glow_reach", rim_reach);
    cfg.set<float>("after_image_strength", opacity);
    cfg.set<bool>("after_image_keep_detail", keep_mask);
    cfg.set<bool>("after_image_through_walls", through_walls);
    cfg.set<bool>("after_image_use_models", use_model_paths);
    static const char* chars[2] = { "dante", "nero" };
    for (int c = 0; c < 2; ++c)
        for (int p = 0; p < 4; ++p)
            cfg.set("after_image_model_" + std::string(chars[c]) + "_" + std::to_string(p), std::string(model_paths[c][p]));
}
