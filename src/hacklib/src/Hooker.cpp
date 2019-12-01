#include "hacklib/Hooker.h"
#include "hacklib/PageAllocator.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_map>


#ifdef ARCH_64BIT
static const int JMPHOOKSIZE = 14;
#else
static const int JMPHOOKSIZE = 5;
#endif


using namespace hl;


struct FakeVT
{
    FakeVT(uintptr_t** instance, int vtBackupSize) : m_data(vtBackupSize), m_orgVT(*instance), m_refs(1)
    {
        // Copy original VT.
        for (int i = 0; i < vtBackupSize; i++)
        {
            m_data[i] = m_orgVT[i];
        }
    }
    hl::data_page_vector<uintptr_t> m_data;
    uintptr_t* m_orgVT;
    int m_refs;
};

class VTHookManager
{
public:
    uintptr_t getOrgFunc(uintptr_t** instance, int functionIndex)
    {
        return m_fakeVTs[instance]->m_orgVT[functionIndex];
    }
    void addHook(uintptr_t** instance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
    {
        auto& fakeVT = m_fakeVTs[instance];
        if (fakeVT)
        {
            // The VT of this object was already hooked. Make the fake VT writable again.
            hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ_WRITE);

            fakeVT->m_refs++;
        }
        else
        {
            // Create new fake VT (mirroring the original one).
            fakeVT = std::make_unique<FakeVT>(instance, vtBackupSize);

            // Replace the VT pointer in the object instance.
            *instance = fakeVT->m_data.data();
        }

        // Overwrite the hooked function in VT. This applies the hook.
        fakeVT->m_data[functionIndex] = cbHook;

        // Make the fake VT read-only like a real VT would be.
        hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ);
    }
    void removeHook(uintptr_t** instance, int functionIndex)
    {
        auto& fakeVT = m_fakeVTs[instance];
        if (fakeVT)
        {
            if (fakeVT->m_refs == 1)
            {
                // Last reference. Restore pointer to original VT.
                *instance = fakeVT->m_orgVT;

                m_fakeVTs.erase(instance);
            }
            else
            {
                // Keep using the fake VT, but restore the unhooked function pointer.
                hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ_WRITE);
                fakeVT->m_data[functionIndex] = fakeVT->m_orgVT[functionIndex];
                hl::PageProtectVec(fakeVT->m_data, PROTECTION_READ);

                fakeVT->m_refs--;
            }
        }
    }

private:
    std::unordered_map<uintptr_t**, std::unique_ptr<FakeVT>> m_fakeVTs;
};


static VTHookManager g_vtHookManager;


class VTHook : public IHook
{
public:
    VTHook(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
        : instance((uintptr_t**)classInstance), functionIndex(functionIndex)
    {
        g_vtHookManager.addHook(instance, functionIndex, cbHook, vtBackupSize);
    }
    ~VTHook() override { g_vtHookManager.removeHook(instance, functionIndex); }

    uintptr_t getLocation() const override { return g_vtHookManager.getOrgFunc(instance, functionIndex); }

    uintptr_t** instance;
    int functionIndex;
};

class JMPHook : public IHook
{
public:
    JMPHook(uintptr_t location, int offset) : location(location), offset(offset), wrapperCode(offset, 0xcc)
    {
        memcpy(wrapperCode.data(), (void*)location, offset);
    }
    ~JMPHook() override
    {
        hl::PageProtect((void*)location, offset, PROTECTION_READ_WRITE_EXECUTE);
        memcpy((void*)location, wrapperCode.data(), offset);
        hl::PageProtect((void*)location, offset, PROTECTION_READ_EXECUTE);
    }

    uintptr_t getLocation() const override { return location; }

    uintptr_t location;
    int offset;
    hl::code_page_vector wrapperCode;
};

class DetourHook : public IHook
{
public:
    DetourHook(uintptr_t location, int offset, Hooker::HookCallback_t cbHook)
        : location(location), offset(offset), cbHook(cbHook), wrapperCode(0x1000, 0xcc)
    {
    }
    ~DetourHook() override
    {
        hl::PageProtect((void*)location, offset, PROTECTION_READ_WRITE_EXECUTE);
        memcpy((void*)location, originalCode, offset);
        hl::PageProtect((void*)location, offset, PROTECTION_READ_EXECUTE);

        // In case the hook is currently executing, wait for it to end before releasing the wrapper code.
        std::lock_guard<std::mutex> lock(mutex);

        // BUG: There is a slight chance that the execution flow will enter the hook again and
        // will crash when trying to return because the wrapper code is gone.
    }

    uintptr_t getLocation() const override { return location; }

