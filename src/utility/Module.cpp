#include <shlwapi.h>

#include "String.hpp"
#include "Module.hpp"

using namespace std;

namespace utility {
    optional<size_t> get_module_size(const string& module) {
        return get_module_size(GetModuleHandle(module.c_str()));
    }

    optional<size_t> get_module_size(HMODULE module) {
        if (module == nullptr) {
            return {};
        }

        // Get the dos header and verify that it seems valid.
        auto dos_header = (PIMAGE_DOS_HEADER)module;

        if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) {
            return {};
        }

        // Get the nt headers and verify that they seem valid.
        auto nt_headers = (PIMAGE_NT_HEADERS)((uintptr_t)dos_header + dos_header->e_lfanew);

        if (nt_headers->Signature != IMAGE_NT_SIGNATURE) {
            return {};
        }

        // OptionalHeader is not actually optional.
        return nt_headers->OptionalHeader.SizeOfImage;
    }

    std::optional<std::string> get_module_directory(HMODULE module) {
        wchar_t file_name[MAX_PATH]{ 0 };
        if (GetModuleFileNameW(module, file_name, MAX_PATH) >= MAX_PATH) {
            return {};
        }

        PathRemoveFileSpecW(file_name);

        return utility::narrow(file_name);
    }

    std::optional<HMODULE> get_module_within(Address address) {
        HMODULE module = nullptr;
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, address.as<LPCSTR>(), &module)) {
            return module;
        }

        return {};
    }

    std::optional<std::string> get_module_path(HMODULE module) {
        wchar_t filename[MAX_PATH]{0};
        if (GetModuleFileNameW(module, filename, MAX_PATH) >= MAX_PATH) {
            return {};
        }

        return utility::narrow(filename);
    }

    optional<uintptr_t> ptr_from_rva(uint8_t* dll, uintptr_t rva) {
        // Get the first section.
        auto dos_header = (PIMAGE_DOS_HEADER)&dll[0];
        auto nt_headers = (PIMAGE_NT_HEADERS)&dll[dos_header->e_lfanew];
        auto section = IMAGE_FIRST_SECTION(nt_headers);

        // Go through each section searching for where the rva lands.
        for (uint16_t i = 0; i < nt_headers->FileHeader.NumberOfSections; ++i, ++section) {
            auto size = section->Misc.VirtualSize;

            if (size == 0) {
                size = section->SizeOfRawData;
            }

            if (rva >= section->VirtualAddress && rva < (section->VirtualAddress + size)) {
                auto delta = section->VirtualAddress - section->PointerToRawData;

                return (uintptr_t)(dll + (rva - delta));
            }
        }

        return {};
    }
}
