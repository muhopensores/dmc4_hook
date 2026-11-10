#include "Coop.hpp"
#include "CoopPads.hpp"
#include "CoopDiag.hpp"
#include "CoopContext.hpp"
#include "CoopHud.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/Players.hpp"
#include "../../mods/CharSwitcher.hpp"
#include "../../mods/DebugCam.hpp" // REGION_* viewport modes
#include "../../mods/Macro.hpp"
#include <cstring>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

// The camera has to come back to its player after a camera event. These read sCamera when the BP portal transition
// finishes (in order): 0x40C89E mov edx,[0xE552D4]; 0x4F7B50 mov ecx,[0xE552D4]; 0x4F7BB6 mov eax,[0xE552D4].

bool Coop::mod_enabled = false;
unsigned int Coop::player_num = 2;

uintptr_t Coop::jmp_ret2 = NULL;
uintptr_t Coop::jmp_ret5 = NULL;
uintptr_t Coop::jmp_ret_water = NULL;
uintptr_t Coop::jmp_ret_unit_tick = NULL;
uintptr_t Coop::jmp_ret_unit_pass2 = NULL;
uintptr_t Coop::jmp_ret_unit_pass3 = NULL;
uintptr_t Coop::jmp_ret_unit_init = NULL;

namespace coop {

Player players[kMaxPlayers] = {{0, 0}, {0, 1}, {0, 2}, {0, 3}};

} // namespace coop

namespace {

using coop::players;
using coop::kMaxPlayers;

uintptr_t sCamera_ptr = 0x00E552D4;           // sCamera::mpInstance
uintptr_t sMediator_ptr = 0x00E558B8;         // sMediator::mpInstance
uintptr_t uCameraCtrl_setup_end = 0x004F76DA; // uCameraCtrl::setup, after the viewport assignment
uintptr_t setup_develop_fn = 0x004F76F0;      // uCameraCtrl::setupDevelop (eax = camera)

constexpr uintptr_t kNeroCtor     = 0x7E1A70; // uPlayerNero::uPlayerNero
constexpr uintptr_t kDanteCtor    = 0x7B2130; // uPlayerDante::uPlayerDante
constexpr uintptr_t kNeroHudCtor  = 0x507A60;
constexpr uintptr_t kDanteHudCtor = 0x503210;
constexpr uintptr_t kCameraCtor   = 0x4F71B0; // uCameraCtrl::uCameraCtrl
// The lock-on reticle: uCockpitMgr::uCockpitMgr 0x507370 makes the game's one, uLockOnCock for Nero (0x5074CF) or
// uLockOnDante for Dante (0x507473), and runs it from its own list (move / draw by the cock's status bits +0x2C). Its
// draw check (sub_76FAA0 for Nero) reads mpPlayer's lock-on target and the target's mIsCameraIn. Players 2-4 get their
// own as a unit of its own on the HUD line, like their cockpit, drawn in their view, updated and drawn in their context
// (CoopContext, CoopHud). Each constructor builds one in memory passed in eax, from UnitAllocator 0xE1434C (vtbl +0x14).
constexpr uintptr_t kUnitAllocator   = 0xE1434C;
constexpr uintptr_t kLockOnNeroCtor  = 0x76E790; // uLockOnCock
constexpr uint32_t  kLockOnNeroSize  = 0x70;
constexpr uintptr_t kLockOnDanteCtor = 0x76F200; // uLockOnDante
constexpr uint32_t  kLockOnDanteSize = 0x58;

constexpr int kLinePlayer = 13;
constexpr int kLineCamera = 0x17;
// Players 2-4's cameras run on the game camera's line, after it (sUnit::addBottom 0x8DC540): after every player, enemy and
// shell has moved, with the camera line's time step. On line 18 they ran among some enemies (half of them moved, half
// not) and on the shells' time step, and the follow camera's auto-adjust (cCameraPlayer::cameraAngleYHosei 0x4255B0)
// snapped.
constexpr int kLineExtraCamera = kLineCamera;
constexpr int kLineHud = 25;

void* construct(uintptr_t ctor) { return ((void* (*)())ctor)(); }

void* make_reticle(int character) {
    void* allocator = *reinterpret_cast<void**>(kUnitAllocator);
    if (!allocator) {
        return nullptr;
    }
    const uint32_t size = character == 0 ? kLockOnNeroSize : kLockOnDanteSize;
    using AllocFn = void*(__fastcall*)(void* self, void* edx, uint32_t size, uint32_t align);
    void* mem = reinterpret_cast<AllocFn>((*reinterpret_cast<uintptr_t**>(allocator))[5])(allocator, nullptr, size, 16);
    if (!mem) {
        return nullptr;
    }
    const uintptr_t ctor = character == 0 ? kLockOnNeroCtor : kLockOnDanteCtor;
    void* reticle = nullptr;
    __asm {
        mov eax, mem
        call ctor
        mov reticle, eax
    }
    return reticle;
}

void spawn_player(int character, int slot) {
    void* player = construct(character == 0 ? kNeroCtor : kDanteCtor);
    *reinterpret_cast<uint8_t*>((uintptr_t)player + coop::kControlNumber) = (uint8_t)slot;
    players[slot].unit = (uintptr_t)player;
    devil4_sdk::sUnit_spawn(player, kLinePlayer);

    void* hud = construct(character == 0 ? kNeroHudCtor : kDanteHudCtor);
    players[slot].hud = hud;
    ((cUnit*)hud)->flags.bits.mTransView = (1 << slot);
    devil4_sdk::sUnit_spawn(hud, kLineHud);

    if (void* reticle = make_reticle(character)) {
        players[slot].reticle = reticle;
        ((cUnit*)reticle)->flags.bits.mTransView = (1 << slot);
        devil4_sdk::sUnit_spawn(reticle, kLineHud);
    }
}

void __stdcall setup_develop(uCameraCtrl* cam) {
    __asm {
        mov eax, cam
        call dword ptr [setup_develop_fn]
    }
}

uCameraCtrl* make_cam() {
    uCameraCtrl* cam = (uCameraCtrl*)construct(kCameraCtor);
    devil4_sdk::sUnit_spawn(cam, kLineExtraCamera);
    uCameraCtrl* main_cam = players[0].cam;
    if (!main_cam) {
        main_cam = (uCameraCtrl*)devil4_sdk::get_sUnit()->mMoveLine[kLineCamera].mTop;
        players[0].cam = main_cam;
    }
    cam->mpResource = main_cam->mpResource;
    setup_develop(cam);
    return cam;
}

void apply_viewport_layout() {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam) {
        return;
    }
    static const int kModes[3][4] = {
        {REGION_TOP, REGION_BOTTOM, 0, 0},
        {REGION_TOPLEFT, REGION_TOPRIGHT, REGION_BOTTOMLEFT, 0},
        {REGION_TOPLEFT, REGION_TOPRIGHT, REGION_BOTTOMLEFT, REGION_BOTTOMRIGHT},
    };
    const int n = (int)Coop::player_num;
    for (int i = 0; i < n; i++) {
        if (i > 0 || players[0].cam) {   // viewport 0 keeps the game's camera until Coop knows player 1's
            s_cam->viewports[i].mpCamera = players[i].cam;
        }
        s_cam->viewports[i].mAttr = 0x17;
        s_cam->viewports[i].mMode = (uint8_t)kModes[n - 2][i];
    }
}

