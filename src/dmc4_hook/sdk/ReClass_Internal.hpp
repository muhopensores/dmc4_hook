#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

//#include "glm/mat3x3.hpp"

using Vector3f = glm::vec3;
using Vector4 = glm::vec4;

// Generated using ReClass 2016
#include <cstdint>

class cAreaJump;
class roomInfo;

class cAreaJump
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char pad_0x0004[0x28]; //0x0004
	roomInfo* roomInfoPtr; //0x002C 
	char pad_0x0030[0x20]; //0x0030
	Vector3f unkVec; //0x0050 
	Vector3f unkVec2; //0x005C 
	__int32 initJump; //0x0068 
	__int32 roomId; //0x006C 
	__int32 mission; //0x0070 
	__int32 bpFloorStage; //0x0074 
	char pad_0x0078[0x48]; //0x0078

}; //Size=0x00C0

class roomInfo
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char N0000127E[20]; //0x11E5888 
	char pad_0x0018[0x3EC]; //0x0018

}; //Size=0x0404

class sWorkRate;
class unkClass;
class unkClass2;

class sWorkRate
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	char pad_0x0004[0x1C]; //0x0004
	float unkFloat; //0x0020 
	float unkFloat2; //0x0024 
	float globalSpeed; //0x0028 
	float playerSpeed; //0x002C 
	float enemySpeed; //0x0030 
	float roomSpeed; //0x0034 
	float turboSpeed; //0x0038 
	float unkFloat3; //0x003C 
	char pad_0x0040[0x10]; //0x0040
	unkClass* unkClassPtr; //0x0050 
	char pad_0x0054[0x4]; //0x0054

}; //Size=0x0058

class unkClass
{
public:
	unkClass2* next; //0x0000 
	uint32_t N0000045C; //0x0004 
	char pad_0x0008[0x4]; //0x0008
	uint32_t N0000045E; //0x000C 
	char pad_0x0010[0x10]; //0x0010
	uint32_t N00000463; //0x0020 
	char pad_0x0024[0x8]; //0x0024

}; //Size=0x002C

class unkClass2
{
public:
	uint32_t nextPtr; //0x0000 
	char pad_0x0004[0x400]; //0x0004

}; //Size=0x0404

class Inputs
{
public:
	virtual void Function0(); //
	virtual void Function1(); //
	virtual void Function2(); //
	virtual void Function3(); //
	virtual void Function4(); //
	virtual void Function5(); //
	virtual void Function6(); //
	virtual void Function7(); //
	virtual void Function8(); //
	virtual void Function9(); //

	__int32 ButtonPressed; //0x0004 
	__int32 ButtonReleased; //0x0008 
	__int32 ButtonWentDown; //0x000C 
	__int32 ButtonPressed02; //0x0010 
	char pad_0x0014[0x4]; //0x0014
	float unkFloat; //0x0018 
	char pad_0x001C[0x18]; //0x001C

}; //Size=0x0034

class MtObject;

class MtDTI
{
public:
	char *mName; //0x0004
	class MtDTI *mpNext; //0x0008
	class MtDTI *mpChild; //0x000C
	class MtDTI *mpParent; //0x0010
	class MtDTI *mpLink; //0x0014
	uint32_t mpLink1; //0x0018
	uint32_t mID; //0x001C
	// hope it's fucking thiscall
	//virtual void vecDelDtor(MtDTI* this, unsigned int x)();
	//virtual void MtObject* newInstance(MtDTI *this)();

	virtual void vecDelDtor(unsigned int x);
	virtual MtObject* newInstance();
	virtual void Function2();
	virtual void Function3();
	virtual void Function4();
	virtual void Function5();
	virtual void Function6();
	virtual void Function7();
	virtual void Function8();
	virtual void Function9();
}; //Size: 0x0020

class MtObject
{
public:

	virtual void vecDelDtor(unsigned int i);
	virtual void createUI(void* prop);
	virtual bool isEnableInstance();
	virtual void createProperty(void* prop);
	virtual MtDTI* getDTI();
	virtual MtObject* allocate(size_t size, int a2);
	virtual void Function6();
	virtual void Function7();
	virtual void Function8();
	virtual void Function9();
	virtual void Function10();
	virtual void Function11();
}; //Size: 0x0004

class MtMatrix
{
public:
	Vector4 m1; //0x0000
	Vector4 m2; //0x0010
	Vector4 m3; //0x0020
	Vector4 m4; //0x0030
}; //Size: 0x0040
using Matrix4x4 = MtMatrix;

