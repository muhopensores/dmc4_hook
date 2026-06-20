#include "GuardTimer.hpp"
#include "imgui/imgui.h"
#include "imgui_internal.h"

#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/sWorkRate.hpp"

bool      GuardTimer::mod_enabled   = false;
bool      GuardTimer::chart_enabled   = false;
uintptr_t GuardTimer::jmp_ret1      = NULL;
uintptr_t GuardTimer::jmp_ret2      = NULL;
uintptr_t GuardTimer::jmp_ret3      = NULL;
uintptr_t GuardTimer::jmp_ret4      = NULL;
float GuardTimer::lastGuardTime = 0.0f;
static float guardTimeline = 0.0f;

static std::chrono::time_point<std::chrono::high_resolution_clock> damageTime = std::chrono::high_resolution_clock::now();
static std::chrono::time_point<std::chrono::high_resolution_clock> guardTime  = std::chrono::high_resolution_clock::now();

static void record_damage_time() {
    damageTime = std::chrono::high_resolution_clock::now();
}
static void record_guard_time() {
    guardTime = std::chrono::high_resolution_clock::now();
}
struct ChartEntry {
    float time;
    float blockTimer;
    bool blockHeld;
    bool canBlock;
    bool isHit;
};
static constexpr int CHART_SIZE = 512;
static ChartEntry guardChart[CHART_SIZE];
static int guardIndex = 0;
static ChartEntry hitChart[CHART_SIZE];
static int hitIndex = 0;
static bool checkCanBlock(uPlayer* player) {
    __asm {
        mov ecx, [player]
        test ecx, ecx
        je retcode
        mov edx, 0x7BC120
        call edx
        retcode:
    }
}
static void update_chart_hit(float timeline) {
    const ChartEntry& snapshot = guardChart[(guardIndex + CHART_SIZE - 1) % CHART_SIZE];
    hitChart[hitIndex] = {snapshot.time, snapshot.blockTimer, snapshot.blockHeld, snapshot.canBlock, true};
    hitIndex = (hitIndex + 1) % CHART_SIZE;
}
static void update_chart_tick(uPlayer* player, float timeline) {
    bool guardPressed      = (player->inputHold[1] & 0x6) != 0;
    bool canBlock          = checkCanBlock(player);
    float guardTimer       = player->guardTimer;
    guardChart[guardIndex] = {timeline, guardTimer, guardPressed, canBlock, false};
    guardIndex = (guardIndex + 1) % CHART_SIZE;
}

// chart hits + timer hits
naked void detour1(void) { // called when the player is hit
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        je GuardTimerCode
        cmp byte ptr [GuardTimer::chart_enabled], 1
        je GuardChartCode

        GuardTimerCode:
        pushad
        call record_damage_time
        popad

        sub esp, 4
        movss [esp], xmm0
        movss xmm0, [ecx+0x14D44]
        movss [GuardTimer::lastGuardTime],xmm0 // game reading slider
        movss xmm0, [esp]
        add esp, 4
        jmp check2

        check2:
        cmp byte ptr [GuardTimer::chart_enabled], 1
        jne originalcode

        GuardChartCode:
        sub esp, 0xC
        movss [esp], xmm0
        movss [esp+4], xmm1
        movss [esp+8], xmm2
        pushad
        push guardTimeline
        push eax // player
        call update_chart_hit
        add esp, 0x8
        popad
        movss xmm2, [esp+8]
        movss xmm1, [esp+4]
        movss xmm0, [esp]
        add esp, 0xC

        originalcode:
        mov edx, [eax+0x000002D8]
        jmp dword ptr [GuardTimer::jmp_ret1]
    }
}

naked void detour2(void) { // called when the player presses guard // player in edi
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        jne originalcode

        pushad
        call record_guard_time
        popad

        originalcode:
        xorps xmm0, xmm0
        movss [edi+0x00014D44],xmm0
        jmp dword ptr [GuardTimer::jmp_ret2]
    }
}

naked void detour3(void) { // called when the player presses release // player in esi
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        jne originalcode

        pushad
        call record_guard_time
        popad

        originalcode:
        movss xmm0, ds:[0x0B9A27C] // omg after all these years all I needed was ds:
        jmp dword ptr [GuardTimer::jmp_ret3]
    }
}

