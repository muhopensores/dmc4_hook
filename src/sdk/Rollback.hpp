#pragma once
// rollback - mod state that goes with the game's frames under rollback netplay (dev/netplay/HeapSnapshot.hpp).
//
// A rewind puts the game's own memory back; a mod's own variables about the game (a move's state per player, the style a
// player picked, the shots a mod fired) live in the hook DLL and stay as they were, so after a rewind the mod would act on
// what the mispredicted frames did. A mod registers that state here: it is copied at every save and put back at every
// load. Plain data only (pointers to units are fine: the units come back with the frames). Presentation state (readouts,
// GUI) stays out. Off netplay nothing is copied.
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace rollback {

using SaveFn = void (*)(std::vector<uint8_t>& out);   // append the state
using LoadFn = void (*)(const uint8_t* in, size_t bytes);   // what SaveFn appended, back

int add(const char* name, SaveFn save, LoadFn load);   // > 0 id
int add_range(const char* name, void* data, size_t bytes);
void remove(int id);

// netplay
void capture(std::vector<uint8_t>& out);
void apply(const std::vector<uint8_t>& in);
int count();
// diagnostics: a line for netplay's log (CoopOnline sets the writer; nothing is written without one)
extern void (*diag_writer)(const char* text);
inline void diag(const char* text) { if (diag_writer) diag_writer(text); }
// diagnostics: each participant's name and a hash of its state, words in [lo, hi) (pointers into the game's heaps) as 0
void entry_hashes(std::vector<std::pair<const char*, uint32_t>>& out, uintptr_t lo, uintptr_t hi);

} // namespace rollback
