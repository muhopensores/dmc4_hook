#include "MultipleGameInstances.hpp"
#include "processenv.h"

static uintptr_t jmp_ret = NULL;

static std::unique_ptr<FunctionHook> create_sDevil4Main_mutex;
static char mutex_name[MAX_PATH] = {};


naked void detour() {
    _asm {
        lea eax, [mutex_name]
    originalcode:
        jmp dword ptr [jmp_ret]
    }
}


std::optional<std::string> MultipleGameInstances::on_initialize() {

    const char* args = GetCommandLineA();
    const char* needle = "-mutex ";
    const char* mutex_param = strstr(args, needle);
    
    if(mutex_param == NULL) { 
        return Mod::on_initialize(); 
    }
    
    strcpy(mutex_name, mutex_param + strlen(needle));

    static constexpr auto address = 0x008D8B0B;
    if (!install_hook_absolute(address, create_sDevil4Main_mutex, &detour, &jmp_ret, 5)) {
        spdlog::error("Failed to init MultipleGameInstances mod 1\n");
        return "Failed to init MultipleGameInstances mod";
    }

    return Mod::on_initialize();
}
