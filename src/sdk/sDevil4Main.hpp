#pragma once
#include <cstdint>
#include "cSystem.hpp"
#include "windef.h"

// Created with ReClass.NET 1.2 by KN4CK3R

enum class sMainJobMode : int32_t
{
	JOB_DYNAMIC = 0,
	JOB_DYNAMICID = 1,
	JOB_DYNAMIC32 = 2,
	JOB_DYNAMIC64 = 3,
	JOB_DYNAMICPTR = 4,
	JOB_DYNAMICDPTR = 5
};

class sMainJobWorkData
{
public:
	class MtObject *pObject; //0x0000
	void* pThreadFuncMaybe; //0x0004
}; //Size: 0x0008
static_assert(sizeof(sMainJobWorkData) == 0x8);

class sMainJobWorkParams
{
public:
	uint32_t unkjobIdMaybe; //0x0000
	uint32_t unkthreadIdMaybe; //0x0004
	class MtObject *pObject; //0x0008
	uint32_t unkFlagMaybe; //0x000C
	uint32_t N00005B8D; //0x0010
	uint32_t N00005B8E; //0x0014
	uint32_t N00005B8F; //0x0018
	uint32_t jobParam0; //0x001C
	uint32_t N00005C68; //0x0020
	uint32_t threadHandle; //0x0024
}; //Size: 0x0028
static_assert(sizeof(sMainJobWorkParams) == 0x28);

class sDevil4Main : public cSystem
{
	virtual void reset_();
	virtual void move();
public:
	uint64_t mTimer; //0x0020
	int64_t mTimeStart; //0x0028
	int32_t mFrameTimer; //0x0030
	char pad_0034[4]; //0x0034
	float mFps; //0x0038
	float mFPSlock; //0x003C
	uint32_t mVsynMaybeIdk; //0x0040
	float mFPSActual; //0x0044
	int64_t mRealCounter; //0x0048
	int64_t mIdealCounter; //0x0050
	int64_t mPrevCounter; //0x0058
	uint32_t mSkip; //0x0060
	bool mPause; //0x0064
	bool mUnpause; //0x0065
	bool mJobInit; //0x0066
	bool mFinalize; //0x0067
	float mDeltaTimeNorm; //0x0068
	float mDeltaTimeMax; //0x006C
	float mDletaTimeMax; //0x0070
	float mSpeedGlobal; //0x0074
	float mDelayFrame; //0x0078
	uint32_t mThreadHandle; //0x007C
	uint32_t mJobNum; //0x0080
	uint32_t mJobStats; //0x0084
	sMainJobMode mJobType; //0x0088
	uint32_t mNumThreads; //0x008C
	uint32_t N00005726; //0x0090
	uint32_t mNumThreadsLateUpdate; //0x0094
	class sMainJobWorkData mJobs[8096]; //0x0098
	char pad_FD98[776]; //0xFD98
	class sMainJobWorkParams mJobParams[8]; //0x100A0
	float secs_per_tick; //0x101E0
	float currTimeSeconds; //0x101E4
	float appDt; //0x101E8
	float appTimeSeconds; //0x101EC
	float mftime; //0x101F0
	float mltime; //0x101F4
	float mffps; //0x101F8
	uint32_t mfrm_ctr; //0x101FC
	char pad_10200[8]; //0x10200
	LARGE_INTEGER mPerfCounterLastTicks; //0x10208
	LARGE_INTEGER mPerfCounterAppTicks; //0x10210
	LARGE_INTEGER mPerfCounterAppDeltaTicks; //0x10218
	LARGE_INTEGER mPerfCounterAppAccumTicks; //0x10220
	LARGE_INTEGER mPerfCounterDeltaTicks; //0x10228
	uint32_t mCPUCaps; //0x10230
	uint32_t mCPUCoreNum; //0x10234
	uint32_t mCPULogicalProcessorNum; //0x10238
	bool mCPUHyperThreading; //0x1023C
	char N00005C90[256]; //0x1023D
	char pad_1033D[11]; //0x1033D
	class sRender *mpRender; //0x10348
	char pad_1034C[124]; //0x1034C
}; //Size: 0x103C8
static_assert(sizeof(sDevil4Main) == 0x103C8);

class sDevil4Main_JobData
{
public:
	class MtObject *mtObject; //0x0000
	void* callbackMaybe; //0x0004
}; //Size: 0x0008
static_assert(sizeof(sDevil4Main_JobData) == 0x8);

class sMainProcess
{
public:
	float mftime; //0x0000
	float mltime; //0x0004
	float mffps; //0x0008
}; //Size: 0x000C
static_assert(sizeof(sMainProcess) == 0xC);

class sDevil4MainPtr {
public:
    class sDevil4Main* mp_devil4_main; // 0x0000
    char pad_0004[60];                 // 0x0004
}; // Size: 0x0040
static_assert(sizeof(sDevil4MainPtr) == 0x40);