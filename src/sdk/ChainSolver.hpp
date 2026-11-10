#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

// Our own solver for model chains (cCnsChain: coats, hair, tails), in place of the game's for the chains a list opts in
// (ModelExtension <physics>). Position based, with substeps; the algorithm, step by step, is Vibed\ModelExt\chain_solver.md
// (Albam's chain preview follows the same text), the game's solver it replaces is RE note chain_cnschain.md section 3.
// Over the game's (version 2 of the spec):
//   - substeps on the frame's real length: the same motion at any frame rate, no tunnelling through shapes in fast moves;
//   - soft constraints with fixed compliances (XPBD), so a setting means the same at any substep or pass count;
//   - settings that vary from root to tip, damping toward the animated motion, inertia split into travel and turning;
//   - collision on every segment, both ends pushed (the game keeps only the root's push), with friction;
//   - optional: the stage floor under each point, the tendrils colliding with each other, a pull toward the animation's
//     shape (stiffness) or place (pin), a cone limit around the animated direction, a partial inertia;
//   - the aim axis can be +Z (mDir 2 is +Z here; the game's reset puts the tail along -Z for it).
// It reads the .clt's settings the game copied into the chain (gravity, wind with turbulence, damping as a floor, max speed, tail
// length, collision size, stretch, up axis, parent mode and Local flags, mute flags, the blend fade) and keeps its points
// in the chain's own mat[] / vel[], so <link> (sdk/ChainLink) and the collision readout work on it unchanged.
namespace chain_solver {

// A setting along a tendril (chain_solver.md V2.2): one value, or 2-8 spread evenly from the root (u = 0) to the tail
// (u = 1), read linearly.
struct Curve {
    float v[8] = {};
    int n = 1;
    Curve(float value = 0.0f) { v[0] = value; }
    bool unset() const { return n == 1 && v[0] < 0.0f; }   // a single negative value: the setting's own default
    float at(float u) const {
        if (n <= 1) return v[0];
        const float f = std::clamp(u, 0.0f, 1.0f) * (n - 1);
        const int i = std::min((int)f, n - 2);
        return v[i] + (v[i + 1] - v[i]) * (f - i);
    }
};

struct Settings {
    int substeps = 8;           // per 60 fps frame, 1-16
    int iterations = 1;         // constraint passes per substep, 1-8
    Curve stiffness = 0.0f;     // 0-1 per frame: each segment turns toward its animated direction
    Curve pin = 0.0f;           // 0-1 per frame: each point moves toward its animated place
    float inertia = -1.0f;      // 0-1: how much of the model's motion the chain lags behind; < 0 = from the Local flags
    float inertia_move = -1.0f; // the same for the model's travel only; < 0 = inertia
    float inertia_turn = -1.0f; // the same for the model's turning only; < 0 = inertia
    Curve limit = 0.0f;         // degrees a segment may leave its animated direction, 0 = no limit
    float friction = 0.2f;      // 0-1 per frame: sliding speed lost on a contact
    Curve damping = -1.0f;      // 0-1 per frame: speed lost against the animated motion; < 0 = 0.1, or 1 - mDamping if more
    Curve radius = -1.0f;       // cm, < 0 = the .clt's mCollisionSize
    Curve gravity = 1.0f;       // times the .clt's mGravity
    bool ground = false;        // points stay above the stage floor (one line cast per point per frame)
    bool self = false;          // the chain's tendrils push each other apart
    int axis = -1;              // the axis aimed down the chain, 0-5 = +x +y +z -x -y -z; -1 = the .clt's mDir
};

// A <cloth> sheet: tendrils of chains on one model (each on this solver), in order across the sheet, solved together and
// joined point by point (chain_solver.md "Sheets").
struct ClothMember {
    uintptr_t chain;            // the cCnsChain
    int slot;                   // its tendril's root slot
    Settings settings;          // that chain's <physics> settings
};
struct Cloth {
    uintptr_t model;
    std::vector<ClothMember> members;
    bool closed = false;        // the last tendril joins the first (all the way round)
    float stretch = 1.0f, shear = 0.5f, bend = 0.0f;   // 0-1 per frame
};
// The live sheets, replacing the last set (main thread; the solves read them on any thread).
void set_cloths(std::vector<Cloth> cloths);

// cCnsChain::move 0x42DB00 has run for `chain`: its frame step joins the time the next solve covers. Any thread.
void on_move(uintptr_t chain);
// The chain's constraint call for `joint` of `model` (cCnsChain::calcJoint 0x430A00, from the model's world pass; any
// thread): the joint's mWmat holds its FK. A tendril's root call solves the whole tendril; every call writes its joint.
void calc_joint(uintptr_t chain, uintptr_t joint, uintptr_t model, const Settings& s);
void forget(uintptr_t chain);   // the chain is going (its destructor), or no longer ours
void forget_all();

struct Stats { int chains; float last_ms; };
Stats stats();

} // namespace chain_solver
