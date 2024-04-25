// include your mod header file
#include "MutatorSelfAdvertisement.hpp"
#include "utility/Dx9Utils.hpp"
#include "utility/Compressed.hpp"
#include "misc/CompressedDrawing.cpp"

#include <cstdlib>
#include <ctime>

// shit memory access patterns but i want bools for imgui checkboxes
static bool mod_enabled{ false };
static bool dvd_mode{ false };
static float gdt{ 0.0f };

std::optional<std::string> MutatorSelfAdvertisement::on_initialize() {
    after_reset();
    MutatorRegistry::define("SelfAdvertisement")
        .description("hehe").weight(5)
        .on_init([&] {
            auto& io = ImGui::GetIO();
            ImVec2 disp_sz = io.DisplaySize;
            m_pos = { disp_sz.x / 2.0f, disp_sz.y / 2.0f };
            m_accel = { 2.0f, 2.0f };

            mod_enabled = true;
            dvd_mode = true;
        })
        .set_timer(30.0, [&] {
            mod_enabled = false;
            dvd_mode    = false;
        });

	return Mod::on_initialize();
}

static float Hash21(glm::vec2 p) {
    p = glm::fract(p * glm::vec2(123.489, 234.95));
    p += glm::dot(p, p + 34.4f);
    return glm::fract(p.x * p.y);
}

static ImColor rnd_hue(glm::vec2 p) {
    return ImColor::HSV(Hash21(p), 1.0f, 1.0, 1.0f);
}

void MutatorSelfAdvertisement::custom_imgui_window() {
    if (!mod_enabled) { return; }

    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration;
    static const ImVec2 size = ImVec2(220.0f,202.0f);
    if (dvd_mode) {
        static uint32_t frame = 0;
        if (gdt > 1.0f) { gdt = 1.0f; }
        // NOTE(): dumb idea to get rid of floating point precision errors if this runs for too long
        if ((frame % (30 * 60)) == 0) { m_accel = { 2.0f * glm::sign(m_accel.x), 2.0f * glm::sign(m_accel.y)}; }
        m_pos += m_accel * gdt;
        ImVec2 disp_size = ImGui::GetIO().DisplaySize;
        
        if (m_pos.x + size.x >= disp_size.x) {
            m_accel.x *= -1.0f;
            m_pos.x = disp_size.x - size.x;
            m_tint_color = rnd_hue(m_pos);
        }
        if (m_pos.x <= 0.0f) {
            m_accel.x *= -1.0f;
            m_pos.x = 0.0f;
            m_tint_color = rnd_hue(m_pos);
        }

        if (m_pos.y + size.y >= disp_size.y) {
            m_accel.y *= -1.0f;
            m_pos.y = disp_size.y - size.y;
            m_tint_color = rnd_hue(m_pos);
        }
        if (m_pos.y <= 0.0f) {
            m_accel.y *= -1.0f;
            m_pos.y = 0.0f;
            m_tint_color = rnd_hue(m_pos);
        }
        frame++;
    }
    //else {
    //    ImGui::Text("Achived with: ");
    //}
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(m_pos);
    ImGui::Begin("DirectX9 Texture Test", nullptr, window_flags);
    ImGui::Image((void*)m_texture_handle, ImVec2((float)m_texture_width, (float)m_texture_height),ImVec2(0.0f,0.0f), ImVec2(1.25f,1.25f), m_tint_color);
    ImGui::End();
}

void MutatorSelfAdvertisement::on_gui_frame() {
    if (ImGui::CollapsingHeader(_("Self advertisement"))) {
        ImGui::Checkbox(_("Show self advertisement?", &mod_enabled));
        ImGui::SameLine();
        help_marker(_("Thank you for supporting dmc4_hook"));
        ImGui::Checkbox(_("Dvd screensaver", &dvd_mode));
        if (!dvd_mode) {
            ImGui::DragFloat2(_("Image pos:"), (float*)&m_pos, 1.0f, 0.0f, 4096.0f, "%.1f");
            ImGui::ColorEdit4(_("Tint color:"), (float*)&m_tint_color);
        }
    }
}

void MutatorSelfAdvertisement::on_frame(fmilliseconds& dt) {
    gdt = dt.count();
}

void MutatorSelfAdvertisement::on_reset() {
    if(m_texture_handle != nullptr) {
        m_texture_handle->Release();
        m_texture_handle = nullptr;
    }

}

void MutatorSelfAdvertisement::after_reset() {
    auto [data, size] = utility::decompress_file_from_memory_with_size(drawing_compressed_data, drawing_compressed_size);

    if (!utility::dx9::load_texture_from_file(data, size, &m_texture_handle, &m_texture_width, &m_texture_height)) {
        spdlog::error("Failed to unpack and load compressed texture");
        mod_enabled = false;
    }
    free(data);
}

