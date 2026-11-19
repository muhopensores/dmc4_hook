#include "MissionMenu.hpp"
#include "Menus.hpp"
#include "RoomClasses.hpp"
#include "../mods/AreaJump.hpp"
#include "../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>

namespace mission_menu {
namespace {

constexpr uintptr_t kVtable   = 0xBDFD88;           // uMissionMenu
constexpr uintptr_t kMainSlot = kVtable + 17 * 4;
constexpr uintptr_t kMain     = 0x778490;           // uMissionMenu::main (the slot's stock value)
constexpr uintptr_t kSave     = 0xE558C8;           // sSave*: +0x98 the menu's start cursor (the last played difficulty)
constexpr uintptr_t kResultSite = 0x778ADD;         // result, case 0: mov ecx, [ebp+40h] / cmp ecx, 0Eh (6 bytes)
constexpr uintptr_t kResultDefault = 0x7791AC;      // the switch's default: back to selecting
constexpr uint8_t kResultBytes[6] = {0x8B, 0x4D, 0x40, 0x83, 0xF9, 0x0E};

// the row a plugin can take over: 12 My History (rows: 0-6 difficulties, 7 Bloody Palace, 8 Story Theater, 9 Total
// Ranking, 10 Gallery, 11 Library, 12 My History, 13 Options, 14 Save / Load; Vibed\RE\menus.md "uMissionMenu")
constexpr int kRow = kRowMyHistory;
constexpr int kNormalSprite = 7 + kRow, kLitSprite = 65 + kRow;   // its label images (normal / selected)
constexpr int kInfoX = 638, kInfoY = 475;            // the description box (the font entries' position)
constexpr int kPrio = 100;                           // sMessage::printf priority: over the menu's layers

uintptr_t g_main_original = kMain;
uintptr_t g_ret_result = 0;
uintptr_t g_ret_result_default = kResultDefault;
std::unique_ptr<FunctionHook> g_hook_result;
std::atomic<int> g_room_request{-1};
std::atomic<uint64_t> g_last_seen{0};               // GetTickCount64 of the menu's last frame
std::mutex g_mutex;
bool g_installed = false;
std::shared_ptr<Row> g_row;                          // set_row (any thread); read on the game thread
volatile bool g_row_on = false;                      // read by the result detour

template <typename T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

std::shared_ptr<Row> row_now() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_row;
}

sprMap* sprite(uintptr_t menu, uint32_t i) {
    const uintptr_t rs = at<uintptr_t>(menu, 0x18), spr = at<uintptr_t>(menu, 0x1C);
    if (!rs || !spr || i >= at<uint32_t>(rs, 0x68)) return nullptr;
    return reinterpret_cast<sprMap*>(spr + 0x48 * i);
}

fontMap* font(uintptr_t menu, uint32_t i) {
    const uintptr_t rs = at<uintptr_t>(menu, 0x18), fnt = at<uintptr_t>(menu, 0x24);
    if (!rs || !fnt || i >= at<uint32_t>(rs, 0x6C)) return nullptr;
    return reinterpret_cast<fontMap*>(fnt + 0x30 * i);
}

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

// after it: the plugin's row in My History's place (its label images and preview picture hidden, ours drawn)
void __stdcall after_main(uintptr_t menu) {
    std::shared_ptr<Row> r = row_now();
    if (!r) return;
    sprMap* normal = sprite(menu, kNormalSprite);
    sprMap* lit = sprite(menu, kLitSprite);
    if (!normal || !lit) return;
    const bool on = at<int>(menu, 0x40) == kRow;
    const sprMap* shown = lit->isDisp ? lit : normal->isDisp ? normal : nullptr;
    const uint8_t alpha = shown ? shown->color.a : 0;
    const MtPoint pos = (shown ? shown : normal)->posHDTVOrg;
    normal->isDisp = false;
    lit->isDisp = false;
    if (on) {
        at<uint32_t>(menu, 0x120) = 0;   // no preview picture (My History's)
        if (fontMap* f = font(menu, kRow)) f->isDisp = false;   // nor its description
    }
    if (!alpha) return;
    const uint8_t v = on ? 0xFF : 0x9A;
    if (menus::load_menu_font()) {   // the menus' own font
        menus::print(menus::kMenuFontSlot, pos.x + 8, pos.y - 16, 32, 32, MtColor{v, v, v, alpha}, kPrio, r->label.c_str());
        if (on && !r->description.empty())
            menus::print(menus::kMenuFontSlot, kInfoX, kInfoY, 26, 26, MtColor{0xFF, 0xFF, 0xFF, alpha}, kPrio,
                         r->description.c_str());
        return;
    }
    menus::print(menus::kDebugFontSlot, pos.x, pos.y, 24, 30, MtColor{v, v, v, alpha}, kPrio, r->label.c_str());
}

// result: the row was decided (game thread); the switch's default then goes back to selecting
void __stdcall on_decide(uintptr_t menu) {
    (void)menu;
    std::shared_ptr<Row> r = row_now();
    if (r && r->decide) r->decide();
}

naked void main_thunk() {   // thiscall (ecx = the menu)
    __asm {
        pushad
        push ecx
        call before_main
        popad
        push ecx                  // kept for after_main
        call dword ptr [g_main_original]
        pop ecx
        pushad
        push ecx
        call after_main
        popad
        ret
    }
}

naked void detour_result() {   // ebp = the menu
    __asm {
        mov ecx, [ebp + 0x40]
        cmp ecx, 12               // kRow
        jne stock
        cmp byte ptr [g_row_on], 0
        je stock
        pushad
        push ebp
        call on_decide
        popad
        jmp dword ptr [g_ret_result_default]
    stock:
        cmp ecx, 0x0E
        jmp dword ptr [g_ret_result]   // ja default: the flags of this cmp
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
    if (std::memcmp(reinterpret_cast<const void*>(kResultSite), kResultBytes, sizeof kResultBytes) == 0) {
        g_hook_result = std::make_unique<FunctionHook>(kResultSite, reinterpret_cast<uintptr_t>(&detour_result));
        if (g_hook_result->create()) g_ret_result = kResultSite + sizeof kResultBytes;
        else g_hook_result.reset();
    }
    if (!g_hook_result) spdlog::error("MissionMenu: couldn't hook the menu's result (0x{:X}); no rows of plugins", kResultSite);
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

bool set_row(const Row& row) {
    if (!install() || !g_hook_result) return false;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_row = std::make_shared<Row>(row);
    g_row_on = true;
    return true;
}

void clear_row() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_row.reset();
    g_row_on = false;
}

} // namespace mission_menu
