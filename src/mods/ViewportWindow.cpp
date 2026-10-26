#include "ViewportWindow.hpp"
#include "../sdk/Devil4.hpp"

#include <algorithm>
#include <atomic>
#include <thread>

namespace vpwin {

bool mod_enabled    = false;
int  viewport_index = 3;
int  width          = 1280;
int  height         = 720;
bool full_main      = true;

// The render target, made in memory (no .rtex file): rRenderTargetTexture's DTI newInstance builds an empty one
// (rTexture ctor: refcount 1, mID 0, so it's in no sResource table), then sub_A69380 does what the .rtex load
// (vtbl slot 8, sub_A69480) does after parsing the header: makes the cTrans render target and stores it at +0xCC.
// The load's texture-quality shrink and screen-size clamp are skipped. Releasing it to refcount 0 runs its destructor
// (frees the target); cResource::release's table lookup in between finds nothing for mID 0 and returns.
static uintptr_t rt_new_instance = 0x00A69280; // rRenderTargetTexture::DTI newInstance
static uintptr_t rt_create       = 0x00A69380; // ecx = type (2 = 2D), edi = levels, esi = format; this, w, h, usage-0x4000 flag; retn 10h

static void* g_rt   = nullptr; // our rRenderTargetTexture
static int   g_rt_w = 0, g_rt_h = 0;

static void* make_render_target(int w, int h) {
    void* rt = ((void* (*)())rt_new_instance)();
    if (!rt) return nullptr;
    // what the load writes before creating: 1 / size, size, 0
    *(float*)((uintptr_t)rt + 0xD0)    = 1.0f / (float)w;
    *(float*)((uintptr_t)rt + 0xD4)    = 1.0f / (float)h;
    *(uint32_t*)((uintptr_t)rt + 0xD8) = (uint32_t)w;
    *(uint32_t*)((uintptr_t)rt + 0xDC) = (uint32_t)h;
    *(uint32_t*)((uintptr_t)rt + 0xE0) = 0;
    bool ok = false;
    _asm {
        push 0
        push h
        push w
        push rt
        mov ecx, 2
        mov edi, 1
        mov esi, 0x15
        call dword ptr [rt_create]
        mov ok, al
    }
    if (!ok) {
        devil4_sdk::release_resource((cResource*)rt);
        return nullptr;
    }
    return rt;
}

// The render target's D3D texture: rRenderTargetTexture +0xCC cTransTexture -> +0x10
static IDirect3DTexture9* d3d_texture(void* rt) {
    uintptr_t trans = *(uintptr_t*)((uintptr_t)rt + 0xCC);
    return trans ? *(IDirect3DTexture9**)(trans + 0x10) : nullptr;
}

// The viewport: kept off the screen and on our target every frame (sCamera::reset clears mpCamera on a stage load,
// Coop / DebugCam write mAttr 0x17 and their layout's mMode). sCamera::move -> Viewport::updateRegion sizes it to
// the target; mMode would still cut it (2 = the target's top half), so it's held at 0 (whole).
static auto set_render_target = (void(__thiscall*)(sCamera_ViewPort*, void*))0x008E5E50; // add-refs it + "SceneZBuffer"; null releases both

struct Saved {
    int      index = -1; // the viewport we hold, -1 = none
    uint32_t attr;
    uint8_t  active, mode;
    uint8_t  main_mode;  // viewport 0's
    void*    lent;       // viewport 0's camera, put there by us because it had none (null = not lent)
};
static Saved g_saved;

static sCamera* get_camera_system() {
    return *(sCamera**)0x00E552D4;
}

static void detach() {
    sCamera* cam = get_camera_system();
    if (g_saved.index < 0 || !cam) {
        g_saved.index = -1;
        return;
    }
    sCamera_ViewPort& vp = cam->viewports[g_saved.index];
    if (vp.mpRenderTarget == g_rt) set_render_target(&vp, nullptr);
    if (g_saved.lent && vp.mpCamera == g_saved.lent) vp.mpCamera = nullptr;
    vp.mAttr   = g_saved.attr;
    vp.mActive = g_saved.active;
    vp.mMode   = g_saved.mode;
    cam->viewports[0].mMode = g_saved.main_mode;
    g_saved.index = -1;
}

static void attach() {
    sCamera* cam = get_camera_system();
    if (!cam || !g_rt) return;
    if (g_saved.index != viewport_index) {
        detach();
        sCamera_ViewPort& vp = cam->viewports[viewport_index];
        g_saved.index       = viewport_index;
        g_saved.attr        = vp.mAttr;
        g_saved.active      = vp.mActive;
        g_saved.mode        = vp.mMode;
        g_saved.main_mode   = cam->viewports[0].mMode;
        g_saved.lent        = nullptr;
    }
    sCamera_ViewPort& vp = cam->viewports[viewport_index];
    vp.mActive = 1;
    vp.mAttr &= ~1u; // not on the screen: Viewport::setup renders into mpRenderTarget
    vp.mMode = 0;
    // no camera of its own (or still the one we lent): viewport 0's, again every frame, since that one can change
    // within a stage (cutscenes) and the old pointer would dangle
    if (!vp.mpCamera || (g_saved.lent && vp.mpCamera == g_saved.lent)) {
        vp.mpCamera  = cam->viewports[0].mpCamera;
        g_saved.lent = vp.mpCamera;
    } else {
        g_saved.lent = nullptr; // Coop / DebugCam put a camera there
    }
    if (vp.mpRenderTarget != g_rt) set_render_target(&vp, g_rt);
    if (full_main) cam->viewports[0].mMode = 0;
}

// The window: its own thread and message loop (the game's pump only serves the game's thread). WS_EX_NOACTIVATE: a
// click doesn't take the focus, so the game window keeps it and DirectInput keeps reading. X asks the mod to turn off;
// stop() has the thread destroy the window (DestroyWindow only works on the creating thread) and joins it.
static constexpr UINT WM_VPWIN_DESTROY = WM_APP + 1;

static std::thread       g_thread;
static std::atomic<HWND> g_hwnd{ nullptr };
static std::atomic<bool> g_close_requested{ false };
static std::atomic<bool> g_window_ready{ false }; // the thread got past creating the window (or failed to)

static LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CLOSE:
        g_close_requested = true; // the next frame turns the mod off
        return 0;
    case WM_VPWIN_DESTROY:
        DestroyWindow(wnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(wnd, msg, wp, lp);
}

static void window_thread(int w, int h, int index) {
    HINSTANCE inst = GetModuleHandleA(nullptr);
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "dmc4hook_vpwin";
    RegisterClassExA(&wc); // fails harmlessly when it's already registered (a second start)
    DWORD style    = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = WS_EX_NOACTIVATE | WS_EX_APPWINDOW;
    RECT  r{ 0, 0, w, h };
    AdjustWindowRectEx(&r, style, FALSE, ex_style); // client area = the target's size
    char title[64];
    sprintf_s(title, "DMC4 viewport %d", index);
    HWND wnd = CreateWindowExA(ex_style, wc.lpszClassName, title, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                               r.bottom - r.top, nullptr, nullptr, inst, nullptr);
    if (!wnd) {
        spdlog::error("ViewportWindow: CreateWindowEx failed ({})", GetLastError());
        g_close_requested = true;
        g_window_ready    = true;
        return;
    }
    ShowWindow(wnd, SW_SHOWNOACTIVATE);
    g_hwnd         = wnd;
    g_window_ready = true;
    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    g_hwnd = nullptr;
}

static void open_window() {
    g_close_requested = false;
    g_window_ready    = false;
    g_thread          = std::thread(window_thread, g_rt_w, g_rt_h, viewport_index);
}

static void close_window() {
    if (!g_thread.joinable()) return;
    while (!g_window_ready) Sleep(1); // turned off right after on: let the window exist first, or the post is lost
    if (HWND wnd = g_hwnd) PostMessageA(wnd, WM_VPWIN_DESTROY, 0, 0);
    g_thread.join();
    g_hwnd = nullptr;
}

// The swap chain on our window (made lazily once the window exists; D3DPOOL_DEFAULT, so on_reset drops it and the next
// frame makes it again). Back buffer = the target's size; Present stretches it to the window's client area, so a
// resize needs nothing. The copy runs from on_frame, inside the game's Present hook (0x8F33F5): sRender has dispatched
// (or waited for) the frame's commands by then, so the target holds this frame. The game keeps the target's D3D texture
// (it can rebuild it on a reset), so its surface is fetched and released every frame.
static IDirect3DSwapChain9* g_swap        = nullptr;
static bool                 g_swap_failed = false; // logged once until it works again
static std::string          g_status;              // shown in the GUI

static bool game_windowed(IDirect3DDevice9* dev) {
    IDirect3DSwapChain9* sc = nullptr;
    if (FAILED(dev->GetSwapChain(0, &sc)) || !sc) return false;
    D3DPRESENT_PARAMETERS pp{};
    sc->GetPresentParameters(&pp);
    sc->Release();
    return pp.Windowed != FALSE;
}

static void make_swap_chain(IDirect3DDevice9* dev, HWND wnd) {
    D3DPRESENT_PARAMETERS pp{};
    pp.BackBufferWidth      = g_rt_w;
    pp.BackBufferHeight     = g_rt_h;
    pp.BackBufferFormat     = D3DFMT_UNKNOWN; // the desktop's
    pp.BackBufferCount      = 1;
    pp.SwapEffect           = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow        = wnd;
    pp.Windowed             = TRUE;
    pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // the game's own Present paces the frames
    HRESULT hr = dev->CreateAdditionalSwapChain(&pp, &g_swap);
    if (FAILED(hr)) {
        g_swap = nullptr;
        if (!g_swap_failed) spdlog::error("ViewportWindow: CreateAdditionalSwapChain failed ({:#x})", (uint32_t)hr);
        g_swap_failed = true;
        return;
    }
    g_swap_failed = false;
}

static void release_swap_chain() {
    if (g_swap) {
        g_swap->Release();
        g_swap = nullptr;
    }
}

static void present_frame(IDirect3DDevice9* dev) {
    IDirect3DTexture9* tex = d3d_texture(g_rt);
    if (!tex) return;
    IDirect3DSurface9* src = nullptr;
    IDirect3DSurface9* dst = nullptr;
    if (SUCCEEDED(tex->GetSurfaceLevel(0, &src)) && SUCCEEDED(g_swap->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &dst))) {
        dev->StretchRect(src, nullptr, dst, nullptr, D3DTEXF_NONE); // same size: a plain copy
    }
    if (src) src->Release();
    if (dst) dst->Release();
    g_swap->Present(nullptr, nullptr, nullptr, nullptr, 0); // a lost device fails here; on_reset deals with it
}

