#pragma once
// sim_view - a camera's view volume from the camera alone, for simulation code that asks "is it on screen".
//
// The game tests against sCamera's viewport frusta (isDispSphere 0x8EE300: viewport 0), built by its camera update from
// the viewport's region, so the answer depends on the screen: the split layout, a view moved to another window, the
// window's shape. Under netplay every machine has to get the same answer, so the simulation's frusta are built here from
// the camera unit's own fields with one fixed aspect ratio.
#include <cmath>
#include <cstdint>

namespace sim_view {

constexpr float kAspect = 16.0f / 9.0f;

// uCamera: +0x18 far, +0x1C near, +0x24 vertical field of view (degrees), +0x30 eye, +0x40 up, +0x50 target
// Six planes (x y z w), normals inward: a sphere is outside when -r > n . p + w for one of them (isDispSphere's test).
inline bool camera_frustum(uintptr_t camera, float out[6][4]) {
    if (!camera) return false;
    const float* c = (const float*)camera;
    const float far_plane = c[0x18 / 4], near_plane = c[0x1C / 4], fov = c[0x24 / 4];
    const float* eye = c + 0x30 / 4;
    const float* up = c + 0x40 / 4;
    const float* target = c + 0x50 / 4;
    float f[3] = {target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]};
    float fl = std::sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (!(fl > 1e-6f) || !(fov > 0.0f && fov < 180.0f)) return false;
    for (float& v : f) v /= fl;
    float r[3] = {f[1] * up[2] - f[2] * up[1], f[2] * up[0] - f[0] * up[2], f[0] * up[1] - f[1] * up[0]};
    const float rl = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    if (!(rl > 1e-6f)) return false;
    for (float& v : r) v /= rl;
    const float u[3] = {r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0]};
    const float tv = std::tan(fov * 0.5f * 3.14159265f / 180.0f);
    const float th = tv * kAspect;
    const auto plane = [&](float* p, float nx, float ny, float nz) {
        const float l = std::sqrt(nx * nx + ny * ny + nz * nz);
        p[0] = nx / l; p[1] = ny / l; p[2] = nz / l;
        p[3] = -(p[0] * eye[0] + p[1] * eye[1] + p[2] * eye[2]);
    };
    plane(out[0], f[0], f[1], f[2]);
    out[0][3] -= near_plane;
    plane(out[1], -f[0], -f[1], -f[2]);
    out[1][3] += far_plane;
    plane(out[2], f[0] * th + r[0], f[1] * th + r[1], f[2] * th + r[2]);   // left
    plane(out[3], f[0] * th - r[0], f[1] * th - r[1], f[2] * th - r[2]);   // right
    plane(out[4], f[0] * tv + u[0], f[1] * tv + u[1], f[2] * tv + u[2]);   // bottom
    plane(out[5], f[0] * tv - u[0], f[1] * tv - u[1], f[2] * tv - u[2]);   // top
    return true;
}

inline bool sphere_in(const float planes[6][4], const float* s) {
    for (int i = 0; i < 6; ++i)
        if (-s[3] > planes[i][0] * s[0] + planes[i][1] * s[1] + planes[i][2] * s[2] + planes[i][3]) return false;
    return true;
}

} // namespace sim_view
