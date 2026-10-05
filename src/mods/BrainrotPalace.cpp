#include "BrainrotPalace.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/cResource.hpp"
#include <D3dx9tex.h>
#include <filesystem>
#include "GuiFunctions.hpp"

constexpr uintptr_t rModelDTI = 0x00EADF48;

static bool mod_enabled = false;
static bool g_rotate_cef_screen = false;
static bool g_should_draw_cef_window = false;
static bool g_cef_initialized = false;
static bool g_cef_shutdown_pending = false;
static bool g_files_present = true;

static HMODULE g_libcef_plugin = NULL;

static constexpr auto CEF_URL_BUFFSIZE = 256;
static char g_cef_url[CEF_URL_BUFFSIZE] = "https://www.youtube.com/watch?v=rsxJf9WXM5Q&t=176s";

class rTexture_ {
public:
    char pad_0000[204];                    // 0x0000
    class cTransTexture* cTransTexturePtr; // 0x00CC
    char pad_00D0[1908];                   // 0x00D0
}; // Size: 0x0844
static_assert(sizeof(rTexture_) == 0x844);

class cMaterialStandard_ {
public:
    char pad_0000[28];            // 0x0000
    class rTexture_* rTexturePtr; // 0x001C
    char pad_0020[2084];          // 0x0020
}; // Size: 0x0844
static_assert(sizeof(cMaterialStandard_) == 0x844);

class cMaterialPtr_ {
public:
    class cMaterialStandard_* cMaterial; // 0x0000
    char pad_0004[2112];                 // 0x0004
}; // Size: 0x0844
static_assert(sizeof(cMaterialPtr_) == 0x844);

class uDevil4Model_ {
public:
    char pad_0000[2608];               // 0x0000
    class rModel* rModelPtr;           // 0x0A30
    char pad_0A34[68];                 // 0x0A34
    class cMaterialPtr_* cMaterialPtr; // 0x0A78
    char pad_0A7C[1476];               // 0x0A7C
}; // Size: 0x1040
static_assert(sizeof(uDevil4Model_) == 0x1040);

struct uCefScreenModel {
    // uActor actor;
    uDevil4Model model;
    MtObject* res_ptr; // rModel
    uCefScreenModel(MtObject* resource) {
        res_ptr = resource;
        // uactor_sdk::uActorCons(&this->actor);
        uactor_sdk::uDevil4ModelCons(&this->model);

        uactor_sdk::get_model(&this->model, (void*)resource);

        // this->actor.mCameraTransparencyEnable = false;
        // this->actor.Render.mVFCullLevel       = 0;
        // this->actor.Render.mRenderMode        = 0;
        this->model.Render.mVFCullLevel = 0;
        this->model.Render.mRenderMode  = 0;
        // this->actor.Render.mZPrepassDist      = 2000;
        // this->actor.Render.mPriorityBias      = -100;
        // this->actor.mActorType                = 5;
        // this->actor.mWorkRate.mType           = 0x9;
    }
};

static uCefScreenModel* g_cef_screen_mod = nullptr;

static void swap_game_texture_to_cef(uCefScreenModel* cefmodel, IDirect3DTexture9* texture);

static IDirect3DTexture9* g_cef_texture = nullptr;

#pragma region libcef_crap

static constexpr auto CEF_TEXTURE_WIDTH  = 1280;
static constexpr auto CEF_TEXTURE_HEIGHT = 720;

static constexpr auto CEF_TEXTURE_MIN_WIDTH = 320;
static constexpr auto CEF_TEXTURE_MIN_HEIGHT = 180;

static glm::i32vec2 CEF_TEXTURE_DIMS = {
    CEF_TEXTURE_WIDTH, CEF_TEXTURE_HEIGHT
};

#define CEF_FRAME_CALLBACK(name) void CALLBACK name(const void* buffer, int width, int height)

#define CEF_INIT(name) BOOL name(const char* cef_binaries_path, const char* subprocess_name)
#define CEF_CREATE_BROWSER(name) BOOL name(const char* url, int width, int height)
#define CEF_SET_SIZE(name) void name(int width, int height)
#define CEF_NAVIGATE(name) void name(const char* url)
#define CEF_PUMP(name) void name(void)
#define CEF_SHUTDOWN(name) void name(void)

