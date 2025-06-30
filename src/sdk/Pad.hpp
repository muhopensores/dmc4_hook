#pragma once
#include "MtMath.hpp"

struct PAD_FREE {
    schar Press_free;
    uchar Trigger_free;
    short Analog_free;
    short Analog_cross_free;
};

struct PAD_DATA {
    uint On;
    uint Old;
    uint Trg;
    uint Rel;
    uint Chg;
    uint Rep;
    int Rx;
    int Ry;
    int Lx;
    int Ly;
    uchar Rz;
    uchar Lz;
    undefined field12_0x2a;
    undefined field13_0x2b;
};

struct PAD_INFO {
    uchar Be_flag;
    uchar Rno;
    schar Pad_no;
    uchar Kind;
    uint Ability;
    uint Input_attr;
    uint Socket_no;
};

struct kBtnInfo {
    uint on;
    uint trg;
    uint rel;
    uint rep;
};

struct kPressInfo {
    float Lup;
    float Lright;
    float Ldown;
    float Lleft;
    float Rup;
    float Rright;
    float Rdown;
    float Rleft;
    float L1;
    float L2;
    float R1;
    float R2;
    float Select;
    float L3;
    float R3;
};

struct PAD_VIB_LIST {
    uchar Be_flag;
    uchar reserved;
    ushort Timer;
    int Milisec;
    int Value;
    int Add;
};

struct PAD_VIB {
    int Vib_data[2];
    uchar Vib_result;
    uchar Vib_zero_count;
    uchar Vib_retry_flag;
    uchar Vib_set_timer;
    struct PAD_VIB_LIST List_high[8];
    struct PAD_VIB_LIST List_low[8];
};

struct kAnlgInfo {
    float x;
    float y;
};

struct kVibInfo {
    bool mIsMove;
    bool mIsPause;
    undefined field2_0x2;
    undefined field3_0x3;
    int mType;
    int mTimeLeft;
    int mVib;
    int mEndVib;
    int mAdd;
};

struct kSensorInfo {
    float x;
    float y;
    float z;
    float g;
};

struct cPadInfo {
    undefined field0_0x0;
    undefined field1_0x1;
    undefined field2_0x2;
    undefined field3_0x3;
    struct Pad* mpPad;
    bool mIsEnable;
    bool mIsVibEnable;
    bool mIsFake;
    undefined field8_0xb;
    uint mDecideBtn;
    uint mCancelBtn;
    struct kBtnInfo mBtn;
    struct kAnlgInfo mAnlg[2];
    struct kPressInfo mPress;
    struct kSensorInfo mSensor;
    struct kVibInfo mVibInfo[16];
};

struct PAD_REPEAT {
    ushort Start;
    ushort Next;
    ushort Timer[24];
};

struct kAnlg {
    short x;
    short y;
    float len;
    float radian;
    uint dgtlOn;
    uint dgtlTrg;
    uint dgtlRel;
};

struct cPeripheral {
    void* vtable;
    uint mPadBtnOn;
    uint mPadBtnTrg;
    uint mPadBtnRel;
    float mPadBtnPress[15];
    struct kAnlg mAnlgL;
    struct kAnlg mAnlgR;
    struct kAnlg mHoldAnlgL;
    bool mIsHold;
    undefined field9_0x95;
    undefined field10_0x96;
    undefined field11_0x97;
    int mFakeTimer;
    float mAutoPadRollFrame[4];
    bool mIsFakeData;
    undefined field15_0xad;
    undefined field16_0xae;
    undefined field17_0xaf;
};

struct Pad {
    undefined field0_0x0[4];
    struct PAD_INFO Info;
    struct PAD_FREE Free;
    struct PAD_REPEAT Repeat;
    undefined field7_0x4e;
    undefined field8_0x4f;
    struct PAD_VIB field9_0x50;
    struct PAD_DATA field10_0x15c;
    bool mTriggerVibLow;
    bool mTriggerVibHigh;
    ushort mVibStartValue;
    ushort mVibEndValue;
    ushort mVibTime;
    undefined pad[304];
};
static_assert(sizeof(Pad) == 0x2C0);

