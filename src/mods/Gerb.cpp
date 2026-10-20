#include "Gerb.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include <cmath>
#include <filesystem>
#include <string>

using namespace wallrun;

bool  Gerb::mod_enabled   = false;
int   Gerb::input_cmd     = LOCKON_ANYDIRECTION_STYLE;   // lock-on + stick in any direction + style; the stick's direction picks the jump's
int   Gerb::lmt_bank      = 13;
char  Gerb::lmt_path[128] = "motion\\pl000\\gerb";          // Nero's
int   Gerb::slot[Gerb::DIR_COUNT] = { 0, 1, 2, 4, 3, 5 };   // forward, down, back, left, right, up
int   Gerb::ctn_bank      = 0;
int   Gerb::ctn_slot      = 5;           // the fall motion's script: no cancels until the game's air rules say so
uint32_t Gerb::down_mask  = 0;           // pick a button (mBtnOn bits) to get the down jump; 0 = never
float Gerb::launch_frame  = 0.0f;
float Gerb::speed         = 14.0f;
float Gerb::rise          = 14.0f;
float Gerb::up_speed      = 32.0f;
float Gerb::down_speed    = 24.0f;
float Gerb::gravity       = -1.1f;
float Gerb::max_fall      = 30.0f;
float Gerb::cancel_frame  = 10.0f;
float Gerb::side_angle    = 50.0f;
bool  Gerb::face_direction  = true;
bool  Gerb::airborne_status = true;

// ---------------------------------------------------------------------------------------------------------------
// Game functions (the few the jump needs; WallRun.cpp keeps its own copies file-local).
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_setActionLand = 0x007AA6A0;   // uPlayer::setActionLand(this@ecx)