#define CEF_MOUSE_MOVE(name) void name(int x, int y, int modifiers, BOOL mouse_leave)
#define CEF_MOUSE_CLICK(name) void name(int x, int y, int button, BOOL mouse_up, int click_count, int modifiers)
#define CEF_MOUSE_WHEEL(name) void name(int x, int y, int delta_x, int delta_y, int modifiers)
#define CEF_KEY_EVENT(name) void name(BOOL key_down, int windows_key_code, int modifiers)
#define CEF_CHAR_EVENT(name) void name(wchar_t character, int modifiers)
#define CEF_HANDLE_WNDPROC(name) BOOL name(HWND handle, UINT msg, WPARAM wparam, LPARAM lparam)

#define CEF_REGISTER_CALLBACK(name) void name(void(CALLBACK * cb)(const void*, int, int))

typedef CEF_INIT(cef_init_p);
typedef CEF_CREATE_BROWSER(cef_create_browser_p);
typedef CEF_NAVIGATE(cef_navigate_p);
typedef CEF_SET_SIZE(cef_set_size_p);
typedef CEF_PUMP(cef_pump_p);
typedef CEF_SHUTDOWN(cef_shutdown_p);
typedef CEF_MOUSE_MOVE(cef_mouse_move_p);
typedef CEF_MOUSE_CLICK(cef_mouse_click_p);
typedef CEF_MOUSE_WHEEL(cef_mouse_wheel_p);
typedef CEF_KEY_EVENT(cef_key_event_p);
typedef CEF_CHAR_EVENT(cef_char_event_p);
typedef CEF_HANDLE_WNDPROC(cef_handle_wndproc_p);
typedef CEF_REGISTER_CALLBACK(cef_register_cb_p);

