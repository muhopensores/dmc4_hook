#include "EnemyStepDisplay.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/sRender.hpp"

#if 1
bool EnemyStepDisplay::mod_enabled = false;
uintptr_t EnemyStepDisplay::jmp_ret1 = NULL;
uintptr_t EnemyStepDisplay::jmp_ret2 = NULL;
uintptr_t EnemyStepDisplay::jmp_ret3 = NULL;
bool EnemyStepDisplay::jc_possible = false;
float EnemyStepDisplay::jc_possible_timer = 0.0f;
bool EnemyStepDisplay::showExtraStats = false;
ImVec2 EnemyStepDisplay::windowPos{ 0.0f, 0.0f };
static constexpr uintptr_t sUnit = 0xE552CC;
static constexpr uintptr_t sMediator = 0xE558B8;
static constexpr uintptr_t detour1_getEnemies = 0x402BD0;
static constexpr uintptr_t detour1_getCanJC = 0x4AB170;

bool EnemyStepDisplay::chart_enabled = false;
static constexpr int STEP_CHART_SIZE = 512;
static constexpr float stepIncFloat = 0.01f;
static float stepTimeline = 0.0f;

static constexpr ImColor steppableCol = {255, 215, 0, 255};
static constexpr ImColor hitCol = {220, 0, 0, 255};
static constexpr ImColor attemptCol = {60, 140, 255, 255};

struct StepAttemptEntry {
    float time;
    bool isAttempt;
};
struct StepChartEntry {
    float time;
    bool canStep;
};
struct StepDamageEntry {
    float time;
    bool canStepAtDamage;
    bool isAttempt;
};

static StepAttemptEntry stepAttemptChart[STEP_CHART_SIZE];
static int stepAttemptIndex = 0;
static StepChartEntry stepChart[STEP_CHART_SIZE];
static int stepChartIndex = 0;
static StepDamageEntry stepDamageChart[STEP_CHART_SIZE];
static int stepDamageIndex = 0;

static void update_step_chart_tick() {
    stepChart[stepChartIndex] = {stepTimeline, EnemyStepDisplay::jc_possible};
    stepChartIndex = (stepChartIndex + 1) % STEP_CHART_SIZE;
}

void EnemyStepDisplay::record_damage() {
    const StepChartEntry& snapshot = stepChart[(stepChartIndex + STEP_CHART_SIZE - 1) % STEP_CHART_SIZE];
    stepDamageChart[stepDamageIndex] = {snapshot.time, snapshot.canStep, true};
    stepDamageIndex = (stepDamageIndex + 1) % STEP_CHART_SIZE;
}

void EnemyStepDisplay::record_enemy_step_attempt() {
    const StepChartEntry& snapshot = stepChart[(stepChartIndex + STEP_CHART_SIZE - 1) % STEP_CHART_SIZE];
    stepAttemptChart[stepAttemptIndex] = {snapshot.time, true};
    stepAttemptIndex = (stepAttemptIndex + 1) % STEP_CHART_SIZE;
}

naked void call1(void) {
    _asm {
        pushad
        movzx ebx, byte ptr [EnemyStepDisplay::jc_possible]
        
        mov esi, [sUnit]
        mov esi, [esi]
        mov edx, [esi+0xD4] // get delta time
        test edx, edx
        je popcode
        sub esp, 4
        movss xmm0, [edx+0x10]
        movss [esp], xmm0 // Store delta time
        mov esi, [esi+0x194] // get enemies
        test esi, esi
        je cleanup
        jmp CheckCanJC

    GetNextEnemy:
        mov esi, [esi+0x8]
        test esi, esi
        je save_and_exit_false
    CheckCanJC:
        push esi // enemy arg
        call dword ptr [detour1_getCanJC] // call 0x4AB170
        
        mov cl, al
        cmp cl, 1
        jne GetNextEnemy
        
        mov esi, [sMediator]
        mov esi, [esi]
        test esi, esi
        je save_and_exit_false
        
        mov esi, [esi+0x24]
        test esi, esi
        je save_and_exit_false
        
        cmp dword ptr [esi+0x1FF4], 0 // has jc timer expired 
        jne save_and_exit_false

        cmp byte ptr [esi+0x2005], 0 // is enemy step banned
        je save_and_exit_false
        
        test dword ptr [esi+0x1554], 2
        je save_and_exit_false

        //mov esi, [esi+0x1e8c]
        //cmp dword ptr [esi+0x1c], 0 // groundedActual
        //jne save_and_exit_false
        
        cmp ebx, 0
        jne just_update_timer
        
        mov dword ptr [EnemyStepDisplay::jc_possible_timer], 0
        
    just_update_timer:
        movss xmm0, [EnemyStepDisplay::jc_possible_timer]
        movss xmm1, [esp]
        addss xmm0, xmm1
        movss [EnemyStepDisplay::jc_possible_timer], xmm0
        
        mov al, 1
        jmp save_and_exit
        
    save_and_exit_false:
        xor al, al
        
    save_and_exit:
        mov [EnemyStepDisplay::jc_possible], al
        
    cleanup:
        add esp, 4
        
    popcode:
        popad
        ret
    }
}

