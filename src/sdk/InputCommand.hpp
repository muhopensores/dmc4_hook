#pragma once

#include <cstdint>

// Extra input commands: inputs the game's command table doesn't have (RE note player_move_pipeline.md, "Command
// layouts and a command of one's own").
//
// uPlayer::executeCommandCheck 0x805920 runs the 85 commands of uPlayer::commandTbl (0xC3E328, 16-byte heads) every
// tick, each through uPlayer::commandCheckSub with its own kSPlCmd state (mPlCmd[85] +0x2190), and sets bit CMD_ID
// of mCmdFlag[3] (+0x2184) for the ones that fire; a COMMAND row of the attack table then needs its bit. The table
// and the loop are fixed (mPlCmd[85] is followed by other fields), but the three flag words hold 96 bits, so a
// command of our own gets one of bits 85-95: the same checker, run from a hook right after executeCommandCheck,
// with our own state. A mod installs the hook once (Gerb does: install_hook_absolute(0x7A83B5, hook, &detour,
// &jmp_ret, 8)) and every mod registers its commands with add().
//
// A command is a kCmdHead with `num` kCmdData steps, each a button or a lever test with a time limit for reaching
// the next step. The game's own 0x52 (lock-on + back, forward + melee) is LEVER mode 1 lever 0 (stick centred, no
// limit), LEVER 1 / 4 (back) within 15 frames, LEVER 1 / 1 (forward) within 15, BTN mode 1 (mBtnTrg) button 1.
namespace input_commands {

    enum CMD_TYPE : uint32_t { CMD_TYPE_NONE = 0, CMD_TYPE_BTN = 1, CMD_TYPE_LEVER = 2, CMD_TYPE_TAME = 3, CMD_TYPE_RENDA = 4 };
    enum CND_TYPE : uint32_t { CND_NONE = 0, CND_POSE = 1 /* lock-on held, mBtnOn 0x10 */, CND_DASH = 2 /* 0x200 */, CND_LASER = 3 /* 0x400 */ };
    // BTN mode: 1 = mBtnTrg (pressed this tick), 2 = mBtnRel (released), 3 = mBtnOn (held). Buttons: 1 melee, 2 gun, 0x200 style.
    // LEVER mode: 1 = within 45 deg of the lock-on target's direction (else the facing), 2 = 70 deg, 3 = screen relative.
    // LEVER lever: 0 = stick centred (passes only while mAnlgPow is 0), 1 forward, 4 back, 15 any direction.
    struct kCmdData { uint32_t type; uint32_t btn; uint16_t mode; uint16_t lever; int16_t timer; uint16_t pad; };   // timer: frames allowed to reach the next step, -1 none
    struct kCmdHead { uint16_t num; uint16_t pad0; uint32_t condition; int16_t active; uint16_t pad1; const kCmdData* data; };   // active: frames the result stays true (the game: 1)
    struct kSPlCmd  { uint8_t step, status, pad0[2]; float activeF, timerF; uint8_t cnt, cmdFlag, pad1[2]; float tame; };
    static_assert(sizeof(kCmdData) == 16 && sizeof(kCmdHead) == 16 && sizeof(kSPlCmd) == 20, "command layouts");

    constexpr int FIRST_FREE_BIT = 85;   // 0-84 are the game's
    constexpr int LAST_BIT = 95;

    enum BtnMode : uint16_t { BTN_TRG = 1 /* pressed this tick */, BTN_REL = 2 /* released */, BTN_ON = 3 /* held */ };
    enum Button : uint32_t { BTN_MELEE = 0x1, BTN_GUN = 0x2, BTN_JUMP = 0x4, BTN_LOCKON = 0x10, BTN_STYLE = 0x200 };
    enum LeverMode : uint16_t { LEVER_TARGET_45 = 1 /* vs the lock-on target's direction (else the facing) */, LEVER_TARGET_70 = 2, LEVER_SCREEN = 3 /* vs the camera */ };
    enum Lever : uint16_t { LEVER_CENTRED = 0 /* passes only while mAnlgPow is 0 */, LEVER_FORWARD = 1, LEVER_BACK = 4, LEVER_ANY = 15 };

