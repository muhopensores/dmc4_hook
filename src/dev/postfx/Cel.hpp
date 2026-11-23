#pragma once

#include <string>

namespace utility { class Config; }

// Cel shading for characters (PostFX step 4, Vibed\PostFX\README.md): banded light, flattened ambient, a hard highlight.
// The game computes lighting in each material's pixel shader (XfMaterialStandard.bfx), and the usual character permutation
// shares its pixel shader with static ones (PS 22), so characters are routed to shaders of their own: the skinned
// SPECULAR_POWMAP permutations, which only characters use. With cel shading on:
//   - every skinned SPECULAR_POWMAP pixel shader record that is the twin (key + (3 - specular) x 140) of a skinned lit
//     NONE / STANDARD key with the same vertex shader gets our cel.inc, compiled from its constant table;
//   - those keys' permutation table entries point at the twin's pixel shader (their vertex shader stays).
// Off: the table entries and the records' shaders go back. cMaterialStandard::beginDraw reads the key on every draw, so
// both apply at once.
namespace cel {

struct Settings {
    bool  on = false;
    float edge = 0.25f;        // N.L where light turns to shade
    float soft = 0.04f;        // half width of that edge
    float flat = 0.7f;         // 0 ambient by the normal (stock) .. 1 one flat ambient colour
    float highlight = 0.6f;    // the hard highlight's strength (0 none)
    float shaded = 0.3f;       // how much of the light the shaded side keeps
    float hl_size = 0.5f;      // the highlight's threshold on (N.H)^power: higher = smaller
    float amb_shade = 0.6f;    // the ambient light on the side facing away from where most of it comes from
};
extern Settings settings;

struct Status {
    bool        found = false;
    std::string waiting;
    int         targets = 0;       // pixel shader records with our shader
    int         routes = 0;        // permutation keys pointed at them
    int         replaced = 0;      // records holding ours now
    int         failed = 0;
    bool        compiling = false;
    bool        from_file = false;
    std::string error;
};
Status status();

void init();                       // the bind listener (tuning registers); once
void update();                     // every frame: find, compile, install or put back
void draw_gui();
void load(const utility::Config& cfg);
void save(utility::Config& cfg);

} // namespace cel
