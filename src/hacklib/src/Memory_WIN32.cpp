#include "hacklib/Memory.h"
#include <Windows.h>
#include <stdexcept>


static DWORD ToWindowsProt(hl::Protection protection)
{
    DWORD windowsProt = 0;

    if (protection & hl::PROTECTION_GUARD)
    {
        protection &= ~hl::PROTECTION_GUARD;
        windowsProt |= PAGE_GUARD;
    }

    switch (protection)
    {
    case hl::PROTECTION_NOACCESS:
        windowsProt |= PAGE_NOACCESS;
        break;
    case hl::PROTECTION_READ:
        windowsProt |= PAGE_READONLY;
        break;
    case hl::PROTECTION_READ_WRITE:
        windowsProt |= PAGE_READWRITE;
        break;
    case hl::PROTECTION_READ_EXECUTE:
        windowsProt |= PAGE_EXECUTE_READ;
        break;
    case hl::PROTECTION_READ_WRITE_EXECUTE:
        windowsProt |= PAGE_EXECUTE_READWRITE;
        break;
    default:
        throw std::runtime_error("protection not supported");
    }

    return windowsProt;
}

static hl::Protection FromWindowsProt(DWORD windowsProt)
{
    hl::Protection protection = hl::PROTECTION_NOACCESS;

    if (windowsProt & PAGE_GUARD)
    {
        protection |= hl::PROTECTION_GUARD;
    }

    // Strip modifiers.
    windowsProt &= ~(PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE);

    switch (windowsProt)
    {
    case PAGE_NOACCESS:
        break;
    case PAGE_READONLY:
        protection |= hl::PROTECTION_READ;
        break;
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
        protection |= hl::PROTECTION_READ_WRITE;
        break;
    case PAGE_EXECUTE:
        protection |= hl::PROTECTION_EXECUTE;
        break;
    case PAGE_EXECUTE_READ:
        protection |= hl::PROTECTION_READ_EXECUTE;
        break;
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        protection |= hl::PROTECTION_READ_WRITE_EXECUTE;
        break;
    default:
        throw std::runtime_error("unknown windows protection");
    }

    return protection;
}


uintptr_t hl::GetPageSize()
{
    static uintptr_t pageSize = 0;

    if (!pageSize)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        pageSize = (uintptr_t)sys_info.dwPageSize;
    }

    return pageSize;
}


void* hl::PageAlloc(size_t n, hl::Protection protection)
{
    return VirtualAlloc(NULL, n, MEM_COMMIT, ToWindowsProt(protection));
}

void hl::PageFree(void* p, size_t n)
{
    VirtualFree(p, 0, MEM_RELEASE);
}

void hl::PageProtect(const void* p, size_t n, hl::Protection protection)
{
    DWORD dwOldProt;

    // const_cast: The memory contents will remain unchanged, so this is fine.
    VirtualProtect(const_cast<LPVOID>(p), n, ToWindowsProt(protection), &dwOldProt);
}


hl::ModuleHandle hl::GetModuleByName(const std::string& name)
{
    if (name == "")
    {
        return GetModuleHandleA(NULL);
    }
    else
    {
        return GetModuleHandleA(name.c_str());
    }
}

hl::ModuleHandle hl::GetModuleByAddress(uintptr_t adr)
{
    hl::ModuleHandle hModule = hl::NullModuleHandle;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (LPCTSTR)adr, &hModule) == 0)
    {
        if (GetLastError() != ERROR_MOD_NOT_FOUND)
        {
            throw std::runtime_error("GetModuleHandleEx failed");
        }
    }

    return hModule;
}

std::string hl::GetModulePath(hl::ModuleHandle hModule)
{
    char path[MAX_PATH];

    if (GetModuleFileNameA(hModule, path, MAX_PATH) == 0)
    {
        throw std::runtime_error("GetModuleFileName failed");
    }

    return path;
}


hl::MemoryRegion hl::GetMemoryByAddress(uintptr_t adr, int pid)
{
    MEMORY_BASIC_INFORMATION mbi = {};
    hl::MemoryRegion region;

    if (pid)
    {
        auto hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProc)
        {
            throw std::runtime_error("OpenProcess failed");
        }
        if (VirtualQueryEx(hProc, (LPCVOID)adr, &mbi, sizeof(mbi)) != sizeof(mbi))
        {
            CloseHandle(hProc);
            return region;
        }
        CloseHandle(hProc);
    }
    else
    {
        if (VirtualQuery((LPCVOID)adr, &mbi, sizeof(mbi)) != sizeof(mbi))
        {
            return region;
        }
    }

    region.base = (uintptr_t)mbi.BaseAddress;
    region.size = mbi.RegionSize;

    if (mbi.State != MEM_COMMIT)
    {
        region.status = hl::MemoryRegion::Status::Free;
        return region;
    }

    region.status = hl::MemoryRegion::Status::Valid;
    region.protection = FromWindowsProt(mbi.Protect);

    if (mbi.Type == MEM_IMAGE && pid == 0)
    {
        region.hModule = hl::GetModuleByAddress(adr);
        region.name = hl::GetModulePath(region.hModule);
    }

    return region;
}

std::vector<hl::MemoryRegion> hl::GetMemoryMap(int pid)
{
    std::vector<hl::MemoryRegion> regions;

    uintptr_t adr = 0;
    auto region = hl::GetMemoryByAddress(adr, pid);
    while (region.status != hl::MemoryRegion::Status::Invalid)
    {
        if (region.status == hl::MemoryRegion::Status::Valid)
        {
            regions.push_back(region);
        }

        adr += region.size;
        region = hl::GetMemoryByAddress(adr, pid);
    }

    return regions;
}