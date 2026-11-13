#include "EnemyAI.hpp"
#include "EnemyAIThinkTrace.hpp"
#include "../sdk/Custom.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/MtDTI.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/RimGlow.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace provoke {

bool  mod_enabled         = false;
float radius              = 1500.0f;
float duration            = 8.0f;
float speed               = 1.25f;
float dt_rate             = 2.0f;
bool  dt_any_difficulty   = false;
float think_rate          = 1.5f;
bool  counted_only        = false;
bool  targeted            = true;
bool  targeted_and_radius = false;
bool  aura                = true;
float aura_color[3]       = { 1.0f, 0.25f, 0.1f };
float aura_brightness     = 1.0f;
float aura_reach          = 0.3f;
float aura_scale          = 1.03f;

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

// ---- Game addresses ------------------------------------------------------------------------------------------------
constexpr uintptr_t kTauntCall      = 0x803E67;   // uPlayer::checkProvoke: call uPlayer::setProvoke; mov al, 1
constexpr uintptr_t kSetProvoke     = 0x7AE9F0;   // esi = the player
constexpr uintptr_t kDeltaWrite     = 0x8DC314;   // sUnit::move: movss [esi+10h], xmm0 (esi = the unit)
constexpr uintptr_t kNextThinkTimer = 0x45BEA0;   // cThinkMgr::NextThinkTimer: ecx = the manager, xmm2 = the rate
constexpr uintptr_t kThinkFirstOp   = 0xC0ECF4;   // its first instruction: movss xmm1, [0C0ECF4h]
constexpr uintptr_t kEnemyDti       = 0xE596C8;   // uEnemy::DTI
constexpr uintptr_t kMediator       = 0xE558B8;   // sMediator::mpInstance
constexpr uintptr_t kArea           = 0xE552C8;   // sArea::mpInstance

// uPlayer
constexpr uintptr_t kPos          = 0x30;     // uCoord::mPos
constexpr uintptr_t kTauntCounts  = 0x1F38;   // set by setProvoke: an enemy is near and the style guard allows it
constexpr uintptr_t kLockOn       = 0x3078;   // mLockOn: locked
constexpr uintptr_t kLockTarget   = 0x3080;   // mLockOn.pTarget (cLockOnTarget: +0x08 mpModel)
// uEnemy
constexpr uintptr_t kDtTimer      = 0x1488;   // mDevilTriggerTimer
constexpr uintptr_t kDtOk         = 0x148C;   // mDevilTriggerOK
constexpr uintptr_t kDtTimerOn    = 0x148D;   // mDevilTriggerTimerEnable
constexpr uintptr_t kDtOn         = 0x148E;   // mDevilTrigger
constexpr uintptr_t kDtSign       = 0x148F;   // mDevilTriggerSign
constexpr uintptr_t kSetDtSign    = 0x13C;    // vtable: setDevilTriggerSign
constexpr float     kDtSignAt     = 900.0f;
// sMediator
constexpr uintptr_t kDifficulty   = 0x140;    // 3 = Dante Must Die
constexpr uintptr_t kMission      = 0xE0;     // 50 = Bloody Palace
constexpr uintptr_t kSecret       = 0x126;    // a secret mission
constexpr uintptr_t kAreaBp       = 0x3830;   // sArea: Bloody Palace state (+0x74 floor)
// uEnemy::move's DT code, which other mods patch (read live, so Provoke follows them):
//   0x73099A cmp dword ptr [mediator+140h], 3 / 0x7309A1 jz: the difficulty gate. DifficultySelect rewrites the
//   immediate and the jump (0x7309A0: 03 74 / 05 75), LdkWithDmd makes it jge (0x7309A1: 7D, LDK counts too),
//   EnemyDT's unused "any difficulty" patch a jmp (EB).
//   0x7309DD jz (skip the count while the timer is off): EnemyDT's "Enemies Don't DT" makes it jnz (75), so it never counts.
constexpr uintptr_t kDtGateImm    = 0x7309A0;
constexpr uintptr_t kDtGateJump   = 0x7309A1;
constexpr uintptr_t kDtCountJump  = 0x7309DD;

constexpr int kAuraLine = 1;                  // the unit line the auras run on (AfterImage's copies' line)

