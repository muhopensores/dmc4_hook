#include <windows.h>
#include <DbgHelp.h>
#include <spdlog/spdlog.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

#include "utility/Module.hpp"
#include "utility/Scan.hpp"
#include "utility/Patch.hpp"

#include "ExceptionHandler.hpp"
#include "StackTrace.hpp"

#include "Console.hpp"

#include "../Config.hpp"

static const char* get_exception_code_info(UINT code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
        return "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
    case EXCEPTION_BREAKPOINT:
        return "A breakpoint was encountered.";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit "
               "values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a "
               "standard floating-point value.";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "The thread tried to divide a floating-point value by a floating-point divisor of zero.";
    case EXCEPTION_FLT_INEXACT_RESULT:
        return "The result of a floating-point operation cannot be represented exactly as a decimal fraction.";
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "This exception represents any floating-point exception not included in this list.";
    case EXCEPTION_FLT_OVERFLOW:
        return "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
    case EXCEPTION_FLT_STACK_CHECK:
        return "The stack overflowed or underflowed as the result of a floating-point operation.";
    case EXCEPTION_FLT_UNDERFLOW:
        return "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "The thread tried to execute an invalid instruction.";
    case EXCEPTION_IN_PAGE_ERROR:
        return "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this "
               "exception might occur if a network connection is lost while running a program over the network.";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "The thread tried to divide an integer value by an integer divisor of zero.";
    case EXCEPTION_INT_OVERFLOW:
        return "The result of an integer operation caused a carry out of the most significant bit of the result.";
    case EXCEPTION_INVALID_DISPOSITION:
        return "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language "
               "such as C should never encounter this exception.";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return "The thread tried to continue execution after a noncontinuable exception occurred.";
    case EXCEPTION_PRIV_INSTRUCTION:
        return "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
    case EXCEPTION_SINGLE_STEP:
        return "A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
    case EXCEPTION_STACK_OVERFLOW:
        return "The thread used up its stack.";
    default:
        return "Unknown exception";
    }
}

namespace {
std::string join_path(const std::string& base, const std::string& leaf) {
    if (base.empty()) {
        return leaf;
    }

    const auto last = base.back();
    if (last == '\\' || last == '/') {
        return base + leaf;
    }

    return base + "\\" + leaf;
}

bool ensure_directory(const std::string& path) {
    if (path.empty()) {
        return false;
    }

    if (CreateDirectoryA(path.c_str(), nullptr) != 0) {
        return true;
    }

    return GetLastError() == ERROR_ALREADY_EXISTS;
}

std::string make_crash_timestamp() {
    SYSTEMTIME st{};
    GetLocalTime(&st);

    std::ostringstream out;
    out << std::setfill('0')
        << std::setw(4) << st.wYear
        << std::setw(2) << st.wMonth
        << std::setw(2) << st.wDay
        << "_"
        << std::setw(2) << st.wHour
        << std::setw(2) << st.wMinute
        << std::setw(2) << st.wSecond
        << "_pid"
        << GetCurrentProcessId();

    return out.str();
}

std::string module_info_for_address(uintptr_t address) {
    const auto module_within = utility::get_module_within(address);

    if (!module_within) {
        return "Module: Unknown\n";
    }

    std::ostringstream out;
    out << fmt::format("Module base: 0x{:x}\n", (uintptr_t)*module_within);
    out << fmt::format("Module offset: 0x{:x}\n", address - (uintptr_t)*module_within);

    const auto module_path = utility::get_module_path(*module_within);
    if (module_path) {
        out << fmt::format("Module path: {}\n", *module_path);
    } else {
        out << "Module path: Unknown\n";
    }

    return out.str();
}

std::string build_exception_report(_EXCEPTION_POINTERS* ei) {
    std::stringstream err_msg_buf;
    err_msg_buf << fmt::format("Exception occurred: 0x{:x}", ei->ExceptionRecord->ExceptionCode) << '\n';
    err_msg_buf << "-= " << get_exception_code_info(ei->ExceptionRecord->ExceptionCode) << " =-" << '\n';
    err_msg_buf << "Please describe what you were doing when DMC4 crashed and send this crash folder to the developers." << '\n';
    err_msg_buf << fmt::format("Process ID: {}", GetCurrentProcessId()) << '\n';
    err_msg_buf << fmt::format("Thread ID: {}", GetCurrentThreadId()) << '\n';
    err_msg_buf << fmt::format("Exception address: 0x{:x}", (uintptr_t)ei->ExceptionRecord->ExceptionAddress) << '\n';

    if (ei->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
        ei->ExceptionRecord->NumberParameters >= 2) {
        const auto operation = ei->ExceptionRecord->ExceptionInformation[0];
        const char* operation_name = "access";
        if (operation == 0) {
            operation_name = "read";
        } else if (operation == 1) {
            operation_name = "write";
        } else if (operation == 8) {
            operation_name = "execute";
        }

        err_msg_buf << fmt::format(
            "Access violation: attempted {} at 0x{:x}",
            operation_name,
            (uintptr_t)ei->ExceptionRecord->ExceptionInformation[1]
        ) << '\n';
    }

    err_msg_buf << fmt::format("EIP: 0x{:x}", ei->ContextRecord->Eip) << '\n';
    err_msg_buf << fmt::format("ESP: 0x{:x}", ei->ContextRecord->Esp) << '\n';
    err_msg_buf << fmt::format("ECX: 0x{:x}", ei->ContextRecord->Ecx) << '\n';
    err_msg_buf << fmt::format("EDX: 0x{:x}", ei->ContextRecord->Edx) << '\n';
    err_msg_buf << fmt::format("EAX: 0x{:x}", ei->ContextRecord->Eax) << '\n';
    err_msg_buf << fmt::format("EBX: 0x{:x}", ei->ContextRecord->Ebx) << '\n';
    err_msg_buf << fmt::format("EBP: 0x{:x}", ei->ContextRecord->Ebp) << '\n';
    err_msg_buf << fmt::format("ESI: 0x{:x}", ei->ContextRecord->Esi) << '\n';
    err_msg_buf << fmt::format("EDI: 0x{:x}", ei->ContextRecord->Edi) << '\n';
    err_msg_buf << fmt::format("EFLAGS: 0x{:x}", ei->ContextRecord->EFlags) << '\n';
    err_msg_buf << fmt::format("CS: 0x{:x}", ei->ContextRecord->SegCs) << '\n';
    err_msg_buf << fmt::format("DS: 0x{:x}", ei->ContextRecord->SegDs) << '\n';
    err_msg_buf << fmt::format("ES: 0x{:x}", ei->ContextRecord->SegEs) << '\n';
    err_msg_buf << fmt::format("FS: 0x{:x}", ei->ContextRecord->SegFs) << '\n';
    err_msg_buf << fmt::format("GS: 0x{:x}", ei->ContextRecord->SegGs) << '\n';
    err_msg_buf << fmt::format("SS: 0x{:x}", ei->ContextRecord->SegSs) << '\n';
    err_msg_buf << module_info_for_address(ei->ContextRecord->Eip);

#ifdef GIT_HASH
    err_msg_buf << fmt::format("Build hash: {}\n", GIT_HASH);
#endif
#ifdef GIT_DATE
    err_msg_buf << fmt::format("Build date: {}\n", GIT_DATE);
#endif

    return err_msg_buf.str();
}

void write_text_file(const std::string& path, const std::string& text) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (file) {
        file << text;
    }
}