// Player 1's view on the whole screen, the extra views off (a viewport without a camera isn't drawn).
void apply_single_view() {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam) {
        return;
    }
    s_cam->viewports[0].mMode = REGION_FULLSCREEN;
    for (int v = 1; v < kMaxPlayers; v++) {
        for (int i = 1; i < kMaxPlayers; i++) {
            if (players[i].cam && s_cam->viewports[v].mpCamera == players[i].cam) {
                s_cam->viewports[v].mpCamera = nullptr;
            }
        }
    }
}

// The split screen follows the game: split while a player 2-4 has his unit and camera and the game isn't paused (the
// pause menu and its screens draw in view 0, which then was a half). Only on a change, so a camera event's own
// viewport changes in between are left alone ("Refresh cameras" puts the split back). Entering a mission split only
// after "Refresh cameras" before: the extra camera's setup took its viewport but nothing set the modes.

void update_split() {
    bool extra = false;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        extra |= players[i].unit != 0 && players[i].cam != nullptr;
    }
    const bool want = extra && !devil4_sdk::is_paused();
    if (want == coop_hud::split_shown) {
        return;
    }
    if (want) {
        apply_viewport_layout();
    } else {
        apply_single_view();
    }
    coop_hud::split_shown = want;
}

void __stdcall setup_cam(void* cam) {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    for (int i = 1; i < (int)Coop::player_num; i++) {
        if (cam == players[i].cam) {
            s_cam->viewports[i].mpCamera = cam;
            s_cam->viewports[i].mAttr = 0x17;
            return;
        }
    }
    players[0].cam = (uCameraCtrl*)cam;
    apply_viewport_layout();
}

// True while `unit` is linked in move line `line` and not being deleted. Only pointers are compared until a match, so
// a freed unit is never read.
bool in_line(int line, const void* unit) {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su || !unit) {
        return false;
    }
    int n = 0;
    for (const cUnit* u = (const cUnit*)su->mMoveLine[line].mTop; u && n < 8192; u = u->mp_next_unit, ++n) {
        if (u == unit) {
            return u->flags.bits.mBeFlag < BEFLAG_PRE_DELETE;
        }
    }
    return false;
}

