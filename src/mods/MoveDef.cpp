#include "MoveDef.hpp"
#include "XmlMove.hpp"
#include "MoveTable.hpp"
#include "WallRun.hpp"               // wallrun::at / wallrun::pl offsets
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/CtnScript.hpp"
#include "../sdk/MoveRegistry.hpp"
#include "../sdk/MtMath.hpp"
#include "../sdk/uPlayer.hpp"      // kAtckDefTbl
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace wallrun;

namespace movedef {

bool mod_enabled = true;
uintptr_t jmp_ret_wjump = 0;
char folder[128] = "nativePC\\movedef";

namespace {

constexpr int kPlayerSlots = 4;        // uPlayer objects tracked at once per move (the player, NPC / boss copies, a partner)
constexpr int PHASE_ENTRY = 0, PHASE_WAIT = 1, PHASE_AIRBORNE = 2;   // the game's mBdyAction[1].phase, zeroed by setAction

constexpr uintptr_t fn_setActionLand    = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)
constexpr uintptr_t fn_checkEmJumpBegin = 0x00804970;   // uPlayer::checkEmJumpBegin, __usercall this@edi, result@al
constexpr uintptr_t MIN_ID[2] = { 0x34, 0x6C };          // first free atckId: Nero (moveActAtckU ends at 0x33), Dante (vanilla ends at 0x6B)

// ---- game helpers (Gerb's / DanteAirDodge's) -------------------------------------------------------------------
void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed_) {
    devil4_sdk::indexed_anim_call((uint32_t)motion, (uPlayer*)pl, attr, speed_, start, interp);   // requestSetMotion_2, whole body
}

void clear_add_speed(uintptr_t pl) {
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

void face(uintptr_t pl, float yaw) {
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

glm::vec3 wmat_row(uintptr_t pl, int row) {           // 0 right, 1 up, 2 forward
    const float* r = &at<float>(pl, pl::mWmat + 0x10 * row);
    return glm::vec3(r[0], r[1], r[2]);
}

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }

bool lmt_file_exists(const std::string& game_path) {
    return !game_path.empty() && std::filesystem::exists(".\\nativePC\\" + game_path + ".lmt");
}

int character_of(uintptr_t pl) { return at<int>(pl, 0xE64) == 1 ? move_registry::NERO : move_registry::DANTE; }   // uPlayer::mPlayerID: 0 Dante, 1 Nero

// The player's orientation with its up leaned `tilt` degrees toward the level part of `dir`, then rolled `roll` degrees
// about its forward (180 = upside down, Gerb's dive), as an MT quaternion (mt_basis_quat, MtMath.hpp).
void basis_quat(uintptr_t pl, const glm::vec3& dir, float tilt, float roll, float out[4]) {
    glm::vec3 b[3] = { wmat_row(pl, 0), wmat_row(pl, 1), wmat_row(pl, 2) };
    const glm::vec3 h(dir.x, 0.0f, dir.z);
    if (tilt != 0.0f && glm::length(h) > 1e-3f) {
        const glm::vec3 axis = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(h)));
        for (glm::vec3& v : b) v = mt_rodrigues(v, axis, tilt);
    }
    if (roll != 0.0f) {
        const glm::vec3 fwd = b[2];
        b[0] = mt_rodrigues(b[0], fwd, roll);
        b[1] = mt_rodrigues(b[1], fwd, roll);
    }
    mt_basis_quat(b[0], b[1], b[2], out);
}

// uPlayer::setActionAtck 0x7AAC80 (__userpurge: this@eax, atckId@ecx, body on the stack, callee pops): the hand-off
// the vanilla moves use to chain without a new row match (Stinger -> Million Stab: `setActionAtck(this, 6, -1)` then the
// new state function in the same tick). It swaps mAtckId (mOldAtckId keeps the previous), zeroes the lower phase through
// setActionLowerBdy(12) and the upper's when `body` is non-zero, bumps the attack counters, sets AST_ATCK and clears the
// cancel gate. RE: player_move_pipeline.md "Hand-off between moves".
constexpr uintptr_t fn_setActionAtck = 0x007AAC80;
void set_action_atck(uintptr_t pl, uint32_t id, int body) {
    uintptr_t fn = fn_setActionAtck;
    __asm {
        pushad
        mov eax, pl
        mov ecx, id
        push body
        call fn
        popad
    }
}

// uPlayer::setActionFootwork 0x7AA2C0 (__userpurge: this@esi, float hokan, char isMotSet; callee pops): how a ground
// move ends (Million Stab, Stinger: `setActionFootwork(10.0, 1)`): zeroes velocity / mMoveVel / mYoyakuAtckId / mAtckId,
// requests the (battle) footwork motion with `hokan` interpolation frames, setAction(1) (13 when posing).
constexpr uintptr_t fn_setActionFootwork = 0x007AA2C0;
void set_action_footwork(uintptr_t pl, float hokan, int is_mot_set) {
    uintptr_t fn = fn_setActionFootwork;
    __asm {
        pushad
        mov esi, pl
        push is_mot_set
        push hokan
        call fn
        popad
    }
}

// Leave the move: the footwork on the ground, the neutral air transition in the air.
void exit_move(uintptr_t pl) {
    if (at<uint32_t>(pl, pl::mActStat) & 0x1) set_action_footwork(pl, 10.0f, 1);
    else devil4_sdk::neutral_air_transition((uPlayer*)pl);
}

// uPlayer::checkEmJumpBegin: the enemy step (jump cancel) reads no cancel window, only a jump press, the upgrade and an
// enemy to step on; on success it starts action 25 itself. Called by the move while its gate is still off.
bool check_enemy_step(uintptr_t pl) {
    uint8_t r = 0;
    uintptr_t fn = fn_checkEmJumpBegin;
    __asm {
        pushad
        mov edi, pl
        mov eax, fn
        call eax
        mov byte ptr [r], al
        popad
    }
    return r != 0;
}

// ---- the loaded moves ------------------------------------------------------------------------------------------
struct PlayerState {                   // what the phase word can't hold, per player object
    uintptr_t player = 0;
    uint32_t last_seen = 0;
    int dir = -1;
    glm::vec3 dir_vec{0.0f};
    bool script_set = false;           // the synthesised script has replaced the borrowed one
    bool jump_held = false;            // the starting press hasn't been released yet (eat_press)
    int used = 0;                      // starts since the last landing
    int wait_ticks = 0, stuck_ticks = 0;
    float last_frame = -1.0f;
    int idle[4] = { 0, 0, 0, 0 };      // ticks since melee / gun / jump / style was last pressed (<next idle=>)
    uint64_t fx_fired = 0;             // timeline events fired this entry (bit per event, 64 at most)
    float fx_prev = -1.0f;             // the motion frame last tick (fires on prev < frame <= cur)
    uint32_t fx_motion = 0xFFFF;       // the motion the timeline is running on (overlays: reset when it changes)
    uintptr_t bank_rsrc = 0;           // the rMotionList this move put in its bank for this player (0 = none yet)
    bool bank_conflict = false;        // the bank already held another LMT (another mod's): reported once, the move refuses
    ctn::Script script;                // the in-memory .ctn script (its bytes must outlive the motion)
};
constexpr uint32_t kIdleButtons[4] = { 0x1, 0x2, 0x4, 0x200 };

