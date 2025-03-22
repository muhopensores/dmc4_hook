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

static const float displayDuration = 1.0f;
static const uint32_t maxScores = 5;

static const uint32_t maxTonyScores = UINT32_MAX;
static const int maxPerRow = 7;
static const int maxRows = 5;
static float comboScore = 0.0f;
static std::chrono::steady_clock::time_point lastTrickTime = std::chrono::steady_clock::now();

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
    case 0: return { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f, 0.0f }; // E, same as D
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

std::unordered_map<std::string, std::string> textLookupTable = {
    // trickster
    {"TS-ehsuc",        "Mustang"},
    // rebellion
    {"REBE-combo303",   "Reb A1"},
    {"REBE-combo304",   "Reb A2"},
    {"REBE-combo305",   "Reb A3"},
    {"REBE-stings",     "Stinger"},
    {"REBE-sting",      "Stinger Hit"},
    {"REBE-stab",       "Million Stab"},
    {"REBE-hightime",   "High Time"},
    {"REBE-combo306-1", "Reb B1-1"},
    {"REBE-combo306-2", "Reb B1-2"},
    {"REBE-combo306-3", "Reb B1-3"},
    {"REBE-combo306-1", "Reb B1-1"},
    {"REBE-stabf",      "Reb B2"},
    {"REBE-helmbrkLV1", "Low Helm Breaker"},
    {"REBE-helmbrkLV2", "Medium Helm Breaker"},
    {"REBE-helmbrkLV3", "High Helm Breaker"},
    {"REBE-helmbrkf",   "Helm Breaker Landing"},
    {"REBE-DTstg1",     "DT Stinger"},
    {"REBE-DTstg2",     "DT Stinger"},
    {"REBE-DTstg3",     "DT Stinger"},
    {"REBE-DTstg4",     "DT Stinger"},
    {"REBE-DTstg5",     "DT Stinger"},
    // rebellion swordmaster
    {"REBE-props1",     "Prop"},
    {"REBE-props20",    "Shredder"},
    {"REBE-props21",    "Shredder"},
    {"REBE-dm363",      "Macabre 1"},
    {"REBE-dm365",      "Macabre 2"},
    {"REBE-dm366",      "Macabre 3"},
    {"REBE-dm367",      "Macabre 4"},
    {"REBE-dm368-1",    "Macabre 5-1"},
    {"REBE-dm368-2",    "Macabre 5-2"},
    {"REBE-dm368-3",    "Macabre 5-3"},
    {"REBE-dm369",      "Macabre Stab"},
    {"REBE-dm370",      "Macabre 7"},
    {"REBE-dm371",      "Macabre 8"},
    {"REBE-dm372-1",    "Macabre 9"},
    {"REBE-dm364",      "Macabre End"},
    {"REBE-arial353",   "Rave 1"},
    {"REBE-arial354",   "Rave 2"},
    {"REBE-arial355",   "Rave 3"},
    {"REBE-arial356",   "Rave 4"},
    // gilgamesh
    {"GIL-klbe",        "Full House"},
    // gilgamesh swordmaster

    // lucifer
    {"NORMAL",          "Embed"}, // pins and pandora's normal shot
    {"Bomb",            "Pin Explosion"},
    {"ROSE",            "Rose"},
    {"LUCI-combo514-1", "Splash"},
    {"LUCI-combo514-2", "Splash"},
    {"LUCI-combo514-3", "Splash"},
    // lucifer swordmaster

    // e&i
    {"SHL000",          "E&I Shot"},
    {"SHL00-LEN2",      "E&I Shot"},
    {"SHL00_LEN3",      "E&I Shot"},
    {"CHARGE",          "Charged E&I Shot"},
    {"CHARGE_LEN2",     "Charged E&I Shot"},
    {"CHARGE_LEN3",     "Charged E&I Shot"},

    // e&i gunslinger
    {"RAIN_STORM",      "Rainstorm"},
    {"RAIN_STORM_LEN2", "Rainstorm"},
    {"RAIN_STORM_LEN3", "Rainstorm"},
    {"DT_RAIN_STORM",   "DT Rainstorm"},
    {"DT_RAIN_STORM_L", "DT Rainstorm"},

    // coyote
    {"NRML_ETC_LV1",    "Coyote Pellet"},
    {"NRML_CEN_LV1",    "Coyote Pellet"},
    {"NRML_ETC_LV2",    "Coyote Pellet"},
    {"NRML_CEN_LV2",    "Coyote Pellet"},
    {"NRML_ETC_LV3",    "Coyote Pellet"},
    {"NRML_CEN_LV3",    "Coyote Pellet"},
    // coyote gunslinger

    // pandora
    {"MACHINEGUN",      "Gatling Shot"},
    {"MACHINEGUN_LV2",  "Gatling Shot"},
    {"MACHINEGUN_LV3",  "Gatling Shot"},
    {"DT_MGUN",         "Gatling Shot"},
    {"DT_MGUN_LV2",     "Gatling Shot"},
    {"DT_MGUN_LV3",     "Gatling Shot"},
    // pandora gunslinger

    // yamato
    {"YMT-combo1006-1", "Yamato Rave 1"},
    {"YMT-combo1006-2", "Yamato Rave 1"},
    {"YMT-combo1007",   "Yamato Rave 2"},
};