void drop_viewport_camera(const void* cam) {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam || !cam) {
        return;
    }
    for (int v = 1; v < 8; v++) {
        if (s_cam->viewports[v].mpCamera == cam) {
            s_cam->viewports[v].mpCamera = nullptr;
        }
    }
}

// The game frees units without telling Coop (aRoom::cleanRoom's killUnit frees everything without UATTR_FIX on every
// room change; cutscenes and deaths kill units too): forget the extra players' units that are gone, and take their
// cameras out of sCamera before it draws a freed one.
void check_units() {
    for (int i = 1; i < kMaxPlayers; i++) {
        coop::Player& p = players[i];
        if (p.unit && !in_line(kLinePlayer, (const void*)p.unit)) {
            p.unit = 0;
        }
        if (p.hud && !in_line(kLineHud, p.hud)) {
            p.hud = nullptr;
        }
        if (p.reticle && !in_line(kLineHud, p.reticle)) {
            p.reticle = nullptr;
        }
        if (p.cam && !in_line(kLineExtraCamera, p.cam)) {
            drop_viewport_camera(p.cam);
            p.cam = nullptr;
        }
    }
}

void __stdcall coop_game_tick() {
    check_units();
    update_split();
    bool extra = false;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        extra |= players[i].unit != 0;
    }
    coop_context::game_tick(extra);
    coop_pads::game_tick();
}

// Once a frame on the game thread, before the pad system moves: unit checks, the coop pads and their rumble.
naked_extern void detour2() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        pushad
        call coop_game_tick
        popad
    originalcode:
        mov ecx, [ebp+0x00010388]
        jmp dword ptr [Coop::jmp_ret2]
    }
}

// Camera setup's viewport step: 1 = handled here (skip the game's viewport 0 assignment).
uint8_t cam_setup_handled = 0;

void __stdcall route_cam_setup(void* cam) {
    cam_setup_handled = 0;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        if (cam == players[i].cam) {   // an extra camera: checked first, its setup runs in its own context (mpCamera = itself)
            setup_cam(cam);
            cam_setup_handled = 1;
            return;
        }
    }
    const uintptr_t med = *(uintptr_t*)sMediator_ptr;
    const uintptr_t active = med ? *(uintptr_t*)(med + 0xD0) : 0;
    if (active && active != (uintptr_t)cam) {   // a new camera while another is active: the new main one
        setup_cam(cam);
        cam_setup_handled = 1;
    }
}

// Camera setup (esi = camera): extra cameras take their own viewport.
naked_extern void detour5() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        pushad
        push esi
        call route_cam_setup
        popad
        cmp byte ptr [cam_setup_handled], 1
        jne originalcode
        jmp dword ptr [uCameraCtrl_setup_end]
    originalcode:
        mov ecx, [0x00E552D4]
        mov ecx, [ecx]
        jmp dword ptr [Coop::jmp_ret5]
    }
}

// uInteractiveWater::draw runs once per view. The water's first move 0xAC0E90 builds only vertex phases 0 and 1 and
// sets mTransInitialize (+0x1B4); the next draw takes the first-draw path 0xAC443E and clears it (0xAC4A50), and every
// later draw copies phase 2 (mpVBWaterPhase2[0] +0x154), which exists from the second move. With two views on a room's
// first frame, view 0 clears the flag and view 1 copied from null. That view now skips the water until phase 2 exists.
// ebx = the water.
constexpr uintptr_t kWaterFirstDraw = 0xAC443E;
constexpr uintptr_t kWaterDrawEnd   = 0xAC4A57;   // the epilogue: pops edi / esi / ebx, retn 4
uintptr_t water_first_draw = kWaterFirstDraw;
uintptr_t water_draw_end = kWaterDrawEnd;

naked_extern void detour_water() {
    __asm {
        cmp byte ptr [ebx+0x1B4], 0
        jne first_draw
        cmp dword ptr [ebx+0x154], 0
        je no_phase2
        jmp dword ptr [Coop::jmp_ret_water]
    first_draw:
        jmp dword ptr [water_first_draw]
    no_phase2:
        jmp dword ptr [water_draw_end]
    }
}

bool __stdcall context_enter(uintptr_t unit) {
    return coop_context::enter(unit);
}

void __stdcall context_leave() {
    coop_context::leave();
}

void __stdcall context_leave_tick(uintptr_t unit) {
    coop_hud::after_update(unit);
    coop_context::leave();
}

// sUnit::move's unit init (esi = unit), serial tick (esi), pass 2 (edi) and pass 3 (esi): the unit's update between
// the context swap.
// eax / ecx / edx are free afterwards (the next instruction reloads sDevil4Main); the stdcall helpers keep the rest.
naked_extern void detour_unit_init() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        push esi
        call context_enter
        mov eax, [esi]
        mov edx, [eax+0x14]
        mov ecx, esi
        call edx
        call context_leave
        jmp dword ptr [Coop::jmp_ret_unit_init]
    originalcode:
        mov eax, [esi]
        mov edx, [eax+0x14]
        mov ecx, esi
        call edx
        jmp dword ptr [Coop::jmp_ret_unit_init]
    }
}

