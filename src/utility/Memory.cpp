#include <optional>
#include <vector>

#include <Windows.h>

#include "String.hpp"
#include "Memory.hpp"

using namespace std;

namespace utility {
    thread_local vector<MEMORY_BASIC_INFORMATION> g_pages{};

    optional<MEMORY_BASIC_INFORMATION> find_in_cache(uintptr_t ptr, size_t len) {
        for (const auto& mbi : g_pages) {
            auto start = (uintptr_t)mbi.BaseAddress;
            auto end = start + mbi.RegionSize;

            if (start <= ptr && ptr + len < end) {
                return mbi;
            }
        }
        
        return {};
    }

    bool memory_has_access(const MEMORY_BASIC_INFORMATION& mbi, DWORD protect) {
        // Pages are commited, not guarded or no access, and same protect.
        return (mbi.State & MEM_COMMIT && 
                !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) && 
                mbi.Protect & protect);
    }

    bool is_good_ptr(uintptr_t ptr, size_t len, uint32_t access) {
        // See if the address is in our cache.
        auto cached_mbi = find_in_cache(ptr, len);

        if (cached_mbi) {
            return memory_has_access(*cached_mbi, access);
        }
        
        MEMORY_BASIC_INFORMATION mbi{};

        if (VirtualQuery((LPCVOID)ptr, &mbi, sizeof(mbi)) == 0) {
            return false;
        }

        // Add it to our cache if its not there.
        if (!find_in_cache((uintptr_t)mbi.BaseAddress, 0)) {
            g_pages.push_back(mbi);
        }

        return memory_has_access(mbi, access);
    }

    bool is_good_read_ptr(uintptr_t ptr, size_t len) {
        return is_good_ptr(ptr, len, PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
    }

    bool is_good_write_ptr(uintptr_t ptr, size_t len) {
        return is_good_ptr(ptr, len, PAGE_READWRITE | PAGE_EXECUTE_READWRITE);
    }

    bool is_good_code_ptr(uintptr_t ptr, size_t len) {
        return is_good_ptr(ptr, len, PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE);
    }
}