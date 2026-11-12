#include "Coop.hpp"
#include "CoopPads.hpp"
#include "CoopDiag.hpp"
#include "CoopContext.hpp"
#include "CoopHud.hpp"
#include "CoopPvp.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/Players.hpp"
#include "../../mods/CharSwitcher.hpp"
#include "../../mods/DebugCam.hpp" // REGION_* viewport modes; the free camera (Gameplay Cam off) takes the screen
#include "../../mods/Macro.hpp"
#include "../../mods/ViewportWindow.hpp"
#include <cstring>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

// The camera has to come back to its player after a camera event. These read sCamera when the BP portal transition
// finishes (in order): 0x40C89E mov edx,[0xE552D4]; 0x4F7B50 mov ecx,[0xE552D4]; 0x4F7BB6 mov eax,[0xE552D4].

bool Coop::mod_enabled = false;
unsigned int Coop::player_num = 2;

namespace coop_tint {
bool enabled = false;
float colors[4][4] = {{1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, 0.6f, 0.6f, 0.0f}, {0.6f, 0.6f, 1.0f, 0.0f}, {0.6f, 1.0f, 0.6f, 0.0f}};
} // namespace coop_tint
float Coop::revive_delay = 5.0f;
float Coop::revive_hp = 0.5f;

uintptr_t Coop::jmp_ret2 = NULL;
uintptr_t Coop::jmp_ret5 = NULL;
uintptr_t Coop::jmp_ret_water = NULL;
uintptr_t Coop::jmp_ret_unit_tick = NULL;
uintptr_t Coop::jmp_ret_unit_pass2 = NULL;
uintptr_t Coop::jmp_ret_unit_pass3 = NULL;
uintptr_t Coop::jmp_ret_unit_init = NULL;
uintptr_t Coop::jmp_ret_em_step = NULL;

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
constexpr uintptr_t kResourceAddRef = 0x8E1F00; // cResource::addRef (edi = resource)
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

// cResource::addRef 0x8E1F00 (edi = the resource): uCameraCtrl's destructor 0x4F7440 releases mpResource (+0x3A0), so
// a shared one needs its own reference.
void add_ref(void* resource) {
    const uintptr_t fn = kResourceAddRef;
    __asm {
        mov edi, resource
        call fn
    }
}

// As aRoom::setRoom 0x40DB4C builds the room's camera: the room's rDevilCamera (scr\stXXX\cam\stXXX: the follow
// camera's distance, height, angles, field of view per area), shared with player 1's camera; uCameraCtrl::setupDevelop
// only when there is none (the game's fallback: it replaces mpResource with a default cCameraNormal setup, the
// developer camera, which is what the extra cameras used to get: closer than the room's).
uCameraCtrl* make_cam() {
    uCameraCtrl* cam = (uCameraCtrl*)construct(kCameraCtor);
    devil4_sdk::sUnit_spawn(cam, kLineExtraCamera);
    uCameraCtrl* main_cam = players[0].cam;
    if (!main_cam) {
        main_cam = (uCameraCtrl*)devil4_sdk::get_sUnit()->mMoveLine[kLineCamera].mTop;
        players[0].cam = main_cam;
    }
    void* resource = main_cam ? (void*)main_cam->mpResource : nullptr;
    if (resource) {
        add_ref(resource);
        cam->mpResource = decltype(cam->mpResource)(resource);
    } else {
        setup_develop(cam);
    }
    return cam;
}

const int kModes[3][4] = {
    {REGION_TOP, REGION_BOTTOM, 0, 0},
    {REGION_TOPLEFT, REGION_TOPRIGHT, REGION_BOTTOMLEFT, 0},
    {REGION_TOPLEFT, REGION_TOPRIGHT, REGION_BOTTOMLEFT, REGION_BOTTOMRIGHT},
};

// The viewport ViewportWindow shows in its window when it is one of the players' (-1: none). That one keeps its own
// settings (off the screen, its render target); the views left on the screen share it.
int layout_detached = -1;

int detached_player_view() {
    const int v = vpwin::detached_viewport();
    return (v >= 1 && v < (int)Coop::player_num && v < kMaxPlayers) ? v : -1;
}

// The mode of each player's viewport on the screen (-1 = in the window).
void screen_modes(int detached, int* modes) {
    const int n = (int)Coop::player_num;
    int order[kMaxPlayers];
    int m = 0;
    for (int i = 0; i < n && i < kMaxPlayers; i++) {
        modes[i] = -1;
        if (i != detached) order[m++] = i;
    }
    for (int k = 0; k < m; k++) {
        modes[order[k]] = m >= 2 ? kModes[m - 2][k] : REGION_FULLSCREEN;
    }
}

