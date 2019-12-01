#include "hacklib/Memory.h"
#include <algorithm>
#include <dlfcn.h>
#include <fstream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>


static int ToUnixProt(hl::Protection protection)
{
    int unixProt = PROT_NONE;

    if (protection & hl::PROTECTION_READ)
        unixProt |= PROT_READ;
    if (protection & hl::PROTECTION_WRITE)
        unixProt |= PROT_WRITE;
    if (protection & hl::PROTECTION_EXECUTE)
        unixProt |= PROT_EXEC;
    if (protection & hl::PROTECTION_GUARD)
        throw std::runtime_error("protection not supported");

    return unixProt;
}

static hl::Protection FromUnixProt(int unixProt)
{
    hl::Protection protection = hl::PROTECTION_NOACCESS;

    if (unixProt & PROT_READ)
        protection |= hl::PROTECTION_READ;
    if (unixProt & PROT_WRITE)
        protection |= hl::PROTECTION_WRITE;
    if (unixProt & PROT_EXEC)
        protection |= hl::PROTECTION_EXECUTE;

    return protection;
}


uintptr_t hl::GetPageSize()
{
    static uintptr_t pageSize = 0;

    if (!pageSize)
    {
        pageSize = (uintptr_t)sysconf(_SC_PAGESIZE);
    }

    return pageSize;
}


void* hl::PageAlloc(size_t n, hl::Protection protection)
{
    return mmap(nullptr, n, ToUnixProt(protection), MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void hl::PageFree(void* p, size_t n)
{
    if (!n)
        throw std::runtime_error("you must specify the free size on linux");

    munmap(p, n);
}

void hl::PageProtect(const void* p, size_t n, hl::Protection protection)
{
    // Align to page boundary.
    auto pAligned = (const void*)((uintptr_t)p & ~(hl::GetPageSize() - 1));
    size_t nAligned = (uintptr_t)p - (uintptr_t)pAligned + n;

    // const_cast: The memory contents will remain unchanged, so this is fine.
    mprotect(const_cast<void*>(pAligned), nAligned, ToUnixProt(protection));
}


hl::ModuleHandle hl::GetModuleByName(const std::string& name)
{
    void* handle;
    if (name == "")
    {
        handle = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    }
    else
    {
        handle = dlopen(name.c_str(), RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
    }

    if (handle)
    {
        // This is undocumented, but works and is easy.
        auto hModule = *(hl::ModuleHandle*)handle;
        if (!hModule)
        {
            hModule = (hl::ModuleHandle)0x400000;
        }
        dlclose(handle);
        return hModule;
    }
    else
    {
        return hl::NullModuleHandle;
    }
}

hl::ModuleHandle hl::GetModuleByAddress(uintptr_t adr)
{
    Dl_info info = { 0 };
    dladdr((void*)adr, &info);
    return info.dli_fbase;
}


std::string hl::GetModulePath(hl::ModuleHandle hModule)
{
    Dl_info info = { 0 };
    if (dladdr((void*)hModule, &info) == 0)
    {
        throw std::runtime_error("dladdr failed");
    }
    return info.dli_fname;
}


hl::MemoryRegion hl::GetMemoryByAddress(uintptr_t adr, int pid)
{
    hl::MemoryRegion region;

    auto memoryMap = hl::GetMemoryMap(pid);
    auto itRegion = std::find_if(memoryMap.begin(), memoryMap.end(),
                                 [adr](const hl::MemoryRegion& r) { return adr > r.base && adr < r.base + r.size; });
    if (itRegion != memoryMap.end())
    {
        region = *itRegion;
    }

    return region;
}

std::vector<hl::MemoryRegion> hl::GetMemoryMap(int pid)
{
    std::vector<hl::MemoryRegion> regions;

    if (!pid)
        pid = getpid();

    char fileName[32];
    sprintf(fileName, "/proc/%d/maps", pid);

    std::ifstream file(fileName);

    unsigned long long start, end;
    char flags[32];
    unsigned long long file_offset;
    int dev_major, dev_minor;
    unsigned long long inode;
    char path[512];

    std::string line;
    while (std::getline(file, line))
    {
        path[0] = '\0';
        sscanf(line.c_str(), "%Lx-%Lx %31s %Lx %x:%x %Lu %511s", &start, &end, flags, &file_offset, &dev_major,
               &dev_minor, &inode, path);

        hl::MemoryRegion region;
        region.status = hl::MemoryRegion::Status::Valid;
        region.base = (uintptr_t)start;
        region.size = (size_t)(end - start);
        region.protection = ((flags[0] == 'r') ? hl::PROTECTION_READ : 0) |
                            ((flags[1] == 'w') ? hl::PROTECTION_WRITE : 0) |
                            ((flags[2] == 'x') ? hl::PROTECTION_EXECUTE : 0);
        region.hModule = hl::GetModuleByAddress(region.base);
        region.name = path;

        regions.push_back(region);
    }

    return regions;
}