naked void detour1(void) {
    _asm {
        cmp byte ptr [EnemyStepDisplay::mod_enabled], 1
        je jcDisplay
        cmp byte ptr [EnemyStepDisplay::chart_enabled], 0
        je originalcode

    jcDisplay: // both mods depend on this
        pushad
        call EnemyStepDisplay::update_jc_possible
        popad

        cmp byte ptr [EnemyStepDisplay::chart_enabled], 0
        je originalcode

        sub esp, 4
        movss [esp], xmm0
        movss xmm0, [stepTimeline]
        addss xmm0, [stepIncFloat]
        movss [stepTimeline], xmm0
        movss [esp], xmm0
        pushad
        call update_step_chart_tick
        popad
        movss xmm0, [esp]
        add esp, 4

    originalcode:
        mov eax, [esi+0x000014F0]
        jmp dword ptr [EnemyStepDisplay::jmp_ret1]
    }
}

naked void detour2(void) { // called when damage happens
    _asm {
        cmp byte ptr [EnemyStepDisplay::chart_enabled], 1
        jne originalcode

        push ecx
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        test ecx, ecx
        je popcode
        mov ecx, [ecx+0x24]
        test ecx, ecx
        je popcode
        lea ecx, [ecx+0x15B4]
        cmp ecx, esi
        je popcode

        pushad
        call EnemyStepDisplay::record_damage
        popad

    popcode:
        pop ecx

    originalcode:
        movss xmm0, [esi+0x18]
        jmp dword ptr [EnemyStepDisplay::jmp_ret2]
    }
}

naked void detour3(void) { // called when player tries to enemy step
    _asm {
        cmp byte ptr [EnemyStepDisplay::chart_enabled], 1
        jne originalcode
        pushad
        call EnemyStepDisplay::record_enemy_step_attempt
        popad
    originalcode:
        mov esi, [edx+0x00000194]
        jmp dword ptr [EnemyStepDisplay::jmp_ret3]
    }
}

void EnemyStepDisplay::update_jc_possible() {
    __try {
        call1();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        jc_possible = false;
        jc_possible_timer = 0.0f;
    }
}

namespace {
struct EnemyStepExtraInfo {
    bool valid = false;
    bool enemy_step_enabled = false;
    bool grounded = false;
    float jc_timer = 0.0f;
};

uPlayer* get_enemy_step_player_safe() {
    __try {
        return devil4_sdk::get_local_player();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

EnemyStepExtraInfo read_enemy_step_extra_info(uPlayer* player) {
    EnemyStepExtraInfo info{};
    if (!player) {
        return info;
    }

    __try {
        info.enemy_step_enabled = player->enemyStepEnabled;
        info.grounded = player->collisionSettings && player->collisionSettings->mLand != 0;
        info.jc_timer = player->jcTimer;
        info.valid = true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return {};
    }

    return info;
}
}

void EnemyStepDisplay::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = get_enemy_step_player_safe();
        if (!player) { return; }
        static constexpr int WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Enemy Step Possible UI", NULL, WindowFlags);
        ImGui::SetWindowPos(windowPos, ImGuiCond_Once);
        windowPos = ImGui::GetWindowPos();
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::Checkbox("##Enemy Step Possible Checkbox", &jc_possible);
        if (showExtraStats) {
            const auto extra_info = read_enemy_step_extra_info(player);
            bool enemy_step_enabled = extra_info.enemy_step_enabled;
            bool grounded = extra_info.grounded;
            float jc_timer = extra_info.jc_timer;
            ImGui::SameLine();
            help_marker(_("Is enemy step possible?"));
            ImGui::SameLine();
            ImGui::Checkbox("##Enemy Step Enabled Checkbox", &enemy_step_enabled);
            ImGui::SameLine();
            help_marker(_("Is enemy step enabled?\n(Nero's Buster moves and possibly other things disable enemy step)"));
            ImGui::SameLine();
            ImGui::Checkbox("##Grounded", &grounded);
            ImGui::SameLine();
            help_marker(_("Is player grounded?"));
            ImGui::InputFloat("##Enemy Step Possible InputFloat", &jc_possible_timer);
            ImGui::SameLine();
            help_marker(_("Enemy step was possible for this number of 60fps frames"));
            ImGui::SliderFloat("##Jump Cooldown SliderFloat", &jc_timer, 0.0f, 20.0f);
            ImGui::SameLine();
            help_marker(_("Jump Cooldown"));
        }
        ImGui::PopItemWidth();
        ImGui::End();
    }