CEF_INIT(cef_init_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

    return FALSE;
}
CEF_CREATE_BROWSER(cef_create_browser_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

    return FALSE;
}
CEF_NAVIGATE(cef_navigate_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG
}
CEF_SET_SIZE(cef_set_size_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG
}
CEF_PUMP(cef_pump_stub) {
    // dont want to clog up logs with these;
}
CEF_SHUTDOWN(cef_shutdown_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_MOUSE_MOVE(cef_mouse_move_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_MOUSE_CLICK(cef_mouse_click_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_MOUSE_WHEEL(cef_mouse_wheel_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_KEY_EVENT(cef_key_event_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_CHAR_EVENT(cef_char_event_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

}
CEF_HANDLE_WNDPROC(cef_handle_wndproc_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG

    return TRUE;
}
CEF_REGISTER_CALLBACK(cef_register_frame_ready_callback_stub) {
#ifndef NDEBUG
    spdlog::info("cef_stub " __FUNCSIG__);
#endif // !NDEBUG
}

static cef_init_p* cef_init                                 = cef_init_stub;
static cef_create_browser_p* cef_create_browser             = cef_create_browser_stub;
static cef_navigate_p* cef_navigate                         = cef_navigate_stub;
static cef_set_size_p* cef_set_size                         = cef_set_size_stub;
static cef_pump_p* cef_pump_messages                        = cef_pump_stub;
static cef_shutdown_p* cef_shutdown_all                     = cef_shutdown_stub;
static cef_mouse_move_p* cef_send_mouse_move_event          = cef_mouse_move_stub;
static cef_mouse_click_p* cef_send_mouse_click_event        = cef_mouse_click_stub;
static cef_mouse_wheel_p* cef_send_mouse_wheel_event        = cef_mouse_wheel_stub;
static cef_key_event_p* cef_send_key_event                  = cef_key_event_stub;
static cef_char_event_p* cef_send_char_event                = cef_char_event_stub;
static cef_handle_wndproc_p* cef_handle_windows_message     = cef_handle_wndproc_stub;
static cef_register_cb_p* cef_register_frame_ready_callback = cef_register_frame_ready_callback_stub;

static HMODULE plugin_libcef_resolve_fptrs() {

    HMODULE h = LoadLibrary("cef\\plugin_libcef.dll");
    if (h == NULL) {
        return h;
    }

#define LOAD(lib, ptr_type, name) ((ptr_type*)GetProcAddress((lib), #name))

    cef_init                          = LOAD(h, cef_init_p, cef_init);
    cef_create_browser                = LOAD(h, cef_create_browser_p, cef_create_browser);
    cef_navigate                      = LOAD(h, cef_navigate_p, cef_navigate);
    cef_set_size                      = LOAD(h, cef_set_size_p, cef_set_size);
    cef_pump_messages                 = LOAD(h, cef_pump_p, cef_pump_messages);
    cef_shutdown_all                  = LOAD(h, cef_shutdown_p, cef_shutdown_all);
    cef_send_mouse_move_event         = LOAD(h, cef_mouse_move_p, cef_send_mouse_move_event);
    cef_send_mouse_click_event        = LOAD(h, cef_mouse_click_p, cef_send_mouse_click_event);
    cef_send_mouse_wheel_event        = LOAD(h, cef_mouse_wheel_p, cef_send_mouse_wheel_event);
    cef_send_key_event                = LOAD(h, cef_key_event_p, cef_send_key_event);
    cef_send_char_event               = LOAD(h, cef_char_event_p, cef_send_char_event);
    cef_handle_windows_message        = LOAD(h, cef_handle_wndproc_p, cef_handle_windows_message);
    cef_register_frame_ready_callback = LOAD(h, cef_register_cb_p, cef_register_frame_ready_callback);
#undef LOAD

    if(cef_init == NULL)                          { FreeLibrary(h); return NULL; }
    if(cef_create_browser == NULL)                { FreeLibrary(h); return NULL; }
    if(cef_navigate == NULL)                      { FreeLibrary(h); return NULL; }
    if(cef_set_size == NULL)                      { FreeLibrary(h); return NULL; }
    if(cef_pump_messages == NULL)                 { FreeLibrary(h); return NULL; }
    if(cef_shutdown_all == NULL)                  { FreeLibrary(h); return NULL; }
    if(cef_send_mouse_move_event == NULL)         { FreeLibrary(h); return NULL; }
    if(cef_send_mouse_click_event == NULL)        { FreeLibrary(h); return NULL; }
    if(cef_send_mouse_wheel_event == NULL)        { FreeLibrary(h); return NULL; }
    if(cef_send_key_event == NULL)                { FreeLibrary(h); return NULL; }
    if(cef_send_char_event == NULL)               { FreeLibrary(h); return NULL; }
    if(cef_handle_windows_message == NULL)        { FreeLibrary(h); return NULL; }
    if(cef_register_frame_ready_callback == NULL) { FreeLibrary(h); return NULL; }
    return h;
}

static void plugin_libcef_unload(HMODULE h) {
    g_cef_initialized = false;

    cef_init                          = cef_init_stub;
    cef_create_browser                = cef_create_browser_stub;
    cef_pump_messages                 = cef_pump_stub;
    cef_navigate                      = cef_navigate_stub;
    cef_shutdown_all                  = cef_shutdown_stub;
    cef_send_mouse_move_event         = cef_mouse_move_stub;
    cef_send_mouse_click_event        = cef_mouse_click_stub;
    cef_send_mouse_wheel_event        = cef_mouse_wheel_stub;
    cef_send_key_event                = cef_key_event_stub;
    cef_send_char_event               = cef_char_event_stub;
    cef_handle_windows_message        = cef_handle_wndproc_stub;
    cef_register_frame_ready_callback = cef_register_frame_ready_callback_stub;

    FreeLibrary(h);
    g_libcef_plugin = NULL;
}

static bool fill_d3d_texture(IDirect3DTexture9* pTexture, const void* bgraPixelData, int width, int height) {
    if (!pTexture || !bgraPixelData || width <= 0 || height <= 0) {
        return false;
    }

    D3DSURFACE_DESC desc;
    if (FAILED(pTexture->GetLevelDesc(0, &desc))) {
        return false;
    }

    const int copyWidth  = std::min(width,  static_cast<int>(desc.Width));
    const int copyHeight = std::min(height, static_cast<int>(desc.Height));

    D3DLOCKED_RECT lockedRect;
    HRESULT hr = pTexture->LockRect(0, &lockedRect, nullptr, 0);
    if (FAILED(hr)) {
        return false;
    }

    const std::size_t srcRowBytes  = static_cast<std::size_t>(width) * 4;
    const std::size_t copyRowBytes = static_cast<std::size_t>(copyWidth) * 4;

    uint8_t* pDest      = static_cast<uint8_t*>(lockedRect.pBits);
    const uint8_t* pSrc = static_cast<const uint8_t*>(bgraPixelData);

    if (copyWidth == width && copyHeight == height && lockedRect.Pitch == static_cast<LONG>(copyRowBytes)) {
        memcpy(pDest, pSrc, copyRowBytes * static_cast<std::size_t>(copyHeight));
    }
    else {
        for (int y = 0; y < copyHeight; ++y) {
            memcpy(pDest, pSrc, copyRowBytes);
            pDest += lockedRect.Pitch;
            pSrc  += srcRowBytes;
        }
    }

    hr = pTexture->UnlockRect(0);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

CEF_FRAME_CALLBACK(on_cef_frame) {
    //static int g_frame_count = 0;
    if (!g_cef_texture) return;
#if 0
        const unsigned char* p = (const unsigned char*)buffer;
        printf("[frame #%d] %dx%d  first pixel: R=%u G=%u B=%u A=%u\n",
               g_frame_count, width, height,
              p[2], p[1], p[0], p[3]);
#else
    // buffer = BGRA pixels, width * height * 4 bytes
    fill_d3d_texture(g_cef_texture, buffer, width, height);
#endif
}

static bool create_d3d_bgra_texture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppOutTexture, UINT width, UINT height) {
    if (!pDevice || !ppOutTexture) {
        return false;
    }

    IDirect3DTexture9* pTexture = nullptr;

    HRESULT hr = D3DXCreateTexture(pDevice, width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pTexture);

    if (FAILED(hr)) {
        return false;
    }

    *ppOutTexture = pTexture;
    return true;
}

#pragma endregion libcef_crap

class cTransTexture {
public:
    char pad_0000[16];              // 0x0000
    PDIRECT3DTEXTURE9 mD3D9Texture; // 0x0010
}; // Size: 0x0014
static_assert(sizeof(cTransTexture) == 0x14);

struct alignas(16) cMaterial {
    uintptr_t vtable;
    // uint32_t uknFlag1;
    uint32_t type : 8;
    uint32_t alphaRef : 8; // 16
    uint32_t blend : 7;    // 23
    uint32_t ukn : 4;      // 27
    uint32_t zwrite : 1;   // 28
    uint32_t zthrough : 1; // 29
    uint32_t solidpriority : 2;
    uint32_t uknFlag2;
    uint32_t TechniqueNum;
    uint32_t PipelineNum;
    uint32_t blend_state;
    uint32_t draw_state;
    cTransTexture* BaseMap;
    float Transparency;
    int padding[3];
    Vector4f BaseMapFactor;
    Vector3f DiffuseFactor;
    float SpecularFactor;
    char VectorGroups[4];
};
// static_assert(offsetof(cMaterial, BaseMapFactor) == 0x24);
static_assert(sizeof(cMaterial) == 0x60);

static void try_libcef() {
    static bool once_flag = false;

    if (g_cef_initialized || once_flag) {
        return;
    }

    g_libcef_plugin = plugin_libcef_resolve_fptrs();
    if (g_libcef_plugin == NULL) {
        spdlog::error("FAIL: could not load libcef plugin\n");
        plugin_libcef_unload(g_libcef_plugin);
        return;
    }
    once_flag = true;
    if (!cef_init(".\\cef", "cef_subprocess.exe")) {
        spdlog::error("FAIL: cef_init\n");
        plugin_libcef_unload(g_libcef_plugin);
        return;
    }
    spdlog::info("OK: cef_init\n");

    // Width/height are viewport dimensions in pixels.
    if (!cef_create_browser(g_cef_url,
            CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y)) {
        spdlog::error("FAIL: cef_create_browser\n");
        cef_shutdown_all();
        plugin_libcef_unload(g_libcef_plugin);
        return;
    }
    spdlog::info("OK: cef_create_browser ({}x{})\n", CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y);
    

    if (!create_d3d_bgra_texture(g_framework->get_d3d9_device(), &g_cef_texture, CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y)) {
        spdlog::error("FAIL: create_d3d_bgra_texture for cef failed!\n");
        cef_shutdown_all();
        plugin_libcef_unload(g_libcef_plugin);
        return;
    }
    spdlog::info("OK: (cef)create_d3d_bgra_texture (texture={}, dims={}x{})\n", (void*)g_cef_texture, CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y);
    cef_register_frame_ready_callback(on_cef_frame);
    spdlog::info("OK: (cef)create_d3d_bgra_texture ({}x{})\n", CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y);

    g_cef_initialized = true;
}

static glm::i16vec2 g_relative_mouse_delta = { 0, 0 };
static bool g_cef_mouse_inside = false;

static void imgui_draw_cef_window(bool* imgui_open) {

    if (!*imgui_open) {
        if (g_cef_mouse_inside) {
            g_cef_mouse_inside = false;
            cef_send_mouse_move_event(0, 0, 0, TRUE);
        }
        return;
    }

    ImGuiIO& io          = ImGui::GetIO();
    const ImVec2 win_siz = ImVec2(io.DisplaySize.x * 0.8f, io.DisplaySize.y * 0.8f + (ImGui::GetTextLineHeightWithSpacing() * 4.0f));
    const ImVec2 win_pos = ImVec2((io.DisplaySize.x - win_siz.x) * 0.5f, (io.DisplaySize.y - win_siz.y) * 0.5f);
    ImGui::SetNextWindowPos(win_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(win_siz, ImGuiCond_Always);

    ImGui::Begin("cefclient", imgui_open);

    const float input_width = ImGui::CalcItemWidth();
    ImGui::SetCursorPosX((win_siz.x - input_width) / 2.f);
    ImGui::InputText("URL: ", g_cef_url, CEF_URL_BUFFSIZE);
    ImGui::SameLine();
    if (ImGui::Button("go")) {
        cef_navigate(g_cef_url);
    }
    ImGui::SameLine();

    Mod::help_marker(_("You can press 'Save Config' in main window to save this url to load on startup\n"));
    ImVec2 image_pos  = ImGui::GetCursorScreenPos();
    ImVec2 area       = ImGui::GetContentRegionAvail();
    ImVec2 image_size = ImVec2(area.x, glm::ceil(area.x * ((float)CEF_TEXTURE_DIMS.y / (float)CEF_TEXTURE_DIMS.x)));

    int texture_width  = CEF_TEXTURE_DIMS.x;
    int texture_height = CEF_TEXTURE_DIMS.y;
    if (g_cef_initialized && g_cef_texture) {
        ImGui::Image((ImTextureID)(intptr_t)g_cef_texture, image_size);
    }

    if (ImGui::IsItemHovered()) {
        g_cef_mouse_inside = true;
        ImVec2 mouse_pos       = ImGui::GetMousePos();
        ImVec2 local_mouse_pos = ImVec2(mouse_pos.x - image_pos.x, mouse_pos.y - image_pos.y);
        ImVec2 uv_coords       = ImVec2(local_mouse_pos.x / image_size.x, local_mouse_pos.y / image_size.y);

        int pixel_x = (int)(uv_coords.x * texture_width);
        int pixel_y = (int)(uv_coords.y * texture_height);

        pixel_x = glm::clamp(pixel_x, 0, texture_width - 1);
        pixel_y = glm::clamp(pixel_y, 0, texture_height - 1);

#ifndef NDEBUG
        ImGui::BeginTooltip();
        ImGui::Text("Local UV: (%.2f, %.2f)", uv_coords.x, uv_coords.y);
        ImGui::Text("Pixel Coordinate: (%d, %d)", pixel_x, pixel_y);
#endif // !NDEBUG
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            cef_send_mouse_click_event(pixel_x, pixel_y, 0, FALSE, 1, 0);
            cef_send_mouse_click_event(pixel_x, pixel_y, 0, TRUE, 1, 0);
        }
#ifndef NDEBUG
        ImGui::EndTooltip();
#endif // !NDEBUG

        g_relative_mouse_delta.y = pixel_y;
        g_relative_mouse_delta.x = pixel_x;
    }
    else {
        if (g_cef_mouse_inside) {
            g_cef_mouse_inside = false;
            cef_send_mouse_move_event(0, 0, 0, TRUE);
        }
        g_relative_mouse_delta.y = 0;
        g_relative_mouse_delta.x = 0;
    }
    ImGui::End();
}

static void release_our_cef_texture_ref(IDirect3DTexture9** texture) {
    if (*texture) {
        ULONG refcount = (*texture)->Release();
        spdlog::info("BrainrotPalace::release_our_cef_texture_ref refcount: {}", refcount);
        *texture = nullptr;
    }
}

static void on_cef_resize(glm::i32vec2& dims, IDirect3DTexture9** texture) {
    
    if (!g_cef_initialized) { return; }

    cef_set_size(dims.x, dims.y);

    release_our_cef_texture_ref(texture);
    //ULONG refcount = texture->Release(); // games ref
    //spdlog::info("g_cef_texture refcount0: {}",refcount);
    //assert(refcount == 1);
    //refcount = g_cef_texture->Release(); // our ref
    //spdlog::info("g_cef_texture refcount1: {}",refcount);
    //assert(refcount == 0);
    //g_cef_texture = nullptr;
    if (create_d3d_bgra_texture(g_framework->get_d3d9_device(), texture, dims.x, dims.y)) {
        swap_game_texture_to_cef(g_cef_screen_mod, *texture);
    } else {
        spdlog::error("FAIL: cef failed to recreate texture after resize in BrainrotPalace::on_cef_resize");
        return;
    }
}

void BrainrotPalace::on_gui_frame(int display) {
    if (!ImGui::CollapsingHeader(_("Brainrot Palace"))) {
        return;
    }
    if (!g_files_present) {
        ImGui::Text(_("Download BrainrotPalace plugin at:"));
        static gui::ImGuiURL link { 
            "github.com/muhopensores/dmc4_hook_cef_plugin/BrainrotPalacePlugin.zip",
            "https://github.com/muhopensores/dmc4_hook_cef_plugin/releases/download/1.0/BrainrotPalacePlugin.zip" 
        };
        link.draw();
        return;
    }
    //ImGui::Text("cam angle %f", cam_angle);
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("BrainrotPalace"), &mod_enabled)) {
            if (mod_enabled) {
                try_libcef();
                if (g_cef_initialized && !g_cef_texture) {
                    create_d3d_bgra_texture(g_framework->get_d3d9_device(), &g_cef_texture, CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y);
                }
                on_stage_start();
                cef_navigate(g_cef_url);
            }
            if (!mod_enabled) {
                cef_navigate("about:blank");
                on_stage_end();
                release_our_cef_texture_ref(&g_cef_texture);
            }
        }
        ImGui::SameLine();
        help_marker(_("Spawns a projector screen with a chromium tab, you can browse websites and watch some videos (not all codecs might be supported)\n"));
        
        if (ImGui::InputInt2(_("CEF screen size"), (int*)&CEF_TEXTURE_DIMS)) {
            CEF_TEXTURE_DIMS.x = glm::min(glm::max(CEF_TEXTURE_DIMS.x, CEF_TEXTURE_MIN_WIDTH), 4096);
            CEF_TEXTURE_DIMS.y = glm::min(glm::max(CEF_TEXTURE_DIMS.y, CEF_TEXTURE_MIN_HEIGHT), 4096);
            on_cef_resize(CEF_TEXTURE_DIMS, &g_cef_texture);
        }
        ImGui::Checkbox(_("Control browser?"),  &g_should_draw_cef_window);
        ImGui::Checkbox(_("Try to keep cef in camera?"), &g_rotate_cef_screen);
        imgui_draw_cef_window(&g_should_draw_cef_window);
    }
}

// void BrainrotPalace::on_game_pause(bool toggle) {}
// bool BrainrotPalace::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}
std::optional<std::string> BrainrotPalace::on_initialize() {

    g_files_present &= std::filesystem::exists(".\\nativePC\\scr\\st705\\stage\\st705-spidertwerk.mod");
    g_files_present &= std::filesystem::exists(".\\cef\\plugin_libcef.dll");
    g_files_present &= std::filesystem::exists(".\\cef\\libcef.dll");
    
    return Mod::on_initialize();
}

void BrainrotPalace::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("BrainrotPalace").value_or(false);

    CEF_TEXTURE_DIMS.x = glm::min(glm::max(cfg.get<int32_t>("BrainrotPalaceCefScreenWidth").value_or(CEF_TEXTURE_WIDTH), CEF_TEXTURE_MIN_WIDTH), 4096);
    CEF_TEXTURE_DIMS.y = glm::min(glm::max(cfg.get<int32_t>("BrainrotPalaceCefScreenHeight").value_or(CEF_TEXTURE_HEIGHT), CEF_TEXTURE_MIN_HEIGHT), 4096);

    g_rotate_cef_screen = cfg.get<bool>("BrainrotPalaceCefScreenRotate").value_or(false);

    auto url = cfg.get("BrainrotPalaceCefUrl");
    if (url) {
        strncpy(g_cef_url, url->c_str(), CEF_URL_BUFFSIZE - 1);
        g_cef_url[CEF_URL_BUFFSIZE - 1] = '\0';
    }
}

void BrainrotPalace::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("BrainrotPalace", mod_enabled);

    cfg.set<int32_t>("BrainrotPalaceCefScreenWidth", CEF_TEXTURE_DIMS.x);
    cfg.set<int32_t>("BrainrotPalaceCefScreenHeight", CEF_TEXTURE_DIMS.y);

    cfg.set<bool>("BrainrotPalaceCefScreenRotate", g_rotate_cef_screen);

    cfg.set("BrainrotPalaceCefUrl", g_cef_url);
}

