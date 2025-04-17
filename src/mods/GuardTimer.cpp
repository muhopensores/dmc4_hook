#include "GuardTimer.hpp"

#if 1
bool      GuardTimer::mod_enabled   = false;
uintptr_t GuardTimer::jmp_ret1      = NULL;
uintptr_t GuardTimer::jmp_ret2      = NULL;
uintptr_t GuardTimer::jmp_ret3      = NULL;
float     GuardTimer::lastGuardTime = 0.0f;
static std::chrono::time_point<std::chrono::high_resolution_clock> damageTime = std::chrono::high_resolution_clock::now();
static std::chrono::time_point<std::chrono::high_resolution_clock> guardTime  = std::chrono::high_resolution_clock::now();
static constexpr float hundredFloat = 100.0f;
static float playerDelta = 0.0f;
static glm::ivec2 screen_res{ 0, 0 };

void record_damage_time() {
    damageTime = std::chrono::high_resolution_clock::now();
}

void record_guard_time() {
    guardTime = std::chrono::high_resolution_clock::now();
}

naked void detour1(void) { // called when the player is hit
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        jne originalcode

        pushad
        call record_damage_time
        popad

        movss xmm0, [ecx+0x14D44]
        comiss xmm0, [hundredFloat]
        je originalcode
        comiss xmm0, [hundredFloat]
        jae originalcode
        movss [GuardTimer::lastGuardTime],xmm0

        originalcode:
        mov edx,[eax+0x000002D8]
        jmp dword ptr [GuardTimer::jmp_ret1]
    }
}

naked void detour2(void) { // called when the player presses guard // player in edi
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        jne originalcode

        pushad
        mov eax, [edi+0x10]
        mov [playerDelta], eax
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
        mov eax, [esi+0x10]
        call record_guard_time
        popad

        originalcode:
        movss xmm0, ds:[0x0B9A27C] // omg after all these years all I needed was ds:
        jmp dword ptr [GuardTimer::jmp_ret3]
    }
}

void GuardTimer::on_gui_frame(int display) {
    ImGui::Checkbox(_("Royal Guard Timing Display"), &mod_enabled);
}

void GuardTimer::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            ImGuiIO& io = ImGui::GetIO();
            if (sRender* sRen = devil4_sdk::get_sRender()) {
                screen_res = sRen->screenRes;
                ImGui::Begin("GuardTimerGUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
                ImVec2 window_size = ImGui::GetWindowSize();
                ImVec2 window_pos((screen_res.x - window_size.x) * 0.5f, 0);
                ImGui::SetWindowPos(window_pos);

                float damageTimeSeconds = std::chrono::duration<float>(guardTime - damageTime).count();
                float turboSpeed = devil4_sdk::get_sMediator()->turboEnabled ? devil4_sdk::get_work_rate()->turbo_speed : devil4_sdk::get_work_rate()->game_speed;
                float windowWidth = ImGui::GetWindowSize().x;
                float blockSliderWidth = screen_res.x * 0.4f;
                static const float frameTime = 1.0f / 60.0f; // 0.01667f

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
                    ImGui::SliderFloat("## Block Timing", &damageTimeSeconds, (-0.0833f / turboSpeed) * 4, (0.0833f / turboSpeed) * 4, "%.4fs", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker(_("0.0, the middle, is when damage was applied\n"
                        "The marker location is your block timing relative to that damage application in seconds"));
                }
                /*{
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing 1", &damageTimeSeconds, -0.0833f / turboSpeed, 0.0f, "%.4fs", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker("This is the perfect block window in seconds\n"
                        "Perfect block / release cutoff is 0.0833 seconds for non-turbo\n"
                        "and 0.0694 seconds for turbo\n"
                        "(5 frames * 0.01667 frame time = 0.0833 seconds)\n"
                        "(5 frames / 1.2 turbo (4.1666) * 0.01667 frame time = 0.0694 seconds)");
                }*/
                {
                    float damageTimeFrames = damageTimeSeconds / frameTime;
                    float minFrames = -5.0f / turboSpeed;
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing 2", &damageTimeFrames, minFrames, 0.0f, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker(_("This is the perfect block window\n"
                        "In turbo, you must guard before 4.1667 frames instead of before 5"));
                }
                /*{
                    float damageTimeFrames = damageTimeSeconds / (frameTime / turboSpeed);
                    float minFrames = -5.0f;
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing 3", &damageTimeFrames, minFrames, 0.0f, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker("This is the perfect block window calculated from real time\n"
                        "anything above -5 to below or equal 0 is a perfect block / release");
                }*/
                /*{
                    ImGui::SetCursorPosX(perfectSliderLeftX);
                    ImGui::PushItemWidth(perfectSliderWidth);
                    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 2);
                    ImGui::SliderFloat("## Royal Block Timing Game", &lastGuardTime, 5.0f, 0.0f, "%.2ff", ImGuiSliderFlags_ReadOnly);
                    ImGui::PopStyleVar();
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    help_marker("This is the game's internal calculation of the perfect block window\n"
                        "anything below 5 is a perfect block");
                }*/
                ImGui::End();
            }
        }
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
    return Mod::on_initialize();
}

void GuardTimer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("guard_timer_display", mod_enabled);
}

void GuardTimer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("guard_timer_display").value_or(false);
}
#endif
