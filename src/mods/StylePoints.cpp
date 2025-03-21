#include "StylePoints.hpp"

bool StylePoints::mod_enabled = false;
uintptr_t StylePoints::jmp_ret1 = NULL;

struct TrickScore {
    std::string text;
    float score = 0.0f;
    float multiplier = 1.0f;
    int styleLetter = 0;
    std::chrono::time_point<std::chrono::steady_clock> timePerformed = std::chrono::steady_clock::now();
};

std::vector<TrickScore> trickScores;
const float displayDuration = 1.0f;
const int maxScores = 5;

const char* GetStyleChar(int styleNum) {
    switch (styleNum) {
        case 0: return "E";
        case 1: return "D";
        case 2: return "C";
        case 3: return "B";
        case 4: return "A";
        case 5: return "S";
        case 6: return "SS";
        case 7: return "SSS";
        default: return "";
    }
}

Vector3f GetStyleColor(int styleNum) {
    switch (styleNum) {
        case 0: return { 0.0f, 0.0f, 0.0f };
        case 1: return { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f }; // D
        case 2: return {  53 / 255.0f, 203 / 255.0f, 244 / 255.0f }; // C
        case 3: return { 213 / 255.0f,  92 / 255.0f,  15 / 255.0f }; // B
        case 4: return { 245 / 255.0f,  68 / 255.0f,  46 / 255.0f }; // A
        case 5: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f }; // S
        case 6: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f }; // SS
        case 7: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f }; // SSS
        default: return { 0.0f, 0.0f, 0.0f };
    }
}

static void DrawTrickScores() {
    if (devil4_sdk::get_local_player()) {
        ImGui::SetNextWindowPos(ImVec2(devil4_sdk::get_sRender()->xRes * 0.6f, devil4_sdk::get_sRender()->yRes * 0.4f)); // 0.73 for centred (and 0.2 size)
        ImGui::SetNextWindowSize(ImVec2(devil4_sdk::get_sRender()->xRes * 0.3f, devil4_sdk::get_sRender()->yRes * 0.3f));
        ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::SetWindowFontScale(2.0f);
        float fontSize = ImGui::GetFontSize();
        auto now = std::chrono::steady_clock::now();

        for (uint16_t i = 0; i < trickScores.size(); ++i) {
            TrickScore& score = trickScores[i];
            float elapsed = std::chrono::duration<float>(now - score.timePerformed).count();

            if (elapsed > displayDuration) {
                trickScores.erase(trickScores.begin() + i);
                --i;
                continue;
            }

            float fade = 1.0f - (elapsed / displayDuration);
            ImVec4 color(1.0f, 1.0f, 1.0f, fade);
            ImVec4 color2(GetStyleColor(score.styleLetter).r,
                GetStyleColor(score.styleLetter).g,
                GetStyleColor(score.styleLetter).b,
                fade
            );

            if (score.styleLetter != 0) {
                float windowWidth = ImGui::GetContentRegionAvail().x;
                char scoreBuffer[32];
                snprintf(scoreBuffer, sizeof(scoreBuffer), "%.1f", score.score * 0.1 * score.multiplier);

                float textWidth = ImGui::CalcTextSize(score.text.c_str()).x;
                float scoreWidth = ImGui::CalcTextSize(scoreBuffer).x;
                float customSpacing = fontSize * 1.0f;
                float rightAlignX = (windowWidth * 0.8f) - (customSpacing * 0.5f);
                float leftAlignX = (windowWidth * 0.8f) + (customSpacing * 0.5f);

                ImGui::SetCursorPosX(rightAlignX - textWidth);
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::Text("%s", score.text.c_str());
                ImGui::PopStyleColor();
                ImGui::SameLine(leftAlignX);
                ImGui::PushStyleColor(ImGuiCol_Text, color2);
                ImGui::Text("%.1f", score.score * 0.1 * score.multiplier);
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
    }
}

static void AddTrickScore(const char* text, float score, float multiplier, int styleLetter) {
    TrickScore newScore;
    newScore.text = text;
    newScore.score = score;
    newScore.multiplier = multiplier;
    newScore.styleLetter = styleLetter;
    newScore.timePerformed = std::chrono::steady_clock::now();
    if (trickScores.size() >= maxScores) {
        trickScores.pop_back();
    }
    trickScores.insert(trickScores.begin(), newScore);
}

static float xmm0backup = 0.0f;
static float xmm1backup = 0.0f;
naked void detour1(void) {
    _asm {
        cmp byte ptr [StylePoints::mod_enabled], 1
        jne originalcode

        movss [xmm0backup], xmm0
        movss [xmm1backup], xmm1

        push eax // 0x4
        push ebx // 0x8
        push ecx // 0xC
        push edx // 0x10
        push esi // 0x14
        push edi // 0x18

        // backup pushes, 2 args
        push [esp+0x18+0xC] // style letter
        sub esp, 0x4
        movss [esp], xmm2 // multiplier
        push [esp+0x18+0x8+0x4] // score
        push ebx // text
        call AddTrickScore // fucks eax, ecx, edx
        add esp,0x10 // 4 args

        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax

        movss xmm0, [xmm0backup]
        movss xmm1, [xmm1backup]

        originalcode:
        comiss xmm0,xmm1
        movss [edx+0x04], xmm0
        jmp dword ptr [StylePoints::jmp_ret1]
    }
}

std::optional<std::string> StylePoints::on_initialize() {
    if (!install_hook_offset(0x5480F, hook1, &detour1, &jmp_ret1, 5)) {
		spdlog::error("Failed to init StylePoints mod\n");
		return "Failed to init StylePoints mod";
	}
    return Mod::on_initialize();
}

void StylePoints::on_gui_frame() {
    ImGui::Checkbox("Style Point Display", &mod_enabled);
    /*if (ImGui::Button("Trigger Action UI")) {
        AddTrickScore("Rave 1", 50.0f, 1.0f, 5);
    }*/
}

void StylePoints::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        DrawTrickScores();
    }
}

void StylePoints::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("style_points_display").value_or(false);
}

void StylePoints::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("style_points_display", mod_enabled);
}
