// Fixture: validate the harness's allocator-site patch convention against a
// compiler generated call site. `synthetic_site` calls the imported VirtualAlloc
// with four arguments, which MSVC emits as the same 6-byte `FF 15 <iat>` indirect
// call the game's allocator wrappers use (IDA at 0x008BB561 / 0x008BB6F8: push
// flProtect, push flAllocationType, push dwSize, push lpAddress, call
// ds:VirtualAlloc). Patching that site with `E8 rel32` straight into a same-arity,
// same-order, std-call-cleanup wrapper plus a `90` (the indirect call is 6 bytes,
// the direct call 5) must preserve the site's arguments, return the allocation in
// eax, and leave the caller undisturbed. netplay_test::page_site_allocate /
// page_site_release are that same-arity wrapper pair, so this fixture covers the
// exact bytes netplay_test::install_allocator_site_patches writes.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdint>

#pragma comment(lib, "kernel32.lib")

static int g_seen_count = 0;
static SIZE_T g_seen_size = 0;
static DWORD g_seen_type = 0;
static DWORD g_seen_protect = 0;
static LPVOID g_seen_address = nullptr;

// Same arity/order/cleanup as the site's own arguments: the patched call needs no
// argument-shuffling thunk at all. This mirrors netplay_test::page_site_allocate.
extern "C" LPVOID WINAPI probe_site_allocate(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) {
    ++g_seen_count;
    g_seen_address = address;
    g_seen_size = bytes;
    g_seen_type = type;
    g_seen_protect = protect;
    return ::VirtualAlloc(address, bytes, type, protect);
}

static __declspec(noinline) void* synthetic_site() {
    return ::VirtualAlloc(NULL, (SIZE_T)0x1000, MEM_COMMIT, PAGE_READWRITE);
}

int main() {
    auto* function_bytes = (BYTE*)&synthetic_site;
    BYTE* site = nullptr;
    for (int scan = 0; scan < 96 && site == nullptr; ++scan) {
        auto* candidate = function_bytes + scan;
        if (candidate[0] == 0xFF && candidate[1] == 0x15 && candidate[5] == 0x00) {
            site = candidate;
        }
    }
    if (site == nullptr) {
        std::printf("synthetic_site: no FF 15 indirect call emitted\n");
        return 1;
    }
    std::printf("site at %p bytes: %02X %02X %02X %02X %02X %02X\n", (void*)site, site[0],
                site[1], site[2], site[3], site[4], site[5]);
    std::fflush(stdout);
    DWORD old_protect = 0;
    if (!VirtualProtect(site, 6, PAGE_EXECUTE_READWRITE, &old_protect)) {
        std::printf("VirtualProtect on the site failed: %lu\n", GetLastError());
        return 1;
    }
    const auto target = (uintptr_t)&probe_site_allocate;
    const int32_t relative = (int32_t)(target - ((uintptr_t)site + 5));
    site[0] = 0xE8;
    site[1] = (BYTE)(relative & 0xFF);
    site[2] = (BYTE)((relative >> 8) & 0xFF);
    site[3] = (BYTE)((relative >> 16) & 0xFF);
    site[4] = (BYTE)((relative >> 24) & 0xFF);
    site[5] = 0x90;
    FlushInstructionCache(GetCurrentProcess(), site, 6);
    std::printf("patched site: E8 rel32 90, target %p\n", (void*)target);
    std::fflush(stdout);
    void* made = synthetic_site();
    std::fflush(stdout);
    std::printf("wrapper calls=%d address=%p size=%llx type=%lx protect=%lx result=%p\n",
                g_seen_count, g_seen_address, (unsigned long long)g_seen_size, g_seen_type,
                g_seen_protect, made);
    std::fflush(stdout);
    if (made == nullptr || g_seen_count != 1 || g_seen_size != 0x1000
        || g_seen_type != MEM_COMMIT || g_seen_protect != PAGE_READWRITE
        || g_seen_address != nullptr) {
        std::printf("patched site marshalling wrong\n");
        return 1;
    }
    memset(made, 0x22, 0x1000);
    return 0;
}
