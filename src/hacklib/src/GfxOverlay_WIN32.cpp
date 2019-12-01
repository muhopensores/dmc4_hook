#include "hacklib/GfxOverlay.h"
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <dwmapi.h>


using namespace hl;


static std::atomic<int> g_wndClassRefCount{ 0 };
static const char WNDCLASS_OVERLAY[] = "HL_GFXOVERLAY";


class hl::GfxOverlayImpl
{
public:
    D3DPRESENT_PARAMETERS getPresentParams() const;

public:
    GfxOverlay* overlay = nullptr;
    IDirect3D9* d3d = nullptr;
    IDirect3DDevice9* device = nullptr;
};


static LRESULT CALLBACK s_WndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wparam, lparam);
}


D3DPRESENT_PARAMETERS GfxOverlayImpl::getPresentParams() const
{
    DWORD multiSampleQuality = 0;
    D3DFORMAT depthFormat = D3DFMT_D16;

    D3DDISPLAYMODE D3Ddm = {};
    if (d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &D3Ddm) == D3D_OK)
    {
        // use best multisampling available
        if (d3d->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, TRUE,
                                            D3DMULTISAMPLE_NONMASKABLE, &multiSampleQuality) != D3D_OK)
        {
            // D3D may set this value even on failure, so reset it.
            multiSampleQuality = 0;
        }

        // use a better depth buffer if available and compatible with our backbuffer
        if (d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, D3DUSAGE_DEPTHSTENCIL,
                                   D3DRTYPE_SURFACE, D3DFMT_D24S8) == D3D_OK &&
            d3d->CheckDepthStencilMatch(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3Ddm.Format, D3DFMT_A8R8G8B8,
                                        D3DFMT_D24S8) == D3D_OK)
        {
            depthFormat = D3DFMT_D24S8;
        }
    }

    // the backbuffer will be created with the format D3DFMT_A8R8G8B8 so it has an alpha channel.
    // so when drawing something the alphachannel will not be calculated and combined to a opaque color.
    // instead the alpha value is written to the backbuffer and can be presented to the alphablendable frontbuffer.

    D3DPRESENT_PARAMETERS D3Dparams = {};
    D3Dparams.BackBufferWidth = 0;
    D3Dparams.BackBufferHeight = 0;
    D3Dparams.BackBufferFormat = D3DFMT_A8R8G8B8; // Alpha is needed in the backbuffer.
    D3Dparams.BackBufferCount = 0;
    D3Dparams.MultiSampleType = multiSampleQuality ? D3DMULTISAMPLE_NONMASKABLE : D3DMULTISAMPLE_NONE;
    D3Dparams.MultiSampleQuality = multiSampleQuality ? multiSampleQuality - 1 : 0; // Must be valid even if not used.
    D3Dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    D3Dparams.hDeviceWindow = overlay->m_hWnd;
    D3Dparams.Windowed = TRUE;
    D3Dparams.EnableAutoDepthStencil = TRUE;
    D3Dparams.AutoDepthStencilFormat = depthFormat;
    D3Dparams.Flags = 0;
    D3Dparams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    D3Dparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    return D3Dparams;
}


bool GfxOverlay::IsCompositionEnabled()
{
    BOOL check = FALSE;
    if (DwmIsCompositionEnabled(&check) != S_OK)
        return false;
    return check != FALSE;
}


GraphicsContext GfxOverlay::getContext() const
{
    return m_impl->device;
}


void GfxOverlay::resetContext()
{
    D3DPRESENT_PARAMETERS D3Dparams = m_impl->getPresentParams();
    m_impl->device->Reset(&D3Dparams);

    // magic. combined with the stuff above, this makes the window background completly transparent.
    // when something is drawn to the window frontbuffer, it will be visibe tough and it can use transparency.

    MARGINS margs;
    margs.cxLeftWidth = m_posX;
    margs.cxRightWidth = m_width;
    margs.cyTopHeight = m_posY;
    margs.cyBottomHeight = m_height;
    DwmExtendFrameIntoClientArea(m_hWnd, &margs);

    // set correct values for alphablending

    m_impl->device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_impl->device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_impl->device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // turn lighting off by default

    m_impl->device->SetRenderState(D3DRS_LIGHTING, FALSE);
}