class cUnit : public MtObject
{
public:
	uint32_t bitfield; //0x0004
	class cUnit *mpNextUnit; //0x0008
	class cUnit *mpPrevUnit; //0x000C
	float mDeltaTime; //0x0010
	uint32_t someUnion; //0x0014
}; //Size: 0x0018
static_assert(sizeof(cUnit) == 0x18);

class uCoord : public cUnit
{
public:
	class uCoord *mpParent; //0x0018
	char pad_001C[20]; //0x001C
	Vector3f mPos; //0x0030
	char pad_003C[4]; //0x003C
	Vector4 mQuat; //0x0040
	Vector3f mScale; //0x0050
	Matrix4x4 mLmat; //0x005C
	Matrix4x4 mWmat; //0x009C
}; //Size: 0x00DC
static_assert(sizeof(uCoord) == 0xDC);

class uFilter : public cUnit
{
public:
	uint16_t mPass; //0x0018
	char pad_001A[2]; //0x001A
	uint32_t mPriority; //0x001C
}; //Size: 0x0020
static_assert(sizeof(uFilter) == 0x20);

class MtHermiteCurve
{
public:
	float x[8]; //0x0000
	float y[8]; //0x0020
}; //Size: 0x0040
static_assert(sizeof(MtHermiteCurve) == 0x40);

enum class CC_TYPE : int32_t
{
	TYPE_UNUSED = 0,
	TYPE_HUE = 1,
	TYPE_CONTRAST = 2,
	TYPE_CONTRAST2 = 3,
	TYPE_CHROMA = 4,
	TYPE_CHROMA2 = 5,
	TYPE_BRIGHTNESS = 6,
	TYPE_BRIGHTNESS2 = 7,
	TYPE_SCALE = 8,
	TYPE_SCALE2 = 9,
	TYPE_INPUTLEVEL = 10,
	TYPE_INPUTLEVEL2 = 11,
	TYPE_OUTPUTLEVEL = 12,
	TYPE_OUTPUTLEVEL2 = 13,
	TYPE_SEPIA = 14,
	TYPE_SEPIA2 = 15,
	TYPE_NEGA = 16,
	TYPE_NEGA2 = 17,
	TYPE_GRAYSCALE = 18,
	TYPE_RREPLACE = 19,
	TYPE_GREPLACE = 20,
	TYPE_BREPLACE = 21,
	TYPE_SEPIA3 = 22
}; //0x0008

class uColorCorrectFilter__Corrector
{
public:

	bool mEnable; //0x0004
	/*uint16_t N00002DAF; //0x0005
	uint8_t N00002DB0; //0x0007*/
	CC_TYPE mType; //0x0008
	Vector4 mFactor; //0x000C
	Vector4 mFactor2; //0x001C
	char pad_002C[4]; //0x002C

	virtual void Function0();
	virtual void Function1();
	virtual void Function2();
	virtual void Function3();
	virtual void Function4();
	virtual void Function5();
	virtual void Function6();
	virtual void Function7();
	virtual void Function8();
	virtual void Function9();
}; //Size: 0x0030

class uColorCorrectFilter : public uFilter
{
public:

	class uColorCorrectFilter__Corrector correctors[16]; //0x0020
	Matrix4x4 mMatrix; //0x0320
	class MtHermiteCurve mToneCurveR; //0x0360
	class MtHermiteCurve mToneCurveG; //0x03A0
	class MtHermiteCurve mToneCurveB; //0x03E0
	Vector4 mGamma; //0x0420
	class cTransTexture *cTansTexture; //0x0430
}; //Size: 0x0434


class uEnemySomething
{
public:
	char pad_0000[4992]; //0x0000
	Vector3f mSpawnCoords; //0x1380
	char pad_138C[8]; //0x138C
	uint8_t N00004023; //0x1394
	char pad_1395[3]; //0x1395
	int32_t mEnemySpawnEffectSomething; //0x1398
	char pad_139C[12]; //0x139C
	uint32_t unkTypeSomething; //0x13A8
	uint32_t N00003FEF; //0x13AC
	uint32_t N00003FF0; //0x13B0
	uint32_t N00003FF1; //0x13B4
	uint32_t N00003FF2; //0x13B8
	uint32_t N00003FF3; //0x13BC
	Matrix4x4 N00003FF4; //0x13C0
	uint32_t N00003FF5; //0x1400
	uint8_t N00003FF6; //0x1404
	uint8_t N00004033; //0x1405
	uint8_t N00004037; //0x1406
	char pad_1407[1]; //0x1407
	float N00003FF7; //0x1408
	uint32_t N00003FF8; //0x140C
	uint32_t N00003FF9; //0x1410
	uint32_t N00003FFA; //0x1414
	uint32_t N0000403D; //0x1418 Ptr
	char pad_141C[24]; //0x141C
	float N00004000; //0x1434
	uint8_t N00004001; //0x1438
	char pad_1439[3]; //0x1439
	uint32_t N00004002; //0x143C
	uint32_t N00004003; //0x1440
	uint32_t N00004004; //0x1444
	char pad_1448[228]; //0x1448
	bool N00003C7B; //0x152C
	bool N00003FDD; //0x152D
	bool N00003FE0; //0x152E
	char pad_152F[4409]; //0x152F
}; //Size: 0x2668

