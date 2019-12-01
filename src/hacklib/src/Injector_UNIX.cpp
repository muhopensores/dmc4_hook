#include "hacklib/ExeFile.h"
#include "hacklib/Injector.h"
#include "hacklib/Memory.h"
#include <algorithm>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <fstream>
#include <limits.h>
#include <sstream>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>


#ifdef ARCH_64BIT
#define USER_REG_IP(u) u.regs.rip
#define USER_REG_SP(u) u.regs.rsp
#define USER_REG_AX(u) u.regs.rax
#else
#define USER_REG_IP(u) u.regs.eip
#define USER_REG_SP(u) u.regs.esp
#define USER_REG_AX(u) u.regs.eax
#endif


class Injection
{
public:
    Injection(std::string* error) : m_error(error) {}
    ~Injection()
    {
        if (m_pid)
        {
            if (m_remoteLibName)
            {
                // Free the remote memory for the library name.
                call(m_free, m_remoteLibName);
            }

            if (m_restoreBackup)
            {
                // Restore registers.
                memcpy(&m_regs, &m_regsBackup, sizeof(m_regs));
                setRegs();
            }

            ptrace(PTRACE_DETACH, m_pid, NULL, NULL);
        }
    }

    void writeErr(const std::string& error)
    {
        if (m_error)
        {
            *m_error += error;
        }
    }

    bool findFile(const std::string& fileName)
    {
        if (!realpath(fileName.c_str(), m_fileName))
        {
            writeErr("Fatal: Could not get the full library name\n");
            return false;
        }

        if (access(m_fileName, F_OK) == -1)
        {
            writeErr("Fatal: Could not find the library file\n");
            return false;
        }

        return true;
    }

    bool openProcess(int pid)
    {
        if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: Could not attach with ptrace (errno = " + std::to_string(errno) + ")\n");
            return false;
        }

        m_pid = pid;

        if (!wait())
            return false;

        // Verify matching bitness of injector and target.
        char regsBuffer[256]; // x86_64 GPRs should be 27*sizeof(uint64_t)=216 bytes.
        struct iovec regSet;
        regSet.iov_base = &regsBuffer;
        regSet.iov_len = sizeof(regsBuffer);
        if (ptrace(PTRACE_GETREGSET, m_pid, NT_PRSTATUS, &regSet) < 0)
        {
            writeErr("Warning: Could not determine bitness of target process (errno = " + std::to_string(errno) + ")\n");
        }
        else
        {
            bool isTarget64 = regSet.iov_len != 17 * sizeof(uint32_t);
#ifdef ARCH_64BIT
            if (!isTarget64)
            {
                writeErr("Fatal: Can not inject into 32-bit process from 64-bit injector\n");
                return false;
            }
#else
            if (isTarget64)
            {
                writeErr("Fatal: Can not inject into 64-bit process from 32-bit injector\n");
                return false;
            }
#endif
        }

        if (!getRegs())
            return false;

        // Backup registers for restoring afterwards.
        memcpy(&m_regsBackup, &m_regs, sizeof(m_regs));

        // Don't touch 128 byte System V ABI redzone. Account for arguments that will be written after the SP.
        USER_REG_SP(m_regs) -= 256;

        m_restoreBackup = true;

