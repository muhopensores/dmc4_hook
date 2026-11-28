#pragma once

#include "../../mod.hpp"

// WaterWakes: players and enemies disturb interactive water (uInteractiveWater: the practice stage's floor, BP floors
// such as Infinite Azure's st705). In DX9 nothing feeds it but rain and the edge waves, so characters wade through a
// still surface.
//
// The simulation is GPU only (sub-scenes IWaterInit / IWaterUpdate); the CPU feeds it a force list: mpExternalForce
// (+0x104, room for 512 vec4s, 0x2000 bytes), write pointer +0x108, each force x, y = map position in -1..1, z = size (map
// units), w = strength. uInteractiveWater::move (0xAC0E90) runs two passes; each calls rain (0xAC4EB0: mDestinyOfRain
// (+0x138) random drops, size 1/256, strength 0.2) and wave (0xAC4F80: 32 edge forces while mWaveHeight +0x13C > 0), then
// turns the list into quads for the simulation (sub_AC4B20: 6 vertices of 20 bytes a force) and resets the pointer. Both
// passes' quads go into one vertex scratch (+0xF0, 0x1E000 bytes, from the ctor): 1040 bytes of fixed quads plus 120 a
// force leave room for 1015 forces a move, while each pass's list holds 512, so two full lists ran 1200 bytes past the
// scratch into the force list's heap header (the Unit heap's block list broken, blocks handed out over the water's
// buffers). The stock game pushes about 50 a pass; this keeps each pass at kPassForces. This hooks the call to rain
// (0xAC19DC, esi = the water) and pushes a force per character in the water before it.
//
// Map space: the surface grid spans local x and z from -1000 to 1000 (move's bounding box: corners +-1000 x mScale,
// mWmat (+0x50) carrying the scale too: st705's rows are 10 / 1 / 10, its translation the surface's height); map
// x = local x / 1000, map y = -local z / 1000: the simulation draws its forces in clip space (+y = the map's top row).
// Measured 2026-11-26 (the bridge's height map dump, BP stage 1, the player 7-11 m off the centre line): with +z the
// wakes, cuts and splashes landed mirrored across the centre line, as far from the player as the player was from it,
// so the user saw none near the characters. A character is in the water when its position (uActor +0x30, the feet)
// is inside that square and between 150 cm below the surface and 15 cm above it.
// rain is called twice a move (its two passes: 120 calls a second at 60 fps; characters move on every other call) and
// each pass hands the simulation its own list, so the wake is pushed on every call from a smoothed per-call speed: pushing only on the calls
// with movement left half the calls without the wake (checked by counters, 2026-11-26).
// Forces (first tuning, st705 2026-11-26): a wake while moving, stronger with speed; a splash when one comes down into
// the water from above it, stronger with the height fallen, and larger and stronger with the speed it hits at (dives and
// slams: Split, Helm Breaker; each splash is logged with its numbers), and attack shapes cut it: the actors' attack groups
// (cCollisionGroup mAttr & 1) in their collision manager (the player's uPlayer +0x1E8C, an enemy's +0x1600), each sphere
// / capsule pushing forces where it is within its radius of the surface, stronger the faster it moves (2026-11-26: Nero's
// sword combo, 45 cm spheres at ~80 cm a frame, raised the water near him from +-2 to +-15 cm at ripple height 25).
class WaterWakes : public Mod {
public:
    WaterWakes() = default;
    std::string get_mod_name() override { return "WaterWakes"; };
    std::vector<std::string> get_search_terms() override { return {"water", "ripple", "wake", "splash", "interactive"}; }

    static bool mod_enabled;

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook_rain;
};

namespace water_wakes {
// Diagnostics (netplay): called with each water as it moves, before its forces are pushed, whether wakes are on or not
void set_watch(void (*watch)(uintptr_t water));
} // namespace water_wakes
