#pragma once

#include "../../mod.hpp"
#include <cstdint>
#include <string>

// ToneMap: a filmic curve in place of the game's Reinhard (Vibed\ToneMap\README.md). The game makes the final picture in one
// of 207 pixel shaders of XfFilter.bfx (techniques tXfFilter*): Reinhard with a constant exposure (96), Reinhard with the
// exposure (and white) from the auto-exposure texture (88), or no tone map at all (23: the stages with EnableToneMapping
// off, nearly all), each a combination of the afterimage blend (two forms), the scotopic blue shift, extended or plain
// Reinhard and any of three bloom layers. Ours (tonemap.inc, or
// nativePC\shader\tonemap\tonemap.hlsl when that file exists) is compiled once per original with its registers and
// features, and put in its pixel shader record ({IDirect3DPixelShader9*, bytecode*}, as PbrMaterials / ShadowReceiver);
// the originals come back when the mod is off. With the curve at Game ours computes what the original did (checked
// offline on all 207: scratch tonemap\validate2.py). The settings go to c200 when one of ours is bound (ps_bind listener).
//
// Finding the shaders: sShader (0xE55700) +0x603C mTechniqueNum, +0x3038 mTechniqueDesc[] (12 bytes, +4 name), +0x4838
// mpTechnique[]; a technique's +0x10 info: [3] pixel shader count, [8] the records. Every pixel shader of a tXfFilter*
// technique is classified from its constant table (PointSampler0, nothing outside the composites' parameters; gXfMiddleGray
// = TONE 1, PointSampler4 = TONE 2, neither = 0) and its instruction tokens (only the composites' instructions, at most 6
// texld, three rcp with a tone map and none without, a def without; the float 1.27 in a def = the blue shift, 4.0 = the
// blend's alpha form, the adaptation texel read as .y = TONE 2's extended Reinhard): exactly the 207 in the DX9 bfx
// (scratch tonemap\classify2.py).
//
// It matters most with a 16-bit float scene buffer (PbrMaterials > Scene buffer: Use 16-bit float): 8 and 10-bit buffers
// hold the scene only up to twice white.
namespace tone_map {

struct Settings {
    bool  on = true;
    int   curve = 2;            // 0 the game's (Reinhard), 1 ACES, 2 AgX
    float ev = 0.0f;            // exposure on top of the game's own, in stops
};
extern Settings settings;

struct Status {
    bool        found = false;
    std::string waiting;
    int         targets = 0;
    int         replaced = 0;
    int         failed = 0;
    bool        compiling = false;
    bool        from_file = false;
    std::string error;
};
Status status();

} // namespace tone_map

class ToneMap : public Mod {
public:
    ToneMap() = default;
    std::string get_mod_name() override { return "ToneMap"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
