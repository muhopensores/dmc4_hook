#include <Windows.h>


LONG WINAPI Our_TopLevelExceptionFilter ( _In_ struct _EXCEPTION_POINTERS *ExceptionInfo );
bool InstallExceptionHandlerHooks();