static void swap_game_texture_to_cef(uCefScreenModel* cefmodel, IDirect3DTexture9* texture) {
    if (!cefmodel || !texture) { // to shut up linter, i think constructors cant fail in dmc4, right?
        return;
    }
    uDevil4Model_* model = (uDevil4Model_*)&cefmodel->model;
    // null pointer HADOUKEN:
    // 
    //            -=>}())
    if (!model->cMaterialPtr) {
        return;
    }
    //                         -=>}())
    if (!model->cMaterialPtr->cMaterial) {
        return;
    }
    //                                     -=>}())
    if (!model->cMaterialPtr->cMaterial->rTexturePtr) {
        return;
    }
    //                                                     -=>}())
    if (!model->cMaterialPtr->cMaterial->rTexturePtr->cTransTexturePtr) {
        return;
    }
    //                                                                     -=>}())
    if (!model->cMaterialPtr->cMaterial->rTexturePtr->cTransTexturePtr->mD3D9Texture) {
        return;
    }
    auto* game_tex = model->cMaterialPtr->cMaterial->rTexturePtr->cTransTexturePtr->mD3D9Texture;
    ULONG game_tex_refcount = model->cMaterialPtr->cMaterial->rTexturePtr->cTransTexturePtr->mD3D9Texture->Release();
    //spdlog::info("BrainrotPalace::swap_game_texture_to_cef game_tex_refcount={}", game_tex_refcount);
    model->cMaterialPtr->cMaterial->rTexturePtr->cTransTexturePtr->mD3D9Texture = texture;
    ULONG our_tex_refcount = texture->AddRef();
    //spdlog::info("BrainrotPalace::swap_game_texture_to_cef our_tex_refcount={}", our_tex_refcount);
    spdlog::info("BrainrotPalace::swap_textures: releasing game_tex{} (game_tex_refcount {}), installing {} (our_tex_refcount {})",
    (void*)game_tex , game_tex_refcount, (void*)texture, our_tex_refcount);
}

