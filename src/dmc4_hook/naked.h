#pragma once


extern "C"
{
    const float limitadjust = 0.0f;
    int moveID = 0;
    int lockOnAlloc = 0;
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
	float enemyHPDisplay = 0.0f;
    uintptr_t _damagemodifierContinue = NULL;
	uintptr_t _orbDisplayContinue = 0x004FDD3C;
    uintptr_t _infiniteDTContinue = NULL;
    uintptr_t _infinitePlayerHealthContinue = NULL;
    uintptr_t _berialDazeContinue = NULL;
    uintptr_t _moveIDAllocContinue = 0x0083EBDC;
    uintptr_t _selectiveCancelsContinue = 0x0080332F;
    uintptr_t _stunAnythingContinue = NULL;
    uintptr_t _cameraHeightContinue = NULL;
    uintptr_t _cameraDistanceContinue = NULL;
    uintptr_t _cameraDistanceLockonContinue = NULL;
    uintptr_t _cameraAngleContinue = NULL;
    uintptr_t _ldkWithDMDContinue = 0x00494AF3;
    uintptr_t _lockOnContinue = NULL;
    uintptr_t _lockOffContinue = NULL;
    uintptr_t _trackingFullHouseContinue = 0x007D3478;
    uintptr_t _timerAllocContinue = 0x007AD774;
    uintptr_t _backForwardContinue = 0x00805A60; // This isn't return, its the next opcode
    uintptr_t _trickDownContinue = 0x007CB121;
    uintptr_t _floorTouchContinue = 0x007CB345;
    uintptr_t _roseRemovesPinsContinue = 0x008158CD;
    uintptr_t _roseRemovesPinsJE = 0x00815970;
    uintptr_t _noHelmBreakerKnockbackContinue = 0x0051C389;
    uintptr_t _noHelmBreakerKnockbackJE = 0x0051C367;

	//bools to check for toggling purposes within a gui checkbox, if false, then checkbox toggle is required to activate
    bool g_InfDTEnable = false;
    bool g_InfPlayerHealthEnable = false;
    bool g_berialDazeEnable = false;
    bool g_moveIDAllocEnable = false;
    bool g_selectiveCancelsEnable = false;
		bool g_ecstasyCancelEnable = false;
		bool g_argumentCancelEnable = false;
		bool g_kickThirteenCancelEnable = false;
		bool g_slashDimensionCancelEnable = false;
		bool g_propCancelEnable = false;
		bool g_shockCancelEnable = false;
		bool g_omenCancelEnable = false;
    bool g_damageModifierEnable = false;
	bool g_orbDisplayEnable = false;
    bool g_stunAnythingEnable = false;
    bool g_ldkWithDMDEnable = false;
    bool g_trackingFullHouseEnable = false;
    bool g_trickDownEnable = false;
    bool g_honeyCombEnable = false;
	bool g_roseRemovesPinsEnable = false;
	bool g_noHelmBreakerKnockbackEnable = false;
}

//procs that apply changes directly using inline asm, work in conjunction with hooks
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
			movss [enemyHPDisplay],xmm0		// Writes to an address we'll use for orb display. In originalcode so its not dependent on this checkbox
            jmp dword ptr [_damagemodifierContinue]
    }
}

_declspec(naked) void orbDisplay_proc(void)
{
    _asm {
			cmp byte ptr [g_orbDisplayEnable],0
			je originalcode

			//cmp dword ptr [enemyHPDisplay],0x00000000	// Check to see if enemy is dead.
			// checking to see if enemyHP >= 0
			// this clobbers xmm0 register but it does
			// not seem to affect the game? not sure.
			xorps xmm0, xmm0
			comiss xmm0, DWORD PTR [enemyHPDisplay]
			jae originalcode									// If yes, show default Orb Count

			cvttss2si eax, [enemyHPDisplay] // If no, write Enemy HP Display to orbs rather than Orb Count	// cvttss2si
			push ebx //the push ebx instruction was missing
			jmp dword ptr [_orbDisplayContinue]

			originalcode:
			mov eax,[eax+0x00000114]
			push ebx //the push ebx instruction was missing
            jmp dword ptr [_orbDisplayContinue]
    }
}

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

_declspec(naked) void moveIDAlloc_proc(void)
{
    _asm {
			cmp byte ptr [g_moveIDAllocEnable], 0
			je originalcode

			mov [moveID],ecx

		originalcode:
			//mov [esi+0x0000225C],ecx
			jmp dword ptr [_moveIDAllocContinue]
    }
}

