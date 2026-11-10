#include "ChargeChecker.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cUnit.hpp"
#include <cstring>

bool ChargeChecker::roundTripCharge_enabled = false;
bool ChargeChecker::blueRoseCharge_enabled = false;
bool ChargeChecker::keepCharge_enabled = false;
uintptr_t ChargeChecker::jmp_ret = NULL;

static constexpr float round_trip_charge_mult = 2.0f;
static constexpr float blue_rose_charge_mult_1 = 2.0f;
static constexpr float blue_rose_charge_mult_2 = 2.0f;

naked void detour(void) { // player in edi 
    _asm {
        cmp byte ptr [ChargeChecker::roundTripCharge_enabled], 1
        je DanteChecks
        jmp NeroChecks

        DanteChecks:
        push eax // keep player in eax
        push edx // put compares in edx
        mov eax, [static_mediator_ptr] // only get player (thanks boss dante)
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp dword ptr [edi+0x1494], 0 // controller id (0 Dante 1 Nero)
        je GotDante
        pop edx
        pop eax
        jmp NeroChecks

        ////////// Dante //////////

        GotDante:
        // cmp dword ptr [edi+0x1DB4], 4 // rebellion
        // je RoundTrip
        jmp RoundTrip

        RoundTrip:
        lea edx, [eax+0x23E8]  // 0x23F8 == Round Trip charge
        cmp edx, ecx // ecx = affected address
        jne popcode
        mulss xmm0, [round_trip_charge_mult]
        jmp popcode

        ////////// Nero //////////

        NeroChecks:
        cmp byte ptr [ChargeChecker::blueRoseCharge_enabled], 0
        je code
        push eax // keep player in eax
        push edx // put compares in edx
        mov eax, [static_mediator_ptr] // only get player (thanks boss dante)
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp dword ptr [edi+0x1494], 1 // Nero
        je GotNero

        GotNero:
        jmp BlueRose
            
        BlueRose:
        // lea edx, [eax+0x2420] // unkn
        // lea edx, [eax+0x2434] // 0
        // lea edx, [eax+0x2448] // unkn
        // lea edx, [eax+0x245C] // unkn
        lea edx, [eax+0x2470]    // 1 to 2
        sub edx, 0x10
        cmp edx, ecx             // ecx = affected address
        je BlueRoseCharge1to2
        // lea edx, [eax+0x2484] // unkn
        lea edx, [eax+0x2498]    // 2 to max
        sub edx, 0x10
        cmp edx, ecx             // ecx = affected address
        je BlueRoseCharge2to3
        // lea edx, [eax+0x24AC] // unkn
        jmp popcode

        BlueRoseCharge1to2:
        mulss xmm0, [blue_rose_charge_mult_1]
        jmp popcode

        BlueRoseCharge2to3:
        mulss xmm0, [blue_rose_charge_mult_2]
        jmp popcode

    popcode:
        pop edx
        pop eax
    code:
        addss xmm0, [ecx+0x10]
        jmp dword ptr [ChargeChecker::jmp_ret]
    }
}