    if (chart_enabled) {
        if (uPlayer* player = devil4_sdk::get_local_player()) {
            if (sRender* sRen = devil4_sdk::get_sRender()) {
                Vector2f screen_res = sRen->screenRes;
                float uiScale = screen_res.y / 1080.0f;
                float panelWidth = screen_res.x;
                static const float timelineHeight  = 20.0f * uiScale;
                static const float hitExtendAmount = 10.0f * uiScale;
                static const float panelHeight = timelineHeight + (hitExtendAmount * 2.0f);
                float posX = (screen_res.x - panelWidth) * 0.5f;
                float posY = screen_res.y;

                ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
                ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("Enemy Step Timeline", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 origin = ImGui::GetCursorScreenPos();
                draw->AddRectFilled(origin, ImVec2(origin.x + panelWidth, origin.y + panelHeight), IM_COL32(20, 20, 20, 255));
                const float timelineTop = origin.y + hitExtendAmount;
                const float timelineBottom = timelineTop + timelineHeight;
                const float graphLeft = origin.x;
                const float graphWidth = panelWidth;
                const float timeWindow = 4.0f;
                float now = stepChart[(stepChartIndex + STEP_CHART_SIZE - 1) % STEP_CHART_SIZE].time;

                // enemy step possible
                for (int i = 0; i < STEP_CHART_SIZE; i++) {
                    const StepChartEntry& entry = stepChart[i];
                    if (entry.time <= 0.0f || !entry.canStep)
                        continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    draw->AddLine(ImVec2(x, timelineTop), ImVec2(x, timelineBottom), steppableCol, 2.0f * uiScale);
                }

                // damage
                for (int i = 0; i < STEP_CHART_SIZE; i++) {
                    const StepDamageEntry& entry = stepDamageChart[i];
                    if (!entry.isAttempt || entry.time <= 0.0f)
                        continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x        = graphLeft + normalizedTime * graphWidth;
                    ImU32 hitColor = hitCol;
                    draw->AddLine(ImVec2(x, timelineTop - hitExtendAmount), ImVec2(x, timelineBottom + hitExtendAmount), hitColor, 1.0f * uiScale);
                }

                // enemy step attempts
                for (int i = 0; i < STEP_CHART_SIZE; i++) {
                    const StepAttemptEntry& entry = stepAttemptChart[i];
                    if (!entry.isAttempt || entry.time <= 0.0f)
                        continue;
                    float normalizedTime = (entry.time - (now - timeWindow)) / timeWindow;
                    if (normalizedTime < 0.0f || normalizedTime > 1.0f)
                        continue;
                    float x = graphLeft + normalizedTime * graphWidth;
                    draw->AddLine(
                        ImVec2(x, timelineTop - hitExtendAmount), ImVec2(x, timelineBottom + hitExtendAmount), attemptCol, 1.0f * uiScale);
                }
                ImGui::PopStyleVar();
                ImGui::End();
            }
        }
    }
}

void EnemyStepDisplay::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::BeginGroup();
        ImGui::Checkbox(_("Enemy Step Display"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("See if it was possible to enemy step in that combo after all"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            ImGui::Checkbox(_("Show Extra Info"), &showExtraStats);
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Enemy Step Chart"), &chart_enabled);
    }
}

std::optional<std::string> EnemyStepDisplay::on_initialize() {
    if (!install_hook_offset(0x4035A9, hook1, &detour1, &EnemyStepDisplay::jmp_ret1, 6)) {
        spdlog::error("Failed to init EnemyStepDisplay mod 1\n");
        return "Failed to init EnemyStepDisplay mod 1";
    }

    if (!install_hook_offset(0x11BFD4, hook2, &detour2, &EnemyStepDisplay::jmp_ret2, 5)) {
        spdlog::error("Failed to init EnemyStepDisplay mod 2\n");
        return "Failed to init EnemyStepDisplay mod 2";
    }

    if (!install_hook_offset(0x4049FE, hook3, &detour3, &EnemyStepDisplay::jmp_ret3, 6)) {
        spdlog::error("Failed to init EnemyStepDisplay mod 3\n");
        return "Failed to init EnemyStepDisplay mod 3";
    }

    return Mod::on_initialize();
}

void EnemyStepDisplay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_step_display").value_or(false);
    chart_enabled  = cfg.get<bool>("enemy_step_chart").value_or(false);
    showExtraStats = cfg.get<bool>("enemy_step_display_extra").value_or(false);
    windowPos.x = cfg.get<float>("enemy_step_display_pos_x").value_or(0.0f);
    windowPos.y = cfg.get<float>("enemy_step_display_pos_y").value_or(0.0f);
}

void EnemyStepDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_step_display", mod_enabled);
    cfg.set<bool>("enemy_step_chart", chart_enabled);
    cfg.set<bool>("enemy_step_display_extra", showExtraStats);
    cfg.set<float>("enemy_step_display_pos_x", windowPos.x);
    cfg.set<float>("enemy_step_display_pos_y", windowPos.y);
}

#endif