// ---- Provoked enemies ----------------------------------------------------------------------------------------------
// Written on the game thread only (the taunt and the unit delta); seen is the render thread's frame count when the enemy
// last ticked, so an entry whose enemy is gone (killed, the room left) is reused after a while.
struct Entry {
    uintptr_t enemy = 0;
    uintptr_t by = 0;        // the player who provoked it
    uintptr_t size = 0;      // its DTI size (which cThinkMgr is its own)
    float left = 0.0f;       // frames, unscaled
    uint32_t seen = 0;
};
constexpr int kMaxEntries = 64;
constexpr uint32_t kStaleFrames = 120;
Entry g_list[kMaxEntries];
bool g_any = false;                  // any entry live (the hooks' fast way out)
volatile uint32_t g_frame = 0;       // render frames
uint32_t g_taunts = 0, g_provoked = 0, g_auras_made = 0;   // readout

uintptr_t dti_of(uintptr_t unit) {
    return (uintptr_t)((MtDTI*(__fastcall*)(uintptr_t, int))(at<uintptr_t>(at<uintptr_t>(unit, 0), 0x10)))(unit, 0);
}

bool is_enemy(uintptr_t unit) {
    if (!unit || !at<uintptr_t>(unit, 0)) return false;
    const uint32_t state = at<uint32_t>(unit, 0x04) & 7;
    if (state != 1 && state != 2) return false;
    for (const MtDTI* d = (const MtDTI*)dti_of(unit); d; d = d->mp_parent)
        if ((uintptr_t)d == kEnemyDti) return true;
    return false;
}

Entry* find(uintptr_t enemy) {
    for (Entry& e : g_list)
        if (e.enemy == enemy && e.left > 0.0f) return &e;
    return nullptr;
}

void update_any() {
    g_any = false;
    for (const Entry& e : g_list)
        if (e.enemy && e.left > 0.0f) { g_any = true; return; }
}

void provoke_one(uintptr_t enemy, uintptr_t by) {
    Entry* e = find(enemy);
    if (!e) {
        for (Entry& c : g_list) {
            if (!c.enemy || c.left <= 0.0f || g_frame - c.seen > kStaleFrames) { e = &c; break; }
        }
        if (!e) return;
        *e = Entry{};
        e->enemy = enemy;
        e->size = at<uintptr_t>(dti_of(enemy), 0x18);   // MtDTI::m_size
        ++g_provoked;
    }
    e->by = by;
    e->left = std::max(duration, 0.0f) * 60.0f;
    e->seen = g_frame;
    g_any = true;
}

float dist2(uintptr_t a, uintptr_t b) {
    float d = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float v = at<float>(a, kPos + 4 * k) - at<float>(b, kPos + 4 * k);
        d += v * v;
    }
    return d;
}

// After setProvoke (the taunt has started).
void __stdcall on_taunt(uintptr_t player) {
    if (!mod_enabled || !player) return;
    if (counted_only && !at<uint8_t>(player, kTauntCounts)) return;
    ++g_taunts;
    if (targeted && at<uint8_t>(player, kLockOn)) {
        const uintptr_t t = at<uintptr_t>(player, kLockTarget);
        const uintptr_t model = t ? at<uintptr_t>(t, 0x08) : 0;
        if (is_enemy(model)) {
            provoke_one(model, player);
            if (!targeted_and_radius) return;
        }
    }
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) return;
    const float r2 = radius * radius;
    for (int line = 0; line < 32; ++line) {
        int n = 0;
        for (cUnit* c = su->mMoveLine[line].mTop; c && n < 4096; c = c->mp_next_unit, ++n) {
            const uintptr_t u = (uintptr_t)c;
            if (is_enemy(u) && dist2(u, player) <= r2) provoke_one(u, player);
        }
    }
}

// Enemy DT is switched off (EnemyDT's "Enemies Don't DT"): leave the timer alone.
bool dt_disabled() {
    return at<uint8_t>(kDtCountJump, 0) == 0x75;
}

// The game's own count of the Devil Trigger timer runs for this enemy now (uEnemy::move 0x730990), with the gate as
// the difficulty mods left it.
bool game_counts_dt() {
    const uintptr_t med = at<uintptr_t>(kMediator, 0);
    if (!med || at<uint8_t>(med, kSecret)) return false;
    const int difficulty = at<int>(med, kDifficulty);
    const int imm = at<uint8_t>(kDtGateImm, 0);
    const uint8_t op = at<uint8_t>(kDtGateJump, 0);
    const bool gate = op == 0xEB || (op == 0x74 && difficulty == imm) || (op == 0x75 && difficulty != imm) ||
                      (op == 0x7D && difficulty >= imm);
    if (gate) return true;
    if (at<int>(med, kMission) != 50) return false;
    const uintptr_t area = at<uintptr_t>(kArea, 0);
    const uintptr_t bp = area ? at<uintptr_t>(area, kAreaBp) : 0;
    return bp && at<uint32_t>(bp, 0x74) > 0x50;
}

