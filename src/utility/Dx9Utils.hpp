#pragma once
#include <D3dx9tex.h>
namespace utility {
	namespace dx9 {
		bool load_texture_from_file(LPCVOID data, UINT size, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height);
	}
}