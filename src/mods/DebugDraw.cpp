#if 1
// #define DEBUG_DRAW_IMPLEMENTATION
#include "DebugDraw.hpp"
#include "../dependencies/debug-draw/samples/vectormath/vectormath.h"

// #include <cstdlib>
// #include <tuple>

// #define NOIME
// #define NOMINMAX
// #define WIN32_LEAN_AND_MEAN
// #include <ShellScalingAPI.h>
// #include <windows.h>

// #include <DirectXMath.h>
// #include <d3d9.h>
// #include <d3dcompiler.h>
// #include <dxgi.h> // redefs

// #pragma comment(lib, "Shcore")
// #pragma comment(lib, "d3d9")
// #pragma comment(lib, "dxguid")
// #pragma comment(lib, "d3dcompiler")

static bool g_enabled = false;

Vector2f get_window_dimensions() {
    //return Vector2f{*(float*)0x00832914, *(float*)0x00832918};
    float x = 1920.0f;
    float y = 1080.0f; // these aren't in static memory and I'm yet to get a pointer
    return Vector2f(x, y);
}

// Math.hpp defines Vector4 so I use the full name here and in the vectormath hpp file :harold:

// ========================================================
// DebugDraw Camera
// ========================================================

// Angle in degrees to angle in radians for sin/cos/etc.
static inline float degToRad(const float degrees) {
    return (degrees * 3.1415926535897931f / 180.0f);
}

struct Camera {
    //
    // Camera Axes:
    //
    //    (up)
    //    +Y   +Z (forward)
    //    |   /
    //    |  /
    //    | /
    //    + ------ +X (right)
    //  (eye)
    //
    Vectormath::Aos::Vector3 right;
    Vectormath::Aos::Vector3 up;
    Vectormath::Aos::Vector3 forward;
    Vectormath::Aos::Vector3 eye;
    Matrix4 viewMatrix;
    Matrix4 projMatrix;
    Matrix4 vpMatrix;
    float windowDims[2];

    // Frustum planes for clipping:
    enum { A, B, C, D };
    Vectormath::Aos::Vector4 planes[6];

    // Tunable values:
    float movementSpeed = 3.0f;
    float lookSpeed     = 6.0f;

    enum MoveDir {
        Forward, // Move forward relative to the camera's space
        Back,    // Move backward relative to the camera's space
        Left,    // Move left relative to the camera's space
        Right    // Move right relative to the camera's space
    };
    Camera() = delete;
    Camera(float WindowWidth, float WindowHeight) {
        right      = Vectormath::Aos::Vector3(1.0f, 0.0f, 0.0f);
        up         = Vectormath::Aos::Vector3(0.0f, 1.0f, 0.0f);
        forward    = Vectormath::Aos::Vector3(0.0f, 0.0f, 1.0f);
        eye        = Vectormath::Aos::Vector3(0.0f, 0.0f, 0.0f);
        viewMatrix = Matrix4::identity();
        vpMatrix   = Matrix4::identity();

        windowDims[0] = WindowWidth;
        windowDims[1] = WindowHeight;

        for (int i = 0; i < 6; ++i) {
            planes[i] = Vectormath::Aos::Vector4(0.0f);
        }
    }

