#pragma once

#include "../../mod.hpp"
#include <cstdint>
#include <string>

// PbrMaterials: physically based shading (GGX specular, metal / roughness) for the materials Albam exports in its PBR mode.
// Plan, RE findings and decisions: Vibed\PBR\README.md.
//
// How: Albam's PBR materials use the SPECULAR_RIM permutations of tXfMaterialStandard (specular function 4), which no game
// file uses. The lit ones (lighting 4SPOT / SH4SPOT / EMITSH4SPOT / THINSH4SPOT) have pixel shaders of their own, with the
// registers of their SPECULAR_STANDARD twin and the twin's vertex shader, so swapping those pixel shaders changes no vanilla
// material. The swap: sShader::mpInstance (0xE55700) +0x4838 = mpTechnique[], tXfMaterialStandard's slot at 0xE55678;
// technique +0x10 = info ([2] permutations, [3] pixel shader count, [5] component records, [6] permutation table: u32 per
// key, pixel shader low 16; [8] pixel shader records). A record is {IDirect3DPixelShader9*, bytecode*}: sRender::
// dispatchCommands creates the shader from the bytecode while the handle is 0 (0x8F4A1D) and binds the handle (0x8F4E3E).
// Ours go in both; the originals come back when the mod is off.
//
// The shader (material_pbr.inc, or nativePC\shader\pbr\material_pbr.hlsl when that file exists) is compiled with
// d3dcompiler_47 on a worker thread, once per replaced pixel shader: its permutation as LIGHTING / NORMALMAP / LIGHTMAP and
// every parameter's register from the original's constant table (REG_<name> / HAS_<name>), since the engine uploads each
// parameter to the register the original lists. Unsupported permutations (detail / parallax normal maps, HDR vertex colour,
// texture colour lightmaps, multitexture) keep the stock RIM shader.
//
// The material (Albam): MaskMap = ORM (R occlusion, G roughness, B metal); FresnelFactor / FresnelBias / SpecularPow /
// EnvMapPower = roughness scale, metal scale, dielectric F0, reflection strength; envmap_bias 5 (the cube's last mip =
// roughness 1).
namespace pbr_materials {

extern bool mod_enabled;
extern bool linear_light;      // light in linear space (the scene stays gamma)
extern bool simple_specular;   // the 4 point lights get specular too (the stock shader gives them diffuse only)
extern int  reflect_mode;      // 0 the scene's ambient light (SH), 1 the cube relit to it, 2 the cube as is (usually
                               // DefaultCube_CM, a day sky: 2B looked chrome)
extern int  debug_view;        // 0 off, 1 albedo, 2 normal, 3 roughness, 4 metal, 5 occlusion, 6 diffuse light, 7 specular

struct Status {
    bool        found = false;      // the technique's records exist
    std::string waiting;            // while not found: which check failed
    int         replaced = 0;       // pixel shaders running ours
    int         targets = 0;        // pixel shaders ours is compiled for
    int         unsupported = 0;    // lit RIM pixel shaders left stock
    int         failed = 0;         // compile / create failures (stock kept)
    bool        compiling = false;
    bool        from_file = false;  // the source came from nativePC
    unsigned    max_slots = 0;      // the device's ps_3_0 instruction slots
    std::string error;              // the first compile error
};

Status status();
void request_compile();             // compile again (settings, Reload); any thread
bool write_source();                // the embedded source -> nativePC\shader\pbr\material_pbr.hlsl
void update();                      // render thread: find the records, take finished compiles, keep ours installed

} // namespace pbr_materials

class PbrMaterials : public Mod {
public:
    PbrMaterials() = default;
    std::string get_mod_name() override { return "PbrMaterials"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
