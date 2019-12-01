#ifndef HACKLIB_HOOKER_H
#define HACKLIB_HOOKER_H

#include <cstdint>
#include <memory>
#include <vector>


namespace hl
{
class Hooker;


/// Base interface class for hook instances.
class IHook
{
public:
    virtual ~IHook() {}
    /// Returns the memory address that was hooked by this hook.
    virtual uintptr_t getLocation() const = 0;
};


/// Core CPU context for x86.
struct CpuContext_x86
{
    uintptr_t EIP;
    uintptr_t EFLAGS;
    uintptr_t EDI;
    uintptr_t ESI;
    uintptr_t EBP;
    uintptr_t ESP;
    uintptr_t EBX;
    uintptr_t EDX;
    uintptr_t ECX;
    uintptr_t EAX;
};

/// Core CPU context for x86_64
struct CpuContext_x86_64
{
    uintptr_t RIP;
    uintptr_t RFLAGS;
    uintptr_t R15;
    uintptr_t R14;
    uintptr_t R13;
    uintptr_t R12;
    uintptr_t R11;
    uintptr_t R10;
    uintptr_t R9;
    uintptr_t R8;
    uintptr_t RDI;
    uintptr_t RSI;
    uintptr_t RBP;
    uintptr_t RBX;
    uintptr_t RDX;
    uintptr_t RCX;
    uintptr_t RAX;
    uintptr_t RSP;
};

#ifdef ARCH_64BIT
/// A typedef referring to the CPU context of the compilation architecture.
typedef CpuContext_x86_64 CpuContext;
#else
/// A typedef referring to the CPU context of the compilation architecture.
typedef CpuContext_x86 CpuContext;
#endif


/// Helper for creating and removing various types of hooks.
class Hooker
{
public:
    typedef void (*HookCallback_t)(CpuContext*);

    /// Hook by replacing an object instances virtual table pointer.
    /// This method can only target virtual functions. It should always
    /// be preferred if possible as it is almost impossible to detect.
    /// No read-only target memory is modified.
    /// \param classInstance: The class instance to modify.
    /// \param functionIndex: Zero based ordinal number of the targeted virtual function.
    /// \param cbHook: The hook target location.
    /// \param vtBackupSize: Amount of memory to use for backing up the original virtual table.
    const IHook* hookVT(uintptr_t classInstance, int functionIndex, uintptr_t cbHook, int vtBackupSize = 1024);

    /// Hook by patching the target location with a jump instruction.
    /// Simple but has maximum flexibility. Your code has the responsibility
    /// to resume execution somehow. A simple return will likely crash the target.
    /// \param location: The location to hook.
    /// \param nextInstructionOffset: The offset from location to the next instruction after
    ///     the jump that will be written. Currently 5 bytes on 32-bit and 14 bytes on 64-bit are required for the jump.
    /// \param cbHook: The hook target location.
    /// \param jmpBack: Optional output parameter to receive the address of wrapper code that
    ///     executes the overwritten code and jumps back. Do a jump to this address
    ///     at the end of your hook to resume execution.
    const IHook* hookJMP(uintptr_t location, int nextInstructionOffset, uintptr_t cbHook, uintptr_t* jmpBack = nullptr);

    /// Hook by patching the location with a jump like hookJMP, but jumps to
    /// wrapper code that preserves registers, calls the given hook callback and
    /// executes the overwritten instructions for maximum convenience.
    const IHook* hookDetour(uintptr_t location, int nextInstructionOffset, HookCallback_t cbHook);

    /// Hook by using memory protection and a global exception handler.
    /// This method is very slow.
    /// No memory in the target is modified at all.
    const IHook* hookVEH(uintptr_t location, HookCallback_t cbHook);

    /// Removes the hook represented by the given hl::IHook object and releases all associated resources.
    void unhook(const IHook* pHook);


    /// \overload
    template <typename T, typename C>
    const IHook* hookVT(T* classInstance, int functionIndex, C cbHook, int vtBackupSize = 1024)
    {
        return hookVT((uintptr_t)classInstance, functionIndex, (uintptr_t)cbHook, vtBackupSize);
    }

    /// \overload
    template <typename F, typename C>
    const IHook* hookJMP(F location, int nextInstructionOffset, C cbHook, uintptr_t* jmpBack = nullptr)
    {
        return hookJMP((uintptr_t)location, nextInstructionOffset, (uintptr_t)cbHook, jmpBack);
    }

    /// \overload
    template <typename F>
    const IHook* hookDetour(F location, int nextInstructionOffset, HookCallback_t cbHook)
    {
        return hookDetour((uintptr_t)location, nextInstructionOffset, cbHook);
    }


private:
    std::vector<std::unique_ptr<IHook>> m_hooks;
};
}

#endif
