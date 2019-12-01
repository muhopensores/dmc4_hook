#ifndef HACKLIB_FOREIGNCLASS_H
#define HACKLIB_FOREIGNCLASS_H

#include <cstdint>


namespace hl
{
/* C++ style casting is avoided for readability. */


// A foreign class whose layout is defined at runtime
class ForeignClass
{
public:
    ForeignClass(void* ptr) : m_ptr(ptr) {}

    template <typename T>
    T get(uintptr_t offset) const
    {
        return *(T*)((uintptr_t)m_ptr + offset);
    }

    template <typename T>
    void set(uintptr_t offset, T const& value)
    {
        *(T*)((uintptr_t)m_ptr + offset) = value;
    }

    /**
     * Calls a virtual function of the foreign class instance.
     *
     * \param offset The offset from the virtual table base to the function to call
     * \param args The arguments to call the function with
     * \return The return value of the function that was called
     */
    template <typename T, typename... Ts>
    T call(uintptr_t offset, Ts... args)
    {
        return ((T(__thiscall*)(void*, Ts...))(*(uintptr_t*)((*(uintptr_t*)m_ptr) + offset)))(m_ptr, args...);
    }

    explicit operator bool() const { return m_ptr != nullptr; }

    void* data() { return m_ptr; }

    const void* data() const { return m_ptr; }

private:
    void* m_ptr;
};


inline bool operator==(const ForeignClass& lhs, const ForeignClass& rhs)
{
    return lhs.data() == rhs.data();
}

inline bool operator==(const ForeignClass& lhs, nullptr_t)
{
    return lhs.data() == nullptr;
}

inline bool operator==(nullptr_t, const ForeignClass& rhs)
{
    return nullptr == rhs.data();
}

inline bool operator!=(const ForeignClass& lhs, const ForeignClass& rhs)
{
    return lhs.data() != rhs.data();
}

inline bool operator!=(const ForeignClass& lhs, nullptr_t)
{
    return lhs.data() != nullptr;
}

inline bool operator!=(nullptr_t, const ForeignClass& rhs)
{
    return nullptr != rhs.data();
}


inline ForeignClass operator+(const ForeignClass& base, uintptr_t offset)
{
    return (void*)((uintptr_t)base.data() + offset);
}

inline ForeignClass operator+(uintptr_t offset, const ForeignClass& base)
{
    return base + offset;
}

inline ForeignClass operator-(const ForeignClass& base, uintptr_t offset)
{
    return (void*)((uintptr_t)base.data() - offset);
}

inline ForeignClass operator-(uintptr_t offset, const ForeignClass& base)
{
    return base - offset;
}
}

#endif