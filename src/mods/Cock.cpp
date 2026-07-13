#include "Cock.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sUnit.hpp"
#include "../sdk/cResource.hpp"

struct sprMapHead : MtObject {
    unsigned __int16 version; // 0x4
    unsigned __int8 sprListId; // 0x6
    unsigned __int8 pad; // 0x7
    unsigned int sprNum; // 0x8
    unsigned int fontNum; // 0xc
};
static_assert(sizeof(sprMapHead) == 0x10);

struct sprMap : MtObject {
    unsigned __int16 rot;
    bool isDisp; // 0x4
    char pad_05[3]; // 0x5
    unsigned __int16 sprId; // 0x8
    unsigned __int16 rot; // 0xa
    unsigned __int16 rotA; // 0xc
    unsigned __int16 rotD; // 0xe
    unsigned int attrFlag; // 0x10
    unsigned int prio; // 0x14
    unsigned __int16 srcAlpha; // 0x18
    unsigned __int16 dstAlpha; // 0x1a
    unsigned __int16 opAlpha; // 0x1c
    char pad_1e[2]; // 0x1e
    MtColor color; // 0x20
    MtColor colorOrg; // 0x24
    MtPoint posHDTV; // 0x28
    MtPoint posHDTVOrg; // 0x30
    MtSize scaleHDTV; // 0x38
    MtSize scaleHDTVOrg; // 0x40
};
static_assert(sizeof(sprMap) == 0x48);

struct ANM_HEADER {
    unsigned __int8 m_ID; // 0x0
    unsigned __int8 m_version; // 0x1
    unsigned __int16 m_flag; // 0x2
    unsigned __int16 m_numSpriteGroup; // 0x4
    unsigned __int16 m_numSequence; // 0x6
};
static_assert(sizeof(ANM_HEADER) == 0x8);

struct ANM_SEQUENCE {
    unsigned __int16 m_iSpriteGroup; // 0x0
    unsigned __int8 m_time; // 0x2
    unsigned __int8 m_attribute; // 0x3
    __int16 m_hotspotX; // 0x4
    __int16 m_hotspotY; // 0x6
};
static_assert(sizeof(ANM_SEQUENCE) == 0x8);

struct ANM_SPRITE {
    unsigned __int8 m_texU; // 0x0
    unsigned __int8 m_texV; // 0x1
    unsigned __int8 m_ofsX; // 0x2
    unsigned __int8 m_ofsY; // 0x3
    unsigned __int16 m_CBA; // 0x4
    unsigned __int16 m_flag; // 0x6
    __int16 m_sizeW; // 0x8
    __int16 m_sizeH; // 0xa
    __int16 m_rot; // 0xc
    unsigned __int16 m_flag2; // 0xe
    __int16 m_scaleX; // 0x10
    __int16 m_scaleY; // 0x12
};
static_assert(sizeof(ANM_SPRITE) == 0x14);

namespace sPrim {
    struct Material {
        unsigned __int32 type : 5;
        unsigned __int32 bs_idx : 11;
        unsigned __int32 tex_handle : 10;
        unsigned __int32 transform : 4;
        unsigned __int32 pad : 2;
        unsigned int attribute; // 0x4
    };
    static_assert(sizeof(Material) == 0x8);
} // namespace sPrim

struct __declspec(align(8)) ANM_WORK {
    unsigned __int8 mBe_flag; // 0x0
    unsigned __int8 pad; // 0x1
    unsigned __int16 mRot_y; // 0x2
    int mPos_x; // 0x4
    int mPos_y; // 0x8
    int mPos_z; // 0xc
    unsigned int mAttr; // 0x10
    __int16 mCenter_x; // 0x14
    __int16 mCenter_y; // 0x16
    int mPrio; // 0x18
    int mScale_x; // 0x1c
    int mScale_y; // 0x20
    unsigned __int16 mSeq_num; // 0x24
    char pad_26[2]; // 0x26
    int mUse_sub_seq; // 0x28
    unsigned __int16 mFrame_top; // 0x2c
    unsigned __int16 mFrame_num; // 0x2e
    unsigned __int16 mNow_frame; // 0x30
    unsigned __int16 mNow_spr_gp; // 0x32
    int mTimer; // 0x34
    MtColor mColor; // 0x38
    sPrim::Material mMaterial; // 0x3c
    ANM_SEQUENCE* mpSeq; // 0x44
};
static_assert(sizeof(ANM_WORK) == 0x48);

struct cAnmSprData {
    void* __vftable; // 0x0
    unsigned __int8* mpAnmSource; // 0x4
    void* mpAnmResource; // 0x8
    ANM_HEADER* mpAnmHead; // 0xc
    ANM_SEQUENCE* mpAnmSeqData; // 0x10
    ANM_SPRITE* mpAnmSprData; // 0x14
    ANM_SEQUENCE** mppAnmSeqList; // 0x18
    ANM_SPRITE** mppAnmSprList; // 0x1c
    unsigned int* mpPartsNumList; // 0x20
    unsigned __int16* mpSeqGpNumList; // 0x24
    unsigned __int16 mpSeqGpNum; // 0x28
    char pad_2a[2]; // 0x2a
    int mIsEnable; // 0x2c
    int mIsSet; // 0x30
};
static_assert(sizeof(cAnmSprData) == 0x34);