    // A command built step by step. Keep it static (the game reads the steps in place):
    //     static input_commands::Command cmd;
    //     cmd.condition(CND_POSE).lever(LEVER_CENTRED).lever(LEVER_BACK, LEVER_TARGET_45, 15).lever(LEVER_FORWARD, LEVER_TARGET_45, 15)
    //        .button(BTN_STYLE);
    //     input_commands::add(cmd.head(), 85, &mod_enabled);
    // = the game's 0x52 (lock-on + back, forward + melee) with the style button.
    struct Command {
        static constexpr int kMaxSteps = 8;
        kCmdData steps[kMaxSteps] = {};
        kCmdHead head_ = { 0, 0, CND_NONE, 1, 0, nullptr };
        int num = 0;

        Command& condition(uint32_t cnd) { head_.condition = cnd; return *this; }
        Command& active(int16_t frames)  { head_.active = frames; return *this; }   // frames the result stays true (the game: 1)
        // a button step: `timer` = frames allowed, once this step is reached, to reach the NEXT one (<= 0 = no limit; a
        // window that runs out starts the command over; unused on the last step). uPlayer::commandCheckSub 0x805A10
        Command& button(uint32_t btn, uint16_t mode = BTN_TRG, int16_t timer = 0) {
            return step(CMD_TYPE_BTN, btn, mode, 0, timer);
        }
        // a lever step (see Lever / LeverMode); `timer` as above. Calibur's 0x52: centred -1, back 15, forward 15, melee 0
        Command& lever(uint16_t lever, uint16_t mode = LEVER_TARGET_45, int16_t timer = -1) {
            return step(CMD_TYPE_LEVER, 0, mode, lever, timer);
        }
        Command& step(uint32_t type, uint32_t btn, uint16_t mode, uint16_t lever, int16_t timer) {
            if (num < kMaxSteps) { steps[num] = kCmdData{ type, btn, mode, lever, timer, 0 }; ++num; }
            return *this;
        }
        const kCmdHead* head() {        // the head over the steps so far (re-callable; the data pointer stays this object's)
            head_.num = (uint16_t)num; head_.data = steps; return &head_;
        }
    };

    // Registers `head` (it and its steps must outlive the program: make them static) to set mCmdFlag bit `bit`
    // (85-95) when it fires. `enabled`, when given, is read every tick and the command is skipped (state reset)
    // while it is false; `predicate`, when given, must hold for the player that tick as well (a condition the
    // game's steps can't express, e.g. the stick leaning sideways). Returns false when the bit is out of range or
    // already taken.
    typedef bool (*Predicate)(uintptr_t player);
    typedef bool (*PredicateCtx)(uintptr_t player, void* ctx);   // the same with a context pointer (one function, many commands)
    bool add(const kCmdHead* head, int bit, const bool* enabled = nullptr, Predicate predicate = nullptr);

    // uPlayer::commandCheckSub 0x805A10 (stdcall this, kSPlCmd*, kCmdHead*, kCmdData*; result in al; its helpers
    // read the player from esi, so it is called with esi = this too). 1 = the command fired this tick.
    int command_check_sub(uintptr_t player, kSPlCmd* state, const kCmdHead* head, const kCmdData* steps);

    // The hook's body: every registered command for `player`, with a state per (command, player). Called by the
    // detour for every uPlayer::main (players, NPC / boss Dante ...).
    void __stdcall run(uintptr_t player);

