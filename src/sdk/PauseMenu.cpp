#include "PauseMenu.hpp"
#include "Menus.hpp"
#include "RoomClasses.hpp"
#include "sArea.hpp"
#include "../utility/Patch.hpp"
#include "../mod.hpp"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>

// Notes and addresses: PauseMenu.hpp (DX9 exe, read in IDA 2026-11-19).

namespace pause_menu {
namespace {

// ---- the game ----
constexpr uintptr_t kPauseVtable = 0xBE2270;            // uPauseMenu
constexpr uintptr_t kPauseMainSlot = kPauseVtable + 17 * 4;
constexpr uintptr_t kPauseMain = 0x79FE30;              // uPauseMenu::main
constexpr uintptr_t kMapCtorCall = 0x7A01A1;            // result, case 2 / cursor 2: call uMapMgr::uMapMgr (usercall eax)
constexpr uintptr_t kMapCtor = 0x774450;
constexpr uintptr_t kMapVtable = 0xBDF978;              // uMapMgr, 24 slots
constexpr int kMapSlots = 24;
constexpr uintptr_t kMediator = 0xE558B8;               // sMediator*
constexpr uintptr_t kPad = 0xE559C4;                    // sDevil4Pad*
constexpr uintptr_t kSetFadeOut = 0x493B40;             // edi = sMediator, push float frames; retn 4
constexpr uintptr_t kSetFadeIn = 0x493BB0;              // same
constexpr uintptr_t kIsFadeEnd = 0x493C90;              // eax = sMediator; al
constexpr uintptr_t kGetCursor = 0x4C1BB0;              // uListSelect::getCursor<char>: ecx = axis; obj, rows, cursor, wrap, mouse table; retn 14h
constexpr uintptr_t kDrawRect = 0x533020;               // eax = uDevilCock, ecx = cDraw; colour, prio, 1, pos{x,y}, size{w,h}; retn 1Ch
constexpr uintptr_t kDevilCockTransMain = 0x532F80;     // uDevilCock::transMain(cDraw*), thiscall, retn 4

constexpr int kMapRow = 2;
constexpr int kLabelSprite = 11 + kMapRow, kLitSprite = 16 + kMapRow;   // the rows' label images: 11-15, highlighted 16-20

// sMediator::setFadeIn / setFadeOut (30 frames) and isFadeEnd
void fade(bool in) {
    const uintptr_t med = *reinterpret_cast<const uintptr_t*>(kMediator);
    if (!med) return;
    const uintptr_t fn = in ? kSetFadeIn : kSetFadeOut;
    const float frames = 30.0f;
    __asm {
        push edi
        mov edi, med
        push frames
        call fn
        pop edi
    }
}

bool fade_done() {
    const uintptr_t med = *reinterpret_cast<const uintptr_t*>(kMediator);
    if (!med) return true;
    uint8_t done = 0;
    __asm {
        mov eax, med
        mov ecx, kIsFadeEnd
        call ecx
        mov done, al
    }
    return done != 0;
}

// the game's cursor step: pad (system repeat bits), keyboard repeat, mouse wheel, with its cursor sound
int get_cursor(uintptr_t obj, int axis, int rows, int cursor, bool wrap) {
    int result = cursor;
    const int w = wrap ? 1 : 0;
    __asm {
        push 0
        push w
        push cursor
        push rows
        push obj
        mov ecx, axis
        mov eax, kGetCursor
        call eax
        mov result, eax
    }
    return result;
}

uint32_t pad_word(uintptr_t off) {
    const uintptr_t pad = *reinterpret_cast<const uintptr_t*>(kPad);
    return pad ? *reinterpret_cast<const uint32_t*>(pad + off) : 0;
}
bool pad_decide() { return (pad_word(0x5E8) & pad_word(0x5B4)) != 0; }   // as uListSelect::isDecide 0x76D340
bool pad_cancel() { return (pad_word(0x5E8) & pad_word(0x5E0)) != 0; }   // as uListSelect::isCancel 0x76D3D0

template <typename T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

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

// colours for menus::print: its bytes are B, G, R, A
MtColor grey(uint8_t a) { return MtColor{0x9A, 0x9A, 0x9A, a}; }
MtColor white(uint8_t a) { return MtColor{0xFF, 0xFF, 0xFF, a}; }
MtColor gold(uint8_t a) { return MtColor{0x50, 0xC8, 0xFF, a}; }   // R FF, G C8, B 50

// ---- state ----
std::mutex g_mutex;
std::shared_ptr<Row> g_row;              // set by set_row (any thread), read on the game thread
std::unique_ptr<Patch> g_ctor_patch;
uintptr_t g_main_original = kPauseMain;
bool g_installed = false;
uintptr_t g_vtable[kMapSlots];

enum class Phase { Off, Opening, Running, Closing, Done };
struct Open {
    uintptr_t unit = 0;
    std::shared_ptr<Row> row;
    Phase phase = Phase::Off;
    int cursor = 0;
    int frames = 0;
};
Open g_open;                             // the list menu (one at a time); game thread
std::atomic<bool> g_menu_open{false};

std::shared_ptr<Row> row_now() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_row;
}

bool row_here(const Row& r) {
    if (r.room < 0) return true;
    aRoom* room = room_classes::current();
    return room && room->room_no == r.room;
}

// ---- the pause menu: our label and description in the Map row's place (after its main, game thread) ----
constexpr int kPrio = 100;   // sMessage::printf priority: over the menus' layers (MenuRows, 2026-11-18)

void __stdcall after_pause_main(uintptr_t menu) {
    std::shared_ptr<Row> r = row_now();
    if (!r || !row_here(*r)) return;
    at<uint8_t>(menu, 0xF4 + kMapRow) = 1;   // selectable whatever the mission says
    sprMap* normal = sprite(menu, kLabelSprite);
    sprMap* lit = sprite(menu, kLitSprite);
    if (!normal || !lit) return;
    const bool on = at<int>(menu, 0x40) == kMapRow;
    const sprMap* shown = lit->isDisp ? lit : normal->isDisp ? normal : nullptr;
    const uint8_t alpha = shown ? shown->color.a : 0;
    const MtPoint pos = (shown ? shown : normal)->posHDTV;
    normal->isDisp = false;
    lit->isDisp = false;
    if (fontMap* f = font(menu, kMapRow)) {
        if (on && f->isDisp && !r->description.empty() && menus::load_menu_font())
            menus::print(menus::kMenuFontSlot, f->posHDTV.x, f->posHDTV.y, 26, 26, white(0xFF), kPrio, r->description.c_str());
        f->isDisp = false;
    }
    if (!alpha || at<uintptr_t>(menu, 0xF0)) return;   // faded out, or a child menu is up
    if (menus::load_menu_font())
        menus::print(menus::kMenuFontSlot, pos.x + 8, pos.y - 4, 32, 32, on ? white(alpha) : grey(alpha), kPrio, r->label.c_str());
    else
        menus::print(menus::kDebugFontSlot, pos.x, pos.y, 24, 30, on ? white(alpha) : grey(alpha), kPrio, r->label.c_str());
}

naked void pause_main_thunk() {   // thiscall (ecx = the menu)
    __asm {
        push ecx                  // kept for after_pause_main
        call dword ptr [g_main_original]
        pop ecx
        pushad
        push ecx
        call after_pause_main
        popad
        ret
    }
}

// ---- the list menu: a uMapMgr with our move / transMain / isEnd / getDecValue ----
constexpr int kTitleX = 160, kTitleY = 110, kRowX = 200, kValueX = 720, kRowY = 190, kRowStep = 44, kTextSize = 30;

void draw_menu(const Open& o) {
    const ListMenu& m = o.row->menu;
    const bool menu_font = menus::load_menu_font();
    const int slot = menu_font ? menus::kMenuFontSlot : menus::kDebugFontSlot;
    menus::print(slot, kTitleX, kTitleY, 40, 40, white(0xFF), kPrio, m.title.c_str());
    const int n = m.rows ? m.rows() : 0;
    std::string label, value;
    for (int i = 0; i < n; ++i) {
        label.clear();
        value.clear();
        if (m.text) m.text(i, label, value);
        const bool on = i == o.cursor;
        const int y = kRowY + i * kRowStep;
        menus::print(slot, kRowX, y, kTextSize, kTextSize, on ? gold(0xFF) : grey(0xFF), kPrio, label.c_str());
        if (!value.empty()) {
            const std::string shown = on ? "[  " + value + "  ]" : value;   // moji00 has no < >
            menus::print(slot, on ? kValueX - 22 : kValueX, y, kTextSize, kTextSize, on ? white(0xFF) : grey(0xFF), kPrio, shown.c_str());
        }
    }
    menus::print(slot, kRowX, kRowY + n * kRowStep + 24, 24, 24, grey(0xFF), kPrio, "Left / Right: change      A: select      B: back");
}

void __stdcall list_move(uintptr_t unit) {
    Open& o = g_open;
    if (o.unit != unit || !o.row) return;
    ++o.frames;
    const ListMenu& m = o.row->menu;
    switch (o.phase) {
    case Phase::Opening:
        fade(true);   // the pause menu faded to black before making us
        o.phase = Phase::Running;
        break;
    case Phase::Running: {
        const int n = m.rows ? m.rows() : 0;
        if (n > 0) {
            if (o.cursor >= n) o.cursor = n - 1;
            o.cursor = get_cursor(unit, 0, n, o.cursor, true);
            std::string label, value;
            if (m.text) m.text(o.cursor, label, value);
            if (!value.empty() && m.change) {
                const int step = get_cursor(unit, 1, 3, 1, false) - 1;   // left / right as -1 / +1
                if (step) m.change(o.cursor, step);
            }
            if (o.frames > 2 && pad_decide() && m.decide) m.decide(o.cursor);
        }
        if (o.frames > 2 && pad_cancel()) {
            fade(false);
            o.phase = Phase::Closing;
        }
        break;
    }
    case Phase::Closing:
        if (fade_done()) o.phase = Phase::Done;
        break;
    default:
        break;
    }
    if (o.phase == Phase::Running || o.phase == Phase::Closing) draw_menu(o);
}

bool __stdcall list_is_end(uintptr_t unit) { return g_open.unit != unit || g_open.phase == Phase::Done; }

void __stdcall list_trans(uintptr_t unit, uintptr_t draw) {
    if (g_open.unit == unit && (g_open.phase == Phase::Running || g_open.phase == Phase::Closing)) {
        const uint32_t colour = 0xA0000000;   // black, a little darker than uPracPlSet's 0x90 backdrop
        __asm {
            sub esp, 8
            mov dword ptr [esp], 1280         // size (pushed first: the deepest argument)
            mov dword ptr [esp + 4], 720
            sub esp, 8
            mov dword ptr [esp], 0            // position
            mov dword ptr [esp + 4], 0
            push 1
            push 300                          // priority, as uPracPlSet's backdrop
            push colour
            mov eax, unit
            mov ecx, draw
            mov edx, kDrawRect
            call edx
        }
    }
    // not uDevilCock::transMain (kDevilCockTransMain): it would draw the layout uMapMgr loads, under our rows
}

naked void list_move_thunk() {
    __asm {
        pushad
        push ecx
        call list_move
        popad
        ret
    }
}
naked void list_is_end_thunk() {
    __asm {
        push ecx
        call list_is_end
        ret
    }
}
naked void list_dec_value_thunk() {   // 0: back to the pause menu
    __asm {
        xor eax, eax
        ret
    }
}
naked void list_trans_thunk() {   // thiscall (cDraw*), retn 4
    __asm {
        push ebx
        mov ebx, [esp + 8]
        pushad
        push ebx
        push ecx
        call list_trans
        popad
        pop ebx
        ret 4
    }
}

// result, case 2 / cursor 2: the uMapMgr is built; ours when our row is here
uintptr_t __stdcall after_map_ctor(uintptr_t unit) {
    std::shared_ptr<Row> r = row_now();
    if (!unit || !r || !row_here(*r)) return unit;
    *reinterpret_cast<uintptr_t*>(unit) = reinterpret_cast<uintptr_t>(g_vtable);
    g_open = Open{};
    g_open.unit = unit;
    g_open.row = r;
    g_open.phase = Phase::Opening;
    g_menu_open = true;
    return unit;
}

naked void map_ctor_thunk() {   // usercall: eax = the object; returns it in eax
    __asm {
        mov edx, kMapCtor
        call edx
        push ecx
        push edx
        push eax
        call after_map_ctor
        pop edx
        pop ecx
        ret
    }
}

bool code_matches() {
    const uint8_t call[5] = {0xE8, 0xAA, 0x42, 0xFD, 0xFF};   // call 0x774450
    return std::memcmp(reinterpret_cast<const void*>(kMapCtorCall), call, 5) == 0 &&
           *reinterpret_cast<const uintptr_t*>(kPauseMainSlot) == kPauseMain &&
           *reinterpret_cast<const uintptr_t*>(kMapVtable + 6 * 4) == 0x774720;
}

} // namespace

