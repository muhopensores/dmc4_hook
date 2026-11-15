#include "PbrSpheres.hpp"
#include "../../mod.hpp"
#include "../../sdk/Custom.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MtDTI.hpp"
#include "../../sdk/RimGlow.hpp"
#include "../../sdk/sUnit.hpp"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace pbr_spheres {

Settings settings;

namespace {

char kModelPath[] = "model\\game\\pbrtest\\pbr_sphere";
constexpr int  kLine = 1;                   // the unit line AfterImage's copies and EnemyAI's auras use
constexpr int  kMax = 9;
constexpr uintptr_t kTextureDti = 0xEAD680;  // rTexture::DTI
constexpr uintptr_t kEnvTex = 0xE8, kEnvLod = 0x60;
constexpr int  kMaxMats = 8;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

struct Sphere;
Sphere*     g_spheres[kMax] = {};
std::string g_error;
std::string g_cube_name;                     // the cube the spheres reflect (its path)

constexpr uintptr_t kDevil4ModelDti = 0xE58838;  // uDevil4Model::DTI; its parent (+0x10) is uModel's
constexpr uintptr_t kIsKindOf = 0x402BD0;        // MtObject::isKindOf (thiscall: the object, a DTI)

bool is_sphere(const void* u);

// The env cube the area's models reflect: the most common non-default rTexture in the env slot (+0xE8) of the
// cMaterialStandards of every uModel on the unit lines. 0 if none.
uintptr_t find_room_cube() {
    sUnit* su = devil4_sdk::get_sUnit();
    const uintptr_t shader = *reinterpret_cast<uintptr_t*>(0xE55700);
    if (!su || !shader) return 0;
    const uintptr_t default_cube = at<uintptr_t>(shader, 0x6874);          // sShader::mpSysTexture[3], DefaultCube_CM
    void* model_dti = *reinterpret_cast<void**>(kDevil4ModelDti + 0x10);
    if (!model_dti) return 0;
    auto is_kind_of = reinterpret_cast<char(__thiscall*)(const void*, void*)>(kIsKindOf);
    std::unordered_map<uintptr_t, int> count;
    for (int line = 0; line < 32; ++line) {
        int n = 0;
        for (const cUnit* u = su->mMoveLine[line].mTop; u && n < 8192; u = u->mp_next_unit, ++n) {
            if (is_sphere(u) || !is_kind_of(u, model_dti)) continue;
            const auto* m = reinterpret_cast<const uModel*>(u);
            const uintptr_t* mats = (const uintptr_t*)m->mpMaterial;
            const uint32_t num = mats ? std::min<uint32_t>(m->MaterialNum, 1024) : 0;
            for (uint32_t i = 0; i < num; ++i) {
                const uintptr_t mat = mats[i];
                if (!mat || !rim_glow::is_standard(mat)) continue;
                const uintptr_t env = at<uintptr_t>(mat, 0xE8);
                if (env && env != default_cube) ++count[env];
            }
        }
    }
    uintptr_t best = 0;
    int most = 0;
    for (const auto& [tex, c] : count)
        if (c > most) { best = tex; most = c; }
    return best;
}

// the roughness of sphere i of n
float roughness_of(int i, int n) {
    if (n <= 1) return settings.rough_from;
    return settings.rough_from + (settings.rough_to - settings.rough_from) * (float)i / (float)(n - 1);
}

struct Sphere : CustomUnit<Sphere> {
    int   index = 0;
    void* res = nullptr;                    // our reference to the rModel
    void* cube = nullptr;                   // our reference to the environment cube put in the materials
    int   cube_now = 0;                     // the Cube choice it was set up for (0 none)
    float cube_lod = 0.0f;
    uintptr_t saved_mat[kMaxMats] = {};     // the materials given the cube, with what their slot held before
    uintptr_t saved_env[kMaxMats] = {};
    float     saved_lod[kMaxMats] = {};
    int       saved_num = 0;

    Sphere(int i, float x, float y, float z) : index(i) {
        mActorType = 5;
        mWorkRate.mType = 9;
        mPos.x = x;
        mPos.y = y;
        mPos.z = z;
    }

    void startup() {
        res = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, kModelPath, 1);
        if (!res) {
            g_error = "model\\game\\pbrtest\\pbr_sphere.mod didn't load: install the pbr_test package";
            custom_utils::despawn(this);
            return;
        }
        auto* m = (uDevil4Model*)this;
        custom_utils::get_model(m, res);
        m->mCameraTransparencyEnable = false;
        m->Render.mVFCullLevel = 0;
        if (auto* pl = (uDevil4Model*)devil4_sdk::get_local_player()) m->mLightGroup = pl->mLightGroup;   // lit as the player
        g_error.clear();
    }

    // the materials' environment slots back as setMaterials made them, and our cube released
    void drop_cube() {
        for (int i = 0; i < saved_num; ++i) {
            at<uintptr_t>(saved_mat[i], kEnvTex) = saved_env[i];
            at<float>(saved_mat[i], kEnvLod) = saved_lod[i];
        }
        saved_num = 0;
        if (cube) devil4_sdk::release_resource((cResource*)cube);
        cube = nullptr;
        cube_now = 0;
    }

