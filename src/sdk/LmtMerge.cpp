#include "LmtMerge.hpp"

#include <cstring>
#include <set>

namespace lmt_merge {

namespace {
    constexpr uint32_t kMagic = 0x00544D4C;     // "LMT\0"
    constexpr uint16_t kVersion = 49;
    constexpr uint32_t kBlockSize = 0xC0, kTrackSize = 0x20;
    constexpr uint32_t kBlockTracks = 0x00, kBlockTrackNum = 0x04, kBlockEvents1 = 0x74, kBlockEvents2 = 0xBC;
    constexpr uint32_t kTrackData = 0x0C;

    uint32_t rd32(const std::vector<uint8_t>& b, size_t at) { uint32_t v; std::memcpy(&v, b.data() + at, 4); return v; }
    uint16_t rd16(const std::vector<uint8_t>& b, size_t at) { uint16_t v; std::memcpy(&v, b.data() + at, 2); return v; }
    void wr32(std::vector<uint8_t>& b, size_t at, uint32_t v) { std::memcpy(b.data() + at, &v, 4); }
    void wr16(std::vector<uint8_t>& b, size_t at, uint16_t v) { std::memcpy(b.data() + at, &v, 2); }

    bool fail(std::string* why, const char* text) { if (why) *why = text; return false; }

    // Where a field to rebase sits in the file (rebasing a shared track table twice would break it).
    struct Fields {
        std::vector<uint32_t> at;           // file positions of the u32 offsets inside blocks and tracks
    };

    // Walks the file like the loader and lists every field it relocates. Checks every offset stays inside the file.
    bool scan(const std::vector<uint8_t>& b, int* count, Fields* f, std::string* why) {
        const size_t n = b.size();
        if (n < 8 || rd32(b, 0) != kMagic) return fail(why, "not an LMT (no LMT\\0 magic)");
        if (rd16(b, 4) != kVersion) return fail(why, "not LMT version 49 (the DX9 game loads 49 only)");
        const uint32_t blocks = rd16(b, 6);
        if (8 + 4ull * blocks > n) return fail(why, "block table runs past the end of the file");
        std::set<uint32_t> seen_blocks, seen_tracks;
        for (uint32_t i = 0; i < blocks; ++i) {
            const uint32_t bo = rd32(b, 8 + 4 * i);
            if (!bo) continue;
            if (bo + (uint64_t)kBlockSize > n) return fail(why, "a block offset points past the end of the file");
            if (!seen_blocks.insert(bo).second) return fail(why, "two block table entries share one block (the game's loader would relocate it twice)");
            if (f) {
                f->at.push_back(bo + kBlockTracks);
                f->at.push_back(bo + kBlockEvents1);
                f->at.push_back(bo + kBlockEvents2);
            }
            const uint32_t to = rd32(b, bo + kBlockTracks), tn = rd32(b, bo + kBlockTrackNum);
            if (to + (uint64_t)tn * kTrackSize > n) return fail(why, "a track table runs past the end of the file");
            for (uint32_t t = 0; t < tn; ++t) {
                const uint32_t tr = to + t * kTrackSize;
                if (!seen_tracks.insert(tr).second) continue;       // a table shared by two blocks: its fields once
                if (f) f->at.push_back(tr + kTrackData);
            }
        }
        if (count) *count = (int)blocks;
        return true;
    }
}

int block_count(const std::vector<uint8_t>& lmt, std::string* why) {
    int count = -1;
    return scan(lmt, &count, nullptr, why) ? count : -1;
}

bool merge(const std::vector<Piece>& pieces, int total, std::vector<uint8_t>& out, std::string* why) {
    if (total < 0 || total > kMaxBlocks) return fail(why, "more than 256 blocks in one bank");
    std::vector<int> owner(total, -1);
    for (size_t p = 0; p < pieces.size(); ++p) {
        const Piece& pc = pieces[p];
        if (pc.first < 0 || pc.count < 0 || pc.first + pc.count > total) return fail(why, "a set's blocks fall outside the bank");
        for (int i = pc.first; i < pc.first + pc.count; ++i) {
            if (owner[i] >= 0) return fail(why, "two sets placed on the same blocks");
            owner[i] = (int)p;
        }
    }
    const size_t table = 8 + 4 * (size_t)total;
    std::vector<uint8_t> o((table + 15) & ~size_t(15), 0);
    wr32(o, 0, kMagic);
    wr16(o, 4, kVersion);
    wr16(o, 6, (uint16_t)total);
    for (const Piece& pc : pieces) {
        if (!pc.lmt || pc.count == 0) continue;
        const std::vector<uint8_t>& src = *pc.lmt;
        int blocks = 0;
        Fields f;
        if (!scan(src, &blocks, &f, why)) return false;
        const uint32_t P = (uint32_t)o.size();
        o.insert(o.end(), src.begin(), src.end());
        o.resize((o.size() + 15) & ~size_t(15), 0);
        for (uint32_t at : f.at) wr32(o, P + at, rd32(o, P + at) + P);   // the copy's own block table is unused
        for (int j = 0; j < pc.count && j < blocks; ++j) {
            const uint32_t bo = rd32(src, 8 + 4 * j);
            wr32(o, 8 + 4 * (pc.first + j), bo ? bo + P : 0);
        }
    }
    out.swap(o);
    return true;
}

} // namespace lmt_merge
