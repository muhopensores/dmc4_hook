#pragma once


extern "C"
{
    const float limitadjust = 0.0f;
    int moveIDOne = 0;
    int lockOnAlloc = 0;
    int backForwardAlloc = 0;
    float damagemultiplier = 1.0f;
    float cameraHeight = 170.0f;
    float cameraDistance = 550.0f;
    float cameraDistanceLockon = 520.0f;
    float cameraAngle = 0.2f;
    float fullHouseAngle = 65.0f;
    float timerMem = 0.0f;
    float timerMemTick = 1.0f;
    float downFloat = -200.0f;
    float momentumMem = 0.0f;
    uintptr_t _damagemodifierContinue = NULL;
    uintptr_t _infiniteDTContinue = NULL;
    uintptr_t _infinitePlayerHealthContinue = NULL;
    uintptr_t _berialDazeContinue = NULL;
    uintptr_t _moveIDAllocContinue = NULL;
    uintptr_t _cancelEcstasyContinue = NULL;
    uintptr_t _stunAnythingContinue = NULL;
    uintptr_t _cameraHeightContinue = NULL;
    uintptr_t _cameraDistanceContinue = NULL;
    uintptr_t _cameraDistanceLockonContinue = NULL;
    uintptr_t _cameraAngleContinue = NULL;
    uintptr_t _ldkWithDMDContinue = NULL;
    uintptr_t _lockOnContinue = NULL;
    uintptr_t _lockOffContinue = NULL;
    uintptr_t _trackingFullHouseContinue = 0x007D3478;
    uintptr_t _timerAllocContinue = 0x007AD774;
    uintptr_t _backForwardContinue = 0x00805A60; // This isn't return, its the next opcode
    uintptr_t _trickDownContinue = 0x007CB121;
    uintptr_t _floorTouchContinue = 0x007CB345;
    uintptr_t _tricksterDashMove = 0x00C413A4;
    uintptr_t _skyStarMove = 0x00C413DC;
    bool g_InfDTEnable = false;
    bool g_InfPlayerHealthEnable = false;
    bool g_berialDazeEnable = false;
    bool g_moveIDAllocEnable = false;
    bool g_cancelEcstasyEnable = false;
    bool g_damageModifierEnable = false;
    bool g_stunAnythingEnable = false;
    bool g_ldkWithDMDEnable = false;
    bool g_trackingFullHouseEnable = false;
    bool g_trickDownEnable = false;
}


_declspec(naked) void limitadjust_patch(void)
{
    _asm {
			fld dword ptr [limitadjust]
    }
}

_declspec(naked) void damagemodifier_proc(void)
{
    _asm {
			cmp byte ptr [g_damageModifierEnable],0
			je originalcode

			cmp dword ptr [esi+0x1C], 0x469C4000
			je originalcode

			movss xmm4,[esi+0x18]
			subss xmm4,xmm0
            movss xmm0,[esi+0x18]
            mulss xmm4,[damagemultiplier]
            subss xmm0,xmm4

		originalcode:
            movss [esi+0x18],xmm0
            jmp dword ptr [_damagemodifierContinue]
    }
}

_declspec(naked) void infiniteDT_proc(void)
{
    _asm {
			cmp byte ptr [g_InfDTEnable], 0
			je originalcode

			mov dword ptr [ebp+0x00001F24], 0x461C4000

		originalcode:
			movss xmm0,[ebp+0x00001F24]
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
			movss xmm0,[ebp+0x000015CC]
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
			movss xmm0,[esi+0x00001B80]
			jmp dword ptr [_berialDazeContinue]
    }
}

_declspec(naked) void moveIDAlloc_proc(void)
{
    _asm {
			cmp byte ptr [g_moveIDAllocEnable], 0
			je originalcode

			mov [moveIDOne],ecx

		originalcode:
			mov [esi+0x0000225C],ecx
			jmp dword ptr [_moveIDAllocContinue]
    }
}

_declspec(naked) void cancelEcstasy_proc(void)
{
    _asm {
			cmp byte ptr [g_cancelEcstasyEnable], 0
			je originalcode

			cmp [moveIDOne],0x412
			je writecancel

		originalcode:
			cmp dword ptr [esi+0x00003174],0x02
			jmp dword ptr [_cancelEcstasyContinue]

		writecancel:
			mov [esi+0x00003174],0x02
			jmp dword ptr [_cancelEcstasyContinue]
    }
}

_declspec(naked) void cameraHeight_proc(void)
{
    _asm {
			push eax
			mov eax,[cameraHeight]
			mov [edi+0x000000D0], eax
			pop eax
			movss xmm0,[edi+0x000000D0]
			jmp dword ptr [_cameraHeightContinue]
    }
}

