#pragma once
#include <cstdint>
#include "ReClass.hpp"
#include "uActor.hpp"

struct MtRect {
    uint32_t l;
    uint32_t t;
    uint32_t r;
    uint32_t b;
};

struct MtColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct sCamera_ViewPort {
    void* vtable;
    void* mpCamera;
    void* mpTestCamera;
    uint32_t mAttr;
    uint8_t mActive;
    uint8_t mNo;
    uint8_t mSceneNo;
    uint8_t mMode;
    MtRect mRegion;
    char padding0[12];
    struct uActorMain::MtVector4 mFrustum[6];
    MtColor mClearColor;
    float mClearZ;
    uint32_t mClearStencil;
    void* mpRenderTarget;
    void* mpDepthStencil;
    void* mpSubPixelMask;
    char padding1[8];
    MtMatrix mViewMat;
    MtMatrix mProjMat;
    MtMatrix mPrevViewMat;
    MtMatrix mPrevProjMat;
    float mFogStart;
    float mFogEnd;
    uint32_t mFogPlaneType;
    float mFogPlaneWidth;
    float mFogPlaneDist;
    float mFogAttentuation;
    char padding2[8];
    struct uActorMain::MtVector4 mFogPlaneDir;
    struct uActorMain::MtVector4 mFogColor;
    struct uActorMain::MtVector4 UknVecs1[2];
    struct uActorMain::MtVector4 UknVecs2[2];
    struct uActorMain::MtVector4 mTransFogFactor[2];
    struct uActorMain::MtVector4 mTransFogColor[2];
    MtMatrix mTransViewMat[2];
    MtMatrix mTransProjMat[2];
    MtMatrix mTransViewProjMat[2];
    MtMatrix mTransPrevViewMat[2];
    MtMatrix mTransPrevProjMat[2];
    MtMatrix mTransPrevViewProjMat[2];
    struct uActorMain::MtVector4 mTransEyePos[2];
};
static_assert(sizeof(sCamera_ViewPort) == 0x590);

struct sCamera : CSystem {
    float mSubPixelOfsX;
    float mSubPixelOfsY;
    float mViewSubFrame;
    float mWorldSubFrame;
    sCamera_ViewPort viewports[8];
    MtRect ScreenRect;
    float SceneSize[2];
    uint32_t mLayoutMode;
    uint32_t mPause;
};