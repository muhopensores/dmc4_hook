#include "modMoveIDsNero.hpp"
#if 1

uintptr_t MoveIdsNero::jmp_return{ NULL };
uint32_t MoveIdsNero::moveIDNero{ 0 };

MoveIdsNero::MoveIdsNero()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
		mov [MoveIdsNero::moveIDNero], esi
        mov [ecx+295Ch], esi
		jmp dword ptr [MoveIdsNero::jmp_return]
    }
}

std::optional<std::string> MoveIdsNero::onInitialize()
{
    if (!install_hook_offset(0x4212B6, hook, &detour, &jmp_return, 6))
    {
        HL_LOG_ERR("Failed to init MoveIdsNero mod\n");
        return "Failed to init MoveIdsNero mod";
    }

    return Mod::onInitialize();
}
#endif