struct cSprAnm : MtObject {
    bool mIsOwnTexture; // 0x4
    bool mIsOwnAnmData; // 0x5
    char pad_06[2]; // 0x6
    cAnmSprData mAnmSprData; // 0x8
    void* mpTexture; // 0x3c
    ANM_WORK* mpAnm_work; // 0x40
    unsigned int mAnmWorkNum; // 0x44
};
static_assert(sizeof(cSprAnm) == 0x48);

struct fontMap : MtObject {
    bool isDisp; // 0x4
    char pad_05[1]; // 0x5
    unsigned __int16 fontId; // 0x6
    __int16 listId; // 0x8
    unsigned int prio; // 0xc
    MtPoint posHDTV; // 0x10
    MtPoint posHDTVOrg; // 0x18
    MtSize size; // 0x20
    MtSize sizeOrg; // 0x28
};

struct rSprLayout : cResource {
    sprMapHead mSprMapHead; // 0x60
    sprMap* mpSprMap; // 0x70
    fontMap* mpFontMap; // 0x74
};
static_assert(sizeof(rSprLayout) == 0x78);

struct uSprLayout : cUnit {
    rSprLayout* mpResource; // 0x18
    sprMap* mpData; // 0x1c
    cSprAnm* mpSprAnm; // 0x20
    fontMap* mpFontData; // 0x24
};
static_assert(sizeof(uSprLayout) == 0x28);

struct __declspec(align(8)) uDevilCock : uSprLayout {
    uDevilCock* mpNextPtr; // 0x28
    unsigned int mStatus; // 0x2c
    sprMap** mpDispSprMap; // 0x30
};
static_assert(sizeof(uDevilCock) == 0x38);

struct uCockpitMgr : cUnit {
    uDevilCock* mpStartPtr; // 0x18
    uDevilCock* mpEndPtr; // 0x1C
};
static_assert(sizeof(uCockpitMgr) == 0x20);

struct CockpitOverride {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int xOrg = 0;
    int yOrg = 0;
    int wOrg = 0;
    int hOrg = 0;
    int rot = 0;
    int rotA = 0;
    int rotD = 0;
    float color[4] = {1.f, 1.f, 1.f, 1.f};
    int selectedSprite = 0;
};

std::vector<std::vector<sprMap>> baseSpr;
std::vector<CockpitOverride> cockpitOverrides;

static void write_bgra(void* dst, const float in[4]) {
    uint8_t* c = (uint8_t*)dst;
    c[0] = uint8_t(in[2] * 255.0f);
    c[1] = uint8_t(in[1] * 255.0f);
    c[2] = uint8_t(in[0] * 255.0f);
    c[3] = uint8_t(in[3] * 255.0f);
}