class sMediator
{
public:
	char pad_0000[36]; //0x0000
	class uPlayer* playerPtr; //0x0024
	char pad_0028[24]; //0x0028

	// TODO(): make it into methods for this shit?
	//uPlayer* getLocalPlayer();
}; //Size: 0x0040

class uModel__Joint
{
public:
	char pad_0000[48]; //0x0000
	Vector3f size; //0x0030
	char pad_003C[84]; //0x003C
}; //Size: 0x0090
static_assert(sizeof(uModel__Joint) == 0x90);

class uModelJointArr
{
public:
	class uModel__Joint joint[75]; //0x0000
}; //Size: 0x2A30
static_assert(sizeof(uModelJointArr) == 0x2A30);

class uPlayer : public uCoord
{
public:
    char pad_00DC[0x204]; //0x00DC
	uint32_t mJointArraySize; //0x02E0
	class uModelJointArr *jointArray; //0x02E4
	char pad_02E8[0x13E8]; //0x02E8
	bool lockontoggle; //0x16D0
    char pad_16D1[0x77F]; //0x16D1
	Vector3f m_dVelocity; //0x1E50
	char pad_1E5C[0x4]; //0x1E5C
	float m_dVelMagnitude; //0x1E60
	char pad_1E64[0xA04]; //0x1E64
}; //Size: 0x2868
static_assert(sizeof(uPlayer) == 0x2868);

//0x0330 start of animation stuff (also a pointer)
//0x0348 float animationFrame
//0x1201 bool isHitstop
//0x1408 start of inputs stuff (also a pointer)
//0x140C bool*4 inputs on hold
//0x1410 bool*4 inputs on press?
//0x1414 bool*4 inputs on release?
//0x1504 int partOfMove? (shows int 1-5 when doing actions)
//0x15CC float currentHP
//0x15D0 float maxHP
//0x1640 bool isDead
//0x1660 float 180float (some float that starts at 0, incs to 180, then resets?)
//0x16AC int movementType (0=still, 1=walking, 2=running)
//0x19B8 pointer to uPlayer
//0x1D94 dante gilgamesh pointer
//->0x137C bool shouldShow
//0x1D98 dante lucifer pointer
//->0x137C bool shouldShow
//0x1D9C dante ebony+ivory pointer
//->0x137C bool shouldShow
//0x1DA0 dante shotgun pointer
//->0x137C bool shouldShow
//0x1DA4 dante pandora pointer
//->0x137C bool shouldShow
//0x1DA8 pointer only in dantes char, contains what I used for moveid
//->0x225C int moveiddante (should replace this with uPlayer+0x295C moveid)
//0x1DFC start of move properties stuff (also a pointer)
//0x1E14 bool*4 canCancel
//0x1E68 float hitStopTimer
//0x1E64 float direction of magnitude?
//0x1E7B byte airAttackCount
//0x1E7D byte weight
//0x1E7E byte airHikeCount
//0x1F24 float currentDT
//0x1F28 float maxDT
//0x1FF4 float some jump timer
//0x1FF8 float another jump timer
//0x28A4 pointer to uPlayer+330 for animation stuff, e.g. uPlayer + 0x28A4->0x18 = frame of animation = uPlayer + 348
//0x2008 bool grounded
//0x295C int moveid
//0x30C4 int canBuffer (0=can't act, 1=can buffer, 2=can act)
//0x3178 bool isJumpBuffered (1 will make the player jump on the first available frame)

class uStageSetTimeSlow
{
public:
	char pad_0000[4992]; //0x0000
	float mTimeLeft; //0x1380
	float mDuration; //0x1384
	float mEnemyWorkRate; //0x1388
	float mStageWorkRate; //0x138C
	char pad_1390[3288]; //0x1390
}; //Size: 0x2068
static_assert(sizeof(uStageSetTimeSlow) == 0x2068);