int idle_index(uint32_t button) { for (int i = 0; i < 4; ++i) if (kIdleButtons[i] & button) return i; return -1; }

void count_idle(PlayerState& s, uintptr_t pl) {
    const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
    for (int i = 0; i < 4; ++i) s.idle[i] = (trg & kIdleButtons[i]) ? 0 : s.idle[i] + 1;
}

struct Move {
    xmlmove::File def;
    bool enabled = true;
    int row_marker = 0;                // MoveTable::row(character, marker)
    int cmd_bit = -1;                  // the <command>'s mCmdFlag bit, -1 none
    uint32_t cmd_id = 0;               // the row's CMD_ID byte: the bit, or a vanilla CMD_ID number
    input_commands::Command cmd;       // read in place by the game: a Move never moves (unique_ptr)
    PlayerState states[kPlayerSlots];
    uint32_t tick = 0;
    uintptr_t bank_failed_for = 0;     // the player object the LMT load failed for (reported once)
    int entries = 0;                   // readout

    const xmlmove::Stage& stage() const { return def.stages[0]; }

    PlayerState& state_for(uintptr_t p) {           // 4 slots, least-recently-seen eviction (design.md 5b)
        ++tick;
        for (PlayerState& s : states) if (s.player == p) { s.last_seen = tick; return s; }
        PlayerState* victim = &states[0];
        for (PlayerState& s : states) {
            if (s.player == 0) { victim = &s; break; }
            if (s.last_seen < victim->last_seen) victim = &s;
        }
        *victim = PlayerState{};
        victim->player = p;
        victim->last_seen = tick;
        return *victim;
    }
};

std::vector<std::unique_ptr<Move>>& moves() { static std::vector<std::unique_ptr<Move>> v; return v; }

// A <command> defined for sharing (a <commands> file, or a move file's second and later <command>s): its own flag bit,
// always on while the mod is; its predicate can only be the stick test with the command's own side_angle.
struct SharedCommand {
    xmlmove::Command def;
    int character = -1;
    int bit = -1;
    input_commands::Command cmd;
    std::string source;
};
std::vector<std::unique_ptr<SharedCommand>>& shared_commands() { static std::vector<std::unique_ptr<SharedCommand>> v; return v; }

SharedCommand* find_shared(int character, const std::string& name) {
    for (auto& c : shared_commands()) if (c->character == character && c->def.name == name) return c.get();
    return nullptr;
}

bool shared_predicate(uintptr_t pl, void* ctx);   // below

Move* find_move(uint32_t id) {
    for (auto& m : moves()) if (m->def.id == id) return m.get();
    return nullptr;
}
Move* find_move_by_name(const std::string& name) {
    for (auto& m : moves()) if (m->def.name == name) return m.get();
    return nullptr;
}

// readout counters
int g_suppressed = 0;
char g_last_event[160] = "";

void note(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(g_last_event, sizeof g_last_event, fmt, ap);
    va_end(ap);
    spdlog::info("MoveDef: {}", g_last_event);
}

// ---- the row ---------------------------------------------------------------------------------------------------
void apply_row(Move& m) {
    kAtckDefTbl* row = MoveTable::row(m.def.character, m.row_marker);
    if (!row) return;
    const bool on = mod_enabled && m.enabled;
    row->command.buffer = (m.cmd_id & 0xFF) | (on ? 0u : 0x100u);     // byte 1 = 1 keeps the row unmatchable
}

// ---- direction and slots ---------------------------------------------------------------------------------------
bool has_slot(const xmlmove::Stage& st, int dir) {
    for (const xmlmove::Slot& s : st.slots) if (s.dir == dir) return true;
    return false;
}

// The motion word for `dir`: the stage's <slot> block in the move's bank, else the stage's motion=; -1 none.
int motion_for(const Move& m, int dir) {
    const xmlmove::Stage& st = m.stage();
    for (const xmlmove::Slot& s : st.slots) if (s.dir == dir && m.def.lmt.bank >= 0) return (m.def.lmt.bank << 8) | (s.block & 0xFF);
    return st.motion;
}

// The stick relative to the facing: left / right beyond `side_angle` off the facing axis, else forward / back.
void stick_dirs(uintptr_t pl, float side_angle, int order[4]) {
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    const float across = glm::dot(stick, wmat_row(pl, 0));
    const float along  = glm::dot(stick, wmat_row(pl, 2));
    const bool sideways = std::fabs(across) >= std::cos(glm::radians(side_angle));
    const int side = across >= 0.0f ? xmlmove::RIGHT : xmlmove::LEFT;
    const int line = along >= 0.0f ? xmlmove::FORWARD : xmlmove::BACK;
    if (sideways) { order[0] = side; order[1] = line; }
    else          { order[0] = line; order[1] = side; }
    order[2] = order[1] == xmlmove::RIGHT || order[0] == xmlmove::RIGHT ? xmlmove::LEFT : xmlmove::RIGHT;
    order[3] = order[1] == xmlmove::FORWARD || order[0] == xmlmove::FORWARD ? xmlmove::BACK : xmlmove::FORWARD;
}

// Pick the direction (and its unit vector) the stage will play: by the stick or the facing, among the slots it has.
// A stage without slots plays its motion= facing forward.
int resolve_dir(const Move& m, uintptr_t pl, glm::vec3& out) {
    const xmlmove::Stage& st = m.stage();
    int dir = xmlmove::FORWARD;
    if (!st.slots.empty()) {
        int order[4] = { xmlmove::FORWARD, xmlmove::RIGHT, xmlmove::LEFT, xmlmove::BACK };
        if (st.launch.direction == "stick" && at<int>(pl, pl::mAnlgPow) != 0) stick_dirs(pl, st.launch.side_angle, order);
        dir = -1;
        for (int d : order) if (has_slot(st, d)) { dir = d; break; }
        if (dir < 0) dir = st.slots[0].dir;              // only up / down slots: take the first
    }
    switch (dir) {
    case xmlmove::RIGHT:   out =  wmat_row(pl, 0); break;
    case xmlmove::LEFT:    out = -wmat_row(pl, 0); break;
    case xmlmove::BACK:    out = -wmat_row(pl, 2); break;
    case xmlmove::UP:      out = glm::vec3(0.0f,  1.0f, 0.0f); break;
    case xmlmove::DOWN:    out = glm::vec3(0.0f, -1.0f, 0.0f); break;
    default:               out =  wmat_row(pl, 2); break;
    }
    return dir;
}