    uintptr_t location;
    int offset;
    uintptr_t ipBackup = 0;
    unsigned char* originalCode = nullptr;
    hl::code_page_vector wrapperCode;
    Hooker::HookCallback_t cbHook;
    std::mutex mutex;
};


static void JMPHookLocker(DetourHook* pHook, CpuContext* ctx)
{
    std::lock_guard<std::mutex> lock(pHook->mutex);

    pHook->cbHook(ctx);
}


const IHook* Hooker::hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize)
{
    // Check for invalid parameters.
    if (!classInstance || functionIndex < 0 || functionIndex >= vtBackupSize || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<VTHook>(classInstance, functionIndex, cbHook, vtBackupSize);

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


#ifndef ARCH_64BIT
static std::vector<unsigned char> GenJumpOverwrite_x86(uintptr_t target, uintptr_t location, int nextInstructionOffset)
{
    // Calculate delta.
    uintptr_t jmpFromLocToTarget = (uintptr_t)target - location - 5;

    // Generate patch. Fill with NOPs.
    std::vector<unsigned char> jmpPatch(nextInstructionOffset, 0x90);
    jmpPatch[0] = 0xe9; // JMP target
    *(uintptr_t*)&jmpPatch[1] = jmpFromLocToTarget;

    return jmpPatch;
}

static void GenWrapper_x86(DetourHook* pHook)
{
    unsigned char* buffer = pHook->wrapperCode.data();

    // Calculate delta.
    uintptr_t callFromWrapperToLocker = (uintptr_t)JMPHookLocker - (uintptr_t)buffer - 13 - 5;

    // Push context to the stack. General purpose, flags and instruction pointer.
    buffer[0] = 0x60; // PUSHAD
    buffer[1] = 0x9c; // PUSHFD
    buffer[2] = 0x68; // PUSH jumpBack
    *(uintptr_t*)&buffer[3] = pHook->location + pHook->offset;
    // Push stack pointer to stack as second argument to the locker function. Pointing to the context.
    buffer[7] = 0x54; // PUSH ESP
    // Push pHook instance pointer as first argument to the locker function.
    buffer[8] = 0x68; // PUSH pHook
    *(uintptr_t*)&buffer[9] = (uintptr_t)pHook;
    // Call the hook callback.
    buffer[13] = 0xe8; // CALL cbHook
    *(uintptr_t*)&buffer[14] = callFromWrapperToLocker;
    // Cleanup parameters from cdecl call.
    buffer[18] = 0x58; // POP EAX
    buffer[19] = 0x58; // POP EAX
    // Backup the instruction pointer that may have been modified by the callback.
    buffer[20] = 0x8f; // POP [ipBackup]
    buffer[21] = 0x05;
    *(uintptr_t**)&buffer[22] = &pHook->ipBackup;
    // Restore general purpose and flags registers.
    buffer[26] = 0x9d; // POPFD
    buffer[27] = 0x61; // POPAD

    buffer += 28;
    pHook->originalCode = buffer;

    // Copy originally overwritten code.
    memcpy(buffer, (void*)pHook->location, pHook->offset);

    buffer += pHook->offset;

    // Jump to the backed up instruction pointer.
    buffer[0] = 0xff; // JMP [ipBackup]
    buffer[1] = 0x25;
    *(uintptr_t**)&buffer[2] = &pHook->ipBackup;
}

#else

static std::vector<unsigned char> GenJumpOverwrite_x86_64(uintptr_t target, int nextInstructionOffset)
{
    uint32_t target_lo = (uint32_t)target;
    uint32_t target_hi = (uint32_t)(target >> 32);

    // Generate patch. Fill with NOPs.
    std::vector<unsigned char> jmpPatch(nextInstructionOffset, 0x90);
    jmpPatch[0] = 0x68; // PUSH cbHook@lo
    *(uint32_t*)&jmpPatch[1] = target_lo;
    jmpPatch[5] = 0xc7; // MOV [RSP+4], cbHook@hi
    jmpPatch[6] = 0x44;
    jmpPatch[7] = 0x24;
    jmpPatch[8] = 0x04;
    *(uint32_t*)&jmpPatch[9] = target_hi;
    jmpPatch[13] = 0xc3; // RETN

    return jmpPatch;
}

static void GenWrapper_x86_64(DetourHook* pHook)
{
    uintptr_t returnAdr = pHook->location + pHook->offset;
    uint32_t return_lo = (uint32_t)returnAdr;
    uint32_t return_hi = (uint32_t)(returnAdr >> 32);

    unsigned char* buffer = pHook->wrapperCode.data();

    // TODO: Respect red zone for System V ABI! 128 bytes above rsp may not be clobbered!
    // I think this must be done in the overwrite code
    // or use this weird trick for jumping: jmp [rip+0]; dq adr

    // Push context to the stack. General purpose, flags and instruction pointer.
    buffer[0] = 0x54; // PUSH RSP
    buffer[1] = 0x50; // PUSH RAX
    buffer[2] = 0x51; // PUSH RCX
    buffer[3] = 0x52; // PUSH RDX
    buffer[4] = 0x53; // PUSH RBX
    buffer[5] = 0x55; // PUSH RBP
    buffer[6] = 0x56; // PUSH RSI
    buffer[7] = 0x57; // PUSH RDI
    buffer[8] = 0x41; // PUSH R8
    buffer[9] = 0x50;
    buffer[10] = 0x41; // PUSH R9
    buffer[11] = 0x51;
    buffer[12] = 0x41; // PUSH R10
    buffer[13] = 0x52;
    buffer[14] = 0x41; // PUSH R11
    buffer[15] = 0x53;
    buffer[16] = 0x41; // PUSH R12
    buffer[17] = 0x54;
    buffer[18] = 0x41; // PUSH R13
    buffer[19] = 0x55;
    buffer[20] = 0x41; // PUSH R14
    buffer[21] = 0x56;
    buffer[22] = 0x41; // PUSH R15
    buffer[23] = 0x57;
    buffer[24] = 0x9c; // PUSHFQ
    buffer[25] = 0x68; // PUSH returnAdr@lo
    *(uint32_t*)&buffer[26] = return_lo;
    buffer[30] = 0xc7; // MOV [RSP+4], returnAdr@hi
    buffer[31] = 0x44;
    buffer[32] = 0x24;
    buffer[33] = 0x04;
    *(uint32_t*)&buffer[34] = return_hi;

    buffer += 38;

    // Backup RSP to RBX and align it on 16 byte boundary.
    buffer[0] = 0x48; // MOV RBX, RSP
    buffer[1] = 0x89;
    buffer[2] = 0xe3;
    buffer[3] = 0x48; // AND RSP, 0xffffffff_fffffff0
    buffer[4] = 0x83;
    buffer[5] = 0xe4;
    buffer[6] = 0xf0;

    buffer += 7;

    // Call the locker function.
#if defined(_WIN64)                                         // Microsoft x64 calling convention
    // Second parameter: CpuContext*
    buffer[0] = 0x48; // MOV RDX, RBX
    buffer[1] = 0x89;
    buffer[2] = 0xda;
    // First parameter: DetourHook*
    buffer[3] = 0x48; // MOV RCX, pHook
    buffer[4] = 0xb9;
    *(uintptr_t*)&buffer[5] = (uintptr_t)pHook;
    // Shadow space for callee.
    buffer[13] = 0x48; // SUB RSP, 0x20
    buffer[14] = 0x83;
    buffer[15] = 0xec;
    buffer[16] = 0x20;
    buffer[17] = 0x48; // MOV RAX, JMPHookLocker
    buffer[18] = 0xb8;
    *(uintptr_t*)&buffer[19] = (uintptr_t)JMPHookLocker;
    buffer[27] = 0xff; // CALL RAX
    buffer[28] = 0xd0;

    buffer += 29;
#elif defined(unix) || defined(__unix__) || defined(__unix) // System V AMD64 ABI
    // Second parameter: CpuContext*
    buffer[0] = 0x48; // MOV RSI, RBP
    buffer[1] = 0x89;
    buffer[2] = 0xde;
    // First parameter: DetourHook*
    buffer[3] = 0x48; // MOV RDI, pHook
    buffer[4] = 0xbf;
    *(uintptr_t*)&buffer[5] = (uintptr_t)pHook;
    buffer[13] = 0x48; // MOV RAX, JMPHookLocker
    buffer[14] = 0xb8;
    *(uintptr_t*)&buffer[15] = (uintptr_t)JMPHookLocker;
    buffer[23] = 0xff; // CALL RAX
    buffer[24] = 0xd0;

    buffer += 25;
#endif

    // Restore RSP.
    buffer[0] = 0x48; // MOV RSP, RBX
    buffer[1] = 0x89;
    buffer[2] = 0xdc;

    // Backup the instruction pointer that may have been modified by the callback.
    buffer[3] = 0x58; // POP EAX
    buffer[4] = 0x48; // MOV [ipBackup], RAX
    buffer[5] = 0xa3;
    *(uintptr_t**)&buffer[6] = &pHook->ipBackup;

    buffer += 14;

    // Restore general purpose and flags registers.
    buffer[0] = 0x9d; // POPFQ
    buffer[1] = 0x41; // POP R15
    buffer[2] = 0x5f;
    buffer[3] = 0x41; // POP R14
    buffer[4] = 0x5e;
    buffer[5] = 0x41; // POP R13
    buffer[6] = 0x5d;
    buffer[7] = 0x41; // POP R12
    buffer[8] = 0x5c;
    buffer[9] = 0x41; // POP R11
    buffer[10] = 0x5b;
    buffer[11] = 0x41; // POP R10
    buffer[12] = 0x5a;
    buffer[13] = 0x41; // POP R9
    buffer[14] = 0x59;
    buffer[15] = 0x41; // POP R8
    buffer[16] = 0x58;
    buffer[17] = 0x5f; // POP RDI
    buffer[18] = 0x5e; // POP RSI
    buffer[19] = 0x5d; // POP RBP
    buffer[20] = 0x5b; // POP RBX
    buffer[21] = 0x5a; // POP RDX
    buffer[22] = 0x59; // POP RCX
    buffer[23] = 0x58; // POP RAX
    buffer[24] = 0x5c; // POP RSP

    buffer += 25;

    pHook->originalCode = buffer;
    // Copy originally overwritten code.
    memcpy(buffer, (void*)pHook->location, pHook->offset);

    buffer += pHook->offset;

    // Jump to the backed up instruction pointer.
    buffer[0] = 0x50; // PUSH RAX
    buffer[1] = 0x48; // MOV RAX, [ipBackup]
    buffer[2] = 0xa1;
    *(uintptr_t**)&buffer[3] = &pHook->ipBackup;
    buffer[11] = 0x48; // XCHG [RSP], RAX
    buffer[12] = 0x87;
    buffer[13] = 0x04;
    buffer[14] = 0x24;
    buffer[15] = 0xc3; // RETN
}

#endif


const IHook* Hooker::hookJMP(uintptr_t location, int nextInstructionOffset, uintptr_t cbHook, uintptr_t* jmpBack)
{
    // Check for invalid parameters.
    if (!location || nextInstructionOffset < JMPHOOKSIZE || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<JMPHook>(location, nextInstructionOffset);

    // The jump back must only be written if used.
    if (jmpBack)
    {
#ifdef ARCH_64BIT
        auto jmpBackPatch = GenJumpOverwrite_x86_64(location + nextInstructionOffset, JMPHOOKSIZE);
#else
        auto jmpBackPatch =
            GenJumpOverwrite_x86(location + nextInstructionOffset,
                                 (uintptr_t)pHook->wrapperCode.data() + nextInstructionOffset, JMPHOOKSIZE);
#endif
        // It is safe to write out of bounds here, because we allocated a whole page.
        memcpy(pHook->wrapperCode.data() + nextInstructionOffset, jmpBackPatch.data(), JMPHOOKSIZE);
        *jmpBack = (uintptr_t)pHook->wrapperCode.data();
    }

#ifdef ARCH_64BIT
    auto jmpPatch = GenJumpOverwrite_x86_64(cbHook, nextInstructionOffset);
#else
    auto jmpPatch = GenJumpOverwrite_x86(cbHook, location, nextInstructionOffset);
#endif

    // Apply the hook by writing the jump.
    hl::PageProtect((void*)location, nextInstructionOffset, PROTECTION_READ_WRITE_EXECUTE);
    memcpy((void*)location, jmpPatch.data(), nextInstructionOffset);
    hl::PageProtect((void*)location, nextInstructionOffset, PROTECTION_READ_EXECUTE);

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


const IHook* Hooker::hookDetour(uintptr_t location, int nextInstructionOffset, HookCallback_t cbHook)
{
    // Check for invalid parameters.
    if (!location || nextInstructionOffset < JMPHOOKSIZE || !cbHook)
        return nullptr;

    auto pHook = std::make_unique<DetourHook>(location, nextInstructionOffset, cbHook);

#ifdef ARCH_64BIT
    GenWrapper_x86_64(pHook.get());
    auto jmpPatch = GenJumpOverwrite_x86_64((uintptr_t)pHook->wrapperCode.data(), nextInstructionOffset);
#else
    GenWrapper_x86(pHook.get());
    auto jmpPatch = GenJumpOverwrite_x86((uintptr_t)pHook->wrapperCode.data(), location, nextInstructionOffset);
#endif

    // Apply the hook by writing the jump.
    hl::PageProtect((void*)location, nextInstructionOffset, PROTECTION_READ_WRITE_EXECUTE);
    memcpy((void*)location, jmpPatch.data(), nextInstructionOffset);
    hl::PageProtect((void*)location, nextInstructionOffset, PROTECTION_READ_EXECUTE);

    auto result = pHook.get();
    m_hooks.push_back(std::move(pHook));
    return result;
}


void Hooker::unhook(const IHook* pHook)
{
    auto cond = [pHook](const auto& uptr) { return uptr.get() == pHook; };

    m_hooks.erase(std::remove_if(m_hooks.begin(), m_hooks.end(), cond), m_hooks.end());
}
