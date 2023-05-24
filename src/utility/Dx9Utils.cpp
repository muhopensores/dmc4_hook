#include "Dx9Utils.hpp"
#include "ModFramework.hpp"

#pragma comment(lib, "D3dx9") // TODO(): fix cmakelists.txt

bool utility::dx9::load_texture_from_file(LPCVOID data, UINT size, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
    auto device = g_framework->get_d3d9_device();
    if (!device) { 
        spdlog::error("LoadTextureFromFile failed due to null d3d9 device");
        return false; 
    }
    // Load texture from memory
    PDIRECT3DTEXTURE9 texture;
    HRESULT hr = D3DXCreateTextureFromFileInMemory(device, data, size, &texture);
    if (hr != S_OK) {
        spdlog::error("LoadTextureFromFile failed to CreateTextureFromFileInMemory");
        return false;
    }
    // Retrieve description of the texture surface so we can access its size
    D3DSURFACE_DESC my_image_desc;
    texture->GetLevelDesc(0, &my_image_desc);
    *out_texture = texture;
    *out_width = (int)my_image_desc.Width;
    *out_height = (int)my_image_desc.Height;
    return true;
}
