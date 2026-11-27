#include "AllocatorThings.hpp"
#include "sdk/MtAllocator.hpp"
#include "ModFramework.hpp"
#include "imgui/imgui.h"
#include <cstdio>


struct Devil4Allocators {
    MtHeapAllocator** m_global      = (MtHeapAllocator**)0x00C307A8;
    MtHeapAllocator** m_temp        = (MtHeapAllocator**)0x00C307AC;
    MtHeapAllocator** m_resource    = (MtHeapAllocator**)0x00E14348;
    MtHeapAllocator** m_material    = (MtHeapAllocator**)0x00E143E4;
    MtHeapAllocator** m_system      = (MtHeapAllocator**)0x00E14378;
    MtHeapAllocator** m_unit        = (MtHeapAllocator**)0x00E1434C;
    MtHeapAllocator** m_area        = (MtHeapAllocator**)0x00E14344;
    MtHeapAllocator** m_trans       = (MtHeapAllocator**)0x00E143E0;
    MtHeapAllocator** m_array       = (MtHeapAllocator**)0x00E142FC;
    MtHeapAllocator** m_string      = (MtHeapAllocator**)0x00E142F8;
    MtHeapAllocator** m_sound       = (MtHeapAllocator**)0x00E143DC;

};

static Devil4Allocators g_d4_allocators;

static std::array allocators = {
    (MtHeapAllocator*)g_d4_allocators.m_global,
    (MtHeapAllocator*)g_d4_allocators.m_temp,
    (MtHeapAllocator*)g_d4_allocators.m_resource,
    (MtHeapAllocator*)g_d4_allocators.m_material,
    (MtHeapAllocator*)g_d4_allocators.m_system,
    (MtHeapAllocator*)g_d4_allocators.m_unit,
    (MtHeapAllocator*)g_d4_allocators.m_area,
    (MtHeapAllocator*)g_d4_allocators.m_trans,
    (MtHeapAllocator*)g_d4_allocators.m_array,
    (MtHeapAllocator*)g_d4_allocators.m_string,
    (MtHeapAllocator*)g_d4_allocators.m_sound,
};

// NOTE: the elements of `allocators` are NOT allocator objects but the addresses
// of the game's static `MtHeapAllocator*` globals - the slot addresses in
// Devil4Allocators are 4 bytes apart while sizeof(MtHeapAllocator) == 0x5C, so
// each element must be re-read as a `MtHeapAllocator**` and dereferenced once
// before any field is touched.
bool g_show_allocator_statistics = false;

