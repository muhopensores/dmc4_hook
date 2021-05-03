#include "modQuicksilver.hpp"

void qsOperatorNew() {

	constexpr uintptr_t uStageSlowConstructor = 0x00887440;
	constexpr uintptr_t uSomeGlob = 0x00E552CC;
	constexpr uintptr_t spawnShit = 0x008DC540;
	constexpr uintptr_t uColorCorrectFilterConstructor = 0x0093D9A0;
	constexpr uintptr_t uTVNoiseFilterConstructor = 0x00922290;
	float tvNoiseArg1 = 0.06f;
	float tvNoiseArg2 = 0.02f;
	float tvNOiseArg3 = 0.005f;
	__asm {
		pusha
		pushf

		// noise
		/*call uTVNoiseFilterConstructor
		xor [eax+0x4], 0x00000800
		mov edx, [tvNoiseArg1]
		mov [eax+0x34], edx
		mov edx, [tvNoiseArg2]
		mov [eax+0x68], edx
		mov edx, [tvNOiseArg3]
		mov [eax+0x6C], edx
		mov [eax+0x44], 0x00000000
		mov esi,eax
		test esi, esi
		jz bounce

		mov eax, uSomeGlob
		mov eax, [eax]
		push 0x1D
		call spawnShit*/

		// cc
		call uColorCorrectFilterConstructor
		mov ecx, 00000800h
		xor DWORD PTR [eax+0x4], ecx
		mov ecx, 16
		mov DWORD PTR [eax+0x58], ecx
		/*mov [eax+0x74], 0x00000004
		mov [eax+0x80], 0x00000000
		mov [eax+0x84], 0x00000000
		mov [eax+0x88], 0x00000000*/
		mov esi, eax
		test esi, esi
		jz bounce

		mov eax, uSomeGlob
		mov eax, [eax]
		push 0x1D
		call spawnShit

		// slowdown

		call uStageSlowConstructor
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
}

std::optional<std::string> Quicksilver::onInitialize()
{
	return Mod::onInitialize();
}

void Quicksilver::onGUIframe()
{
	// from main.cpp
	// line 907 -> main->getMods()->onDrawUI("NoClip"_hash);
	if (ImGui::Button("Quicksilver test"))
	{
		qsOperatorNew();
	}
}

void Quicksilver::onTwitchCommand(std::size_t hash)
{
	HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_command);
	if (hash == m_command) 
	{
	}
}

void Quicksilver::onFrame(fmilliseconds& dt) 
{
	//m_timer->tick(dt);
};