naked_extern void detour_unit_tick() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        push esi
        call context_enter
        mov edx, [esi]
        mov eax, [edx+0x18]
        mov ecx, esi
        call eax
        push esi
        call context_leave_tick
        jmp dword ptr [Coop::jmp_ret_unit_tick]
    originalcode:
        mov edx, [esi]
        mov eax, [edx+0x18]
        mov ecx, esi
        call eax
        jmp dword ptr [Coop::jmp_ret_unit_tick]
    }
}

naked_extern void detour_unit_pass2() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        push edi
        call context_enter
        mov edx, [edi]
        mov eax, [edx+0x1C]
        mov ecx, edi
        call eax
        call context_leave
        jmp dword ptr [Coop::jmp_ret_unit_pass2]
    originalcode:
        mov edx, [edi]
        mov eax, [edx+0x1C]
        mov ecx, edi
        call eax
        jmp dword ptr [Coop::jmp_ret_unit_pass2]
    }
}

naked_extern void detour_unit_pass3() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        push esi
        call context_enter
        mov eax, [esi]
        mov edx, [eax+0x20]
        mov ecx, esi
        call edx
        call context_leave
        jmp dword ptr [Coop::jmp_ret_unit_pass3]
    originalcode:
        mov eax, [esi]
        mov edx, [eax+0x20]
        mov ecx, esi
        call edx
        jmp dword ptr [Coop::jmp_ret_unit_pass3]
    }
}

// uPlWpRightHand::uCollisionCtrl::grabHitMessage (thiscall, two cCollisionGroup*): the Buster / Snatch hit on an enemy,
// run in the context of the hand's Nero so the enemy's grab starts from that Nero (see CoopContext.hpp).
constexpr uintptr_t kGrabHitMessage = 0x83DFE0;
constexpr uintptr_t kHandClsnToParentPl = 0x1B84;   // uPlWpRightHand::mpParentPl +0x2F34 - clsn +0x13B0
using GrabHitFn = int(__fastcall*)(uintptr_t clsn, uintptr_t edx, void* group, void* target);
GrabHitFn grab_hit_original = nullptr;

int __fastcall grab_hit_message(uintptr_t clsn, uintptr_t edx, void* group, void* target) {
    const uintptr_t owner = *(uintptr_t*)(clsn + kHandClsnToParentPl);
    const bool swapped = Coop::mod_enabled && coop_context::enter_player(owner);
    const int result = grab_hit_original(clsn, edx, group, target);
    if (Coop::mod_enabled) {
        coop_context::note_grab_hit(owner, swapped, target);
    }
    if (swapped) {
        coop_context::leave();
    }
    return result;
}

// uPlayer::setGameSystem 0x7A91C0 (vtable slot 93, run when a player is set up): sMediator::mpPlayer = this, then
// sub_4A5AA0 resets sStylishCount (the style meter). Players 2-4 skip it, or the last one spawned becomes the game's
// player for every mpPlayer read outside CoopContext's swaps (player 1's Snatch took player 2's arm return point, his
// Buster started from player 2) and player 1's style rank is wiped.
constexpr uintptr_t kSetGameSystem = 0x7A91C0;
using SetGameSystemFn = int(__fastcall*)(uintptr_t player, uintptr_t edx);
SetGameSystemFn set_game_system_original = nullptr;

int __fastcall set_game_system(uintptr_t player, uintptr_t edx) {
    if (Coop::mod_enabled) {
        for (int i = 1; i < kMaxPlayers; i++) {
            if (player && players[i].unit == player) {
                return 0;
            }
        }
    }
    return set_game_system_original(player, edx);
}

// uPlayer::updateLockOn (thiscall, returns cLockOnTarget*): players 2-4 pick targets from their own distances and view
// (see CoopContext.hpp, lock_on_begin).
constexpr uintptr_t kUpdateLockOn = 0x820840;
using UpdateLockOnFn = void*(__fastcall*)(uintptr_t player, uintptr_t edx);
UpdateLockOnFn update_lock_on_original = nullptr;

void* __fastcall update_lock_on(uintptr_t player, uintptr_t edx) {
    if (!Coop::mod_enabled) {
        return update_lock_on_original(player, edx);
    }
    coop_context::inter_flags_for(player);
    const bool began = coop_context::lock_on_begin(player);
    void* result = update_lock_on_original(player, edx);
    coop_context::lock_on_report(player);
    if (began) {
        coop_context::lock_on_end();
    }
    return result;
}

