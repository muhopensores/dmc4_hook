#include "modQuicksilver.hpp"

#include "glm/gtx/compatibility.hpp"

// Found out that the game does not load anything
// not related to current stage, after writing
// all this nonsence
#if 0
void spawnOrSomething(MtObject* toSpawn) {
	constexpr uintptr_t uSomeGlob = 0x00E552CC;
	constexpr uintptr_t spawnShit = 0x008DC540;
	
	__asm {
		mov eax, uSomeGlob
		mov eax, [eax]
		push 0xA
		mov esi, toSpawn
		call spawnShit
	}
}

rEffectList* LoadEffectAsmCall() {

	uintptr_t sDevil4ResourcePtr = 0x00E552D0;
	uintptr_t MtDtiPtr= 0x00EADE88;
	const char* path = "effect\\efl\\scr\\es402_81v0";
	uintptr_t LoadEffectResourceFptr = 0x008DF530;

	rEffectList* asmRet = nullptr;

	__asm {
		mov eax, [sDevil4ResourcePtr]
		mov eax, [eax]
		push 01
		mov ebx, [path]
		push ebx
		mov ebx, [MtDtiPtr]
		push ebx
		call LoadEffectResourceFptr
		mov [asmRet], eax
	}
	return asmRet;
}

uEfctCam* getEffctCam(MtObject* mto) {

	uEfctCam* returnValue = nullptr;
	uintptr_t funcPtr = 0x00539270;

	__asm {
		mov esi, [mto]
		call funcPtr
		mov [returnValue], eax
	}

	return returnValue;
}


struct someArg {
	glm::vec3 one { 0.0f, 0.0f, 0.0f };
	glm::vec4 two { 1.0f, 0.0f, 0.0f, 0.0f };
};


void someWeirdAssFunction(someArg arg, uEfctCam* uCam, MtObject* alloced) {
	
	uintptr_t funcPtr = 0x0096A0A0;
	__asm {
		push 0
		lea ecx, arg
		push arg
		push -1
		push -1
		push alloced
		push uCam
		mov eax, 4
		or ecx, -1
		call funcPtr
	}
}

void screenBlinkEffect() {
	if (LoadEffectAsmCall()) {

		uintptr_t staticMtHeapAllocPtr = 0x00E1434C;
		MtHeapAllocator* mth = *(MtHeapAllocator**)staticMtHeapAllocPtr;

		MtObject* allocated = mth->allocate(0x210, 0x10);
		if (allocated) {
			uEfctCam* cam = getEffctCam(allocated);
			if (cam) {
				cam->someField = 1;
				someArg arg{};
				someWeirdAssFunction(arg, cam, allocated);
				spawnOrSomething((MtObject*)cam);
			}
		}
	}
}
#endif

struct PostProcessingEffects {
	uTVNoiseFilter* tv;
	uColorCorrectFilter* cc;
};

PostProcessingEffects pps;
uStageSetTimeSlow* g_ss;

void TVNoiseConstructorParams(uTVNoiseFilter* tv) {
	tv->mPriority ^= 2048;
	tv->mNoisePower = 0.06f;
	tv->mShockNoiseFreq = 0.02f;
	tv->mVSyncNoiseAmplitude = 0.005f;
}

void uColorCorrectConstructorParams(uColorCorrectFilter* cc) {
	cc->mPriority ^= 2048;
	
	/*cc->correctors[0].mType = CC_TYPE::TYPE_NEGA2;
	cc->correctors[0].mEnable = true;*/

	cc->correctors[0].mType = CC_TYPE::TYPE_CHROMA;
	cc->correctors[0].mFactor = glm::vec4{ 1.0f, 0.1f, 0.1f, 0.1f };
	cc->correctors[0].mEnable = true;
}

void uStageSetTimeSlowConstructorParams(uStageSetTimeSlow* ss) {
	g_ss = ss;
	ss->mDuration = 1100.0f; // hardcoded for now idk what they used
}
bool startInterpolating = false;

void qsOperatorNew() {

	constexpr uintptr_t uStageSlowConstructor = 0x00887440;
	constexpr uintptr_t uSomeGlob = 0x00E552CC;
	constexpr uintptr_t spawnShit = 0x008DC540;
	constexpr uintptr_t uColorCorrectFilterConstructor = 0x0093D9A0;
	constexpr uintptr_t uTVNoiseFilterConstructor = 0x00922290;
	float tvNoiseArg1 = 0.06f;
	float tvNoiseArg2 = 0.02f;
	float tvNOiseArg3 = 0.005f;
	startInterpolating = true;

	__asm {
		pusha
		pushf
		//call screenBlinkEffect // not working on 4 lmao
		// noise
		call uTVNoiseFilterConstructor
		/*xor [eax+0x4], 0x00000800
		mov edx, [tvNoiseArg1]
		mov [eax+0x34], edx
		mov edx, [tvNoiseArg2]
		mov [eax+0x68], edx
		mov edx, [tvNOiseArg3]
		mov [eax+0x6C], edx
		mov [eax+0x44], 0x00000000*/
		push eax
		call TVNoiseConstructorParams
		pop eax
		mov esi,eax
		test esi, esi
		jz bounce

		mov dword ptr [pps.tv], esi
		mov eax, uSomeGlob
		mov eax, [eax]
		push 0x1D
		call spawnShit

		// cc
		call uColorCorrectFilterConstructor
		pusha
		push eax
		call uColorCorrectConstructorParams
		pop eax
		popa
		/*mov ecx, 00000800h
		xor DWORD PTR [eax+0x4], ecx
		mov ecx, 16
		mov DWORD PTR [eax+0x58], ecx
		mov [eax+0x74], 0x00000004
		mov [eax+0x80], 0x00000000
		mov [eax+0x84], 0x00000000
		mov [eax+0x88], 0x00000000*/

		mov esi, eax
		test esi, esi
		jz bounce
		mov dword ptr [pps.cc], esi
		mov eax, uSomeGlob
		mov eax, [eax]
		push 0x1D
		call spawnShit

		// slowdown

		call uStageSlowConstructor
		pusha
		push eax
		call uStageSetTimeSlowConstructorParams
		pop eax
		popa
		mov esi, eax
		test esi, esi
		jz bounce
		mov eax, uSomeGlob
		mov eax, [eax]
		push 01
		call spawnShit
	bounce:
		popf
		popa
	}
}

static void onTimerCallback() {
	g_ss->mTimeLeft = 0.0f;
	pps.tv->bitfield ^= 2048;
	pps.cc->bitfield ^= 2048;
}

std::optional<std::string> Quicksilver::onInitialize()
{
	m_command = std::hash<std::string>{}("\\Quicksilver");
	m_shorthand = std::hash<std::string>{}("\\qs");

	// timer duration in float and callback function once it finishes
	m_timer = new utils::Timer(15.0f, onTimerCallback);

	return Mod::onInitialize();
}

void Quicksilver::onGUIframe()
{
	if (ImGui::Button("QSTEST")) {
		qsOperatorNew();
		m_timer->start();
	}
	// TODO(): not twitch shit
}

void Quicksilver::onTwitchCommand(std::size_t hash)
{
	if (!m_timer->m_active) { return; }

	if (hash == m_command || hash == m_shorthand) 
	{
		if (m_timer) {
			qsOperatorNew();
			m_timer->start();
		}
	}
}

void Quicksilver::onFrame(fmilliseconds& dt) 
{
	if (m_timer) {
		m_timer->tick(dt);
	}
};