#pragma once

#include <cstddef>
#include <cstdint>
#include "cUnit.hpp"
#include "cResource.hpp"
#include "MtMath.hpp"

// The game's menus (DX9; Vibed/RE/menus.md, title_screen.md, practice_mode.md; plan Vibed/Menus/README.md).
// Field names and order from the SE PDB; offsets are DX9's (cUnit 0x18, cResource 0x60), and every class's size
// matches DX9's DTI size: uListSelect 0x6C, uTitle 0x298, uTitlePc 0x74, uPauseBase 0x90, uPauseMenu 0xFC,
// uPausePractice 0xB8, uPracPlSet 0x80. A menu = a uListSelect subclass + its .sprmap layout (sprMap sprites,
// fontMap labels) + message files in sMessage slots (labels: message `fontId` of slot `listId + 4`).

class rTexture;

// ---- layout data (rSprLayout, the .sprmap; XFS v5: dmc4xml reads / writes it) ----

struct sprMap {                     // a sprite of a layout (0x48)
    void* vtable;                   // 0x00 MtObject
    bool isDisp;                    // 0x04 shown
    char pad_05;
    uint16_t sprId;                 // 0x06 frame in the layout's .anm (rSprAnm)
    uint16_t rot;                   // 0x08
    uint16_t rotA;                  // 0x0A
    uint16_t rotD;                  // 0x0C
    char pad_0e[2];
    uint32_t attrFlag;              // 0x10
    uint32_t prio;                  // 0x14
    uint16_t srcAlpha;              // 0x18
    uint16_t dstAlpha;              // 0x1A
    uint16_t opAlpha;               // 0x1C
    char pad_1e[2];
    MtColor color;                  // 0x20 (a = +0x23: the menus fade by it)
    MtColor colorOrg;               // 0x24
    MtPoint posHDTV;                // 0x28 position, HDTV coordinates
    MtPoint posHDTVOrg;             // 0x30
    MtSize scaleHDTV;               // 0x38
    MtSize scaleHDTVOrg;            // 0x40
};
static_assert(sizeof(sprMap) == 0x48);
static_assert(offsetof(sprMap, color) == 0x20 && offsetof(sprMap, posHDTV) == 0x28);

struct fontMap {                    // a text label of a layout (0x30)
    void* vtable;                   // 0x00
    bool isDisp;                    // 0x04
    char pad_05;
    uint16_t fontId;                // 0x06 message number
    int16_t listId;                 // 0x08 sMessage slot - 4 (< 0: not drawn)
    char pad_0a[2];
    uint32_t prio;                  // 0x0C
    MtPoint posHDTV;                // 0x10
    MtPoint posHDTVOrg;             // 0x18
    MtSize size;                    // 0x20
    MtSize sizeOrg;                 // 0x28
};
static_assert(sizeof(fontMap) == 0x30);

struct sprMapHead {                 // 0x10
    void* vtable;
    uint16_t version;               // 0x04
    uint8_t sprListId;              // 0x06
    uint8_t pad_07;
    uint32_t sprNum;                // 0x08
    uint32_t fontNum;               // 0x0C
};
static_assert(sizeof(sprMapHead) == 0x10);

class rSprLayout : public cResource {   // .sprmap
public:
    sprMapHead mSprMapHead;         // 0x60 (sprNum 0x68, fontNum 0x6C: read by the menus' loops)
    sprMap* mpSprMap;               // 0x70
    fontMap* mpFontMap;             // 0x74
};
static_assert(sizeof(rSprLayout) == 0x78);

// ---- sprite frames: the .anm (rSprAnm) behind a layout (types from mods/Cock.cpp, checked on the pause and Mission
// Menu sheets 2026-11-19). A sprite's sprId is a sprite group; group n's first part is mppAnmSprList[n]: its rectangle on
// the sheet is (texU + 256 * (flag2 & 3), texV + 256 * ((flag2 >> 2) & 3), sizeW, sizeH). In the file: an 8-byte header,
// then per group a u32 part count and its ANM_SPRITEs. ----

