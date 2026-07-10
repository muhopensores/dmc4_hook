#include "ExceedDisplay.hpp"
#include "..\sdk\Devil4.hpp"
#include "..\sdk\sRender.hpp"

bool ExceedDisplay::mod_enabled = false;
bool ExceedDisplay::chart_enabled = false;
uintptr_t ExceedDisplay::jmp_ret1 = NULL;
uintptr_t ExceedDisplay::jmp_ret2 = NULL;
uintptr_t ExceedDisplay::jmp_ret3 = NULL;
ImVec2 ExceedDisplay::windowPos{ 0.0f, 0.0f };
static glm::ivec2 screen_res{ 0, 0 };
static std::chrono::time_point<std::chrono::high_resolution_clock> exceedPressTime  = std::chrono::high_resolution_clock::now();
static std::chrono::time_point<std::chrono::high_resolution_clock> exceedWindowTime  = std::chrono::high_resolution_clock::now();
static float exceedTimerSnapshot = 0.0f;

static constexpr float incFloat = 0.01f;
static float exceedTimeline = 0.0f;
static constexpr ImColor failedExceedCol  = {220, 0, 0, 255}; // red
static constexpr ImColor perfectExceedCol = {255, 215, 0, 255}; // gold
static constexpr ImColor lateExceedCol    = {184, 144, 16, 255}; // dark gold
struct ExceedChartEntry {
    float time;
    float exceedTimer;
    bool canExceed;
};
struct ExceedAttemptEntry {
    float time;
    float exceedTimer;
    bool canExceed;
    bool isAttempt;
};
static constexpr int EXCEED_CHART_SIZE = 512;
static ExceedChartEntry exceedChart[EXCEED_CHART_SIZE];
static int exceedChartIndex = 0;
static ExceedAttemptEntry exceedAttemptChart[EXCEED_CHART_SIZE];
static int exceedAttemptIndex = 0;
static void update_exceed_chart_tick(uPlayer* player) {
    if (!player || player->controllerID != 1) {
        return;
    }

    exceedChart[exceedChartIndex] = { exceedTimeline, player->exceedTimer, player->canExceed == 1 };
    exceedChartIndex = (exceedChartIndex + 1) % EXCEED_CHART_SIZE;
}
static void update_exceed_chart_attempt() {
    const ExceedChartEntry& snapshot = exceedChart[(exceedChartIndex + EXCEED_CHART_SIZE - 1) % EXCEED_CHART_SIZE];
    exceedAttemptChart[exceedAttemptIndex] = { snapshot.time, snapshot.exceedTimer, snapshot.canExceed, true };
    exceedAttemptIndex = (exceedAttemptIndex + 1) % EXCEED_CHART_SIZE;
}
static void record_exceed_press_time(uPlayer* player) {
    if (!player || player->controllerID != 1) {
        return;
    }

    exceedPressTime = std::chrono::high_resolution_clock::now();
    exceedTimerSnapshot = player->exceedTimer;
}

// get when exceed was pressed
naked void detour1() {
    _asm {
            cmp byte ptr [ExceedDisplay::mod_enabled], 1
            jne checkChart

            pushad
            push esi
            call record_exceed_press_time
            add esp, 4
            popad

        checkChart:
            cmp byte ptr [ExceedDisplay::chart_enabled], 1
            jne originalcode
            pushad
            call update_exceed_chart_attempt
            popad

        originalcode:
            cmp [esi+0x00001D7F], bl
            jmp dword ptr [ExceedDisplay::jmp_ret1]
    }
}

static void record_exceed_window_time(int canExceedSource) {
    uint8_t secondByte = (canExceedSource >> 8) & 0xFF;
    bool canExceed = (secondByte == 1);
    static bool previousCanExceed = false;
    bool newlyExceedable = (canExceed && !previousCanExceed);
    if (newlyExceedable) {
        exceedWindowTime = std::chrono::high_resolution_clock::now();
    }
    previousCanExceed = canExceed;
}

// get when exceed window started
naked void detour2() {
    _asm {
            mov al, [esp+0x0D]
            cmp byte ptr [ExceedDisplay::mod_enabled], 1
            jne originalcode

            pushad
            push eax
            call record_exceed_window_time
            add esp, 4
            popad

        originalcode:
            test al, al
            jmp dword ptr [ExceedDisplay::jmp_ret2]
    }
}

// get player tick before exceed logic
naked void detour3() {
    _asm {
            cmp byte ptr [ExceedDisplay::chart_enabled], 1
            jne originalcode

            sub esp, 4
            movss [esp], xmm1
            movss xmm1, [exceedTimeline]
            addss xmm1, [incFloat]
            movss [exceedTimeline], xmm1
            movss xmm1, [esp]
            add esp, 4

            sub esp, 8
            movss [esp], xmm0
            movss [esp+4], xmm1
            pushad
            push esi
            call update_exceed_chart_tick
            add esp, 4
            popad
            movss xmm1, [esp+4]
            movss xmm0, [esp]
            add esp, 8

        originalcode:
            movss xmm0,[esi+0x0000CDB8]
            jmp dword ptr [ExceedDisplay::jmp_ret3]
    }
}

