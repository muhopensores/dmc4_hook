#pragma once

#include "cUnit.hpp"
#include "../MtMath.hpp"

class uShadow : public cUnit {
public:
    using uint32_t = unsigned int;
    private: char pad_18[0x28]; public:
    Vector3f rotation; // 0x40
    private: char pad_4c[0x4]; public:
    float howMuchToIgnoreModels; // 0x50
    private: char pad_54[0xc]; public:
    Vector4f colour; // 0x60
    private: char pad_70[0x30]; public:
    float diffuse; // 0xa0
    private: char pad_a4[0x10]; public:
    float solidity; // 0xb4
    private: char pad_b8[0x40]; public:
    float bloom; // 0xf8
    private: char pad_fc[0x128]; public:
    uint32_t resolution; // 0x224
}; // Size: 0x228
static_assert(sizeof(uShadow) == 0x228);