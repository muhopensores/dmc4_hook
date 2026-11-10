#include "CoopDiag.hpp"
#include "../../mod.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/cUnit.hpp"
#include <algorithm>

namespace coop_diag {

bool serial_lines = false;

} // namespace coop_diag

namespace {

constexpr uintptr_t kDevil4Main      = 0xE5574C; // sDevil4Main::mpInstance
constexpr uintptr_t kJobThreadNum    = 0x8C;     // sMain::mJobThreadNum
constexpr uintptr_t kNextJobThreadNum = 0x94;    // sMain::mNextJobThreadNum

// Move lines whose units read sMediator's player / camera: players, stand, enemies (and shells on 18), camera, HUD.
constexpr int kContextLines[] = {13, 14, 15, 16, 18, 23, 25};

int  saved_parallel[32];
bool saved_valid = false;
bool forced = false;

float frame_ms[120] = {};
int   frame_pos = 0;

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

void apply(sUnit* su, bool serial) {
    if (serial) {
        if (!saved_valid) {
            for (int line : kContextLines) {
                saved_parallel[line] = su->mMoveLine[line].mParallel;
            }
            saved_valid = true;
        }
        for (int line : kContextLines) {
            su->mMoveLine[line].mParallel = 0;
        }
    } else if (saved_valid) {
        for (int line : kContextLines) {
            su->mMoveLine[line].mParallel = saved_parallel[line];
        }
        saved_valid = false;
    }
}

int count_units(const MoveLine& line) {
    int n = 0;
    for (const cUnit* u = reinterpret_cast<const cUnit*>(line.mTop); u && n < 4096; u = u->mp_next_unit) {
        n++;
    }
    return n;
}

} // namespace

namespace coop_diag {

void set_serial(bool serial) {
    serial_lines = serial;
}

void set_forced(bool on) {
    forced = on;
}

bool serial_now() {
    return saved_valid;
}

void game_tick() {
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) {
        return;
    }
    const bool want = serial_lines || forced;
    if (want || saved_valid) {
        apply(su, want);   // every tick while serial, in case the game set a line parallel again
    }
}

void on_frame(float ms) {
    frame_ms[frame_pos] = ms;
    frame_pos = (frame_pos + 1) % IM_ARRAYSIZE(frame_ms);
}

void draw() {
    const uintptr_t main = *reinterpret_cast<uintptr_t*>(kDevil4Main);
    if (main) {
        ImGui::Text(_("Job threads: %u (next %u)"), at<uint32_t>(main, kJobThreadNum), at<uint32_t>(main, kNextJobThreadNum));
    }
    float sum = 0.0f, worst = 0.0f;
    for (float ms : frame_ms) {
        sum += ms;
        worst = (std::max)(worst, ms);
    }
    ImGui::Text(_("Frame: %.2f ms average, %.2f ms worst (last %d frames)"), sum / IM_ARRAYSIZE(frame_ms), worst,
                (int)IM_ARRAYSIZE(frame_ms));

    sUnit* su = devil4_sdk::get_sUnit();
    if (!su) {
        return;
    }
    if (ImGui::TreeNode(_("Move lines"))) {
        if (ImGui::BeginTable("##CoopLines", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn(_("Line"));
            ImGui::TableSetupColumn(_("Name"));
            ImGui::TableSetupColumn(_("Parallel"));
            ImGui::TableSetupColumn(_("Paused"));
            ImGui::TableSetupColumn(_("Units"));
            ImGui::TableHeadersRow();
            for (int i = 0; i < 32; i++) {
                const MoveLine& line = su->mMoveLine[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%d", i);
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(line.mName ? line.mName : "");
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(line.mParallel ? _("yes") : _("no"));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(line.mPause ? _("yes") : _("no"));
                ImGui::TableNextColumn();
                ImGui::Text("%d", count_units(line));
            }
            ImGui::EndTable();
        }
        ImGui::TreePop();
    }
}

} // namespace coop_diag