bool install() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_installed) return true;
    if (!code_matches()) {
        spdlog::error("PauseMenu: the pause menu isn't the expected code; no pause rows");
        return false;
    }
    std::memcpy(g_vtable, reinterpret_cast<const void*>(kMapVtable), sizeof g_vtable);
    g_vtable[6] = reinterpret_cast<uintptr_t>(&list_move_thunk);
    g_vtable[9] = reinterpret_cast<uintptr_t>(&list_trans_thunk);    // draw: uMapMgr draws from here (its own slot), not transMain
    g_vtable[19] = reinterpret_cast<uintptr_t>(&list_trans_thunk);
    g_vtable[20] = reinterpret_cast<uintptr_t>(&list_is_end_thunk);
    g_vtable[21] = reinterpret_cast<uintptr_t>(&list_dec_value_thunk);
    const int32_t rel = static_cast<int32_t>(reinterpret_cast<uintptr_t>(&map_ctor_thunk) - (kMapCtorCall + 5));
    g_ctor_patch = Patch::create(kMapCtorCall, {0xE8, int16_t(rel & 0xFF), int16_t((rel >> 8) & 0xFF),
                                                int16_t((rel >> 16) & 0xFF), int16_t((rel >> 24) & 0xFF)}, true);
    auto* slot = reinterpret_cast<uintptr_t*>(kPauseMainSlot);
    DWORD old = 0;
    if (VirtualProtect(slot, sizeof *slot, PAGE_READWRITE, &old)) {
        *slot = reinterpret_cast<uintptr_t>(&pause_main_thunk);
        VirtualProtect(slot, sizeof *slot, old, &old);
    }
    g_installed = true;
    return true;
}

void set_row(const Row& row) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_row = std::make_shared<Row>(row);
}

void clear_row() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_row.reset();
}

bool menu_open() { return g_open.phase != Phase::Off && g_open.phase != Phase::Done; }

} // namespace pause_menu
