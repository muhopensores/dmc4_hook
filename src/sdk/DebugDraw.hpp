#pragma once

#include "MtMath.hpp"
#include <cmath>
#include <cstdint>

// Debug drawing with the game's own line primitive (sPrim::submitLine), so the lines are depth-tested against the
// world like anything the game draws, unlike an ImGui overlay. Use from a unit's render slot (uActor vtable +0x24,
// CustomUnit::render) with the ptrans it receives:
//
//     void MyUnit::render(void* ptrans) {
//         void* cprim = debug_draw::get_cprim(ptrans, 6, owner_unit);   // once per frame
//         debug_draw::draw_capsule(cprim, p0, p1, radius, 0xFF00FFFF);
//     }
//
// Colours are 0xAABBGGRR as RayTest uses them (0x00FF00 green, 0xFF0000 red). First used by RayTest's CollLine
// (a ray) and Gerb's shell (its capsule).
namespace debug_draw {

    // sPrim::getCPrim 0xA34EE0: the primitive buffer of this frame for `prim_type` (6 = lines). `unit` is the owner
    // the game records for it (RayTest passes the player; the drawing unit itself works too).
    static void* get_cprim(void* ptrans, int prim_type = 6, void* unit = nullptr) {
        uintptr_t fn_getCPrim = 0x00A34EE0;
        uintptr_t sPrim_ptr   = 0x00E559D4;   // sPrim::mpInstance
        void* res = nullptr;
        _asm {
            pushad
            mov esi, ptrans
            push unit
            push prim_type
            mov ecx, sPrim_ptr
            mov eax, [ecx]
            push eax
            call fn_getCPrim
            mov res, eax
            popad
        }
        return res;
    }

    // sPrim::submitLine 0x45D1F0 (cprim@ecx, start@edi, end, color, flags*). `flags` = { 0xFFFFFFFF, 16 } is what the
    // game's own debug lines pass; the two words aren't decoded.
    static void submit_line(void* cprim, const MtVector3* a, const MtVector3* b, unsigned int color, unsigned int* flags) {
        uintptr_t fn_submitLine = 0x0045D1F0;
        _asm {
            pushad
            mov ecx, cprim
            mov edi, a
            push flags
            push color
            push b
            call fn_submitLine
            popad
        }
    }

    static void draw_line(void* cprim, const glm::vec3& a, const glm::vec3& b, unsigned int color) {
        if (!cprim) return;
        unsigned int flags[2] = { 0xFFFFFFFF, 16 };
        MtVector3 va{ a.x, a.y, a.z, 0 }, vb{ b.x, b.y, b.z, 0 };
        submit_line(cprim, &va, &vb, color, flags);
    }

    // A local point of an MT matrix (rows: right, up, forward, position) in world space.
    static glm::vec3 world_point(const MtMatrix& m, const glm::vec3& local) {
        const glm::vec3 r(m.m1.x, m.m1.y, m.m1.z), u(m.m2.x, m.m2.y, m.m2.z), f(m.m3.x, m.m3.y, m.m3.z), p(m.m4.x, m.m4.y, m.m4.z);
        return p + r * local.x + u * local.y + f * local.z;
    }

    // Two unit vectors perpendicular to `axis` (and to each other).
    static void perpendicular(const glm::vec3& axis, glm::vec3& u, glm::vec3& v) {
        const glm::vec3 helper = std::fabs(axis.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        u = glm::normalize(glm::cross(axis, helper));
        v = glm::cross(axis, u);
    }

    // A circle of radius r around `center` in the plane spanned by u and v.
    static void draw_ring(void* cprim, const glm::vec3& center, const glm::vec3& u, const glm::vec3& v, float r, unsigned int color, int segments = 16) {
        for (int i = 0; i < segments; ++i) {
            const float a0 = i * 6.2831853f / segments, a1 = (i + 1) * 6.2831853f / segments;
            draw_line(cprim, center + (u * std::cos(a0) + v * std::sin(a0)) * r, center + (u * std::cos(a1) + v * std::sin(a1)) * r, color);
        }
    }

    // A wireframe capsule from p0 to p1 (a sphere when they coincide): the axis, four longitudinal edges, and three
    // rings at each end. The kCollPrim capsule of a collision is (world_point(mat, mPos0), world_point(mat, mPos1),
    // mRadius).
    static void draw_capsule(void* cprim, const glm::vec3& p0, const glm::vec3& p1, float r, unsigned int color, int segments = 16) {
        if (!cprim) return;
        glm::vec3 axis = p1 - p0;
        const float len = glm::length(axis);
        axis = len > 1e-3f ? axis / len : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 u, v;
        perpendicular(axis, u, v);
        if (len > 1e-3f) {
            draw_line(cprim, p0, p1, color);
            for (int k = 0; k < 4; ++k) {
                const float a = k * 1.5707963f;
                const glm::vec3 o = (u * std::cos(a) + v * std::sin(a)) * r;
                draw_line(cprim, p0 + o, p1 + o, color);
            }
        }
        for (const glm::vec3& c : { p0, p1 }) {
            draw_ring(cprim, c, u, v, r, color, segments);       // around the axis
            draw_ring(cprim, c, axis, u, r, color, segments);
            draw_ring(cprim, c, axis, v, r, color, segments);
            if (len <= 1e-3f) break;
        }
    }

    static void draw_sphere(void* cprim, const glm::vec3& center, float r, unsigned int color, int segments = 16) {
        draw_capsule(cprim, center, center, r, color, segments);
    }

} // namespace debug_draw