void imgui_draw_allocator_statistics() {
    if (!g_show_allocator_statistics) {
        return;
    }
    ImFont* font = g_framework ? g_framework->m_imfont_main : nullptr;
    if (font == nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) {
        return;
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();

    constexpr float k_font = 16.0f;
    constexpr float k_pad = 10.0f;
    constexpr float k_col_gap = 10.0f;
    constexpr float k_row_gap = 5.0f;
    constexpr float k_bar_h = k_font; // bar spans the full text line height
    constexpr float k_margin = 12.0f;
    constexpr const char* k_header = "DEVIL 4 ALLOCATORS";

    struct Row {
        const char* name;
        char stat[40];
        float name_w;
        float stat_w;
        uint32_t used;
        uint32_t max_used;
        uint32_t total;
    };

    std::array<Row, allocators.size() + 1> rows{};
    size_t count = 0;
    uint64_t grand_used = 0;
    uint64_t grand_total = 0;

    auto fmt_kb = [](char* out, int cap, uint32_t used, uint32_t total) {
        const uint32_t uw = used >> 10;
        const uint32_t uf = (uint32_t)(((uint64_t)used * 100) >> 10) % 100;
        const uint32_t tw = total >> 10;
        const uint32_t tf = (uint32_t)(((uint64_t)total * 100) >> 10) % 100;
        std::snprintf(out, cap, "%u.%02u / %u.%02u KB", uw, uf, tw, tf);
    };

    for (MtHeapAllocator* slot : allocators) {
        MtHeapAllocator** indirect = reinterpret_cast<MtHeapAllocator**>(slot);
        MtHeapAllocator* alloc = indirect ? *indirect : nullptr;
        if (alloc == nullptr || alloc->m_total_size == 0) {
            continue;
        }

        Row& row = rows[count++];
        row.name = (alloc->m_name != nullptr && alloc->m_name[0] != '\0') ? alloc->m_name : "unnamed";
        row.used = alloc->m_used_size;
        row.max_used = alloc->m_max_used_size;
        row.total = alloc->m_total_size;
        fmt_kb(row.stat, (int)sizeof row.stat, row.used, row.total);
        row.name_w = font->CalcTextSizeA(k_font, 1e30f, 0.0f, row.name).x;
        row.stat_w = font->CalcTextSizeA(k_font, 1e30f, 0.0f, row.stat).x;

        grand_used += row.used;
        grand_total += row.total;
    }

    if (count == 0) {
        return;
    }

    {
        Row& row = rows[count++];
        row.name = "TOTAL";
        row.used = grand_used > UINT32_MAX ? UINT32_MAX : (uint32_t)grand_used;
        row.max_used = row.used;
        row.total = grand_total > UINT32_MAX ? UINT32_MAX : (uint32_t)grand_total;
        fmt_kb(row.stat, (int)sizeof row.stat, row.used, row.total);
        row.name_w = font->CalcTextSizeA(k_font, 1e30f, 0.0f, row.name).x;
        row.stat_w = font->CalcTextSizeA(k_font, 1e30f, 0.0f, row.stat).x;
    }

    float name_col = 0.0f;
    float stat_col = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        if (rows[i].name_w > name_col) {
            name_col = rows[i].name_w;
        }
        if (rows[i].stat_w > stat_col) {
            stat_col = rows[i].stat_w;
        }
    }

    const float header_w = font->CalcTextSizeA(k_font, 1e30f, 0.0f, k_header).x;
    float body_w = name_col + k_col_gap + stat_col + k_col_gap + stat_col;
    if (header_w > body_w) {
        body_w = header_w;
    }
    const float panel_w = body_w + 2.0f * k_pad;
    const float row_step = k_font + 2.0f * k_row_gap;
    const float panel_h = k_pad + k_font + 2.0f * k_row_gap
                        + (float)(count - 1) * row_step + k_font + k_row_gap + k_pad;

    float x0 = io.DisplaySize.x - panel_w - k_margin;
    if (x0 < k_margin) {
        x0 = k_margin;
    }
    const float y0 = k_margin;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + panel_w, y0 + panel_h), IM_COL32(6, 8, 14, 160));
    dl->AddRect(ImVec2(x0, y0), ImVec2(x0 + panel_w, y0 + panel_h), IM_COL32(150, 150, 170, 110));
    dl->AddText(font, k_font, ImVec2(x0 + k_pad, y0 + k_pad), IM_COL32(255, 240, 120, 255), k_header);

    const float text_x = x0 + k_pad;
    const float bar_x = text_x + name_col + k_col_gap;
    const float stat_x = bar_x + stat_col + k_col_gap;

    float y = y0 + k_pad + k_font + 2.0f * k_row_gap;
    for (size_t i = 0; i < count; ++i) {
        const Row& row = rows[i];
        const bool is_total = (i + 1 == count);

        float frac = (float)row.used / (float)row.total;
        if (frac > 1.0f) {
            frac = 1.0f;
        }
        float max_frac = (float)row.max_used / (float)row.total;
        if (max_frac > 1.0f) {
            max_frac = 1.0f;
        }

        const float bar_y = y; // bar shares the row with name/stat text, aligned to the line box
        dl->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + stat_col, bar_y + k_bar_h), IM_COL32(24, 28, 38, 230));
        if (frac > 0.0f) {
            const ImU32 fill = frac >= 0.90f ? IM_COL32(235, 80, 80, 255)
                                 : frac >= 0.70f ? IM_COL32(245, 205, 90, 255)
                                                 : IM_COL32(90, 215, 125, 255);
            dl->AddRectFilled(ImVec2(bar_x, bar_y), ImVec2(bar_x + stat_col * frac, bar_y + k_bar_h), fill);
        }
        if (!is_total && row.max_used > row.used) {
            const float max_x = bar_x + stat_col * max_frac;
            dl->AddLine(ImVec2(max_x, bar_y - 1.5f), ImVec2(max_x, bar_y + k_bar_h + 1.5f), IM_COL32(255, 255, 255, 230), 1.5f);
        }

        const ImU32 name_col32 = is_total ? IM_COL32(120, 205, 255, 255) : IM_COL32(230, 230, 240, 255);
        dl->AddText(font, k_font, ImVec2(text_x, y), name_col32, row.name);
        dl->AddText(font, k_font, ImVec2(stat_x, y), IM_COL32(195, 195, 210, 255), row.stat);

        y += row_step;
    }
}