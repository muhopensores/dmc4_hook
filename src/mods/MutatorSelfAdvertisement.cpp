// include your mod header file
#include "MutatorSelfAdvertisement.hpp"
#include "utility/Dx9Utils.hpp"
#include "utility/Compressed.hpp"
#include "misc/CompressedDrawing.cpp"

#include <cstdlib>
#include <ctime>

// shit memory access patterns but i want bools for imgui checkboxes
static bool mod_enabled = false;
static bool dvd_mode = false;
static float gdt = 0.0f;

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

ImColor MutatorSelfAdvertisement::rnd_hue(const glm::vec2& p) {  
   return ImColor::HSV(Hash21(p), 1.0f, 1.0f, m_tint_color.Value.w);  
}

void MutatorSelfAdvertisement::on_gui_frame(int display) {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Show Self Advertisement"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Thank you for supporting dmc4_hook"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::Checkbox(_("Dvd screensaver"), &dvd_mode);
        ImGui::Text(_("Position"));
        ImGui::DragFloat2("##PositionDragFloat2", (float*)&m_pos, 1.0f, 0.0f, 4096.0f, "%.1f");
        ImGui::SameLine();
        if (ImGui::Button(_("Reset##ResetPos"))) {
            m_pos = { 1700.0f, 890.0f };
        }
        ImGui::Text(_("Colour"));
        ImGui::ColorEdit4("##ColourColorEdit4", (float*)&m_tint_color);
        ImGui::SameLine();
        if (ImGui::Button(_("Reset##ResetColour"))) {
            m_tint_color = { 1.0f, 1.0f, 1.0f, 0.5f };
        }
        ImGui::PopItemWidth();
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
}

void MutatorSelfAdvertisement::on_frame(fmilliseconds& dt) {
    gdt = dt.count();

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
    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(m_pos);
    ImGui::Begin("DirectX9 Texture Test", nullptr, window_flags);
    ImGui::ImageWithBg((ImTextureID)m_texture_handle, ImVec2((float)m_texture_width, (float)m_texture_height),ImVec2(0.0f,0.0f), ImVec2(1.25f,1.25f), m_bg_color, m_tint_color);
    ImGui::End();
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

void MutatorSelfAdvertisement::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("self_advertisement").value_or(false);
    dvd_mode             = cfg.get<bool>("self_advertisement_dvd_mode").value_or(false);
    m_pos.x     = cfg.get<float>("self_advertisement_pos_x").value_or(1700.0f);
    m_pos.y     = cfg.get<float>("self_advertisement_pos_y").value_or(890.0f);
    m_tint_color.Value.x = cfg.get<float>("self_advertisement_tint_color_x").value_or(1.0f);
    m_tint_color.Value.y = cfg.get<float>("self_advertisement_tint_color_y").value_or(1.0f);
    m_tint_color.Value.z = cfg.get<float>("self_advertisement_tint_color_z").value_or(1.0f);
    m_tint_color.Value.w = cfg.get<float>("self_advertisement_tint_color_w").value_or(0.5f);
};

void MutatorSelfAdvertisement::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("self_advertisement", mod_enabled);
    cfg.set<bool>("self_advertisement_dvd_mode", dvd_mode);
    cfg.set<float>("self_advertisement_pos_x", m_pos.x);
    cfg.set<float>("self_advertisement_pos_y", m_pos.y);
    cfg.set<float>("self_advertisement_tint_color_x", m_tint_color.Value.x);
    cfg.set<float>("self_advertisement_tint_color_y", m_tint_color.Value.y);
    cfg.set<float>("self_advertisement_tint_color_z", m_tint_color.Value.z);
    cfg.set<float>("self_advertisement_tint_color_w", m_tint_color.Value.w);
};