struct ANM_HEADER {
    uint8_t m_ID;                   // 0x0 (0x21)
    uint8_t m_version;              // 0x1 (4)
    uint16_t m_flag;                // 0x2
    uint16_t m_numSpriteGroup;      // 0x4: the frames a sprId can name
    uint16_t m_numSequence;         // 0x6
};
static_assert(sizeof(ANM_HEADER) == 0x8);

struct ANM_SEQUENCE {
    uint16_t m_iSpriteGroup;        // 0x0
    uint8_t m_time;                 // 0x2
    uint8_t m_attribute;            // 0x3
    int16_t m_hotspotX;             // 0x4
    int16_t m_hotspotY;             // 0x6
};
static_assert(sizeof(ANM_SEQUENCE) == 0x8);

struct ANM_SPRITE {
    uint8_t m_texU;                 // 0x0 low byte of x on the sheet (flag2 bits 0-1 = x / 256)
    uint8_t m_texV;                 // 0x1 low byte of y (flag2 bits 2-3 = y / 256)
    uint8_t m_ofsX;                 // 0x2 the anchor (signed in practice: 0xD8 = -40)
    uint8_t m_ofsY;                 // 0x3
    uint16_t m_CBA;                 // 0x4
    uint16_t m_flag;                // 0x6
    int16_t m_sizeW;                // 0x8
    int16_t m_sizeH;                // 0xA
    int16_t m_rot;                  // 0xC
    uint16_t m_flag2;               // 0xE
    int16_t m_scaleX;               // 0x10 (0x1000 = 1)
    int16_t m_scaleY;               // 0x12
};
static_assert(sizeof(ANM_SPRITE) == 0x14);

struct cAnmSprData {
    void* vtable;                   // 0x00
    uint8_t* mpAnmSource;           // 0x04 the .anm file image
    void* mpAnmResource;            // 0x08 its rSprAnm
    ANM_HEADER* mpAnmHead;          // 0x0C
    ANM_SEQUENCE* mpAnmSeqData;     // 0x10
    ANM_SPRITE* mpAnmSprData;       // 0x14
    ANM_SEQUENCE** mppAnmSeqList;   // 0x18
    ANM_SPRITE** mppAnmSprList;     // 0x1C per sprite group, its first part
    uint32_t* mpPartsNumList;       // 0x20
    uint16_t* mpSeqGpNumList;       // 0x24
    uint16_t mpSeqGpNum;            // 0x28
    char pad_2a[2];
    int mIsEnable;                  // 0x2C
    int mIsSet;                     // 0x30
};
static_assert(sizeof(cAnmSprData) == 0x34);

namespace sPrim {
struct Material {
    uint32_t type : 5;
    uint32_t bs_idx : 11;
    uint32_t tex_handle : 10;
    uint32_t transform : 4;
    uint32_t pad : 2;
    uint32_t attribute;             // 0x4
};
static_assert(sizeof(Material) == 0x8);
} // namespace sPrim

struct alignas(8) ANM_WORK {        // a playing sequence of a cSprAnm
    uint8_t mBe_flag;               // 0x00
    uint8_t pad;                    // 0x01
    uint16_t mRot_y;                // 0x02
    int mPos_x;                     // 0x04
    int mPos_y;                     // 0x08
    int mPos_z;                     // 0x0C
    uint32_t mAttr;                 // 0x10
    int16_t mCenter_x;              // 0x14
    int16_t mCenter_y;              // 0x16
    int mPrio;                      // 0x18
    int mScale_x;                   // 0x1C
    int mScale_y;                   // 0x20
    uint16_t mSeq_num;              // 0x24
    char pad_26[2];
    int mUse_sub_seq;               // 0x28
    uint16_t mFrame_top;            // 0x2C
    uint16_t mFrame_num;            // 0x2E
    uint16_t mNow_frame;            // 0x30
    uint16_t mNow_spr_gp;           // 0x32
    int mTimer;                     // 0x34
    MtColor mColor;                 // 0x38
    sPrim::Material mMaterial;      // 0x3C
    ANM_SEQUENCE* mpSeq;            // 0x44
};
static_assert(sizeof(ANM_WORK) == 0x48);