class cResource : public MtObject
{
public:
	char path[64]; //0x0004
	uint32_t mRefCount; //0x0044
	uint32_t mAttr; //0x0048
	uint32_t mFlagsSomething; //0x004C
	uint32_t mSize; //0x0050
	int64_t mID; //0x0054
	char pad_005C[4]; //0x005C
}; //Size: 0x0060

class N00002D7A
{
public:
	char pad_0000[4]; //0x0000
}; //Size: 0x0004
static_assert(sizeof(N00002D7A) == 0x4);

class rEffectList : public cResource
{
public:
	float mBaseFps; //0x0060
	char *mParamBuf; //0x0064
	uint32_t mParamBuffSize; //0x0068
	uint32_t mListNum; //0x006C
	uint32_t N00002D88; //0x0070
	uint32_t N00002D89; //0x0074
	uint32_t mResourcePtr; //0x0078
	uint32_t mResourceInfoNum; //0x007C
}; //Size: 0x0080

class MtAllocator : public MtObject
{
public:
	uint32_t mUsedSize; //0x0004
	uint32_t mMaxUsedSize; //0x0008
	uint32_t mTotalSize; //0x000C
	char *mName; //0x0010
	uint16_t mType; //0x0014
	uint16_t mAttr; //0x0016
	uint32_t mOwner; //0x0018
	class MtCriticalSection *mCS; //0x001C
}; //Size: 0x0020

class MtHeapAllocator : public MtAllocator
{
public:
	char pad_0020[60]; //0x0020
}; //Size: 0x005C

class uEfctCam
{
public:
	char pad_0000[512]; //0x0000
	uint32_t someField; //0x0200
}; //Size: 0x0204

class uTVNoiseFilter : public uFilter
{
public:
	Vector3f mNoisyUVOffset; //0x0020
	char pad_002C[8]; //0x002C
	float mNoisePower; //0x0034
	float mNoisePowerCroma; //0x0038
	float mScaleY; //0x003C
	float mScaleCrCb; //0x0040
	uint32_t mScanlineSize; //0x0044
	float mScanlineAlpha; //0x0048
	uint32_t noiseTexturePtr; //0x004C
	uint32_t mpTVMaskTexturePtr; //0x0050
	bool mEnableBlankScan; //0x0054
	char pad_0055[3]; //0x0055
	float mBlankOfs_y; //0x0058
	float mBlankSpeed; //0x005C
	uint8_t mBlankAlpha; //0x0060
	char pad_0061[3]; //0x0061
	float mBlankSize; //0x0064
	float mShockNoiseFreq; //0x0068
	float mHSyncNoiseAmplitude; //0x006C
	float mVSyncNoiseAmplitude; //0x0070
	float mShockDetailReduction; //0x0074
	float mHSyncBlankWidth; //0x0078
	float mVSyncBlankWidth; //0x007C
	Vector4 mVSyncBlankColor; //0x0080
	uint32_t mShockNoiseTime; //0x0090
	float mHSyncNoiseValue; //0x0094
	float mVSyncNoiseValue; //0x0098
	float mHSyncLag; //0x009C
	float mVSyncLag; //0x00A0
	float mDetailReduction; //0x00A4
	float mHSyncNoiseOffset; //0x00A8
	char pad_00AC[1992]; //0x00AC
}; //Size: 0x0874


class sStylishCount
{
public:
	char pad_0000[32]; //0x0000
	uint32_t currentStyleTier; //0x0020
	char pad_0024[508]; //0x0024
}; //Size: 0x0220
static_assert(sizeof(sStylishCount) == 0x220);

class uEm003Shl : public MtObject
{
public:
	char pad_0004[44]; //0x0004
	Vector3f mPosMaybe; //0x0030
	char pad_003C[3716]; //0x003C
	Vector3f mSomeFloatVec; //0x0EC0
	char pad_0ECC[2260]; //0x0ECC
	uint32_t mRequiredIdkWtfIsThisNeedsToBe2; //0x17A0
	uint32_t mRequiredIdkWtfIsThisNeedsToBe6; //0x17A4
	char pad_17A8[12]; //0x17A8
	uint16_t N00005A23; //0x17B4
	uint8_t mRequiredSomeByteIdk; //0x17B6
	char pad_17B7[57]; //0x17B7
}; //Size: 0x17F0
static_assert(sizeof(uEm003Shl) == 0x17F0);