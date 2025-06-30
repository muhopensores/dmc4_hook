#pragma once
#ifndef MTMATH_HPP
#define MTMATH_HPP
#endif
#include <cstdint>
#include "Math.hpp"

typedef unsigned char undefined;
typedef signed char schar;
typedef unsigned char uchar;
typedef uint32_t uint;
typedef unsigned short ushort;
typedef unsigned short word;
typedef short sword;
typedef unsigned char uchar;
typedef unsigned int uint;

struct MtRect {
    uint32_t l;
    uint32_t t;
    uint32_t r;
    uint32_t b;
};

struct MtColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct MtFloat3 {
    float x;
    float y;
    float z;
};

struct MtSphere {
    struct MtFloat3 pos;
    float r;
};

struct MtVector3 { /* 10 */
    float x;
    float y;
    float z;
    uint32_t padding;
};
static_assert(sizeof(MtVector3) == 0x10);


struct MtVector4 { /* MtFramework vector */
    float x;
    float y;
    float z;
    float w;
};
static_assert(sizeof(MtVector4) == 0x10);

struct MtEaseCurve {
    float p1;
    float p2;
};

struct MtMatrix { /* MtFramework matrix */
    Vector4f m1;  // 0x0000
    Vector4f m2;  // 0x0010
    Vector4f m3;  // 0x0020
    Vector4f m4;  // 0x0030
};
static_assert(sizeof(MtMatrix) == 0x40);

//struct MtCapsule {
//    MtVector3 p0;
//    MtVector3 p1;
//    alignas(16) float r;
//};
//static_assert(sizeof(MtCapsule) == 0x30);

struct MtCapsule {
    struct MtVector3 p0;
    struct MtVector3 p1;
    float r;
    undefined field3_0x24;
    undefined field4_0x25;
    undefined field5_0x26;
    undefined field6_0x27;
    undefined field7_0x28;
    undefined field8_0x29;
    undefined field9_0x2a;
    undefined field10_0x2b;
    undefined field11_0x2c;
    undefined field12_0x2d;
    undefined field13_0x2e;
    undefined field14_0x2f;
};
static_assert(sizeof(MtCapsule) == 0x30);


struct MtLineSegment {
    MtVector3 p0;
    MtVector3 p1;
};

struct MtAABB {
    MtVector3 minpos;
    MtVector3 maxpos;
};

class MtHermiteCurve {
public:
    float x[8]; // 0x0000
    float y[8]; // 0x0020
};              // Size: 0x0040
static_assert(sizeof(MtHermiteCurve) == 0x40);