struct cSprAnm {
    void* vtable;                   // 0x00 (MtObject)
    bool mIsOwnTexture;             // 0x04
    bool mIsOwnAnmData;             // 0x05
    char pad_06[2];
    cAnmSprData mAnmSprData;        // 0x08
    rTexture* mpTexture;            // 0x3C
    ANM_WORK* mpAnm_work;           // 0x40 [mAnmWorkNum]
    uint32_t mAnmWorkNum;           // 0x44
};
static_assert(sizeof(cSprAnm) == 0x48);

// ---- the unit side ----

struct uSprLayout : cUnit {         // 0x28
    rSprLayout* mpResource;         // 0x18
    sprMap* mpData;                 // 0x1C this unit's copy of the sprites (0x48 apart)
    cSprAnm* mpSprAnm;              // 0x20 the .anm frames + texture
    fontMap* mpFontData;            // 0x24 its copy of the labels (0x30 apart)
};
static_assert(sizeof(uSprLayout) == 0x28);

struct uDevilCock : uSprLayout {    // 0x34: every HUD / menu sprite layout
    uDevilCock* mpNextPtr;          // 0x28
    uint32_t mStatus;               // 0x2C
    sprMap** mpDispSprMap;          // 0x30
};
static_assert(sizeof(uDevilCock) == 0x34);

struct uButton : uDevilCock {       // 0x38
    int mType;                      // 0x34
};
static_assert(sizeof(uButton) == 0x38);

struct uListSelect : uDevilCock {   // 0x6C: the base of every cursor menu (105 classes)
    int mStatus;                    // 0x34
    int mSelAng;                    // 0x38
    int mSelMax;                    // 0x3C rows
    int mCursor;                    // 0x40
    float mScale;                   // 0x44
    MtColor mColor;                 // 0x48
    int mDecoRno;                   // 0x4C
    float mDecoTimer;               // 0x50
    bool mIsResourceCk;             // 0x54
    char pad_55[3];
    int mDecideBtn;                 // 0x58
    int mCancelBtn;                 // 0x5C
    int mStartBtn;                  // 0x60
    int* mpSelBtn;                  // 0x64
    uButton* mpMouseBtn;            // 0x68
};
static_assert(sizeof(uListSelect) == 0x6C);
static_assert(offsetof(uListSelect, mCursor) == 0x40);

// uListSelect's vtable (DX9 0xBC50E0, 44 slots; title_screen.md maps them to SE's names). Every slot is thiscall on
// the menu. The owner of a menu creates it, polls isEnd, reads getDecValue, kills it.
struct uListSelect_vtbl {
    void* (__thiscall* dtor)(void* self, unsigned flags);           // 0x00
    void* createUI;                                                 // 0x04
    void* isEnableInstance;                                         // 0x08
    void (__thiscall* createProperty)(void* self, void* list);      // 0x0C
    void* (__thiscall* getDTI)(void* self);                         // 0x10
    void (__thiscall* setup)(void* self);                           // 0x14
    void (__thiscall* move)(void* self);                            // 0x18 the state machine
    void* cunit_1c;                                                 // 0x1C
    void* cunit_20;                                                 // 0x20
    void (__thiscall* draw)(void* self, void* draw);                // 0x24
    void* cunit_28;                                                 // 0x28
    void* cunit_2c;                                                 // 0x2C
    void (__thiscall* kill)(void* self);                            // 0x30
    void (__thiscall* loadResource)(void* self);                    // 0x34 its layout
    void* cunit_38;                                                 // 0x38
    void* cunit_3c;                                                 // 0x3C
    void (__thiscall* init)(void* self);                            // 0x40
    void (__thiscall* main)(void* self);                            // 0x44
    void (__thiscall* delData)(void* self);                         // 0x48
    void (__thiscall* transMain)(void* self, void* draw);           // 0x4C drawing
    bool (__thiscall* isEnd)(void* self);                           // 0x50
    int (__thiscall* getDecValue)(void* self);                      // 0x54 the result
    int (__thiscall* slot22)(void* self);                           // 0x58
    void (__thiscall* moveCursor)(void* self);                      // 0x5C
    void (__thiscall* selectMove)(void* self);                      // 0x60
    void (__thiscall* result)(void* self);                          // 0x64
    void (__thiscall* resultCancel)(void* self);                    // 0x68
    void (__thiscall* exit)(void* self);                            // 0x6C
    void (__thiscall* slot28)(void* self);                          // 0x70
    bool (__thiscall* isDecide)(void* self);                        // 0x74 input
    void (__thiscall* decide)(void* self);                          // 0x78
    bool (__thiscall* isCancel)(void* self);                        // 0x7C
    void (__thiscall* cancel)(void* self);                          // 0x80
    bool (__thiscall* isStart)(void* self);                         // 0x84
    bool (__thiscall* isRepeatL1)(void* self);                      // 0x88
    bool (__thiscall* isRepeatR1)(void* self);                      // 0x8C
    bool (__thiscall* isYButton)(void* self);                       // 0x90
    bool (__thiscall* isDecideMouse)(void* self);                   // 0x94
    bool (__thiscall* isDecideClick)(void* self, bool*, int, int);  // 0x98
    bool (__thiscall* isDecideClick2)(void* self, bool*, int, int); // 0x9C
    bool (__thiscall* isCancelMouse)(void* self);                   // 0xA0
    bool (__thiscall* isStartMouse)(void* self);                    // 0xA4
    void (__thiscall* setDecideClickCursor)(void* self);            // 0xA8
    bool (__thiscall* isMouseHitSelCursor)(void* self, sprMap*, uButton*, int, int); // 0xAC
};
static_assert(sizeof(uListSelect_vtbl) == 44 * 4);

