#include "SpiderTwerk.hpp"
#include "utility/CommandLineArgs.hpp"
#include "sdk/cResource.hpp"
#include <D3dx9tex.h>

#define SPIDERTWERK_RANDOM_TWERK 0

static constexpr int       SPIDERTWERK_NUM_FRAMES    = 75;
static constexpr char*     SPIDERTWERK_FORMAT_STRING = "spidertwerk\\spidertwerk_%05d.DDS";
static constexpr uintptr_t RENDER_TARGET_CONS_ADDR   = 0x0A69280;

#if SPIDERTWERK_RANDOM_TWERK

thread_local uint32_t guard_seed = 1;

static inline uint32_t xorshift_bad(void) {
    uint32_t x = guard_seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    guard_seed = x;
    return x;
}

static inline void seed_guard_rng(uint32_t seed) {
    guard_seed = seed ? seed : 1;
}

static inline uint32_t biased_frame(void) {
    uint32_t a = xorshift_bad();
    uint32_t b = xorshift_bad();
    uint32_t m = (a > b) ? a : b;          // biases toward max
    return (uint32_t)((m * SPIDERTWERK_NUM_FRAMES) >> 32);
}
#endif


class rRenderTargetTexture : public cResource
{
public:
	char pad_0060[108]; //0x0060
	class cTransTexture *cTransTexture; //0x00CC
	char pad_00D0[8]; //0x00D0
	uint32_t width; //0x00D8
	uint32_t height; //0x00DC
	char pad_00E0[184]; //0x00E0
}; //Size: 0x0198
static_assert(sizeof(rRenderTargetTexture) == 0x198);

class cTransTexture
{
public:
	char pad_0000[16]; //0x0000
	PDIRECT3DTEXTURE9 mD3D9Texture; //0x0010
}; //Size: 0x0014
static_assert(sizeof(cTransTexture) == 0x14);

static std::unique_ptr<FunctionHook> g_render_target_cons_hook;
static rRenderTargetTexture* g_rt_agnus = nullptr;

static std::vector<rRenderTargetTexture*> g_render_targets = {};

rRenderTargetTexture* __fastcall rRenderTarget_cons(rRenderTargetTexture* pthis) {
    auto res = g_render_target_cons_hook->get_original<decltype(rRenderTarget_cons)>()(pthis);
    if (res) {
        g_render_targets.emplace_back(res);
    }
    return res;
}

std::vector<PDIRECT3DTEXTURE9> g_dds_seq = {};
static bool g_dds_seq_loaded = false;

void load_dds_seq() {
    char filename[MAX_PATH] = {};
    auto device = g_framework->get_d3d9_device();
    for (int i = 1; i < SPIDERTWERK_NUM_FRAMES; i++) {
        sprintf(filename, SPIDERTWERK_FORMAT_STRING, i);
        DWORD attribs = GetFileAttributes(filename);
        if ((attribs == INVALID_FILE_ATTRIBUTES) || (attribs & FILE_ATTRIBUTE_DIRECTORY)) {
            return;
        }
        PDIRECT3DTEXTURE9 tex;
        HRESULT hr = D3DXCreateTextureFromFile(device, filename, &tex);
        if (FAILED(hr)) {
            return;
        }
        g_dds_seq.push_back(tex);
    }
    g_dds_seq_loaded = true;
}