// A character's save data in the mission: sGameState::player_spawn_state -> sub_494B30 copies ONE block of sSave into
// sMediator +0x368, Nero's (sSave +0x1D8, 0x3A dwords) in a Nero mission, Dante's (+0xD8, 0x40 dwords) otherwise, and the
// players read it through uPlayerNero::getCommonSaveData 0x7E2AF0 (vtable +0x1E0 of both: equipped weapon ids, the
// first 0x38 bytes) and getProperSaveData (+0x2D4: Dante 0x7B3700, 0x20 bytes at +0x38, which guns exist; Nero 0x7E2B30,
// 7 bytes at +0x38). A Dante player 2 in a Nero mission got Nero's ids and gun flags, and uPlayer::cWeaponCtrl::
// setShortEnable 0x7A5630 called a gun he never created (null, 0x7A5653). Players 2-4 of the other character read their
// own block of sSave instead (same layout: the mission block is a plain copy).
constexpr uintptr_t kSave = 0xE558C8;   // sSave::mpInstance
constexpr uintptr_t kSaveDante = 0xD8, kSaveNero = 0x1D8;
constexpr uintptr_t kPlayerID = 0x1494;  // uPlayer::mPlayerID (0 Dante, 1 Nero)

const uint8_t* own_save_block(uintptr_t player) {
    if (!Coop::mod_enabled || player_info::index_of(player) < 1 || !players[0].unit) {
        return nullptr;
    }
    const uint32_t id = *(uint32_t*)(player + kPlayerID);
    if (id == *(uint32_t*)(players[0].unit + kPlayerID) || id > 1) {
        return nullptr;   // player 1's character: the mission block is his
    }
    const uintptr_t save = *(uintptr_t*)kSave;
    return save ? (const uint8_t*)(save + (id == 0 ? kSaveDante : kSaveNero)) : nullptr;
}

using SaveDataFn = void*(__fastcall*)(uintptr_t player, uintptr_t edx, void* dst);
SaveDataFn common_save_original = nullptr, dante_save_original = nullptr, nero_save_original = nullptr;

void* __fastcall common_save_data(uintptr_t player, uintptr_t edx, void* dst) {
    if (const uint8_t* block = own_save_block(player)) {
        std::memcpy(dst, block, 0x38);
        return dst;
    }
    return common_save_original(player, edx, dst);
}

void* __fastcall dante_save_data(uintptr_t player, uintptr_t edx, void* dst) {
    if (const uint8_t* block = own_save_block(player)) {
        std::memcpy(dst, block + 0x38, 0x20);
        return dst;
    }
    return dante_save_original(player, edx, dst);
}

void* __fastcall nero_save_data(uintptr_t player, uintptr_t edx, void* dst) {
    if (const uint8_t* block = own_save_block(player)) {
        std::memcpy(dst, block + 0x38, 7);
        return dst;
    }
    return nero_save_original(player, edx, dst);
}

// sCamera::isDispSphere (edi = MtSphere, esi = sCamera, result in al): in a player 2-4's context, his viewport.
constexpr uintptr_t kIsDispSphere = 0x8EE300;
uintptr_t disp_sphere_original = 0;

bool __stdcall disp_sphere_for_view(const void* camera_system, const float* sphere) {
    return coop_context::sphere_in_viewport(camera_system, coop_context::view_slot, sphere);
}

naked_extern void detour_disp_sphere() {
    __asm {
        cmp dword ptr [coop_context::view_slot], 1
        jl originalcode
        push edi
        push esi
        call disp_sphere_for_view
        ret
    originalcode:
        jmp dword ptr [disp_sphere_original]
    }
}

bool __stdcall pad_update_fallback(cPeripheral* peripheral) {
    return coop_pads::update_peripheral(peripheral);
}

const char* kCharNames[] = {"Nero", "Dante"};
const char* kPadNames[] = {"XInput 1", "XInput 2", "XInput 3", "XInput 4"};
const char* kP1PadNames[] = {"Game's pad", "XInput 1", "XInput 2", "XInput 3", "XInput 4"};

} // namespace

void Coop::toggle(bool enable) {
    if (enable) {
        devil4_sdk::get_stuff_from_files((MtDTI*)0x00EAD4A0, "rom\\player\\uPlayerNero",
                                        MODE_ASYNC | MODE_USECACHE | MODE_QUALITY_HIGHEST);
        devil4_sdk::get_stuff_from_files((MtDTI*)0x00EAD4A0, "rom\\player\\uPlayerDante",
                                        MODE_ASYNC | MODE_USECACHE | MODE_QUALITY_HIGHEST);
        install_patch_offset(0xDE86, patch1, "\xEB\x13", 2); // no pause on a pad disconnect
    } else {
        patch1.reset();
        apply_single_view();
        coop_hud::split_shown = false;
        coop_pads::stop_all();
        coop_diag::release();   // Coop's game tick no longer runs: put the lines' threading back now
    }
    CharSwitcher::blocked = enable; // it swaps the game's player too: CharSwitcher turns itself off on its next frame
}

