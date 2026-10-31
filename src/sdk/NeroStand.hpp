#pragma once

#include <cstdint>

// Nero's Devil Bringer stand: uPlNeroDevil (DX9), the model Nero's DT summons. Nero holds it at +0xCDF8 (mpDevilStand);
// the stand holds Nero (mpParent) and its arm model uPlWpRightHand (mpRightHand). It is a unit of its own on sUnit move
// line 0x0E, Nero is on 0x0D and the arm / sword on 0x14, and lines run in ascending order, so a value written in Nero's
// tick (the pre-check tick, uPlayer::main) is read by the stand's main the same frame.
//
// What the game does with it: mActionNo picks the state function (uPlNeroDevil::moveAction 0x829D60). Action 4 (Yamato)
// is the DT mimic: uPlayerNero::setRedQueenMotion 0x7E6DF0 sets it with mYamatoParam = { motion, attr, start frame } and
// moveActYamato 0x82A830 plays that motion on the stand while uPlNeroDevil::checkPlayerAction allows it (Nero in action
// 12 with a Red Queen attack id 9-0x29, table 0xE16458), waiting first for Nero's per-motion mimic event byte (+0xCDF4).
// Visibility is mTpPhase (updateTransparency 0x8295F0), and nearly every update / draw path is gated on mIsDevilTrigger
// (+0x22C9), which is Nero's DT state itself (uPlayerNero::isDevilTrigger 0x7E19E6 returns this byte).
//
// The bytes +0x22D3 and +0x22D9 are padding no instruction touches; GuardianDevil uses them (kShown, kMod).
namespace nero_stand {

// uPlNeroDevil offsets
constexpr uintptr_t mPos                   = 0x0030;   // MtVector3 (uModel)
constexpr uintptr_t mQuat                  = 0x0040;   // MtQuaternion
constexpr uintptr_t mMotionNo0             = 0x0334;   // u16, MotionArray[0].mMotionNo (the body's motion)
constexpr uintptr_t mFrame0                = 0x0348;   // float, MotionArray[0].mFrame
constexpr uintptr_t mpParent               = 0x1370;   // uPlayerNero*
constexpr uintptr_t mpRightHand            = 0x1374;   // uPlWpRightHand* (the arm: Buster / Snatch hit through it)
constexpr uintptr_t mpTarget               = 0x1378;   // cLockOnTarget* (setSnatchParam)
constexpr uintptr_t mMotionCtrl            = 0x2230;   // cMotionCtrl[2], 0x3C each (+0x14 motion, +0x20 start frame, +0x2A attr)
constexpr uintptr_t mActionNo              = 0x22A8;   // int, see Action
constexpr uintptr_t mYamatoMotNo           = 0x22B4;   // int, what action 4 plays
constexpr uintptr_t mYamatoAttr            = 0x22B8;   // u16, the motion attribute passed to setMotion
constexpr uintptr_t mYamatoStartFrm        = 0x22BC;   // float, read once at moveActYamato phase 0
constexpr uintptr_t mActPhase              = 0x22C0;   // int, per action
constexpr uintptr_t mTpPhase               = 0x22C4;   // int, see TpPhase
constexpr uintptr_t mIsTraceParent         = 0x22C8;   // u8: 1 = the stand copies Nero's position / rotation every tick
constexpr uintptr_t mIsDevilTrigger        = 0x22C9;   // u8: Nero's DT state
constexpr uintptr_t mIsDevilTriggerReserve = 0x22CA;   // u8: a DT change is pending (setAction with a non-zero action flips DT)
constexpr uintptr_t mIsAppearEnable        = 0x22D0;   // u8: read by updateTransparency phases 0 / 10
constexpr uintptr_t mIsKeepDT              = 0x22D2;   // u8
constexpr uintptr_t kShown                 = 0x22D3;   // mod byte (padding): DT reads retargeted here = mIsDevilTrigger || a mod attack shows the stand
constexpr uintptr_t kMod                   = 0x22D9;   // mod byte (padding): Mod bits, tested by GuardianDevil's hooks
constexpr uintptr_t mAnglY                 = 0x22E0;   // float, facing yaw
constexpr uintptr_t mBdyTransparency       = 0x22E8;   // float, 0..1

// uPlayerNero offsets the stand code needs
constexpr uintptr_t pl_mpDevilStand        = 0xCDF8;   // uPlNeroDevil*
constexpr uintptr_t pl_mMimicEvent         = 0xCDF4;   // u8, copied every tick from Nero's motion events (procBeforeMoveMotion 0x7E3F80)
constexpr uintptr_t pl_mFacingYaw          = 0x1210;   // float, copied into the stand's mAnglY
constexpr uintptr_t pl_mLockOnTarget       = 0x3080;   // cLockOnTarget*

enum Action : int { ACT_NONE = 0, ACT_APPEAR = 1, ACT_BUSTER = 2, ACT_SNATCH = 3, ACT_YAMATO = 4, ACT_EM_BLOCK = 7, ACT_ILLUSION_SWORD = 8 };
enum TpPhase : int { TP_HIDDEN = 0, TP_APPEAR_WAIT = 10, TP_FADE_IN = 20, TP_SHOWN = 30, TP_FADE_OUT = 40, TP_FADE_OUT_RUN = 41 };
enum ModBit : uint8_t { MOD_ATTACK = 1 /* a guardian attack runs */, MOD_HOLD = 2 /* the stand is held in place during EmBlock */,
                        MOD_PLACED = 4 /* the mod detached the stand and it hasn't faded out yet */ };

constexpr uintptr_t fn_setAction      = 0x829BE0;   // uPlNeroDevil::setAction: this = edi, action = eax
constexpr uintptr_t fn_setSnatchParam = 0x829CE0;   // uPlNeroDevil::setSnatchParam: ecx = cLockOnTarget*, dl = bool, stack = this, retn 4

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *(T*)(base + off); }

inline uintptr_t stand_of(uintptr_t player) { return player ? at<uintptr_t>(player, pl_mpDevilStand) : 0; }
inline uintptr_t hand_of(uintptr_t stand) { return stand ? at<uintptr_t>(stand, mpRightHand) : 0; }

// setAction(n): also resets the arm's action, the phase and the appear flag; with a DT reserve pending and n != 0 it
// flips DT, so callers check mIsDevilTriggerReserve first.
inline void set_action(uintptr_t stand, int action) {
    const uintptr_t fn = fn_setAction;
    __asm {
        pushad
        mov eax, action
        mov edi, stand
        call fn
        popad
    }
}

// setSnatchParam(target, false): the stand and its arm aim at the lock-on target (mpTarget / mpTargetActor).
inline void set_snatch_param(uintptr_t stand, uintptr_t target) {
    const uintptr_t fn = fn_setSnatchParam;
    __asm {
        pushad
        push stand
        xor edx, edx
        mov ecx, target
        call fn
        popad
    }
}

// mYamatoParam: what action 4 plays (motion, motion attribute, start frame).
inline void yamato_param(uintptr_t stand, int motion, uint16_t attr, float start_frame) {
    at<int>(stand, mYamatoMotNo) = motion;
    at<uint16_t>(stand, mYamatoAttr) = attr;
    at<float>(stand, mYamatoStartFrm) = start_frame;
}

} // namespace nero_stand
