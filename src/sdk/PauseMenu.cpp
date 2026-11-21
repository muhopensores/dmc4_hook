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
#include <iterator>
#include <map>
#include <mutex>

// Notes and addresses: PauseMenu.hpp (DX9 exe, read in IDA 2026-11-19).

namespace pause_menu {
namespace {

// ---- the game ----
constexpr uintptr_t kPauseVtable = 0xBE2270;            // uPauseMenu
constexpr uintptr_t kPauseMainSlot = kPauseVtable + 17 * 4;
constexpr uintptr_t kPauseTransBeforeSlot = kPauseVtable + 14 * 4;
constexpr uintptr_t kPauseTransBefore = 0x7A0630;   // uPauseMenu::transBefore: draw only while no child (mpChild == 0)
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
uintptr_t g_trans_before_original = kPauseTransBefore;
volatile bool g_list_shown = false;     // the list menu is on screen: the pause menu draws under it (its frame, title, prompts)
bool g_installed = false;
uintptr_t g_vtable[kMapSlots];

enum class Phase { Off, Opening, Running, Closing, Done };
struct Open {
    uintptr_t unit = 0;
    std::shared_ptr<Row> row;
    Phase phase = Phase::Off;
    int cursor = 0;
    int frames = 0;
    int page = 0;
    std::map<int, int> cursors;   // per page
    int top = 0;                  // the first row shown (lists longer than the window scroll)
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

// The pause layout (dumped in the game 2026-11-19): sprite 0 the title, 2 / 4 background and ornament, 3 the description
// box, 11-15 the rows' labels, 16-20 the cursor's highlight bar on each row; font entries 0-5 the description texts.
constexpr int kDescriptionBox = 3, kFirstRowSprite = 11, kLastRowSprite = 20, kFonts = 6;
constexpr int kArrowSprite = 16;   // row 0's highlight arrow, borrowed for the list's cursor

uintptr_t g_pause = 0;   // the pause menu our row was last drawn in (the list menu's parent); game thread
// what hide_pause_rows changed, put back when the list closes (the pause menu goes back to selecting without its intro)
bool g_saved = false, g_saved_fonts[kFonts] = {};
sprMap g_saved_sprites[kLastRowSprite + 1];   // whole: the list borrows them as image rows (frame, position, colour)
uint16_t g_saved_title = 0;   // the title sprite's frame (Row::title_image swaps it while the list is up)
constexpr int kTitleSprite = 0;

void save_pause_rows(uintptr_t menu) {
    for (int i = 0; i <= kLastRowSprite; ++i)
        if (sprMap* s = sprite(menu, i)) g_saved_sprites[i] = *s;
    for (int i = 0; i < kFonts; ++i)
        if (fontMap* f = font(menu, i)) g_saved_fonts[i] = f->isDisp;
    if (sprMap* t = sprite(menu, kTitleSprite)) g_saved_title = t->sprId;
    g_saved = true;
}

void restore_pause_rows(uintptr_t menu) {
    if (!g_saved) return;
    g_saved = false;
    for (int i = 0; i <= kLastRowSprite; ++i)
        if (sprMap* s = sprite(menu, i)) *s = g_saved_sprites[i];
    for (int i = 0; i < kFonts; ++i)
        if (fontMap* f = font(menu, i)) f->isDisp = g_saved_fonts[i];
    if (sprMap* arrow = sprite(menu, kArrowSprite)) arrow->posHDTV = arrow->posHDTVOrg;
    if (sprMap* t = sprite(menu, kTitleSprite)) t->sprId = g_saved_title;
}

// our list is up: the pause menu is its frame (title, background, prompts), not its rows. Done from the list's own
// move: the pause menu doesn't run its main while a child is up.
void hide_pause_rows(uintptr_t menu) {
    if (sprMap* s = sprite(menu, kDescriptionBox)) s->isDisp = false;
    for (int i = 5; i <= 8; ++i)   // hidden icons, borrowed as value images
        if (sprMap* s = sprite(menu, i)) s->isDisp = false;
    for (int i = kFirstRowSprite; i <= kLastRowSprite; ++i)
        if (sprMap* s = sprite(menu, i)) s->isDisp = false;
    for (int i = 0; i < kFonts; ++i)
        if (fontMap* f = font(menu, i)) f->isDisp = false;
}

void __stdcall after_pause_main(uintptr_t menu) {
    g_pause = menu;
    std::shared_ptr<Row> r = row_now();
    if (!r || !row_here(*r)) return;
    at<uint8_t>(menu, 0xF4 + kMapRow) = 1;   // selectable whatever the mission says
    sprMap* normal = sprite(menu, kLabelSprite);   // the "Map" label image; its highlight bar (kLitSprite) stays
    if (!normal) return;
    const bool on = at<int>(menu, 0x40) == kMapRow;
    const uint8_t alpha = normal->isDisp ? normal->color.a : 0;
    const MtPoint pos = normal->posHDTV;
    const bool image = r->label_image >= 0;
    if (image) {   // a frame of the pause sheet in the label's place (narrowed: the frame is edited, shared by the .anm)
        normal->sprId = static_cast<uint16_t>(r->label_image);
        if (r->label_image_width > 0)
            if (ANM_SPRITE* f = menus::anm_sprite(reinterpret_cast<uSprLayout*>(menu), r->label_image))
                f->m_sizeW = static_cast<int16_t>(r->label_image_width);
    } else {
        normal->isDisp = false;
    }
    if (fontMap* f = font(menu, kMapRow)) {
        if (on && f->isDisp && !r->description.empty() && menus::load_menu_font())
            menus::print(menus::kMenuFontSlot, f->posHDTV.x, f->posHDTV.y, 26, 26, white(0xFF), kPrio, r->description.c_str());
        f->isDisp = false;
    }
    if (image || !alpha || at<uintptr_t>(menu, 0xF0)) return;   // an image, faded out, or a child menu is up
    if (menus::load_menu_font())
        menus::print(menus::kMenuFontSlot, pos.x + 8, pos.y - 4, 32, 32, on ? white(alpha) : grey(alpha), kPrio, r->label.c_str());
    else
        menus::print(menus::kDebugFontSlot, pos.x, pos.y, 24, 30, on ? white(alpha) : grey(alpha), kPrio, r->label.c_str());
}

naked void pause_trans_before_thunk() {   // thiscall, al: draw this frame
    __asm {
        cmp byte ptr [g_list_shown], 0
        je stock
        mov al, 1
        ret
    stock:
        jmp dword ptr [g_trans_before_original]
    }
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
// inside the pause menu's window, where its rows and description box were (HDTV coordinates). Text pages: rows 30 apart.
// Pages with images use the pause menu's own spacing: label anchors at x 280 (305 selected), centres 284 + 56 n.
constexpr int kTitleX = 640, kTitleY = 166, kRowX = 300, kValueX = 660, kRowY = 262, kRowStep = 30, kTextSize = 26;
constexpr int kImageRowX = 280, kImageRowSelX = 305, kImageRowY = 284, kImageRowStep = 56, kImageValueX = 640;
// the pause sprites shown as our rows' images while the list is up (hidden otherwise; all restored when it closes)
const int kLabelPool[] = {11, 12, 13, 14, 15};
const int kValuePool[] = {5, 6, 7, 8, 17, 18, 19, 20};
constexpr int kArrowLeft = 44, kArrowRight = 43;   // the pause sheet's ◁ / ▷ (the cut practice menus' value arrows)
constexpr int kTextVisible = 7, kImageVisible = 5;   // rows in the window; longer pages scroll

// ---- the scroll element: the settings menus' own, from the key config's layout (uOptionConfigKey, DX9 0x791A30 /
// 0x791AE0: arc rom_pc\id\option\option_config_key, layout 0x1B9 = id\pc\opt_key_pc, sheet id\pc\key_pc). Its sprites
// 29 (up arrow, frame 72), 30 (down arrow, 73) and 31 (the bar, frame 68, 256 tall at a 3712 / 4096 scale = 232 px);
// moveScrIcon / init: the bar is scaled to visible / rows and moves along a 200-px track by top / (rows - visible).
// The list menu's unit (a uDevilCock) loads that layout when the arc is in and shows only those three. ----
constexpr uintptr_t kResourceInstance = 0xE552D0;   // sDevil4Resource*
constexpr uintptr_t kResCreate = 0x8DF530;          // sResource::create: eax = instance; DTI, path, flags; retn 0Ch
constexpr uintptr_t kResRelease = 0x8DDA00;         // cResource::release: eax = instance, edi = the resource
constexpr uintptr_t kArchiveDti = 0xEAD4A0;         // rArchive::DTI
constexpr uintptr_t kSetData = 0x85FCD0;            // uSprLayout::setData(this, id): stack, retn 8
constexpr uintptr_t kLoadResource = 0x532E60;       // uDevilCock::loadResource: thiscall
constexpr uintptr_t kSetPrio2 = 0x4AD7C0;           // uDevilCock::setResourcePrio2: edi = this, push prio; retn 4
const char* const kScrollArc = "rom_pc\\id\\option\\option_config_key";
constexpr uint32_t kScrollLayout = 0x1B9;
constexpr int kScrollUp = 29, kScrollDown = 30, kScrollBar = 31, kScrollX = 1010;
uintptr_t g_scroll_arc = 0;
bool g_scroll_ready = false;

uintptr_t resource_create(uintptr_t dti, const char* path, uint32_t flags) {
    const uintptr_t inst = *reinterpret_cast<const uintptr_t*>(kResourceInstance);
    uintptr_t r = 0;
    if (!inst) return 0;
    __asm {
        push flags
        push path
        push dti
        mov eax, inst
        mov ecx, kResCreate
        call ecx
        mov r, eax
    }
    return r;
}

void resource_release(uintptr_t res) {
    const uintptr_t inst = *reinterpret_cast<const uintptr_t*>(kResourceInstance);
    if (!inst || !res) return;
    __asm {
        push edi
        mov eax, inst
        mov edi, res
        mov ecx, kResRelease
        call ecx
        pop edi
    }
}

// the arc in: the unit takes the key config's layout (as uOptionConfigKey::loadResource, without its fonts)
void load_scroll_layout(uintptr_t unit) {
    if (g_scroll_ready || !g_scroll_arc || !(at<uint32_t>(g_scroll_arc, 0x4C) & 1)) return;
    const uint32_t id = kScrollLayout;
    __asm {
        push id
        push unit
        mov eax, kSetData
        call eax
        mov ecx, unit
        mov eax, kLoadResource
        call eax
        push edi
        mov edi, unit
        push 0x64
        mov eax, kSetPrio2
        call eax
        pop edi
    }
    g_scroll_ready = at<uintptr_t>(unit, 0x18) != 0;
}

// only the up / down arrows and the bar, at the list's right, for a list of n rows showing `visible` from `top`
void update_scroll(uintptr_t unit, int n, int visible, int top, int window_top, int window_h) {
    const uintptr_t rs = at<uintptr_t>(unit, 0x18);
    if (!g_scroll_ready || !rs) return;
    for (uint32_t i = 0; i < at<uint32_t>(rs, 0x68); ++i)
        if (sprMap* s = sprite(unit, i)) s->isDisp = false;
    if (n <= visible) return;
    sprMap* up = sprite(unit, kScrollUp);
    sprMap* down = sprite(unit, kScrollDown);
    sprMap* bar = sprite(unit, kScrollBar);
    if (!up || !down || !bar) return;
    const int track_top = window_top + 24, track = window_h - 48;   // the arrows above and below the track
    const int bar_len = track * visible / n, travel = track - bar_len;
    const int bar_y = top >= n - visible ? travel : travel * top / (n - visible);
    up->posHDTV = MtPoint{kScrollX, track_top - 24};
    down->posHDTV = MtPoint{kScrollX, track_top + track};
    bar->posHDTV = MtPoint{kScrollX, track_top + bar_y};
    bar->scaleHDTV.h = static_cast<uint32_t>(static_cast<uint64_t>(bar->scaleHDTVOrg.h) * bar_len / 232);   // 232 px at its own scale
    up->isDisp = down->isDisp = bar->isDisp = true;
}

struct RowView {
    std::string label, value;
    int label_image = -1, value_image = -1;
};

RowView row_view(const ListMenu& m, int i) {
    RowView v;
    if (m.text) m.text(i, v.label, v.value);
    if (m.images) m.images(i, v.label_image, v.value_image);
    return v;
}

bool image_page(const ListMenu& m, int n) {
    for (int i = 0; i < n; ++i) {
        const RowView v = row_view(m, i);
        if (v.label_image >= 0 || v.value_image >= 0) return true;
    }
    return false;
}

// the row's label font, loaded (its text rows then take the image rows' spacing)
bool label_font(const Open& o) { return o.row && !o.row->font.empty() && menus::load_label_font(o.row->font.c_str()); }
bool spaced(const Open& o, const ListMenu& m, int n) { return label_font(o) || image_page(m, n); }

int row_centre(bool images, int i) { return images ? kImageRowY + i * kImageRowStep : kRowY + i * kRowStep + 13; }

// A frame with its left edge at left and its middle at cy. Frames anchor two ways (pause sheet, 2026-11-19): m_CBA 6
// (the row labels) from their left edge + ofsX, m_CBA 5 (titles, arrows) from their middle - w / 2 + ofsX; both put the
// top at the sprite's y + ofsY.
// False when the sheet has no such frame (a plugin's own frame on the game's sheet): the row's text is drawn instead.
bool show_image(int index, int frame, int left, int cy, bool on) {
    sprMap* s = g_pause ? sprite(g_pause, index) : nullptr;
    const ANM_SPRITE* f = g_pause ? menus::anm_sprite(reinterpret_cast<uSprLayout*>(g_pause), frame) : nullptr;
    if (!s || !f) return false;
    const int ox = static_cast<int8_t>(f->m_ofsX), oy = static_cast<int8_t>(f->m_ofsY);
    s->sprId = static_cast<uint16_t>(frame);
    s->posHDTV = MtPoint{f->m_CBA == 5 ? left + f->m_sizeW / 2 - ox : left - ox, cy - f->m_sizeH / 2 - oy};
    const uint8_t v = on ? 0xFF : 0x80;
    s->color = MtColor{v, v, v, 0xFF};
    s->isDisp = true;
    return true;
}

void draw_menu(const Open& o) {
    const ListMenu& m = o.row->menu;
    const bool menu_font = menus::load_menu_font();
    const int slot = menu_font ? menus::kMenuFontSlot : menus::kDebugFontSlot;
    int title = m.title_image ? m.title_image() : -1;
    if (title < 0) title = o.row->title_image;
    if (title >= 0) {
        if (sprMap* t = g_pause ? sprite(g_pause, kTitleSprite) : nullptr) t->sprId = static_cast<uint16_t>(title);
    } else {
        menus::print(slot, kTitleX, kTitleY, 36, 36, white(0xFF), kPrio, m.title.c_str());
    }
    const int n = m.rows ? m.rows() : 0;
    const bool font = label_font(o);   // the text rows in the label font, as the image rows
    const bool images = font || image_page(m, n);
    size_t labels = 0, values = 0;
    const int visible = images ? kImageVisible : kTextVisible;
    for (int i = o.top; i < n && i < o.top + visible; ++i) {
        const RowView v = row_view(m, i);
        const bool on = i == o.cursor;
        const int cy = row_centre(images, i - o.top);
        bool label_shown = false;
        if (v.label_image >= 0 && labels < std::size(kLabelPool))
            label_shown = show_image(kLabelPool[labels++], v.label_image, on ? kImageRowSelX : kImageRowX, cy, on);
        const MtColor lit = on ? white(0xFF) : MtColor{0x80, 0x80, 0x80, 0xFF};   // as show_image dims the frames
        if (!label_shown && !v.label.empty() && font)
            menus::print_label(on ? kImageRowSelX : kImageRowX, cy - 20, lit, kPrio, v.label.c_str());
        else if (!label_shown && !v.label.empty())
            menus::print(slot, images ? kImageRowX + 8 : kRowX, cy - (images ? 16 : 13), images ? 32 : kTextSize,
                         images ? 32 : kTextSize, on ? gold(0xFF) : grey(0xFF), kPrio, v.label.c_str());
        const int vx = images ? kImageValueX : kValueX;
        if (v.value_image >= 0 && values < std::size(kValuePool)) {
            show_image(kValuePool[values++], v.value_image, vx, cy, on);
            if (on && values + 2 <= std::size(kValuePool)) {   // the cut menu's value arrows: 44 (mirrored) and 43, 32 x 32
                const ANM_SPRITE* f = menus::anm_sprite(reinterpret_cast<uSprLayout*>(g_pause), v.value_image);
                const int w = f ? f->m_sizeW : 256;   // the value frames are 256 wide, their words centred: arrows just inside
                show_image(kValuePool[values++], kArrowLeft, vx + w * 6 / 100 - 16, cy, true);
                show_image(kValuePool[values++], kArrowRight, vx + w * 96 / 100 - 16, cy, true);
            }
        } else if (!v.value.empty() && font) {   // centred in a value frame's place, the arrows where the frames' are
            const int tw = menus::label_width(v.value.c_str()), start = vx + 128 - tw / 2 - 2;
            // the sheet's value words (NORMAL, ON) are gold metal: the silver letters tinted (B, G, R, A), dimmed off the cursor
            const MtColor brass = on ? MtColor{0x90, 0xD8, 0xFF, 0xFF} : MtColor{0x48, 0x6C, 0x80, 0xFF};
            menus::print_label(start, cy - 20, brass, kPrio, v.value.c_str());
            if (on && values + 2 <= std::size(kValuePool)) {   // or further out, clear of a long value
                show_image(kValuePool[values++], kArrowLeft, (std::min)(vx - 1, start - 48), cy, true);
                show_image(kValuePool[values++], kArrowRight, (std::max)(vx + 229, start + tw - 4), cy, true);
            }
        } else if (!v.value.empty()) {
            const std::string shown = on ? "[  " + v.value + "  ]" : v.value;
            menus::print(slot, on ? vx - 22 : vx, cy - 13, kTextSize, kTextSize, on ? white(0xFF) : grey(0xFF), kPrio,
                         shown.c_str());
        }
    }
    if (sprMap* arrow = g_pause ? sprite(g_pause, kArrowSprite) : nullptr) {   // the game's arrow on our row
        arrow->posHDTV = images ? MtPoint{kImageRowSelX + 7, row_centre(true, o.cursor - o.top) - 4}
                                : MtPoint{kRowX + 12, kRowY + (o.cursor - o.top) * kRowStep + 9};
        arrow->isDisp = true;
    }
    menus::print(slot, kRowX, 600, 22, 22, grey(0xFF), kPrio, "Up / Down: choose      Left / Right: change");
}

void __stdcall list_move(uintptr_t unit) {
    Open& o = g_open;
    if (o.unit != unit || !o.row) return;
    ++o.frames;
    const ListMenu& m = o.row->menu;
    switch (o.phase) {
    case Phase::Opening:
        fade(true);   // the pause menu faded to black before making us
        if (!g_scroll_arc) g_scroll_arc = resource_create(kArchiveDti, kScrollArc, 2);   // async; the layout once it's in
        o.phase = Phase::Running;
        break;
    case Phase::Running: {
        const int page = m.page ? m.page() : 0;
        if (page != o.page) {   // the cursor is kept per page (back to a page: where it was)
            o.cursors[o.page] = o.cursor;
            o.page = page;
            const auto it = o.cursors.find(page);
            o.cursor = it != o.cursors.end() ? it->second : 0;
            o.top = 0;
        }
        const int n = m.rows ? m.rows() : 0;
        if (n > 0) {
            if (o.cursor >= n) o.cursor = n - 1;
            o.cursor = get_cursor(unit, 0, n, o.cursor, true);
            const int visible = spaced(o, m, n) ? kImageVisible : kTextVisible;   // keep the cursor in the window
            if (o.cursor < o.top) o.top = o.cursor;
            if (o.cursor >= o.top + visible) o.top = o.cursor - visible + 1;
            if (o.top > (std::max)(0, n - visible)) o.top = (std::max)(0, n - visible);
            const RowView v = row_view(m, o.cursor);
            if ((!v.value.empty() || v.value_image >= 0) && m.change) {
                const int step = get_cursor(unit, 1, 3, 1, false) - 1;   // left / right as -1 / +1
                if (step) m.change(o.cursor, step);
            }
            if (o.frames > 2 && pad_decide() && m.decide) m.decide(o.cursor);
        }
        if (o.frames > 2 && pad_cancel() && !(m.back && m.back())) {   // B: the page's back first, else close
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
    g_list_shown = o.phase == Phase::Running || o.phase == Phase::Closing;
    if (g_list_shown) {
        if (g_pause) hide_pause_rows(g_pause);
        draw_menu(o);
        load_scroll_layout(unit);
        const int n = m.rows ? m.rows() : 0;
        const bool images = spaced(o, m, n);
        const int visible = images ? kImageVisible : kTextVisible;
        update_scroll(unit, n, visible, o.top, images ? kImageRowY - 28 : kRowY,
                      visible * (images ? kImageRowStep : kRowStep));
    }
}
bool __stdcall list_is_end(uintptr_t unit) {
    const bool end = g_open.unit != unit || g_open.phase == Phase::Done;
    if (end && g_open.unit == unit) {
        g_list_shown = false;
        if (g_pause) restore_pause_rows(g_pause);
        resource_release(g_scroll_arc);   // the unit's layout goes with the unit
        g_scroll_arc = 0;
        g_scroll_ready = false;
    }
    return end;
}

// Draws nothing: the pause menu's own window is the backdrop (g_list_shown keeps it drawn). Not uMapMgr's draw (a
// rectangle) nor uDevilCock::transMain (kDevilCockTransMain: the layout uMapMgr loads would show under our rows). A dim
// full-screen rectangle, if one is ever wanted: kDrawRect (Vibed\RE\pause_menu.md "Calls").
// Draws the scroll element only (the unit's layout = the key config's, every other sprite hidden by update_scroll).
void __stdcall list_trans(uintptr_t unit, uintptr_t draw) {
    if (!g_scroll_ready || unit != g_open.unit || !g_list_shown || !at<uintptr_t>(unit, 0x18)) return;
    __asm {
        push draw
        mov ecx, unit
        mov eax, kDevilCockTransMain
        call eax
    }
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
    if (g_pause) {
        save_pause_rows(g_pause);
        hide_pause_rows(g_pause);
    }
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
    auto* tb = reinterpret_cast<uintptr_t*>(kPauseTransBeforeSlot);
    DWORD old_tb = 0;
    if (*tb == kPauseTransBefore && VirtualProtect(tb, sizeof *tb, PAGE_READWRITE, &old_tb)) {
        *tb = reinterpret_cast<uintptr_t>(&pause_trans_before_thunk);
        VirtualProtect(tb, sizeof *tb, old_tb, &old_tb);
    }
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