static void start() {
    IDirect3DDevice9* dev = g_framework->get_d3d9_device();
    if (!dev || !game_windowed(dev)) {
        g_status = "The game runs fullscreen: a second window needs windowed mode (borderless or windowed).";
        spdlog::error("ViewportWindow: {}", g_status);
        mod_enabled = false;
        return;
    }
    g_status.clear();
    g_rt = make_render_target(width, height);
    if (!g_rt) {
        spdlog::error("ViewportWindow: the render target ({}x{}) wasn't made", width, height);
        mod_enabled = false;
        return;
    }
    g_rt_w = width;
    g_rt_h = height;
    D3DSURFACE_DESC desc{};
    if (IDirect3DTexture9* tex = d3d_texture(g_rt)) tex->GetLevelDesc(0, &desc);
    spdlog::info("ViewportWindow: render target {} {}x{}, D3D {}x{} format {}", g_rt, g_rt_w, g_rt_h, desc.Width,
                 desc.Height, (int)desc.Format);
    open_window();
}

static void stop() {
    release_swap_chain(); // before its window goes
    close_window();
    detach();
    if (g_rt) {
        devil4_sdk::release_resource((cResource*)g_rt);
        g_rt = nullptr;
    }
}

} // namespace vpwin

using namespace vpwin;