    void take_cube(int want) {
        drop_cube();
        if (want < 1 || want > kRoomCube) return;
        cube_now = want;                    // also when it failed: no retry every tick
        if (want == kRoomCube) {
            const uintptr_t found = find_room_cube();
            if (!found) {
                g_error = "no model in this area reflects a cube of its own: pick another Cube";
                return;
            }
            InterlockedIncrement(reinterpret_cast<volatile LONG*>(found + 0x44));   // ours, released as a loaded one
            cube = (void*)found;
        } else {
            char path[128];
            strncpy_s(path, kCubes[want - 1].path, _TRUNCATE);
            cube = devil4_sdk::get_stuff_from_files((MtDTI*)kTextureDti, path, 1);
            if (!cube) {
                g_error = std::string(kCubes[want - 1].path) + " didn't load (the pbr_test package's cubes: install it)";
                return;
            }
        }
        const char* name = reinterpret_cast<const char*>((uintptr_t)cube + 0x04);           // cResource path
        g_cube_name.assign(name, strnlen(name, 63));
        const uintptr_t tex = at<uintptr_t>((uintptr_t)cube, 0xCC);     // cTrans::Texture
        const uint32_t mips = tex ? (at<uint32_t>(tex, 0x20) >> 20) & 0xF : 1;
        cube_lod = (float)(mips > 0 ? mips - 1 : 0);
        const auto* m = (const uDevil4Model*)this;
        const uintptr_t* mats = (const uintptr_t*)m->mpMaterial;
        const uint32_t num = mats ? m->MaterialNum : 0;
        for (uint32_t i = 0; i < num && saved_num < kMaxMats; ++i) {
            const uintptr_t mat = mats[i];
            if (!mat || !rim_glow::is_standard(mat)) continue;
            saved_mat[saved_num] = mat;
            saved_env[saved_num] = at<uintptr_t>(mat, kEnvTex);
            saved_lod[saved_num] = at<float>(mat, kEnvLod);
            ++saved_num;
            at<uintptr_t>(mat, kEnvTex) = (uintptr_t)cube;
            at<float>(mat, kEnvLod) = cube_lod;
        }
    }

    void tick() {
        custom_utils::updateLmat(this);
        custom_utils::updateWmat(this);
        if (res && settings.cube != cube_now) take_cube(settings.cube);
        const auto* m = (const uDevil4Model*)this;
        const uintptr_t* mats = (const uintptr_t*)m->mpMaterial;
        const uint32_t num = mats ? m->MaterialNum : 0;
        int n = 0;
        for (Sphere* s : g_spheres)
            if (s) ++n;
        const float rough = std::clamp(roughness_of(index, n), 0.0f, 1.0f);
        for (uint32_t i = 0; i < num; ++i) {
            const uintptr_t mat = mats[i];
            if (!mat || !rim_glow::is_standard(mat)) continue;
            at<float>(mat, 0x90) = rough;               // FresnelFactor: the roughness scale (x the ORM's G, white here)
            at<float>(mat, 0x94) = settings.metal;      // FresnelBias: the metal scale
            at<float>(mat, 0x98) = settings.f0;         // SpecularPow: F0 of the non-metal part
            // EnvMapPower: reflection strength; negative = reflect the material's own cube as it is (material_pbr.inc)
            at<float>(mat, 0x9C) = cube ? -settings.reflect : settings.reflect;
        }
    }

    void render(void* trans) {
        auto* m = (uDevil4Model*)this;
        const uintptr_t context = at<uintptr_t>((uintptr_t)trans, 0x2A90);
        if (!context || !at<uint8_t>(context, 0x1FC)) return;
        if (!m->mpRModel || !m->flags.bits.mTransMode) return;
        uactor_sdk::render_call(m, trans);
    }

    void destructor(uint32_t flag) {
        if (index >= 0 && index < kMax && g_spheres[index] == this) g_spheres[index] = nullptr;
        drop_cube();                            // before the materials go: they release what their slot holds
        void* r = res;
        res = nullptr;
        CustomActor::destructor(flag);          // the model's own reference goes with it
        if (r) devil4_sdk::release_resource((cResource*)r);
    }
};

bool is_sphere(const void* u) {
    for (Sphere* s : g_spheres)
        if (s && (const void*)s == u) return true;
    return false;
}

void remove_all() {
    for (Sphere*& s : g_spheres) {
        if (!s) continue;
        Sphere* p = s;
        s = nullptr;                            // the game deletes it later
        custom_utils::despawn(p);
    }
}