_declspec(naked) void selectiveCancels_proc(void)
{
    _asm {
			cmp byte ptr [g_selectiveCancelsEnable], 0
			je originalcode

			cmp byte ptr [esi+0x13C],0xFFFFFFFF
			jne originalcode
			cmp byte ptr [esi+0x144],0xFFFFFFFF
			jne originalcode

			cmp [moveID],0x411				// Grounded Ecstasy	// Checks move id like usual every tick
			je cancellableecstasy				// If correct moveid, check against Gui:
			cmp [moveID],0x412						// Aerial Ecstasy
			je cancellableecstasy
			cmp [moveID],0x732						// Argument
			je cancellableargument
			cmp [moveID],0x30E						// Kick 13
			je cancellablekickthirteen
			cmp [moveID],0x30F						// DT Kick 13
			je cancellablekickthirteen
			cmp [moveID],0x900						// Slash Dimension
			je cancellableslashdimension
			cmp [moveID],0x232						// Prop
			je cancellableprop
			cmp [moveID],0x333						// Shock
			je cancellableshock
			cmp [moveID],0x735						// Omen
			je cancellableomen
			jmp originalcode

			cancellableecstasy:
			cmp [g_ecstasyCancelEnable],0x1	// If Gui is ticked,
			je cancellable						// make the move cancellable
			jmp originalcode						// if not, don't make it cancellable

			cancellableargument:
			cmp [g_argumentCancelEnable],0x1
			je cancellable
			jmp originalcode

			cancellablekickthirteen:
			cmp [g_kickThirteenCancelEnable],0x1
			je cancellable
			jmp originalcode

			cancellableslashdimension:
			cmp [g_slashDimensionCancelEnable],0x1
			je cancellable
			jmp originalcode

			cancellableprop:
			cmp [g_propCancelEnable],0x1
			je cancellable
			jmp originalcode

			cancellableshock:
			cmp [g_shockCancelEnable],0x1
			je cancellable
			jmp originalcode

			cancellableomen:
			cmp [g_omenCancelEnable],0x1
			je cancellable
			jmp originalcode

		cancellable:
			mov dword ptr [esi+0x8C],0x02	// only movs to [esi+8C] after filtering out anything that doesn't have [esi+13C],FFFFFFFF and [esi+144],FFFFFFFF
												// a change of cmps would allow for different types of cancels such as cancelling an animation with walking or another attack
		originalcode:								// buffers are also used around this area - the je a few bytes down used to be an inconvenience
			mov edi,0x00000008				// originalcode has nothing to do with our newmem, just a convenient jmp point so always run
			jmp dword ptr [_selectiveCancelsContinue]
    }
}

_declspec(naked) void cameraHeight_proc(void)
{
    _asm {
			push eax
			mov eax,[cameraHeight]
			mov [edi+0x000000D0], eax
			pop eax
			//movss xmm0,[edi+0x000000D0]
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
			//movss xmm3,[esi+0x000000D8]
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
			//movss xmm1,[ebx+0x000000DC]
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
			//movss xmm2,[edi+0x000000D4]
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
			mov [lockOnAlloc],0x01			// mov lock on state 0/1 to an address we can access anywhere
			//mov [edi+0x000016D0],0x1			// this is never toggled off, so can be used whenever and doesn't rely on any checkbox
			jmp dword ptr [_lockOnContinue]
    }
}

_declspec(naked) void lockOff_proc(void)
{
    _asm {
			mov [lockOnAlloc],0x0
			//mov [edi+0x000016D0],0x0
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
			comiss xmm0, [fullHouseAngle]	// 65 float

		skipcode:
			jmp dword ptr [_trackingFullHouseContinue]
    }
}

