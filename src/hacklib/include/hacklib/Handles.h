#ifndef HACKLIB_HANDLES_H
#define HACKLIB_HANDLES_H

#ifdef _WIN32
#include <Windows.h>
#include <d3d9.h>
#endif


namespace hl
{
#ifdef _WIN32

/// Represents a module handle. Can interpreted as pointer to the module base address.
typedef HINSTANCE ModuleHandle;
/// Represents a window handle.
typedef HWND WindowHandle;
/// Represents a graphics context.
typedef IDirect3DDevice9* GraphicsContext;

/// Represents a null value for hl::ModuleHandle.
static const ModuleHandle NullModuleHandle = NULL;

#else

/// Represents a module handle. Analogous to Windows this represents the module base address, not a dlopen handle!
typedef void* ModuleHandle;
/// Represents a window handle. Must be compatible with X11 Window type.
typedef long unsigned int WindowHandle;
/// Represent a graphics context.
typedef void* GraphicsContext;

/// Represents a null value for hl::ModuleHandle.
static const ModuleHandle NullModuleHandle = nullptr;

#endif
}

#endif