bool stick_sideways(uintptr_t pl, float side_angle) {
    if (at<int>(pl, pl::mAnlgPow) == 0) return false;
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    return std::fabs(glm::dot(stick, wmat_row(pl, 0))) >= std::cos(glm::radians(side_angle));
}

bool shared_predicate(uintptr_t pl, void* ctx) {
    const SharedCommand& c = *(SharedCommand*)ctx;
    if (c.def.predicate == "stick_sideways") return stick_sideways(pl, c.def.side_angle);
    return true;
}

// The command's per-tick predicate: the named check plus the <air_jump> budget.
bool can_start(Move& m, uintptr_t pl) {
    const xmlmove::Stage& st = m.stage();
    PlayerState& s = m.state_for(pl);
    if (at<uint32_t>(pl, pl::mActStat) & 0x1) s.used = 0;                        // standing: a new airtime
    if (st.air_jump.per_air > 0 && s.used >= st.air_jump.per_air) return false;
    if (st.air_jump.required) {
        const uintptr_t tbl = at<uintptr_t>(pl, pl::mpActDataTbl);               // kActionData: +0 wJumpCnt
        if (tbl && at<char>(pl, pl::mWJumpCnt) >= at<char>(tbl, 0)) return false;
    }
    if (m.def.command.predicate == "stick_sideways") return stick_sideways(pl, st.launch.side_angle);
    return true;
}

bool predicate_thunk(uintptr_t pl, void* ctx) { return can_start(*(Move*)ctx, pl); }

// the steps of the command the row's <input> names: the move's own, or a shared one
const std::vector<xmlmove::CommandStep>* input_steps(const Move& m) {
    const std::string& c = m.def.table.command;
    if (m.def.command.present && (c.empty() || c == m.def.command.name)) return &m.def.command.steps;
    if (const SharedCommand* sc = find_shared(m.def.character, c)) return &sc->def.steps;
    return nullptr;
}

bool command_uses(const Move& m, uint32_t btn) {
    const std::vector<xmlmove::CommandStep>* steps = input_steps(m);
    if (!steps) return false;
    for (const xmlmove::CommandStep& s : *steps) if (s.type == 1 && (s.btn & btn)) return true;
    return false;
}

uint32_t input_condition(const Move& m) {
    const std::string& c = m.def.table.command;
    if (m.def.command.present && (c.empty() || c == m.def.command.name)) return m.def.command.condition;
    if (const SharedCommand* sc = find_shared(m.def.character, c)) return sc->def.condition;
    return 0;
}

uint32_t condition_mask(uint32_t cnd) {       // CND_TYPE -> the mBtnOn bit it tests
    switch (cnd) { case 1: return 0x10; case 2: return 0x200; case 3: return 0x400; default: return 0; }
}

// ---- the air hike steps aside ----------------------------------------------------------------------------------
// Hook on uPlayer::checkWJumpBegin 0x803880 (the base both characters' overrides call after their own checks): return 0
// when this tick's jump press belongs to an XML move (its command uses the jump button, its condition holds, its
// predicate passes, its row could match where the player is), so checkAtck, which runs after it, can take the row.
bool __stdcall suppress_air_hike(uintptr_t pl) {
    if (!mod_enabled || !pl) return false;
    if (!(at<uint32_t>(pl, pl::mBtnTrg) & 0x4)) return false;
    const uint32_t on = at<uint32_t>(pl, pl::mBtnOn);
    const int character = character_of(pl);
    for (auto& up : moves()) {
        Move& m = *up;
        if (!m.enabled || m.def.character != character || !m.stage().air_jump.suppress_air_hike) continue;
        if (!command_uses(m, 0x4)) continue;
        const uint32_t mask = condition_mask(input_condition(m));
        if (mask && !(on & mask)) continue;
        const uint32_t stat = at<uint32_t>(pl, pl::mActStat);
        if (m.def.table.air == 2 && !(stat & 0x2)) continue;      // an air row while on the ground: not ours
        if (m.def.table.air == 1 && !(stat & 0x1)) continue;
        if (!can_start(m, pl)) continue;
        ++g_suppressed;
        // updateMotCancel (top of the tick) has already latched this press as the C_JUMP reserve; it is ours
        at<bool>(pl, pl::mMotCnclJumpReserve) = false;
        return true;
    }
    return false;
}

static bool s_suppress = false;

// 6 prologue bytes of 0x803880 (`push ebp; mov ebp, esp; and esp, -8`), re-executed when the air hike may run.
naked void detour_wjump_check() {
    _asm {
        pushad
        push ecx                        // this
        call suppress_air_hike          // __stdcall: pops its argument, result in al
        mov byte ptr [s_suppress], al
        popad
        cmp byte ptr [s_suppress], 0
        je original
        xor al, al                      // no air hike this tick
        ret
    original:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF8
        jmp dword ptr [jmp_ret_wjump]
    }
}

// ---- the .ctn script, the gate, the press ------------------------------------------------------------------
float first_cancel_frame(const xmlmove::Stage& st) { return st.cancels.empty() ? -1.0f : st.cancels.front().frame; }

bool gate_open(const xmlmove::Stage& st, uintptr_t pl) {
    const float f = first_cancel_frame(st);
    return f >= 0.0f && at<float>(pl, pl::mMotionFrame0) >= f;
}

// Cleared by setActionAtck; uPlayer::main evaluates no cancel of any kind while it is off (design.md 5b).
void open_cancel_gate(const xmlmove::Stage& st, uintptr_t pl) {
    if (gate_open(st, pl)) at<bool>(pl, pl::mIsCancelCheckEnable) = true;
}

// First tick after the motion request: replace the borrowed script with one built from the stage's <cancel>s.
void arm_script(const xmlmove::Stage& st, PlayerState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    if (st.cancels.empty() && st.triggers.empty()) return;
    ctn::Script& sc = s.script;
    sc.reset();
    // cancels and triggers merged by frame (both lists are sorted)
    size_t ci = 0, ti = 0;
    float cur = -1.0f;
    while (ci < st.cancels.size() || ti < st.triggers.size()) {
        const bool take_cancel = ti >= st.triggers.size() || (ci < st.cancels.size() && st.cancels[ci].frame <= st.triggers[ti].frame);
        const float frame = take_cancel ? st.cancels[ci].frame : st.triggers[ti].frame;
        if (frame != cur) { sc.wait(frame); cur = frame; }
        if (take_cancel) {
            const xmlmove::Cancel& c = st.cancels[ci++];
            if (c.categories.empty()) {
                if (c.mode == ctn::CANCEL_CLOSE) sc.close_all();
                else if (c.mode == ctn::CANCEL_RESERVE) sc.reserve({ C_NORM0, C_COMMAND, C_JUMP, C_CHOUHATSU, C_GUN, C_ALL });
                else sc.open_all();
            } else {
                CANCEL_NO cats[8]; size_t n = 0;
                for (int cat : c.categories) if (n < 8) cats[n++] = (CANCEL_NO)cat;
                sc.cancels((ctn::CancelMode)c.mode, cats, n);
            }
        } else {
            const xmlmove::Trigger& t = st.triggers[ti++];
            uint8_t info[6] = { 0, 0, 0, 0, 0, 0 };     // set 0: atck, free2, camVib, free1, free0, padVib; a zero byte changes nothing
            info[t.byte] = t.value;
            sc.contact(ctn::CONTACT_MOVE, info);
        }
    }
    sc.end();
    devil4_sdk::install_ctn_script((void*)pl, sc.data());
}