// Before the enemy's move, which takes `scaled` (its delta, sped up) off the timer when the game counts it.
void quicken_dt(uintptr_t e, float scaled) {
    if (dt_disabled() || at<uint8_t>(e, kDtOn) || !at<uint8_t>(e, kDtTimerOn)) return;
    float& timer = at<float>(e, kDtTimer);
    if (timer < 0.0f) return;
    if (game_counts_dt()) {
        if (dt_rate > 1.0f) timer = std::max(0.0f, timer - scaled * (dt_rate - 1.0f));   // 0: the game's next count sets OK
        return;
    }
    if (!dt_any_difficulty) return;
    timer -= scaled * std::max(dt_rate, 1.0f);
    if (timer <= 0.0f) {
        timer = 0.0f;
        at<uint8_t>(e, kDtOk) = 1;
    } else if (timer <= kDtSignAt && !at<uint8_t>(e, kDtSign)) {
        reinterpret_cast<void(__thiscall*)(uintptr_t)>(at<uintptr_t>(at<uintptr_t>(e, 0), kSetDtSign))(e);
    }
}

void ensure_aura(uintptr_t enemy);
void release_aura(uintptr_t enemy);

// sUnit::move, every unit, before its move: the factor its delta is multiplied by.
float __stdcall unit_rate(uintptr_t unit, float raw) {
    Entry* e = find(unit);
    if (!e) return 1.0f;
    e->seen = g_frame;
    e->left -= raw;
    if (!mod_enabled || e->left <= 0.0f) {
        release_aura(unit);
        *e = Entry{};
        update_any();
        return 1.0f;
    }
    const float s = std::clamp(speed, 0.1f, 4.0f);
    quicken_dt(unit, raw * s);
    if (aura && at<uint8_t>(unit, kDtOn)) ensure_aura(unit);
    return s;
}

// cThinkMgr::NextThinkTimer: the factor its rate is multiplied by.
float __stdcall think_scale(uintptr_t mgr) {
    if (think_rate == 1.0f) return 1.0f;
    for (const Entry& e : g_list) {
        if (!e.enemy || e.left <= 0.0f || mgr < e.enemy || mgr >= e.enemy + e.size) continue;
        return at<uint8_t>(e.enemy, kDtOn) ? std::clamp(think_rate, 0.1f, 10.0f) : 1.0f;
    }
    return 1.0f;
}

// ---- Aura ----------------------------------------------------------------------------------------------------------
rim_glow::Cube g_cube;
bool glow_ready() { return rim_glow::base_ready() && g_cube.ready(); }

struct Aura;
constexpr int kAuraMax = 16;
Aura* g_auras[kAuraMax] = {};

// A copy of one provoked enemy's model in DT, posed from its joints at every draw. Its memory is its own to free (the
// game deletes a unit without running C++ destructors).
struct Aura : CustomUnit<Aura> {
    int index = 0;
    uintptr_t enemy = 0;        // whose model it copies, 0 = free
    uintptr_t built = 0;        // the rModel it was set up with
    rim_glow::Saved* saved = nullptr;
    uint32_t saved_num = 0, saved_cap = 0;

    explicit Aura(int i) : index(i) {
        mActorType = 5;
        mWorkRate.mType = 9;
        mTransparency = 0.0f;
    }

    void restore_materials() {
        for (uint32_t i = 0; i < saved_num; ++i) rim_glow::restore(saved[i]);
        saved_num = 0;
    }

    void release() {
        enemy = 0;
        mTransparency = 0.0f;
    }

    // Set up as the enemy's model (again when its rModel changed: a DT model swap).
    bool build(uDevil4Model* src) {
        const uintptr_t rmodel = src->mpRModel;
        if (!rmodel) return false;
        if (built == rmodel) return true;
        restore_materials();
        auto* m = (uDevil4Model*)this;
        custom_utils::get_model(m, (void*)rmodel);
        built = rmodel;
        ++g_auras_made;
        m->mCameraTransparencyEnable = false;
        m->Render.mVFCullLevel = 0;
        m->Render.mRenderMode = 0;
        m->mShadowCastGroup = 0;
        m->mShadowRecvGroup = 0;
        const uintptr_t* mats = (const uintptr_t*)m->mpMaterial;
        const uint32_t num = mats ? m->MaterialNum : 0;
        if (num > saved_cap) {
            auto* p = (rim_glow::Saved*)std::realloc(saved, num * sizeof(rim_glow::Saved));
            if (!p) return false;
            saved = p;
            saved_cap = num;
        }
        for (uint32_t i = 0; i < num; ++i) {
            const uintptr_t mat = mats[i];
            if (!mat || !rim_glow::is_standard(mat)) continue;
            saved[saved_num] = rim_glow::save(mat);
            rim_glow::put_look(saved[saved_num++], true, &g_cube, false, false);
        }
        return true;
    }