// ---- the title (aTitle owns one uTitle; getDecValue 1 game, 2 benchmark, 3 quit, else attract demo) ----

struct uTitleA : uDevilCock {};     // the logo layouts (also uTitleB / uTitleC)
static_assert(sizeof(uTitleA) == 0x34);

struct uTitlePc : uListSelect {     // the PC menu: start, PC settings, benchmark, quit
    int mCursorPc;                  // 0x6C (SE mCursor)
    int mLoop;                      // 0x70
};
static_assert(sizeof(uTitlePc) == 0x74);

struct uTitle : uListSelect {       // 0x298; state byte cUnit+0x14 (title_screen.md: 16 states)
    void* mpArchive;                // 0x6C
    rTexture* mpTex;                // 0x70
    rTexture* mpTex2;               // 0x74
    rTexture* mpCryTex;             // 0x78
    void* mpStream;                 // 0x7C
    void* mpSe;                     // 0x80
    uDevilCock* mpOption;           // 0x84
    void* mpScheduler;              // 0x88
    void* mpSave;                   // 0x8C
    uTitleA mTitleA;                // 0x90
    uTitleA mTitleB;                // 0xC4 (uTitleB)
    uTitleA mTitleC;                // 0xF8 (uTitleC)
    uTitlePc mTitlePc;              // 0x12C
    float mFTimer0;                 // 0x1A0
    float mFTimer1;                 // 0x1A4
    int mMenuNo;                    // 0x1A8 the game menu's cursor: 0 new game, 1 load, 2 options
    uint8_t mBright;                // 0x1AC
    uint8_t mBright2;               // 0x1AD
    char mInitType;                 // 0x1AE
    char mCurNewGame[4];            // 0x1AF (0x1B0: toGame's difficulty)
    char pad_1b3;
    uint16_t mMessTbl[91];          // 0x1B4
    char pad_26a[2];
    int mFriendCtr;                 // 0x26C
    bool mIsLoad;                   // 0x270
    char pad_271[3];
    MtColor mWhiteColor;            // 0x274
    MtColor mBlackColor;            // 0x278
    MtColor mCryTexColor;           // 0x27C
    float mCryTexMul;               // 0x280
    int mAlpha;                     // 0x284
    int mGraphicID;                 // 0x288
    bool mDispTex2Del;              // 0x28C
    char pad_28d[3];
    float mShiftX;                  // 0x290
    uDevilCock* mpPcMess;           // 0x294 the quit prompt
};
static_assert(sizeof(uTitle) == 0x298);
static_assert(offsetof(uTitle, mTitlePc) == 0x12C && offsetof(uTitle, mMenuNo) == 0x1A8);
static_assert(offsetof(uTitle, mAlpha) == 0x284 && offsetof(uTitle, mpPcMess) == 0x294);

