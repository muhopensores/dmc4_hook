#pragma once

#include <cstdint>
#include <string>

// The motion registry: runtime motion banks shared by every mod (plan: Vibed/MotionRegistry/README.md; RE:
// RE/player_move_pipeline.md "Registry RE"). A bank holds 256 motions (a motion word is bank << 8 | block), so mods no
// longer claim a whole bank each: they register motion SETS (a body LMT, shared by both characters or per character,
// plus optional coat files) and the registry merges every registered set into the free banks of each player's models
// (lmt_merge, LmtMerge.hpp), as real rMotionList resources the game refcounts and frees with the model.
//
// Rules for a mod:
//   - register at init (`add`), ask `ready` before playing (it installs or refreshes the player's banks when needed;
//     call it from the player's own tick or a serial point, never from another unit's tick), then get each motion's word
//     with `word` for that player: never store `bank << 8 | block`, a set can sit on other blocks for the other
//     character, and moves after a reload;
//   - always install a .ctn script for a registry motion (borrow_ctn or a synthesised one): the game would read the
//     .ctn bank of the motion's bank number, which belongs to nobody;
//   - a mod that still loads its own bank (load_motion_bank) calls `reserve` for it at config load, so the registry
//     keeps out of it. A bank found holding another LMT when the registry builds is skipped too (logged once).
//
// Banks: sets registered for both characters go in 10-14 (free on both); the others fill what is left, Nero's also
// 4, 6-9 (his setup never loads them, his code never requests them; 2 is free as well but the stand's bank 2 is a stand
// LMT, and in DT the stand replays Nero's Red Queen motion words). The stand (STAND sets, Nero) has 4-14. Sets
// are placed shared-first, then by path, so the same mods always give the same words. A rebuild (a set added or removed
// after a player has its banks) waits until none of the player's motion controllers plays a registry motion.
// Files are loose files: nativePC\<path>.lmt, LMT v49. Reading and merging them runs on a builder thread started by
// every change (add / remove / reserve / refresh), so the game thread only installs finished bytes; until a rebuild
// is done a player keeps the banks it has, and a set added since isn't ready yet (a few ms after the change).
namespace motion_registry {

    enum Character : int { NERO = 0, DANTE = 1 };      // as move_registry
    enum Target : int { PLAYER = 0, STAND = 1 };

    struct SetDesc {
        std::string owner;              // for the log and readouts ("MoveDef nero_million_stab", "WallRun")
        Target target = PLAYER;
        std::string body[2];            // PLAYER: the body LMT per character ([NERO], [DANTE]); "" = not for that character
        std::string coat[2];            // PLAYER, optional: the coat LMT per character (Dante's goes on both of his coats);
                                        // block j of the coat file plays with block j of the body file
        std::string stand, stand_sub;   // STAND (Nero): the stand's and its sub-model's LMTs
    };

    typedef int Handle;                 // -1 = none

    // Register a set; the same files again return the same handle (counted: one `remove` per `add`).
    Handle add(const SetDesc& d);
    void remove(Handle h);

    // A bank a mod fills itself (`owner` again with another bank moves the reservation; bank < 0 drops it).
    void reserve(const char* owner, int character, int bank);

    // The set's motions are on `player`'s models (installs / refreshes them when needed; false while the builder works
    // on a set added since the last build).
    bool ready(Handle h, uintptr_t player);

    // The motion word of the set's `block` (its file's block index) for `player`; -1 when not ready or out of range.
    int word(Handle h, uintptr_t player, int block);

    // Read-only for GUIs and other threads (never installs): the set is on this player's models right now.
    bool placed(Handle h, uintptr_t player);

    // Read every registered file from disk again (a "Reload LMT" button); players get the new bytes on their next
    // `ready`, once none of their registry motions plays.
    void refresh();

    // Readout: "bank 10 blocks 0-5", or why the set isn't ready for this player.
    std::string describe(Handle h, uintptr_t player);

} // namespace motion_registry
