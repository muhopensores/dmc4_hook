#include "MoveIDs.hpp"
#include "MoveIDsNero.hpp"
#if 1

uintptr_t  MoveIds::jmp_return{ NULL };
uint32_t   MoveIds::move_id{ 0 };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void detour() {
	_asm {
        push ecx
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp edi, ecx
        pop ecx
        jne code

		mov [MoveIds::move_id], ecx
		mov [MoveIdsNero::move_id_nero], 0 // for hb knockback
	code:
		mov [esi+0x0000225C], ecx
		jmp dword ptr [MoveIds::jmp_return]
	}
}

std::optional<std::string> MoveIds::on_initialize() {
	if (!install_hook_offset(0x43EBD6, hook, &detour, &jmp_return, 6)) {
		spdlog::error("Failed to init MoveIds mod\n");
		return "Failed to init MoveIds mod";
	}

	return Mod::on_initialize();
}

#else
// making this variable external so that the other modules can link against it.
// note that we set a value upon declaration only here.
extern "C" int moveID = 0;

namespace modMoveIDs {
	bool    modInit = false;
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
			return modInit;
		}
		modInit = true;
		return modInit;
	};
	// inc/dec references every time a mod is toggled since it's used in multiple places 
	void toggle(bool value) {
		if (!modInit) { std::runtime_error("toggling modMoveIDs before init"); }
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
#endif
