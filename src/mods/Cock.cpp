#include "Cock.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cResource.hpp"

static int x_pos = 0;
static int y_pos = 0;
static int x_scale = 0;
static int y_scale = 0;

struct sprMapHead : MtObject {
    unsigned __int16 version;
    unsigned __int8 sprListId;
    unsigned __int8 pad;
    unsigned int sprNum;
    unsigned int fontNum;
};

struct sprMap : MtObject {
    bool isDisp;
    unsigned __int16 sprId;
    unsigned __int16 rot;
    unsigned __int16 rotA;
    unsigned __int16 rotD;
    unsigned int attrFlag;
    unsigned int prio;
    unsigned __int16 srcAlpha;
    unsigned __int16 dstAlpha;
    unsigned __int16 opAlpha;
    MtColor color;
    MtColor colorOrg;
    MtPoint posHDTV;
    MtPoint posHDTVOrg;
    MtSize scaleHDTV;
    MtSize scaleHDTVOrg;
};
static_assert(sizeof(sprMap)==0x48);

struct ANM_HEADER {
    unsigned __int8 m_ID;
    unsigned __int8 m_version;
    unsigned __int16 m_flag;
    unsigned __int16 m_numSpriteGroup;
    unsigned __int16 m_numSequence;
};

struct ANM_SEQUENCE {
    unsigned __int16 m_iSpriteGroup;
    unsigned __int8 m_time;
    unsigned __int8 m_attribute;
    __int16 m_hotspotX;
    __int16 m_hotspotY;
};

struct ANM_SPRITE {
    unsigned __int8 m_texU;
    unsigned __int8 m_texV;
    unsigned __int8 m_ofsX;
    unsigned __int8 m_ofsY;
    unsigned __int16 m_CBA;
    unsigned __int16 m_flag;
    __int16 m_sizeW;
    __int16 m_sizeH;
    __int16 m_rot;
    unsigned __int16 m_flag2;
    __int16 m_scaleX;
    __int16 m_scaleY;
};

namespace sPrim {
    struct Material {
        unsigned __int32 type : 5;
        unsigned __int32 bs_idx : 11;
        unsigned __int32 tex_handle : 10;
        unsigned __int32 transform : 4;
        unsigned __int32 pad : 2;
        unsigned int attribute;
    };
}

struct __declspec(align(8)) ANM_WORK {
    unsigned __int8 mBe_flag;
    unsigned __int8 pad;
    unsigned __int16 mRot_y;
    int mPos_x;
    int mPos_y;
    int mPos_z;
    unsigned int mAttr;
    __int16 mCenter_x;
    __int16 mCenter_y;
    int mPrio;
    int mScale_x;
    int mScale_y;
    unsigned __int16 mSeq_num;
    int mUse_sub_seq;
    unsigned __int16 mFrame_top;
    unsigned __int16 mFrame_num;
    unsigned __int16 mNow_frame;
    unsigned __int16 mNow_spr_gp;
    int mTimer;
    MtColor mColor;
    sPrim::Material mMaterial;
    ANM_SEQUENCE* mpSeq;
};
static_assert(sizeof(ANM_WORK)==0x48);

struct cAnmSprData {
    void* __vftable;
    unsigned __int8* mpAnmSource;
    void* mpAnmResource;
    ANM_HEADER* mpAnmHead;
    ANM_SEQUENCE* mpAnmSeqData;
    ANM_SPRITE* mpAnmSprData;
    ANM_SEQUENCE** mppAnmSeqList;
    ANM_SPRITE** mppAnmSprList;
    unsigned int* mpPartsNumList;
    unsigned __int16* mpSeqGpNumList;
    unsigned __int16 mpSeqGpNum;
    int mIsEnable;
    int mIsSet;
};
static_assert(sizeof(cAnmSprData) == 0x34);


struct cSprAnm : MtObject {
    bool mIsOwnTexture;
    bool mIsOwnAnmData;
    cAnmSprData mAnmSprData;
    void* mpTexture;
    ANM_WORK* mpAnm_work;
    unsigned int mAnmWorkNum;
};
static_assert(sizeof(cSprAnm) == 0x48);

struct fontMap : MtObject {
    bool isDisp;
    unsigned __int16 fontId;
    __int16 listId;
    unsigned int prio;
    MtPoint posHDTV;
    MtPoint posHDTVOrg;
    MtSize size;
    MtSize sizeOrg;
};

struct rSprLayout : cResource {
    sprMapHead mSprMapHead;
    sprMap* mpSprMap;
    fontMap* mpFontMap;
};

struct uSprLayout : cUnit {
    rSprLayout* mpResource;
    sprMap* mpData;
    cSprAnm* mpSprAnm;
    fontMap* mpFontData;
};
static_assert(sizeof(uSprLayout) == 0x28);

struct __declspec(align(8)) uDevilCock : uSprLayout {
    uDevilCock* mpNextPtr;
    unsigned int mStatus;
    sprMap** mpDispSprMap;
};
static_assert(sizeof(uDevilCock) == 0x38);

struct uCockpitMgr : cUnit {
    uDevilCock* mpStartPtr;
    uDevilCock* mpEndPtr;
};
static_assert(sizeof(uCockpitMgr) == 0x20);


void Cock::on_gui_frame(int display) {
    sUnit* su = devil4_sdk::get_sUnit();
    ImGui::BeginGroup();
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::Text(_("Pos"));
    ImGui::SliderInt(_("x_pos"), &x_pos, -1000, 1000);
    ImGui::SameLine();
    ImGui::SliderInt(_("y_pos"), &y_pos, -1000, 1000);
    ImGui::Text(_("Scale"));
    ImGui::SliderInt(_("x_scale"), &x_scale, -1000, 1000);
    ImGui::SameLine();
    ImGui::SliderInt(_("y_scale"), &y_scale, -1000, 1000);
    ImGui::PopItemWidth();
    if (su != nullptr) {
        MoveLine* uiLine = &(su->mMoveLine[25]);
        if ((su != nullptr) && (uiLine->mTop !=  nullptr)) {
            uCockpitMgr* cockmgr = (uCockpitMgr*)(uiLine->mTop);
            uDevilCock* cock = cockmgr->mpStartPtr;
            if (cock != nullptr) {
                while (true) {
                    for (int i = 0; i < cock->mpResource->mSprMapHead.sprNum; i++) {
                        cock->mpData[i].posHDTV.x = cock->mpResource->mpSprMap[i].posHDTV.x + x_pos;
                        cock->mpData[i].posHDTV.y = cock->mpResource->mpSprMap[i].posHDTV.y + y_pos;
                        cock->mpData[i].scaleHDTV.w = cock->mpResource->mpSprMap[i].scaleHDTV.w + x_scale;
                        cock->mpData[i].scaleHDTV.h = cock->mpResource->mpSprMap[i].scaleHDTV.h + y_scale;
                    }
                    if (cock == cockmgr->mpEndPtr)
                        break;
                    else
                        cock = cock->mpNextPtr;
                }
            }
        }
    }
    ImGui::EndGroup();
} 