    void tick() {
        if (!enemy) return;
        Entry* e = find(enemy);
        auto* src = (uDevil4Model*)enemy;
        if (!e || !mod_enabled || !aura || !glow_ready() || !at<uint8_t>(enemy, kDtOn) || !is_enemy(enemy) ||
            src->mTransparency <= 0.0f || !build(src)) {
            release();
            return;
        }
        auto* m = (uDevil4Model*)this;
        m->mTransparency = 1.0f;     // the glow keeps mAlpha at 1; the strength is the blend factor
        m->mLightGroup = src->mLightGroup;
        for (uint32_t i = 0; i < saved_num; ++i)
            rim_glow::set_strength(saved[i].mat, true, 1.0f, aura_brightness, aura_reach);
    }

    void render(void* trans) {
        const uintptr_t src_u = enemy;
        if (!src_u || mTransparency <= 0.0f) return;
        const uintptr_t context = at<uintptr_t>((uintptr_t)trans, 0x2A90);
        if (!context || !at<uint8_t>(context, 0x1FC)) return;
        auto* m = (uDevil4Model*)this;
        auto* src = (uDevil4Model*)src_u;
        if (!m->mpRModel || !m->flags.bits.mTransMode || m->mpRModel != src->mpRModel) return;
        // Each joint's matrix scaled about its own origin (MT matrices are row vectors: rows 1-3 the axes, row 4 the
        // position), so every part grows around its own bone and the copy wraps the enemy instead of z-fighting with it.
        const float s = std::clamp(aura_scale, 1.0f, 1.5f);
        auto grow = [s](MtMatrix& dst, const MtMatrix& src) {
            dst = src;
            for (Vector4f* r : { &dst.m1, &dst.m2, &dst.m3 }) {
                r->x *= s;
                r->y *= s;
                r->z *= s;
            }
        };
        grow(m->mWmat, src->mWmat);
        const uint32_t n = std::min(src->mJointNum, m->mJointNum);
        for (uint32_t i = 0; i < n; ++i) grow(m->mpJoint[i].mWmat, src->mpJoint[i].mWmat);
        uactor_sdk::render_call(m, trans);
    }

    void destructor(uint32_t flag) {
        restore_materials();
        std::free(saved);
        saved = nullptr;
        if (index >= 0 && index < kAuraMax && g_auras[index] == this) g_auras[index] = nullptr;
        CustomActor::destructor(flag);
    }
};

void ensure_aura(uintptr_t enemy) {
    if (!glow_ready()) return;
    Aura* pick = nullptr;
    for (Aura* a : g_auras)
        if (a && a->enemy == enemy) return;
    for (Aura* a : g_auras)
        if (a && !a->enemy) { pick = a; break; }
    for (int i = 0; i < kAuraMax && !pick; ++i)
        if (!g_auras[i]) pick = g_auras[i] = Aura::spawn(kAuraLine, i);
    if (pick) pick->enemy = enemy;
}

void release_aura(uintptr_t enemy) {
    for (Aura* a : g_auras)
        if (a && a->enemy == enemy) a->release();
}

// ---- Hooks ---------------------------------------------------------------------------------------------------------
uintptr_t fn_set_provoke = kSetProvoke;
uintptr_t ret_taunt = kTauntCall + 7;
uintptr_t ret_delta = kDeltaWrite + 5;
uintptr_t ret_think = kNextThinkTimer + 8;

// esi = the player
naked void detour_taunt() {
    __asm {
        call dword ptr [fn_set_provoke]
        pushad
        push esi
        call on_taunt
        popad
        mov al, 1
        jmp dword ptr [ret_taunt]
    }
}

