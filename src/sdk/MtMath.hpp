#pragma once
#ifndef MTMATH_HPP
#define MTMATH_HPP
#endif
#include <cstdint>
#include <cmath>
#include "sdk/Math.hpp"

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

    // TODO(remove): added for compat with existing code
    MtVector3& operator=(const glm::vec3& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }

    MtVector3& operator+=(const glm::vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    // NOTE(): added an implicit conversion cause we had two uCoords
    // one is using glm::vec3s and other uses MtVector3 :shrug:
    operator glm::vec3() const { return glm::vec3(x, y, z); }

    // NOTE(): added array subscript operator since some siyan code
    // uses both xyz and [012] and i'm too scared to touch his code
    float& operator[](size_t i) {
        assert(i < 3);
        return (&x)[i];
    }
    const float& operator[](size_t i) const {
        assert(i < 3);
        return (&x)[i];
    }
};
static_assert(sizeof(MtVector3) == 0x10);


struct MtVector4 {
    float x;
    float y;
    float z;
    float w;

    MtVector4 operator*(const MtVector4& rhs) const {
        MtVector4 r;
        r.w = w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z;
        r.x = w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y;
        r.y = w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x;
        r.z = w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w;
        return r;
    }
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

    operator glm::mat4() const {
        return glm::mat4(glm::vec4(m1.x, m1.y, m1.z, m1.w), glm::vec4(m2.x, m2.y, m2.z, m2.w), glm::vec4(m3.x, m3.y, m3.z, m3.w),
            glm::vec4(m4.x, m4.y, m4.z, m4.w));
    }
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

struct MtPoint {
    int x;
    int y;
};

struct MtSize {
    uint w;
    uint h;
};

// Conventions (RE notes, uEffectVFR::setQuatParentOfs): an MtQuaternion / MtVector4 quaternion is x y z w and builds
// the same rotation as Blender's (w, x, y, z); an MtMatrix's rows are the basis vectors right / up / forward and the
// position; the game's yaw is atan2(x, z) (forward (0, 0, 1) -> (sin yaw, 0, cos yaw)).

// v turned `deg` degrees about the unit `axis` (Rodrigues).
inline glm::vec3 mt_rodrigues(const glm::vec3& v, const glm::vec3& axis, float deg) {
    const float a = deg * 3.14159265f / 180.0f, c = std::cos(a), s = std::sin(a);
    return v * c + glm::cross(axis, v) * s + axis * (glm::dot(axis, v) * (1.0f - c));
}

// A yaw about Y as an MT quaternion.
inline void mt_yaw_quat(float yaw, MtVector4& q) {
    q.x = 0.0f; q.y = std::sin(yaw * 0.5f); q.z = 0.0f; q.w = std::cos(yaw * 0.5f);
}

// An orthonormal basis (the rows of an MtMatrix: right, up, forward) as an MT quaternion (x, y, z, w). The rows are
// taken as the columns of a column-vector rotation matrix, which is what makes the result equal the game's own mQuat
// for its mWmat (checked against uPlayer::mQuat in Gerb's readout).
inline void mt_basis_quat(const glm::vec3& right, const glm::vec3& up, const glm::vec3& fwd, float out[4]) {
    const float m00 = right.x, m10 = right.y, m20 = right.z;
    const float m01 = up.x,    m11 = up.y,    m21 = up.z;
    const float m02 = fwd.x,   m12 = fwd.y,   m22 = fwd.z;
    const float tr = m00 + m11 + m22;
    float x, y, z, w, S;
    if (tr > 0.0f)                       { S = std::sqrt(tr + 1.0f) * 2.0f;             w = 0.25f * S;       x = (m21 - m12) / S; y = (m02 - m20) / S; z = (m10 - m01) / S; }
    else if (m00 > m11 && m00 > m22)     { S = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f; w = (m21 - m12) / S; x = 0.25f * S;       y = (m01 + m10) / S; z = (m02 + m20) / S; }
    else if (m11 > m22)                  { S = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f; w = (m02 - m20) / S; x = (m01 + m10) / S; y = 0.25f * S;       z = (m12 + m21) / S; }
    else                                 { S = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f; w = (m10 - m01) / S; x = (m02 + m20) / S; y = (m12 + m21) / S; z = 0.25f * S; }
    out[0] = x; out[1] = y; out[2] = z; out[3] = w;
}
