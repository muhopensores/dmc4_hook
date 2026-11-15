#pragma once

// PBR test spheres (Graphics tab > Test spheres): a row of metal spheres in front of the player, one per roughness
// level, for comparing how the PBR shader reads at each roughness. Asked for by another modder (2026-11-14).
//
// The model is model\game\pbrtest\pbr_sphere.mod (Albam: a UV sphere, radius 50 cm, sitting on its origin; one PBR material
// with plain values roughness 1 / metal 1, so no ORM mask: the engine's NullWhite, and the record's FresnelFactor /
// FresnelBias = the roughness / metal themselves). Source: E:\DMC mod stuffs\pbr_test\pbr_sphere.blend; package
// pbr_test in the mod manager.
//
// Each sphere is a CustomUnit (sdk/Custom.hpp) on unit line 1 whose own uDevil4Model takes the rModel (vtable +0x40) and
// draws through uDevil4Model::draw (0x522DC0), so it casts and receives the stage shadow like any model. Materials are
// per model (uModel::setMaterials 0x9E7410 builds new cMaterialStandards on every setModel), so every frame each sphere
// writes its own values into its materials: +0x90 FresnelFactor = roughness, +0x94 FresnelBias = metal, +0x98
// SpecularPow = F0, +0x9C EnvMapPower = reflection strength (the shader's gXfFresnelFactor, cMaterialStandard::beginDraw
// 0xA62C30). The game deletes the spheres with the area's units (a stage change).
//
// Reflections: PBR materials reflect the scene's SH by default (PbrMaterials > Reflections), which is equally smooth at every
// roughness, so the row would only differ in its highlights. With a Cube picked, each sphere loads that cube (an rTexture,
// DTI 0xEAD680) into its material's environment slot (+0xE8, its LOD constant +0x60 = mips - 1: max(0, mips + envmap_bias
// 5 - 6), as uModel::setMaterials sets it) and writes the reflection strength negated: material_pbr.inc reads a negative
// gXfFresnelFactor.w as "this material's own cube as it is", whatever the Reflections setting. The original slot and LOD
// go back before the sphere is deleted (the material releases what its slot holds) and when the Cube changes.
namespace pbr_spheres {

struct Settings {
    int   count = 5;            // spheres in the row (1-9)
    float rough_from = 0.05f;   // the first sphere's roughness (the only one's with a count of 1)
    float rough_to = 1.0f;      // the last's
    float metal = 1.0f;
    float f0 = 0.04f;           // reflectance of the non-metal part
    float reflect = 1.0f;       // reflection of the surroundings
    int   cube = 4;             // 0 the scene's ambient (the Reflections setting), 1-3 kCubes, 4 kRoomCube
    float spacing = 120.0f;     // cm between centres
    float distance = 300.0f;    // cm in front of the player
    float height = 30.0f;       // cm above the player's feet (the sphere's bottom; its centre is 50 cm higher)
};
extern Settings settings;

// the environment cubes to pick from (game paths without the extension; 2 and 3 are in the pbr_test package)
struct CubeChoice { const char* name; const char* path; };
constexpr CubeChoice kCubes[] = {
    { "Default cube (DefaultCube_CM, 64 px)", "system\\texture\\DefaultCube_CM" },
    { "Stage cube A (st400 CM00, 128 px)",    "model\\game\\pbrtest\\pbr_test_cube_a_CM" },
    { "Stage cube B (st011 cubesui, 64 px)",  "model\\game\\pbrtest\\pbr_test_cube_b_CM" },
};
constexpr int kCubeCount = (int)(sizeof(kCubes) / sizeof(kCubes[0]));
// the Cube choice after the files: the cube the area's own models reflect (the most common env cube in the materials of the
// models on every unit line, the game's DefaultCube_CM and the spheres' own left out)
constexpr int kRoomCube = kCubeCount + 1;

void draw_gui();                // Graphics tab > Test spheres (PbrMaterials display C)
int  alive();                   // spheres spawned and not yet deleted

} // namespace pbr_spheres
