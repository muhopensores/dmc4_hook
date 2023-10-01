#if 1
#include "DebugDraw.hpp"
#define DEBUG_DRAW_IMPLEMENTATION
#define DEBUG_DRAW_EXPLICIT_CONTEXT
#include "../dependencies/DebugDraw/debug_draw.hpp"
#include <DirectXMath.h>
#include <d3d9.h>
#include <wrl.h>
#include <D3dx9math.h>

#include <glm/glm.hpp> //basic vector/matrix math and defs
#include <glm/gtc/matrix_transform.hpp> //we will use this to make tranformation matrices

////////////////////////////////////////////////////////////////////////

class cCameraCtrl {
public:
    Vector3 upVector;    // 0x0004
    float fltUnused0;    // 0x0010
    float FOV;           // 0x0014
    float roll_radians;  // 0x0018
    Matrix4x4 transform; // 0x001C
    Vector4 pos;         // 0x005C
    Vector4 lookat;      // 0x006C
    friend static int cCameraCtrl__something_idk_sub_416880(cCameraCtrl& this_ptr);
    virtual void Function0();
    virtual void Function1();
    virtual void Function2();
    virtual void Function3();
    virtual void Function4();
    virtual void Function5();
    virtual void Function6();
    virtual void Function7();
    virtual void Function8();
    virtual void Function9();
}; // Size: 0x007C
static_assert(sizeof(cCameraCtrl) == 0x7C);

class CPlDante {
public:
    int pad_0000;                              // 0x0000
    uint8_t unkFlag;                           // 0x0004
    char pad_0005[11];                         // 0x0005
    float workrate;                            // 0x0010
    float speed;                               // 0x0014
    char pad_0018[52];                         // 0x0018
    Vector4 Poistion;                          // 0x004C
    Vector4 DeltaP;                            // 0x005C
    char pad_006C[4];                          // 0x006C
    float weightMaybe;                         // 0x0070
    char pad_0074[24];                         // 0x0074
    uint16_t angle;                            // 0x008C
    char pad_008E[110];                        // 0x008E
    Vector4 somePosVector;                     // 0x00FC
    Vector4 somePrevPosVector;                 // 0x010C
    Matrix4x4 TransformMatrix;                 // 0x011C
    float momentumMagnitude;                   // 0x015C cm
    float momentumDelta;                       // 0x0160
    char pad_0164[9736];                       // 0x0164
    class VelLookupTable* velocityLookupTable; // 0x276C 0x276c zeroesUpMomentum_sub_5A4CB20
    char pad_2770[32];                         // 0x2770
    uint32_t animationStatus01;                // 0x2790
    char pad_2794[4];                          // 0x2794
    uint32_t animationStatusPrev;              // 0x2798
    char pad_279C[4];                          // 0x279C
    uint8_t animationFlagUnk;                  // 0x27A0
    char pad_27A1[35];                         // 0x27A1
    float someAnimationVelocityFloat;          // 0x27C4 walking when below 2
    char pad_27C8[144];                        // 0x27C8
    uint16_t stickDirection;                   // 0x2858
    uint16_t someOtherDirection;               // 0x285A
    uint16_t anotherDirection;                 // 0x285C
    uint16_t idkDirection;                     // 0x285E
    uint16_t viewDirection;                    // 0x2860
    char pad_2862[22];                         // 0x2862
    uint8_t N00000AD3;                         // 0x2878
    uint8_t N00000FDA;                         // 0x2879
    char pad_287A[2];                          // 0x287A
    uint32_t N00000AD4;                        // 0x287C 0x287c edx at start zeroesUpMomentum_sub_5A4CB0ú
    char pad_2880[5044];                       // 0x2880
    uint32_t rg_meter_uint;                    // 0x3C34
    float rg_meter_flt;                        // 0x3C38
    float block_timer;                         // 0x3C3C
    float release_timer;                       // 0x3C40
    char pad_3C44[108];                        // 0x3C44
};                                             // Size: 0x3CB0
static_assert(sizeof(CPlDante) == 0x3CB0);

namespace Devil3SDK {
CPlDante* get_pl_dante();
cCameraCtrl* get_cam_ctrl();
Vector2f get_window_dimensions();
} // namespace Devil3SDK

CPlDante* Devil3SDK::get_pl_dante() {
    return (CPlDante*)0x1C8A600;
}

cCameraCtrl* Devil3SDK::get_cam_ctrl() {
    static cCameraCtrl** camera_ptr_ptr = (cCameraCtrl**)0x00B6BAAC;
    return *camera_ptr_ptr;
}