// The split layout. `main_camera`: viewport 0 also gets player 1's follow camera (a new main camera, the Refresh
// button); otherwise viewport 0 keeps the camera it has (a boss camera). A view in ViewportWindow's window only gets
// its player's camera; the others share the screen (one left: the whole screen), and only their HUDs are scaled.
void apply_viewport_layout(bool main_camera = true) {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam) {
        return;
    }
    const int n = (int)Coop::player_num;
    const int detached = detached_player_view();
    int modes[kMaxPlayers];
    screen_modes(detached, modes);
    int on_screen = 0;
    for (int i = 0; i < n && i < kMaxPlayers; i++) {
        on_screen += modes[i] >= 0 ? 1 : 0;
    }
    for (int i = 0; i < n && i < kMaxPlayers; i++) {
        if (i > 0 || (main_camera && players[0].cam)) {   // viewport 0 keeps the game's camera until Coop knows player 1's
            s_cam->viewports[i].mpCamera = players[i].cam;
        }
        coop_hud::view_scaled[i] = modes[i] >= 0 && on_screen >= 2;
        if (modes[i] < 0) {
            continue;
        }
        s_cam->viewports[i].mAttr = 0x17;
        s_cam->viewports[i].mMode = (uint8_t)modes[i];
    }
    layout_detached = detached;
}

// Player 1's view on the whole screen, the extra views off (a viewport without a camera isn't drawn).
void apply_single_view() {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam) {
        return;
    }
    s_cam->viewports[0].mMode = REGION_FULLSCREEN;
    const int detached = vpwin::detached_viewport();   // the window keeps its view
    for (int v = 1; v < kMaxPlayers; v++) {
        if (v == detached) continue;
        for (int i = 1; i < kMaxPlayers; i++) {
            if (players[i].cam && s_cam->viewports[v].mpCamera == players[i].cam) {
                s_cam->viewports[v].mpCamera = nullptr;
            }
        }
    }
}

// Viewport 0 shows a player's view: player 1's follow camera, or the boss camera (sMediator +0xD4, uCameraBoss::setup
// 0x4C39E0; it stays player 1's, the others keep their follow cameras). Anything else there is an event camera
// (uIdCamera 0x7495C0, uItemGetCamera, uPowerUpCamera, uGameOverCamera, uMotionCamera ...: their setup puts them on
// viewport 0 through sCamera::setCamera and their end puts the saved camera back).
bool player_view_on_main(const sCamera* s_cam) {
    const void* cam0 = s_cam->viewports[0].mpCamera;
    if (!cam0) {
        return false;
    }
    if (cam0 == players[0].cam) {
        return true;
    }
    const uintptr_t med = *(uintptr_t*)sMediator_ptr;
    return med && cam0 == *(void**)(med + 0xD4);
}

// The split's modes or extra cameras were changed under it: uCameraCtrl::resumeCamera 0x4F7B50 / resetCamera 0x4F7A90
// (portals, the end of an event) set viewport 0 back to the whole screen.
bool layout_broken(const sCamera* s_cam) {
    const int n = (int)Coop::player_num;
    const int detached = detached_player_view();
    if (detached != layout_detached) {   // the window opened, closed or took another view
        return true;
    }
    int modes[kMaxPlayers];
    screen_modes(detached, modes);
    if (s_cam->viewports[0].mMode != (uint8_t)modes[0]) {
        return true;
    }
    for (int i = 1; i < n && i < kMaxPlayers; i++) {
        if (players[i].cam && s_cam->viewports[i].mpCamera != players[i].cam) {
            return true;
        }
    }
    return false;
}

// The split screen follows the game, checked every game tick: split while a player 2-4 has his unit and camera, the
// game isn't paused (the pause menu and its screens draw in view 0), viewport 0 shows a player's view and the free
// camera is off; else player 1's view on the whole screen, so events and cutscenes play full screen. While split, a
// layout the game changed is put back (no Refresh needed after a portal or an event). DebugCam's / PhotoMode's free
// camera (Gameplay Cam off) hides viewport 0 and puts itself full screen in viewport 1, player 2's: the split waits
// until Gameplay Cam is back on.
void update_split() {
    sCamera* s_cam = *(sCamera**)sCamera_ptr;
    if (!s_cam) {
        return;
    }
    bool extra = false;
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        extra |= players[i].unit != 0 && players[i].cam != nullptr;
    }
    const bool want = extra && !devil4_sdk::is_paused() && player_view_on_main(s_cam) && DebugCam::toggle_gameplay_cam;
    if (want) {
        if (!coop_hud::split_shown || layout_broken(s_cam)) {
            apply_viewport_layout(false);
            coop_hud::split_shown = true;
        }
    } else if (coop_hud::split_shown) {
        apply_single_view();
        coop_hud::split_shown = false;
    }
}

