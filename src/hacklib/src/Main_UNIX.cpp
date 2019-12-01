#include "hacklib/Main.h"
#include "hacklib/MessageBox.h"
#include "hacklib/PageAllocator.h"
#include <cstring>
#include <dlfcn.h>
#include <pthread.h>
#include <thread>


void FreeLibAndExitThread(void* hModule, int (*adr_dlclose)(void*), void (*adr_pthread_exit)(void*))
{
    // This can not be executed from inside the module.
    // Don't generate any code that uses relative addressing to the IP.
    adr_dlclose(hModule);
    adr_pthread_exit((void*)0);
}
void FreeLibAndExitThread_after() {}

void hl::StaticInitImpl::runMainThread()
{
    std::thread th(&StaticInitImpl::mainThread, this);
    th.detach();
}

void hl::StaticInitImpl::unloadSelf()
{
    // Get own module handle by path name. The dlclose just restores the refcount.
    auto modName = hl::GetCurrentModulePath();
    auto hModule = dlopen(modName.c_str(), RTLD_NOW | RTLD_LOCAL);
    dlclose(hModule);

    /*
    Linux does not have an equivalent to FreeLibraryAndExitThread, so a race between
    dlclose and pthread_exit would exist and lead to crashes.

    The following method of detaching leaks a page = 4KiB each time, but is 100% reliable.
    Alternative: Start a thread on dlclose. => Too unreliable, even with priority adjustments.
    Alternative: Let the injector do dlclose. => Signaling mechanism needed; injector might die or be killed.
    */

    size_t codeSize = (size_t)((uintptr_t)&FreeLibAndExitThread_after - (uintptr_t)&FreeLibAndExitThread);
    hl::code_page_vector code(codeSize);
    memcpy(code.data(), (void*)&FreeLibAndExitThread, codeSize);
    decltype (&FreeLibAndExitThread)(code.data())(hModule, &dlclose, &pthread_exit);
}
