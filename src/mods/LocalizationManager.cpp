// include your mod header file
#include "LocalizationManager.hpp"
#include "utility\Compressed.hpp"

#include "misc/FontRoboto.cpp"

// compiled gettext files here
#include "../i18n/en_dmc4hook.hpp"
#include "../i18n/ru_dmc4hook.hpp"
#include "../i18n/zh_dmc4hook.hpp"

typedef const ImWchar* (ImFontAtlas::*GGR)();

struct CompressedMoFile {
    void* m_data{};
    const unsigned int m_size{};
    GGR m_imgui_glyph_range{};
    const char* m_font_file{};
};

static std::unordered_map<const char*, CompressedMoFile> g_locales_map {
    // NOTE(): example locale, doesnt contain any translated strings
    { "en", { (void*)en_dmc4hook_compressed_data, en_dmc4hook_compressed_size, &ImFontAtlas::GetGlyphRangesDefault, nullptr } },
    // for zh, also consider C:\\Windows\\Fonts\\msyh.ttc
    { "zh", { (void*)zh_dmc4hook_compressed_data, zh_dmc4hook_compressed_size, &ImFontAtlas::GetGlyphRangesChineseFull, "C:\\Windows\\Fonts\\simhei.ttf" } },
#ifndef NDEBUG // testing - machine translated file
    { "ru", { (void*)ru_dmc4hook_compressed_data, ru_dmc4hook_compressed_size, &ImFontAtlas::GetGlyphRangesCyrillic, nullptr } },
#endif
};

std::optional<std::string> LocalizationManager::on_initialize() {
    return Mod::on_initialize();
}

ImFont* load_locale_and_imfont(const char* country_code) noexcept {
    auto& io = ImGui::GetIO();
    if(io.Fonts->IsBuilt()) {
        io.Fonts->Clear();
    }
    
    const ImWchar* glyph_range{};
    ImFontAtlas* atlas = io.Fonts;
    ImFont* result = nullptr;
    
    for (const auto& pair : g_locales_map) {
        // fuck off chat jipity, i used strncmp this time for maximum safety
        if (std::strncmp(pair.first, country_code, 3) == 0) {
            const CompressedMoFile& mo = pair.second;
            auto [data, size] = utility::decompress_file_from_memory_with_size(mo.m_data, mo.m_size);
            IM_ASSERT(data != nullptr);
            IM_ASSERT(size > 0);
            utility::mo_load(data);

            // NOTE(): calls ImGui GetGlyphRanges*(); but i wanted to store it in a map 
            // along with compiled gettext localization file. imgui had something to 
            // calculate glyph ranges from provided text, but that might require 
            // getting all translated string from compiled mo file.
            // TODO(): eh good enough for now...
            
            glyph_range = (atlas->*(mo.m_imgui_glyph_range))();
            
            if (mo.m_font_file != nullptr) {
                result = atlas->AddFontFromFileTTF(mo.m_font_file, 18.0f, NULL, glyph_range);
            } else {
                result = atlas->AddFontFromMemoryCompressedBase85TTF(roboto_medium_compressed_data_base85, 18.0f, NULL, glyph_range);
            }
            
            break;
        }
    }
    
    if (!result) {
        result = atlas->AddFontFromMemoryCompressedBase85TTF(roboto_medium_compressed_data_base85, 18.0f, NULL, nullptr);
    }
    
    g_framework->m_imfont_main = result;
    return result;
}

//void LocalizationManager::on_frame(fmilliseconds& dt) {}
 
void LocalizationManager::on_config_save(utility::Config& cfg) { 
    std::string locale(g_framework->m_glob_locale);
    cfg.set("language", locale); 
};

void LocalizationManager::on_config_load(const utility::Config& cfg) {
    std::string country_code = cfg.get("language").value_or("en");
    g_framework->on_locale_update(country_code.c_str());
};

void LocalizationManager::on_gui_frame(int display) { 
    static const char* current_item = g_framework->m_glob_locale;
    const char* lang_string = _("Language");
    IM_ASSERT(lang_string);
    const float str_size = ImGui::CalcTextSize(lang_string).x + (ImGui::GetTextLineHeightWithSpacing() * 1.1f);
    ImGui::PushItemWidth(str_size);
    if (ImGui::BeginCombo(lang_string, current_item)) {
        for (const auto& pair : g_locales_map) {
            bool is_selected = (current_item == pair.first);
            if (ImGui::Selectable(pair.first, is_selected)) {
                current_item = pair.first;
                g_framework->on_locale_update(current_item);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    help_marker(_("Save config to load language settings on start.\nPlease submit your translations for dmc4hook :pray:"));
};
