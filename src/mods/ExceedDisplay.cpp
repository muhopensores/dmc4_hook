#include "ExceedDisplay.hpp"
#include "..\sdk\Devil4.hpp"

bool ExceedDisplay::mod_enabled = false;
uintptr_t ExceedDisplay::jmp_ret1 = NULL;
uintptr_t ExceedDisplay::jmp_ret2 = NULL;
ImVec2 ExceedDisplay::windowPos{ 0.0f, 0.0f };
static glm::ivec2 screen_res{ 0, 0 };
static std::chrono::time_point<std::chrono::high_resolution_clock> exceedPressTime  = std::chrono::high_resolution_clock::now();
static std::chrono::time_point<std::chrono::high_resolution_clock> exceedWindowTime  = std::chrono::high_resolution_clock::now();
static float exceedTimerSnapshot = 0.0f;

static void record_exceed_press_time(uPlayer* player) {
    exceedPressTime = std::chrono::high_resolution_clock::now();
    exceedTimerSnapshot = player->exceedTimer;
}

// get when exceed was pressed
naked void detour1() {
    _asm {
            cmp byte ptr [ExceedDisplay::mod_enabled], 1
            jne originalcode

            pushad
            push esi
            call record_exceed_press_time
            add esp, 4
            popad

        originalcode:
            cmp [esi+0x00001D7F], bl
            jmp dword ptr [ExceedDisplay::jmp_ret1]
    }
}

static void record_exceed_window_time(int canExceedSource) {
    //uint8_t firstByte = canExceedSource & 0xFF;
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

void ExceedDisplay::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (!player) { return; }
        if (player->controllerID == 0) { return; }
        float exceedTimeSeconds = std::chrono::duration<float>(exceedPressTime - exceedWindowTime).count();
        static const float frameTime = 1.0f / 60.0f; // 0.01667f
        float exceedTimeFrames = exceedTimeSeconds / frameTime;

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
        ImGui::SliderFloat("##exceedTimer", &player->exceedTimer, 0.0f, 10.0f);
        ImGui::SameLine();
        help_marker(_("This slider shows the exceed timer in realtime"));
        ImGui::PopItemWidth();
        ImGui::End();
    }
}

void ExceedDisplay::on_gui_frame(int display) {
    ImGui::Checkbox(_("Exceed Display"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("WIP, will be neater when more understood"));
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
    return Mod::on_initialize();
}

void ExceedDisplay::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("ExceedDisplay").value_or(false);
    windowPos.x = cfg.get<float>("exceed_display_pos_x").value_or(0.0f);
    windowPos.y = cfg.get<float>("exceed_display_pos_y").value_or(0.0f);
}

void ExceedDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("ExceedDisplay", mod_enabled);
    cfg.set<float>("exceed_display_pos_x", windowPos.x);
    cfg.set<float>("exceed_display_pos_y", windowPos.y);
}