// ---- pause menus (and the cut practice ones: practice_mode.md) ----

struct uPauseBase : uListSelect {   // 0x90
    float mWinDecoTime;             // 0x6C
    float mWinTime;                 // 0x70
    uint32_t mMiniWinTime;          // 0x74
    uint32_t mBGTime;               // 0x78
    bool mTitleFinish;              // 0x7C
    bool mMiniFinish;               // 0x7D
    bool mButtonFinish;             // 0x7E
    char pad_7f;
    float mExpandClear;             // 0x80
    float mBackScaleH;              // 0x84
    uint8_t mBackColor;             // 0x88
    uint8_t mButtonClear;           // 0x89
    char pad_8a[2];
    rTexture* mpBackTex;            // 0x8C
};
static_assert(sizeof(uPauseBase) == 0x90);

struct uPauseMenu : uPauseBase {    // 0xFC: the pause menu
    float mTimer0;                  // 0x90
    float mOffsetTime;              // 0x94
    float mItemTime;                // 0x98
    float mSkillTime;               // 0x9C
    float mMapTime;                 // 0xA0
    float mSystemTime;              // 0xA4
    float mEndTime;                 // 0xA8
    int mLowRno;                    // 0xAC
    int mLastLowRno;                // 0xB0
    uButton mButton;                // 0xB4
    bool mMenuFinish;               // 0xEC
    bool mChildEnd;                 // 0xED
    bool mChildkill;                // 0xEE
    char pad_ef;
    uDevilCock* mpChild;            // 0xF0 the open sub-menu
    bool mMenuEnable[5];            // 0xF4
    char pad_f9[3];
};
static_assert(sizeof(uPauseMenu) == 0xFC);
static_assert(offsetof(uPauseMenu, mpChild) == 0xF0);

struct uPausePractice : uPauseBase {   // 0xB8, cut: no input in DX9; 7 entries
    float mTimer0;                  // 0x90
    float mOffsetTime;              // 0x94
    float mPlSetTime;               // 0x98 Player Setup
    float mEmSetTime;               // 0x9C Enemy Setup
    float mOptionTime;              // 0xA0 Options
    float mSkillTime;               // 0xA4 Skill List
    float mEmSelectTime;            // 0xA8 Enemy Select
    float mPracEndTime;             // 0xAC End Practice
    float mEndTime;                 // 0xB0
    bool mMenuFinish;               // 0xB4
    char pad_b5[3];
};
static_assert(sizeof(uPausePractice) == 0xB8);

struct uPracPlSet : uListSelect {   // 0x80, cut: the practice player setup
    float mTitleTime;               // 0x6C
    float mExpandClear;             // 0x70
    int mHpGaugeType;               // 0x74 row 0 toggle
    int mDtGaugeType;               // 0x78 row 1 toggle
    bool mTitleFinish;              // 0x7C
    char pad_7d[3];
};
static_assert(sizeof(uPracPlSet) == 0x80);

// ---- text (sMessage; 12 slots of 0x340 bytes) ----