Vector2f Devil3SDK::get_window_dimensions() {
    return Vector2f{*(float*)0x00832914, *(float*)0x00832918};
}

////////////////////////////////////////////////////////////////////////

bool g_enabled = false;
constexpr int STRIDE_MAGIC = 6;
using Microsoft::WRL::ComPtr;

std::optional<Vector2> world_to_screen(const Vector3f& world_pos) {
	cCameraCtrl* camera = Devil3SDK::get_cam_ctrl();
	if (!camera || camera == (cCameraCtrl*)-1) { return Vector2{ 0.0f, 0.0f }; };

	Vector2f window = Devil3SDK::get_window_dimensions();
	
	float near_plane = 0.1f; //nearest distance from which you can see
	float far_plane  = 100.f; //you cant see more
	float aspect = window.x / window.y;

	//the perspective matrix
	glm::mat4 projection = glm::perspective(camera->FOV*-1.0f, aspect, near_plane, far_plane);

	glm::mat4 model = camera->transform;
	auto res = glm::project(world_pos, model, projection, Vector4{ 0.0f, 0.0f, window.x, window.y });
	return Vector2{ res.x, res.y };
}

class RenderInterfaceD3D9 final
	: public dd::RenderInterface
{
public:
	void setCameraFrame(const Vector3 & up, const Vector3 & right, const Vector3 & origin)
	{
		camUp = up; camRight = right; camOrigin = origin;
	}

	void beginDraw() override
	{
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::SetNextWindowPos(ImVec2{ 0, 0 });
		ImGui::SetNextWindowSize(ImVec2{1920,1080});
		auto imgui_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
		ImGui::Begin("WEW", 0, imgui_flags);
		draw_list = ImGui::GetWindowDrawList();
	}

	void endDraw() override
	{
		ImGui::End();
		// No work done here at the moment.
	}

	void drawPointList(const dd::DrawVertex * points, int count, bool depthEnabled) override
	{
		(void)depthEnabled; // TODO: not implemented yet - not required by this sample

							// Emulating points as billboarded quads, so each point will use 6 vertexes.
							// D3D11 doesn't support "point sprites" like OpenGL (gl_PointSize).
		const int maxVerts = DEBUG_DRAW_VERTEX_BUFFER_SIZE / 6;

		const float D3DPointSpriteScalingFactor = 1.0f;

		assert(points != nullptr);
		assert(count > 0 && count <= maxVerts);

		// Map the vertex buffer:

		const int numVerts = count * 6;
		//const int indexes[6] = {0, 1, 2, 2, 3, 0};

		// Expand each point into a quad:
		for (int p = 0; p < count; ++p)
		{
			const float ptSize      = points[p].point.size * D3DPointSpriteScalingFactor;
			const Vector2 halfWidth = (ptSize * 0.5f) * Vector2f(1.0f, 0.0f); // X
			const Vector2 halfHeigh = (ptSize * 0.5f) * Vector2f(0.0f, -1.0f);//camUp;    // Y
			const Vector3 origin    = Vector3{ points[p].point.x, points[p].point.y, points[p].point.z };
			const Vector2 sorigin   = world_to_screen(origin).value();

			Vector2 corners[4];
			corners[0] = sorigin + halfWidth + halfHeigh;
			corners[1] = sorigin - halfWidth + halfHeigh;
			corners[2] = sorigin - halfWidth - halfHeigh;
			corners[3] = sorigin + halfWidth - halfHeigh;

			Vector2 center = world_to_screen(origin).value();
			draw_list->AddQuadFilled(corners[0], corners[1], corners[2], corners[3],
				ImColor((int)(points[p].point.r * 255.0f), (int)(points[p].point.g * 255.0f), (int)(points[p].point.b * 255.0f)));
		}
	}

	void drawLineList(const dd::DrawVertex * lines, int count, bool depthEnabled) override
	{
		(void)depthEnabled; // TODO: not implemented yet - not required by this sample

		assert(lines != nullptr);
		assert(count > 0 && count <= DEBUG_DRAW_VERTEX_BUFFER_SIZE);

		for (int v = 1; v < count; ++v)
		{
			Vector3 pp1; 
			Vector3 pp2;
			Vector2 p1, p2;
			pp1 = *(Vector3*)&lines[v - 1];
			pp2 = *(Vector3*)&lines[v];
			p1 = world_to_screen(pp1).value();
			p2 = world_to_screen(pp2).value();

            draw_list->PathLineTo(p1);
            if (v % STRIDE_MAGIC != 0) {
                draw_list->PathLineTo(p2);
            }
            
                draw_list->PathStroke(
                    ImColor(
                    (int)(lines[v].line.r * 255.0f),
                        (int)(lines[v].line.g * 255.0f),
                        (int)(lines[v].line.b * 255.0f), 64), 0, 0.5f);
		}
		
	}

	struct ConstantBufferData
	{
		DirectX::XMMATRIX mvpMatrix        = DirectX::XMMatrixIdentity();
	};

	struct Vertex
	{
		DirectX::XMFLOAT4A pos;   // 3D position
		DirectX::XMFLOAT4A uv;    // Texture coordinates
		DirectX::XMFLOAT4A color; // RGBA float
	};

	//
	// Members:
	//

	ImDrawList* draw_list = nullptr;

	ComPtr<IDirect3DDevice9>          p_device;

	// Camera vectors for the emulated point sprites
	Vector3                       camUp     = Vector3{0.0f};
	Vector3                       camRight  = Vector3{0.0f};
	Vector3                       camOrigin = Vector3{0.0f};

};