        return true;
    }

    bool findLibs()
    {
        auto memoryMap = hl::GetMemoryMap(m_pid);

        auto findLib = [&](const std::string& libName) {
            return std::find_if(memoryMap.begin(), memoryMap.end(), [&](const hl::MemoryRegion& r) {
                auto pos = r.name.find(libName);
                if (pos != std::string::npos)
                {
                    pos += libName.size();
                    return r.name[pos] == '.' || r.name[pos] == '-';
                }
                return false;
            });
        };

        auto itCheck = std::find_if(memoryMap.begin(), memoryMap.end(),
                                    [this](const hl::MemoryRegion& r) { return r.name == m_fileName; });
        if (itCheck != memoryMap.end())
        {
            writeErr("Fatal: The specified module is already loaded\n");
            return false;
        }

        auto libc = findLib("libc");
        auto libdl = findLib("libdl");
        if (libc == memoryMap.end() || libdl == memoryMap.end())
        {
            writeErr("Fatal: Did not find mapping for libc and libdl libraries\n");
            return false;
        }

        m_libc = *libc;
        m_libdl = *libdl;

        return true;
    }

    bool findApis()
    {
        hl::ExeFile libc;
        if (!libc.loadFromFile(m_libc.name))
        {
            writeErr("Fatal: Could not load libc ELF file\n");
            return false;
        }
        m_malloc = libc.getExport("malloc");
        m_free = libc.getExport("free");

        if (!m_malloc || !m_free)
        {
            writeErr("Fatal: Did not find exports for malloc, free\n");
            return false;
        }
        m_malloc += m_libc.base;
        m_free += m_libc.base;

        hl::ExeFile libdl;
        if (!libdl.loadFromFile(m_libdl.name))
        {
            writeErr("Fatal: Could not load libdl ELF file\n");
            return false;
        }
        m_dlopen = libdl.getExport("dlopen");
        m_dlclose = libdl.getExport("dlclose");
        m_dlerror = libdl.getExport("dlerror");

        if (!m_dlopen || !m_dlclose || !m_dlerror)
        {
            writeErr("Fatal: Did not find exports for dlopen, dlclose or dlerror\n");
            return false;
        }
        m_dlopen += m_libdl.base;
        m_dlclose += m_libdl.base;
        m_dlerror += m_libdl.base;

        return true;
    }

    bool remoteLoadLib()
    {
        size_t libNameLen = strlen(m_fileName) + 1;
        size_t paddedLibNameLen = ((libNameLen - 1) & ~(sizeof(uintptr_t) - 1)) + sizeof(uintptr_t);

        // Resolve weird state after syscall.
        if (!call(0, 0, 0))
            return false;

        // Allocate memory in the target process.
        if (!call(m_malloc, paddedLibNameLen))
            return false;

        m_remoteLibName = USER_REG_AX(m_regs);
        if (!m_remoteLibName)
        {
            writeErr("Fatal: malloc failed in the remote process\n");
            return false;
        }

        // Copy the library name to the remote process. Assume that PATH_MAX is padded to pointer size boundary.
        for (uintptr_t i = 0; i < paddedLibNameLen; i += sizeof(uintptr_t))
        {
            uintptr_t value = *(uintptr_t*)&m_fileName[i];
            if (ptrace(PTRACE_POKEDATA, m_pid, m_remoteLibName + i, (void*)value) < 0)
            {
                writeErr("Fatal: ptrace POKEDATA failed\n");
                return false;
            }
        }

        // Load the library from the target process.
        if (!call(m_dlopen, m_remoteLibName, RTLD_NOW | RTLD_LOCAL))
            return false;

        // Check whether dlopen succeeded.
        if (!USER_REG_AX(m_regs))
        {
            writeErr("Fatal: Remote process could not load library\n");

            if (call(m_dlerror))
            {
                uintptr_t remoteStr = USER_REG_AX(m_regs);
                if (remoteStr)
                {
                    size_t remoteStrLen = 0;
                    uintptr_t value = 0;
                    std::string errorMsg;
                    auto hasNull = [](uintptr_t word) {
                        for (size_t i = 0; i < sizeof(uintptr_t); i++)
                        {
                            if ((word & ((uintptr_t)0xff << 8 * i)) == 0)
                                return true;
                        }
                        return false;
                    };
                    do
                    {
                        value = ptrace(PTRACE_PEEKDATA, m_pid, remoteStr + remoteStrLen, NULL);
                        if (errno != 0)
                        {
                            writeErr("Warning: ptrace PEEKDATA failed when getting dlerror\n");
                            break;
                        }
                        remoteStrLen += sizeof(uintptr_t);
                        errorMsg.append((char*)&value, sizeof(uintptr_t));
                    } while (!hasNull(value));

                    writeErr("    dlerror returned:\n    ");
                    writeErr(errorMsg);
                }
            }

            return false;
        }

        return true;
    }