void BrainrotPalace::on_reset() {
    return;
#if 0 // D3DPOOL_MANAGED does not require cleaning up on reset but we might want to change it later idk
    if (!mod_enabled) { return; }

    if (g_cef_texture != nullptr) {
        g_cef_texture->Release();
        g_cef_texture = nullptr;
    }
#endif
}

void BrainrotPalace::after_reset() {
    return;
#if 0 // D3DPOOL_MANAGED does not require cleaning up on reset but we might want to change it later idk
    if (!mod_enabled || !g_cef_initialized) { return; }

    if (!create_d3d_bgra_texture(g_framework->get_d3d9_device(), &g_cef_texture, CEF_TEXTURE_DIMS.x, CEF_TEXTURE_DIMS.y)) {
        spdlog::error("FAIL: cef failed to recreate texture after reset in BrainrotPalace::after_reset");
    }

    swap_game_texture_to_cef(g_cef_screen_mod, g_cef_texture);
#endif
}

static uDevil4Model* g_model = nullptr;
static MtObject* mod_resource = nullptr;
static void* cef_mem  = nullptr;

void BrainrotPalace::on_stage_start() {
    if (!mod_enabled || !g_files_present) {
        return;
    }

    if (!g_cef_screen_mod) {
        mod_resource = devil4_sdk::get_stuff_from_files((MtDTI*)rModelDTI, "scr\\st705\\stage\\st705-spidertwerk", 1);

        if (mod_resource) {
            cef_mem = devil4_sdk::mt_allocate_heap(sizeof(uCefScreenModel), 16);
            if (cef_mem) {
                g_cef_screen_mod = new (cef_mem) uCefScreenModel(mod_resource);
                devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)g_cef_screen_mod, 8);
            }
        }
    }
    swap_game_texture_to_cef(g_cef_screen_mod, g_cef_texture);
    if(g_cef_screen_mod) {
        g_model = (uDevil4Model*)&g_cef_screen_mod->model;
    }
}

