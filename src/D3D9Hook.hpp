#pragma once

#include <functional>

#include <d3d9.h>

#include "utility/FunctionHook.hpp"
#include "utility/Patch.hpp"
#include "sdk/ReClass.hpp"

class D3D9Hook {
public:
    //typedef std::function<void(D3D9Hook&)> OnEndSceneFn;
    typedef std::function<void(D3D9Hook&)> OnPresentFn;
    typedef std::function<void(D3D9Hook&)> OnResetFn;

	D3D9Hook() = default;
    virtual ~D3D9Hook();

    bool hook();
    bool unhook();

    //void on_end_scene(OnEndSceneFn fn) { m_on_end_scene = fn; }
    void on_reset(OnResetFn fn)        { m_on_reset     = fn; }
    void on_present(OnPresentFn fn)    { m_on_present   = fn; }

	IDirect3DDevice9* get_device() { return m_device; }
	void set_device(IDirect3DDevice9* p_device) { m_device = p_device; }

	D3DPRESENT_PARAMETERS* get_presentation_params() { return m_presentation_params; }

	//OnEndSceneFn m_on_end_scene{ nullptr };
    OnPresentFn m_on_present{ nullptr };
	OnResetFn m_on_reset{ nullptr };

    IDirect3DDevice9* m_device{ nullptr };
    D3DPRESENT_PARAMETERS* m_presentation_params{ nullptr };

protected:

    bool m_hooked{ false };

    //std::unique_ptr<FunctionHook> m_end_scene_hook{};
    std::unique_ptr<FunctionHook> m_reset_hook{};
    std::unique_ptr<FunctionHook> m_reset_hook2{};

    std::unique_ptr<FunctionHook> m_present_hook{};
    //static HRESULT WINAPI end_scene(LPDIRECT3DDEVICE9 p_device);
    static HRESULT WINAPI reset(IDirect3DDevice9 *p_device, D3DPRESENT_PARAMETERS *p_presentation_parameters);
    static HRESULT WINAPI reset_vtable(IDirect3DDevice9 *p_device, D3DPRESENT_PARAMETERS *p_presentation_parameters);
    static HRESULT WINAPI present(IDirect3DDevice9 *p_device, RECT* p_source_rect, RECT* p_dest_rect, HWND h_dest_window_override, RGNDATA* p_dirty_region);
};
