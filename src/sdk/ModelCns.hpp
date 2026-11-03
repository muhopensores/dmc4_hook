#pragma once

#include <cstdint>

// Joint constraints on any model at runtime: the game's own classes, on blocks of ours, installed in a joint's mpConstraint
// (joint +0x0C), which uModel::updateWorldMatrix 0x9EA8B0 runs right after that joint's FK (vtbl +0x14, thiscall(joint,
// model), retn 8), so the joint's children follow the result. RE note model_constraints.md (DX9).
//   PIN    cCnsMatrix: the joint takes a source matrix, whole or rotation only, with an offset. The stock class has no blend
//          (adjust never reads mBlend), so our vtable copy runs it and mixes the result with the joint's FK.
//   OFFSET cCnsOffset: scale / rotation / translation on the joint itself, no source.
//   AIM    cCnsDirPos: an axis of the joint points at a source matrix's position, blended by the class itself. Stock it
//          reads a world point somebody sets; our vtable copy sets it from the source before running it.
// The blocks are never freed (a line job can be inside `adjust` when a model goes): `release` keeps them for reuse.
// A source pointer goes stale when its model runs setModel or dies, so the caller re-binds per frame and clears it from
// those hooks. Call from the main thread; the adjust functions run in the world pass, possibly on a line job.
namespace model_cns {

enum Kind { PIN, OFFSET, AIM };

struct Pin    { bool rotation_only = false; float blend = 1.0f; float pos[3] = { 0, 0, 0 }, rot[3] = { 0, 0, 0 }, scale[3] = { 1, 1, 1 }; };
struct Offset { bool add_motion = true; float pos[3] = { 0, 0, 0 }, rot[3] = { 0, 0, 0 }, scale[3] = { 1, 1, 1 }; };
// axes: 0 +x, 1 +y, 2 +z, 3 -x, 4 -y, 5 -z; `axis` and `up` must be different axes. limit: degrees either way, per joint axis.
struct Aim    { int axis = 2, up = 1; float blend = 1.0f; bool limited = false; float limit[3] = { 0, 0, 0 }; };

uintptr_t make_pin(const Pin& p);
uintptr_t make_offset(const Offset& o);
uintptr_t make_aim(const Aim& a);
void release(uintptr_t block);                 // the caller has taken it off its joint (`remove`)

// The source of a PIN / AIM: a 4x4 matrix (a joint's mWmat), 0 = none (the block then leaves the joint alone).
void bind(uintptr_t block, const float* source);

enum Install { INSTALLED, NO_JOINT, TAKEN };
// Into `joint_no` of `model` (a joint NUMBER). TAKEN: a chain or another constraint holds the joint, left alone. A skin
// attachment's own constraint gives way (an explicit entry says more than "same number as the owner").
Install install(uintptr_t block, uintptr_t model, int joint_no);
void remove(uintptr_t block, uintptr_t model, int joint_no);   // only if the joint still holds it

const float* joint_matrix(uintptr_t model, int joint_no);      // &mWmat, 0 when the model lacks the joint

} // namespace model_cns