std::optional<std::string> Coop::on_initialize() {
    if (!coop_pads::init()) {
        spdlog::warn("Coop: no XInput DLL, players 2-4 have no pad\n");
    }
    struct Site { uintptr_t addr; std::unique_ptr<FunctionHook>* hook; void* detour; uintptr_t* ret; int len; const char* what; };
    const Site sites[] = {
        {coop_pads::kTickSite, &hook2, (void*)&detour2, &jmp_ret2, 6, "pad tick"},
        {0x4F75DA, &hook5, (void*)&detour5, &jmp_ret5, 6, "camera setup"},
        {coop_pads::kKbMergeSite, &hook_kb_merge, (void*)&coop_pads::detour_kb_merge, &coop_pads::ret_kb_merge, 6, "keyboard merge"},
        {coop_pads::kKbExceedSite, &hook_kb_exceed, (void*)&coop_pads::detour_kb_exceed, &coop_pads::ret_kb_exceed, 6, "keyboard Exceed"},
        {coop_pads::kVibSite, &hook_vib, (void*)&coop_pads::detour_vib, &coop_pads::ret_vib, 10, "rumble"},
        {coop_pads::kVibStopSite, &hook_vib_stop, (void*)&coop_pads::detour_vib_stop, &coop_pads::ret_vib_stop, 6, "rumble stop"},
        {0xAC205B, &hook_water, (void*)&detour_water, &jmp_ret_water, 13, "water draw per view"},
        {0x8DC324, &hook_unit_init, (void*)&detour_unit_init, &jmp_ret_unit_init, 9, "unit init context"},
        {0x8DC39C, &hook_unit_tick, (void*)&detour_unit_tick, &jmp_ret_unit_tick, 9, "unit tick context"},
        {0x8DC44B, &hook_unit_pass2, (void*)&detour_unit_pass2, &jmp_ret_unit_pass2, 9, "unit pass 2 context"},
        {0x8DC4C4, &hook_unit_pass3, (void*)&detour_unit_pass3, &jmp_ret_unit_pass3, 9, "unit pass 3 context"},
    };
    for (const Site& s : sites) {
        if (!install_hook_absolute(s.addr, *s.hook, s.detour, s.ret, s.len)) {
            spdlog::error("Failed to init Coop ({})\n", s.what);
            return std::string("Failed to init Coop: ") + s.what;
        }
    }
    hook_grab_hit = std::make_unique<FunctionHook>(kGrabHitMessage, (uintptr_t)&grab_hit_message);
    if (!hook_grab_hit->create()) {
        spdlog::error("Failed to init Coop (grab hit context)\n");
        return "Failed to init Coop: grab hit context";
    }
    grab_hit_original = hook_grab_hit->get_original<std::remove_pointer_t<GrabHitFn>>();
    hook_set_game_system = std::make_unique<FunctionHook>(kSetGameSystem, (uintptr_t)&set_game_system);
    if (!hook_set_game_system->create()) {
        spdlog::error("Failed to init Coop (setGameSystem)\n");
        return "Failed to init Coop: setGameSystem";
    }
    set_game_system_original = hook_set_game_system->get_original<std::remove_pointer_t<SetGameSystemFn>>();
    hook_lock_on = std::make_unique<FunctionHook>(kUpdateLockOn, (uintptr_t)&update_lock_on);
    if (!hook_lock_on->create()) {
        spdlog::error("Failed to init Coop (lock-on)\n");
        return "Failed to init Coop: lock-on";
    }
    update_lock_on_original = hook_lock_on->get_original<std::remove_pointer_t<UpdateLockOnFn>>();
    hook_disp_sphere = std::make_unique<FunctionHook>(kIsDispSphere, (uintptr_t)&detour_disp_sphere);
    if (!hook_disp_sphere->create()) {
        spdlog::error("Failed to init Coop (isDispSphere)\n");
        return "Failed to init Coop: isDispSphere";
    }
    disp_sphere_original = hook_disp_sphere->get_original();
    struct TrampolineSite { uintptr_t addr; std::unique_ptr<FunctionHook>* hook; void* detour; SaveDataFn* original; const char* what; };
    const TrampolineSite save_sites[] = {
        {0x7E2AF0, &hook_common_save, (void*)&common_save_data, &common_save_original, "getCommonSaveData"},
        {0x7B3700, &hook_dante_save, (void*)&dante_save_data, &dante_save_original, "Dante getProperSaveData"},
        {0x7E2B30, &hook_nero_save, (void*)&nero_save_data, &nero_save_original, "Nero getProperSaveData"},
    };
    for (const TrampolineSite& s : save_sites) {
        *s.hook = std::make_unique<FunctionHook>(s.addr, (uintptr_t)s.detour);
        if (!(*s.hook)->create()) {
            spdlog::error("Failed to init Coop ({})\n", s.what);
            return std::string("Failed to init Coop: ") + s.what;
        }
        *s.original = (*s.hook)->get_original<std::remove_pointer_t<SaveDataFn>>();
    }
    if (auto err = coop_hud::install()) {
        spdlog::error("{}\n", *err);
        return err;
    }
    Macro::set_pad_update_fallback(&pad_update_fallback);
    return Mod::on_initialize();
}

