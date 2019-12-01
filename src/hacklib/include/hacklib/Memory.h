#ifndef HACKLIB_MEMORY_H
#define HACKLIB_MEMORY_H

#include "hacklib/Handles.h"
#include <string>
#include <vector>


namespace hl
{
typedef int Protection;

static const Protection PROTECTION_NOACCESS = 0x0;
static const Protection PROTECTION_READ = 0x1;
static const Protection PROTECTION_WRITE = 0x2;
static const Protection PROTECTION_EXECUTE = 0x4;
static const Protection PROTECTION_GUARD = 0x8; // Only supported on Windows.
static const Protection PROTECTION_READ_WRITE = PROTECTION_READ | PROTECTION_WRITE;
static const Protection PROTECTION_READ_EXECUTE = PROTECTION_READ | PROTECTION_EXECUTE;
static const Protection PROTECTION_READ_WRITE_EXECUTE = PROTECTION_READ_WRITE | PROTECTION_EXECUTE;


// A region of memory on page boundaries with equal protection.
struct MemoryRegion
{
    enum class Status
    {
        // A valid committed memory region. All fields are valid.
        Valid,
        // A free memory region. Only the base and size fields are valid.
        Free,
        // An invalid memory region. This is usually kernel address space. No fields are valid.
        Invalid
    };

    Status status = Status::Invalid;
    uintptr_t base = 0;
    size_t size = 0;
    Protection protection = hl::PROTECTION_NOACCESS;
    hl::ModuleHandle hModule = 0;
    std::string name;
};


uintptr_t GetPageSize();

// Allocate memory on own pages. This is useful when the protection
// is adjusted to prevent other data from being affected.
// PageFree must be used to free the retrieved memory block.
void* PageAlloc(size_t n, Protection protection);
void PageFree(void* p, size_t n = 0);
void PageProtect(const void* p, size_t n, Protection protection);

template <typename T, typename A>
void PageProtectVec(const std::vector<T, A>& vec, Protection protection)
{
    PageProtect(vec.data(), vec.size() * sizeof(T), protection);
}


// An empty string requests the main module.
hl::ModuleHandle GetModuleByName(const std::string& name = "");

// Returns hl::NullModuleHandle when the address does not reside in a module.
hl::ModuleHandle GetModuleByAddress(uintptr_t adr);

std::string GetModulePath(hl::ModuleHandle hModule);


// On linux it is way more performant to use GetMemoryMap than to build
// one using this function.
MemoryRegion GetMemoryByAddress(uintptr_t adr, int pid = 0);

// The resulting memory map only contains regions with Status::Valid.
// A pid of zero can be passed for the current process.
std::vector<MemoryRegion> GetMemoryMap(int pid = 0);
}

#endif