    // The same, with a context pointer handed back to the predicate (MoveDef: one predicate function for every move).
    bool add(const kCmdHead* head, int bit, const bool* enabled, PredicateCtx predicate, void* ctx);
    int  next_free_bit();            // the first of FIRST_FREE_BIT..LAST_BIT no command holds, -1 when none is left
    // ---- Per-tick callbacks on the player ------------------------------------------------------------------------
    // uPlayer::main 0x7A7DC0 runs a player's tick in this order (RE/player_move_pipeline.md, "The cancel gate" and
    // "Upper-body gun shots"):
    //   1. updateCharTbl (the .ctn scripts step: contact bytes, cancel windows)        0x7A7DF1
    //   2. updateMotCancel (a press is latched into the windows' reserve flags)        0x7A7DF8
    //   3. setInterface (mBtnOn / mBtnTrg / the stick for this tick)
    //   4. executeCommandCheck (the 85 input commands -> mCmdFlag bits)                0x7A83B0
    //      -> [PRE-CHECK TICK]  the commands registered with add(), then every add_tick() callback    0x7A83B5
    //   5. checkBeforeAction / checkAction (vtbl 147 / 148): checkAtck (the rows), the jump checks, pose begin / end,
    //      run begin ... only while mIsCancelCehckEnable is set; a match calls setAction* and the move starts
    //   6. moveAction 0x821520: moveActionLower (the lower body's state function, incl. MoveTable's dispatch of custom
    //      ids) THEN moveActionUpper (the upper body's: the gun shots, the gun pose)   0x7A844E
    //      -> [POST-ACTION TICK] every add_post_tick() callback                        0x7A8453
    //   7. checkAfterAction (vtbl 149), the process-speed controller, hit stop
    //   8. updateMotion 0x820F50: the motion controllers' requests are consumed (cMotionCtrl::update: motion, blend
    //      rate, attributes), a new motion resets the windows (initMotCancel) and picks its .ctn script
    //   9. updateCollision, moveMotion (the frame advances) ...
    //
    // The pre-check tick (4) is where a mod sees the player before any of this tick's decisions: what the previous
    // tick's state functions left in mActLimit / mActStat / the windows is what the checks in 5 will read, so this is
    // the place to re-assert a move's words (a move that must stay airborne while an upper-body action keeps
    // reloading the limits from its own table), to eat a press before checkAtck matches a row on it, or to set a
    // command bit by hand. The post-action tick (6) sees what both halves' state functions requested this tick and
    // runs before updateMotion consumes it: the place to rescale a blend rate or change a motion attribute a
    // vanilla function just set (its mIsBlendRateSetReq / mIsMotionSetReq flags say what was set this tick). Both run
    // for EVERY uPlayer that ticks (the player, NPC and boss copies, a co-op partner): key any state by the pointer
    // (a 4-slot least-recently-seen table, never "reset slot 0"), and keep the callbacks cheap, they run 60+ times a
    // second per player. A callback may call into the game (setActionAtck, requestSetMotion ...); it must not throw.
    //
    // Installation: the hooks are shared, one installer each. The pre-check hook at 0x7A83B5 replaces the 8 bytes
    // `mov edx, [edi]; mov eax, [edx+248h]` (edi = the player) with `detour` and continues at jmp_ret = 0x7A83BD;
    // the post-action hook at 0x7A8453 replaces `mov edx, [edi]; mov eax, [edx+254h]` (the checkAfterAction vtable
    // fetch) with `detour_post`, jmp_ret_post = 0x7A845B. A mod that needs one checks the installed flag first:
    //     if (!input_commands::hook_installed) { install_hook_absolute(0x7A83B5, hook, &input_commands::detour, &input_commands::jmp_ret, 8); input_commands::hook_installed = true; }
    //     input_commands::add_tick(&my_tick);
    // (Gerb and MoveDef do this for the pre-check hook; nothing installs the post-action hook today, its first user does.)
    // Callbacks run in registration order and can't be removed; register from on_initialize.
    typedef void (*TickFn)(uintptr_t player);
    bool add_tick(TickFn fn);            // pre-check tick: after executeCommandCheck, before checkBeforeAction
    bool add_post_tick(TickFn fn);       // post-action tick: after moveAction (both halves), before updateMotion
    extern bool hook_installed;          // the 0x7A83B5 hook is in place (its installer sets this)
    extern bool post_hook_installed;     // the 0x7A8453 hook is in place
    extern uintptr_t jmp_ret_post;       // 0x7A845B once installed
    void detour_post();

    // The hook: 0x7A83B5 in uPlayer::main, right after `call uPlayer::executeCommandCheck` (edi = this); the 8
    // bytes replaced are `mov edx, [edi]; mov eax, [edx+248h]`, so jmp_ret = 0x7A83BD.
    extern uintptr_t jmp_ret;
    void detour();

} // namespace input_commands