void Coop::on_frame(fmilliseconds& dt) {
    coop_diag::on_frame(dt.count());
}

void Coop::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) {
        return;
    }
    ImGui::PushID("Coop");
    if (ImGui::Checkbox(_("Coop mode"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Split-screen co-op. The extra players spawn when the next stage starts"));
    ImGui::SameLine(sameLineWidth);
    ImGui::SetNextItemWidth(sameLineItemWidth / 2.0f);
    ImGui::SliderInt(_("Players"), (int*)&player_num, 2, kMaxPlayers);
    ImGui::SameLine();
    help_marker(_("How many players, including you. Change it before the stage starts"));

    if (!mod_enabled) {
        ImGui::PopID();
        return;
    }
    ImGui::Indent(lineIndent);
    for (int i = 0; i < (int)player_num; i++) {
        ImGui::PushID(i);
        ImGui::Text(_("Player %d"), i + 1);
        if (i > 0) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(sameLineItemWidth);
            ImGui::Combo(_("Character"), &players[i].character, kCharNames, IM_ARRAYSIZE(kCharNames));
            ImGui::SameLine();
            help_marker(_("Who this player spawns as, at the next stage start"));
            ImGui::SameLine();
            ImGui::SetNextItemWidth(sameLineItemWidth / 1.5f);
            ImGui::Combo(_("Pad"), &players[i].xinput, kPadNames, IM_ARRAYSIZE(kPadNames));
            ImGui::SameLine();
            help_marker(_("The XInput controller this player uses"));
        } else {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(sameLineItemWidth / 1.5f);
            int p1_pad = players[0].xinput + 1;
            if (ImGui::Combo(_("Pad"), &p1_pad, kP1PadNames, IM_ARRAYSIZE(kP1PadNames))) {
                players[0].xinput = p1_pad - 1;
                coop_pads::stop_all();
            }
            ImGui::SameLine();
            help_marker(_("Game's pad: player 1 plays on whatever the game reads (its own controller settings). An XInput "
                          "pad: player 1 uses that controller like the other players, with his own button layout below "
                          "and rumble on it. Menus still follow the game's pad"));
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(coop_pads::connected(i) ? _("connected") : _("no pad"));
        if (coop_pads::uses_stand_in(i)) {
            ImGui::Indent(lineIndent);
            coop_pads::draw_bindings(i);
            ImGui::Unindent(lineIndent);
        }
        ImGui::PopID();
    }

    int owner = coop_pads::keyboard_owner + 1;
    const char* owners[] = {"Nobody", "Player 1", "Player 2", "Player 3", "Player 4"};
    ImGui::SetNextItemWidth(sameLineItemWidth);
    if (ImGui::Combo(_("Keyboard"), &owner, owners, (int)player_num + 1)) {
        coop_pads::set_keyboard_owner(owner - 1);
    }
    ImGui::SameLine();
    help_marker(_("Which player the keyboard controls (buttons, movement keys, Exceed key). The others ignore it"));
    ImGui::SameLine(sameLineWidth + lineIndent);
    if (ImGui::Checkbox(_("Rumble for players 2-4"), &coop_pads::rumble) && !coop_pads::rumble) {
        coop_pads::stop_all();
    }
    ImGui::SameLine();
    help_marker(_("Players 2-4 feel their own hits on their own pad. Player 1's rumble is the game's"));

    ImGui::SetNextItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("HUD size"), &coop_hud::hud_scale, 0.25f, 1.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("Every player's HUD (gauges, timer, minimap) is shrunk by this toward the corner of their part of the "
                  "screen while it is split (full size while paused). 0.5 fits a half or a quarter of the screen, 1 is the normal size. The "
                  "lock-on reticle keeps its size, as it follows the target"));

    if (ImGui::Button(_("Refresh cameras"))) {
        apply_viewport_layout();
        coop_hud::split_shown = true;
    }
    ImGui::SameLine();
    help_marker(_("Give every camera its viewport again, after a camera event took the screen back"));

    if (ImGui::TreeNode(_("Diagnostics"))) {
        if (ImGui::Checkbox(_("Serial context lines"), &coop_diag::serial_lines)) {
            coop_diag::set_serial(coop_diag::serial_lines);
        }
        ImGui::SameLine();
        help_marker(_("Runs the player, stand, enemy, camera and HUD move lines one unit at a time instead of on the job "
                      "threads (on by default). They run that way anyway while players 2-4 are in the room: the players' "
                      "cameras, lock-on and grabs need it. Off only to compare frame times"));
        ImGui::Text(_("Lines serial now: %s, context swaps last frame: %u"), coop_diag::serial_now() ? _("yes") : _("no"),
                    coop_context::swaps_last_frame());
        coop_context::draw_diag();
        coop_diag::draw();
        ImGui::TreePop();
    }
    ImGui::Unindent(lineIndent);
    ImGui::PopID();
}

void Coop::on_stage_start() {
    if (!mod_enabled) {
        return;
    }
    sUnit* s_unit = devil4_sdk::get_sUnit();
    coop::Player& main_pl = players[0];
    main_pl.unit = (uintptr_t)s_unit->mMoveLine[kLinePlayer].mTop;
    main_pl.cam = (uCameraCtrl*)s_unit->mMoveLine[kLineCamera].mTop;
    main_pl.hud = s_unit->mMoveLine[kLineHud].mTop;
    if (main_pl.hud) {
        ((cUnit*)main_pl.hud)->flags.bits.mTransView = 1;
    }
    if (main_pl.unit) {
        *reinterpret_cast<uint8_t*>(main_pl.unit + coop::kControlNumber) = 0;
    }
    for (int i = 1; i < (int)player_num; i++) {
        spawn_player(players[i].character, i);
        players[i].cam = make_cam();
    }
}

void Coop::on_stage_end() {
    // aRoom::cleanRoom is about to free the extra cameras: sCamera must not keep them. The screens after the mission
    // (results, shop) draw in view 0, which goes back to the whole screen.
    apply_single_view();
    coop_hud::split_shown = false;
    for (int i = 1; i < kMaxPlayers; i++) {
        drop_viewport_camera(players[i].cam);
    }
    for (int i = 0; i < kMaxPlayers; i++) {
        players[i].unit = 0;
        players[i].cam = nullptr;
        players[i].hud = nullptr;
        players[i].reticle = nullptr;
    }
    coop_context::reset();
    coop_pads::stop_all();
}

void Coop::on_config_load(const utility::Config& cfg) {
    player_num = (unsigned)std::clamp(cfg.get<int>("coop_players").value_or(2), 2, kMaxPlayers);
    players[0].xinput = std::clamp(cfg.get<int>("coop_p1_pad").value_or(-1), -1, 3);
    for (int b = 0; b < coop_pads::kButtons; b++) {
        const int v = cfg.get<int>("coop_p1_button" + std::to_string(b)).value_or(b);
        coop_pads::remap[0][b] = std::clamp(v, -1, coop_pads::kButtons - 1);
    }
    for (int i = 1; i < kMaxPlayers; i++) {
        const std::string n = std::to_string(i + 1);
        players[i].character = std::clamp(cfg.get<int>("coop_p" + n + "_character").value_or(0), 0, 1);
        players[i].xinput = std::clamp(cfg.get<int>("coop_p" + n + "_pad").value_or(i), 0, 3);
        for (int b = 0; b < coop_pads::kButtons; b++) {
            const int v = cfg.get<int>("coop_p" + n + "_button" + std::to_string(b)).value_or(b);
            coop_pads::remap[i][b] = std::clamp(v, -1, coop_pads::kButtons - 1);
        }
    }
    coop_pads::set_keyboard_owner(std::clamp(cfg.get<int>("coop_keyboard").value_or(0), -1, kMaxPlayers - 1));
    coop_pads::rumble = cfg.get<bool>("coop_rumble").value_or(true);
    coop_hud::hud_scale = std::clamp(cfg.get<float>("coop_hud_scale").value_or(0.5f), 0.25f, 1.0f);
}

void Coop::on_config_save(utility::Config& cfg) {
    cfg.set<int>("coop_players", (int)player_num);
    cfg.set<int>("coop_p1_pad", players[0].xinput);
    for (int b = 0; b < coop_pads::kButtons; b++) {
        cfg.set<int>("coop_p1_button" + std::to_string(b), coop_pads::remap[0][b]);
    }
    for (int i = 1; i < kMaxPlayers; i++) {
        const std::string n = std::to_string(i + 1);
        cfg.set<int>("coop_p" + n + "_character", players[i].character);
        cfg.set<int>("coop_p" + n + "_pad", players[i].xinput);
        for (int b = 0; b < coop_pads::kButtons; b++) {
            cfg.set<int>("coop_p" + n + "_button" + std::to_string(b), coop_pads::remap[i][b]);
        }
    }
    cfg.set<int>("coop_keyboard", coop_pads::keyboard_owner);
    cfg.set<bool>("coop_rumble", coop_pads::rumble);
    cfg.set<float>("coop_hud_scale", coop_hud::hud_scale);
}

#undef naked_extern
#pragma pop_macro("naked")
