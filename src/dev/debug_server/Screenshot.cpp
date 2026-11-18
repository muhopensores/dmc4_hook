#include "Tools.hpp"
#include "Server.hpp"
#include "../../ModFramework.hpp"
#include "../../sdk/FrameHooks.hpp"
#include <d3d9.h>
#include <D3dx9tex.h>
#include <windows.h>
#include <algorithm>
#include <chrono>
#include <deque>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>

// S4 (plan Vibed/GameMCP/README.md): `screenshot`. The request waits for the next Present: DebugServer::on_frame runs
// in dmc4_hook's Present hook (ModFramework::on_frame) with the finished frame in the back buffer, before ImGui draws
// the hook's own windows, so shots show the game without the menu. The back buffer is copied into a render target of
// our own with StretchRect (which also scales, and resolves a multisampled buffer), then D3DX writes it as a PNG.

namespace debug_server {
namespace {

namespace fs = std::filesystem;

struct Shot {
    json args;
    std::promise<json> done;
};
std::mutex g_mutex;
std::deque<std::shared_ptr<Shot>> g_shots;
int g_count = 0;

fs::path game_folder() {
    char p[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, p, MAX_PATH);
    return fs::path(p).parent_path();
}

template <typename T> struct Released {   // releases a COM pointer at scope end
    T* p = nullptr;
    ~Released() { if (p) p->Release(); }
};

json take(IDirect3DDevice9* dev, const json& a) {
    if (!dev) throw ToolError("failed", "no D3D9 device");
    Released<IDirect3DSurface9> back;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back.p)) || !back.p)
        throw ToolError("failed", "no back buffer");
    D3DSURFACE_DESC d{};
    back.p->GetDesc(&d);

    RECT src{0, 0, (LONG)d.Width, (LONG)d.Height};
    if (a.contains("crop")) {   // [x, y, w, h] in back-buffer pixels
        const json& c = a["crop"];
        if (!c.is_array() || c.size() != 4) throw ToolError("bad_args", "crop: [x, y, w, h] in pixels");
        const LONG x = std::clamp(c[0].get<LONG>(), 0L, (LONG)d.Width - 1);
        const LONG y = std::clamp(c[1].get<LONG>(), 0L, (LONG)d.Height - 1);
        src = RECT{x, y, std::min<LONG>(x + std::max(1L, c[2].get<LONG>()), d.Width),
                   std::min<LONG>(y + std::max(1L, c[3].get<LONG>()), d.Height)};
    }
    const double scale = std::clamp(a.value("scale", 0.5), 0.05, 1.0);
    const UINT w = std::max<UINT>(1, (UINT)((src.right - src.left) * scale + 0.5));
    const UINT h = std::max<UINT>(1, (UINT)((src.bottom - src.top) * scale + 0.5));

    // a multisampled back buffer is resolved at full size first (StretchRect can't stretch out of one)
    Released<IDirect3DSurface9> full, out;
    IDirect3DSurface9* from = back.p;
    RECT from_rect = src;
    if (d.MultiSampleType != D3DMULTISAMPLE_NONE) {
        if (FAILED(dev->CreateRenderTarget(d.Width, d.Height, d.Format, D3DMULTISAMPLE_NONE, 0, FALSE, &full.p, nullptr)) ||
            FAILED(dev->StretchRect(back.p, nullptr, full.p, nullptr, D3DTEXF_NONE)))
            throw ToolError("failed", "can't resolve the multisampled back buffer");
        from = full.p;
    }
    if (FAILED(dev->CreateRenderTarget(w, h, D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &out.p, nullptr)))
        throw ToolError("failed", "can't make a " + std::to_string(w) + " x " + std::to_string(h) + " render target");
    const bool same = w == (UINT)(src.right - src.left) && h == (UINT)(src.bottom - src.top);
    if (FAILED(dev->StretchRect(from, &from_rect, out.p, nullptr, same ? D3DTEXF_POINT : D3DTEXF_LINEAR)))
        throw ToolError("failed", "StretchRect from the back buffer failed");

    fs::path path;
    if (a.contains("path") && a["path"].is_string() && !a["path"].get<std::string>().empty()) {
        path = fs::path(a["path"].get<std::string>());
        if (path.is_relative()) path = game_folder() / path;
    } else {
        char name[64];
        std::snprintf(name, sizeof(name), "shot_%llu_%d.png", (unsigned long long)frame_hooks::frames(), ++g_count);
        path = game_folder() / "dmc4_hook_screenshots" / name;
    }
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    const HRESULT hr = D3DXSaveSurfaceToFileA(path.string().c_str(), D3DXIFF_PNG, out.p, nullptr, nullptr);
    if (FAILED(hr)) {
        char b[96];
        std::snprintf(b, sizeof(b), "D3DXSaveSurfaceToFile failed (0x%08lX)", (unsigned long)hr);
        throw ToolError("failed", b);
    }
    return json{{"path", path.string()}, {"width", w}, {"height", h}, {"source", json::array({d.Width, d.Height})},
                {"crop", json::array({src.left, src.top, src.right - src.left, src.bottom - src.top})},
                {"frame", frame_hooks::frames()}};
}

json screenshot_tool(const json& a) {
    auto shot = std::make_shared<Shot>();
    shot->args = a;
    std::future<json> f = shot->done.get_future();
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_shots.push_back(shot);
    }
    if (f.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_shots.erase(std::remove(g_shots.begin(), g_shots.end(), shot), g_shots.end());
        throw ToolError("game_busy", "no frame was presented within 5 s (minimised, or loading)");
    }
    return f.get();
}

} // namespace

void screenshot_present() {
    std::deque<std::shared_ptr<Shot>> shots;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_shots.empty()) return;
        shots.swap(g_shots);
    }
    IDirect3DDevice9* dev = g_framework ? g_framework->get_d3d9_device() : nullptr;
    for (auto& s : shots) {
        try {
            s->done.set_value(take(dev, s->args));
        } catch (...) {
            s->done.set_exception(std::current_exception());
        }
    }
}

void register_screenshot() {
    json crop = json{{"type", "array"}, {"description", "[x, y, w, h] in back-buffer pixels (default: all)"}};
    add_tool({"screenshot",
              "Save the next presented frame as a PNG (the game's image, without dmc4_hook's own windows) and return its "
              "path, to view it: dmc4_hook_screenshots\\shot_<frame>_<n>.png in the game folder unless `path` is "
              "given. scale (default 0.5, 0.05-1) shrinks it, crop takes a part (back-buffer pixels).",
              json{{"type", "object"},
                   {"properties", {{"scale", {{"type", "number"}, {"description", "0.05-1, default 0.5"}}},
                                   {"crop", crop},
                                   {"path", {{"type", "string"}, {"description", "a .png path; relative = to the game folder"}}}}}},
              false, screenshot_tool});
}

} // namespace debug_server