// esi = the unit, xmm0 = its delta; the flags of sUnit::move's `cmp ecx, 1` are read after the write (jnz 0x8DC36E)
naked void detour_delta() {
    __asm {
        pushfd
        cmp byte ptr [g_any], 0
        je plain
        sub esp, 0x84
        movdqu [esp+0x00], xmm0
        movdqu [esp+0x10], xmm1
        movdqu [esp+0x20], xmm2
        movdqu [esp+0x30], xmm3
        movdqu [esp+0x40], xmm4
        movdqu [esp+0x50], xmm5
        movdqu [esp+0x60], xmm6
        movdqu [esp+0x70], xmm7
        pushad
        sub esp, 4
        movss dword ptr [esp], xmm0
        push esi
        call unit_rate
        fstp dword ptr [esp+0x20+0x80]
        popad
        movdqu xmm0, [esp+0x00]
        movdqu xmm1, [esp+0x10]
        movdqu xmm2, [esp+0x20]
        movdqu xmm3, [esp+0x30]
        movdqu xmm4, [esp+0x40]
        movdqu xmm5, [esp+0x50]
        movdqu xmm6, [esp+0x60]
        movdqu xmm7, [esp+0x70]
        mulss xmm0, dword ptr [esp+0x80]
        add esp, 0x84
    plain:
        popfd
        movss dword ptr [esi+0x10], xmm0
        jmp dword ptr [ret_delta]
    }
}

// ecx = the manager, xmm2 = the rate (function entry)
naked void detour_think() {
    __asm {
        cmp byte ptr [g_any], 0
        je original
        sub esp, 0x84
        movdqu [esp+0x00], xmm0
        movdqu [esp+0x10], xmm1
        movdqu [esp+0x20], xmm2
        movdqu [esp+0x30], xmm3
        movdqu [esp+0x40], xmm4
        movdqu [esp+0x50], xmm5
        movdqu [esp+0x60], xmm6
        movdqu [esp+0x70], xmm7
        pushad
        push ecx
        call think_scale
        fstp dword ptr [esp+0x20+0x80]
        popad
        movdqu xmm0, [esp+0x00]
        movdqu xmm1, [esp+0x10]
        movdqu xmm2, [esp+0x20]
        movdqu xmm3, [esp+0x30]
        movdqu xmm4, [esp+0x40]
        movdqu xmm5, [esp+0x50]
        movdqu xmm6, [esp+0x60]
        movdqu xmm7, [esp+0x70]
        mulss xmm2, dword ptr [esp+0x80]
        add esp, 0x84
    original:
        movss xmm1, dword ptr ds:[0x00C0ECF4]   // kThinkFirstOp
        jmp dword ptr [ret_think]
    }
}

} // namespace

uintptr_t provoker(uintptr_t enemy) {
    if (!g_any || !enemy) return 0;
    const Entry* e = find(enemy);
    return e ? e->by : 0;
}

} // namespace provoke


namespace anti_air {

bool  enabled       = false;
float window_factor = 0.6f;
bool  win_roll      = true;
bool  skip_cooldown = true;

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

constexpr uintptr_t kThinkCheck    = 0x45C5E0;   // cThinkMgr::thinkCheck (stdcall: the manager, the row by value)
constexpr uintptr_t kGetActionNo   = 0x45C240;   // cThinkMgr::GetActionNo (stdcall: the manager, motion state, distance,
                                                 // angle, height, level)
constexpr float     kAboveMin      = 100.0f;    // a row whose lower height bound is this or more needs the target above
constexpr float     kUnbounded     = 1.0e30f;   // the tables write FLT_MAX for "no bound"
// cThinkMgr (DX9)
constexpr uintptr_t kHeight        = 0x54;      // mfHeight
constexpr uintptr_t kRndStart      = 0x60;      // mbRndStart
constexpr uintptr_t kRndProb       = 0x64;      // mfRndProb (the block's roll, 0-100)
constexpr uintptr_t kRndSubProb    = 0x68;      // mfRndSubProb (the shares used up so far in the block)
// kTHINKDATA
constexpr uintptr_t kRowFlagLo     = 0x00;
constexpr uintptr_t kRowFlagHi     = 0x04;
constexpr uintptr_t kRowHeightA    = 0x18;
constexpr uintptr_t kRowHeightB    = 0x1C;
constexpr uintptr_t kRowShare      = 0x50;      // fsys_param[4]
constexpr uint32_t  kRndBlockStart = 0x400000;
constexpr uint32_t  kNoTimerGates  = 0x20000000;   // high dword: skip the cooldown and pick-count gates

uintptr_t think_check_original = 0, get_action_no_original = 0;
uint32_t g_adjusted = 0;   // readout

void __stdcall adjust_row(uintptr_t mgr, uintptr_t row) {
    if (!enabled || !mgr || !row) return;
    float& low = at<float>(row, kRowHeightA);
    const float high = at<float>(row, kRowHeightB);
    if (!(low < kUnbounded) || low < kAboveMin) return;              // not an anti-air row
    if (high < kUnbounded && high < low) return;                       // an "outside this band" row
    const float height = at<float>(mgr, kHeight);
    const float wide = low * std::clamp(window_factor, 0.1f, 1.0f);
    if (height < wide || (high < kUnbounded && height > high)) return;  // the target isn't in its (widened) window
    low = wide;
    if (skip_cooldown) at<uint32_t>(row, kRowFlagHi) |= kNoTimerGates;
    if (win_roll) {
        if ((at<uint32_t>(row, kRowFlagLo) & kRndBlockStart) && !at<uint8_t>(mgr, kRndStart)) {
            at<uint8_t>(mgr, kRndStart) = 1;      // start the block ourselves, the roll on this row
            at<float>(mgr, kRndSubProb) = 0.0f;
        }
        if (at<uint8_t>(mgr, kRndStart)) {
            float& share = at<float>(row, kRowShare);
            if (share <= 0.0f) share = 1.0f;
            at<float>(mgr, kRndProb) = at<float>(mgr, kRndSubProb);
        }
    }
    ++g_adjusted;
}

// The two cThinkMgr hooks, shared with the think trace (Debug tab). Both are plain stdcall functions whose callers
// only rely on the callee-saved registers and eax, so they are wrapped in C++: the row is a 0x68-byte argument by value.
struct ThinkRow { uint8_t b[0x68]; };
using ThinkCheckFn  = int(__stdcall*)(uintptr_t mgr, ThinkRow row);
using GetActionNoFn = int(__stdcall*)(uintptr_t mgr, uint32_t mot, float leng, float deg, float height, int lv);

int __stdcall think_check(uintptr_t mgr, ThinkRow row) {
    const bool traced = think_trace::recording(mgr);
    think_trace::Pre pre;
    if (traced) pre = think_trace::before(mgr, &row);     // before anti-air changes the copy: its place in the table
    adjust_row(mgr, (uintptr_t)&row);
    const int result = reinterpret_cast<ThinkCheckFn>(think_check_original)(mgr, row);
    if (traced) think_trace::after(mgr, &row, pre, result);
    return result;
}

int __stdcall get_action_no(uintptr_t mgr, uint32_t mot, float leng, float deg, float height, int lv) {
    const bool traced = think_trace::begin(mgr, mot, leng, deg, height, lv);
    const int result = reinterpret_cast<GetActionNoFn>(get_action_no_original)(mgr, mot, leng, deg, height, lv);
    if (traced) think_trace::end(result);
    return result;
}

} // namespace

} // namespace anti_air