static RenderInterfaceD3D9* dd_render_iface = nullptr;
static dd::ContextHandle dd_context = nullptr;

class colisioni
{
public:
	uint32_t flags_maybe; //0x0000
	uint32_t idk01; //0x0004
	uint32_t idk02; //0x0008
	uint32_t idk03; //0x000C
	float radius_maybe; //0x0010
	float idk_float; //0x0014
	char pad_0018[4]; //0x0018
	Matrix4x4 tranform01; //0x001C
	Matrix4x4 tranform02; //0x005C
	char pad_009C[32]; //0x009C
	Vector4 pos01; //0x00BC
	Vector4 pos02; //0x00CC
	Vector4 pos03; //0x00DC
	char pad_00EC[24]; //0x00EC
	Vector4 pos04; //0x0104
	float rad01; //0x0114
	char pad_0118[412]; //0x0118
}; //Size: 0x02B4
static_assert(sizeof(colisioni) == 0x2B4);


static void draw_sphere_maybe(colisioni* col) {
	if (!g_enabled) { return; }

	/*auto origin = Vector4f{ col->tranform01[3][0], col->tranform01[3][1], col->tranform01[3][2], 1.0f };
	auto pos = rot * origin;*/
	auto right = Vector3f{ col->tranform01[0][0], col->tranform01[0][1], col->tranform01[0][2] };
	auto up = Vector3f{ col->tranform01[1][0], col->tranform01[1][1], col->tranform01[1][2] };
	auto forward = Vector3f{ col->tranform01[2][0], col->tranform01[2][1], col->tranform01[2][2] };

	dd::circle(dd_context, *(ddVec3*)&col->pos01, *(ddVec3*)&up, dd::colors::Coral, col->radius_maybe, 8, 32);
	dd::circle(dd_context, *(ddVec3*)&col->pos01, *(ddVec3*)&right, dd::colors::Chartreuse, col->radius_maybe, 8, 32);
	dd::circle(dd_context, *(ddVec3*)&col->pos01, *(ddVec3*)&forward, dd::colors::Crimson, col->radius_maybe, 8, 32);
}

static uintptr_t g_detour_jmp = NULL;
// clang-format off
naked void detour(void) {
    _asm {
		fstp dword ptr [esi+10h]
		pushad
		push esi
		call draw_sphere_maybe
		pop esi
		popad
		pop esi
		pop ebp

		jmp DWORD PTR [g_detour_jmp]
	}
}
// clang-format on

std::optional<std::string> DebugDraw::on_initialize() {
	dd_render_iface = new RenderInterfaceD3D9;
	dd::initialize(&dd_context, dd_render_iface);
	if (!install_hook_absolute(0x006617B4, m_function_hook, &detour, &g_detour_jmp, 5)) {
		return "error";
	}
  return Mod::on_initialize();
}

void DebugDraw::custom_imgui_window()
{
	if (!g_enabled) { return; }
	CPlDante* pl = Devil3SDK::get_pl_dante();
	// keeping capcom traditions of typoing member fields like a motehfckure
	auto screen = world_to_screen(pl->Poistion);
	if (!screen.has_value()) { return; }
	//dd::xzSquareGrid(dd_context, -5000, 5000, pl->Poistion.y, 1.5f, dd::colors::Green);
	ddVec3 origin = { pl->Poistion.x, pl->Poistion.y, pl->Poistion.z };
	dd::sphere(dd_context, origin, dd::colors::Blue, 10.0f);
	//dd::point(dd_context, origin, dd::colors::Crimson, 15.0f);
	dd::flush(dd_context, ImGui::GetIO().DeltaTime);
}

void DebugDraw::on_gui_frame() {
	ImGui::Checkbox("Draw Hitspheres", &g_enabled);
}
#endif
