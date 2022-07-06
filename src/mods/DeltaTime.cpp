#include "DeltaTime.hpp"
#if 1

uintptr_t DeltaTime::jmp_return{ NULL };
float DeltaTime::current_delta_time{ 0.0f };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void detour()
{
    _asm {
        movss [edi+0x0C],xmm0
        push eax
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        mov eax, [eax+0x24]
        cmp [edi+0x10],eax // entity pointer at + 10 (includes things like like pots, benches etc)
        pop eax
        jne retcode
        movss [DeltaTime::current_delta_time],xmm0 // only get player delta
    retcode:
		jmp dword ptr [DeltaTime::jmp_return]
    }
}

std::optional<std::string> DeltaTime::on_initialize()
{
    if (!install_hook_offset(0x063C94, hook, &detour, &jmp_return, 5))
    {
        spdlog::error("Failed to init DeltaTime mod\n");
        return "Failed to init DeltaTime mod";
    }

    return Mod::on_initialize();
}
#endif
