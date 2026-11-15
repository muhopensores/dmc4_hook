#pragma once

#include "../../mod.hpp"

// LightSwitches: no shader compile hitch when effect lights appear (Rainstorm's muzzle flashes, Pandora's shots).
//
// The game's material shaders switch each light on with a bool constant: b0-b3 gXfLightEnable0-3 (spot lights), b4-b7
// gXfSimpleLightEnable0-3 (point lights). NVIDIA's DX9 driver builds a version of a pixel shader for each on / off
// combination it is drawn with, the first time, and the game waits for it: after a restart the first Rainstorm walked
// every lit shader on screen through 1, 2, 3 and 4 point lights and stalled the frame four times for ~230 ms (trace
// 2026-11-15: the render thread waiting on nvd3dum, nvgpucomp32 busy on nine driver threads). With this on, those switches
// are always TRUE and a light the game had off gets zeroed registers instead (zero colour adds nothing), so each shader
// is only ever drawn with one combination. The picture is the same; the GPU evaluates all eight lights.
//
// sRender::dispatchCommands 0x8F4120 uploads a draw's shader parameters from its binding list (4 bytes per binding:
// parameter index in the low 16 bits, register in byte 2; the values beside them, edi), skipping a parameter whose value
// is the one it sent last in this call (one cache for every pixel shader parameter, [esp + 0x1A0 + index * 4]):
//   0x8F54FD pixel shader bools: SetPixelShaderConstantB(reg, value != 0). Here the light switches are sent as TRUE.
//   0x8F569B pixel shader float arrays of varying length ({count | flags, data}): gXfLightParam (c0, 4 registers per spot
//            light) and gXfLightSimple (c23, 2 per point light) carry only the lights that are on, so an off light's
//            registers hold what another shader left there (other techniques write c0-c7 and c16 up). Noted here.
//   0x8F56F8 after the pixel shader parameters (also reached when a draw binds the same set as the last one): the off
//            lights' registers are zeroed, and the array's cache entry cleared so the next draw sends it again.
// Parameter indices: globals sShader sets at start-up (gXfLightEnable0-3 0xE5571C.., gXfSimpleLightEnable0-3 0xE556A4..,
// gXfLightParam 0xE556F4, gXfLightSimple 0xE55494). Vertex shaders bind no bools.
//
// Strongest lights first: a model gets its point lights (attribute 0x10, type 2: every effect light) in sub_A66180
// (called per model by uModel::transDefault and sub_529CC0), which walks the shared light list (+0x20, count +0x2020)
// and gives the first 4 that pass its tests the shader's slots (sShader +0x68F8 = 4), in list order. The list is in
// creation order, so with more than 4 lights alive (the Dante Cyber muzzle flashes live ~15 frames, a Rainstorm fires
// every few) the oldest, fading ones keep the slots and new full-strength flashes are left out until one dies: the
// lighting strobes. With this on, 0xA66448 (where the loop takes the list and its count) gets a copy, per call and
// thread, with those lights in order of what they give the model (colour x falloff at the model's box: 1 up to +0x50,
// 0 at +0x54, from position +0x40) in the places the point lights held, other lights where they were. Only when more
// point lights pass the model's light group than there are slots; otherwise the game's own list.
class LightSwitches : public Mod {
public:
    LightSwitches() = default;
    std::string get_mod_name() override { return "LightSwitches"; };
    std::vector<std::string> get_search_terms() override { return {"light", "hitch", "stutter", "lag", "shader", "compile"}; }

    static bool mod_enabled;
    static bool rank_enabled;   // strongest point lights first, per model

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook_bool, hook_array, hook_end, hook_rank;
};