// uCameraCtrl::sync 0x4F90B0 writes its fog (sCamera +0x1E0..+0x21C = viewport 0's +0x1B0..+0x1EC: mFogStart / End,
// the plane, the colour ...) into viewport 0 three times, after loading sCamera::mpInstance at 0x4F94F2 and 0x4F985A
// (A1 D4 52 E5 00; the third block at 0x4F9A18 reuses the second load). An extra camera's sync runs in its context,
// so the load is moved to its viewport (sCamera + slot * 0x590): his view had no fog of its own and player 1's
// changed with player 2's room.
constexpr uintptr_t kFogLoad1 = 0x4F94F2;
constexpr uintptr_t kFogLoad2 = 0x4F985A;
uintptr_t jmp_ret_fog1 = 0, jmp_ret_fog2 = 0;

naked_extern void detour_fog1() {
    __asm {
        pushfd
        push ecx
        mov ecx, 0xE552D4
        mov ecx, [ecx]
        mov eax, [coop_context::view_slot]
        cmp eax, 1
        jl own_view
        imul eax, eax, 0x590
        add eax, ecx
        jmp done
    own_view:
        mov eax, ecx
    done:
        pop ecx
        popfd
        jmp dword ptr [jmp_ret_fog1]
    }
}

naked_extern void detour_fog2() {
    __asm {
        pushfd
        push ecx
        mov ecx, 0xE552D4
        mov ecx, [ecx]
        mov eax, [coop_context::view_slot]
        cmp eax, 1
        jl own_view
        imul eax, eax, 0x590
        add eax, ecx
        jmp done
    own_view:
        mov eax, ecx
    done:
        pop ecx
        popfd
        jmp dword ptr [jmp_ret_fog2]
    }
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

// Rooms. aRoom::setRoom rebuilds every player, and uPlayer::setup 0x7A7A00 starts each from the state the game keeps in
// sMediator for player 1: HP +0x38 (-> +0x15CC) and its max +0x34 (+0x15D0), Devil Trigger +0x40 (+0x1F24) and its
// max +0x3C (+0x1F28), position +0x60 (mPos) and angle +0x70. So players 2-4 arrived with player 1's HP and DT. Their
// own are kept when a room ends and given back once the new room's players are set up, changed by what the room change
// did to player 1's (a door: nothing; a retry or a checkpoint: refilled), so no door / retry / mission start rule is
// needed. They still arrive at player 1's spot.
constexpr uintptr_t kHp = 0x15CC, kHpMax = 0x15D0, kDt = 0x1F24, kDtMax = 0x1F28;
constexpr uintptr_t kParamTbl = 0x1E9C;   // set by uPlayer::setup: the player is set up

struct Carry { bool saved = false; float hp = 0.0f, dt = 0.0f; };
Carry carry[kMaxPlayers];

float& field(uintptr_t pl, uintptr_t off) { return *reinterpret_cast<float*>(pl + off); }
bool set_up(uintptr_t pl) { return pl && *reinterpret_cast<uintptr_t*>(pl + kParamTbl) != 0; }

void save_carry() {
    for (int i = 0; i < kMaxPlayers; i++) {
        carry[i] = Carry{};
        const uintptr_t pl = players[i].unit;
        if (i < (int)Coop::player_num && set_up(pl)) {
            carry[i] = {true, field(pl, kHp), field(pl, kDt)};
        }
    }
}

void apply_carry() {
    const uintptr_t p1 = players[0].unit;
    if (!carry[0].saved || !set_up(p1)) {
        return;
    }
    const float d_hp = field(p1, kHp) - carry[0].hp;
    const float d_dt = field(p1, kDt) - carry[0].dt;
    bool waiting = false;
    for (int i = 1; i < kMaxPlayers; i++) {
        const uintptr_t pl = players[i].unit;
        if (!carry[i].saved) continue;
        if (!set_up(pl)) { waiting = true; continue; }
        // at least 1 HP until the death rule (step 6) decides what a downed player does
        field(pl, kHp) = std::clamp(carry[i].hp + d_hp, 1.0f, field(pl, kHpMax));
        field(pl, kDt) = std::clamp(carry[i].dt + d_dt, 0.0f, field(pl, kDtMax));
        carry[i].saved = false;
    }
    if (!waiting) {
        carry[0].saved = false;
    }
}

// Death (the user's rule: down, then up again). The game ends the mission only for player 1: aRoom::move's dead state
// (6, sub_40D120) waits for uPlayer::checkDeadEnd 0x826D90 on mpPlayer, outside every player's update. A player 2-4
// whose death motion has ended (checkDeadEnd: lower action 0x15 finished) stays down revive_delay seconds (game time,
// sDevil4Main +0x68, not while paused), then gets uPlayer::mIsReviveReq (+0x216B), the Gold Orb continue's switch:
// his own update (uPlayer::main 0x7A83FA) refills his HP, clears mIsDead (+0x1E82) and the damage block's dead flag
// and starts the revive action. Once it has, his HP is cut to revive_hp of the max. Enemies leave a dead player alone
// (CoopContext's target pick skips mIsDead).
constexpr uintptr_t kReviveReq = 0x216B;
constexpr uintptr_t kIsDead    = 0x1E82;
constexpr uintptr_t kMain      = 0xE5574C;   // sDevil4Main::mpInstance
constexpr uintptr_t kCheckDeadEnd = 0x826D90; // uPlayer::checkDeadEnd (eax = player, al = ended)

struct Down { float frames = 0.0f; bool reviving = false; };
Down down[kMaxPlayers];

bool dead_end(uintptr_t pl) {
    bool ended = false;
    const uintptr_t fn = kCheckDeadEnd;
    __asm {
        mov eax, pl
        call fn
        mov ended, al
    }
    return ended;
}

void update_revive() {
    const uintptr_t main = *reinterpret_cast<uintptr_t*>(kMain);
    const float dt = (main && !devil4_sdk::is_paused()) ? *reinterpret_cast<float*>(main + 0x68) : 0.0f;
    for (int i = 1; i < kMaxPlayers; i++) {
        Down& d = down[i];
        const uintptr_t pl = players[i].unit;
        if (i >= (int)Coop::player_num || !set_up(pl)) {
            d = Down{};
            continue;
        }
        uint8_t& req = *reinterpret_cast<uint8_t*>(pl + kReviveReq);
        if (d.reviving) {
            if (!req) {   // his update took it: up again with full HP
                field(pl, kHp) = std::max(1.0f, field(pl, kHpMax) * Coop::revive_hp);
                d = Down{};
            }
            continue;
        }
        if (!dead_end(pl)) {
            d.frames = 0.0f;
            continue;
        }
        d.frames += dt;
        if (d.frames >= Coop::revive_delay * 60.0f) {
            req = 1;
            d.reviving = true;
        }
    }
}

// Control. uPlayer::main reads the pad only while uPlayer::mIsInterfaceSetting (+0x1509) is set, and the exception
// inputs while mIsEnableExceptionInput (+0x150A). uPlayer::setup 0x7A7BB9 clears both when sMediator +0xA0 is 1 (a
// mission that opens on a script: M12), and the mission script's player control commands write them on mpPlayer only
// (on 0x44BDC6 / 0x44B339, off 0x44BD93): players 2-4 never got control back. They follow player 1's every tick, so
// they also lose it with him in scripted moments.
constexpr uintptr_t kInterfaceSetting = 0x1509;
constexpr uintptr_t kExceptionInput   = 0x150A;

void sync_control() {
    const uintptr_t p1 = players[0].unit;
    if (!set_up(p1)) {
        return;
    }
    for (int i = 1; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const uintptr_t pl = players[i].unit;
        if (!set_up(pl)) continue;
        *reinterpret_cast<uint8_t*>(pl + kInterfaceSetting) = *reinterpret_cast<uint8_t*>(p1 + kInterfaceSetting);
        *reinterpret_cast<uint8_t*>(pl + kExceptionInput) = *reinterpret_cast<uint8_t*>(p1 + kExceptionInput);
    }
}

void __stdcall coop_game_tick() {
    check_units();
    update_split();
    apply_carry();
    update_revive();
    sync_control();
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

// Player tints. uDevil4Model::setBaseMapFactor 0x9F7A80 (thiscall, MtVector3*) writes an RGB multiplier into every
// material of one model (+0x30..+0x38; 1 1 1 = as authored; uDevil4Model::setBMF 0x526B60 is the same with the colour
// times its alpha). The boss Dante's dark colouring is this, every frame after his update (uPlayerDanteBoss::
// movePlayerBefore 0x7C04E0): body, head +0x3270, hair +0x4340, coat +0x52C0 and each weapon through its own method
// (uPlWeapon::setBaseMapFactor, vtable +0x150; uPlayer::mWeaponCtrl.mpWeapon[7] +0x1D90). Dante's own code writes 1 1 1
// back during Devil Trigger changes (uPlayerDante::updateChgDevilType), so a tint goes on after every update of the
// player and, for a Nero, of his stand (mpDevilStand +0xCDF8) and its arm (uPlNeroDevil::mpRightHand +0x1374); Dante's
// Devil Trigger coat (+0xCFC0) too. Turning tints off puts 1 1 1 back once.
constexpr uintptr_t kSetBaseMapFactor = 0x9F7A80;
constexpr uintptr_t kWeaponSetBmf     = 0x150;
constexpr uintptr_t kHeadMdl = 0x3270, kHairMdl = 0x4340, kCoatMdl = 0x52C0, kCoatDevilMdl = 0xCFC0;
constexpr uintptr_t kWeapons = 0x1D90, kStand = 0xCDF8, kStandHand = 0x1374;
constexpr uintptr_t kTintPlayerId = 0x1494;   // uPlayer::mPlayerID (0 Dante, 1 Nero)
using BmfFn = void(__fastcall*)(uintptr_t, uintptr_t, const float*);
bool tint_restore = false;   // tints went off: put 1 1 1 back on the next frame

void set_bmf(uintptr_t model, const float* f) {
    if (model) reinterpret_cast<BmfFn>(kSetBaseMapFactor)(model, 0, f);
}

void tint_player(uintptr_t pl, const float* f) {
    set_bmf(pl, f);
    set_bmf(pl + kHeadMdl, f);
    set_bmf(pl + kHairMdl, f);
    set_bmf(pl + kCoatMdl, f);
    if (*(uint32_t*)(pl + kTintPlayerId) == 0) {
        set_bmf(pl + kCoatDevilMdl, f);
    }
    for (int w = 0; w < 7; w++) {
        const uintptr_t weapon = *(uintptr_t*)(pl + kWeapons + 4 * w);
        if (weapon) reinterpret_cast<BmfFn>(*(uintptr_t*)(*(uintptr_t*)weapon + kWeaponSetBmf))(weapon, 0, f);
    }
}

uintptr_t stand_of(uintptr_t pl) {
    return *(uint32_t*)(pl + kTintPlayerId) == 1 ? *(uintptr_t*)(pl + kStand) : 0;
}

void tint_after_update(uintptr_t unit) {
    if (!coop_tint::enabled || !unit) {
        return;
    }
    for (int i = 0; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl) continue;
        const float* f = coop_tint::colors[i];
        if (unit == pl) {
            tint_player(pl, f);
            return;
        }
        const uintptr_t stand = stand_of(pl);
        if (stand && unit == stand) {
            set_bmf(stand, f);
            set_bmf(*(uintptr_t*)(stand + kStandHand), f);
            return;
        }
        if (stand && unit == *(uintptr_t*)(stand + kStandHand)) {
            set_bmf(unit, f);
            return;
        }
    }
}

void tint_restore_all() {
    const float one[4] = {1.0f, 1.0f, 1.0f, 0.0f};
    for (int i = 0; i < kMaxPlayers; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl) continue;
        tint_player(pl, one);
        if (const uintptr_t stand = stand_of(pl)) {
            set_bmf(stand, one);
            set_bmf(*(uintptr_t*)(stand + kStandHand), one);
        }
    }
}