// gate_open / arm_script above make the script's cancels usable; this makes the stage's <trigger> bytes readable:
// mMotCont is {atck, camVib, free0, free1, free2, padVib} in memory (+0x1D7C ..), the .ctn info order differs
uint8_t contact_value(uintptr_t pl, int info_index) {
    static const int kOffsetOf[6] = { 0, 4, 1, 3, 2, 5 };   // info index -> byte offset in mMotCont
    return info_index >= 0 && info_index < 6 ? at<uint8_t>(pl, 0x1D7C + kOffsetOf[info_index]) : 0;
}

// The press that started the move can outlive its tick (the pad reports slower than the game ticks) and
// updateMotCancel latches it as the C_JUMP reserve; until the button is released once, eat both.
void eat_held_press(const xmlmove::Stage& st, PlayerState& s, uintptr_t pl) {
    if (!s.jump_held || !st.eat_press) return;
    if (!(at<uint32_t>(pl, pl::mBtnOn) & st.eat_press)) { s.jump_held = false; return; }
    at<uint32_t>(pl, pl::mBtnTrg) &= ~st.eat_press;
    if (st.eat_press & 0x4) at<bool>(pl, pl::mMotCnclJumpReserve) = false;
}

// ---- the runtime timeline (<effect> / <se> / <kill_effect> / <finish_effect>) -----------------------------------
void fire_fx(const xmlmove::FxEvent& e, uintptr_t pl) {
    switch (e.kind) {
    case xmlmove::FxEvent::EFFECT: {
        void* parent = (void*)pl;
        if (e.joint == "weapon") { parent = devil4_sdk::held_weapon((void*)pl); if (!parent) return; }
        const uint32_t material = e.material == -2 ? devil4_sdk::ground_material((void*)pl) : (uint32_t)e.material;
        void* effect = nullptr;
        if (e.list >= 0) effect = devil4_sdk::effect_const(e.list, e.index, parent, (uint32_t)e.group, material, e.unit_line, (void*)pl);
        else             effect = devil4_sdk::effect_generator((char*)e.efl.c_str(), (void*)pl, (uint8_t)e.unit_line, (uint32_t)e.group, material);
        if (effect) devil4_sdk::register_effect(effect, (void*)pl, e.type, e.id);
        return;
    }
    case xmlmove::FxEvent::SE:     devil4_sdk::play_player_se((void*)pl, e.id); return;
    case xmlmove::FxEvent::KILL:   devil4_sdk::kill_effect((void*)pl, e.id); return;
    case xmlmove::FxEvent::FINISH: devil4_sdk::finish_effect((void*)pl, e.id); return;
    }
}

// Once per entry (or per loop with repeat="loop"), when the motion frame passes the event's frame: prev < frame <= cur.
// The first tick after the request has prev = -1, so events at or before the start frame fire then.
void run_fx(const xmlmove::Stage& st, PlayerState& s, uintptr_t pl) {
    if (st.fx.empty()) return;
    const uint32_t motion = at<uint32_t>(pl, pl::mMotionCtrl0No);
    const float cur = at<float>(pl, pl::mMotionFrame0);
    if (motion != s.fx_motion) { s.fx_motion = motion; s.fx_fired = 0; s.fx_prev = -1.0f; }
    if (cur < s.fx_prev) {                                            // the block looped: re-arm the looping events
        for (size_t i = 0; i < st.fx.size() && i < 64; ++i) if (st.fx[i].repeat) s.fx_fired &= ~(1ull << i);
        s.fx_prev = -1.0f;
    }
    for (size_t i = 0; i < st.fx.size() && i < 64; ++i) {
        const uint64_t bit = 1ull << i;
        if (s.fx_fired & bit) continue;
        if (s.fx_prev < st.fx[i].frame && st.fx[i].frame <= cur) { s.fx_fired |= bit; fire_fx(st.fx[i], pl); }
    }
    s.fx_prev = cur;
}

// ---- stage transitions (<next>) --------------------------------------------------------------------------------
bool next_fires(const xmlmove::Next& n, const PlayerState& s, uintptr_t pl) {
    if (n.always) return true;
    const MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    if (n.motion_end && !at<bool>(pl, pl::mMotionFrameEnd0)) return false;
    if (n.landing) {
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (!(vel.y <= 0.0f && coll && at<bool>(coll, 0x1C))) return false;
    }
    if (n.frame >= 0.0f && at<float>(pl, pl::mMotionFrame0) < n.frame) return false;
    if (n.byte >= 0 && contact_value(pl, n.byte) != (uint8_t)n.value) return false;
    if (n.button && n.idle >= 0) {                      // the mash stopped: no press of that button for `idle` ticks
        const int i = idle_index(n.button);
        if (i < 0 || s.idle[i] < n.idle) return false;
    } else if (n.button) {
        const uint32_t word = n.held ? at<uint32_t>(pl, pl::mBtnOn) : at<uint32_t>(pl, pl::mBtnTrg);
        if (!(word & n.button)) return false;
    }
    if (n.stick_neutral && at<int>(pl, pl::mAnlgPow) != 0) return false;
    return n.frame >= 0.0f || n.motion_end || n.landing || n.byte >= 0 || n.button;   // no condition = never
}

// Returns true when the stage ended (the caller returns at once). "exit" leaves through the neutral transition; a move
// name hands off the Stinger way: setActionAtck(id, both halves) then that move's state function this same tick.
bool run_nexts(Move& m, uintptr_t pl);

// ---- the LMT bank, per player object ---------------------------------------------------------------------------
// The bank is ready only when it holds the LMT this move loaded: a bank another mod filled first (Gerb's, the wall
// run's, a second XML move's) would play that file's blocks under our slot numbers (a six-block file asked for slot 8
// wedges the motion controller: T-pose, frames stuck). Such a conflict is reported once and the move refuses.
bool ready(Move& m, uintptr_t pl) {
    if (m.def.lmt.bank < 0) return true;
    const uintptr_t existing = devil4_sdk::get_motion_bank((void*)pl, m.def.lmt.bank);
    PlayerState& s = m.state_for(pl);
    return existing != 0 && existing == s.bank_rsrc;
}