bool newTrickAdded() {
    static size_t lastSize = 0;
    size_t currentSize = trickScores.size();

    if (currentSize != lastSize) {
        lastSize = currentSize;
        return true;
    }
    return false;
}

// update names on visual thread rather than have the game wait on lookups i think this is better
static void UpdateTrickScores() {
    size_t previousSize = trickScores.size();
    if (newTrickAdded()) {
        for (auto& score : trickScores) { // can't get just the most recent because it might change mid call
            std::string scoreText = score.text;
            auto it = textLookupTable.find(scoreText);
            if (it != textLookupTable.end()) {
                score.text = it->second.c_str();
            }
        }
    }
}

static void DrawTrickScores() {
    if (devil4_sdk::get_local_player()) {
        UpdateTrickScores();
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

static void DrawTonyScores() {
    auto* player = devil4_sdk::get_local_player();
    if (!player) return;

    UpdateTrickScores();
    auto now = std::chrono::steady_clock::now();
    
    float elapsedSinceLastTrick = std::chrono::duration<float>(now - lastTrickTime).count();
    bool turbo = devil4_sdk::get_sMediator()->turboEnabled;
    float speedMultiplier = turbo ? 1.2f : 1.0f; // why is true first its 0 1 ffs
    
    float fade = 1.0f - (elapsedSinceLastTrick * speedMultiplier);

    if (fade <= 0.0f) {
        trickScores.clear();
        comboScore = 0.0f;
        return;
    }

    if (trickScores.empty()) return;
    ImGui::SetNextWindowPos(ImVec2(devil4_sdk::get_sRender()->xRes * 0.0f, devil4_sdk::get_sRender()->yRes * 0.8f));
    ImGui::SetNextWindowSize(ImVec2(devil4_sdk::get_sRender()->xRes * 1.0f, devil4_sdk::get_sRender()->yRes * 0.2f));
    ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    float shakeAmount = 0.0f;
    if (fade > 0.9f) {
        shakeAmount = dis(gen);
    }

    float scoreFontBaseScale = 2.0f;
    float trickFontBaseScale = 1.5f;

    float scoreFontScale = scoreFontBaseScale + shakeAmount * 0.1f;
    float trickFontScale = trickFontBaseScale; // + shakeAmount * 0.1f;

    ImGui::SetWindowFontScale(scoreFontScale);

    std::vector<std::string> trickLines[maxRows];
    std::vector<std::pair<std::string, int>> orderedTricks;

    std::string lastTrick = "";
    int repeatCount = 1;
    comboScore = 0.0f;
    TrickScore lastScore;

    for (const TrickScore& score : trickScores) {
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

    int rowIndex = 0, countInRow = 0;
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

    ImVec4 scoreColor(GetStyleColor(lastScore.styleLetter).x,
                      GetStyleColor(lastScore.styleLetter).y,
                      GetStyleColor(lastScore.styleLetter).z,
                      fade);

    char scoreText[32];
    snprintf(scoreText, sizeof(scoreText), "%.1f", comboScore);
    float scoreWidth = ImGui::CalcTextSize(scoreText).x;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    float scoreShakeAmount = shakeAmount * 1.0f;
    ImVec2 scorePos = ImVec2((windowWidth - scoreWidth) * 0.5f + scoreShakeAmount, 0.0f + scoreShakeAmount);
    ImGui::SetCursorPos(scorePos);

    ImGui::PushStyleColor(ImGuiCol_Text, scoreColor);
    ImGui::Text("%s", scoreText);
    ImGui::PopStyleColor();

    ImGui::SetWindowFontScale(trickFontScale);

    float trickShakeAmount = shakeAmount * 5.0f;
    float rowStartY = ImGui::GetFontSize() * 1.5f;
    for (int row = 0; row < maxRows; ++row) {
        if (trickLines[row].empty()) continue;

        std::string rowText;
        for (size_t i = 0; i < trickLines[row].size(); ++i) {
            rowText += trickLines[row][i];
            if (i < trickLines[row].size() - 1) rowText += " + ";
        }

        float textWidth = ImGui::CalcTextSize(rowText.c_str()).x;
        ImVec2 trickPos = ImVec2((windowWidth - textWidth) * 0.5f + trickShakeAmount, rowStartY + row * ImGui::GetFontSize() * 1.0f + trickShakeAmount);
        ImGui::SetCursorPos(trickPos);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, fade));
        ImGui::Text("%s", rowText.c_str());
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

static void AddTrickScore(const char* text, float score, float multiplier, int styleLetter) {
    TrickScore newScore;
    newScore.text = text;
    newScore.score = score;
    newScore.multiplier = multiplier;
    newScore.styleLetter = styleLetter;
    newScore.timePerformed = std::chrono::steady_clock::now();

    if (StylePoints::tonyHawk) {
        lastTrickTime = newScore.timePerformed;
        if (trickScores.size() >= maxTonyScores) {
            trickScores.erase(trickScores.begin());
        }
        trickScores.push_back(newScore);
    } else {
        if (trickScores.size() >= maxScores) {
            trickScores.pop_back();
        }
        trickScores.insert(trickScores.begin(), newScore);
    }
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