_declspec(naked) void cameraDistance_proc(void)
{
    _asm {
			push eax
			mov eax,[cameraDistance]
			mov [esi+0x000000D8],eax
			pop eax
			movss xmm3,[esi+0x000000D8]
			jmp dword ptr [_cameraDistanceContinue]
    }
}

_declspec(naked) void cameraDistanceLockon_proc(void)
{
    _asm {
			push eax
			mov eax,[cameraDistanceLockon]
			mov [ebx+0x000000DC],eax
			pop eax
			movss xmm1,[ebx+0x000000DC]
			jmp dword ptr [_cameraDistanceLockonContinue]
    }
}

_declspec(naked) void cameraAngle_proc(void)
{
    _asm {
			push eax
			mov eax,[cameraAngle]
			mov [edi+0x000000D4],eax
			pop eax
			movss xmm2,[edi+0x000000D4]
			jmp dword ptr [_cameraAngleContinue]
    }
}

_declspec(naked) void ldkWithDMD_proc(void)
{
    _asm {
			cmp byte ptr [g_ldkWithDMDEnable], 0
			je originalcode

			add eax,[edi+0x00000140]
			dec eax

		originalcode:
			add eax,[edi+0x00000140]
			jmp dword ptr [_ldkWithDMDContinue]
    }
}

_declspec(naked) void lockOn_proc(void)
{
    _asm {
			cmp byte ptr [g_trackingFullHouseEnable], 0
			je originalcode

			mov [lockOnAlloc],0x01

		originalcode:
			mov [edi+0x000016D0],0x1
			jmp dword ptr [_lockOnContinue]
    }
}

_declspec(naked) void lockOff_proc(void)
{
    _asm {
			cmp byte ptr [g_trackingFullHouseEnable], 0
			je originalcode

			mov [lockOnAlloc],0x0

		originalcode:
			mov [edi+0x000016D0],0x0
			jmp dword ptr [_lockOffContinue]
    }
}

_declspec(naked) void trackingFullHouse_proc(void)
{
    _asm {
			cmp byte ptr [g_trackingFullHouseEnable], 0
			je originalcode

			cmp [lockOnAlloc],0x1
			je skipcode

		originalcode:
			comiss xmm0, [fullHouseAngle] // Thought I'd try make a new alloc because idk how to comiss xmm0,[00C0F9F4] (which was a
                         // static 65 float)

		skipcode:
			jmp dword ptr [_trackingFullHouseContinue]
    }
}

_declspec(naked) void timerAlloc_proc(void)
{
    _asm {
			cmp byte ptr [g_trickDownEnable], 0
			je originalcode

			movss xmm5,[timerMem]
			addss xmm5,[timerMemTick]
			movss [timerMem],xmm5

		originalcode:
			mov [ecx+0x00000EA8],00000000
			jmp dword ptr [_timerAllocContinue]
    }
}

_declspec(naked) void backForward_proc(void)
{
    _asm {
			cmp byte ptr [g_trickDownEnable], 0
			je originalcode

			cmp al,0x3
			jne dontreplace
			mov byte ptr [backForwardAlloc],al
			cmp [backForwardAlloc],3
			je trickreplace
			jmp dontreplace

		dontreplace:
			mov byte ptr [_tricksterDashMove],0x5B
			mov byte ptr [_skyStarMove],0x5C
			jmp originalcode

		trickreplace:
			mov dword ptr [timerMem],0x00000000
			mov byte ptr [_tricksterDashMove],0x5D
			mov byte ptr [_skyStarMove],0x5D

		originalcode:
			mov [ebp+0x00],al
			jmp dword ptr [_backForwardContinue]
    }
}

_declspec(naked) void trickDown_proc(void)
{
    _asm {
			cmp byte ptr [g_trickDownEnable], 0
			je originalcode

			cmp dword ptr [timerMem],0x42480000
			jl trickdownstart
			jmp originalcode

		trickdownstart:
			movss xmm2,[downFloat]

		originalcode:
			movss [esi+0x00000EC4],xmm2
			jmp dword ptr [_trickDownContinue]
    }
}

_declspec(naked) void floorTouch_proc(void)
{
    _asm {
			cmp byte ptr [g_trickDownEnable], 0
			je originalcode

			cmp dword ptr [timerMem],0x42a00000
			jl skipcode

		originalcode:
			movss [esi+0x00000EC4],xmm2

		skipcode:
			jmp dword ptr [_floorTouchContinue]
    }
}