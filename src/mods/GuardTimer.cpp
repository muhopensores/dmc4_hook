#include "GuardTimer.hpp"

#if 1
static std::chrono::time_point<std::chrono::high_resolution_clock> damageTime = std::chrono::high_resolution_clock::now();
static std::chrono::time_point<std::chrono::high_resolution_clock> guardTime  = std::chrono::high_resolution_clock::now();
uintptr_t GuardTimer::jmp_ret1      = NULL;
uintptr_t GuardTimer::jmp_ret2      = NULL;
bool      GuardTimer::mod_enabled   = false;
float     GuardTimer::lastGuardTime = 0.0f;
static constexpr float hundredFloat = 100.0f;
static float playerDelta = 0.0f;

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

        push eax
        push ecx
        push edx
        call record_damage_time
        pop edx
        pop ecx
        pop eax

        movss xmm0, [ecx+0x14D44]
        comiss xmm0, [hundredFloat]
        je originalcode
        comiss xmm0, [hundredFloat]
        jae originalcode
        movss [GuardTimer::lastGuardTime],xmm0

        originalcode:
        mov edx,[eax+0x000002D8]
        // movss xmm0, dword ptr ds:[0xB9A21C] // omg after all these years all I needed was ds:
        jmp dword ptr [GuardTimer::jmp_ret1]
    }
}

naked void detour2(void) { // called when the player presses guard
    _asm {
        cmp byte ptr [GuardTimer::mod_enabled], 1
        jne originalcode

        push eax
        push ecx
        push edx
        mov eax, [edi+0x10]
        mov [playerDelta], eax
        call record_guard_time
        pop edx
        pop ecx
        pop eax

        originalcode:
        movss [edi+0x00014D44],xmm0
        jmp dword ptr [GuardTimer::jmp_ret2]
    }
}

void GuardTimer::on_gui_frame() {
    ImGui::Checkbox("Royal Guard Timing Display", &mod_enabled);
}

void GuardTimer::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            ImGuiIO& io = ImGui::GetIO();
            glm::ivec2 screen_res = devil4_sdk::get_sRender()->screenRes;
        
            ImGui::Begin("GuardTimerGUI", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_pos((screen_res.x - window_size.x) * 0.5f, 0);
            ImGui::SetWindowPos(window_pos);

            float damageTimeSeconds = std::chrono::duration<float>(guardTime - damageTime).count();
            // float turboSpeed = devil4_sdk::get_sMediator()->turboEnabled ? devil4_sdk::get_work_rate()->turbo_speed : devil4_sdk::get_work_rate()->game_speed;
            ImGui::PushItemWidth(screen_res.x * 0.1f);
            ImGui::SliderFloat("## LateBlockPress", &damageTimeSeconds, -0.2f, 0.2f, "%.3f", ImGuiSliderFlags_ReadOnly);
            ImGui::SameLine();
            ImGui::SameLine();
            help_marker("Guard Time - Damage Time (in seconds)\n"
                "0.0, the middle, is when damage was applied\n"
                "The marker location is your block timing\n");

            ImGui::PushItemWidth(screen_res.x * 0.05f);
            float royalBlockCutoff = *(float*)0xB9A21C;
            ImGui::SliderFloat("## LastRoyalBlock", &lastGuardTime, 5.0f, 0.0f, "%.3f", ImGuiSliderFlags_ReadOnly);
            ImGui::SameLine();
            help_marker("This is the same concept but it's the game's calculation because I don't trust my math\n"
                "Only includes after guarding\n"
                "Any value below 5.0f counts as perfectly timed");
            ImGui::PopItemWidth();

            ImGui::End();
        }
    }
}

std::optional<std::string> GuardTimer::on_initialize() {
    // DevilMayCry4_DX9.exe+
    if (!install_hook_offset(0x3BC185, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init GuardTimer mod\n");
		return "Failed to init GuardTimer mod";
	}
    if (!install_hook_offset(0x3B6FD9, hook2, &detour2, &jmp_ret2, 8)) {
		spdlog::error("Failed to init GuardTimer mod\n");
		return "Failed to init GuardTimer mod";
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