void ensure_bank(Move& m, uintptr_t pl) {
    const xmlmove::Lmt& l = m.def.lmt;
    if (!pl || l.bank < 10 || l.bank > 14 || l.path.empty()) return;     // 0-9 are setup banks, 15 Nero's Buster bank
    if (m.bank_failed_for == pl) return;
    PlayerState& s = m.state_for(pl);
    const uintptr_t existing = devil4_sdk::get_motion_bank((void*)pl, l.bank);
    if (existing != 0) {
        if (existing == s.bank_rsrc) return;                            // ours
        if (!s.bank_conflict) {
            s.bank_conflict = true;
            spdlog::warn("MoveDef: {}: bank {} of player {:#x} already holds another LMT ({:#x}): another mod's (Gerb / WallRun / "
                         "another XML move)? pick a free bank in <lmt bank=>; the move refuses to start", m.def.name, l.bank, pl, existing);
        }
        return;
    }
    if (!lmt_file_exists(l.path)) {
        spdlog::warn("MoveDef: {}: nativePC\{}.lmt not found; the move refuses to start until it is", m.def.name, l.path);
        m.bank_failed_for = pl;
        return;
    }
    if (!devil4_sdk::load_motion_bank((void*)pl, l.path.c_str(), l.bank)) { m.bank_failed_for = pl; return; }
    s.bank_rsrc = devil4_sdk::get_motion_bank((void*)pl, l.bank);
    s.bank_conflict = false;
    spdlog::info("MoveDef: {}: '{}' -> bank {} of player {:#x} ({:#x})", m.def.name, l.path, l.bank, pl, s.bank_rsrc);
}

// ---- the state function ----------------------------------------------------------------------------------------
void run_stage(Move& m, uintptr_t pl);

bool run_nexts(Move& m, uintptr_t pl) {
    const xmlmove::Stage& st = m.stage();
    const PlayerState& s = m.state_for(pl);
    for (const xmlmove::Next& n : st.nexts) {
        if (!next_fires(n, s, pl)) continue;
        if (n.to == "exit") {
            note("%s: exit (<next>)", m.def.name.c_str());
            exit_move(pl);
            return true;
        }
        Move* other = find_move_by_name(n.to);
        if (!other || other->def.character != m.def.character) {
            note("%s: <next to='%s'> names no loaded move of this character", m.def.name.c_str(), n.to.c_str());
            return false;
        }
        if (!other->enabled) continue;
        note("%s -> %s (hand-off)", m.def.name.c_str(), other->def.name.c_str());
        set_action_atck(pl, other->def.id, -1);           // mAtckId = other, phases zeroed, AST_ATCK, gate closed
        run_stage(*other, pl);                            // its entry this tick, as Stinger runs Million Stab's
        return true;
    }
    return false;
}