void BrainrotPalace::on_stage_end() {

    if (!g_cef_screen_mod || !cef_mem) {
        return;
    }

    uactor_sdk::uDevil4ModelDest((void*)&g_cef_screen_mod->model);
    uactor_sdk::despawn(g_cef_screen_mod);
    uactor_sdk::destructor_call(g_cef_screen_mod);
    devil4_sdk::unit_deallocate((MtObject*)cef_mem);

    g_cef_screen_mod = nullptr;
    cef_mem = nullptr;
    g_model = nullptr;
    mod_resource = nullptr;
}

bool BrainrotPalace::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!mod_enabled) { return true; }
    if (message == WM_QUIT || message == WM_CLOSE || message == WM_DESTROY) {
        if (g_cef_initialized) {
            g_cef_shutdown_pending = true;
            return true;
        }
    }
    if (!g_should_draw_cef_window || !g_framework->m_draw_ui) {
        return true;
    }
    if (message == WM_MOUSEMOVE) {
        if (!g_cef_mouse_inside) { return true; }
        lParam = MAKELPARAM(g_relative_mouse_delta.x, g_relative_mouse_delta.y);
    }
    if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP ||
        message == WM_RBUTTONDOWN || message == WM_RBUTTONUP) {
        return true; // clicks delivered explicitly via cef_send_mouse_click_event
    }
    BOOL result = cef_handle_windows_message(wnd, message, wParam, lParam);
    return true;
}