// ---- The mod -------------------------------------------------------------------------------------------------------
using namespace provoke;

std::optional<std::string> EnemyAI::on_initialize() {
    if (!install_hook_absolute(kTauntCall, hook_taunt, &detour_taunt, nullptr, 7)) {
        spdlog::error("Provoke: failed to hook uPlayer::checkProvoke (0x803E67)");
        return "Failed to init Provoke (taunt)";
    }
    if (!install_hook_absolute(kDeltaWrite, hook_delta, &detour_delta, nullptr, 5)) {
        spdlog::error("Provoke: failed to hook sUnit::move (0x8DC314)");
        return "Failed to init Provoke (unit speed)";
    }
    if (!install_hook_absolute(kNextThinkTimer, hook_think, &detour_think, nullptr, 8)) {
        spdlog::error("Provoke: failed to hook cThinkMgr::NextThinkTimer (0x45BEA0)");
        return "Failed to init Provoke (think cooldowns)";
    }
    if (!install_hook_absolute(anti_air::kThinkCheck, hook_think_check, (void*)&anti_air::think_check,
                               &anti_air::think_check_original)) {
        spdlog::error("EnemyAI: failed to hook cThinkMgr::thinkCheck (0x45C5E0)");
        return "Failed to init EnemyAI (anti-air)";
    }
    if (!install_hook_absolute(anti_air::kGetActionNo, hook_get_action_no, (void*)&anti_air::get_action_no,
                               &anti_air::get_action_no_original)) {
        spdlog::error("EnemyAI: failed to hook cThinkMgr::GetActionNo (0x45C240)");
        return "Failed to init EnemyAI (think trace)";
    }
    return Mod::on_initialize();
}

void EnemyAI::on_frame(fmilliseconds& dt) {
    g_frame = g_frame + 1;
    if (rim_glow::update_base()) g_cube.update(aura_color);
    think_trace::draw_window();
}

