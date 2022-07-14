#include <algorithm>
#include <spdlog/spdlog.h>

#include "D3D9Hook.hpp"
#include "sdk/ReClass.hpp"

static D3D9Hook* g_d3d9_hook = nullptr;

D3D9Hook::~D3D9Hook() {
    unhook();
}

// this is unfortunate i wanted to do it right this time but pc gaming had another opinion
static uintptr_t present_wrapper_jmp_ret = 0x08F33FA;
__declspec(naked) void present_wrapper(void) {
    __asm {
        call D3D9Hook::present
        jmp dword ptr [present_wrapper_jmp_ret]
    }
}

static uintptr_t reset_jmp_ret = 0x008F138C;
__declspec(naked) void reset_wrapper(void) {
    __asm {
        call D3D9Hook::reset
        jmp dword ptr [reset_jmp_ret]
    }
}


#if 0
static uintptr_t end_scene_jmp_ret = 0;
__declspec(naked) void end_scene_hook(void) {
	__asm {
		pushad
		push eax
		call D3D9Hook::end_scene
		popad
		pop eax
		push eax
		call dword ptr [ecx+0xA8]
		jmp dword ptr [end_scene_jmp_ret]
	}
}
#endif

bool D3D9Hook::hook() {
    spdlog::info("Hooking D3D9");

    g_d3d9_hook = this;

    // didnt fly so well
#if 0
    // assuming these are zero initialized
    SDevil4MainPtr* main_ptr = (SDevil4MainPtr*)0x00E5574C;
    SDevil4Main* s_devil4_main = nullptr;
    do {
        s_devil4_main = main_ptr->mp_devil4_main;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (s_devil4_main == NULL);

    SRender* s_render = nullptr;
    do {
        s_render = s_devil4_main->s_render;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (s_render == NULL);

    IDirect3DDevice9* d3d9_device = nullptr;
    do {
        d3d9_device = s_render->mp_device;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (d3d9_device == NULL);

	m_device = d3d9_device;
#endif

	uintptr_t reset_fn     = 0x008F1387;
	uintptr_t reset2_fn    = 0x008F135A;
    uintptr_t present_fn   = 0x008F33F5;
    //m_reset_fixup = Patch::create(reset_fixup, { 0xEB, 0x0A });
	//uintptr_t end_scene_fn = (*(uintptr_t**)d3d9_device)[42];

    m_reset_hook   = std::make_unique<FunctionHook>(reset_fn, (uintptr_t)&reset_wrapper);
    m_reset_hook2  = std::make_unique<FunctionHook>(reset2_fn, (uintptr_t)&reset_wrapper);

    m_present_hook = std::make_unique<FunctionHook>(present_fn, (uintptr_t)&present_wrapper);

    m_hooked = m_present_hook->create() && m_reset_hook->create() && m_reset_hook2->create();

    return m_hooked;
}

bool D3D9Hook::unhook() {

    if (!m_hooked) { return true; }

    if (m_present_hook->remove() && m_reset_hook->remove() &&  m_reset_hook2->remove()) {
        m_hooked = false;
        return true;
    }

    return false;
}

#if 1

HRESULT WINAPI D3D9Hook::present(IDirect3DDevice9 *p_device, RECT* p_source_rect, RECT* p_dest_rect, HWND h_dest_window_override, RGNDATA* p_dirty_region)
{
    if (g_d3d9_hook->m_on_present) {
        g_d3d9_hook->m_on_present(*g_d3d9_hook);
    }
    /*auto present_fn = d3d9->m_present_hook->get_original<decltype(D3D9Hook::present)>();*/

    return p_device->Present(p_source_rect, p_dest_rect, h_dest_window_override, p_dirty_region);
}
#else
HRESULT WINAPI D3D9Hook::end_scene(LPDIRECT3DDEVICE9 p_device) {

	//d3d9->m_device = pDevice;
	if (g_d3d9_hook->m_on_end_scene) {
        g_d3d9_hook->m_on_end_scene(*g_d3d9_hook);
	}
	//return S_OK;
	auto end_scene_fn = 
        g_d3d9_hook->m_end_scene_hook->get_original<decltype(D3D9Hook::end_scene)>();
	
	return end_scene_fn(p_device);
}

static void end_scene_our(LPDIRECT3DDEVICE9 pDevice) {
	auto d3d9 = g_d3d9_hook;

	d3d9->set_device(pDevice);
	if (d3d9->m_on_end_scene) {
		d3d9->m_on_end_scene(*d3d9);
	}
}
#endif


HRESULT WINAPI D3D9Hook::reset(IDirect3DDevice9 *p_device, D3DPRESENT_PARAMETERS *p_presentation_parameters) {

    g_d3d9_hook->m_presentation_params = p_presentation_parameters;
    g_d3d9_hook->m_device = p_device;

	if (g_d3d9_hook->m_on_reset) {
        g_d3d9_hook->m_on_reset(*g_d3d9_hook);
	}

	return p_device->Reset(p_presentation_parameters);
}