void spawn_row() {
    remove_all();
    auto* pl = (uDevil4Model*)devil4_sdk::get_local_player();
    if (!pl) {
        g_error = "no player";
        return;
    }
    // the player's frame: MT matrices are row vectors (m1 = x axis, m3 = z axis = forward, m4 = position)
    const MtMatrix& w = pl->mWmat;
    float fx = w.m3.x, fz = w.m3.z;
    const float fl = std::sqrt(fx * fx + fz * fz);
    if (fl < 1e-4f) { fx = 0.0f; fz = 1.0f; } else { fx /= fl; fz /= fl; }
    const float rx = fz, rz = -fx;              // right, on the ground
    const int n = std::clamp(settings.count, 1, kMax);
    for (int i = 0; i < n; ++i) {
        const float side = ((float)i - 0.5f * (float)(n - 1)) * settings.spacing;
        const float x = pl->mPos.x + fx * settings.distance + rx * side;
        const float y = pl->mPos.y + settings.height;
        const float z = pl->mPos.z + fz * settings.distance + rz * side;
        g_spheres[i] = Sphere::spawn(kLine, i, x, y, z);
    }
}

} // namespace

int alive() {
    int n = 0;
    for (Sphere* s : g_spheres)
        if (s) ++n;
    return n;
}

void draw_gui() {
    ImGui::PushID("PbrSpheres");
    ImGui::PushItemWidth(160.0f);
    ImGui::SliderInt(_("Spheres"), &settings.count, 1, kMax);
    ImGui::SameLine();
    Mod::help_marker(_("A row of metal spheres in front of you, roughness going from the first value to the second, to compare "
                  "the PBR shading at each level. The sliders apply live. Needs the pbr_test package "
                  "(model\\game\\pbrtest\\pbr_sphere.mod). The spheres go away with Remove or a change of area."));
    ImGui::SliderFloat(settings.count > 1 ? _("Roughness from") : _("Roughness"), &settings.rough_from, 0.0f, 1.0f, "%.2f");
    if (settings.count > 1) ImGui::SliderFloat(_("Roughness to"), &settings.rough_to, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat(_("Metal"), &settings.metal, 0.0f, 1.0f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("1 = metal (reflects in its own colour, no diffuse), 0 = non-metal (paint, plastic)."));
    ImGui::SliderFloat(_("F0"), &settings.f0, 0.0f, 0.16f, "%.3f");
    ImGui::SameLine();
    Mod::help_marker(_("How much a non-metal reflects head-on: 0.04 is most materials (0.02 water, 0.08 gems). Metals take their "
                  "colour instead."));
    {
        const char* names[kRoomCube + 1];
        names[0] = "Off: the scene's ambient (Reflections setting)";
        for (int i = 0; i < kCubeCount; ++i) names[i + 1] = kCubes[i].name;
        names[kRoomCube] = "This room's cube (what its models reflect)";
        ImGui::Combo(_("Cube"), &settings.cube, names, kRoomCube + 1);
        ImGui::SameLine();
        Mod::help_marker(_("What the spheres reflect: a real cube texture (6 faces with mips) in their material's environment "
                           "slot. This room's cube = the one the area's own models reflect (stage materials point at a cube "
                           "made for their place; DMC4 has no reflection probes). Roughness shows best in a cube with detail: "
                           "the sharp spheres mirror it, the rough ones blur it (the cube's smaller mips). Off = what every PBR "
                           "material reflects with the Reflections setting (the scene's ambient light by default), which looks "
                           "the same at every roughness. Only the spheres change; applied live."));
        if (settings.cube > 0 && !g_cube_name.empty()) ImGui::TextDisabled(_("Reflecting: %s"), g_cube_name.c_str());
    }
    ImGui::SliderFloat(_("Reflection"), &settings.reflect, 0.0f, 2.0f, "%.2f");
    ImGui::SameLine();
    Mod::help_marker(_("The material's reflection strength (EnvMapPower): how much of the surroundings it reflects."));
    ImGui::SliderFloat(_("Distance"), &settings.distance, 100.0f, 1000.0f, "%.0f cm");
    ImGui::SliderFloat(_("Spacing"), &settings.spacing, 100.0f, 400.0f, "%.0f cm");
    ImGui::SliderFloat(_("Height"), &settings.height, 0.0f, 300.0f, "%.0f cm");
    ImGui::PopItemWidth();
    if (ImGui::Button(_("Spawn spheres"))) spawn_row();
    ImGui::SameLine();
    if (ImGui::Button(_("Remove spheres"))) remove_all();
    ImGui::SameLine();
    Mod::help_marker(_("Spawn puts a new row in front of you (removing the old one). Distance, spacing and height apply at the "
                  "next Spawn; the material sliders apply live."));
    const int n = alive();
    if (n > 0) {
        char line[160];
        int len = std::snprintf(line, sizeof line, "%s", _("Roughness:"));
        for (int i = 0; i < n && len < (int)sizeof line - 8; ++i)
            len += std::snprintf(line + len, sizeof line - len, " %.2f", roughness_of(i, n));
        ImGui::TextDisabled("%s", line);
    }
    if (!g_error.empty()) ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.3f, 1.0f), "%s", g_error.c_str());
    ImGui::PopID();
}

} // namespace pbr_spheres