void EnemyAI::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_B) {     // the Debug tab: the think trace's switch (its window is drawn in on_frame)
        think_trace::draw_toggle();
        return;
    }
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("EnemyAI");
    ImGui::SeparatorText(_("Provoke"));
    ImGui::Checkbox(_("Provoke"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Taunting provokes the enemies near you: they move and attack faster and reach their Devil Trigger "
                  "sooner. In Devil Trigger they glow at their edges and choose their next attack sooner. A new taunt "
                  "starts the time again."));
    if (mod_enabled) {
        ImGui::Indent();
        ImGui::SliderFloat(_("Radius"), &radius, 100.0f, 5000.0f, "%.0f cm");
        ImGui::SameLine();
        help_marker(_("How far from you an enemy can be and still be provoked (100 cm = 1 m)."));
        ImGui::SliderFloat(_("Duration"), &duration, 1.0f, 30.0f, "%.1f s");
        ImGui::SameLine();
        help_marker(_("How long an enemy stays provoked. Taunting again while it is near starts it again; it doesn't add up."));
        ImGui::SliderFloat(_("Speed"), &speed, 1.0f, 2.0f, "x%.2f");
        ImGui::SameLine();
        help_marker(_("How much faster a provoked enemy runs: its animations, movement and timers."));
        ImGui::SliderFloat(_("Devil Trigger timer"), &dt_rate, 1.0f, 5.0f, "x%.2f");
        ImGui::SameLine();
        help_marker(_("How much faster a provoked enemy's Devil Trigger timer runs, on top of Speed (x2 with Speed x1.25: "
                      "it reaches Devil Trigger in 40% of the usual time). Enemies only Devil Trigger on Dante Must Die "
                      "and in Bloody Palace above floor 80, unless the next option is on."));
        ImGui::Checkbox(_("Devil Trigger on every difficulty"), &dt_any_difficulty);
        ImGui::SameLine();
        help_marker(_("Provoked enemies count down to Devil Trigger on every difficulty, not only where the game does it."));
        ImGui::SliderFloat(_("Attacks in Devil Trigger"), &think_rate, 1.0f, 4.0f, "x%.2f");
        ImGui::SameLine();
        help_marker(_("In Devil Trigger, how much sooner a provoked enemy may repeat its moves (its decision cooldowns run "
                      "this much faster, on top of Speed)."));
        ImGui::Checkbox(_("Locked-on taunt"), &targeted);
        ImGui::SameLine();
        help_marker(_("Taunting while locked on to an enemy provokes that enemy alone, however far away it is. It stays "
                      "provoked if you let go of the lock."));
        if (targeted) {
            ImGui::SameLine();
            ImGui::Checkbox(_("And the radius"), &targeted_and_radius);
            ImGui::SameLine();
            help_marker(_("A locked-on taunt also provokes the enemies near you."));
        }
        ImGui::Checkbox(_("Only taunts that count"), &counted_only);
        ImGui::SameLine();
        help_marker(_("Only taunts the style meter counts provoke (an enemy near you, and not the same taunt repeated)."));
        ImGui::Checkbox(_("Aura in Devil Trigger"), &aura);
        ImGui::SameLine();
        help_marker(_("Provoked enemies in Devil Trigger glow at their edges."));
        if (aura) {
            ImGui::ColorEdit3(_("Aura colour"), aura_color, ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();
            help_marker(_("The colour of the glow."));
            ImGui::SliderFloat(_("Aura brightness"), &aura_brightness, 0.0f, 4.0f, "%.2f");
            ImGui::SameLine();
            help_marker(_("How bright the glow is."));
            ImGui::SliderFloat(_("Aura width"), &aura_reach, 0.02f, 1.0f, "%.2f");
            ImGui::SameLine();
            help_marker(_("How far in from the outline the glow reaches."));
            ImGui::SliderFloat(_("Aura size"), &aura_scale, 1.0f, 1.15f, "x%.3f");
            ImGui::SameLine();
            help_marker(_("How much bigger than the enemy the glowing copy is: each body part grows around its own bone. "
                          "Too close to x1 and the glow flickers against the enemy's own surface; too big and it floats "
                          "off thin parts."));
        }
        int live = 0, auras = 0;
        for (const Entry& e : g_list) live += (e.enemy && e.left > 0.0f) ? 1 : 0;
        for (Aura* a : g_auras) auras += (a && a->enemy) ? 1 : 0;
        ImGui::Text(_("Now: %d provoked, %d aura(s) | %u taunts, %u provokes, %u aura set-ups | glow %s"), live, auras,
                    g_taunts, g_provoked, g_auras_made, glow_ready() ? _("ready") : _("not yet"));
        ImGui::Unindent();
    }

    ImGui::SeparatorText(_("Anti-air"));
    ImGui::Checkbox(_("Aggressive anti-air"), &anti_air::enabled);
    ImGui::SameLine();
    help_marker(_("Enemies use their own anti-air attacks more readily while you are above them. Only enemies that have "
                  "anti-air attacks are affected; how many may attack at once is still set by Enemy Slotting."));
    if (anti_air::enabled) {
        ImGui::Indent();
        ImGui::SliderFloat(_("Reaction height"), &anti_air::window_factor, 0.2f, 1.0f, "x%.2f");
        ImGui::SameLine();
        help_marker(_("How high you must be before they react, as a share of the game's own height for each attack (x1 = "
                      "the game's; x0.6 = at 60% of it, so they answer lower jumps)."));
        ImGui::Checkbox(_("Prefer anti-air"), &anti_air::win_roll);
        ImGui::SameLine();
        help_marker(_("When an enemy would choose its next move at random, it takes the anti-air attack whenever you are "
                      "in its reach above it."));
        ImGui::Checkbox(_("No anti-air cooldown"), &anti_air::skip_cooldown);
        ImGui::SameLine();
        help_marker(_("Anti-air attacks ignore the wait the game puts between two uses of the same move."));
        ImGui::Text(_("Now: %u anti-air choices adjusted"), anti_air::g_adjusted);
        ImGui::Unindent();
    }
    ImGui::PopID();
}