void ExceedDisplay::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (!player) { return; }
        if (player->controllerID != 1) { return; }
        float exceedTimeSeconds = std::chrono::duration<float>(exceedPressTime - exceedWindowTime).count();
        static const float frameTime = 1.0f / 60.0f; // 0.01667f
        float exceedTimeFrames = exceedTimeSeconds / frameTime;
        float exceedTimer = player->exceedTimer;

        ImGui::Begin("Exceed Timer UI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);
        ImGui::SetWindowPos(windowPos, ImGuiCond_Once);
        windowPos = ImGui::GetWindowPos();
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::SliderFloat("##ManualExceedTimer", &exceedTimeFrames, -10.0f, 0.0f);
        ImGui::SameLine();
        help_marker(_("This slider shows how long before the exceed window started you pressed the exceed button. This is calculated by dmc4_hook"));
        ImGui::SliderFloat("##exceedTimerSnapshot", &exceedTimerSnapshot, 0.0f, 10.0f);
        ImGui::SameLine();
        help_marker(_("This slider shows a snapshot of what the exceed timer read when the exceed button was pressed"));
        ImGui::SliderFloat("##exceedTimer", &exceedTimer, 0.0f, 10.0f);
        ImGui::SameLine();
        help_marker(_("This slider shows the exceed timer in realtime"));
        ImGui::PopItemWidth();
        ImGui::End();
    }
    if (chart_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            if (player->controllerID != 1) { return; }
            if (sRender* sRen = devil4_sdk::get_sRender()) {
                Vector2f screen_res = sRen->screenRes;
                float uiScale = screen_res.y / 1080.0f;
                float panelWidth = screen_res.x;
                static const float timelineHeight = 20.0f * uiScale;
                static const float hitExtendAmount = 10.0f * uiScale;
                static const float panelHeight = timelineHeight + (hitExtendAmount * 2.0f);
                float posX = (screen_res.x - panelWidth) * 0.5f;
                float posY = screen_res.y;

                ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
                ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Exceed Timeline", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 origin = ImGui::GetCursorScreenPos();
                draw->AddRectFilled(origin, ImVec2(origin.x + panelWidth, origin.y + panelHeight), IM_COL32(20, 20, 20, 255));
                static const float timelineTop = origin.y + hitExtendAmount;
                static const float timelineBottom = timelineTop + timelineHeight;
                static const float graphLeft = origin.x;
                static const float graphWidth = panelWidth;
                static const float timeWindow = 4.0f;
                float now = exceedChart[(exceedChartIndex + EXCEED_CHART_SIZE - 1) % EXCEED_CHART_SIZE].time;

                // window
                for (int i = 0; i < EXCEED_CHART_SIZE; i++) {
                    const ExceedChartEntry& entry = exceedChart[i];
                    if (entry.time <= 0.0f || !entry.canExceed) continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f) continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    ImU32 color = lateExceedCol;
                    if (entry.exceedTimer >= 0.0f && entry.exceedTimer < 1.0f) {
                        color = perfectExceedCol;
                    }
                    draw->AddLine(ImVec2(x, timelineTop), ImVec2(x, timelineBottom), color, 2.0f * uiScale);
                }

                // presses
                for (int i = 0; i < EXCEED_CHART_SIZE; i++) {
                    const ExceedAttemptEntry& entry = exceedAttemptChart[i];
                    if (!entry.isAttempt || entry.time <= 0.0f) continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f) continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    bool success = entry.canExceed && entry.exceedTimer >= 0.0f;
                    ImU32 hitColor = failedExceedCol;
                    if (success) {
                        hitColor = (entry.exceedTimer < 1.0f) ? perfectExceedCol : lateExceedCol;
                    }
                    draw->AddLine(ImVec2(x, timelineTop - hitExtendAmount), ImVec2(x, timelineBottom + hitExtendAmount), hitColor, 1.0f * uiScale);
                }
                ImGui::PopStyleVar();
                ImGui::End();
            }
        }
    }
}

void ExceedDisplay::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Exceed Timing Display"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("WIP, will be neater when more understood"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Exceed Timing Chart"), &chart_enabled);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(_("See how early or late multiple exceed presses in a row were"));
            ImGui::TextColored(failedExceedCol, _("Failed"));
            ImGui::TextColored(lateExceedCol, _("Late"));
            ImGui::TextColored(perfectExceedCol, _("Perfect"));
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> ExceedDisplay::on_initialize() {
    if (!install_hook_offset(0x3E49CA, hook1, &detour1, &jmp_ret1, 6)) { // get when exceed was pressed
        spdlog::error("Failed to init ExceedDisplay mod 1\n");
        return "Failed to init ExceedDisplay mod 1";
    }
    if (!install_hook_offset(0x3AB824, hook2, &detour2, &jmp_ret2, 6)) { // get when exceed window started
        spdlog::error("Failed to init ExceedDisplay mod 2\n");
        return "Failed to init ExceedDisplay mod 2";
    }
    if (!install_hook_offset(0x3E48F2, hook3, &detour3, &jmp_ret3, 8)) { // get player tick before exceed logic
        spdlog::error("Failed to init ExceedDisplay mod 3\n");
        return "Failed to init ExceedDisplay mod 3";
    }

    return Mod::on_initialize();
}

void ExceedDisplay::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ExceedDisplay").value_or(false);
    chart_enabled = cfg.get<bool>("exceed_display_chart").value_or(false);
    windowPos.x = cfg.get<float>("exceed_display_pos_x").value_or(0.0f);
    windowPos.y = cfg.get<float>("exceed_display_pos_y").value_or(0.0f);
}

void ExceedDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ExceedDisplay", mod_enabled);
    cfg.set<bool>("exceed_display_chart", chart_enabled);
    cfg.set<float>("exceed_display_pos_x", windowPos.x);
    cfg.set<float>("exceed_display_pos_y", windowPos.y);
}
