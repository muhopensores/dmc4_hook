#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Merging LMT (rMotionList, v49) files into one, for the motion registry (MotionRegistry.hpp). Pure byte work, no game
// calls, so it can be tested outside the game.
//
// The layout is the one rMotionList::load 0xA69F20 relocates (RE/player_move_pipeline.md "Registry RE"): `LMT\0`, u16
// version 49, u16 block count, u32 block offsets (0 = a null block); a block is 0xC0 bytes with its track table offset
// at +0, the track count at +4 and the two event buffer offsets at +0x74 / +0xBC; a track is 0x20 bytes with its data
// offset at +12. Every offset is from the start of the file. The merge copies each source file whole at a 16-byte
// aligned position P of the output and adds P to those fields (the loader then adds the buffer address, as it does for
// a single file), so nothing inside a block has to be understood.
namespace lmt_merge {

    constexpr int kMaxBlocks = 256;     // a motion word's block index is its low byte (uModel::setMotionEx 0x9FA065)

    // The file's block count, or -1 when it isn't a v49 LMT whose offsets all stay inside the file (`why` says what).
    int block_count(const std::vector<uint8_t>& lmt, std::string* why = nullptr);

    // Blocks [first, first + count) of the output come from `lmt`'s blocks [0, count); a source with fewer blocks
    // leaves the rest null. `lmt` may be null: the range stays null (a model with no file for that set).
    struct Piece {
        const std::vector<uint8_t>* lmt = nullptr;
        int first = 0;
        int count = 0;
    };

    // One v49 LMT with `total` blocks (<= kMaxBlocks) from `pieces` (ranges must not overlap). False with `why` on a bad
    // source or layout; `out` is then unchanged.
    bool merge(const std::vector<Piece>& pieces, int total, std::vector<uint8_t>& out, std::string* why = nullptr);

} // namespace lmt_merge
