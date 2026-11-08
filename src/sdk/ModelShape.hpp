#pragma once

#include <cstdint>
#include <string>

// Morph targets on any model at runtime: the engine's own GPU shape blending, which no shipped DX9 file uses (RE note
// Vibed\RE\shape_morph.md). A model's cShape (uModel +0xC04) takes an rShape (.shp: per model part a map and its targets,
// position deltas as float textures) and up to four weight lists (cShape::WeightNode, each an rShapeWeight .shw: one
// curve per target, played at its own speed, mixed by `rate`). Each frame cShape's update (sub_A68770) advances the lists
// and sums their weights, and the model's draw blends the weighted targets into one map per part (a render pass), which
// the SKIN_4WT_SHAPE vertex shader adds to the vertex positions. Only meshes made for it move (skin function 5, the mesh
// shape flag, TEXCOORD0 = the vertex's texel in its part's map).
//
// Only uDevil4Model::move runs the update: the players' move (uActor::move) doesn't, so a caller that puts a shape on a
// player runs `update` itself every frame. Call everything from the main thread.
namespace model_shape {

constexpr int kLists = 4;

struct List {
    std::string shw;                 // game path without extension; "" = no list
    float speed = 1.0f;              // curve frames per game frame (negative plays backwards)
    float rate  = 1.0f;              // its share when several lists play (weights are mixed by rate / sum of rates)
    float frame = 0.0f;              // where it starts
    bool  loop  = true;              // false: stops on its last frame
    bool  hold  = false;             // play() with loop false: keep the last frame's weights (else `retire` stops the list
                                     // at its end: a finished list still counts in the mix, rate / sum of rates)
};

// Loads the .shp and the lists' .shw and sets them on `model` through the game's own setters (each keeps its reference;
// ours is released). Lists past `count` are cleared. false (and nothing changed) when a file doesn't load.
bool set(uintptr_t model, const char* shp, const List* lists, int count, std::string& error);
void clear(uintptr_t model);                       // no shape, no lists: the model draws its rest shape
void update(uintptr_t model, float delta);         // cShape's own update (sub_A68770), delta in game frames

// On demand, like requesting a motion: `list` (0-3) of a model that has a shape plays `l.shw` from `l.frame`. A new file
// starts through the setter (which also restarts it); the file the list already has restarts in place (frame, the end
// bit cleared). The first call loads the .shw (sResource::create, cached after). false + error: no shape on the model,
// the file didn't load, or the list stayed off (the .shw has fewer curves than the .shp has targets).
bool play(uintptr_t model, int list, const List& l, std::string& error);
void stop(uintptr_t model, int list);              // the list lets go of its .shw: its weights drop out
// Stops the lists play() started with loop and hold false that reached their end (WeightNode.mState bit 1). Run it every
// frame after the update. `forget`: the model is gone (its destructor releases the files).
void retire(uintptr_t model);
void forget(uintptr_t model);

uintptr_t shape_of(uintptr_t model);               // the rShape set, 0 none
int   targets(uintptr_t model);                    // the rShape's targets, 0 none
float frame(uintptr_t model, int list);
float speed(uintptr_t model, int list);
bool  list_on(uintptr_t model, int list);          // a .shw set and the setter switched it on (curves >= targets)
float weight(uintptr_t model, int target);         // the summed weight of a target this frame
float delta_of(uintptr_t model);                   // the unit's frame delta (cUnit +0x10)

} // namespace model_shape
