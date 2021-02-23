#include "crash_handler.h"
#include "hacklib/Logging.h"
#include "MinHook.h"
#include <DbgHelp.h>
#if 0
// Link with DbgHelp.lib
#pragma comment ( lib, "dbghelp.lib" )

using SetUnhandledExceptionFilter_pfn = LPTOP_LEVEL_EXCEPTION_FILTER (WINAPI *)(
	_In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
	);

SetUnhandledExceptionFilter_pfn
SetUnhandledExceptionFilter_Original = nullptr;

LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter_Detour (_In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	UNREFERENCED_PARAMETER (lpTopLevelExceptionFilter);

	SetUnhandledExceptionFilter_Original (lpTopLevelExceptionFilter);

	return
		SetUnhandledExceptionFilter_Original (Our_TopLevelExceptionFilter);
}

bool InstallExceptionHandlerHooks() {
	SetUnhandledExceptionFilter(Our_TopLevelExceptionFilter);
	if (MH_CreateHookApi(L"kernel32", "SetUnhandledExceptionFilter", &SetUnhandledExceptionFilter_Detour, (LPVOID*)&SetUnhandledExceptionFilter_Original) == MH_OK) {
		//HL_LOG_RAW("[minhook]: SetUnhandledExceptionFilter(kernel32, SetUnhandledExceptionFilter) returned MH_OK\n");
	}
	else {
		//HL_LOG_ERR("[minhook]: SetUnhandledExceptionFilter(kernel32, SetUnhandledExceptionFilter) failed\n");
		return false;
	}
	if (MH_EnableHook(&SetUnhandledExceptionFilter) == MH_OK) {
		//HL_LOG_RAW("[minhook]: EnableHook(&SetUnhandledExceptionFilter) returned MH_OK\n");
	}
	else {
		//HL_LOG_RAW("[minhook]: EnableHook(&SetUnhandledExceptionFilter) failed\n");
		return false;
	}
	return true;
}

void CreateMiniDump( EXCEPTION_POINTERS* pep ) 
{
	// Open the file 

	HANDLE hFile = CreateFile("MiniDump.dmp", GENERIC_READ | GENERIC_WRITE, 
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ); 

	if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) ) 
	{
		// Create the minidump 

		MINIDUMP_EXCEPTION_INFORMATION mdei; 

		mdei.ThreadId           = GetCurrentThreadId(); 
		mdei.ExceptionPointers  = pep; 
		mdei.ClientPointers     = FALSE; 

		MINIDUMP_TYPE mdt       = (MINIDUMP_TYPE)(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory); 

		BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), 
			hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0 ); 

		if (!rv) {
			HL_LOG_ERR("MiniDumpWriteDump failed. Error: %u \n", GetLastError());
		}
		else {
			HL_LOG_RAW("Minidump created.\n");
		}

		CloseHandle( hFile ); 

	}
	else 
	{
		HL_LOG_ERR("CreateFile failed. Error: %u \n", GetLastError() ); 
	}
}

LONG
WINAPI
Our_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo )
{
	MessageBox (NULL, "Application Crashed! Attempting to create minidump file. Look for MiniDump.dmp file in the game directory",
		"Crash Handler [Abnormal Termination]",
		MB_OK | MB_ICONERROR );

	auto last_ctx = *ExceptionInfo->ContextRecord;
	auto last_exc = *ExceptionInfo->ExceptionRecord;

	CreateMiniDump(ExceptionInfo);

	if ( ExceptionInfo->ExceptionRecord->ExceptionFlags == 0 )
	{
		abort();//return EXCEPTION_CONTINUE_EXECUTION;
	}

	else
	{
		abort();
	}

    //return EXCEPTION_EXECUTE_HANDLER;
}
#endif