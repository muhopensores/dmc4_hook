#include "hacklib/Hooker.h"
#include "hacklib/Main.h"
#include "hacklib/PageAllocator.h"
#include <Windows.h>
#include <map>
#include <mutex>


using namespace hl;


#ifdef ARCH_64BIT
#define REG_INSTRUCTIONPTR Rip
#else
#define REG_INSTRUCTIONPTR Eip
#endif


static LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exc);


struct Page
{
    Page(uintptr_t begin, uintptr_t end) : m_begin(begin), m_end(end), m_refs(1) {}
    uintptr_t m_begin;
    uintptr_t m_end;
    int m_refs;
};

class VEHHookManager
{
public:
    VEHHookManager() { m_pages[0] = nullptr; }
    hl::Hooker::HookCallback_t getHook(uintptr_t adr) const
    {
        auto it = m_hooks.find(adr);
        if (it != m_hooks.end())
            return it->second;

        return nullptr;
    }
    Page* getPage(uintptr_t adr)
    {
        std::lock_guard<std::mutex> lock(m_pagesMutex);

        auto it = m_pages.upper_bound(adr);
        if (it != m_pages.end())
            return (--it)->second.get();

        return nullptr;
    }
    void addHook(uintptr_t adr, hl::Hooker::HookCallback_t cbHook)
    {
        m_hooks[adr] = cbHook;

        // Set up a VEH if we have none yet.
        if (!m_pExHandler)
        {
            m_pExHandler = AddVectoredExceptionHandler(1, VectoredHandler);
            if (!m_pExHandler)
            {
                throw std::runtime_error("AddVectoredExceptionHandler failed");
            }
        }

        auto page = getPage(adr);
        if (page)
        {
            page->m_refs++;
        }
        else
        {
            auto region = hl::GetMemoryByAddress(adr);
            uintptr_t lowerBound = region.base;
            uintptr_t upperBound = lowerBound + hl::GetPageSize();

            std::lock_guard<std::mutex> lock(m_pagesMutex);
            m_pages[lowerBound] = std::make_unique<Page>(lowerBound, upperBound);
            m_pages[upperBound] = nullptr;
        }
    }
    void removeHook(uintptr_t adr)
    {
        m_hooks.erase(adr);

        auto page = getPage(adr);
        if (page)
        {
            if (page->m_refs == 1)
            {
                // Trigger the guard page violation to remove it. VEH will not handle the
                // exception so we can be sure the guard page protection was removed.
                [&] {
                    __try
                    {
                        while (true)
                        {
                            auto x = *(volatile int*)adr;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                    }
                }();

                std::lock_guard<std::mutex> lock(m_pagesMutex);
                m_pages.erase(page->m_end);
                m_pages.erase(page->m_begin);
            }
            else
            {
                page->m_refs--;
            }
        }

        // Remove the VEH if all hooks are gone.
        if (m_hooks.empty() && m_pExHandler)
        {
            RemoveVectoredExceptionHandler(m_pExHandler);
            m_pExHandler = nullptr;
        }
    }

private:
    PVOID m_pExHandler = nullptr;
    std::map<uintptr_t, hl::Hooker::HookCallback_t> m_hooks;
    std::map<uintptr_t, std::unique_ptr<Page>> m_pages;
    std::mutex m_pagesMutex;
};


static VEHHookManager g_vehHookManager;


class VEHHook : public IHook
{
public:
    VEHHook(uintptr_t location, hl::Hooker::HookCallback_t cbHook) : location(location)
    {
        g_vehHookManager.addHook(location, cbHook);
    }
    ~VEHHook() override { g_vehHookManager.removeHook(location); }

    uintptr_t getLocation() const override { return location; }

