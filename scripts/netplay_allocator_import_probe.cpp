// Fixture: prove the ABI contract netplay_test::redirect_allocator_imports depends on.
// The game reaches the OS allocators through two IAT cells (IDA: 0x00B850CC VirtualAlloc
// read by `call ds:VirtualAlloc` at 0x008BB561 / 0x008BB6F8 / 0x00B5A9C4 / 0x00B5AA4F, all
// four pushing flProtect, flAllocationType, dwSize, lpAddress; 0x00B850D0 VirtualFree read
// by `call ds:VirtualFree` at 0x008BB5C8 / 0x008BB7DE / 0x008BBBD1E and by `mov esi,
// ds:VirtualFree` (0x00B5A868) + `call esi` (0x00B5A880), all three pushing dwFreeType,
// dwSize, lpAddress). Rewriting only those cells instruments every site - including the
// `call esi` pair no byte patch at a call site could reach - and it only works if the
// tracker adapters keep the imports' exact contract: cdecl, callee cleans its own
// arguments, same arity and order, result in eax. The fixture models each cell as a
// function pointer the call site reads at call time (volatile, so the compiler emits a
// real indirect call through the cell rather than a direct one) and asserts the wrapper
// observed the site's arguments unchanged, esp came back where it started (so the game's
// function continues with its own stack bookkeeping) and the return value flowed back.
#include <windows.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "kernel32.lib")

static int g_alloc_wrapper_calls = 0;
static LPVOID g_alloc_seen_address = nullptr;
static SIZE_T g_alloc_seen_size = 0;
static DWORD g_alloc_seen_type = 0;
static DWORD g_alloc_seen_protect = 0;
static int g_free_wrapper_calls = 0;
static LPVOID g_free_seen_address = nullptr;
static SIZE_T g_free_seen_size = 0;
static DWORD g_free_seen_type = 0;

// The imports, with the imports' exact contract (cdecl, callee cleanup, result in eax).
extern "C" LPVOID WINAPI import_virtual_alloc(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) {
    return ::VirtualAlloc(address, bytes, type, protect);
}

extern "C" BOOL WINAPI import_virtual_free(LPVOID address, SIZE_T bytes, DWORD type) {
    return ::VirtualFree(address, bytes, type) ? TRUE : FALSE;
}

// The tracker adapters netplay_test::page_site_allocate / page_site_release stand in for:
// same arity, same order, same cleanup, result in eax.
extern "C" LPVOID WINAPI probe_site_allocate(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) {
    ++g_alloc_wrapper_calls;
    g_alloc_seen_address = address;
    g_alloc_seen_size = bytes;
    g_alloc_seen_type = type;
    g_alloc_seen_protect = protect;
    return ::VirtualAlloc(address, bytes, type, protect);
}

extern "C" BOOL WINAPI probe_site_release(LPVOID address, SIZE_T bytes, DWORD type) {
    ++g_free_wrapper_calls;
    g_free_seen_address = address;
    g_free_seen_size = bytes;
    g_free_seen_type = type;
    return TRUE;   // the scratch page stays mapped: the fixture never unmaps
}

// The cells. volatile keeps the compiler honest about the call reading the cell's current
// value rather than baking in a target, i.e. the site re-reads the cell per call, exactly
// like `FF 15 <cell>` / `mov esi, ds:cell` + `call esi` do.
static LPVOID(WINAPI* volatile alloc_cell)(LPVOID, SIZE_T, DWORD, DWORD) = &import_virtual_alloc;
static BOOL(WINAPI* volatile free_cell)(LPVOID, SIZE_T, DWORD) = &import_virtual_free;

int main() {
    // Unredirected: the alloc cell still names the import, so the tracker counters stay
    // zero and the caller gets a real page back.
    auto* direct = alloc_cell(nullptr, (SIZE_T)0x2000, MEM_COMMIT, PAGE_READWRITE);
    if (direct == nullptr || g_alloc_wrapper_calls != 0) {
        std::printf("unredirected cell wrong: result=%p wrapper_calls=%d\n", direct,
                    g_alloc_wrapper_calls);
        return 1;
    }
    memset(direct, 0x11, 0x2000);

    // Redirect both cells: only the cell contents change, every read of a cell now lands
    // on the tracker adapter.
    alloc_cell = &probe_site_allocate;
    free_cell = &probe_site_release;

    auto* tracked = alloc_cell(nullptr, (SIZE_T)0x2000, MEM_COMMIT, PAGE_READWRITE);
    auto* scratch = (LPVOID)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT, PAGE_READWRITE);
    if (scratch == nullptr) {
        std::printf("could not prepare the scratch page\n");
        return 1;
    }
    free_cell(scratch, (SIZE_T)0x1000, MEM_RELEASE);

    std::printf("redirect: alloc_wrapper_calls=%d address=%p size=%llx type=%lx protect=%lx "
                "free_wrapper_calls=%d free_address=%p free_size=%llx free_type=%lx\n",
                g_alloc_wrapper_calls, g_alloc_seen_address,
                (unsigned long long)g_alloc_seen_size, g_alloc_seen_type, g_alloc_seen_protect,
                g_free_wrapper_calls, g_free_seen_address,
                (unsigned long long)g_free_seen_size, g_free_seen_type);
    std::fflush(stdout);
    if (tracked == nullptr || g_alloc_wrapper_calls != 1 || g_alloc_seen_address != nullptr
        || g_alloc_seen_size != 0x2000 || g_alloc_seen_type != MEM_COMMIT
        || g_alloc_seen_protect != PAGE_READWRITE) {
        std::printf("redirected alloc cell marshalling wrong\n");
        return 1;
    }
    if (g_free_wrapper_calls != 1 || g_free_seen_address != scratch
        || g_free_seen_size != 0x1000 || g_free_seen_type != MEM_RELEASE) {
        std::printf("redirected free cell marshalling wrong\n");
        return 1;
    }
    memset(tracked, 0x22, 0x2000);
    memset(scratch, 0x33, 0x1000);
    return 0;
}