void run_stage(Move& m, uintptr_t pl) {
    const xmlmove::Stage& st = m.stage();
    const xmlmove::Launch& L = st.launch;
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    PlayerState& s = m.state_for(pl);

    switch (phase) {
    case PHASE_ENTRY: {
        ++m.entries;
        ++s.used;
        ensure_bank(m, pl);
        if (!ready(m, pl)) {
            note("%s: bank %d of player %p is empty or holds another mod's LMT (nativePC\%s.lmt missing, or the bank is taken: see the log): leaving",
                 m.def.name.c_str(), m.def.lmt.bank, (void*)pl, m.def.lmt.path.c_str());
            exit_move(pl);
            return;
        }
        glm::vec3 dv;
        const int dir = resolve_dir(m, pl, dv);
        const int motion = motion_for(m, dir);
        if (motion < 0) {
            note("%s: no motion for direction %s (no <slot> and no motion=)", m.def.name.c_str(), xmlmove::dir_name(dir));
            devil4_sdk::neutral_air_transition((uPlayer*)pl);
            return;
        }
        s.dir = dir; s.dir_vec = dv;
        for (int& i : s.idle) i = 0;                                   // the starting press counts as a press
        if (st.stop) {
            vel.x = vel.y = vel.z = 0.0f; acc.x = acc.y = acc.z = 0.0f;
            at<float>(pl, pl::mMoveVel) = 0.0f; at<float>(pl, pl::mMoveAcc) = 0.0f;
            clear_add_speed(pl);
        }
        if (L.present && L.face && dir != xmlmove::UP && dir != xmlmove::DOWN) face(pl, yaw_of(dv));
        if (st.air_jump.cost > 0) at<char>(pl, pl::mWJumpCnt) += (char)st.air_jump.cost;
        if (st.ctn_bank >= 0) devil4_sdk::borrow_ctn((void*)pl, (uint32_t)st.ctn_bank, (uint32_t)st.ctn_slot);
        request_motion(pl, motion, st.attr, st.interp, st.start, st.speed);
        s.script_set = false;
        s.jump_held = st.eat_press != 0;
        s.wait_ticks = s.stuck_ticks = 0; s.last_frame = -1.0f;
        s.fx_fired = 0; s.fx_prev = -1.0f; s.fx_motion = 0xFFFF;
        note("%s start: %s, motion %04X, player %p", m.def.name.c_str(), xmlmove::dir_name(dir), motion, (void*)pl);
        phase = PHASE_WAIT;
        return;
    }
    case PHASE_WAIT: {
        acc.x = acc.y = acc.z = 0.0f;
        count_idle(s, pl);
        eat_held_press(st, s, pl);
        arm_script(st, s, pl);
        open_cancel_gate(st, pl);
        run_fx(st, s, pl);
        if (st.enemy_step && !gate_open(st, pl) && check_enemy_step(pl)) return;      // stepped: action 25 took over
        if (run_nexts(m, pl)) return;
        if (!L.present) { phase = PHASE_AIRBORNE; return; }                             // no ballistics: just the motion
        if (at<float>(pl, pl::mMotionFrame0) < L.frame) {
            if (++s.wait_ticks > st.watchdog) {
                note("%s: motion did not reach the launch frame %.1f in %d ticks (frame %.1f, controller plays %04X): block missing / empty, or bank %d taken?",
                     m.def.name.c_str(), L.frame, st.watchdog, at<float>(pl, pl::mMotionFrame0), at<uint32_t>(pl, pl::mMotionCtrl0No), m.def.lmt.bank);
                exit_move(pl);
            }
            return;
        }
        const glm::vec3 v = s.dir_vec * L.speed + glm::vec3(0.0f, L.rise, 0.0f);
        vel.x = v.x; vel.y = v.y; vel.z = v.z;
        const glm::vec3 xz(v.x, 0.0f, v.z);                       // keep the game's own XZ model in step
        at<float>(pl, pl::mMoveVel) = glm::length(xz);
        at<float>(pl, pl::mMoveAcc) = 0.0f;
        if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = yaw_of(xz);
        clear_add_speed(pl);
        if (st.status_jump) at<uint32_t>(pl, pl::mActStat) = (at<uint32_t>(pl, pl::mActStat) & ~0x1u) | 0x2u;
        if (L.fx.effect) {
            float q[4];
            if (L.fx.tilt == 0.0f && L.fx.roll == 0.0f) { const float* g = &at<float>(pl, pl::mQuat); q[0] = g[0]; q[1] = g[1]; q[2] = g[2]; q[3] = g[3]; }
            else basis_quat(pl, s.dir_vec, L.fx.tilt, L.fx.roll, q);
            devil4_sdk::effect_stay(L.fx.list, L.fx.index, q, &at<float>(pl, pl::mPos), (void*)pl);
        }
        if (L.fx.se) devil4_sdk::play_player_se((void*)pl, L.fx.se_id);
        phase = PHASE_AIRBORNE;
        return;
    }
    case PHASE_AIRBORNE: {
        count_idle(s, pl);
        eat_held_press(st, s, pl);
        arm_script(st, s, pl);
        open_cancel_gate(st, pl);
        run_fx(st, s, pl);
        if (st.enemy_step && !gate_open(st, pl) && check_enemy_step(pl)) return;
        if (run_nexts(m, pl)) return;
        const uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (st.land == "exit" && L.present && vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand (ballistic moves only)
        if (L.present) {
            vel.y += L.gravity * at<float>(pl, pl::m_delta_time);
            if (vel.y < -L.max_fall) vel.y = -L.max_fall;
        }
        acc.x = acc.y = acc.z = 0.0f;
        const float frame = at<float>(pl, pl::mMotionFrame0);         // watchdog: the exits need the motion to play
        s.stuck_ticks = (frame == s.last_frame) ? s.stuck_ticks + 1 : 0;
        s.last_frame = frame;
        if (s.stuck_ticks > st.watchdog) {
            note("%s: motion frame stuck at %.1f for %d ticks (controller plays %04X): block missing / empty, or bank %d taken?",
                 m.def.name.c_str(), frame, st.watchdog, at<uint32_t>(pl, pl::mMotionCtrl0No), m.def.lmt.bank);
            exit_move(pl);
        }
        return;
    }
    default:
        return;
    }
}

// ---- loading ---------------------------------------------------------------------------------------------------
bool validate(const xmlmove::File& f) {
    if (f.name.empty())            { spdlog::error("MoveDef: {}: <move> needs a name", f.source); return false; }
    if (f.character < 0)           { spdlog::error("MoveDef: {}: character must be pl000 or pl006", f.source); return false; }
    if (f.stages.empty())          { spdlog::error("MoveDef: {}: no <stage>", f.source); return false; }
    if (f.overlay) {               // a vanilla move's overlay: only its <next>s matter
        if (f.id == 0 || f.id >= MIN_ID[f.character]) { spdlog::error("MoveDef: {}: an overlay's id must be a vanilla atckId (below {:#x})", f.source, MIN_ID[f.character]); return false; }
        if (f.stages[0].nexts.empty()) { spdlog::error("MoveDef: {}: an overlay needs <next>s", f.source); return false; }
        return true;
    }
    if (f.id < MIN_ID[f.character]) { spdlog::error("MoveDef: {}: id {:#x} is below the first free id {:#x} for that character", f.source, f.id, MIN_ID[f.character]); return false; }
    // no <table> = a hand-off-only move: no row, reached through another move's <next to=> (it still gets its id)
    if (f.lmt.bank >= 0 && (f.lmt.bank < 10 || f.lmt.bank > 14)) { spdlog::error("MoveDef: {}: <lmt bank> must be 10-14", f.source); return false; }
    const xmlmove::Stage& st = f.stages[0];
    if (st.slots.empty() && st.motion < 0) { spdlog::error("MoveDef: {}: the stage needs <slot>s or motion=", f.source); return false; }
    if (!st.slots.empty() && f.lmt.bank < 0) { spdlog::error("MoveDef: {}: <slot>s need an <lmt bank=>", f.source); return false; }
    return true;
}

// The row's CMD_ID byte: the move's own <command> bit when the <input> names it (or names nothing and the move has one),
// a shared command's bit when it names that, else the number given, else 0 (the attr's plain button).
uint32_t command_id_for(const Move& m) {
    const std::string& c = m.def.table.command;
    if (m.def.command.present && (c.empty() || c == m.def.command.name)) return m.cmd_bit >= 0 ? (uint32_t)m.cmd_bit : 0;
    if (c.empty()) return 0;
    if (const SharedCommand* sc = find_shared(m.def.character, c)) return sc->bit >= 0 ? (uint32_t)sc->bit : 0;
    char* end = nullptr;
    const unsigned long v = std::strtoul(c.c_str(), &end, 0);
    if (end == c.c_str()) { spdlog::error("MoveDef: {}: <input command='{}'> names no <command> of this file, no shared command of the character, and is no number", m.def.name, c); return 0; }
    return (uint32_t)v;
}

// cancelId word: yCancelInfo { cmdNo:1, cmdId:1, cmdOr:1, dmy:21, val:8 } (RE/player_move_pipeline.md); the vanilla
// rows' 0x07000005 = "or: window C_ALL open", 1 = "window C_NORM0 open".
uint32_t cancel_word(bool cmd_no, bool cmd_id, bool cmd_or, uint32_t val) {
    return (val << 24) | (cmd_no ? 1u : 0u) | (cmd_id ? 2u : 0u) | (cmd_or ? 4u : 0u);
}

// The row's five cancelId words: cancel_words= as given, else <follows> / <window> encoded (a <follows> with a window is
// two words), else the hand-coded rows' default. `pending` = every file parsed this load, for names not yet loaded.
bool cancel_words_for(const Move& m, const std::vector<xmlmove::File>& pending, uint32_t out[5]) {
    const xmlmove::Table& t = m.def.table;
    for (int i = 0; i < 5; ++i) out[i] = t.cancel_words[i];
    if (t.explicit_words || (t.follows.empty() && t.windows.empty())) return true;
    for (int i = 0; i < 5; ++i) out[i] = 0;
    int n = 0;
    auto push = [&](uint32_t w) { if (n < 5) out[n++] = w; else ++n; };
    for (const xmlmove::Follows& f : t.follows) {
        uint32_t prev = 0;
        if (const Move* other = find_move_by_name(f.move)) prev = other->def.id;
        else {
            for (const xmlmove::File& pf : pending) if (!pf.commands_only && pf.name == f.move && pf.character == m.def.character) { prev = pf.id; break; }
        }
        if (!prev) {
            char* end = nullptr; const unsigned long v = std::strtoul(f.move.c_str(), &end, 0);   // a vanilla atckId number
            if (end != f.move.c_str()) prev = (uint32_t)v;
        }
        if (!prev) { spdlog::error("MoveDef: {}: <follows move='{}'> names no loaded move of this character and is no id", m.def.name, f.move); return false; }
        push(cancel_word(false, true, f.any, prev));
        if (f.window >= 0) push(cancel_word(true, false, f.any, (uint32_t)f.window));
    }
    for (const xmlmove::Window& w : t.windows) push(cancel_word(true, false, w.any, (uint32_t)w.category));
    if (n > 5) { spdlog::error("MoveDef: {}: <follows> / <window> need {} cancel words, the row holds 5", m.def.name, n); return false; }
    return true;
}

void build_command(Move& m) {
    const xmlmove::Command& c = m.def.command;
    if (!c.present) return;
    using namespace input_commands;
    m.cmd = Command{};
    m.cmd.condition(c.condition).active(c.active);
    for (const xmlmove::CommandStep& s : c.steps) {
        if (s.type == 1) m.cmd.button(s.btn, s.mode, s.timer);
        else             m.cmd.lever(s.lever, s.mode, s.timer);
    }
    m.cmd.head();                      // refresh num / data: the runner reads this object's header in place (reload rebuilds it)
}

void add_move(xmlmove::File&& f, const std::vector<xmlmove::File>& pending) {
    if (!validate(f)) return;
    if (Move* dup = find_move(f.id)) {
        if (!(dup->def.overlay && f.overlay && dup->def.character != f.character)) {   // the same vanilla id on both characters is fine
            spdlog::error("MoveDef: {}: id {:#x} is already '{}' ({})", f.source, f.id, dup->def.name, dup->def.source);
            return;
        }
    }
    auto up = std::make_unique<Move>();
    Move& m = *up;
    m.def = std::move(f);
    if (m.def.overlay) {               // no row, no registry entry, no bank: the vanilla tick finds it by id
        moves().push_back(std::move(up));
        spdlog::info("MoveDef: loaded overlay '{}' on vanilla id {:#x} ({})", moves().back()->def.name, moves().back()->def.id, moves().back()->def.character == 0 ? "Nero" : "Dante");
        return;
    }
    // the command
    if (m.def.command.present) {
        build_command(m);
        const int bit = input_commands::next_free_bit();
        if (bit < 0 || !input_commands::add(m.cmd.head(), bit, &m.enabled, &predicate_thunk, &m)) {
            spdlog::error("MoveDef: {}: no free command bit (85-95 all taken); the move is not loaded", m.def.name);
            return;
        }
        m.cmd_bit = bit;
    }
    m.cmd_id = command_id_for(m);
    if (m.def.table.present) {
        uint32_t cw[5];
        if (!cancel_words_for(m, pending, cw)) return;
        // the row: {attr, id, level, info, command, weapon, style, etc, as, cancelId[5]}
        const xmlmove::Table& t = m.def.table;
        const uint32_t info = (t.start ? 1u : 0u) | (t.cancel ? 2u : 0u) | (t.repeat ? 4u : 0u);
        const uint32_t words[14] = { t.attr, m.def.id, t.level, info, (m.cmd_id & 0xFF) | 0x100u, (uint32_t)t.weapon, (uint32_t)t.style,
                                     t.etc, t.air, cw[0], cw[1], cw[2], cw[3], cw[4] };
        m.row_marker = MoveTable::add_row(m.def.character, words);
    }
    move_registry::register_move(m.def.character, m.def.id, &m.enabled, &run_move);
    moves().push_back(std::move(up));
    apply_row(*moves().back());
    spdlog::info("MoveDef: loaded '{}' ({}, id {:#x}, row marker {} (0 = hand-off only), command bit {})", moves().back()->def.name,
                 moves().back()->def.character == 0 ? "Nero" : "Dante", moves().back()->def.id, moves().back()->row_marker, moves().back()->cmd_bit);
}

void add_shared_commands(const xmlmove::File& f) {
    for (const xmlmove::Command& c : f.shared_commands) {
        if (f.character < 0) { spdlog::error("MoveDef: {}: shared <command> '{}' needs a character", f.source, c.name); continue; }
        if (find_shared(f.character, c.name)) { spdlog::error("MoveDef: {}: shared command '{}' is already defined", f.source, c.name); continue; }
        auto up = std::make_unique<SharedCommand>();
        SharedCommand& sc = *up;
        sc.def = c; sc.character = f.character; sc.source = f.source;
        sc.cmd.condition(c.condition).active(c.active);
        for (const xmlmove::CommandStep& s : c.steps) {
            if (s.type == 1) sc.cmd.button(s.btn, s.mode, s.timer);
            else             sc.cmd.lever(s.lever, s.mode, s.timer);
        }
        const int bit = input_commands::next_free_bit();
        if (bit < 0 || !input_commands::add(sc.cmd.head(), bit, &mod_enabled, &shared_predicate, &sc)) {
            spdlog::error("MoveDef: {}: no free command bit for shared command '{}'", f.source, c.name);
            continue;
        }
        sc.bit = bit;
        shared_commands().push_back(std::move(up));
        spdlog::info("MoveDef: shared command '{}' ({}) on bit {}", c.name, f.character == 0 ? "Nero" : "Dante", bit);
    }
}

// The dispatch detours call this for every tick of a vanilla move (MoveRegistry::vanilla_tick): an overlay with that id
// evaluates its <next>s, on the stage's motion only when motion= is given. True = a hand-off happened and the game's
// function must not run this tick (its phase word now belongs to the new move).
bool __stdcall overlay_tick(int character, uint32_t id, uintptr_t pl) {
    if (!mod_enabled) return false;
    for (auto& up : moves()) {
        Move& m = *up;
        if (!m.def.overlay || !m.enabled || m.def.character != character) continue;
        if (m.def.id != id && std::find(m.def.extra_ids.begin(), m.def.extra_ids.end(), id) == m.def.extra_ids.end()) continue;
        const xmlmove::Stage& st = m.stage();
        if (st.motion >= 0 && (int)at<uint32_t>(pl, pl::mMotionCtrl0No) != st.motion) continue;
        PlayerState& s = m.state_for(pl);
        count_idle(s, pl);
        run_fx(st, s, pl);
        if (run_nexts(m, pl)) { ++m.entries; return true; }
    }
    return false;
}

} // namespace

