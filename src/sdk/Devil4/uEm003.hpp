#pragma once

#include <cstdio>
#include "MtObject.hpp"

#pragma pack(push, r2, 1)

class uEm003Shl : public MtObject {
public:
    char pad_0004[44];                                // 0x0004
    Vector3f m_pos_maybe;                             // 0x0030
    char pad_003_c[3716];                             // 0x003C
    Vector3f m_some_float_vec;                        // 0x0EC0
    char pad_0_ecc[2260];                             // 0x0ECC
    uint32_t m_required_idk_wtf_is_this_needs_to_be2; // 0x17A0
    uint32_t m_required_idk_wtf_is_this_needs_to_be6; // 0x17A4
    char pad_17_a8[12];                               // 0x17A8
    uint16_t n00005_a23;                              // 0x17B4
    uint8_t m_required_some_byte_idk;                 // 0x17B6
    char pad_17_b7[57];                               // 0x17B7
}; // Size: 0x17F0
static_assert(sizeof(uEm003Shl) == 0x17F0);

#pragma pack(pop, r2)
