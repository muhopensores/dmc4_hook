#pragma once

#include <cstddef>
#include <cstdint>

// Distance links between the joints of model chains (uCnsChain units, sdk/CnsChain.hpp): two joints of a model's chains,
// neighbouring tendrils of a skirt say, may be at most `max` cm apart. The game's chains know nothing of each other: each
// tendril solves alone (cCnsChain::calcJointImpl 0x430A20, RE note chain_cnschain.md 3.2), so collision can push two
// tendrils apart and open the cloth between them. A pass of ours runs right after the model's world pass (where its chains
// solve), as position-based constraints:
//   - a stretched link pulls both joints toward each other, each half the excess times `stiffness` (a tendril root keeps
//     its animated place: its partner takes it all). A joint keeps its distance to its parent in the tendril (it turns
//     about it), and everything below it in the tendril follows.
//   - the moved joints turn to aim at their next point again (the smallest turn), the joints below them that no chain
//     holds follow rigidly, and the chain's own points (mat[] translation rows) get the new places, so the next frame's
//     integration starts from them. Their velocity away from the partner is dropped (else the solver pulls them apart again).
// A few passes over all links (one shared joint, chains of links).
namespace chain_link {

struct Link {
    uintptr_t chain_a, chain_b;         // uCnsChain units, both on the model
    int slot_a, slot_b;                 // mBone slots
    float max;                          // cm
    float stiffness;                    // 0-1: the part of the excess taken out per pass
};

// The joint's mBone slot in `chain`, -1 when the chain doesn't hold that joint NUMBER.
int slot_of(uintptr_t chain, int joint_no);
// The slot starts a tendril (slot 0, or a -1 before it): it keeps its animated position.
bool is_root(uintptr_t chain, int slot);

// After `model`'s world pass (uModel::updateWorldMatrix 0x9EA8B0). The chains must be on `model`.
void solve(uintptr_t model, const Link* links, size_t count);

} // namespace chain_link