void __stdcall context_leave_tick(uintptr_t unit) {
    coop_hud::after_update(unit);
    tint_after_update(unit);
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

// uActor::damagePreMessage (edi = the actor hit; stack: attacker group, target group; retn 8): the hit in the context
// of the player it counts for (CoopContext::hit_enter).
constexpr uintptr_t kDamagePreMessage = 0x4A7F30;
uintptr_t damage_pre_original = 0;

bool __stdcall damage_enter(uintptr_t attacker_group, uintptr_t target) {
    return Coop::mod_enabled && coop_context::hit_enter(attacker_group, target);
}

naked_extern void detour_damage_pre() {
    __asm {
        push edi
        push dword ptr [esp+8]
        call damage_enter
        test al, al
        jz plain
        push dword ptr [esp+8]
        push dword ptr [esp+8]
        call dword ptr [damage_pre_original]
        push eax
        call context_leave
        pop eax
        ret 8
    plain:
        jmp dword ptr [damage_pre_original]
    }
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
    coop_pvp::before_lock_on(player);
    coop_context::inter_flags_for(player);
    const bool began = coop_context::lock_on_begin(player);
    coop_pvp::prefer_players(player);
    void* result = update_lock_on_original(player, edx);
    coop_pvp::after_lock_on();
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

// Enemy step off the other players. uPlayer::checkEmJumpBegin 0x804970 (edi = the jumper) looks for a step target on
// move lines 15 / 16 only (the enemies) with uActor::isEmJump 0x4AB170: the actor's step points (+0x1318 a table of
// 32-byte entries: joint, radius, 3 unused, offset in the joint's frame; -1 ends it; +0x131C the mask of points in use,
// +0x1320 on), each against the jumper's position + 85 cm. Enemies get theirs from uEnemy::setup (emjump_data_tbl
// 0xC79070 by kind); players have none and stand on line 13. Before line 16 is scanned (0x804A37: reached only once
// the skill, mIsEmJumpEnable, the action's step bit, the jump press and the step timer have all passed, and line 15
// had no target; every early return shares 0x804A71, so the hook can't go there) the other players are tried with
// step points of their own (the chest, as the Scarecrow's / Angelo's first point); a hit goes on at 0x804A7C as the
// game's: the target's enemyJumpMessage (vtable +0xE8, a nullsub on players), the step timers, action 25.
#pragma pack(push, 1)
struct EmJumpPoint { int32_t joint; float radius, unused[3], x, y, z; };
#pragma pack(pop)
static_assert(sizeof(EmJumpPoint) == 0x20, "em jump entry");
const EmJumpPoint kPlayerStep[] = {
    {2, 150.0f, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f},
    {-1, 0.0f, {0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f},
};
constexpr uintptr_t kEmJumpData = 0x1318, kEmJumpMask = 0x131C, kEmJumpOn = 0x1320;
constexpr uintptr_t kIsEmJump = 0x4AB170;
uintptr_t em_step_target = 0;
uintptr_t em_step_hit = 0x804A7C;
uintptr_t em_step_sunit = 0xE552CC;   // sUnit::mpInstance

uintptr_t __stdcall find_player_step(uintptr_t jumper) {
    em_step_target = 0;
    if (!Coop::mod_enabled) {
        return 0;
    }
    using IsEmJumpFn = bool(__stdcall*)(uintptr_t);
    for (int i = 0; i < (int)Coop::player_num && i < kMaxPlayers; i++) {
        const uintptr_t pl = players[i].unit;
        if (!pl || pl == jumper) continue;
        uintptr_t& data = *reinterpret_cast<uintptr_t*>(pl + kEmJumpData);
        if (data && data != (uintptr_t)kPlayerStep) continue;   // someone else's points: leave them
        data = (uintptr_t)kPlayerStep;
        *reinterpret_cast<uint32_t*>(pl + kEmJumpMask) = 1;
        *reinterpret_cast<uint8_t*>(pl + kEmJumpOn) = 1;
        if (reinterpret_cast<IsEmJumpFn>(kIsEmJump)(pl)) {
            em_step_target = pl;
            return pl;
        }
    }
    return 0;
}

naked_extern void detour_em_step() {
    __asm {
        pushad
        push edi
        call find_player_step
        popad
        mov esi, [em_step_target]
        test esi, esi
        jz enemies
        jmp dword ptr [em_step_hit]
    enemies:
        mov eax, [em_step_sunit]
        mov eax, [eax]
        jmp dword ptr [Coop::jmp_ret_em_step]
    }
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
        {coop_pads::kKbCamSiteX, &hook_kb_cam_x, (void*)&coop_pads::detour_kb_cam_x, &coop_pads::ret_kb_cam_x, 6, "camera keys X"},
        {coop_pads::kKbCamSiteY, &hook_kb_cam_y, (void*)&coop_pads::detour_kb_cam_y, &coop_pads::ret_kb_cam_y, 6, "camera keys Y"},
        {coop_pads::kKbCamSiteY2, &hook_kb_cam_y2, (void*)&coop_pads::detour_kb_cam_y2, &coop_pads::ret_kb_cam_y2, 6, "camera keys Y 2"},
        {coop_pads::kVibStopSite, &hook_vib_stop, (void*)&coop_pads::detour_vib_stop, &coop_pads::ret_vib_stop, 6, "rumble stop"},
        {0xAC205B, &hook_water, (void*)&detour_water, &jmp_ret_water, 13, "water draw per view"},
        {kFogLoad1, &hook_fog1, (void*)&detour_fog1, &jmp_ret_fog1, 5, "fog per view 1"},
        {kFogLoad2, &hook_fog2, (void*)&detour_fog2, &jmp_ret_fog2, 5, "fog per view 2"},
        {0x8DC324, &hook_unit_init, (void*)&detour_unit_init, &jmp_ret_unit_init, 9, "unit init context"},
        {0x8DC39C, &hook_unit_tick, (void*)&detour_unit_tick, &jmp_ret_unit_tick, 9, "unit tick context"},
        {0x8DC44B, &hook_unit_pass2, (void*)&detour_unit_pass2, &jmp_ret_unit_pass2, 9, "unit pass 2 context"},
        {0x8DC4C4, &hook_unit_pass3, (void*)&detour_unit_pass3, &jmp_ret_unit_pass3, 9, "unit pass 3 context"},
        {0x804A37, &hook_em_step, (void*)&detour_em_step, &jmp_ret_em_step, 5, "enemy step off players"},
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
    hook_damage_pre = std::make_unique<FunctionHook>(kDamagePreMessage, (uintptr_t)&detour_damage_pre);
    if (!hook_damage_pre->create()) {
        spdlog::error("Failed to init Coop (hit context)\n");
        return "Failed to init Coop: hit context";
    }
    damage_pre_original = hook_damage_pre->get_original();
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
    if (auto err = coop_pvp::install()) {
        spdlog::error("Coop: {}", *err);
        return err;
    }
    Macro::set_pad_update_fallback(&pad_update_fallback);
    return Mod::on_initialize();
}

void Coop::on_frame(fmilliseconds& dt) {
    coop_diag::on_frame(dt.count());
    if (tint_restore) {
        tint_restore = false;
        tint_restore_all();
    }
    coop_pvp::tick();   // also with Coop off: takes the players' lock-on targets and the em_dante rows out
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
    help_marker(_("The size of every player's HUD while the screen is split (full size while paused). Each part keeps to "
                  "its own edge of the player's view: gauges top-left, style meter and orbs on the right, centred text in "
                  "the middle. 0.5 fits a half or a quarter of the screen, 1 is the normal size. The lock-on reticle keeps "
                  "its size, as it follows the target"));

    if (ImGui::Button(_("Refresh cameras"))) {
        apply_viewport_layout();
        coop_hud::split_shown = true;
    }
    ImGui::SameLine();
    help_marker(_("Give every camera its viewport again, after a camera event took the screen back"));

    ImGui::SliderFloat(_("Revive after"), &revive_delay, 1.0f, 30.0f, "%.0f s");
    ImGui::SameLine();
    help_marker(_("A player 2-4 who dies stays down this long, then gets up by himself. Player 1's death is the game's own "
                  "(game over or a Gold Orb continue). Enemies leave a downed player alone"));
    ImGui::SliderFloat(_("Revive HP"), &revive_hp, 0.1f, 1.0f, "%.2f");
    ImGui::SameLine();
    help_marker(_("The share of his maximum HP a revived player gets up with"));

    if (ImGui::Checkbox(_("PvP hits"), &coop_pvp::hits)) {
        coop_pvp::retry();
    }
    ImGui::SameLine();
    help_marker(_("Players' attacks and shots hit the other players (never their own player), and Nero's Snatch and "
                  "Buster grab them while they're stunned (hit, launched or blown away; the generic reactions of the Dante boss fight). Enemies still hit everyone as usual. "
                  "Players can lock on to each other. Rounds come later"));
    if (ImGui::Checkbox(_("Tint players"), &coop_tint::enabled) && !coop_tint::enabled) {
        tint_restore = true;
    }
    ImGui::SameLine();
    help_marker(_("Each player's character, weapons and (Nero) Devil Trigger stand get his colour, so two of the same "
                  "character can be told apart. White = as the game draws it"));
    if (coop_tint::enabled) {
        ImGui::Indent(lineIndent);
        for (int i = 0; i < (int)player_num && i < kMaxPlayers; i++) {
            ImGui::PushID(100 + i);
            char label[32];
            std::snprintf(label, sizeof(label), "Player %d", i + 1);
            ImGui::ColorEdit3(label, coop_tint::colors[i], ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            help_marker(_("Multiplies this player's colours: white leaves them as they are, red (1, 0.6, 0.6) reddens"));
            ImGui::PopID();
        }
        ImGui::Unindent(lineIndent);
    }

    if (coop_pvp::hits) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Lock on to players first"), &coop_pvp::players_first);
        ImGui::SameLine();
        help_marker(_("While another player is in reach and in your view, lock-on picks a player and skips the enemies. "
                      "Off: players and enemies are picked alike, the nearest first"));
        ImGui::Checkbox(_("Hitting players fills Devil Trigger"), &coop_pvp::dt_from_players);
        ImGui::SameLine();
        help_marker(_("A hit on another player gives the attacker the attack's Devil Trigger, as a hit on an enemy does (not "
                      "while he is in Devil Trigger). Off: hitting players gives none, as the game alone would"));
        ImGui::Unindent(lineIndent);
    }
    if (coop_pvp::hits && ImGui::TreeNode(_("PvP flinching"))) {
        static char* rows[] = {"Flinch", "Blow away", "Flinch in DT", "Blow away in DT"};
        int* data[] = {coop_pvp::flinch[0], coop_pvp::blow[0], coop_pvp::flinch[1], coop_pvp::blow[1]};
        for (int r = 0; r < 4; r++) {
            ImGui::SetNextItemWidth(sameLineItemWidth * 1.5f);
            ImGui::InputScalarN(_(rows[r]), ImGuiDataType_S32, data[r], 5);
            for (int lv = 0; lv < 5; lv++) {
                data[r][lv] = std::clamp(data[r][lv], -1, 100);
            }
            ImGui::SameLine();
            help_marker(_("Hits from another player needed before this reaction, for attack levels 0 to 4 (each attack's "
                          "level is in its .atk entry; combo hits are mostly 2, launchers and finishers 3 and 4). -1 = "
                          "never, 1 = every hit. The count starts over after the reaction, or 3 seconds after the last hit. "
                          "The DT rows apply while the player is in Devil Trigger. Enemies' hits use the game's own values"));
        }
        if (ImGui::Button(_("Default flinching"))) {
            const int f[2][5] = {{-1, -1, 3, 2, 1}, {-1, -1, -1, 3, 2}};
            const int b[2][5] = {{-1, -1, -1, 2, 1}, {-1, -1, -1, 3, 1}};
            memcpy(coop_pvp::flinch, f, sizeof(f));
            memcpy(coop_pvp::blow, b, sizeof(b));
        }
        ImGui::SameLine();
        help_marker(_("Light combo hits need three in a row to stun, finishers still land at once; Devil Trigger takes more. "
                      "The game's own values are 1 for every attack level from 2 up"));
        ImGui::TreePop();
    }
    if (coop_pvp::hits && !coop_pvp::grabs_ready()) {
        ImGui::SameLine();
        ImGui::TextDisabled(_("(grabs off)"));
        ImGui::SameLine();
        help_marker(_("Grabbing players needs the Dante boss fight's Buster motions, read from nativePC\\rom\\enemy\\"
                      "em_dante.arc while PvP hits is on. They didn't load"));
        const char* why = coop_pvp::grabs_error();
        if (why && *why) {
            ImGui::TextDisabled("%s", why);
        }
    }

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
        coop_pvp::draw_grab_diag();
        coop_hud::draw_diag();
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
    save_carry();   // the players are still alive here
    coop_pvp::stage_end();
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
    revive_delay = std::clamp(cfg.get<float>("coop_revive_delay").value_or(5.0f), 1.0f, 30.0f);
    revive_hp = std::clamp(cfg.get<float>("coop_revive_hp").value_or(0.5f), 0.1f, 1.0f);
    coop_pvp::hits = cfg.get<bool>("coop_pvp_hits").value_or(cfg.get<bool>("FriendlyFire").value_or(false));
    coop_tint::enabled = cfg.get<bool>("coop_tint").value_or(false);
    for (int i = 0; i < kMaxPlayers; i++) {
        const std::string n = std::to_string(i + 1);
        coop_tint::colors[i][0] = std::clamp(cfg.get<float>("coop_tint_p" + n + "_r").value_or(coop_tint::colors[i][0]), 0.0f, 2.0f);
        coop_tint::colors[i][1] = std::clamp(cfg.get<float>("coop_tint_p" + n + "_g").value_or(coop_tint::colors[i][1]), 0.0f, 2.0f);
        coop_tint::colors[i][2] = std::clamp(cfg.get<float>("coop_tint_p" + n + "_b").value_or(coop_tint::colors[i][2]), 0.0f, 2.0f);
    }
    coop_pvp::players_first = cfg.get<bool>("coop_pvp_players_first").value_or(true);
    coop_pvp::dt_from_players = cfg.get<bool>("coop_pvp_dt_from_players").value_or(true);
    for (int s = 0; s < 2; s++) {
        for (int lv = 0; lv < 5; lv++) {
            const std::string k = std::to_string(s) + "_" + std::to_string(lv);
            coop_pvp::flinch[s][lv] = std::clamp(cfg.get<int>("coop_pvp_flinch" + k).value_or(coop_pvp::flinch[s][lv]), -1, 100);
            coop_pvp::blow[s][lv] = std::clamp(cfg.get<int>("coop_pvp_blow" + k).value_or(coop_pvp::blow[s][lv]), -1, 100);
        }
    }
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
    cfg.set<float>("coop_revive_delay", revive_delay);
    cfg.set<float>("coop_revive_hp", revive_hp);
    cfg.set<bool>("coop_pvp_hits", coop_pvp::hits);
    cfg.set<bool>("coop_tint", coop_tint::enabled);
    for (int i = 0; i < kMaxPlayers; i++) {
        const std::string n = std::to_string(i + 1);
        cfg.set<float>("coop_tint_p" + n + "_r", coop_tint::colors[i][0]);
        cfg.set<float>("coop_tint_p" + n + "_g", coop_tint::colors[i][1]);
        cfg.set<float>("coop_tint_p" + n + "_b", coop_tint::colors[i][2]);
    }
    cfg.set<bool>("coop_pvp_players_first", coop_pvp::players_first);
    cfg.set<bool>("coop_pvp_dt_from_players", coop_pvp::dt_from_players);
    for (int s = 0; s < 2; s++) {
        for (int lv = 0; lv < 5; lv++) {
            const std::string k = std::to_string(s) + "_" + std::to_string(lv);
            cfg.set<int>("coop_pvp_flinch" + k, coop_pvp::flinch[s][lv]);
            cfg.set<int>("coop_pvp_blow" + k, coop_pvp::blow[s][lv]);
        }
    }
}

#undef naked_extern
#pragma pop_macro("naked")
