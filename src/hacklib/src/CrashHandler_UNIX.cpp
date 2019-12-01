#include "hacklib/CrashHandler.h"
#include <cstring>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>


thread_local sigjmp_buf t_currentEnv;
thread_local bool t_hasHandler = false;


static void SignalHandler(int sigNum)
{
    if (t_hasHandler)
    {
        // Jump out of the protected region to sigsetjmp.
        siglongjmp(t_currentEnv, sigNum);
    }
    else
    {
        // No handling requested. Defer to default handler.
        struct sigaction oldAction, currentAction = { 0 };
        currentAction.sa_handler = SIG_DFL;
        sigemptyset(&currentAction.sa_mask);
        currentAction.sa_flags = 0;

        sigaction(sigNum, &currentAction, &oldAction);
        kill(getpid(), sigNum);
        sigaction(sigNum, &oldAction, NULL);
    }
}


void hl::CrashHandler(const std::function<void()>& body, const std::function<void(uint32_t)>& handler)
{
    struct sigaction oldAction[5], currentAction = { 0 };
    sigjmp_buf oldEnv;

    currentAction.sa_handler = SignalHandler;
    sigemptyset(&currentAction.sa_mask);
    currentAction.sa_flags = 0;

    // Backup nested contexts.
    memcpy(oldEnv, t_currentEnv, sizeof(oldEnv));
    bool hadHandler = t_hasHandler;
    t_hasHandler = true;

    // Will return 0 first, then when siglongjmp is called, it returns the signal code.
    int sigNum = sigsetjmp(t_currentEnv, 1);
    if (!sigNum)
    {
        // Setup signal handlers.
        sigaction(SIGSEGV, &currentAction, &oldAction[0]);
        sigaction(SIGBUS, &currentAction, &oldAction[1]);
        sigaction(SIGFPE, &currentAction, &oldAction[2]);
        sigaction(SIGILL, &currentAction, &oldAction[3]);
        sigaction(SIGSYS, &currentAction, &oldAction[4]);
        body();
    }
    else
    {
        handler(sigNum);
    }

    // Restore signal handlers.
    sigaction(SIGSEGV, &oldAction[0], NULL);
    sigaction(SIGBUS, &oldAction[1], NULL);
    sigaction(SIGFPE, &oldAction[2], NULL);
    sigaction(SIGILL, &oldAction[3], NULL);
    sigaction(SIGSYS, &oldAction[4], NULL);
    memcpy(t_currentEnv, oldEnv, sizeof(oldEnv));
    t_hasHandler = hadHandler;
}
