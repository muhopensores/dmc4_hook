#pragma once

#include <cstdint>

// The registry of custom move ids (MoveDef D19). A mod that adds a kAtckDefTbl row with a new atckId registers the id
// here with its enable flag and its per-tick state function; MoveTable's detours then consult the registry instead of
// hand-written per-mod compares:
//   - the ID gate (uPlayer::checkAtck 0x803C67, the unlocked-moves bitset test) lets a registered id through while its
//     mod is enabled (`id_allowed`);
//   - the dispatch hooks (Nero uPlayerNero::moveActAtckL 0x7F9CDB, Dante 0x7CD1A1) call the id's state function every
//     tick the move is the player's attack (`dispatch`); an id nobody claims does nothing, like the vanilla default.
// Two bugs came from the hand-written chains this replaces: a bypass chain that returned early when one mod was off
// hid every later mod's id, and Dante's dispatch gated on one mod froze players with that mod off (2026-10-22/24).
//
// Registration is static data (plain arrays), so a mod may register from its on_initialize whatever the mod order.
// Ids are per character (Nero's new ids start at 0x34, Dante's at 0x6C); the gate itself is character-agnostic.
namespace move_registry {

    enum Character : int { NERO = 0, DANTE = 1 };

    // the state function MoveTable calls every tick while the move is the current attack: (player, atckId)
    typedef void (__stdcall *StateFn)(uintptr_t player, uint32_t atck_id);

    constexpr int kMaxMoves = 32;

    // Register `id` of `character`; `enabled` is read live (null = always on); `fn` may be null (gate only, e.g. a
    // native routine the dispatch hook already reaches another way). Re-registering the same (character, id) replaces.
    // Returns false when the table is full.
    bool register_move(int character, uint32_t id, const bool* enabled, StateFn fn);
    void unregister_move(int character, uint32_t id);

    // read by MoveTable's detours
    bool __stdcall id_allowed(uint32_t id);                                       // any enabled entry with this id
    void __stdcall dispatch(int character, uint32_t id, uintptr_t player);        // run the entry's fn if enabled

    // readout
    struct Entry { int character; uint32_t id; const bool* enabled; StateFn fn; };
    int count();
    const Entry& entry(int i);

} // namespace move_registry