namespace menus {

constexpr uintptr_t kMessage = 0xE558BC;            // sMessage* (sMessage::mpInstance)
constexpr uintptr_t kMessagePrintf = 0x49A990;      // cdecl printf(sMessage*, u8 slot, MtPoint*, MtSize*, MtColor, int prio, fmt, ...)
constexpr uintptr_t kMessageCreate = 0x497290;      // thiscall createMessage(sMessage*, const char* path, u8 slot, rTexture*)
constexpr int kMessageSlots = 12;
constexpr uintptr_t kSlotStride = 0x340;
constexpr uintptr_t kSlotEnabled = 0x2F4;           // bool, sMessage + slot * 0x340 + 0x2F4 (printf's check)
constexpr int kDebugFontSlot = 11;                  // id\message\msg_debug (dmc4_cmn, ASCII 0x20-0x7A, 12 x 24 cells),
                                                    // loaded at startup: the default slot for print
constexpr int kSystemFontSlot = 7;                  // id\sysFont\sys (rom\id\sysFont.arc, 16 x 32): only while the
                                                    // screens that load it are up (area name, benchmark, PC screens)

// A slot holds a message file the game can draw from. Slots in use by the game: 2 game, 3 room, 4+ layout labels,
// 5 title / 7 / 8 the system font (id\sysfont\sys), 6 mission start, 11 the debug font (loaded at startup).
bool slot_loaded(int slot);

// Draws ASCII text with sMessage::printf in `slot`'s font (that slot's first message must be an ASCII table: the
// system font or the debug font). HDTV coordinates; `w` / `h` = one character's size (the game uses 28 x 28); `\n`
// starts a new line. Game thread, every frame it should show (it's drawn for one frame). Returns false if the slot
// isn't loaded.
// `color`: printf reads the four bytes as B, G, R, A, so MtColor's fields named r and b hold blue and red here
// (seen in a screenshot, 2026-11-18: {r 0xFF, g 0xD0, b 0x40} drew light blue).
bool print(int slot, int x, int y, unsigned w, unsigned h, MtColor color, int prio, const char* text);

// The menus' font (moji00, the small-caps serif of the Mission Menu's descriptions and button prompts) for print: an
// ASCII table in moji00 glyphs (Vibed\Menus\tools\make_ascii_moji00.py), made in memory as an rMessage once, put into
// `slot` with the moji00 sheet whichever slot already holds it (the menus load it). Only a slot that's empty or already
// ours is taken. Game thread; call it every frame you print (cheap once loaded). False while no slot has the sheet.
// printf's advance at char width w: adv * w / 128 + 1, and moji00's advances are quarter pixels at 32, so w = h = 32
// is the font's own size. Characters moji00 lacks (# * < > \ ^) are spaces.
constexpr int kMenuFontSlot = 10;                   // empty in the Mission Menu (2, 4, 5, 11 in use there)
bool load_menu_font(int slot = kMenuFontSlot);
bool menu_font_ready(int slot = kMenuFontSlot);

// A label font: the menus' slanted metal lettering (the pause / option labels: capitals and small caps) as a message
// file + its sheet, `<path>.msg` and `<path>.tex` (loose files under nativePC; made by the practice mode's
// tools\font\make_game_font.py from letters cut out of the game's own sheets). Message 0 is an ASCII table as print
// wants; message 1 holds, per character 0x20-0x7E, the pen advance (code, px) and how far left of the pen its cell
// starts (advance byte, px). The letters lean over their neighbours and printf cuts each character to its own width, so
// print_label draws them one at a time. Upper case = capitals, lower case = small caps ("D.t. Gauge").
// load_label_font loads both files (synchronously, once per path) and puts them into `slot` if it's empty or already
// theirs; game thread, call it every frame you print. False if the files aren't there or the slot is the game's.
constexpr int kLabelFontSlot = 9;                   // empty in the pause menu and the Mission Menu
bool load_label_font(const char* path, int slot = kLabelFontSlot);
int label_width(const char* text, int slot = kLabelFontSlot);   // px, at the font's own size
// text with its first letter's pen at x and the 40-px lettering row's top at y (the row's middle at y + 20, as the
// pause sheet's 40-px label frames), at the font's own size; HDTV coordinates
bool print_label(int x, int y, MtColor color, int prio, const char* text, int slot = kLabelFontSlot);

// A layout's sprite frame (sprite group `frame` of its .anm, the first part), nullptr past the .anm's groups. A sprite
// shows frame n with sprMap::sprId = n; frames are shared by every unit using that .anm (edit them with care).
inline ANM_SPRITE* anm_sprite(const uSprLayout* layout, int frame) {
    if (!layout || !layout->mpSprAnm || frame < 0) return nullptr;
    const cAnmSprData& d = layout->mpSprAnm->mAnmSprData;
    if (!d.mpAnmHead || !d.mppAnmSprList || frame >= d.mpAnmHead->m_numSpriteGroup) return nullptr;
    return d.mppAnmSprList[frame];
}

} // namespace menus