    uintptr_t location;
};


const IHook* Hooker::hookVEH(uintptr_t location, HookCallback_t cbHook)
{
    // Check for invalid parameters.
    if (!location || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<VEHHook>(location, cbHook);

    // Apply hook.
    DWORD dwOldProt;
    if (!VirtualProtect((LPVOID)location, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &dwOldProt))
    {
        return nullptr;
    }

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


static void checkForHookAndCall(uintptr_t adr, CONTEXT* pCtx)
{
    // Check for hooks and call them.
    auto cbHook = g_vehHookManager.getHook(adr);
    if (cbHook)
    {
        hl::CpuContext ctx;
#ifdef ARCH_64BIT
        ctx.RIP = pCtx->Rip;
        ctx.RFLAGS = (uintptr_t)pCtx->EFlags;
        ctx.R15 = pCtx->R15;
        ctx.R14 = pCtx->R14;
        ctx.R13 = pCtx->R13;
        ctx.R12 = pCtx->R12;
        ctx.R11 = pCtx->R11;
        ctx.R10 = pCtx->R10;
        ctx.R9 = pCtx->R9;
        ctx.R8 = pCtx->R8;
        ctx.RDI = pCtx->Rdi;
        ctx.RSI = pCtx->Rsi;
        ctx.RBP = pCtx->Rbp;
        ctx.RSP = pCtx->Rsp;
        ctx.RBX = pCtx->Rbx;
        ctx.RDX = pCtx->Rdx;
        ctx.RCX = pCtx->Rcx;
        ctx.RAX = pCtx->Rax;
#else
        ctx.EIP = pCtx->Eip;
        ctx.EFLAGS = pCtx->EFlags;
        ctx.EDI = pCtx->Edi;
        ctx.ESI = pCtx->Esi;
        ctx.EBP = pCtx->Ebp;
        ctx.ESP = pCtx->Esp;
        ctx.EBX = pCtx->Ebx;
        ctx.EDX = pCtx->Edx;
        ctx.ECX = pCtx->Ecx;
        ctx.EAX = pCtx->Eax;
#endif
        cbHook(&ctx);
#ifdef ARCH_64BIT
        pCtx->Rip = ctx.RIP;
        pCtx->EFlags = (uint32_t)ctx.RFLAGS;
        pCtx->R15 = ctx.R15;
        pCtx->R14 = ctx.R14;
        pCtx->R13 = ctx.R13;
        pCtx->R12 = ctx.R12;
        pCtx->R11 = ctx.R11;
        pCtx->R10 = ctx.R10;
        pCtx->R9 = ctx.R9;
        pCtx->R8 = ctx.R8;
        pCtx->Rdi = ctx.RDI;
        pCtx->Rsi = ctx.RSI;
        pCtx->Rbp = ctx.RBP;
        pCtx->Rsp = ctx.RSP;
        pCtx->Rbx = ctx.RBX;
        pCtx->Rdx = ctx.RDX;
        pCtx->Rcx = ctx.RCX;
        pCtx->Rax = ctx.RAX;
#else
        pCtx->Eip = ctx.EIP;
        pCtx->EFlags = ctx.EFLAGS;
        pCtx->Edi = ctx.EDI;
        pCtx->Esi = ctx.ESI;
        pCtx->Ebp = ctx.EBP;
        pCtx->Esp = ctx.ESP;
        pCtx->Ebx = ctx.EBX;
        pCtx->Edx = ctx.EDX;
        pCtx->Ecx = ctx.ECX;
        pCtx->Eax = ctx.EAX;
#endif
    }
}

static LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS exc)
{
    static uintptr_t guardFaultAdr;

    CONTEXT* pCtx = exc->ContextRecord;

    if (exc->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION)
    {
        // Guard page exeption occured. Guard page protection is gone now.
        // Parameters: [0]: access type that caused the fault, [1]: address that was accessed

        if (exc->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_READ_FAULT)
        {
            HMODULE hModule;
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCTSTR)exc->ContextRecord->REG_INSTRUCTIONPTR, &hModule);

            if (hModule == hl::GetCurrentModule())
            {
                // The read is from VEHHookManager::removeHook. Return without continuing
                // with a single-step. Do not handle the exception to signal success to removeHook.
                return EXCEPTION_CONTINUE_SEARCH;
            }
        }

        // Save the address that was accessed to reprotect it at the single-step exception.
        guardFaultAdr = exc->ExceptionRecord->ExceptionInformation[1];

        // Set single-step flag to advance one instruction.
        pCtx->EFlags |= 0x100;

        // Only hook for execute access. Not if someone tries to read or write the hooked memory.
        if (exc->ExceptionRecord->ExceptionInformation[0] == EXCEPTION_EXECUTE_FAULT)
        {
            checkForHookAndCall(guardFaultAdr, pCtx);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }
    else if (exc->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // Single-step exeption occured. Single-step flag is cleared now.

        // Check if we are within a page that contains hooks.
        if (g_vehHookManager.getPage(pCtx->REG_INSTRUCTIONPTR))
        {
            // We are still on a hooked page. Continue single-stepping.
            pCtx->EFlags |= 0x100;

            checkForHookAndCall(pCtx->REG_INSTRUCTIONPTR, pCtx);
        }
        else
        {
            // We stepped out of a hooked page.
            // Check if the page that was last guarded still contains a hook.
            if (g_vehHookManager.getPage(guardFaultAdr))
            {
                // Restore guard page protection.
                DWORD oldProt;
                VirtualProtect((LPVOID)guardFaultAdr, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &oldProt);
            }
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Do not handle any other exceptions.
    return EXCEPTION_CONTINUE_SEARCH;
}