// Keep charge through doors. A charge is an input command's hold record: uPlayer::mPlCmd[85] at +0x2190, kSPlCmd 0x14
// bytes (mStep, mStatus, mActiveF, mTimerF, mCnt, mCmdFlag bit 0 = held long enough, mTame = frames held at +0x10),
// filled by uPlayer::checkCmdTame 0x805F60 while the command's button is held. Nero's Blue Rose levels are commands 33,
// 36 and 38 (+0x2424 / +0x2460 / +0x2488; updateChargeShotEffect 0x7E8180 steps the glow and sound up to the level
// their flags show), Dante's Round Trip is command 30 (+0x23E8). aRoom::setRoom 0x40D4F0 builds a new player every
// room, so the records start empty.
// Input is cut around a door: uPlayer::main 0x7A7E70 calls setInterface only while mIsInterfaceSetting (+0x1509) is
// set, but executeCommandCheck runs anyway, so a hold resets as soon as input stops, before the room is cleaned, and a
// restored one resets again while the new room's entry keeps input off. So each record is taken on the last tick input
// was live, saved at on_stage_end (aRoom::cleanRoom, before the units are freed) per player in move-line order, written
// into the new room's player every tick its input is still off, and handed over on its first live ticks: the game's
// own check then keeps it (button held) or drops it (released during the door; no release edge, so no shot).
namespace keep_charge {

constexpr uintptr_t kPlCmd       = 0x2190;   // uPlayer::mPlCmd
constexpr int       kCmdCount    = 85;
constexpr uintptr_t kCmdSize     = 0x14;     // sizeof(uPlayer::kSPlCmd)
constexpr uintptr_t kTame        = 0x10;     // kSPlCmd::mTame
constexpr uintptr_t kPlayerID    = 0x1494;   // uPlayer::mPlayerID (0 Dante, 1 Nero)
constexpr uintptr_t kInputOn     = 0x1509;   // uPlayer::mIsInterfaceSetting
constexpr uintptr_t kNeroVtable  = 0xBE4FA0; // uPlayerNero::vftable (not the tutorial / boss classes)
constexpr uintptr_t kDanteVtable = 0xBE3C50;
constexpr int       kLinePlayer  = 13;
constexpr int       kHandOverTicks = 3;      // live ticks a restored hold may read 0 (a fresh press restarts the hold)
constexpr int       kPlayerSlots = 8;

struct Hold { uint8_t rec[kCmdSize]; bool valid; };
struct Tracked { uintptr_t player = 0; uint32_t last_seen = 0; Hold holds[kCmdCount] = {}; };
struct Saved {
    uint32_t  id = 0;
    int       count = 0;
    int       cmd[kCmdCount];
    uint8_t   rec[kCmdCount][kCmdSize];
    uintptr_t player = 0;       // the new room's player it went to
    int       live_ticks = 0;
};

Tracked g_tracked[kPlayerSlots];
Saved   g_saved[kPlayerSlots];
int     g_saved_players = 0;

Tracked& tracked_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (Tracked& t : g_tracked) if (t.player == player) { t.last_seen = tick; return t; }
    Tracked* victim = &g_tracked[0];
    for (Tracked& t : g_tracked) if (t.last_seen < victim->last_seen) victim = &t;
    *victim = Tracked{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

uint8_t* record(uintptr_t player, int k) { return reinterpret_cast<uint8_t*>(player + kPlCmd + k * kCmdSize); }
float& tame(uint8_t* rec) { return *reinterpret_cast<float*>(rec + kTame); }

bool is_player_class(uintptr_t unit) {
    const uintptr_t vt = *reinterpret_cast<uintptr_t*>(unit);
    return vt == kNeroVtable || vt == kDanteVtable;
}

// Index of `player` among the Nero / Dante units on the player line (player 1 first, then co-op partners), -1 if absent.
int line_order(uintptr_t player) {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return -1;
    int i = 0, n = 0;
    for (cUnit* u = (cUnit*)su->mMoveLine[kLinePlayer].mTop; u && n < 8192; u = u->mp_next_unit, ++n) {
        if (!is_player_class((uintptr_t)u)) continue;
        if ((uintptr_t)u == player) return i;
        ++i;
    }
    return -1;
}

// The holds saved for this new player: bound on its first tick by its place on the line, same character only.
Saved* saved_for(uintptr_t player) {
    for (int i = 0; i < g_saved_players; ++i) {
        if (g_saved[i].player == player) return &g_saved[i];
    }
    const int order = line_order(player);
    if (order < 0 || order >= g_saved_players || g_saved[order].player) return nullptr;
    Saved& s = g_saved[order];
    s.player = player;
    if (s.id != *reinterpret_cast<uint32_t*>(player + kPlayerID)) s.count = 0;
    return &s;
}

void drop(Saved& s, int i) {
    --s.count;
    s.cmd[i] = s.cmd[s.count];
    std::memcpy(s.rec[i], s.rec[s.count], kCmdSize);
}

void restore(uintptr_t player, bool live) {
    if (g_saved_players == 0) return;
    Saved* s = saved_for(player);
    if (!s || s->count == 0) return;
    if (!live) {
        for (int i = 0; i < s->count; ++i) std::memcpy(record(player, s->cmd[i]), s->rec[i], kCmdSize);
        return;
    }
    ++s->live_ticks;
    for (int i = s->count - 1; i >= 0; --i) {
        uint8_t* rec = record(player, s->cmd[i]);
        const float now = tame(rec), saved = tame(s->rec[i]);
        if (now >= saved) {
            drop(*s, i);                                  // the game carried on from the restored hold
        } else if (now > 0.0f) {
            std::memcpy(rec, s->rec[i], kCmdSize);        // the hold restarted on a fresh press: add what it had
            tame(rec) = saved + now;
            drop(*s, i);
        } else if (s->live_ticks <= kHandOverTicks) {
            std::memcpy(rec, s->rec[i], kCmdSize);
        } else {
            drop(*s, i);                                  // released during the door
        }
    }
}

void tick(uintptr_t player) {
    if (!ChargeChecker::keepCharge_enabled || !player || !is_player_class(player)) return;
    Tracked& t = tracked_for(player);
    const bool live = *reinterpret_cast<uint8_t*>(player + kInputOn) != 0;
    restore(player, live);
    if (!live) return;                                    // keep what the records held while input was live
    for (int k = 0; k < kCmdCount; ++k) {
        uint8_t* rec = record(player, k);
        Hold& h = t.holds[k];
        h.valid = tame(rec) > 0.0f;
        if (h.valid) std::memcpy(h.rec, rec, kCmdSize);
    }
}

// aRoom::cleanRoom, before the units are freed: the players' holds as input last saw them, in move-line order.
void snapshot() {
    g_saved_players = 0;
    if (!ChargeChecker::keepCharge_enabled) return;
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return;
    int n = 0;
    for (cUnit* u = (cUnit*)su->mMoveLine[kLinePlayer].mTop; u && n < 8192 && g_saved_players < kPlayerSlots; u = u->mp_next_unit, ++n) {
        const uintptr_t pl = (uintptr_t)u;
        if (!is_player_class(pl)) continue;
        Saved& s = g_saved[g_saved_players++];
        s.id = *reinterpret_cast<uint32_t*>(pl + kPlayerID);
        s.count = 0;
        s.player = 0;
        s.live_ticks = 0;
        for (const Tracked& t : g_tracked) {
            if (t.player != pl) continue;
            for (int k = 0; k < kCmdCount; ++k) {
                if (!t.holds[k].valid) continue;
                s.cmd[s.count] = k;
                std::memcpy(s.rec[s.count], t.holds[k].rec, kCmdSize);
                ++s.count;
            }
        }
    }
    for (Tracked& t : g_tracked) t = Tracked{};   // the units are about to be freed
}

} // namespace keep_charge

std::optional<std::string> ChargeChecker::on_initialize() {
    if (!install_hook_offset(0x0405F97, hook, &detour, &jmp_ret, 5)) {
        spdlog::error("Failed to init ChargeChecker mod\n");
        return "Failed to init ChargeChecker mod";
    }
    input_commands::add_tick(&keep_charge::tick);
    if (!input_commands::hook_installed) {     // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("ChargeChecker: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init ChargeChecker (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void ChargeChecker::on_stage_end() {
    keep_charge::snapshot();
}

void ChargeChecker::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Fast Round Trip Charge"), &roundTripCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Half the charge time on Round Trip"));
        ImGui::Checkbox(_("Keep Charge Through Doors##Dante"), &keepCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Keep holding the button through a door or room change and the charge carries over (Round "
                      "Trip, gun charge shots). Let go during the change and it's lost, as usual"));
    }
    if (display == DISPLAY_NERO_A) {
        ImGui::Checkbox(_("Fast Blue Rose Charge"), &blueRoseCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Half the charge time on Blue Rose"));
        ImGui::Checkbox(_("Keep Charge Through Doors##Nero"), &keepCharge_enabled);
        ImGui::SameLine();
        help_marker(_("Keep holding the button through a door or room change and the Blue Rose charge level carries "
                      "over. Let go during the change and it's lost, as usual"));
    }
}

void ChargeChecker::on_config_load(const utility::Config& cfg) {
    roundTripCharge_enabled = cfg.get<bool>("faster_roundtrip").value_or(false);
    // round_trip_charge_mult = cfg.get<float>("roundtrip_charge_mult_1").value_or(2.0f);
    blueRoseCharge_enabled = cfg.get<bool>("faster_bluerose").value_or(false);
    keepCharge_enabled = cfg.get<bool>("keep_charge_through_doors").value_or(false);
    // blue_rose_charge_mult_1 = cfg.get<float>("blue_rose_charge_mult_1").value_or(2.0f);
    // blue_rose_charge_mult_2 = cfg.get<float>("blue_rose_charge_mult_2").value_or(2.0f);
};

void ChargeChecker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("faster_roundtrip", roundTripCharge_enabled);
    // cfg.set<float>("roundtrip_charge_mult_1", round_trip_charge_mult);
    cfg.set<bool>("faster_bluerose", blueRoseCharge_enabled);
    cfg.set<bool>("keep_charge_through_doors", keepCharge_enabled);
    // cfg.set<float>("blue_rose_charge_mult_1", blue_rose_charge_mult_1);
    // cfg.set<float>("blue_rose_charge_mult_2", blue_rose_charge_mult_2);
};
