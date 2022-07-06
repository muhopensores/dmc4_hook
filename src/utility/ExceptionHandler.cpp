#include <windows.h>
#include <DbgHelp.h>
#include <spdlog/spdlog.h>

#include "utility/Module.hpp"
#include "utility/Scan.hpp"
#include "utility/Patch.hpp"

#include "ExceptionHandler.hpp"

LONG WINAPI reframework::global_exception_handler(struct _EXCEPTION_POINTERS* ei) {
    spdlog::flush_on(spdlog::level::err);

    spdlog::error("Exception occurred: {:x}", ei->ExceptionRecord->ExceptionCode);
    spdlog::error("EIP: {:x}", ei->ContextRecord->Eip);
    spdlog::error("ESP: {:x}", ei->ContextRecord->Esp);
    spdlog::error("ECX: {:x}", ei->ContextRecord->Ecx);
    spdlog::error("EDX: {:x}", ei->ContextRecord->Edx);
    spdlog::error("EAX: {:x}", ei->ContextRecord->Eax);
    spdlog::error("EBX: {:x}", ei->ContextRecord->Ebx);
    spdlog::error("EBP: {:x}", ei->ContextRecord->Ebp);
    spdlog::error("ESI: {:x}", ei->ContextRecord->Esi);
    spdlog::error("EDI: {:x}", ei->ContextRecord->Edi);
    spdlog::error("EFLAGS: {:x}", ei->ContextRecord->EFlags);
    spdlog::error("CS: {:x}", ei->ContextRecord->SegCs);
    spdlog::error("DS: {:x}", ei->ContextRecord->SegDs);
    spdlog::error("ES: {:x}", ei->ContextRecord->SegEs);
    spdlog::error("FS: {:x}", ei->ContextRecord->SegFs);
    spdlog::error("GS: {:x}", ei->ContextRecord->SegGs);
    spdlog::error("SS: {:x}", ei->ContextRecord->SegSs);

    const auto module_within = utility::get_module_within(ei->ContextRecord->Eip);

    if (module_within) {

        const auto module_path = utility::get_module_path(*module_within);

        if (module_path) {
            spdlog::error("Module: {:x} {}", (uintptr_t)*module_within, *module_path);
        } else {
            spdlog::error("Module: Unknown");
        }
    } else {
        spdlog::error("Module: Unknown");
    }

    auto dbghelp = LoadLibrary("dbghelp.dll");

    if (dbghelp) {
        const auto mod_dir = utility::get_module_directory(GetModuleHandle(0));
        const auto real_mod_dir = mod_dir ? (*mod_dir + "\\") : "";
        const auto final_path = real_mod_dir + "dmc4hook_crash.dmp";

        spdlog::error("Attempting to write dump to {}", final_path);

        auto f = CreateFile(final_path.c_str(), 
            GENERIC_WRITE, 
            FILE_SHARE_WRITE, 
            nullptr, 
            CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, 
            nullptr
        );

        if (!f || f == INVALID_HANDLE_VALUE) {
            spdlog::error("Exception occurred, but could not create dump file");
            return EXCEPTION_CONTINUE_SEARCH;
        }

        MINIDUMP_EXCEPTION_INFORMATION ei_info{
            GetCurrentThreadId(),
            ei,
            FALSE
        };

        auto minidump_write_dump = (decltype(MiniDumpWriteDump)*)GetProcAddress(dbghelp, "MiniDumpWriteDump");

        minidump_write_dump(GetCurrentProcess(), 
            GetCurrentProcessId(),
            f,
            MINIDUMP_TYPE::MiniDumpNormal, 
            &ei_info, 
            nullptr, 
            nullptr
        );

        CloseHandle(f);
    } else {
        spdlog::error("Exception occurred, but could not load dbghelp.dll");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void reframework::setup_exception_handler() {
    SetUnhandledExceptionFilter(global_exception_handler);
}