void SpiderTwerk::on_frame(fmilliseconds& dt) {
    if (!g_rt_agnus) {
        // need to find agnus rt on next frame cause this path is loaded after rRenderTarget constructor
        for (auto rt : g_render_targets) {
            printf("%s\n", rt->path);
            if (strcmp(rt->path, "scr\\st215\\stage\\XfEffect2D") == 0) {
                g_rt_agnus = rt;
                break;
            }
        }
        if (g_rt_agnus) {
            g_dds_seq.reserve(SPIDERTWERK_NUM_FRAMES);
            load_dds_seq();
            g_render_targets.clear();
#if SPIDERTWERK_RANDOM_TWERK
            seed_guard_rng((uint32_t)g_rt_agnus);
#else
            g_rt_agnus->width  = 256;
            g_rt_agnus->height = 256;
#endif
            return;
        }
        return;
    }
    if (!g_dds_seq_loaded) {
        return;
    }

    
    static fmilliseconds time_accumulator = fmilliseconds{0.0f};
#if SPIDERTWERK_RANDOM_TWERK
    static constexpr fmilliseconds video_timer = fmilliseconds{ 1000.0f / 3.0f };
#else
    static constexpr fmilliseconds video_timer = fmilliseconds{ 1000.0f / 30.0f };
#endif
    static uint32_t frame_idx = 0;
    time_accumulator += dt;
    if (time_accumulator <= video_timer) {
        return;
    }
    time_accumulator = fmilliseconds{0.0f};
    IDirect3DTexture9* twerk_frame = g_dds_seq.at(frame_idx++ % (SPIDERTWERK_NUM_FRAMES - 1));
    IDirect3DTexture9** pTextureLoc = &g_rt_agnus->cTransTexture->mD3D9Texture;

    if ((!g_rt_agnus->cTransTexture) || (!g_rt_agnus->cTransTexture->mD3D9Texture)) {
#if 0
        g_dds_seq.clear();
        g_dds_seq_loaded = false;
        g_render_targets.clear();
#endif
        return;
    }
    #if SPIDERTWERK_RANDOM_TWERK

    int rndint = biased_frame() % (SPIDERTWERK_NUM_FRAMES - 1);
    do {
        rndint = biased_frame() % (SPIDERTWERK_NUM_FRAMES - 1);
    } while( rndint >= 37 && rndint <= 45 );
    
    twerk_frame = g_dds_seq.at(rndint);

    static IDirect3DTexture9* original_texture = nullptr;

    static bool once = false;
    if (!once) {
        original_texture = g_rt_agnus->cTransTexture->mD3D9Texture;
        once = true;
    }

    if ((xorshift_bad() & 7) != 0) {
        g_rt_agnus->width  = 128;
        g_rt_agnus->height = 128;
        while(InterlockedCompareExchange((LONG*)(pTextureLoc), (LONG)original_texture, (LONG)(g_rt_agnus->cTransTexture->mD3D9Texture)) != (LONG)g_rt_agnus->cTransTexture->mD3D9Texture);
        return;
    }

    g_rt_agnus->width  = 256;
    g_rt_agnus->height = 256;

    #endif // SPIDERTWERK_RANDOM_TWERK

    while(InterlockedCompareExchange((LONG*)(pTextureLoc), (LONG)twerk_frame, (LONG)(g_rt_agnus->cTransTexture->mD3D9Texture)) != (LONG)g_rt_agnus->cTransTexture->mD3D9Texture);
}

#if 0
void SpiderTwerk::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("SpiderTwerk"), &mod_enabled)) {
            toggle1(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("SpiderTwerkHelpMarker"));
    }
}
#endif
void SpiderTwerk::on_stage_start() {
    g_render_targets.clear();
}

void SpiderTwerk::on_reset() {
    if (!g_dds_seq_loaded) {
        return;
    }
    for (auto tex : g_dds_seq) {
        tex->Release();
    }
    load_dds_seq();
}

// void SpiderTwerk::on_game_pause(bool toggle) {}
// bool SpiderTwerk::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> SpiderTwerk::on_initialize() {
    if (!utility::check_argument("-spidertwerk")) {
        return Mod::on_initialize();
    }
    g_render_target_cons_hook = std::make_unique<FunctionHook>(RENDER_TARGET_CONS_ADDR, &rRenderTarget_cons);
    if(!g_render_target_cons_hook->create()) {
        return "Failed to initialize SpiderTwerk";
    }
    return Mod::on_initialize();
}

#if 0
void SpiderTwerk::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("SpiderTwerk").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void SpiderTwerk::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("SpiderTwerk", mod_enabled);
}
#endif