private:
    bool getRegs()
    {
        if (ptrace(PTRACE_GETREGS, m_pid, NULL, &m_regs) < 0)
        {
            writeErr("Fatal: ptrace GETREGS failed\n");
            return false;
        }

        return true;
    }
    bool setRegs()
    {
        if (ptrace(PTRACE_SETREGS, m_pid, NULL, &m_regs) < 0)
        {
            writeErr("Fatal: ptrace SETREGS failed\n");
            return false;
        }

        return true;
    }
    bool call(uintptr_t function, uintptr_t arg1 = 0, uintptr_t arg2 = 0)
    {
        // The stack must be aligned on 16 byte boundary when a CALL is done.
        // Since no CALL is executed and a CALL pushes the return value, increment the SP.
        USER_REG_SP(m_regs) &= ~(uintptr_t)0xf;
        USER_REG_SP(m_regs) += sizeof(uintptr_t);

        // Write zero to top of stack, so that a return from a called function will trigger SIGSEGV.
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs), (void*)0) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }

        USER_REG_IP(m_regs) = function;
#ifdef ARCH_64BIT
        m_regs.regs.rdi = arg1;
        m_regs.regs.rsi = arg2;
#else
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs) + sizeof(uintptr_t), (void*)arg1) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }
        if (ptrace(PTRACE_POKEDATA, m_pid, USER_REG_SP(m_regs) + 2 * sizeof(uintptr_t), (void*)arg2) < 0)
        {
            writeErr("Fatal: ptrace POKEDATA failed\n");
            return false;
        }
#endif

        if (!setRegs())
            return false;
        if (!resume())
            return false;
        if (!wait())
            return false;
        if (!getRegs())
            return false;

        return true;
    }
    bool singlestep()
    {
        if (ptrace(PTRACE_SINGLESTEP, m_pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: ptrace SINGLESTEP failed\n");
            return false;
        }

        return true;
    }
    bool resume()
    {
        if (ptrace(PTRACE_CONT, m_pid, NULL, NULL) < 0)
        {
            writeErr("Fatal: ptrace continue failed\n");
            return false;
        }

        return true;
    }
    bool wait()
    {
        int status = 0;
        if (waitpid(m_pid, &status, 0) < 0)
        {
            writeErr("Fatal: waitpid failed\n");
            return false;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            writeErr("Fatal: Process is gone\n");
            return false;
        }

        return true;
    }

private:
    int m_pid = 0;
    char m_fileName[PATH_MAX];
    hl::MemoryRegion m_libc, m_libdl;
    uintptr_t m_malloc, m_free, m_dlopen, m_dlclose, m_dlerror;
    struct user m_regs, m_regsBackup;
    bool m_restoreBackup = false;
    uintptr_t m_remoteLibName = 0;

    std::string* m_error;
};


bool hl::Inject(int pid, const std::string& libFileName, std::string* error)
{
    Injection inj(error);

    return inj.findFile(libFileName) && inj.openProcess(pid) && inj.findLibs() && inj.findApis() && inj.remoteLoadLib();
}

std::vector<int> hl::GetPIDsByProcName(const std::string& pname)
{
    std::vector<int> result;

    DIR* dir = opendir("/proc");
    if (dir)
    {
        struct dirent* entryPID;
        while ((entryPID = readdir(dir)))
        {
            bool isPID = true;
            auto it = entryPID->d_name;
            while (*it)
            {
                if (!std::isdigit(*it))
                {
                    isPID = false;
                    break;
                }
                it++;
            }

            if (isPID)
            {
                std::string fileName = "/proc/";
                fileName += entryPID->d_name;
                fileName += "/cmdline";
                std::ifstream file(fileName);
                std::string processName;
                std::getline(file, processName);

                // This code assumes that the process command line in /proc/pid/cmdline was
                // not edited or set in weird ways. It is impossible to disambiguate something like:
                // /space path/executable with space -argWithSlash/ -argumentBeforeNull\0-otherArg

                // Discard everything but the first argument (being the executable path).
                auto firstArgEnd = processName.find_first_of("\0\n", 0, 2);
                if (firstArgEnd != std::string::npos)
                {
                    processName = processName.substr(0, firstArgEnd);
                }

                // Remove everything up to the last slash to get the name without path.
                // This assumes that there is no slash in the arguments, if the arguments
                // are in the first command line argument.
                auto slash = processName.find_last_of('/');
                if (slash != std::string::npos)
                {
                    processName = processName.substr(slash + 1);
                }

                if (processName == pname)
                {
                    result.push_back(std::atoi(entryPID->d_name));
                }
            }
        }
        closedir(dir);
    }

    return result;
}
