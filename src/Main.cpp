#include <windows.h>
#include <DbgHelp.h>

#include "ModFramework.hpp"
#include "utility/FunctionHook.hpp"

#ifndef NDEBUG
#include "mods/MultipleGameInstances.hpp"

static MultipleGameInstances* multiple_instances_mod = nullptr;
#endif // !NDEBUG

static HMODULE g_dinput;

struct Arena {
    char* beg { nullptr };
    char* end { nullptr };

    void* alloc(Arena* a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count) {

        ptrdiff_t padding   = -(uintptr_t)a->beg & (align - 1);
        ptrdiff_t available = a->end - a->beg - padding;

        if (available < 0 || count > available / size) {
            abort();
        }

        void* p = a->beg + padding;
        a->beg += padding + count * size;

        return memset(p, 0, count * size);
    }
};

Arena g_arena;

extern "C" {
// DirectInput8Create wrapper for dinput8.dll
__declspec(dllexport) HRESULT WINAPI direct_input8_create(
    HINSTANCE hinst, DWORD dw_version, const IID& riidltf, LPVOID* ppv_out, LPVOID* punk_outer) {
// This needs to be done because when we include dinput.h in DInputHook,
// It is a redefinition, so we assign an export by not using the original name
#pragma comment(linker, "/EXPORT:DirectInput8Create=_direct_input8_create@20")
    return ((decltype(direct_input8_create)*)GetProcAddress(g_dinput, "DirectInput8Create"))(
        hinst, dw_version, riidltf, ppv_out, punk_outer);
}
}

#if 0
static bool check_pe_section(const char* section_name) noexcept {
    HMODULE h_module                    = GetModuleHandle(NULL);
    IMAGE_NT_HEADERS* p_nt_hdr          = ImageNtHeader(h_module); // get PE info
    IMAGE_SECTION_HEADER* p_section_hdr = (IMAGE_SECTION_HEADER*)(p_nt_hdr + 1);

    for (int i = 0; i < p_nt_hdr->FileHeader.NumberOfSections; i++) {
        char* name = (char*)p_section_hdr->Name;
        if (strncmp(name, section_name, strlen(section_name)) == 0) {
            return true;
        }
        p_section_hdr++;
    }

    return false;
}
#endif

static void apply_laa_patch()
{
    HMODULE h_executable = GetModuleHandle(NULL);
    if (!h_executable) return;

    PIMAGE_DOS_HEADER dos_header = (PIMAGE_DOS_HEADER)h_executable;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE) return;

    PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)((BYTE*)h_executable + dos_header->e_lfanew);
    if (nt_header->Signature != IMAGE_NT_SIGNATURE) return;

    DWORD oldProtect;
    if (VirtualProtect(&nt_header->FileHeader.Characteristics, sizeof(WORD), PAGE_READWRITE, &oldProtect))
    {
        nt_header->FileHeader.Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
        VirtualProtect(&nt_header->FileHeader.Characteristics, sizeof(WORD), oldProtect, &oldProtect);
    }
}

static bool reserve_memory_virtual_alloc2() {
    return false;
    static constexpr auto get_new_memory_api_fptr = [](const char* procname) {
        HMODULE handle = GetModuleHandleA("KernelBase.dll");
        if (!handle) {
            return (FARPROC)nullptr;
        }
        return GetProcAddress(handle, procname);
    };

    using va2 = PVOID WINAPI(HANDLE Process, PVOID Base, SIZE_T Size, ULONG AllocType, ULONG Prot, MEM_EXTENDED_PARAMETER*, ULONG);
    va2* _VirtualAlloc2 = (va2*)get_new_memory_api_fptr("VirtualAlloc2");
    
    if (!_VirtualAlloc2) {
        return false;
    }

#if 0
    using mvof3 = PVOID(HANDLE Handle, HANDLE Process, PVOID Base, ULONG64 Off, SIZE_T ViewSize, ULONG AllocType, ULONG Prot, MEM_EXTENDED_PARAMETER*, ULONG);
    mvof3* _MapViewOfFile3 = (mvof3*)get_new_memory_api_fptr("MapViewOfFile3");

    using uvof2 = BOOL(HANDLE Process, PVOID BaseAddress, ULONG UnmapFlags);
    uvof2* _UnmapViewOfFile2 = (uvof2*)get_new_memory_api_fptr("UnmapViewOfFile2");
#endif

    static constexpr size_t mt_mem_size = 0x1D000000ul;
    
    if (char* mem = (char*)_VirtualAlloc2(nullptr, (void*)0x80000000ul, mt_mem_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE, nullptr, NULL)) {
        g_arena.beg = mem;
        g_arena.end = g_arena.beg + mt_mem_size;
        char buffer[MAX_PATH] = { 0 };
        sprintf(buffer, "mt_mem_start: %p;\nmt_mem_end : %p;", (void*)g_arena.beg, (void*)g_arena.end);
        MessageBoxA(NULL, buffer, "DMC4", MB_ICONINFORMATION);
    }
}