_declspec(naked) void timerAlloc_proc(void) // This + the next 3 are what make up down trick
{
    _asm {
		// timer
			cmp byte ptr [g_trickDownEnable], 1
			je timerstart
			cmp byte ptr [g_honeyCombEnable],1
			je timerstart
			jmp originalcode				// if nothing is using the timer, skip it	// haven't tested
		timerstart:
			movss xmm5,[timerMem]
			addss xmm5,[timerMemTick]		// Timer starts at 0, has a 1 added to it every tick and is reset every time a backforward input is made
			movss [timerMem],xmm5			// Would like to add a framerate compare here to make it not framerate variable but only 1 xmm was unused

		// trick down check
			cmp byte ptr [g_trickDownEnable], 1
			je tricktimercompare
			jmp honeycombcheck

		tricktimercompare:
			cmp [timerMem],0x41a00000		// 20
			jl replacetrick					// If trick down is enabled, replace trickster dash and sky star to trick when timer is under (float)x
			jmp dontreplacetrick			// By putting replacements on a timer you make a buffer for the input and have a convenient off state

		replacetrick:
			cmp [lockOnAlloc],0x0
			je dontreplacetrick
			push eax
			mov eax,0x00C413A4				// Trickster Dash
			mov dword ptr [eax],0x5D		// Trick
			mov eax,0x00C413DC				// Sky Star
			mov dword ptr [eax],0x5D		// Trick
			pop eax
			jmp honeycombcheck

		dontreplacetrick:
			push eax
			mov eax,0x00C413A4				// Trickster Dash
			mov dword ptr [eax],0x5B		// Trickster Dash
			mov eax,0x00C413DC				// Sky Star
			mov dword ptr [eax],0x5C		// Sky Star
			pop eax

		honeycombcheck:
			cmp byte ptr [g_honeyCombEnable],1
			je honeycombtimercompare		// If instant honeycomb is enabled, replace twosome time with honeycomb fire when timer is under (float)x
			jmp dontreplacetwosome

		honeycombtimercompare:
			cmp [timerMem],0x41a00000		// 20
			jl replacetwosome
			jmp dontreplacetwosome

		replacetwosome:
			cmp [lockOnAlloc],0x0
			je dontreplacetwosome
			push eax
			mov eax,0x00C40DBC				// Twosome Time
			mov dword ptr [eax],0x45		// Honeycomb Fire
			pop eax
			jmp originalcode

		dontreplacetwosome:
			push eax
			mov eax,0x00C40DBC				// Twosome Time
			mov dword ptr [eax],0x44		// Twosome Time
			pop eax

		originalcode:
			mov [ecx+0x00000EA8],00000000
			jmp dword ptr [_timerAllocContinue]
    }
}

_declspec(naked) void backForward_proc(void)
{
    _asm {
		// trickcompare
			cmp byte ptr [g_trickDownEnable],0
			je honeycombcompare

			cmp [timerMem],0x41200000			//=10	// If timer is less than 10, don't reset. With this, timer is only reset once per backforward.
			jl originalcode									// 10 is over before the moves can finish, so no worry them not being available
			cmp al,0x3
			je resettimer

		honeycombcompare:
			cmp byte ptr [g_honeyCombEnable],0
			je originalcode

			cmp [timerMem],0x41200000			//=10
			jl originalcode
			cmp al,0x3
			je resettimer
			jmp originalcode

		resettimer:
			mov dword ptr [timerMem],0x00000000	//=0

		originalcode:
			mov [ebp+0x00],al
			jmp dword ptr [_backForwardContinue]// next opcode was jmp x so rather than returning I do that
    }
}

_declspec(naked) void trickDown_proc(void)
{
    _asm {
			cmp byte ptr [g_trickDownEnable], 0
			je originalcode

			cmp dword ptr [timerMem],0x42480000	//=50 //0x41a00000 //=20	// if timer is less than x, put x in y axis momentum
			jl trickdownstart
			jmp originalcode

		trickdownstart:
			movss xmm2,[downFloat]				// currently (float)-200. This may change, as when you're high enough trick won't reach the floor

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

			cmp dword ptr [timerMem],0x42a00000	//=80	// if timer is less than x, skip the height adjust at the end of trick
			jl skipcode										// this has to be higher than the start to account for how long trick takes to start up and play out

		originalcode:
			movss [esi+0x00000EC4],xmm2

		skipcode:
			jmp dword ptr [_floorTouchContinue]
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

_declspec(naked) void noHelmBreakerKnockback_proc(void)
{
    _asm {
			cmp byte ptr [g_noHelmBreakerKnockbackEnable],0
			je originalcode

			cmp [moveID],0x20A	// 522 // Low
			je newcode
			cmp [moveID],0x213	// 531 // Mid
			je newcode
			cmp [moveID],0x214	// 532 // High
			je newcode
			jmp originalcode

		newcode:
			cmp ecx,0x05
			je nohelmbreakerknockbackje
			jmp dword ptr [_noHelmBreakerKnockbackContinue]

		nohelmbreakerknockbackje:
			jmp dword ptr [_noHelmBreakerKnockbackJE]

		originalcode:
			cmp ecx,0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [_noHelmBreakerKnockbackContinue]
    }
}