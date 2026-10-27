#pragma once

#include <cstdint>

// Model chains (.clt / .phs = rCnsChain: coats, hair, tails) spawned at runtime on any model: a uCnsChain unit, the
// data-driven chain the cutscene schedulers create (RE note chain_cnschain.md section 6, DX9). The unit carries a
// cCnsChain at +0x20 bound to a model (its .clt joint numbers are that model's joints) and, optionally, a .col
// (rCollisionShape) whose shapes sit on a collision model's joints. Each frame its move attaches the chain to the model's
// joints, and the model's own world-matrix pass solves it; so the chain's move line must run BEFORE the model's (a lower
// index) and never be the model's own line (units of one line can run as parallel jobs).
//
// Lifetime: a room change kills the gameplay lines (12 included), so chains are spawned again per room; a chain whose model
// dies nulls its model pointer and idles (`orphaned`). Call from the main thread (a frame callback), not from a line job.
namespace cns_chain {

constexpr int kDefaultLine = 12;        // nothing vanilla adds to it; players are on 13, enemies on 15 / 16 / 18

// The chain unit (0 when `clt_path` doesn't load). Paths are game paths without the extension (".clt" / ".col" are the
// native ones). `col_path` / `collision_model` are optional: collision runs only with both.
uintptr_t spawn(uintptr_t model, const char* clt_path, const char* col_path = nullptr, uintptr_t collision_model = 0,
                int line = kDefaultLine);

// Takes the chain off its model and kills the unit (sUnit deletes it over its next two passes). `model_alive`: the model
// is still a live object, so the chain can release its joints (uCnsChain::setEnable(false)); false when the model is
// already gone (only the pointers are cleared). Never kill a chain any other way while its model lives: cCnsChain's
// destructor clears the constraint of EVERY joint of the model (leg IK, the coat's attachment, other chains).
void despawn(uintptr_t chain, bool model_alive = true);

// Takes the chain off its model without killing the unit: its own joints released (when `model_alive`), the model and
// collision-model pointers cleared, so its destructor (which clears every joint of the model) touches nothing. What a
// hook on ~cCnsChain 0x42C7A0 calls for a chain the game deletes itself (a room change kills its line).
void detach(uintptr_t chain, bool model_alive = true);
constexpr uintptr_t kChainInUnit = 0x20;   // the cCnsChain inside the unit (~cCnsChain's esi = unit + 0x20)

uintptr_t model_of(uintptr_t chain);    // +0xFD4, 0 once the model died (the chain nulled it)
inline bool orphaned(uintptr_t chain) { return model_of(chain) == 0; }

} // namespace cns_chain
