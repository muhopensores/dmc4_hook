#include "hacklib/D3DDeviceFetcher.h"
#include "hacklib/Hooker.h"
#include <Windows.h>
#include <condition_variable>
#include <mutex>


using namespace hl;


std::mutex g_mutex;
std::condition_variable g_condVar;
LPDIRECT3DDEVICE9 g_pD3D9Device;
IDXGISwapChain* g_pD3D11SwapChain;
ID3D11Device* g_pD3D11Device;
ID3D11DeviceContext* g_pD3D11DeviceContext;


class DummyWindow
{
public:
    ~DummyWindow()
    {
        if (m_hWnd)
        {
            DestroyWindow(m_hWnd);
            UnregisterClassA("DXTMP", GetModuleHandleA(NULL));
        }
    }
    HWND create()
    {
        WNDCLASSEXA wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.lpszClassName = "DXTMP";
        RegisterClassExA(&wc);

        m_hWnd =
            CreateWindowA("DXTMP", 0, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), 0, wc.hInstance, 0);

        return m_hWnd;
    }

private:
    HWND m_hWnd = NULL;
};


static void cbEndScene(hl::CpuContext* ctx)
{
    std::lock_guard<std::mutex> lock(g_mutex);
#ifdef ARCH_64BIT
    // Microsoft x64
    g_pD3D9Device = (LPDIRECT3DDEVICE9)ctx->RCX;
#else
    // __stdcall
    g_pD3D9Device = *(LPDIRECT3DDEVICE9*)(ctx->ESP + sizeof(void*));
#endif
    g_condVar.notify_one();
}

IDirect3DDevice9* D3DDeviceFetcher::GetD3D9Device(int timeout)
{
    // clean globals
    g_pD3D9Device = NULL;

    DummyWindow wnd;
    auto hWnd = wnd.create();
    if (!hWnd)
        return NULL;

    // create a dummy d3d device
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D)
        return NULL;
    D3DPRESENT_PARAMETERS d3dPar = { 0 };
    d3dPar.Windowed = TRUE;
    d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
    LPDIRECT3DDEVICE9 pDev = NULL;
    pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPar,
                       &pDev);
    if (!pDev)
        return NULL;

    // fetch the location of endscene in the d3d9 module via vtable of dummy device
    auto endScene = ((uintptr_t**)pDev)[0][42];

    // clean up dummy device
    pDev->Release();
    pD3D->Release();

    Hooker hooker;
    hooker.hookVEH(endScene, cbEndScene);

    std::unique_lock<std::mutex> lock(g_mutex);
    g_condVar.wait_for(lock, std::chrono::milliseconds(timeout), [] { return g_pD3D9Device != NULL; });

    return g_pD3D9Device;
}


static void cbPresent(hl::CpuContext* ctx)
{
    std::lock_guard<std::mutex> lock(g_mutex);
#ifdef ARCH_64BIT
    // Microsoft x64
    g_pD3D11SwapChain = (IDXGISwapChain*)ctx->RCX;
#else
    // __stdcall
    g_pD3D11SwapChain = *(IDXGISwapChain**)(ctx->ESP + sizeof(void*));
#endif
    g_pD3D11SwapChain->GetDevice(__uuidof(g_pD3D11Device), (void**)&g_pD3D11Device);
    g_pD3D11Device->GetImmediateContext(&g_pD3D11DeviceContext);

    // The above calls increased the reference counts, so decrease it again.
    g_pD3D11Device->Release();
    g_pD3D11DeviceContext->Release();

    g_condVar.notify_one();
}

D3DDeviceFetcher::D3D11COMs D3DDeviceFetcher::GetD3D11Device(int timeout)
{
    g_pD3D11SwapChain = NULL;
    g_pD3D11Device = NULL;
    g_pD3D11DeviceContext = NULL;

    D3D11COMs d3d11;

    DummyWindow wnd;
    auto hWnd = wnd.create();
    if (!hWnd)
        return d3d11;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Windowed = TRUE;

    IDXGISwapChain* pSwapChain = NULL;
    ID3D11Device* pDevice = NULL;
    ID3D11DeviceContext* pDeviceContext = NULL;
    HRESULT hr =
        D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION,
                                      &swapChainDesc, &pSwapChain, &pDevice, NULL, &pDeviceContext);
    if (!pSwapChain || !pDevice || !pDeviceContext)
        return d3d11;

    auto present = ((uintptr_t**)pSwapChain)[0][8];

    pDevice->Release();
    pDeviceContext->Release();
    pSwapChain->Release();

    Hooker hooker;
    hooker.hookVEH(present, cbPresent);

    std::unique_lock<std::mutex> lock(g_mutex);
    g_condVar.wait_for(lock, std::chrono::milliseconds(timeout), [] {
        return g_pD3D11SwapChain != NULL && g_pD3D11Device != NULL && g_pD3D11DeviceContext != NULL;
    });

    d3d11.pSwapChain = g_pD3D11SwapChain;
    d3d11.pDevice = g_pD3D11Device;
    d3d11.pDeviceContext = g_pD3D11DeviceContext;

    return d3d11;
}