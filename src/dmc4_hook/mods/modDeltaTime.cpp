#include "modDeltaTime.hpp"
#if 1

uintptr_t DeltaTime::jmp_return{ NULL };
float DeltaTime::currentDeltaTime{ 0.0f };

DeltaTime::DeltaTime()
{
    // onInitialize();
}

naked void detour()
{
    _asm {
        movss [esi],xmm0
        movss [DeltaTime::currentDeltaTime],xmm0
        pop esi
		jmp dword ptr [DeltaTime::jmp_return]
    }
}

std::optional<std::string> DeltaTime::onInitialize()
{
    if (!install_hook_offset(0x063C9A, hook, &detour, &jmp_return, 5))
    {
        HL_LOG_ERR("Failed to init DeltaTime mod\n");
        return "Failed to init DeltaTime mod";
    }

    return Mod::onInitialize();
}
#endif
