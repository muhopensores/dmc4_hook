#ifndef HACKLIB_D3DDEVICEFETCHER_H
#define HACKLIB_D3DDEVICEFETCHER_H

// requires dxsdk linked
#include "d3d11.h"
#include "d3d9.h"

namespace hl
{
namespace D3DDeviceFetcher
{
// Retrieves the current d3d device that the current process uses for rendering.
// This is done by hooking EndScene and waiting for the first call to it.
// When target does not call EndScene, the function returns null after timeout.
// Uses VEH hooking, so it is very likely that this will not work while the process
// is being debugged.
IDirect3DDevice9* GetD3D9Device(int timeout = 20000);

struct D3D11COMs
{
    IDXGISwapChain* pSwapChain = NULL;
    ID3D11Device* pDevice = NULL;
    ID3D11DeviceContext* pDeviceContext = NULL;
};
D3D11COMs GetD3D11Device(int timeout = 20000);
}
}

#endif