void BrainrotPalace::on_frame(fmilliseconds& dt) {

    if(!mod_enabled) { return; }
    // done here to make sure we have directx device and can create texture stuff
    if(!g_cef_initialized && mod_enabled && !g_cef_shutdown_pending) {
        try_libcef();
        if (g_cef_initialized) {
            swap_game_texture_to_cef(g_cef_screen_mod, g_cef_texture);
        }
        return;
    }

    if (g_cef_shutdown_pending) {
        cef_shutdown_all();
        plugin_libcef_unload(g_libcef_plugin);
        release_our_cef_texture_ref(&g_cef_texture);
        PostQuitMessage(0); // in case cef had eaten our WM_QUIT
        g_cef_shutdown_pending = false;
        return;
    }

    if(g_cef_texture) {
        cef_pump_messages();
    }

    if (!g_rotate_cef_screen) { return; }

    uCameraCtrl* cam = devil4_sdk::get_local_camera();
    if(cam) {
        MtVector3 lookDirection = {
            cam->mTargetPos.x - cam->mCameraPos.x,
            cam->mTargetPos.y - cam->mCameraPos.y,
            cam->mTargetPos.z - cam->mCameraPos.z
        };
        float cam_angle = glm::atan(lookDirection.z, lookDirection.x);
        cam_angle = cam_angle - glm::half_pi<float>();
        glm::quat rotationQuat = glm::angleAxis(cam_angle, glm::vec3(cam->mCameraUp.x, -cam->mCameraUp.y, cam->mCameraUp.z));
        uDevil4Model* mod = (uDevil4Model*)g_model;
        if(mod) {
            // updateLmat/updateWmat should be taken care of by the game
            mod->mQuat = { rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w };
            // TODO(): not setting mpos is intentional as mod file is crafted especially for default BP stage
            // but might be nice to have for people running custom stages? or maybe link it to player pos or something
        }
    }
}
