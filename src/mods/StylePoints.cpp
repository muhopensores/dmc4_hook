#include "StylePoints.hpp"

bool StylePoints::mod_enabled = false;
bool StylePoints::tonyHawk = false;
uintptr_t StylePoints::jmp_ret1 = NULL;

struct TrickScore {
    std::string text;
    float score = 0.0f;
    float multiplier = 0.0f;
    int styleLetter = 0;
    std::chrono::time_point<std::chrono::steady_clock> timePerformed = std::chrono::steady_clock::now();
};

std::vector<TrickScore> trickScores;
const float displayDuration = 1.0f;
static uint32_t maxScores = 5;

static uint32_t maxTonyScores = UINT32_MAX;
static const int maxPerRow = 7;
static const int maxRows = 5;
static float comboScore = 0.0f;
static bool comboActive = false;
static std::chrono::steady_clock::time_point comboStartTime;
static std::chrono::time_point<std::chrono::steady_clock> comboEndTime;

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

ImVec4 GetStyleColor(int styleNum) {
    switch (styleNum) {
    case 0: return { 0.0f, 0.0f, 0.0f, 0.0f };
    case 1: return { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f, 0.0f }; // D
    case 2: return {  53 / 255.0f, 203 / 255.0f, 244 / 255.0f, 0.0f }; // C
    case 3: return { 213 / 255.0f,  92 / 255.0f,  15 / 255.0f, 0.0f }; // B
    case 4: return { 245 / 255.0f,  68 / 255.0f,  46 / 255.0f, 0.0f }; // A
    case 5: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // S
    case 6: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // SS
    case 7: return { 234 / 255.0f,  56 / 255.0f,  58 / 255.0f, 0.0f }; // SSS
    default: return { 0.0f, 0.0f, 0.0f, 0.0f };
    }
}

static void DrawTonyScores() {
    auto* player = devil4_sdk::get_local_player();
    if (!player) return;

    auto now = std::chrono::steady_clock::now();

    if (comboActive) {
        float elapsedSinceLastTrick = std::chrono::duration<float>(now - comboStartTime).count();
        if (elapsedSinceLastTrick > displayDuration) {
            comboEndTime = now;
            comboActive = false;
            
            trickScores.clear();
            comboScore = 0.0f;
        }
    }

    if (trickScores.empty()) return;

    ImGui::SetNextWindowPos(ImVec2(devil4_sdk::get_sRender()->xRes * 0.0f, devil4_sdk::get_sRender()->yRes * 0.8f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(devil4_sdk::get_sRender()->xRes * 1.0f, devil4_sdk::get_sRender()->yRes * 0.2f));
    ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(1.5f);

    std::vector<std::string> trickLines[maxRows];
    std::vector<std::pair<std::string, int>> orderedTricks;

    std::string lastTrick = "";
    int repeatCount = 1;
    comboScore = 0.0f;
    TrickScore lastScore;

    for (size_t i = 0; i < trickScores.size(); ++i) {
        const TrickScore& score = trickScores[i];
        comboScore += score.score * 0.1f * score.multiplier;
        if (score.text == lastTrick) {
            repeatCount++;
        } else {
            if (!lastTrick.empty()) {
                orderedTricks.emplace_back(lastTrick, repeatCount);
            }
            lastTrick = score.text;
            repeatCount = 1;
            lastScore = score;
        }
    }

    if (!lastTrick.empty()) {
        orderedTricks.emplace_back(lastTrick, repeatCount);
    }

    int rowIndex = 0;
    int countInRow = 0;
    for (const auto& trickPair : orderedTricks) {
        std::string displayText = trickPair.first;
        if (trickPair.second > 1) {
            displayText += " (x" + std::to_string(trickPair.second) + ")";
        }

        if (countInRow >= maxPerRow) {
            rowIndex++;
            countInRow = 0;
        }
        if (rowIndex >= maxRows) break;

        trickLines[rowIndex].push_back(displayText);
        countInRow++;
    }

    float fade = 1.0f;
    if (!comboActive) {
        float elapsedSinceLanding = std::chrono::duration<float>(now - comboEndTime).count();
        fade = 1.0f - (elapsedSinceLanding / displayDuration);
        fade = std::max(fade, 0.0f);
    } else {
        float elapsedSinceStart = std::chrono::duration<float>(now - comboStartTime).count();
        fade = 1.0f - (elapsedSinceStart / displayDuration);
        fade = std::max(fade, 0.0f);
    }

    ImVec4 scoreColor(GetStyleColor(lastScore.styleLetter).x,
                      GetStyleColor(lastScore.styleLetter).y,
                      GetStyleColor(lastScore.styleLetter).z,
                      fade);

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%.1f", comboScore);
    float scoreWidth = ImGui::CalcTextSize(scoreText).x;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Text, scoreColor);
    ImGui::SetCursorPosX((windowWidth - scoreWidth) * 0.5f);
    ImGui::Text("%s", scoreText);
    ImGui::PopStyleColor();

    for (int row = 0; row < maxRows; ++row) {
        if (trickLines[row].empty()) continue;

        std::string rowText = "";
        for (size_t i = 0; i < trickLines[row].size(); ++i) {
            rowText += trickLines[row][i];
            if (i < trickLines[row].size() - 1) rowText += " + ";
        }

        float textWidth = ImGui::CalcTextSize(rowText.c_str()).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, fade));
        ImGui::Text("%s", rowText.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

static void DrawTrickScores() {
    if (devil4_sdk::get_local_player()) {
        ImGui::SetNextWindowPos(ImVec2(devil4_sdk::get_sRender()->xRes * 0.6f, devil4_sdk::get_sRender()->yRes * 0.4f));
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
            ImVec4 color2(GetStyleColor(score.styleLetter).x,
                GetStyleColor(score.styleLetter).y,
                GetStyleColor(score.styleLetter).z,
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

static void AddTonyScore(const char* text, float score, float multiplier, int styleLetter) {
    TrickScore newScore;
    newScore.text = text;
    newScore.score = score;
    newScore.multiplier = multiplier;
    newScore.styleLetter = styleLetter;
    newScore.timePerformed = std::chrono::steady_clock::now();

    if (!comboActive) {
        comboScore = 0.0f;
        comboActive = true;
        comboStartTime = newScore.timePerformed;
    }

    comboStartTime = newScore.timePerformed;

    if (trickScores.size() >= maxTonyScores) {
        trickScores.erase(trickScores.begin());
    }

    trickScores.push_back(newScore);
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
        cmp byte ptr [StylePoints::tonyHawk], 1
        je callTony
        call AddTrickScore // fucks eax, ecx, edx
        jmp aftercall

        callTony:
        call AddTonyScore
        aftercall:
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
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox("Tony", &tonyHawk);
        ImGui::Unindent();
    }
    /*if (ImGui::Button("Trigger Action UI")) {
        AddTrickScore("Rave 1", 50.0f, 1.0f, 5);
    }*/
}

void StylePoints::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        if (tonyHawk) DrawTonyScores();
        else DrawTrickScores();
    }
}

void StylePoints::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("style_points_display").value_or(false);
    tonyHawk = cfg.get<bool>("hawk_points_display").value_or(false);
}

void StylePoints::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("style_points_display", mod_enabled);
    cfg.set<bool>("hawk_points_display", tonyHawk);
}
