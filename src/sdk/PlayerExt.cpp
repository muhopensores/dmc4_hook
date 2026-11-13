#include "PlayerExt.hpp"
#include "uPlayer.hpp"
#include <cstddef>
#include <mutex>
#include <unordered_map>

#define naked __declspec(naked)

namespace player_ext {

static_assert(offsetof(DanteExt, jdc_flag) == kJdcFlag && offsetof(DanteExt, jdc_ready) == kJdcReady &&
              offsetof(DanteExt, jdc_pos) == kJdcPos && offsetof(DanteExt, jdc_hits_left) == kJdcHitsLeft &&
              offsetof(DanteExt, jdc_offset) == kJdcOffset, "DanteExt offsets");

// the slots are the HOOK_* members of the SDK's player structs (sdk/uPlayer.hpp)
static_assert(kPlayerSlot == player_mod_bytes::kUPlayerExt && kDanteSlot == player_mod_bytes::kDanteExt &&
              kNeroSlot == player_mod_bytes::kNeroExt, "player_ext slots: the HOOK_ members in sdk/uPlayer.hpp");

bool hook_installed = false;
uintptr_t jmp_ret_setup = 0;
uintptr_t jmp_ret_dtor = 0;

namespace {

constexpr uintptr_t kPlayerId = 0x1494;   // uPlayer::mPlayerID: 0 Dante, 1 Nero

struct Blocks {
    UPlayerExt* player = nullptr;
    DanteExt* dante = nullptr;
    NeroExt* nero = nullptr;
};

std::mutex g_mutex;
std::unordered_map<uintptr_t, Blocks>& registry() { static std::unordered_map<uintptr_t, Blocks> m; return m; }

template <class T> T* fresh(T* p) {
    if (p) { *p = T{}; return p; }
    return new T{};
}

template <class T> inline T& slot(uintptr_t player, uintptr_t off) { return *reinterpret_cast<T*>(player + off); }

void __stdcall on_setup(uintptr_t player) {
    if (!player) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    Blocks& b = registry()[player];
    b.player = fresh(b.player);
    slot<UPlayerExt*>(player, kPlayerSlot) = b.player;
    switch (slot<int32_t>(player, kPlayerId)) {
    case 0:
        b.dante = fresh(b.dante);
        slot<DanteExt*>(player, kDanteSlot) = b.dante;
        break;
    case 1:
        b.nero = fresh(b.nero);
        slot<NeroExt*>(player, kNeroSlot) = b.nero;
        break;
    default:
        break;
    }
}

void __stdcall on_destroy(uintptr_t player) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = registry().find(player);
    if (it == registry().end()) return;
    slot<UPlayerExt*>(player, kPlayerSlot) = nullptr;
    if (it->second.dante) slot<DanteExt*>(player, kDanteSlot) = nullptr;
    if (it->second.nero) slot<NeroExt*>(player, kNeroSlot) = nullptr;
    delete it->second.player;
    delete it->second.dante;
    delete it->second.nero;
    registry().erase(it);
}

const Blocks* find(uintptr_t player) {
    auto it = registry().find(player);
    return it == registry().end() ? nullptr : &it->second;
}

} // namespace

UPlayerExt* player(uintptr_t p) {
    std::lock_guard<std::mutex> lock(g_mutex);
    const Blocks* b = find(p);
    return b ? b->player : nullptr;
}

DanteExt* dante(uintptr_t p) {
    std::lock_guard<std::mutex> lock(g_mutex);
    const Blocks* b = find(p);
    return b ? b->dante : nullptr;
}

NeroExt* nero(uintptr_t p) {
    std::lock_guard<std::mutex> lock(g_mutex);
    const Blocks* b = find(p);
    return b ? b->nero : nullptr;
}

naked void detour_setup() {
    __asm {
        pushad
        push ecx
        call on_setup               // __stdcall: pops its argument
        popad
        push ebp                    // the replaced bytes
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [jmp_ret_setup]
    }
}

naked void detour_dtor() {
    __asm {
        pushad
        push ecx
        call on_destroy
        popad
        push ecx                    // the replaced bytes
        push ebx
        push ebp
        push esi
        mov esi, ecx
        jmp dword ptr [jmp_ret_dtor]
    }
}

} // namespace player_ext