// I'm going to tactically omit (_( from this so our zh friend doesn't judge me too much
void Cock::on_gui_frame(int display) {
    if (ImGui::CollapsingHeader("HUD")) {
        sUnit* su = devil4_sdk::get_sUnit();
        ImGui::BeginGroup();
        ImGui::PushItemWidth(sameLineItemWidth);
        if (su != nullptr) {
            MoveLine* uiLine = &(su->mMoveLine[25]);
            if (uiLine->mTop != nullptr) {
                uCockpitMgr* cockmgr = (uCockpitMgr*)uiLine->mTop;
                size_t cockpitCount = 0;
                for (uDevilCock* c = cockmgr->mpStartPtr;; c = c->mpNextPtr) {
                    ++cockpitCount;
                    if (c == cockmgr->mpEndPtr)
                        break;
                }

                if (baseSpr.size() != cockpitCount)
                    baseSpr.resize(cockpitCount);

                if (cockpitOverrides.size() != cockpitCount)
                    cockpitOverrides.resize(cockpitCount);

                size_t initIndex = 0;
                uDevilCock* initCock = cockmgr->mpStartPtr;
                while (true) {
                    rSprLayout* res = initCock->mpResource;
                    auto& base = baseSpr[initIndex];
                    if (base.size() != res->mSprMapHead.sprNum) {
                        base.assign(initCock->mpData, initCock->mpData + res->mSprMapHead.sprNum);
                    }
                    if (initCock == cockmgr->mpEndPtr)
                        break;
                    initCock = initCock->mpNextPtr;
                    ++initIndex;
                }

                if (ImGui::BeginTabBar("Cocks")) {
                    int cockIndex = 0;
                    uDevilCock* cock = cockmgr->mpStartPtr;
                    while (true) {
                        CockpitOverride& ov = cockpitOverrides[cockIndex];
                        char label[32];
                        sprintf_s(label, "Cock %zu", cockIndex);
                        if (ImGui::BeginTabItem(label)) {
                            if (ImGui::Button("Reset cock")) {
                                ov = CockpitOverride{};
                            }
                            ImGui::SliderInt("X", &ov.x, -1000, 1000);
                            ImGui::SliderInt("Y", &ov.y, -1000, 1000);
                            ImGui::SliderInt("W", &ov.w, -500, 500);
                            ImGui::SliderInt("H", &ov.h, -500, 500);
                            ImGui::SliderInt("Org X", &ov.xOrg, -1000, 1000);
                            ImGui::SliderInt("Org Y", &ov.yOrg, -1000, 1000);
                            ImGui::SliderInt("Org W", &ov.wOrg, -500, 500);
                            ImGui::SliderInt("Org H", &ov.hOrg, -500, 500);
                            ImGui::SliderInt("Rotation", &ov.rot, -2000, 2000);
                            ImGui::SliderInt("Rotation A", &ov.rotA, -2000, 2000);
                            ImGui::SliderInt("Rotation D", &ov.rotD, -2000, 2000);
                            ImGui::ColorEdit4("Color", ov.color);
                            ImGui::SeparatorText("Sprites");
                            int spriteCount = (int)baseSpr[cockIndex].size();
                            if (spriteCount > 0) {
                                ImGui::InputInt("Sprite ID", &ov.selectedSprite);
                                ov.selectedSprite = std::clamp(ov.selectedSprite, 0, spriteCount - 1);
                                sprMap& sprite = baseSpr[cockIndex][ov.selectedSprite];
                                ImGui::SliderInt("Sprite X", &sprite.posHDTV.x, -2000, 2000);
                                ImGui::SliderInt("Sprite Y", &sprite.posHDTV.y, -2000, 2000);
                                ImGui::SliderInt("Sprite W", (int*)&sprite.scaleHDTV.w, -1000, 1000);
                                ImGui::SliderInt("Sprite H", (int*)&sprite.scaleHDTV.h, -1000, 1000);
                                ImGui::SliderInt("Sprite Org X", &sprite.posHDTVOrg.x, -2000, 2000);
                                ImGui::SliderInt("Sprite Org Y", &sprite.posHDTVOrg.y, -2000, 2000);
                                ImGui::SliderInt("Sprite Org W", (int*)&sprite.scaleHDTVOrg.w, -1000, 1000);
                                ImGui::SliderInt("Sprite Org H", (int*)&sprite.scaleHDTVOrg.h, -1000, 1000);
                                static constexpr int rotMin = 0;
                                static constexpr int rotMax = UINT16_MAX;
                                ImGui::SliderScalar("Sprite Rot", ImGuiDataType_U16, &sprite.rot, &rotMin, &rotMax);
                                ImGui::SliderScalar("Sprite Rot A", ImGuiDataType_U16, &sprite.rotA, &rotMin, &rotMax);
                                ImGui::SliderScalar("Sprite Rot D", ImGuiDataType_U16, &sprite.rotD, &rotMin, &rotMax);
                            }
                            ImGui::EndTabItem();
                        }
                        if (cock == cockmgr->mpEndPtr)
                            break;

                        cock = cock->mpNextPtr;
                        ++cockIndex;
                    }
                    ImGui::EndTabBar();
                }

                size_t applyIndex = 0;
                uDevilCock* applyCock = cockmgr->mpStartPtr;
                while (true) {
                    rSprLayout* res = applyCock->mpResource;
                    auto& base = baseSpr[applyIndex];
                    CockpitOverride& ov = cockpitOverrides[applyIndex];
                    for (unsigned int i = 0; i < res->mSprMapHead.sprNum; ++i) {
                        sprMap& src = base[i];
                        sprMap& dst = applyCock->mpData[i];
                        dst.posHDTV.x = src.posHDTV.x + ov.x;
                        dst.posHDTV.y = src.posHDTV.y + ov.y;
                        dst.scaleHDTV.w = src.scaleHDTV.w + ov.w;
                        dst.scaleHDTV.h = src.scaleHDTV.h + ov.h;
                        dst.posHDTVOrg.x = src.posHDTVOrg.x + ov.xOrg;
                        dst.posHDTVOrg.y = src.posHDTVOrg.y + ov.yOrg;
                        dst.scaleHDTVOrg.w = src.scaleHDTVOrg.w + ov.wOrg;
                        dst.scaleHDTVOrg.h = src.scaleHDTVOrg.h + ov.hOrg;
                        dst.rot  = src.rot + ov.rot;
                        dst.rotA = src.rotA + ov.rotA;
                        dst.rotD = src.rotD + ov.rotD;
                        write_bgra(&dst.color, ov.color);
                    }

                    if (applyCock == cockmgr->mpEndPtr)
                        break;

                    applyCock = applyCock->mpNextPtr;
                    ++applyIndex;
                }
            }
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();
    }
}
