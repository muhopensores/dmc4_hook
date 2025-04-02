#include <thread>
#include <chrono>
#include <windows.h>

#include "ModFramework.hpp"

static HMODULE g_dinput;

#if 1
extern "C" {
	// DirectInput8Create wrapper for dinput8.dll
	__declspec(dllexport) HRESULT WINAPI direct_input8_create(HINSTANCE hinst, DWORD dw_version, const IID& riidltf, LPVOID* ppv_out, LPUNKNOWN punk_outer) {
		// This needs to be done because when we include dinput.h in DInputHook,
		// It is a redefinition, so we assign an export by not using the original name
		#pragma comment(linker, "/EXPORT:DirectInput8Create=_direct_input8_create@20")
		return ((decltype(direct_input8_create)*)GetProcAddress(g_dinput, "DirectInput8Create"))(hinst, dw_version, riidltf, ppv_out, punk_outer);
	}
}
#endif

static void failed() {
	MessageBox(0, "DMC4 ModFramework: Unable to load the original version.dll. Please report this to the developer.", "ModFramework", 0);
	ExitProcess(0);
}

static DWORD WINAPI startup_thread([[maybe_unused]] LPVOID parameter) {
#ifndef NDEBUG
	AllocConsole();
	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD console_mode{ 0 };
	GetConsoleMode( handle_out , &console_mode);
    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    console_mode |= DISABLE_NEWLINE_AUTO_RETURN;            
	SetConsoleMode( handle_out , console_mode );

	FILE* new_stdin  = nullptr;
	FILE* new_stdout = nullptr;
	FILE* new_stderr = nullptr;

	freopen_s(&new_stdin,  "CONIN$",  "r", stdin);
	freopen_s(&new_stdout, "CONOUT$", "w", stdout);
	freopen_s(&new_stderr, "CONOUT$", "w", stderr);
#endif
#if 1
	wchar_t buffer[MAX_PATH]{ 0 };
	if (GetSystemDirectoryW(buffer, MAX_PATH) != 0) {
		// Load the original dinput8.dll
		if ((g_dinput = LoadLibraryW((std::wstring{ buffer } + L"\\dinput8.dll").c_str())) == NULL) {
			failed();
		}
		g_framework = std::make_unique<ModFramework>();
	}
	else {
		failed();
	}
#else
	g_framework = std::make_unique<ModFramework>();
#endif

	return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH) {
#ifndef NDEBUG
		MessageBox(NULL, "Debug attach opportunity", "DMC4", MB_ICONINFORMATION);
#endif
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)startup_thread, nullptr, 0, nullptr);
	}
	if (reason == DLL_PROCESS_DETACH) {
		FreeLibrary(g_dinput);
	}

	// increase global, temp, resource mem
	static DWORD oldProtect;
	static BYTE* globalMem = (BYTE*)0x8AFB3E; // global
	static BYTE* tempMem   = (BYTE*)0x8AFB76; // temp
	static BYTE* resMem    = (BYTE*)0x8AFBAE; // resource

    VirtualProtect(globalMem, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *globalMem = 9; // was push 06000000
    VirtualProtect(globalMem, 1, oldProtect, &oldProtect);

    VirtualProtect(tempMem, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *tempMem = 3; // was push 02000000
    VirtualProtect(tempMem, 1, oldProtect, &oldProtect);

    VirtualProtect(resMem, 1, PAGE_EXECUTE_READWRITE, &oldProtect);
    *resMem = 9; // was push 06000000
    VirtualProtect(resMem, 1, oldProtect, &oldProtect);
	return TRUE;
}