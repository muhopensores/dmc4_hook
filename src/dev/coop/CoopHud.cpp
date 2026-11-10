#include "CoopHud.hpp"
#include "Coop.hpp"
#include "CoopContext.hpp"
#include "../../sdk/Cam.hpp"
#include "../../mod.hpp"
#include <algorithm>
#include <memory>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

namespace coop_hud {

float hud_scale = 0.5f;
bool split_shown = false;
bool view_scaled[4] = {};

} // namespace coop_hud

namespace {

constexpr uintptr_t kCockDraw       = 0x532F50; // uDevilCock::draw (thiscall, the view in one stack argument)
constexpr uintptr_t kDrawSprite     = 0x45B410; // sPrim::drawSprite (corners in eax)
constexpr uintptr_t kLockOnCockVtbl  = 0xBDE4D0; // uLockOnCock::vftable (Nero)
constexpr uintptr_t kLockOnDanteVtbl = 0xBDE470; // uLockOnDante::vftable
constexpr uintptr_t kReticleCheck    = 0x76FAA0; // both reticles' draw check (vtbl +0x38 -> 0x76EFE0), stdcall(this)
constexpr uintptr_t kProjectToView   = 0xA592F0; // world -> pixels of the draw context (eax = out, edx = pos, esi = cTrans)
constexpr uintptr_t kCameraSystem    = 0xE552D4; // sCamera::mpInstance
constexpr uintptr_t kPrim            = 0xE559D4; // sPrim::mpInstance
constexpr uintptr_t kElementDraw     = 0x4B1990; // one HUD element (sprite parts through sub_45A420)
constexpr uintptr_t kTextDraw        = 0x497950; // one text (its glyphs through sub_497E60 -> sub_45ABA0)

std::unique_ptr<FunctionHook> hook_cock_draw, hook_draw_sprite, hook_reticle_check, hook_project, hook_element, hook_text;
uintptr_t element_original = 0, text_original = 0;
uintptr_t project_original = 0;
// the viewport of the reticle drawing on this thread, -1 for none
thread_local int reticle_view = -1;

using CockDrawFn = int(__fastcall*)(uintptr_t cock, uintptr_t edx, int view);
CockDrawFn cock_draw_original = nullptr;
uintptr_t draw_sprite_original = 0;
using ReticleCheckFn = bool(__stdcall*)(uintptr_t reticle);
ReticleCheckFn reticle_check_original = nullptr;

// players 2-4's reticle check, taken at the end of the reticle's update (his context)
bool reticle_shown[coop::kMaxPlayers] = {};

// The scale for the sprites this thread draws now (the HUD pieces draw on several threads at once: one global flag
// scaled a reticle while a cockpit drew beside it).
thread_local float active_scale = 1.0f;
thread_local uint8_t scaling = 0;   // the piece drawing now is fitted to its view

// Where the HUD's coordinates go. They are 1/16 pixels of the whole screen (sPrim mode +0x4EC0 = 0; else of sPrim's
// virtual size +0x4EB8 / +0x4EBC, 1280 x 720 in mode 2) counted from the view's top-left (sub_45A420 / sub_45ABA0 build
// the corners x16; measured: the cockpit's sprites spanned x 3712..18944 in 1280-wide units). A piece in a shared view is fitted by element: each element
// or text keeps to the edge it is drawn by (left / middle / right third of the screen, top / middle / bottom), at
// hud_scale, in the view's own size; the whole element or text follows its first sprite, so text doesn't tear.
// Scaling everything toward the top-left put the right-hand style meter in the middle of a full-width view.
constexpr float kSubpixel = 16.0f;                 // HUD units per pixel
thread_local float full_w = 0.0f, full_h = 0.0f;   // the screen in HUD units
thread_local float view_w = 0.0f, view_h = 0.0f;   // the view in HUD units
thread_local uint8_t group_new = 1;                // the next sprite starts an element / text
thread_local uint8_t anchor_x = 0, anchor_y = 0;   // 0 left / top, 1 middle, 2 right / bottom
// The boxes of the groups drawn so far in this piece (HUD units, before fitting). An element touching one of them
// (within kTouch of the screen) joins it and takes its anchor: a gauge is drawn as side-by-side elements (frame, fill,
// the end caps a long HP bar puts past the left third), not in one run (the top bar's tip comes after the orb counter),
// and its parts picked anchors of their own and tore apart.
constexpr float kTouch = 0.01f;
constexpr int kMaxBoxes = 64;
struct Box { float x0, y0, x1, y1; uint8_t ax, ay; };
thread_local Box boxes[kMaxBoxes];
thread_local int box_count = 0;
thread_local int box_cur = -1;   // the group the sprites go to now

void __stdcall mark_group() {
    group_new = 1;
}

// Readout of the last fitted draw of each player's HUD pieces (Coop > Diagnostics).
struct HudDiag {
    float full_w = 0, full_h = 0, view_w = 0, view_h = 0, screen_w = 0, screen_h = 0;
    int   mode = 0, prim_w = 0, prim_h = 0, sprites = 0, groups = 0;
    float min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    int   anchors_x[3] = {}, anchors_y[3] = {};
    uint32_t draws = 0;
};
HudDiag hud_diag[coop::kMaxPlayers];
struct DrawDiag { uint32_t calls = 0, split = 0, shared = 0, extent = 0; };
DrawDiag draw_diag_of[coop::kMaxPlayers];   // every non-reticle piece draw per player, and how many passed each gate
thread_local HudDiag hud_cur;

// The screen and the piece's view (slot) in HUD units; false when unknown.
bool view_extent(int slot) {
    const sCamera* cams = *reinterpret_cast<sCamera* const*>(kCameraSystem);
    if (!cams || slot < 0 || slot >= 8) return false;
    const float screen_w = (float)(int32_t)(cams->ScreenRect.r - cams->ScreenRect.l);
    const float screen_h = (float)(int32_t)(cams->ScreenRect.b - cams->ScreenRect.t);
    const MtRect& r = cams->viewports[slot].mRegion;
    const float vp_w = (float)(int32_t)(r.r - r.l);
    const float vp_h = (float)(int32_t)(r.b - r.t);
    if (screen_w <= 0.0f || screen_h <= 0.0f || vp_w <= 0.0f || vp_h <= 0.0f) return false;
    full_w = screen_w;
    full_h = screen_h;
    const uintptr_t prim = *reinterpret_cast<uintptr_t*>(kPrim);
    if (prim && *reinterpret_cast<int32_t*>(prim + 0x4EC0) != 0) {
        full_w = (float)*reinterpret_cast<int32_t*>(prim + 0x4EB8);
        full_h = (float)*reinterpret_cast<int32_t*>(prim + 0x4EBC);
    }
    full_w *= kSubpixel;
    full_h *= kSubpixel;
    view_w = full_w * vp_w / screen_w;
    view_h = full_h * vp_h / screen_h;
    hud_cur = HudDiag{};
    hud_cur.full_w = full_w;
    hud_cur.full_h = full_h;
    hud_cur.view_w = view_w;
    hud_cur.view_h = view_h;
    hud_cur.screen_w = screen_w;
    hud_cur.screen_h = screen_h;
    if (prim) {
        hud_cur.mode = *reinterpret_cast<int32_t*>(prim + 0x4EC0);
        hud_cur.prim_w = *reinterpret_cast<int32_t*>(prim + 0x4EB8);
        hud_cur.prim_h = *reinterpret_cast<int32_t*>(prim + 0x4EBC);
    }
    hud_cur.min_x = hud_cur.min_y = 1e9f;
    hud_cur.max_x = hud_cur.max_y = -1e9f;
    return full_w > 0.0f && full_h > 0.0f;
}

// The views are split now (paused or no extra players: player 1's view on the whole screen, the HUD at full size).
bool split_screen() {
    return Coop::mod_enabled && coop_hud::split_shown;
}

bool is_reticle(uintptr_t cock) {
    const uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(cock);
    return vtbl == kLockOnCockVtbl || vtbl == kLockOnDanteVtbl;
}

// The player whose view a HUD piece draws in: players 2-4's cockpit and reticle, else player 1's (the game's own).
int piece_slot(uintptr_t cock) {
    for (int i = 1; i < coop::kMaxPlayers; i++) {
        if ((uintptr_t)coop::players[i].hud == cock || (uintptr_t)coop::players[i].reticle == cock) return i;
    }
    return 0;
}

// The slot (1-3) whose reticle this is, -1 for none.
int reticle_slot(uintptr_t unit) {
    for (int i = 1; i < coop::kMaxPlayers; i++) {
        if (unit && (uintptr_t)coop::players[i].reticle == unit) return i;
    }
    return -1;
}

bool __stdcall reticle_check(uintptr_t reticle) {
    const int slot = reticle_slot(reticle);
    if (slot < 0) {
        return reticle_check_original(reticle);
    }
    return reticle_shown[slot];
}

int __fastcall cock_draw(uintptr_t cock, uintptr_t edx, int view) {
    const float prev_scale = active_scale;
    const uint8_t prev_scaling = scaling;
    const int prev_view = reticle_view;
    const bool split = split_screen();
    const bool reticle = is_reticle(cock);
    const int slot = piece_slot(cock);
    if (!reticle && slot >= 0 && slot < coop::kMaxPlayers) {
        DrawDiag& g = draw_diag_of[slot];
        ++g.calls;
        g.split += split ? 1 : 0;
        g.shared += coop_hud::view_scaled[slot] ? 1 : 0;
    }
    if (split && !reticle && coop_hud::view_scaled[slot] && view_extent(slot)) {
        ++draw_diag_of[slot].extent;
        active_scale = coop_hud::hud_scale;
        scaling = 1;
        group_new = 1;
        box_count = 0;
        box_cur = -1;
    }
    if (Coop::mod_enabled && reticle) {   // through its own view's matrices, split, full screen or in a window alike
        const int own = reticle_slot(cock);
        reticle_view = own >= 0 ? own : 0;
    }
    const int result = cock_draw_original(cock, edx, view);
    const uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(cock);
    const bool cockpit = vtbl == 0xBC7658 || vtbl == 0xBC7448;   // uCockpitNero / uCockpitDante: the style meter's piece
    if (scaling && !prev_scaling && cockpit && slot >= 0 && slot < coop::kMaxPlayers && hud_cur.sprites > 0) {
        const uint32_t draws = hud_diag[slot].draws + 1;
        hud_diag[slot] = hud_cur;
        hud_diag[slot].draws = draws;
    }
    active_scale = prev_scale;
    scaling = prev_scaling;
    reticle_view = prev_view;
    return result;
}

// The reticle's place in its view: sub_A592F0 projects through the draw context (cTrans::mpContext +0x2A90:
// mViewProjMat +0xC0, mViewport +0x200), which in the HUD pass of a split screen isn't the reticle's view. Projected
// here through the view's own matrices instead, as sCamera::move stores them per viewport (mViewMat = the
// camera's vtbl +0x34, mProjMat = vtbl +0x38 with the camera's aspect from mRegion), in pixels from the view's
// top-left like the original (the HUD draws from the view's top-left).
bool __stdcall project_reticle(int32_t* out, const float* pos) {
    const int slot = reticle_view;
    if (slot < 0) {
        return false;
    }
    const sCamera* cams = *reinterpret_cast<sCamera* const*>(kCameraSystem);
    if (!cams) {
        return false;
    }
    const sCamera_ViewPort& vp = cams->viewports[slot];
    if (!vp.mpCamera) {
        return false;
    }
    const float* v = &vp.mViewMat.m1.x;
    const float* p = &vp.mProjMat.m1.x;
    float view[4];
    for (int c = 0; c < 4; c++) {
        view[c] = pos[0] * v[c] + pos[1] * v[4 + c] + pos[2] * v[8 + c] + v[12 + c];
    }
    float clip[4];
    for (int c = 0; c < 4; c++) {
        clip[c] = view[0] * p[c] + view[1] * p[4 + c] + view[2] * p[8 + c] + view[3] * p[12 + c];
    }
    if (clip[3] == 0.0f) {
        return false;
    }
    const float nx = clip[0] / clip[3];
    const float ny = clip[1] / clip[3];
    out[0] = (int32_t)((float)(int32_t)(vp.mRegion.r - vp.mRegion.l) * (nx + 1.0f) * 0.5f);
    out[1] = (int32_t)((float)(int32_t)(vp.mRegion.b - vp.mRegion.t) * (1.0f - ny) * 0.5f);
    return true;
}

naked_extern void detour_project() {
    __asm {
        push eax
        push edx
        push edx
        push eax
        call project_reticle
        mov cl, al
        pop edx
        pop eax
        test cl, cl
        jz originalcode
        ret
    originalcode:
        jmp dword ptr [project_original]
    }
}

uint8_t band(float c, float full) {
    return c < full * 0.33f ? 0 : (c > full * 0.67f ? 2 : 1);
}

float fit(float v, uint8_t anchor, float full, float view, float s) {
    switch (anchor) {
    case 0:  return v * s;
    case 2:  return view - (full - v) * s;
    default: return view * 0.5f + (v - full * 0.5f) * s;
    }
}

void __stdcall scale_corners(float* corners) {
    if (!scaling) {
        return;
    }
    float x0 = corners[0], x1 = corners[0], y0 = corners[1], y1 = corners[1];
    for (int v = 1; v < 4; v++) {
        x0 = std::min(x0, corners[v * 4 + 0]);
        x1 = std::max(x1, corners[v * 4 + 0]);
        y0 = std::min(y0, corners[v * 4 + 1]);
        y1 = std::max(y1, corners[v * 4 + 1]);
    }
    if (group_new) {
        const float mx = full_w * kTouch, my = full_h * kTouch;
        box_cur = -1;
        for (int k = 0; k < box_count; k++) {
            const Box& o = boxes[k];
            if (x0 <= o.x1 + mx && x1 >= o.x0 - mx && y0 <= o.y1 + my && y1 >= o.y0 - my) {
                box_cur = k;
                break;
            }
        }
        if (box_cur < 0) {
            const uint8_t ax = band((x0 + x1) * 0.5f, full_w);
            const uint8_t ay = band((y0 + y1) * 0.5f, full_h);
            if (box_count < kMaxBoxes) {
                box_cur = box_count++;
                boxes[box_cur] = Box{x0, y0, x1, y1, ax, ay};
            }
            anchor_x = ax;
            anchor_y = ay;
            ++hud_cur.groups;
            ++hud_cur.anchors_x[ax];
            ++hud_cur.anchors_y[ay];
        } else {
            anchor_x = boxes[box_cur].ax;
            anchor_y = boxes[box_cur].ay;
        }
        group_new = 0;
    }
    if (box_cur >= 0) {
        Box& o = boxes[box_cur];
        o.x0 = std::min(o.x0, x0);
        o.x1 = std::max(o.x1, x1);
        o.y0 = std::min(o.y0, y0);
        o.y1 = std::max(o.y1, y1);
    }
    ++hud_cur.sprites;
    for (int v = 0; v < 4; v++) {
        const float x = corners[v * 4 + 0], y = corners[v * 4 + 1];
        if (x < hud_cur.min_x) hud_cur.min_x = x;
        if (x > hud_cur.max_x) hud_cur.max_x = x;
        if (y < hud_cur.min_y) hud_cur.min_y = y;
        if (y > hud_cur.max_y) hud_cur.max_y = y;
    }
    for (int v = 0; v < 4; v++) {
        corners[v * 4 + 0] = fit(corners[v * 4 + 0], anchor_x, full_w, view_w, active_scale);
        corners[v * 4 + 1] = fit(corners[v * 4 + 1], anchor_y, full_h, view_h, active_scale);
    }
}

// Entries of an element and a text: the next sprite starts a group.
naked_extern void detour_element() {
    __asm {
        pushfd
        pushad
        call mark_group
        popad
        popfd
        jmp dword ptr [element_original]
    }
}

naked_extern void detour_text() {
    __asm {
        pushfd
        pushad
        call mark_group
        popad
        popfd
        jmp dword ptr [text_original]
    }
}

naked_extern void detour_draw_sprite() {
    __asm {
        pushad
        push eax
        call scale_corners
        popad
        jmp dword ptr [draw_sprite_original]
    }
}

} // namespace

