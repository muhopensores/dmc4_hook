#include "MissionMenu.hpp"
#include "RoomClasses.hpp"
#include "../mods/AreaJump.hpp"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <mutex>

namespace mission_menu {
namespace {

constexpr uintptr_t kVtable   = 0xBDFD88;           // uMissionMenu
constexpr uintptr_t kMainSlot = kVtable + 17 * 4;
constexpr uintptr_t kMain     = 0x778490;           // uMissionMenu::main (the slot's stock value)
constexpr uintptr_t kSave     = 0xE558C8;           // sSave*: +0x98 the menu's start cursor (the last played difficulty)

uintptr_t g_main_original = kMain;
std::atomic<int> g_room_request{-1};
std::atomic<uint64_t> g_last_seen{0};               // GetTickCount64 of the menu's last frame
std::mutex g_mutex;
bool g_installed = false;

template <typename T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// the menu is selecting: put it on a difficulty row, past the costume choice, with aGame's jump set
void start_into_room(uintptr_t menu, int room) {
    int& cursor = at<int>(menu, 0x40);
    int d = cursor;
    if (d < 0 || d > 6) {
        const uintptr_t save = *reinterpret_cast<const uintptr_t*>(kSave);
        d = save ? at<int>(save, 0x98) : 0;
        if (d < 0 || d > 6) d = 0;
    }
    if (!at<uint8_t>(menu, 0x9C + d)) d = 0;   // a locked difficulty: Human
    if (!AreaJump::jump_to_room_id(room)) {
        spdlog::warn("MissionMenu: no room {} to start in", room);
        return;
    }
    cursor = d;
    at<uint8_t>(menu, 0x14) = 2;   // decided (uListSelect::main's state 2)
    at<uint8_t>(menu, 0x15) = 0;
    at<uint8_t>(menu, 0x16) = 0;   // result step 0: the row's action
    at<uint8_t>(menu, 0x17) = 2;   // a difficulty row's last step: fade out, setDanteStyle, then isEnd
    spdlog::info("MissionMenu: starting mission {} (difficulty row {}) in room {}", at<int>(menu, 0x70 + 4 * d), d, room);
}

// before uMissionMenu::main, every frame the menu runs (game thread)
void __stdcall before_main(uintptr_t menu) {
    g_last_seen = GetTickCount64();
    if (g_room_request.load() >= 0 && at<uint8_t>(menu, 0x14) == 1) start_into_room(menu, g_room_request.exchange(-1));
}

naked void main_thunk() {   // thiscall (ecx = the menu); registers kept for the game's main
    __asm {
        pushad
        push ecx
        call before_main
        popad
        jmp dword ptr [g_main_original]
    }
}

} // namespace

bool install() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_installed) return true;
    auto* slot = reinterpret_cast<uintptr_t*>(kMainSlot);
    if (*slot != kMain) {
        spdlog::error("MissionMenu: uMissionMenu::main's slot isn't the game's (0x{:X}); no menu-into-room", *slot);
        return false;
    }
    DWORD old = 0;
    if (!VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old)) return false;
    *slot = reinterpret_cast<uintptr_t>(&main_thunk);
    VirtualProtect(slot, sizeof *slot, old, &old);
    g_installed = true;
    return true;
}

bool in_mission_menu() { return GetTickCount64() - g_last_seen.load() < 200; }

bool enter_room(int room) {
    if (!AreaJump::is_valid_room_id(room)) return false;
    if (room_classes::current()) return AreaJump::jump_to_room_id(room);
    if (!install() || !in_mission_menu()) return false;
    g_room_request = room;   // taken by the menu's next frame (before_main)
    return true;
}

} // namespace mission_menu