std::optional<std::string> ViewportWindow::on_initialize() {
    return Mod::on_initialize();
}

void ViewportWindow::on_frame(fmilliseconds& dt) {
    if (g_close_requested.exchange(false)) mod_enabled = false; // the window's X
    if (mod_enabled && !g_rt) start();
    if (!mod_enabled && g_rt) stop();
    if (!g_rt) return;
    attach();
    HWND wnd = g_hwnd;
    if (!wnd) return;
    IDirect3DDevice9* dev = g_framework->get_d3d9_device();
    if (!dev) return;
    if (!g_swap) make_swap_chain(dev, wnd);
    if (g_swap) present_frame(dev);
}

void ViewportWindow::on_reset() {
    release_swap_chain(); // D3DPOOL_DEFAULT: has to go before the device's Reset
}

void ViewportWindow::after_reset() {
    g_swap_failed = false; // the next frame makes the swap chain again
}

void ViewportWindow::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Viewport window"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Shows a viewport in a window of its own (for a second monitor): Coop's second player, or a DebugCam "
                  "camera on that viewport; with no camera there, the main camera. The game has to run windowed."));
    if (!g_status.empty()) ImGui::TextWrapped("%s", g_status.c_str());
    ImGui::Indent(lineIndent);
    ImGui::SliderInt(_("Viewport"), &viewport_index, 1, 3);
    ImGui::SameLine();
    help_marker(_("The viewport shown in the window. The game draws characters and stages only in viewports 0-3; 0 is the "
                  "main view."));
    ImGui::InputInt(_("Width"), &width, 16, 128);
    ImGui::InputInt(_("Height"), &height, 16, 128);
    ImGui::SameLine();
    help_marker(_("The picture's size in pixels (the window scales it). A change takes effect the next time the window is "
                  "turned on."));
    width  = std::clamp(width, 64, 4096);
    height = std::clamp(height, 64, 4096);
    ImGui::Checkbox(_("Full-screen main view"), &full_main);
    ImGui::SameLine();
    help_marker(_("Keep viewport 0 on the whole screen while the other viewport is in the window (Coop would split it)."));
    if (g_rt) {
        if (IDirect3DTexture9* tex = d3d_texture(g_rt)) {
            float w = 320.0f;
            ImGui::Image((ImTextureID)(intptr_t)tex, ImVec2(w, w * g_rt_h / g_rt_w));
        }
    }
    ImGui::Unindent(lineIndent);
}

void ViewportWindow::on_config_load(const utility::Config& cfg) {
    mod_enabled    = cfg.get<bool>("vpwin_enabled").value_or(mod_enabled);
    viewport_index = cfg.get<int>("vpwin_viewport").value_or(viewport_index);
    width          = cfg.get<int>("vpwin_width").value_or(width);
    height         = cfg.get<int>("vpwin_height").value_or(height);
    full_main      = cfg.get<bool>("vpwin_full_main").value_or(full_main);
    viewport_index = std::clamp(viewport_index, 1, 3);
}

void ViewportWindow::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("vpwin_enabled", mod_enabled);
    cfg.set<int>("vpwin_viewport", viewport_index);
    cfg.set<int>("vpwin_width", width);
    cfg.set<int>("vpwin_height", height);
    cfg.set<bool>("vpwin_full_main", full_main);
}
