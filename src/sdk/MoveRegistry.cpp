#include "MoveRegistry.hpp"

namespace move_registry {

namespace {
    Entry g_entries[kMaxMoves];
    int g_count = 0;

    int find(int character, uint32_t id) {
        for (int i = 0; i < g_count; ++i)
            if (g_entries[i].character == character && g_entries[i].id == id) return i;
        return -1;
    }
}

bool register_move(int character, uint32_t id, const bool* enabled, StateFn fn) {
    int i = find(character, id);
    if (i < 0) {
        if (g_count >= kMaxMoves) return false;
        i = g_count++;
    }
    g_entries[i] = Entry{ character, id, enabled, fn };
    return true;
}

void unregister_move(int character, uint32_t id) {
    const int i = find(character, id);
    if (i < 0) return;
    for (int j = i + 1; j < g_count; ++j) g_entries[j - 1] = g_entries[j];
    --g_count;
}

bool __stdcall id_allowed(uint32_t id) {
    for (int i = 0; i < g_count; ++i) {
        const Entry& e = g_entries[i];
        if (e.id == id && (!e.enabled || *e.enabled)) return true;
    }
    return false;
}

void __stdcall dispatch(int character, uint32_t id, uintptr_t player) {
    const int i = find(character, id);
    if (i < 0) return;
    const Entry& e = g_entries[i];
    if (e.fn && (!e.enabled || *e.enabled)) e.fn(player, id);
}

int count() { return g_count; }
const Entry& entry(int i) { return g_entries[i]; }

} // namespace move_registry