void GfxOverlay::beginDraw()
{
    m_impl->device->BeginScene();
    m_impl->device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
}

void GfxOverlay::swapBuffers()
{
    m_impl->device->EndScene();
    m_impl->device->Present(NULL, NULL, NULL, NULL);

    auto until = m_lastSwap + m_frameTime;
    m_lastSwap = Clock::now();
    std::this_thread::sleep_until(until);
}


void GfxOverlay::impl_construct()
{
    m_impl = new GfxOverlayImpl;
    m_impl->overlay = this;

    if (!m_hModule)
    {
        m_hModule = GetModuleHandle(NULL);
    }

    if (!g_wndClassRefCount.fetch_add(1))
    {
        WNDCLASSEX wndClass = {};
        wndClass.cbSize = sizeof(WNDCLASSEX);
        wndClass.style = CS_HREDRAW | CS_VREDRAW;
        wndClass.lpfnWndProc = s_WndProc;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = sizeof(this);
        wndClass.hInstance = m_hModule;
        wndClass.hIcon = NULL;         // the icon will not be visible (WS_EX_NOACTIVATE, WS_POPUP)
        wndClass.hCursor = NULL;       // the cursor will not be visible (WS_EX_TRANSPARENT)
        wndClass.hbrBackground = NULL; // tell windows that we draw the background
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = WNDCLASS_OVERLAY;
        wndClass.hIconSm = NULL;

        RegisterClassEx(&wndClass);
    }
}

void GfxOverlay::impl_destruct()
{
    if (g_wndClassRefCount.fetch_sub(1) == 1)
    {
        UnregisterClass(WNDCLASS_OVERLAY, m_hModule);
    }

    delete m_impl;
}

void GfxOverlay::impl_close()
{
    PostMessage(m_hWnd, WM_CLOSE, 0, 0);
}

void GfxOverlay::impl_windowThread(std::promise<Error>& p)
{
    // create a layered window. a layered window has a frontbuffer that is alphablendable.

    m_hWnd =
        CreateWindowEx(WS_EX_LAYERED |         // window can be alphablended
                           WS_EX_TRANSPARENT | // window does not obscure any windows beneath eg clicks fall through
                           WS_EX_TOPMOST |     // window placed and stays above all nontopmost windows
                           WS_EX_NOACTIVATE,   // window does not appear in the taskbar
                       WNDCLASS_OVERLAY, "Hacklib Overlay",
                       WS_VISIBLE |  // window is initially visible
                           WS_POPUP, // window has no frame or border
                       m_posX, m_posY, m_width, m_height, HWND_DESKTOP, NULL, m_hModule, m_impl);

    if (!m_hWnd)
    {
        p.set_value(Error::Window);
        return;
    }

    // after the CreateWindowEx call, the layered window will not become visible until
    // the SetLayeredWindowAttributes or UpdateLayeredWindow function has been called for this window.

    // set the layered window to be fully opaque and not use a transparency key.
    // it still has an alphablendable frontbuffer though.

    if (!SetLayeredWindowAttributes(m_hWnd, 0, 255, LWA_ALPHA))
    {
        p.set_value(Error::Other);
        return;
    }

    // create a usual d3d9 device, but with alphablendable backbuffer, see getPresentParams
    m_impl->d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_impl->d3d)
    {
        p.set_value(Error::Context);
        return;
    }

    D3DPRESENT_PARAMETERS D3Dparams = m_impl->getPresentParams();

    if (m_impl->d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
                                  D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3Dparams,
                                  &m_impl->device) != D3D_OK &&
        m_impl->d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd,
                                  D3DCREATE_MULTITHREADED | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3Dparams,
                                  &m_impl->device) != D3D_OK)
    {
        p.set_value(Error::Context);
        return;
    }

    resetContext();

    p.set_value(Error::Okay);


    typedef std::chrono::high_resolution_clock Clock;
    auto timestamp = Clock::now();
    MSG msg;
    do
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            cbWindowLoop();

            auto until = timestamp + std::chrono::milliseconds(25);
            timestamp = Clock::now();
            std::this_thread::sleep_until(until);
        }
    } while (msg.message != WM_QUIT);

    if (m_impl->device)
        m_impl->device->Release();
    if (m_impl->d3d)
        m_impl->d3d->Release();

    m_isOpen = false;
}