void set_action_land(uintptr_t pl) { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

// .ctn borrow (MoveDef D15): the next motion request reads the cancel script of ctn_bank:ctn_slot for both halves.
void borrow_ctn(uintptr_t pl, uint32_t bank, uint32_t idx) {
    for (int half = 0; half < 2; ++half) {
        uintptr_t p = pl + pl::mMotionSetCharTblParam + 12 * half;
        at<bool>(p, 0)     = true;
        at<uint32_t>(p, 4) = bank;
        at<uint32_t>(p, 8) = idx;
    }
}

// The cancel window the game's own way (MoveDef D11): a .ctn script synthesised in memory, `wait(frame)` then one
// contact command opening every cancel category, and both cCharTblMgr cursors pointed at it. cCharTblMgr::update
// then runs it like any vanilla script and updateContactInfoSub opens the windows, which stay open as a vanilla
// cancel does. One script per player slot; the bytes must outlive the motion, so they're static.
//   00 mode pad16 f32 frame          wait until mFrame >= frame (mode 0: go on)
//   01 01 b0 b1 b2 b3 b4 b5          contact set 1: byte k's low nibble picks cancelNoTbl[k] (0 = C_NORM0, C_COMMAND,
//                                    C_JUMP, C_CHOUHATSU, C_GUN, C_ALL for k = 0..5), high nibble 0x10 = open
//   FF                               end
#pragma pack(push, 1)
struct CancelScript {
    uint8_t  wait_op = 0, wait_mode = 0; uint16_t wait_pad = 0; float wait_frame = 0.0f;
    uint8_t  contact_op = 1, contact_idx = 1, info[6] = { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };
    uint8_t  end_op = 0xFF, pad[3] = { 0, 0, 0 };
};
#pragma pack(pop)
static_assert(sizeof(CancelScript) == 20, "ctn script layout");
static CancelScript g_cancel_scripts[2];

// Point both halves' cursors at the script. Call after updateMotion has installed the borrowed script for the new
// motion (the tick after the request), else updateMotion overwrites the cursors again.
void install_cancel_script(uintptr_t pl, int slot, float frame) {
    CancelScript& sc = g_cancel_scripts[slot & 1];
    sc.wait_frame = frame;
    for (int half = 0; half < 2; ++half) {
        uintptr_t mgr = pl + pl::mCharTblMgr + 0x24 * half;
        at<uint8_t*>(mgr, 0x0C) = (uint8_t*)&sc;   // mpDataTop
        at<uint8_t*>(mgr, 0x10) = (uint8_t*)&sc;   // mpDataCrnt
        at<bool>(mgr, 0x14)     = false;           // mIsDone
    }
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

bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

constexpr int PHASE_ENTRY   = 0;
constexpr int PHASE_WAIT    = 1;     // cycle playing, waiting for launch_frame
constexpr int PHASE_AIRBORNE = 2;

static int g_dbg_n0_at_move = -1, g_dbg_all_at_move = -1;   // mStat[0] / [7] as seen inside Gerb::move (after updateCharTbl + the checks)

struct JumpState {                   // what the phase word can't hold
    uintptr_t player = 0;
    Gerb::Dir dir = Gerb::FORWARD;
    glm::vec3 dir_vec{0.0f};         // world direction of the jump (unit, level for the sideways ones)
    bool script_set = false;         // the cancel script has replaced the borrowed .ctn script
};
JumpState g_jumps[2];
int slot_of(const JumpState& s) { return (int)(&s - g_jumps); }
JumpState& state_for(uintptr_t player) {
    for (JumpState& s : g_jumps) if (s.player == player) return s;
    for (JumpState& s : g_jumps) if (s.player == 0) { s.player = player; return s; }
    g_jumps[0] = JumpState{}; g_jumps[0].player = player; return g_jumps[0];
}

// First tick after the motion request: swap the borrowed script for the cancel script (if a cancel frame is set).
void arm_cancel_script(JumpState& s, uintptr_t pl) {
    if (s.script_set) return;
    s.script_set = true;
    if (Gerb::cancel_frame >= 0.0f) install_cancel_script(pl, slot_of(s), Gerb::cancel_frame);
}

// checkAtck's first gate wants a melee window (C_NORM0-2) open or mActLimit.atck; from the cancel frame on, give it
// the limit bit too (the move functions OR bits into mActLimit the same way; setAction reloads it on the next action).
void allow_attacks(uintptr_t pl) {
    if (Gerb::cancel_frame >= 0.0f && at<float>(pl, pl::mMotionFrame0) >= Gerb::cancel_frame)
        at<uint32_t>(pl, pl::mActLimit) |= 0x400;   // AL_ATCK
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Direction: the stick against the facing. Centred = up (down instead while a down_mask button is held); within
// side_angle of the facing = forward, of its opposite = back; otherwise left / right by the sign against the
// right axis.
// ---------------------------------------------------------------------------------------------------------------
Gerb::Dir Gerb::pick_direction(uintptr_t pl) {
    if (at<int>(pl, pl::mAnlgPow) == 0)
        return (down_mask && (at<uint32_t>(pl, pl::mBtnOn) & down_mask)) ? DOWN : UP;
    const float yaw = at<float>(pl, pl::mAnlgRadian);
    const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
    const glm::vec3 fwd   = wmat_row(pl, 2);
    const glm::vec3 right = wmat_row(pl, 0);
    const float along = glm::dot(stick, fwd);
    const float cosA  = std::cos(glm::radians(side_angle));
    if (along >=  cosA) return FORWARD;
    if (along <= -cosA) return BACK;
    return glm::dot(stick, right) >= 0.0f ? RIGHT : LEFT;
}

bool Gerb::ready(uintptr_t pl) {
    return pl && devil4_sdk::get_motion_bank((void*)pl, lmt_bank) != 0;
}

void Gerb::ensure_bank(uintptr_t pl) {
    if (!pl || lmt_bank < 10 || lmt_bank > 14) return;      // 0-9 are setup banks, 15 Nero's Buster bank
    static uintptr_t s_failed_for = 0;
    if (s_failed_for == pl || ready(pl)) return;
    if (!lmt_file_exists(lmt_path)) {
        spdlog::warn("Gerb: nativePC\\{}.lmt not found; the move is disabled until it is", lmt_path);
        s_failed_for = pl;
        return;
    }
    if (!devil4_sdk::load_motion_bank((void*)pl, lmt_path, lmt_bank)) { s_failed_for = pl; return; }
    spdlog::info("Gerb: '{}' -> bank {}", lmt_path, lmt_bank);
}

// ---------------------------------------------------------------------------------------------------------------
// The move.
// ---------------------------------------------------------------------------------------------------------------
void __stdcall Gerb::move(uintptr_t pl) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    JumpState& s = state_for(pl);

    switch (phase) {
    case PHASE_ENTRY: {
        ensure_bank(pl);
        if (!ready(pl)) { devil4_sdk::neutral_air_transition((uPlayer*)pl); return; }   // no LMT: bail out

        s.dir = pick_direction(pl);
        const glm::vec3 fwd = wmat_row(pl, 2), right = wmat_row(pl, 0);
        switch (s.dir) {
        case FORWARD: s.dir_vec = fwd;    break;
        case BACK:    s.dir_vec = -fwd;   break;
        case LEFT:    s.dir_vec = -right; break;
        case RIGHT:   s.dir_vec = right;  break;
        case UP:      s.dir_vec = glm::vec3(0.0f,  1.0f, 0.0f); break;
        case DOWN:    s.dir_vec = glm::vec3(0.0f, -1.0f, 0.0f); break;
        default: break;
        }
        if (face_direction && (s.dir == FORWARD || s.dir == LEFT || s.dir == RIGHT))   // a back jump keeps facing
            face(pl, yaw_of(s.dir_vec));

        borrow_ctn(pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
        // the cycle is the whole jump: loop as its block says, root translation off
        request_motion(pl, (lmt_bank << 8) | (slot[s.dir] & 0xFF), (uint32_t)A_NULL_TRANS_OFF, 4.0f, 0.0f, 1.0f);

        s.script_set = false;                                           // the cancel script goes in next tick
        vel.x = vel.y = vel.z = 0.0f;                                   // still until the launch frame
        acc.x = acc.y = acc.z = 0.0f;
        clear_add_speed(pl);
        phase = PHASE_WAIT;
        return;
    }
    case PHASE_WAIT: {
        acc.x = acc.y = acc.z = 0.0f;
        arm_cancel_script(s, pl);
        allow_attacks(pl);
        if (at<float>(pl, pl::mMotionFrame0) < launch_frame) return;

        glm::vec3 v;
        switch (s.dir) {
        case UP:   v = glm::vec3(0.0f,  up_speed,   0.0f); break;
        case DOWN: v = glm::vec3(0.0f, -down_speed, 0.0f); break;
        default:   v = s.dir_vec * speed + glm::vec3(0.0f, rise, 0.0f); break;
        }
        vel.x = v.x; vel.y = v.y; vel.z = v.z;
        // keep the game's own XZ model in step (mMoveVel / mJump.moveAng), no table deceleration, no additive speed
        const glm::vec3 xz(v.x, 0.0f, v.z);
        at<float>(pl, pl::mMoveVel) = glm::length(xz);
        at<float>(pl, pl::mMoveAcc) = 0.0f;
        if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = yaw_of(xz);
        clear_add_speed(pl);
        // From the take-off the action counts as a jump (AST_JUMP) so the game's air rules and the air rows of the
        // attack table apply; until then it keeps the status it was entered with (stand on the ground), so ground
        // rows can still cancel the wind-up.
        if (airborne_status) at<uint32_t>(pl, pl::mActStat) = (at<uint32_t>(pl, pl::mActStat) & ~0x1u) | 0x2u;
        phase = PHASE_AIRBORNE;
        return;
    }
    case PHASE_AIRBORNE: {
        g_dbg_n0_at_move  = at<int>(pl, pl::mMotCncl + 4);
        g_dbg_all_at_move = at<int>(pl, pl::mMotCncl + 0x2C * 7 + 4);
        arm_cancel_script(s, pl);
        allow_attacks(pl);
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && coll && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        vel.y += gravity * at<float>(pl, pl::m_delta_time);
        if (vel.y < -max_fall) vel.y = -max_fall;
        acc.x = acc.y = acc.z = 0.0f;
        return;
    }
    default:
        return;
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Mod plumbing. No hooks: MoveTable's dispatch calls Gerb::move.
// ---------------------------------------------------------------------------------------------------------------
std::optional<std::string> Gerb::on_initialize() {
    return Mod::on_initialize();
}

// The row MoveTable::updateKDATbl inserted (index from the front = extra_nero_moves - MoveTable::Gerb, the same
// arithmetic LuciAirThrow uses). Byte 0 of the command word = the ATTACK_DIRECTION_CMD, byte 1 = 1 makes the row
// unmatchable (the table's convention for a disabled custom move).
void Gerb::apply_row() {
    if (!HookNeroKADTbl || !MoveTable::Gerb) return;
    kAtckDefTbl* row = (kAtckDefTbl*)HookNeroKADTbl + (MoveTable::extra_nero_moves - MoveTable::Gerb);
    row->command.buffer = (uint32_t)(input_cmd & 0xFF) | (mod_enabled ? 0u : 0x100u);
}

static const struct { int id; const char* name; } kInputs[] = {
    { LOCKON_MELEE, "lock-on + melee" }, { LOCKON_GUN, "lock-on + gun" }, { LOCKON_STYLE, "lock-on + style" },
    { LOCKON_FORWARD_MELEE, "lock-on + forward + melee" }, { LOCKON_BACK_MELEE, "lock-on + back + melee" },
    { LOCKON_FORWARD_GUN, "lock-on + forward + gun" }, { LOCKON_BACK_GUN, "lock-on + back + gun" },
    { LOCKON_FORWARD_STYLE, "lock-on + forward + style" }, { LOCKON_BACK_STYLE, "lock-on + back + style" },
    { LOCKON_FORWARD_MELEE2, "lock-on + forward + melee (2)" }, { LOCKON_BACK_MELEE2, "lock-on + back + melee (2)" },
    { LOCKON_FORWARD_GUN2, "lock-on + forward + gun (2)" }, { LOCKON_BACK_GUN2, "lock-on + back + gun (2)" },
    { LOCKON_FORWARD_STYLE2, "lock-on + forward + style (2)" }, { LOCKON_BACK_STYLE2, "lock-on + back + style (2)" },
    { LOCKON_ANYDIRECTION_MELEE, "lock-on + any direction + melee" }, { LOCKON_ANYDIRECTION_GUN, "lock-on + any direction + gun" },
    { LOCKON_ANYDIRECTION_STYLE, "lock-on + any direction + style" }, { ANYDIRECTION_MELEE, "any direction + melee" },
    { ANYDIRECTION_GUN, "any direction + gun" }, { ANYDIRECTION_STYLE, "any direction + style" },
    { MELEE_STYLE, "melee + style" }, { LOCKON_BACKFORWARD_MELEE, "lock-on + back-forward + melee" },
};

void Gerb::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (uPlayer* pl = devil4_sdk::get_local_player()) ensure_bank((uintptr_t)pl);
}

void Gerb::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;        // debug page
    if (ImGui::Checkbox(_("Gerb"), &mod_enabled)) apply_row();
    ImGui::SameLine();
    help_marker(_("A free jump in any direction, no wall: forward / back / left / right by the stick, straight up with it "
                  "centred, straight down with it centred while a 'down button' is held. Needs motion\\pl000\\gerb.lmt "
                  "(slots 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up) and a MoveTable row whose dispatch calls Gerb::move."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    {   // the input: the row's ATTACK_DIRECTION_CMD
        int cur = 0;
        for (int i = 0; i < (int)(sizeof(kInputs) / sizeof(kInputs[0])); ++i) if (kInputs[i].id == input_cmd) cur = i;
        if (ImGui::BeginCombo(_("Input"), kInputs[cur].name)) {
            for (int i = 0; i < (int)(sizeof(kInputs) / sizeof(kInputs[0])); ++i) {
                if (ImGui::Selectable(kInputs[i].name, i == cur)) { input_cmd = kInputs[i].id; apply_row(); }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        help_marker(_("The command (ATTACK_DIRECTION_CMD) the move's table row triggers on. Its own input, separate from the jump "
                      "button; pick one no vanilla Nero move uses in the same state."));
        ImGui::InputInt(_("Input id (raw)"), &input_cmd);
        if (ImGui::IsItemDeactivatedAfterEdit()) apply_row();
    }
    ImGui::SliderInt(_("Bank"), &lmt_bank, 10, 14);
    ImGui::InputText(_("LMT"), lmt_path, sizeof(lmt_path));
    if (uPlayer* pl = devil4_sdk::get_local_player()) {
        ImGui::Text(_("File: %s | loaded: %s"), lmt_file_exists(lmt_path) ? "found" : "missing", ready((uintptr_t)pl) ? "yes" : "no");
        if (ImGui::Button(_("Reload LMT"))) devil4_sdk::unload_motion_bank(pl, lmt_bank);
    }
    ImGui::InputInt(_("Slot forward"), &slot[FORWARD]);
    ImGui::InputInt(_("Slot down"), &slot[DOWN]);
    ImGui::InputInt(_("Slot back"), &slot[BACK]);
    ImGui::InputInt(_("Slot left"), &slot[LEFT]);
    ImGui::InputInt(_("Slot right"), &slot[RIGHT]);
    ImGui::InputInt(_("Slot up"), &slot[UP]);
    ImGui::InputInt(_("Borrowed .ctn bank"), &ctn_bank);
    ImGui::InputInt(_("Borrowed .ctn slot"), &ctn_slot);
    ImGui::InputScalar(_("Down button (mBtnOn bits)"), ImGuiDataType_U32, &down_mask, nullptr, nullptr, "%X", ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("Hex mask of cInterface::mBtnOn (+0x140C). Held with the stick centred, the jump goes down instead of up. "
                  "0 = the down jump is never picked. 4 is the jump button."));
    ImGui::SliderFloat(_("Launch frame"), &launch_frame, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Speed"), &speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Rise"), &rise, -20.0f, 40.0f);
    ImGui::SliderFloat(_("Up speed"), &up_speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Down speed"), &down_speed, 0.0f, 60.0f);
    ImGui::SliderFloat(_("Gravity"), &gravity, -5.0f, 0.0f);
    ImGui::SliderFloat(_("Max fall"), &max_fall, 1.0f, 80.0f);
    ImGui::SliderFloat(_("Cancel frame"), &cancel_frame, -1.0f, 60.0f);
    ImGui::SameLine();
    help_marker(_("From this frame of the cycle every cancel window (melee, command, jump, taunt, gun, all) is open, so attacks and "
                  "jumps can interrupt the jump: a .ctn script made in memory replaces the borrowed one. -1 = never (the borrowed "
                  "script's windows apply)."));
    ImGui::SliderFloat(_("Side angle"), &side_angle, 10.0f, 80.0f);
    if (uPlayer* pl = devil4_sdk::get_local_player()) {      // live readout for debugging the cancel window
        const uintptr_t P = (uintptr_t)pl;
        ImGui::Text("actLimit %04X  actStat %02X  frame %.1f  phase %d  motion ctrl0 %04X ctrl1 %04X (half used: %s)",
                    at<uint32_t>(P, pl::mActLimit), at<uint32_t>(P, pl::mActStat), at<float>(P, pl::mMotionFrame0),
                    at<int>(P, pl::mBdyActionLowerPhase), at<uint32_t>(P, pl::mMotionCtrl0No) & 0xFFFF,
                    at<uint32_t>(P, pl::mMotionCtrl0No + 0x3C) & 0xFFFF,
                    (at<uint32_t>(P, pl::mMotionCtrl0No + 0x3C) & 0xFFFF) == 0xFFFF ? "lower = ctn[1]" : "upper = ctn[0]");
        ImGui::Text("inside move(): N0 %d ALL %d", g_dbg_n0_at_move, g_dbg_all_at_move);
        ImGui::Text("mStat N0 %d N1 %d N2 %d CMD %d JMP %d TNT %d GUN %d ALL %d",
                    at<int>(P, pl::mMotCncl + 0x2C * 0 + 4), at<int>(P, pl::mMotCncl + 0x2C * 1 + 4), at<int>(P, pl::mMotCncl + 0x2C * 2 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 3 + 4), at<int>(P, pl::mMotCncl + 0x2C * 4 + 4), at<int>(P, pl::mMotCncl + 0x2C * 5 + 4),
                    at<int>(P, pl::mMotCncl + 0x2C * 6 + 4), at<int>(P, pl::mMotCncl + 0x2C * 7 + 4));
        for (int half = 0; half < 2; ++half) {
            const uintptr_t mgr = P + pl::mCharTblMgr + 0x24 * half;
            const uint8_t* top = at<uint8_t*>(mgr, 0x0C); const uint8_t* cur = at<uint8_t*>(mgr, 0x10);
            const uint8_t* c1 = &at<uint8_t>(mgr, 0x15 + 6);
            ImGui::Text("ctn[%d] top %p cur +%d done %d ours %d  contact1 %02X %02X %02X %02X %02X %02X", half, top, cur && top ? (int)(cur - top) : -1,
                        (int)at<bool>(mgr, 0x14), (int)(top == (const uint8_t*)&g_cancel_scripts[0] || top == (const uint8_t*)&g_cancel_scripts[1]),
                        c1[0], c1[1], c1[2], c1[3], c1[4], c1[5]);
        }
    }
    ImGui::Checkbox(_("Face the direction"), &face_direction);
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Count as airborne"), &airborne_status);
    ImGui::Unindent(lineIndent);
}

void Gerb::on_config_load(const utility::Config& cfg) {
    mod_enabled  = cfg.get<bool>("gerb").value_or(false);
    input_cmd    = cfg.get<int>("gerb_input").value_or(input_cmd);
    apply_row();
    lmt_bank     = cfg.get<int>("gerb_bank").value_or(lmt_bank);
    for (int i = 0; i < DIR_COUNT; ++i) slot[i] = cfg.get<int>("gerb_slot_" + std::to_string(i)).value_or(slot[i]);
    ctn_bank     = cfg.get<int>("gerb_ctn_bank").value_or(ctn_bank);
    ctn_slot     = cfg.get<int>("gerb_ctn_slot").value_or(ctn_slot);
    down_mask    = (uint32_t)cfg.get<int>("gerb_down_mask").value_or(0);
    launch_frame = cfg.get<float>("gerb_launch_frame").value_or(launch_frame);
    speed        = cfg.get<float>("gerb_speed").value_or(speed);
    rise         = cfg.get<float>("gerb_rise").value_or(rise);
    up_speed     = cfg.get<float>("gerb_up_speed").value_or(up_speed);
    down_speed   = cfg.get<float>("gerb_down_speed").value_or(down_speed);
    gravity      = cfg.get<float>("gerb_gravity").value_or(gravity);
    max_fall     = cfg.get<float>("gerb_max_fall").value_or(max_fall);
    cancel_frame = cfg.get<float>("gerb_cancel_frame").value_or(cancel_frame);
    side_angle   = cfg.get<float>("gerb_side_angle").value_or(side_angle);
    face_direction  = cfg.get<bool>("gerb_face_direction").value_or(true);
    airborne_status = cfg.get<bool>("gerb_airborne_status").value_or(true);
}

void Gerb::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("gerb", mod_enabled);
    cfg.set<int>("gerb_input", input_cmd);
    cfg.set<int>("gerb_bank", lmt_bank);
    for (int i = 0; i < DIR_COUNT; ++i) cfg.set<int>("gerb_slot_" + std::to_string(i), slot[i]);
    cfg.set<int>("gerb_ctn_bank", ctn_bank);
    cfg.set<int>("gerb_ctn_slot", ctn_slot);
    cfg.set<int>("gerb_down_mask", (int)down_mask);
    cfg.set<float>("gerb_launch_frame", launch_frame);
    cfg.set<float>("gerb_speed", speed);
    cfg.set<float>("gerb_rise", rise);
    cfg.set<float>("gerb_up_speed", up_speed);
    cfg.set<float>("gerb_down_speed", down_speed);
    cfg.set<float>("gerb_gravity", gravity);
    cfg.set<float>("gerb_max_fall", max_fall);
    cfg.set<float>("gerb_cancel_frame", cancel_frame);
    cfg.set<float>("gerb_side_angle", side_angle);
    cfg.set<bool>("gerb_face_direction", face_direction);
    cfg.set<bool>("gerb_airborne_status", airborne_status);
}
