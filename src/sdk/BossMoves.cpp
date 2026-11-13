#include "BossMoves.hpp"

namespace boss_moves {

namespace {
Move g_moves[kMaxMoves];
int g_count = 0;
}

int add(const Move& m) {
    if (!m.name || g_count >= kMaxMoves) return -1;
    g_moves[g_count] = m;
    return kFirstAction + g_count++;
}

int count() { return g_count; }

const Move* get(int index) { return (index >= 0 && index < g_count) ? &g_moves[index] : nullptr; }

int index_of_action(int action) {
    const int i = action - kFirstAction;
    return (i >= 0 && i < g_count) ? i : -1;
}

} // namespace boss_moves