    void updateMatrices(uCameraCtrl* devil3cam) {
        const float fovY   = devil3cam->FOV;
        const float aspect = windowDims[0] / windowDims[1];
        projMatrix         = Matrix4::perspective(fovY, aspect, 0.1f, 100.0f);

        // wtf do you mean theres 3 vectors to ID
        //viewMatrix = Matrix4::lookAt(toPoint3(devil3cam->eye), toPoint3(devil3cam->lookat), devil3cam->up);
        vpMatrix   = projMatrix * viewMatrix; // Vectormath lib uses column-major OGL style, so multiply P*V*M

        // Compute and normalize the 6 frustum planes:
        const float* const m = toFloatPtr(vpMatrix);
        planes[0][A]         = m[3] - m[0];
        planes[0][B]         = m[7] - m[4];
        planes[0][C]         = m[11] - m[8];
        planes[0][D]         = m[15] - m[12];
        planes[0]            = normalize(planes[0]);
        planes[1][A]         = m[3] + m[0];
        planes[1][B]         = m[7] + m[4];
        planes[1][C]         = m[11] + m[8];
        planes[1][D]         = m[15] + m[12];
        planes[1]            = normalize(planes[1]);
        planes[2][A]         = m[3] + m[1];
        planes[2][B]         = m[7] + m[5];
        planes[2][C]         = m[11] + m[9];
        planes[2][D]         = m[15] + m[13];
        planes[2]            = normalize(planes[2]);
        planes[3][A]         = m[3] - m[1];
        planes[3][B]         = m[7] - m[5];
        planes[3][C]         = m[11] - m[9];
        planes[3][D]         = m[15] - m[13];
        planes[3]            = normalize(planes[3]);
        planes[4][A]         = m[3] - m[2];
        planes[4][B]         = m[7] - m[6];
        planes[4][C]         = m[11] - m[10];
        planes[4][D]         = m[15] - m[14];
        planes[4]            = normalize(planes[4]);
        planes[5][A]         = m[3] + m[2];
        planes[5][B]         = m[7] + m[6];
        planes[5][C]         = m[11] + m[10];
        planes[5][D]         = m[15] + m[14];
        planes[5]            = normalize(planes[5]);
    }

    Point3 getTarget() const { return Point3(eye[0] + forward[0], eye[1] + forward[1], eye[2] + forward[2]); }

    bool isPointInsideFrustum(const float x, const float y, const float z) const {
        for (int i = 0; i < 6; ++i) {
            if ((planes[i][A] * x + planes[i][B] * y + planes[i][C] * z + planes[i][D]) <= 0.0f) {
                return false;
            }
        }
        return true;
    }
};

static uintptr_t g_detour_jmp = NULL;
// clang-format off
naked void detour(void) {
    _asm {
		// pushad
		// push esi
		// call draw_sphere_maybe
		// pop esi
		// popad
		// pop esi
		// pop ebp
		cmp [edi+0x16D0], esi // originalcode
		jmp DWORD PTR [g_detour_jmp]
	}
}
// clang-format on

std::optional<std::string> DebugDraw::on_initialize() {
	// dd_render_iface = new RenderInterfaceD3D9;
	// dd::initialize(&dd_context, dd_render_iface);
	// if (!install_hook_offset(0x3A82F3, m_function_hook, &detour, &g_detour_jmp, 6)) { // this address just compares lockon, threw something in that will only tick when player is loaded
	// 	return "error";
	// }
  return Mod::on_initialize();
}

void DebugDraw::custom_imgui_window() {
    /*
    if (!g_enabled) { return; }
    uPlayer* pl = devil4_sdk::get_local_player();
    if (!pl) { return; }
    auto screen = world_to_screen(pl->m_pos);
    if (!screen.has_value()) { return; }
    //dd::xzSquareGrid(dd_context, -5000, 5000, pl->Position.y, 1.5f, dd::colors::Green);
    ddVec3 origin = {pl->m_pos.x, pl->m_pos.y, pl->m_pos.z};
    dd::sphere(dd_context, origin, dd::colors::Blue, 50.0f);
    //dd::point(dd_context, origin, dd::colors::Crimson, 15.0f);
    dd::flush(dd_context, ImGui::GetIO().DeltaTime);
    */

    /*
    // can I make an imgui inside an imgui? ooh i can cool
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Yo");

    Vector3f playerPos = (Vector3f)(pl->m_pos.x, pl->m_pos.y, pl->m_pos.z);
    std::optional<Vector2> playerPosScreen = world_to_screen(playerPos);
    ImVec2 playerPosScreenIm = ImVec2(playerPosScreen.value().x, playerPosScreen.value().y);

    ImGui::SetWindowPos(playerPosScreenIm);
    ImGui::SetWindowSize(ImVec2(200, 200));
    ImGui::End();
    */
}

void DebugDraw::on_config_load(const utility::Config& cfg) {
    g_enabled = cfg.get<bool>("draw_hitspheres").value_or(false);
}

void DebugDraw::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("draw_hitspheres", g_enabled);
}

void DebugDraw::on_gui_frame() {
	ImGui::Checkbox("Draw Hitspheres", &g_enabled);
	ImGui::SameLine();
    help_marker("WIP");
}
#endif
