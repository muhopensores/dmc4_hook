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

    // Registers `head` (it and its steps must outlive the program: make them static) to set mCmdFlag bit `bit`
    // (85-95) when it fires. `enabled`, when given, is read every tick and the command is skipped (state reset)
    // while it is false. Returns false when the bit is out of range or already taken.
    bool add(const kCmdHead* head, int bit, const bool* enabled = nullptr);

    // uPlayer::commandCheckSub 0x805A10 (stdcall this, kSPlCmd*, kCmdHead*, kCmdData*; result in al; its helpers
    // read the player from esi, so it is called with esi = this too). 1 = the command fired this tick.
    int command_check_sub(uintptr_t player, kSPlCmd* state, const kCmdHead* head, const kCmdData* steps);

    // The hook's body: every registered command for `player`, with a state per (command, player). Called by the
    // detour for every uPlayer::main (players, NPC / boss Dante ...).
    void __stdcall run(uintptr_t player);

    // The hook: 0x7A83B5 in uPlayer::main, right after `call uPlayer::executeCommandCheck` (edi = this); the 8
    // bytes replaced are `mov edx, [edi]; mov eax, [edx+248h]`, so jmp_ret = 0x7A83BD.
    extern uintptr_t jmp_ret;
    void detour();

} // namespace input_commands