static void patch_more_memories() {
    // increase global, temp, resource mem
    // gets gatted by I assume steam drm, works on nonsteam exe
    static DWORD oldProtect;

    static UINT* globalMem = (UINT*)0x8AFB3B; // global
    static UINT* tempMem   = (UINT*)0x8AFB73; // temp
    static UINT* resMem    = (UINT*)0x8AFBAB; // resource

    VirtualProtect(globalMem, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    *globalMem = 0x0A000000; // was push 06000000
    VirtualProtect(globalMem, 4, oldProtect, &oldProtect);

    VirtualProtect(tempMem, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    *tempMem = 0x03000000; // was push 02000000
    VirtualProtect(tempMem, 4, oldProtect, &oldProtect);

    VirtualProtect(resMem, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
    *resMem = 0x10000000; // was push 06000000
    VirtualProtect(resMem, 4, oldProtect, &oldProtect);
}

static void failed() {
    MessageBox(0, "DMC4 ModFramework: Unable to load the original dinput8.dll. Please report this to the developer.", "ModFramework", 0);
    ExitProcess(0);
}

static bool load_original_dinput8() {

    wchar_t buffer[MAX_PATH]{0};

    if (GetSystemDirectoryW(buffer, MAX_PATH) != 0) {
        // Load the original dinput8.dll
        if ((g_dinput = LoadLibraryW((std::wstring{buffer} + L"\\dinput8.dll").c_str())) == NULL) { // NOLINT
            goto failerino;
        }
        return true;
    }

failerino:
    failed();
    return false;
}

#ifndef NDEBUG
static bool reopen_console_stream(FILE** new_stream, const char* name, const char* mode, FILE* stream) {
    errno_t err = freopen_s(new_stream, name, mode, stream);
    return err != 0;
}
#endif
//static DWORD WINAPI startup_thread([[maybe_unused]] LPVOID parameter) {
static void WINAPI startup_proc() {

#ifndef NDEBUG // TODO(deep): freopen_s fails for me in debug, doubt anyone else cares though
    AllocConsole();
    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode{0};
    GetConsoleMode(handle_out, &console_mode);
    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    console_mode |= DISABLE_NEWLINE_AUTO_RETURN;
    SetConsoleMode(handle_out, console_mode);

    FILE* new_stdin  = nullptr;
    FILE* new_stdout = nullptr;
    FILE* new_stderr = nullptr;

    bool success = false;
    success &= reopen_console_stream(&new_stdin,  "CONIN$",  "r", stdin);
    success &= reopen_console_stream(&new_stdout, "CONOUT$", "w", stdout);
    success &= reopen_console_stream(&new_stderr, "CONOUT$", "w", stderr);
    if (!success) {
        OutputDebugStringA("failed to reopen conhost streams");
    }
#endif

    patch_more_memories();

    g_framework = std::make_unique<ModFramework>();

    return /*ERROR_SUCCESS*/;
}

static std::unique_ptr<FunctionHook> g_start_hook;
static int start_hook() {
    static constexpr uintptr_t security_init_cookie_ = 0x00B5D11A;
    _asm { call security_init_cookie_ }

#ifndef NDEBUG
    assert(g_start_hook);

    multiple_instances_mod = new MultipleGameInstances();
    auto err               = multiple_instances_mod->on_initialize();
#endif // !NDEBUG

    startup_proc();
    //reserve_memory_virtual_alloc2();

    static constexpr uintptr_t tmain_CRT_startup_ = 0x00B53BBA;
    /*return*/ __asm { jmp  tmain_CRT_startup_ }
}


static uintptr_t g_steam_drm_jump_back = 0x00EE6987;
#undef naked
static __declspec(naked) void start_hook_steam() {
    __asm {
        //pushad
        //call startup_proc
        //popad
        //mov eax,[ebp-430h]
        mov eax, start_hook
        jmp DWORD PTR [g_steam_drm_jump_back]
    }
}

static bool is_executable_addr(const uintptr_t addr)
{
    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery((void*)addr, &mbi, sizeof(mbi)) != sizeof(mbi))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))
        return false;

    DWORD protect = mbi.Protect & 0xFF;

    return protect == PAGE_EXECUTE ||
           protect == PAGE_EXECUTE_READ ||
           protect == PAGE_EXECUTE_READWRITE ||
           protect == PAGE_EXECUTE_WRITECOPY;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) { //NOLINT
    if (reason == DLL_PROCESS_ATTACH) {
#ifndef NDEBUG
        MessageBox(NULL, "Debug attach opportunity", "DMC4", MB_ICONINFORMATION);
#endif

        assert(DisableThreadLibraryCalls(handle));
        FunctionHook::set_mh_skip_locks(TRUE);

        //apply_laa_patch();
        static constexpr uintptr_t STEAM_STUB_EPILOGUE = 0x00EE6981;

        if (is_executable_addr(STEAM_STUB_EPILOGUE)) { // fucking steam
            g_start_hook = std::make_unique<FunctionHook>(STEAM_STUB_EPILOGUE, &start_hook_steam);
        }
        else { // fucking no steam
            g_start_hook = std::make_unique<FunctionHook>(0x00B53D9A, &start_hook);
        }
        g_start_hook->create();

        load_original_dinput8();
        assert(g_dinput != NULL);

        //CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)startup_thread, nullptr, 0, nullptr);
    }
    if (reason == DLL_PROCESS_DETACH) {
        FreeLibrary(g_dinput);
#ifndef NDEBUG
        delete multiple_instances_mod;
#endif // !NDEBUG

    }
    return TRUE;
}