// chart tick
static constexpr float incFloat = 0.01f;
static constexpr uintptr_t checkBlockActSet = 0x7BDEF0;
static constexpr uintptr_t sub_7b6f80       = 0x7B6F80;
naked void detour4(void) { // called every tick a few opcodes before guard button comparing
    _asm {
        call dword ptr [sub_7b6f80] // ogcode
        cmp byte ptr [GuardTimer::chart_enabled], 1
        jne retcode

        sub esp, 4
        movss [esp], xmm1
        movss xmm1, [guardTimeline]
        addss xmm1, [incFloat]
        movss [guardTimeline], xmm1
        movss xmm1, [esp]
        add esp, 4

        sub esp, 0xC
        movss [esp], xmm0
        movss [esp+4], xmm1
        movss [esp+8], xmm2
        pushad
        push guardTimeline
        push edi // player
        call update_chart_tick
        add esp,0x8
        popad
        movss xmm2, [esp+8]
        movss xmm1, [esp+4]
        movss xmm0, [esp]
        add esp, 0xC
        jmp retcode

        retcode:
        jmp dword ptr [GuardTimer::jmp_ret4]
    }
}

void GuardTimer::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            if (player->controllerID != 0) { return; }
            ImGuiIO& io = ImGui::GetIO();
            if (sRender* sRen = devil4_sdk::get_sRender()) {
                Vector2f screen_res = sRen->screenRes;
                ImGui::Begin("GuardTimerGUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
                ImVec2 window_size = ImGui::GetWindowSize();
                ImVec2 window_pos((screen_res.x - window_size.x) * 0.5f, 0);
                ImGui::SetWindowPos(window_pos);

                float damageTimeSeconds = std::chrono::duration<float>(guardTime - damageTime).count();
                float turboSpeed = devil4_sdk::get_sMediator()->turboEnabled ? devil4_sdk::get_work_rate()->turbo_speed : devil4_sdk::get_work_rate()->game_speed;
                float danteSpeed = player->dtActive ? 1.1f : 1.0f;
                float windowWidth = ImGui::GetWindowSize().x;
                float blockSliderWidth = screen_res.x * 0.4f;
                static const float frameTime = 1.0f / 60.0f; // 0.01667f
                float damageTimeFrames = damageTimeSeconds / frameTime;

                // centre
                float wideLeftX = (windowWidth - blockSliderWidth) * 0.5f;
                float wideCenterX = wideLeftX + (blockSliderWidth * 0.5f);

                // adjustment for the perfect slider because imgui moment
                static float perfectSliderAdjustSize = 1.06f;
                static float perfectSliderAdjustPlacement = 4.0f;

                float perfectSliderWidth = screen_res.x * 0.05f * perfectSliderAdjustSize;
                float perfectSliderLeftX = wideCenterX + (screen_res.x * (perfectSliderAdjustPlacement / 1920.0f) - perfectSliderWidth);
                {
                    ImGui::SetCursorPosX(wideLeftX);
                    ImGui::PushItemWidth(blockSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Block Timing", &damageTimeFrames, -5.0f * 4 / turboSpeed / danteSpeed, 5.0f * 4 / turboSpeed / danteSpeed, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker(_("0.0, the middle, is when damage was applied\n"
                                "The marker location is your block timing relative to that damage application"));
                }
                {
                    float minFrames = -5.0f / turboSpeed / danteSpeed;
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing 2", &damageTimeFrames, minFrames, 0.0f, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::Text("/%f", minFrames);
                    ImGui::SameLine();
                    help_marker(_("This is the perfect block window\n"
                        "Anything above this number to below or equal 0 is a perfect block / release\n"
                        "At 60fps with no turbo and no DT you must guard under 5 frames."));
                }
                {
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing Game", &lastGuardTime, 5.0f, 0.0f, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker("This is the game's internal calculation of the perfect block window\n"
                        "anything below 5 is a perfect block");
                }
                ImGui::End();
            }
        }
    }
    if (chart_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            if (player->controllerID != 0) { return; }
            ImGuiIO& io = ImGui::GetIO();
            if (sRender* sRen = devil4_sdk::get_sRender()) {
                Vector2f screen_res         = sRen->screenRes;
                float uiScale               = screen_res.y / 1080.0f;
                float panelWidth            = screen_res.x;
                const float timelineHeight  = 20.0f * uiScale;
                const float hitExtendAmount = 10.0f * uiScale;
                const float panelHeight     = timelineHeight + (hitExtendAmount * 2.0f);
                float posX                  = (screen_res.x - panelWidth) * 0.5f;
                float posY                  = screen_res.y;

                ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
                ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Guard Timeline", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 origin    = ImGui::GetCursorScreenPos();
                draw->AddRectFilled(origin, ImVec2(origin.x + panelWidth, origin.y + panelHeight), IM_COL32(20, 20, 20, 255));
                const float timelineTop    = origin.y + hitExtendAmount;
                const float timelineBottom = timelineTop + timelineHeight;
                const float graphLeft      = origin.x;
                const float graphWidth     = panelWidth;
                const float timeWindow     = 4.0f;
                float now                  = guardChart[(guardIndex + CHART_SIZE - 1) % CHART_SIZE].time;
                const float sampleWidth = graphWidth / (float)CHART_SIZE;

                // canBlock
                for (int i = 0; i < CHART_SIZE; i++) {
                    const ChartEntry& entry = guardChart[i];
                    if (entry.time <= 0.0f || !entry.canBlock)
                        continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    draw->AddRectFilled(ImVec2(x - sampleWidth * 0.5f, timelineTop - hitExtendAmount),
                        ImVec2(x + sampleWidth * 0.5f, timelineBottom + hitExtendAmount), IM_COL32(0, 64, 0, 100));
                }

                // blocking
                for (int i = 0; i < CHART_SIZE; i++) {
                    const ChartEntry& entry = guardChart[i];
                    if (entry.time <= 0.0f)
                        continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    if (entry.blockHeld) {
                        ImU32 color = (entry.blockTimer < 5.0f) ? IM_COL32(0, 255, 255, 180) : IM_COL32(255, 140, 0, 160);
                        draw->AddLine(ImVec2(x, timelineTop), ImVec2(x, timelineBottom), color, 1.0f * uiScale);
                    }
                }

                // hit markers
                for (int i = 0; i < CHART_SIZE; i++) {
                    const ChartEntry& e = hitChart[i];
                    if (!e.isHit || e.time <= 0.0f)
                        continue;
                    float normalizedTime = (e.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x        = graphLeft + normalizedTime * graphWidth;
                    ImU32 hitColor = (e.blockHeld) ? ((e.blockTimer < 5.0f) ? IM_COL32(0, 255, 255, 255) : IM_COL32(255, 140, 0, 255))
                                                   : IM_COL32(255, 0, 0, 255);
                    draw->AddLine(
                        ImVec2(x, timelineTop - hitExtendAmount), ImVec2(x, timelineBottom + hitExtendAmount), hitColor, 2.0f * uiScale);
                }
                ImGui::PopStyleVar();
                ImGui::End();
            }
        }
    }
}

