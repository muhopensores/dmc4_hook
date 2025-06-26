#pragma once

typedef signed char schar;
typedef unsigned char uchar, undefined;
typedef unsigned int uint;
typedef unsigned short ushort;

struct kSaveDataCommon {
    bool mIsMajin;
    bool mHaveWJump;
    bool mHaveEmJump;
    bool mHaveDash;
    bool mHaveOrbDraw;
    bool mHaveDtgGuageDown;
    uint mShortWeaponId;
    uint mLongWeaponId;
    int mHaveSkillNum;
    int mProudSoulCnt;
    uint mAtckFlag[8];
};
static_assert(sizeof(kSaveDataCommon) == 0x38);

struct kSaveDataDante {
    bool mHaveGmJump;
    bool mHaveEmHike;
    bool mHaveAirEsc;
    bool mIsStyleTs;
    bool mIsStyleRg;
    bool mIsStyleGs;
    bool mIsStyleSm;
    bool mIsStyleYt;
    bool mIsWeaponGm;
    bool mIsWeaponPd;
    bool mIsWeaponLf;
    int mStyleTsLv;
    int mStlyeRgLv;
    int mStyleGsLv;
    int mStyleSmLv;
    int mStyle;
};

struct kSaveDataNero {
    bool mHaveJustEscape;
    bool mHaveSearchSecretMission;
    bool mHaveEnemyBlock;
    bool mHaveJustFollowThrough;
    schar mLongGrabLv;
    schar mExceedLv;
    schar mJustEscapeLv;
};

struct kButtonData {
    uint mActBtn[17];
    uint mActBtnKB[25];
};

struct kDanteData {
    kSaveDataCommon mCommon;
    kSaveDataDante mProper;
    kButtonData mBtn;
};

struct kNeroData {
    kSaveDataCommon mCommon;
    kSaveDataNero mProper;
    kButtonData mBtn;
};
