#include "DeterministicSimulation.hpp"
#include "sdk/sDevil4Main.hpp"

static constexpr uint32_t g_sse_val = 0x1F80u;
static constexpr uint16_t g_x87_val = 0x007Fu;
static uintptr_t g_worker_threads_jmp_back = NULL;
naked void detour_worker_threads_init() {
    static constexpr uint16_t g_x87_val = 0x037F;
    _asm {
        fldcw WORD PTR [g_x87_val]
        ldmxcsr DWORD PTR [g_sse_val]

    originalCode:
        push esi
        mov esi, [esp + 08h]

        jmp DWORD PTR [g_worker_threads_jmp_back]
    }
}

static uintptr_t g_sse_init_jmp_back = NULL;
naked void detour_sse_init() {
    _asm {
        ldmxcsr DWORD PTR [g_sse_val]
        jmp DWORD PTR [g_sse_init_jmp_back]
    }
}

static uintptr_t g_sMain_job_proc_jmp_back = NULL;
naked void detour_sMain_job_proc() {
    _asm {
        fldcw WORD PTR [g_x87_val]
        ldmxcsr DWORD PTR [g_sse_val]

    originalCode:
        mov edx,[eax+8Ch]
        jmp DWORD PTR [g_sMain_job_proc_jmp_back]
    }
}

// The game's job thread count is left as the game sets it (sMain's property setter, at start-up): netplay and the rollback
// probe run their ticks with one job thread instead (CoopOnline's MainThreadJobs).
std::optional<std::string> DeterministicSimulation::on_initialize() {
    if (!install_hook_absolute(0x0090E090, g_worker_thread_init_hook, detour_worker_threads_init, &g_worker_threads_jmp_back, 5)) {
        spdlog::error("Failed to hook worker threads init\n");
        return "Failed to hook worker threads init\n";
    }

    if (!install_hook_absolute(0x00A3E252, sse_init_hook, detour_sse_init, &g_sse_init_jmp_back, 5)) {
        spdlog::error("Failed to hook sse init\n");
        return "Failed to hook sse init\n";
    }
    if (!install_hook_absolute(0x0090DDE8, sMain_job_proc_hook, detour_sMain_job_proc, &g_sMain_job_proc_jmp_back, 6)) {
        spdlog::error("Failed to install hook at sMain jobProc");
        return "Failed to install hook at sMain jobProc";
    }
    fpu_global_init = Patch::create(0x00B8BD38, {0x7F, 0x03}, true);
    return Mod::on_initialize();
}