void EnemyAI::on_config_load(const utility::Config& cfg) {
    mod_enabled         = cfg.get<bool>("provoke").value_or(false);
    radius              = cfg.get<float>("provoke_radius").value_or(radius);
    duration            = cfg.get<float>("provoke_duration").value_or(duration);
    speed               = cfg.get<float>("provoke_speed").value_or(speed);
    dt_rate             = cfg.get<float>("provoke_dt_rate").value_or(dt_rate);
    dt_any_difficulty   = cfg.get<bool>("provoke_dt_any_difficulty").value_or(dt_any_difficulty);
    think_rate          = cfg.get<float>("provoke_think_rate").value_or(think_rate);
    counted_only        = cfg.get<bool>("provoke_counted_only").value_or(counted_only);
    targeted            = cfg.get<bool>("provoke_targeted").value_or(targeted);
    targeted_and_radius = cfg.get<bool>("provoke_targeted_and_radius").value_or(targeted_and_radius);
    aura                = cfg.get<bool>("provoke_aura").value_or(aura);
    aura_color[0]       = cfg.get<float>("provoke_aura_r").value_or(aura_color[0]);
    aura_color[1]       = cfg.get<float>("provoke_aura_g").value_or(aura_color[1]);
    aura_color[2]       = cfg.get<float>("provoke_aura_b").value_or(aura_color[2]);
    aura_brightness     = cfg.get<float>("provoke_aura_brightness").value_or(aura_brightness);
    aura_reach          = cfg.get<float>("provoke_aura_width").value_or(aura_reach);
    aura_scale          = cfg.get<float>("provoke_aura_scale").value_or(aura_scale);
    anti_air::enabled       = cfg.get<bool>("anti_air").value_or(false);
    anti_air::window_factor = cfg.get<float>("anti_air_window").value_or(anti_air::window_factor);
    anti_air::win_roll      = cfg.get<bool>("anti_air_win_roll").value_or(anti_air::win_roll);
    anti_air::skip_cooldown = cfg.get<bool>("anti_air_skip_cooldown").value_or(anti_air::skip_cooldown);
}

void EnemyAI::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("provoke", mod_enabled);
    cfg.set<float>("provoke_radius", radius);
    cfg.set<float>("provoke_duration", duration);
    cfg.set<float>("provoke_speed", speed);
    cfg.set<float>("provoke_dt_rate", dt_rate);
    cfg.set<bool>("provoke_dt_any_difficulty", dt_any_difficulty);
    cfg.set<float>("provoke_think_rate", think_rate);
    cfg.set<bool>("provoke_counted_only", counted_only);
    cfg.set<bool>("provoke_targeted", targeted);
    cfg.set<bool>("provoke_targeted_and_radius", targeted_and_radius);
    cfg.set<bool>("provoke_aura", aura);
    cfg.set<float>("provoke_aura_r", aura_color[0]);
    cfg.set<float>("provoke_aura_g", aura_color[1]);
    cfg.set<float>("provoke_aura_b", aura_color[2]);
    cfg.set<float>("provoke_aura_brightness", aura_brightness);
    cfg.set<float>("provoke_aura_width", aura_reach);
    cfg.set<float>("provoke_aura_scale", aura_scale);
    cfg.set<bool>("anti_air", anti_air::enabled);
    cfg.set<float>("anti_air_window", anti_air::window_factor);
    cfg.set<bool>("anti_air_win_roll", anti_air::win_roll);
    cfg.set<bool>("anti_air_skip_cooldown", anti_air::skip_cooldown);
}
