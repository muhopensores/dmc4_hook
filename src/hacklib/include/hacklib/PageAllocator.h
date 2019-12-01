#ifndef HACKLIB_EXECUTABLE_ALLOCATOR_H
#define HACKLIB_EXECUTABLE_ALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Memory.h"


namespace hl
{
// This allocator cannot directly be used in standard containers, because
// of the second template argument.
template <typename T, Protection P>
class page_allocator
{
public:
    typedef T value_type;

    page_allocator() = default;
    template <typename U>
    page_allocator(const page_allocator<U, P>& other)
    {
    }

    T* allocate(size_t n)
    {
        T* adr = (T*)PageAlloc(n * sizeof(T), P);
        if (!adr)
        {
            throw std::bad_alloc();
        }
        return adr;
    }
    void deallocate(T* p, size_t n) { PageFree(p, n * sizeof(T)); }
};


template <typename T>
class data_page_allocator : public page_allocator<T, hl::PROTECTION_READ_WRITE>
{
    using page_allocator<T, hl::PROTECTION_READ_WRITE>::page_allocator;
};

template <typename T>
using data_page_vector = std::vector<T, data_page_allocator<T>>;

template <typename T>
class code_page_allocator : public page_allocator<T, hl::PROTECTION_READ_WRITE_EXECUTE>
{
    using page_allocator<T, hl::PROTECTION_READ_WRITE_EXECUTE>::page_allocator;
};

typedef std::vector<unsigned char, code_page_allocator<unsigned char>> code_page_vector;
}

#endif
