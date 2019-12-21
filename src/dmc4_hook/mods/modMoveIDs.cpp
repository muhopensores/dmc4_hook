#include "../mods.h"

// making this variable external so that the other modules can link against it.
// note that we set a value upon declaration only here.
extern "C" int moveID = 0;

namespace modMoveIDs {
	bool    g_moveIDAllocEnable = false;
	static  uintptr_t _moveIDAllocContinue = 0x0083EBDC;
	static  hl::Hooker detour;
	uint8_t references = 0;

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

	bool init(uintptr_t base) {
		uintptr_t moveIDAlloc = base + 0x43EBD6;
		auto hook = detour.hookJMP(moveIDAlloc, 6, &moveIDAlloc_proc);
		if (!hook) {
			return false;
		}
		return true;
	};

	void toggle(bool value) {
		if (value) {
			g_moveIDAllocEnable = true;
			references++;
		}
		else {
			references--;
			if (references < 1) { 
				g_moveIDAllocEnable = false;
				references = 0;
			}
		}
	};
}