namespace coop_hud {

void draw_diag() {
    for (int i = 0; i < (int)Coop::player_num && i < coop::kMaxPlayers; i++) {
        const HudDiag& d = hud_diag[i];
        const DrawDiag& g = draw_diag_of[i];
        ImGui::Text(_("P%d HUD: piece draws %u, split %u, view shared %u, fitted %u"), i + 1, g.calls, g.split, g.shared, g.extent);
        ImGui::Text(_("   cockpit: %u fitted draws, %d sprites in %d groups"), d.draws, d.sprites, d.groups);
        ImGui::Text(_("   units %.0f x %.0f (screen %.0f x %.0f, sPrim mode %d %d x %d), view %.0f x %.0f"), d.full_w,
                    d.full_h, d.screen_w, d.screen_h, d.mode, d.prim_w, d.prim_h, d.view_w, d.view_h);
        ImGui::Text(_("   sprites x %.0f..%.0f y %.0f..%.0f, anchors x L%d M%d R%d y T%d M%d B%d"), d.min_x, d.max_x, d.min_y,
                    d.max_y, d.anchors_x[0], d.anchors_x[1], d.anchors_x[2], d.anchors_y[0], d.anchors_y[1], d.anchors_y[2]);
    }
}

void after_update(uintptr_t unit) {
    const int slot = reticle_slot(unit);
    if (slot >= 0) {
        reticle_shown[slot] = reticle_check_original(unit);   // also stores his target at +0x48 for the draw
    }
}

std::optional<std::string> install() {
    hook_draw_sprite = std::make_unique<FunctionHook>(kDrawSprite, (uintptr_t)&detour_draw_sprite);
    if (!hook_draw_sprite->create()) {
        return "Failed to init Coop: drawSprite";
    }
    draw_sprite_original = hook_draw_sprite->get_original();
    hook_element = std::make_unique<FunctionHook>(kElementDraw, (uintptr_t)&detour_element);
    if (!hook_element->create()) {
        return "Failed to init Coop: HUD element";
    }
    element_original = hook_element->get_original();
    hook_text = std::make_unique<FunctionHook>(kTextDraw, (uintptr_t)&detour_text);
    if (!hook_text->create()) {
        return "Failed to init Coop: HUD text";
    }
    text_original = hook_text->get_original();
    hook_cock_draw = std::make_unique<FunctionHook>(kCockDraw, (uintptr_t)&cock_draw);
    if (!hook_cock_draw->create()) {
        return "Failed to init Coop: HUD draw";
    }
    cock_draw_original = hook_cock_draw->get_original<std::remove_pointer_t<CockDrawFn>>();
    hook_reticle_check = std::make_unique<FunctionHook>(kReticleCheck, (uintptr_t)&reticle_check);
    if (!hook_reticle_check->create()) {
        return "Failed to init Coop: reticle check";
    }
    reticle_check_original = hook_reticle_check->get_original<std::remove_pointer_t<ReticleCheckFn>>();
    hook_project = std::make_unique<FunctionHook>(kProjectToView, (uintptr_t)&detour_project);
    if (!hook_project->create()) {
        return "Failed to init Coop: reticle projection";
    }
    project_original = hook_project->get_original();
    return std::nullopt;
}

} // namespace coop_hud

#pragma pop_macro("naked")
