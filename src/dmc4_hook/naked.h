#pragma once
//#include "mods/modPlayerTracker.hpp"

extern "C"
{
    uintptr_t _infiniteDTContinue = NULL;
    uintptr_t _infinitePlayerHealthContinue = NULL;
    uintptr_t _berialDazeContinue = NULL;
    uintptr_t _stunAnythingContinue = NULL;
    uintptr_t _roseRemovesPinsContinue = 0x008158CD;
    uintptr_t _roseRemovesPinsJE = 0x00815970;

	//bools to check for toggling purposes within a gui checkbox, if false, then checkbox toggle is required to activate
    bool g_InfDTEnable = false;
    bool g_InfPlayerHealthEnable = false;
    bool g_berialDazeEnable = false;
    bool g_stunAnythingEnable = false;
	bool g_roseRemovesPinsEnable = false;
}

//procs that apply changes directly using inline asm, work in conjunction with hooks

_declspec(naked) void infiniteDT_proc(void)
{
    _asm {
			cmp byte ptr [g_InfDTEnable], 0
			je originalcode

			mov dword ptr [ebp+0x00001F24], 0x461C4000

		originalcode:
			//movss xmm0,[ebp+0x00001F24]
			jmp dword ptr [_infiniteDTContinue]
    }
}

_declspec(naked) void infinitePlayerHealth_proc(void)
{
    _asm {
			cmp byte ptr [g_InfPlayerHealthEnable], 0
			je originalcode

			mov dword ptr [ebp+0x000015CC], 0x469C4000

		originalcode:
			//movss xmm0,[ebp+0x000015CC]
			jmp dword ptr [_infinitePlayerHealthContinue]
    }
}

_declspec(naked) void berialDaze_proc(void)
{
    _asm {
			cmp byte ptr [g_berialDazeEnable], 0
			je originalcode

			movss xmm0,[esi+0x00001B70]
			addss xmm0,[esi+0x00001B70]
			movss [esi+0x1B80], xmm0
		
		originalcode:
			//movss xmm0,[esi+0x00001B80]
			jmp dword ptr [_berialDazeContinue]
    }
}

_declspec(naked) void roseRemovesPins_proc(void)
{
    _asm {
			cmp byte ptr [g_roseRemovesPinsEnable],0
			je originalcode

			cmp byte ptr [esi+0x14],04
			je removepinsje
			cmp byte ptr [esi+0x14],03
			je threecode
			jmp dword ptr [_roseRemovesPinsContinue]

		threecode:
			mov byte ptr [esi+0x14],00

		removepinsje:
			jmp dword ptr [_roseRemovesPinsJE]

		originalcode:
			cmp byte ptr [esi+0x14],04
			je removepinsje
			jmp dword ptr [_roseRemovesPinsContinue]
    }
}