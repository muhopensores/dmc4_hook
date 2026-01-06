#pragma once

#include <cstdint>
#include "MtObject.hpp"

class MtCriticalSection;

class MtAllocator : public MtObject {
public:
    uint32_t m_used_size;          // 0x0004
    uint32_t m_max_used_size;      // 0x0008
    uint32_t m_total_size;         // 0x000C
    char* m_name;                  // 0x0010
    uint16_t m_type;               // 0x0014
    uint16_t m_attr;               // 0x0016
    uint32_t m_owner;              // 0x0018
    class MtCriticalSection* m_cs; // 0x001C
    virtual MtObject* allocate(size_t size, int align_mask);
    virtual uint16_t deallocate(MtObject* object); // 0x0018

}; // Size: 0x0020
static_assert(sizeof(MtAllocator) == 0x0020);

class MtHeapAllocator : public MtAllocator {
public:
    char pad_0020[60]; // 0x0020
}; // Size: 0x005C
static_assert(sizeof(MtHeapAllocator) == 0x005C);

