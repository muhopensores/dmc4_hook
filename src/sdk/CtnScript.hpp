#pragma once

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include "Enums_Internal.hpp"   // CANCEL_NO

// In-memory .ctn (rCharTbl) scripts: the byte programs cCharTblMgr::update 0x42B8E0 runs per motion to drive the
// contact bytes (cancel windows, stage triggers, camera / pad vibration) by frame. A runtime-loaded motion bank has
// no script, so a mod builds one here and points the manager's cursors at it (devil4_sdk::install_ctn_script, on the
// tick after the motion request). The bytes must outlive the motion: keep the Script static, one per player slot.
// RE: Vibed/RE/chartbl_ctn_hang.md (the opcodes) and player_move_pipeline.md (the contact bytes).
//
// Opcodes:
//   00 mode .. ..  f32 frame   wait: mode 0 = until mFrame >= frame (or the motion's end when frame < 0), then go on
//                              in the same tick; mode 1 = the same test, then the script is done; other modes stall.
//   01 idx  b0 .. b5           set contact set `idx` (6 bytes): set 0 = {atck, free2, camVib, free1, free0, padVib},
//                              set 1 = the cancel windows (see cancels() for the byte encoding)
//   02 idx  .. ..              clear contact set `idx`
//   03 .. .. ..  u32 back      loop: cursor = cursor - back + 8, then return (one pass per tick)
//   FF                         end (what the vanilla scripts end with; dmc4_hook's interpreter guard treats an unknown
//                              opcode as "done")
//
//     static ctn::Script sc;                                      // or ctn::BasicScript<N> for more room
//     sc.reset().wait(10.0f).open({C_JUMP, C_ALL}).end();          // from frame 10 on, jump / everything can cancel
//     devil4_sdk::install_ctn_script(player, sc.data());
namespace ctn {

    enum ContactSet : uint8_t { CONTACT_MOVE = 0, CONTACT_CANCEL = 1 };
    enum CancelMode : uint8_t { CANCEL_OPEN = 0x10 /* mStat 2: the cancel is allowed now */, CANCEL_CLOSE = 0x20, CANCEL_RESERVE = 0x30 /* mStat 1: queued (mYoyakuAtckId) and started when allowed */ };

    // N bytes of room (the vanilla scripts run up to 825 bytes, Nero's grab banks; 617 outside them)
    template <size_t N>
    struct BasicScript {
        static constexpr size_t kCapacity = N;
        uint8_t bytes[kCapacity] = {};
        size_t  len = 0;
        bool    overflow = false;                             // a command didn't fit: the script ends before it (check after building)

        const uint8_t* data() const { return bytes; }
        size_t mark() const { return len; }                   // a position for loop_to()

        BasicScript& reset() { len = 0; overflow = false; std::memset(bytes, 0, sizeof bytes); return *this; }

        BasicScript& wait(float frame, uint8_t mode = 0) {
            uint8_t* p = emit(8);
            p[0] = 0x00; p[1] = mode; p[2] = p[3] = 0;
            std::memcpy(p + 4, &frame, 4);
            return *this;
        }
        // wait until the motion ends, then the script is done: a safe terminator even without the opcode guard
        BasicScript& hold() { return wait(-1.0f, 1); }

        BasicScript& contact(uint8_t set, const uint8_t info[6]) {
            uint8_t* p = emit(8);
            p[0] = 0x01; p[1] = set;
            std::memcpy(p + 2, info, 6);
            return *this;
        }
        BasicScript& contact(uint8_t set, uint8_t b0, uint8_t b1 = 0, uint8_t b2 = 0, uint8_t b3 = 0, uint8_t b4 = 0, uint8_t b5 = 0) {
            const uint8_t info[6] = { b0, b1, b2, b3, b4, b5 };
            return contact(set, info);
        }
        BasicScript& clear(uint8_t set) {
            uint8_t* p = emit(4);
            p[0] = 0x02; p[1] = set; p[2] = p[3] = 0;
            return *this;
        }
        // jump back to `pos` (a mark()), then return for this tick
        BasicScript& loop_to(size_t pos) {
            const uint32_t back = (uint32_t)(len + 8 - pos);
            uint8_t* p = emit(8);
            p[0] = 0x03; p[1] = p[2] = p[3] = 0;
            std::memcpy(p + 4, &back, 4);
            return *this;
        }
        BasicScript& end() { if (!overflow && len < kCapacity) bytes[len++] = 0xFF; return *this; }   // the byte emit() keeps free

        // one command copied as it is (8 or 4 bytes of another script: MoveDef's merge with a vanilla script)
        BasicScript& raw(const uint8_t* cmd, size_t n) { std::memcpy(emit(n), cmd, n); return *this; }

        // One contact command on set 1 for the given cancel categories with `mode`. The byte encoding
        // (uPlayer::updateContactInfoSub through uPlayer::cancelNoTbl): byte k's low nibble n names cancelNoTbl[k][n],
        // where row 0 = {C_NORM0, C_NORM1, C_NORM2} and row k >= 1 = {k + 2, ..., C_ALL}; so a normal goes in byte 0
        // with nibble = its number, and C_COMMAND .. C_ALL go in byte (c - 2) with nibble 0. The high nibble is the
        // mode; a 0 byte changes nothing. Only one of the three normals fits per command (byte 0): emit two commands
        // for more.
        BasicScript& cancels(CancelMode mode, const CANCEL_NO* which, size_t n) {
            uint8_t info[6] = { 0, 0, 0, 0, 0, 0 };
            for (size_t i = 0; i < n; ++i) {
                const CANCEL_NO c = which[i];
                if (c <= C_NORM2)      info[0] = (uint8_t)(mode | (uint8_t)c);
                else if (c <= C_ALL)   info[c - 2] = (uint8_t)mode;
            }
            return contact(CONTACT_CANCEL, info);
        }
        BasicScript& cancels(CancelMode mode, std::initializer_list<CANCEL_NO> which) { return cancels(mode, which.begin(), which.size()); }
        BasicScript& open(std::initializer_list<CANCEL_NO> which)    { return cancels(CANCEL_OPEN, which); }
        BasicScript& close(std::initializer_list<CANCEL_NO> which)   { return cancels(CANCEL_CLOSE, which); }
        BasicScript& reserve(std::initializer_list<CANCEL_NO> which) { return cancels(CANCEL_RESERVE, which); }
        BasicScript& open_all()  { return open({ C_NORM0, C_COMMAND, C_JUMP, C_CHOUHATSU, C_GUN, C_ALL }); }
        BasicScript& close_all() { return close({ C_NORM0, C_COMMAND, C_JUMP, C_CHOUHATSU, C_GUN, C_ALL }); }

    private:
        // a byte stays free for end(); a command that doesn't fit ends the script there and is written to a sink
        uint8_t* emit(size_t n) {
            static uint8_t sink[16];
            if (overflow || n > sizeof sink || len + n + 1 > kCapacity) {
                if (!overflow && len < kCapacity) bytes[len] = 0xFF;
                overflow = true;
                return sink;
            }
            uint8_t* p = bytes + len; len += n; return p;
        }
    };
    using Script = BasicScript<128>;                          // Gerb's / the dodge's short scripts

} // namespace ctn