void GuardTimer::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Royal Guard Timing Display"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("See how early or late your guards were"));
    } else if (display == DISPLAY_SYSTEM_B) {
        ImGui::Checkbox("Royal Guard Timing Chart", &chart_enabled);
        ImGui::SameLine();
        help_marker(_("See how early or late multiple guards in a row were"));
    }
}

std::optional<std::string> GuardTimer::on_initialize() {
    // DevilMayCry4_DX9.exe+
    if (!install_hook_offset(0x3BC185, hook1, &detour1, &jmp_ret1, 6)) { // player takes damage
		spdlog::error("Failed to init GuardTimer mod 1\n");
		return "Failed to init GuardTimer mod 1";
	}
    if (!install_hook_offset(0x3B6FD6, hook2, &detour2, &jmp_ret2, 11)) { // player blocks
	    spdlog::error("Failed to init GuardTimer mod 2\n");
	    return "Failed to init GuardTimer mod 2";
	}
    if (!install_hook_offset(0x3B6B3D, hook3, &detour3, &jmp_ret3, 8)) { // player releases
		spdlog::error("Failed to init GuardTimer mod 3\n");
		return "Failed to init GuardTimer mod 3";
	}
    if (!install_hook_offset(0x3B7286, hook4, &detour4, &jmp_ret4, 5)) { // player tick, after guard logic
        spdlog::error("Failed to init GuardTimer mod 4\n");
        return "Failed to init GuardTimer mod 4";
    }

    return Mod::on_initialize();
}

void GuardTimer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("guard_timer_display", mod_enabled);
    cfg.set<bool>("guard_timer_chart", chart_enabled);
}

void GuardTimer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("guard_timer_display").value_or(false);
    chart_enabled = cfg.get<bool>("guard_timer_chart").value_or(false);
}