struct sDevil4Pad: CSystem {
    bool mActive;
    undefined field33_0x21;
    undefined field34_0x22;
    undefined field35_0x23;
    undefined field36_0x24;
    undefined field37_0x25;
    undefined field38_0x26;
    undefined field39_0x27;
    undefined field40_0x28;
    undefined field41_0x29;
    undefined field42_0x2a;
    undefined field43_0x2b;
    undefined field44_0x2c;
    undefined field45_0x2d;
    undefined field46_0x2e;
    undefined field47_0x2f;
    undefined field48_0x30;
    undefined field49_0x31;
    undefined field50_0x32;
    undefined field51_0x33;
    struct Pad mPad[2];
    uint mDecideButton;
    bool mXInput;
    undefined field55_0x5b9;
    undefined field56_0x5ba;
    undefined field57_0x5bb;
    undefined field58_0x5bc;
    undefined field59_0x5bd;
    undefined field60_0x5be;
    undefined field61_0x5bf;
    undefined field62_0x5c0;
    undefined field63_0x5c1;
    undefined field64_0x5c2;
    undefined field65_0x5c3;
    undefined field66_0x5c4;
    undefined field67_0x5c5;
    undefined field68_0x5c6;
    undefined field69_0x5c7;
    undefined field70_0x5c8;
    undefined field71_0x5c9;
    undefined field72_0x5ca;
    undefined field73_0x5cb;
    undefined field74_0x5cc;
    undefined field75_0x5cd;
    undefined field76_0x5ce;
    undefined field77_0x5cf;
    struct cPadInfo mPadInfo[2];
    int mUserPadId;
    bool mIsFixUserPadId;
    undefined field81_0x9d5;
    undefined field82_0x9d6;
    undefined field83_0x9d7;
    undefined field84_0x9d8;
    undefined field85_0x9d9;
    undefined field86_0x9da;
    undefined field87_0x9db;
    undefined field88_0x9dc;
    undefined field89_0x9dd;
    undefined field90_0x9de;
    undefined field91_0x9df;
    float mDeltaTime;
    undefined field93_0x9e4;
    undefined field94_0x9e5;
    undefined field95_0x9e6;
    undefined field96_0x9e7;
};
static_assert(sizeof(sDevil4Pad)==0x9E8);

struct STATE {
    uint on[8];
    uint old[8];
    uint trg[8];
    uint release[8];
    uint change[8];
    uint rep[8];
    ULONG64 rep_timer[256];
};
static_assert(sizeof(STATE) == 0x8C0);

struct sKeyboard: CSystem {
    uchar mVKTable[256];
    struct STATE mState;
    uint mRepeatStartTime;
    uint mRepeatTime;
    struct MtString mKeyOn;
    struct MtString mKeyOld;
    struct MtString mKeyRelease;
    struct MtString mKeyRep;
    struct MtString mKeyTrg;
    struct MtString mKeyChange;
    bool mActive;
    undefined field43_0xa01;
    undefined field44_0xa02;
    undefined field45_0xa03;
    uintptr_t mpDInput;
    uintptr_t mpDevice;
    bool mPrintScreenEnable;
    bool mWindowsAccessability;
    undefined field50_0xa0e;
    undefined field51_0xa0f;
    struct tagSTICKYKEYS mStickyKeys;
    struct tagTOGGLEKEYS mToggleKeys;
    struct tagFILTERKEYS mFilterKeys;
    uint mSysMouseTrails;
    undefined field56_0xa3c;
    undefined field57_0xa3d;
    undefined field58_0xa3e;
    undefined field59_0xa3f;
};
static_assert(sizeof(sKeyboard) == 0xA40);