// ---- public ----------------------------------------------------------------------------------------------------
void __stdcall run_move(uintptr_t player, uint32_t atck_id) {
    Move* m = find_move(atck_id);
    if (m && m->enabled) run_stage(*m, player);
}

void apply_rows() { for (auto& m : moves()) apply_row(*m); }

void load_all() {
    namespace fs = std::filesystem;
    if (!fs::exists(folder)) { spdlog::info("MoveDef: no {} folder, nothing to load", folder); return; }
    std::vector<std::string> files;
    for (const fs::directory_entry& e : fs::directory_iterator(folder))
        if (e.is_regular_file() && _stricmp(e.path().extension().string().c_str(), ".xml") == 0) files.push_back(e.path().string());
    std::sort(files.begin(), files.end());
    // pass 1: parse everything, so names resolve whatever the file order
    std::vector<xmlmove::File> parsed;
    for (const std::string& p : files) {
        xmlmove::File f;
        if (xmlmove::load(p.c_str(), f)) parsed.push_back(std::move(f));
    }
    // pass 2: the shared commands (a move's <input> may name one from another file)
    for (const xmlmove::File& f : parsed) add_shared_commands(f);
    // pass 3: the moves, with every parsed file in reach for <follows move=> names
    for (xmlmove::File& f : parsed) if (!f.commands_only) add_move(std::move(f), parsed);
    spdlog::info("MoveDef: {} move(s), {} shared command(s) loaded from {}", moves().size(), shared_commands().size(), folder);
}