bool write_minidump(const std::string& path, _EXCEPTION_POINTERS* ei, HMODULE dbghelp) {
    auto f = CreateFile(
        path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (!f || f == INVALID_HANDLE_VALUE) {
        return false;
    }

    MINIDUMP_EXCEPTION_INFORMATION ei_info{
        GetCurrentThreadId(),
        ei,
        FALSE
    };

    auto minidump_write_dump = (decltype(MiniDumpWriteDump)*)GetProcAddress(dbghelp, "MiniDumpWriteDump");
    const bool ok = minidump_write_dump != nullptr && minidump_write_dump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        f,
        MINIDUMP_TYPE::MiniDumpNormal,
        &ei_info,
        nullptr,
        nullptr
    ) != FALSE;

    CloseHandle(f);
    return ok;
}

void copy_if_present(const std::string& directory, const std::string& crash_dir, const std::string& file_name) {
    CopyFileA(join_path(directory, file_name).c_str(), join_path(crash_dir, file_name).c_str(), FALSE);
}
} // namespace

LONG WINAPI reframework::global_exception_handler(struct _EXCEPTION_POINTERS* ei) {
    spdlog::flush_on(spdlog::level::err);

    auto dbghelp = LoadLibrary("dbghelp.dll");
    const auto mod_dir = utility::get_module_directory(GetModuleHandle(0));
    const auto real_mod_dir = mod_dir ? *mod_dir : "";
    const auto crash_root = join_path(real_mod_dir, "dmc4_hook_crashes");
    const auto crash_dir = join_path(crash_root, make_crash_timestamp());
    const auto report = build_exception_report(ei);

    spdlog::error(report);

    if (!real_mod_dir.empty()) {
        ensure_directory(crash_root);
        ensure_directory(crash_dir);

        write_text_file(join_path(crash_dir, "crash_report.txt"), report);

        if (dbghelp) {
            const auto per_crash_dump = join_path(crash_dir, "crash.dmp");
            const auto legacy_dump = join_path(real_mod_dir, "dmc4hook_crash.dmp");

            spdlog::error("Attempting to write dump to {}", per_crash_dump);

            if (!write_minidump(per_crash_dump, ei, dbghelp)) {
                spdlog::error("Exception occurred, but could not create per-crash dump file");
            }
            write_minidump(legacy_dump, ei, dbghelp);
        } else {
            spdlog::error("Exception occurred, but could not load dbghelp.dll");
        }

        const auto per_crash_log = join_path(crash_dir, LOG_FILENAME);
        const auto legacy_log = join_path(real_mod_dir, LOG_FILENAME);
        if (console == nullptr || !console->dump_file(per_crash_log)) {
            spdlog::error("Could not dump per-crash log file");
        }
        if (console != nullptr) {
            console->dump_file(legacy_log);
        }

        copy_if_present(real_mod_dir, crash_dir, CONFIG_FILENAME);
        copy_if_present(real_mod_dir, crash_dir, "macro.txt");
        copy_if_present(real_mod_dir, crash_dir, "keyboard_macro.txt");
    } else {
        spdlog::error("Exception occurred, but could not resolve game directory");
    }

#ifndef NDEBUG
    MessageBoxA(NULL, report.c_str(), "Caught exception", MB_ICONINFORMATION);
    return EXCEPTION_EXECUTE_HANDLER;
#else
    return EXCEPTION_CONTINUE_SEARCH;
#endif
}

void reframework::setup_exception_handler() {
    SetUnhandledExceptionFilter(global_exception_handler);
}
