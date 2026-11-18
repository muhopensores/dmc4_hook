#include "Menus.hpp"

namespace menus {
namespace {

uintptr_t message() { return *reinterpret_cast<const uintptr_t*>(kMessage); }

} // namespace

bool slot_loaded(int slot) {
    const uintptr_t m = message();
    if (!m || slot < 0 || slot >= kMessageSlots) return false;
    return *reinterpret_cast<const uint8_t*>(m + kSlotStride * slot + kSlotEnabled) == 1;
}

bool print(int slot, int x, int y, unsigned w, unsigned h, MtColor color, int prio, const char* text) {
    if (!text || !slot_loaded(slot)) return false;
    MtPoint pos{x, y};
    MtSize size{w, h};
    using Fn = void(__cdecl*)(uintptr_t, uint8_t, const MtPoint*, const MtSize*, MtColor, int, const char*, ...);
    reinterpret_cast<Fn>(kMessagePrintf)(message(), static_cast<uint8_t>(slot), &pos, &size, color, prio, "%s", text);
    return true;
}

} // namespace menus
