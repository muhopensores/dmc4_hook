#include "Thread.hpp"
#include <TlHelp32.h>
#include <mutex>

static std::mutex g_suspend_mutex{};

namespace utility {
std::queue<DWORD> suspend_all_other_threads(void) {
  std::queue <DWORD> threads;
  g_suspend_mutex.lock();

  AutoHandle h_snap (
    CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
  );

  if (h_snap != INVALID_HANDLE_VALUE)
  {
    THREADENTRY32 tent        = {                    };
                  tent.dwSize = sizeof (THREADENTRY32);

    if (Thread32First (h_snap, &tent))
    {
      do
      {
        if ( tent.dwSize >= FIELD_OFFSET (THREADENTRY32, th32OwnerProcessID) +
                                  sizeof (tent.th32OwnerProcessID) )
        {
          if ( tent.th32ThreadID       != GetCurrentThreadId  () &&
               tent.th32OwnerProcessID == GetCurrentProcessId () )
          {
            AutoHandle h_thread (
              OpenThread (THREAD_SUSPEND_RESUME, FALSE, tent.th32ThreadID)
            );

            if (h_thread != nullptr)
            {
              threads.push  (tent.th32ThreadID);

              SuspendThread (h_thread);
            }
          }
        }

        tent.dwSize = sizeof (tent);
      } while (Thread32Next (h_snap, &tent));
    }
  }

  return threads;
}

void resume_threads (std::queue<DWORD>& threads) {
  while (! threads.empty ())
  {
    DWORD tid = threads.front ();

    AutoHandle h_thread (
      OpenThread (THREAD_SUSPEND_RESUME, FALSE, tid)
    );

    if (h_thread != nullptr)
    {
      ResumeThread (h_thread);
    }

    threads.pop ();
  }
  g_suspend_mutex.unlock();
}
} // namespace utility