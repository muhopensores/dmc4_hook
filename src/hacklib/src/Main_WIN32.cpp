#include "hacklib/Main.h"
#include <Windows.h>


static DWORD WINAPI ThreadFunc(LPVOID param)
{
    auto self = (hl::StaticInitImpl*)param;
    self->mainThread();
    return 0;
}

void hl::StaticInitImpl::runMainThread()
{
    // Must use WinAPI threads instead of std threads, because current std
    // implementation blocks within the loader lock.
#ifdef _DEBUG
	MessageBox(NULL, "Now's a good time to attach a debugger.", "DEBUG BUILD", MB_ICONINFORMATION);
#endif
    HANDLE hThread = CreateThread(NULL, 0, ThreadFunc, (LPVOID)this, 0, NULL);
    if (hThread == NULL)
    {
        throw std::runtime_error(std::string("CreateThread failed with code ") + std::to_string(GetLastError()));
    }
    else
    {
        // Thread will be exited by suiciding with FreeLibraryAndExitThread.
        CloseHandle(hThread);
    }
}

void hl::StaticInitImpl::unloadSelf()
{
    FreeLibraryAndExitThread(hl::GetCurrentModule(), 0);
}
