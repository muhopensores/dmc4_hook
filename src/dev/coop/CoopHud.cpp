#include "CoopHud.hpp"
#include "Coop.hpp"
#include "CoopContext.hpp"
#include "../../sdk/Cam.hpp"
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

std::unique_ptr<FunctionHook> hook_cock_draw, hook_draw_sprite, hook_reticle_check, hook_project;
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
thread_local uint8_t scaling = 0;   // active_scale != 1

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
    if (split && !reticle && coop_hud::hud_scale != 1.0f && coop_hud::view_scaled[piece_slot(cock)]) {
        active_scale = coop_hud::hud_scale;
        scaling = 1;
    }
    if (Coop::mod_enabled && reticle) {   // through its own view's matrices, split, full screen or in a window alike
        const int slot = reticle_slot(cock);
        reticle_view = slot >= 0 ? slot : 0;
    }
    const int result = cock_draw_original(cock, edx, view);
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

void __stdcall scale_corners(float* corners) {
    if (!scaling) {
        return;
    }
    for (int v = 0; v < 4; v++) {
        corners[v * 4 + 0] *= active_scale;
        corners[v * 4 + 1] *= active_scale;
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