// Re-read every loaded move's file: tunables, slots, cancels, launch, command steps change in place; the id, the row
// and the command bit stay. A file whose id isn't loaded yet needs a restart (its row must exist before the game
// hands the table to a player).
void reload() {
    namespace fs = std::filesystem;
    if (!fs::exists(folder)) return;
    for (const fs::directory_entry& e : fs::directory_iterator(folder)) {
        if (!e.is_regular_file() || _stricmp(e.path().extension().string().c_str(), ".xml") != 0) continue;
        xmlmove::File f;
        if (!xmlmove::load(e.path().string().c_str(), f)) continue;
        if (f.commands_only) {
            for (const xmlmove::Command& c : f.shared_commands)
                if (SharedCommand* sc = find_shared(f.character, c.name)) {
                    sc->def = c;
                    sc->cmd = input_commands::Command{};
                    sc->cmd.condition(c.condition).active(c.active);
                    for (const xmlmove::CommandStep& s : c.steps) { if (s.type == 1) sc->cmd.button(s.btn, s.mode, s.timer); else sc->cmd.lever(s.lever, s.mode, s.timer); }
                    sc->cmd.head();
                } else spdlog::warn("MoveDef: {}: new shared command '{}' needs a restart", f.source, c.name);
            continue;
        }
        if (!validate(f)) continue;
        Move* m = find_move(f.id);
        if (!m) { spdlog::warn("MoveDef: {}: new move '{}' (id {:#x}) needs a restart to get its row", f.source, f.name, f.id); continue; }
        if (m->def.character != f.character) { spdlog::warn("MoveDef: {}: the character of a loaded move can't change (restart)", f.source); continue; }
        const bool had_cmd = m->def.command.present;
        m->def = std::move(f);
        if (m->def.command.present && had_cmd) build_command(*m);          // the steps are read in place by the game
        else if (m->def.command.present != had_cmd) spdlog::warn("MoveDef: {}: adding / removing <command> needs a restart", m->def.source);
        m->cmd_id = command_id_for(*m);
        m->bank_failed_for = 0;
        apply_row(*m);
        spdlog::info("MoveDef: reloaded '{}'", m->def.name);
    }
}

} // namespace movedef

// ---------------------------------------------------------------------------------------------------------------
// The Mod: hooks, config, GUI.
// ---------------------------------------------------------------------------------------------------------------
using namespace movedef;

std::optional<std::string> MoveDef::on_initialize() {
    load_all();
    move_registry::set_vanilla_tick(&overlay_tick);
    if (!input_commands::hook_installed) {       // Gerb installs the same hook when it is built in; one installer only
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("MoveDef: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init MoveDef (command hook)";
        }
        input_commands::hook_installed = true;
    }
    if (!install_hook_absolute(0x803880, hook_wjump, &detour_wjump_check, &jmp_ret_wjump, 6)) {
        spdlog::error("MoveDef: failed to hook uPlayer::checkWJumpBegin (0x803880)");
        return "Failed to init MoveDef (air hike hook)";
    }
    return Mod::on_initialize();
}

void MoveDef::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    uPlayer* pl = devil4_sdk::get_local_player();
    if (!pl) return;
    const int character = character_of((uintptr_t)pl);
    for (auto& m : moves()) if (m->enabled && m->def.character == character) ensure_bank(*m, (uintptr_t)pl);
}

void MoveDef::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::PushID("MoveDef");
    if (ImGui::Checkbox(_("XML moves (MoveDef)"), &mod_enabled)) apply_rows();
    ImGui::SameLine();
    help_marker(_("Player moves defined in nativePC\\movedef\\*.xml (E:\\DMC mod stuffs\\Vibed\\MoveDef\\schema.md). Each file adds a move-table "
                  "row, an input command and a motion bank, and runs through one generic state function. Reload re-reads the "
                  "loaded files; a new file needs a restart."));
    ImGui::SameLine();
    if (ImGui::Button(_("Reload"))) reload();
    if (ImGui::TreeNode(_("Moves"))) {
        if (moves().empty()) ImGui::TextDisabled("%s", _("none loaded"));
        for (auto& up : moves()) {
            Move& m = *up;
            ImGui::PushID(&m);
            if (ImGui::Checkbox(m.def.name.c_str(), &m.enabled)) apply_row(m);
            ImGui::SameLine();
            if (m.def.overlay)
                ImGui::Text("%s overlay on vanilla id %#x | hand-offs %d", m.def.character == 0 ? "Nero" : "Dante", m.def.id, m.entries);
            else
                ImGui::Text("%s id %#x | bank %d '%s' | command bit %d (row cmd %u) | entries %d",
                            m.def.character == 0 ? "Nero" : "Dante", m.def.id, m.def.lmt.bank, m.def.lmt.path.c_str(), m.cmd_bit, m.cmd_id, m.entries);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        const uintptr_t P = (uintptr_t)pl;
        ImGui::Text("actStat %02X  frame %.1f  phase %d  atckId %u  wjump %d  air hikes suppressed %d",
                    at<uint32_t>(P, pl::mActStat), at<float>(P, pl::mMotionFrame0), at<int>(P, pl::mBdyActionLowerPhase),
                    at<uint32_t>(P, pl::mAtckId), (int)at<char>(P, pl::mWJumpCnt), g_suppressed);
        ImGui::TextWrapped("last: %s", g_last_event);
    }
    ImGui::PopID();
}

void MoveDef::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("movedef_enabled").value_or(true);
    for (auto& m : moves()) m->enabled = cfg.get<bool>("movedef_" + m->def.name).value_or(true);
    apply_rows();
}

void MoveDef::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("movedef_enabled", mod_enabled);
    for (auto& m : moves()) cfg.set<bool>("movedef_" + m->def.name, m->enabled);
}
