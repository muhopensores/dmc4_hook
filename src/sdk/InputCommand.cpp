#include "InputCommand.hpp"
#include <vector>
#include <cstring>

#define naked __declspec(naked)

namespace input_commands {

uintptr_t jmp_ret = 0;
bool hook_installed = false;

namespace {

constexpr int kPlayerSlots = 4;    // uPlayer objects tracked at once (the player, NPC / boss Dante, a co-op partner ...)

struct Entry {
    const kCmdHead* head = nullptr;
    int bit = 0;
    const bool* enabled = nullptr;
    Predicate predicate = nullptr;
    PredicateCtx pred_ctx = nullptr;
    void* ctx = nullptr;
    uintptr_t players[kPlayerSlots] = {};
    uint32_t  seen[kPlayerSlots] = {};
    kSPlCmd   states[kPlayerSlots] = {};
    uint32_t  tick = 0;

    // The state for `player`: the slot seen longest ago is evicted when the table is full (dead players are never
    // reported, and a live one is seen every tick and keeps its slot).
    kSPlCmd& state_for(uintptr_t player) {
        ++tick;
        for (int i = 0; i < kPlayerSlots; ++i) if (players[i] == player) { seen[i] = tick; return states[i]; }
        int victim = 0;
        for (int i = 0; i < kPlayerSlots; ++i) {
            if (players[i] == 0) { victim = i; break; }
            if (seen[i] < seen[victim]) victim = i;
        }
        players[victim] = player; seen[victim] = tick; states[victim] = kSPlCmd{};
        return states[victim];
    }
};

std::vector<Entry>& entries() { static std::vector<Entry> v; return v; }

uintptr_t fn_commandCheckSub = 0x00805A10;

} // namespace

bool add(const kCmdHead* head, int bit, const bool* enabled, Predicate predicate) {
    if (!head || bit < FIRST_FREE_BIT || bit > LAST_BIT) return false;
    for (const Entry& e : entries()) if (e.bit == bit) return false;
    Entry e; e.head = head; e.bit = bit; e.enabled = enabled; e.predicate = predicate;
    entries().push_back(e);
    return true;
}

bool add(const kCmdHead* head, int bit, const bool* enabled, PredicateCtx predicate, void* ctx) {
    if (!head || bit < FIRST_FREE_BIT || bit > LAST_BIT) return false;
    for (const Entry& e : entries()) if (e.bit == bit) return false;
    Entry e; e.head = head; e.bit = bit; e.enabled = enabled; e.pred_ctx = predicate; e.ctx = ctx;
    entries().push_back(e);
    return true;
}

int next_free_bit() {
    for (int bit = FIRST_FREE_BIT; bit <= LAST_BIT; ++bit) {
        bool taken = false;
        for (const Entry& e : entries()) if (e.bit == bit) { taken = true; break; }
        if (!taken) return bit;
    }
    return -1;
}

int command_check_sub(uintptr_t player, kSPlCmd* pstate, const kCmdHead* phead, const kCmdData* psteps) {   // (`st` is a MASM register name)
    int r;
    _asm {
        pushad
        mov esi, player
        push psteps
        push phead
        push pstate
        push esi
        call fn_commandCheckSub
        movzx eax, al
        mov r, eax
        popad
    }
    return r;
}

void __stdcall run(uintptr_t player) {
    if (!player) return;
    const uint32_t btn_on = *(uint32_t*)(player + 0x140C);          // cInterface::mBtnOn
    uint32_t* cmd_flag = (uint32_t*)(player + 0x2184);              // mCmdFlag[3]
    for (Entry& e : entries()) {
        kSPlCmd& st = e.state_for(player);
        if (e.enabled && !*e.enabled) { st = kSPlCmd{}; continue; }
        if (e.predicate && !e.predicate(player)) { st = kSPlCmd{}; continue; }
        if (e.pred_ctx && !e.pred_ctx(player, e.ctx)) { st = kSPlCmd{}; continue; }
        // the head's condition, as executeCommandCheck applies it: failing it resets the command's state
        bool ok = true;
        switch (e.head->condition) {
        case CND_POSE:  ok = (btn_on & 0x10) != 0; break;
        case CND_DASH:  ok = (btn_on & 0x200) != 0; break;
        case CND_LASER: ok = (btn_on & 0x400) != 0; break;
        default: break;
        }
        if (!ok) { st = kSPlCmd{}; continue; }
        if (command_check_sub(player, &st, e.head, e.head->data) == 1)
            cmd_flag[e.bit / 32] |= 1u << (e.bit % 32);
    }
}

naked void detour() {
    _asm {
        pushad
        push edi
        call run                // __stdcall: pops its argument
        popad
        mov edx, [edi]
        mov eax, [edx + 0x248]
        jmp dword ptr [jmp_ret]
    }
}

} // namespace input_commands
