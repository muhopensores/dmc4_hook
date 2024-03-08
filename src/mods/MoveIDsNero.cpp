#include "MoveIDsNero.hpp"
#include "MoveIDs.hpp"
#if 1

uintptr_t MoveIdsNero::jmp_return{ NULL };
uint32_t MoveIdsNero::move_id_nero{ 0 };

naked void detour() {
    _asm {
		mov [MoveIdsNero::move_id_nero], esi
        mov [MoveIds::move_id], 0 // for hb knockback
        mov [ecx+0x295C], esi
		jmp dword ptr [MoveIdsNero::jmp_return]
    }
}

std::optional<std::string> MoveIdsNero::on_initialize() {
    if (!install_hook_offset(0x4212B6, hook, &detour, &jmp_return, 6))
    {
        spdlog::error("Failed to init MoveIdsNero mod\n");
        return "Failed to init MoveIdsNero mod";
    }

    return Mod::on_initialize();
}

#endif
