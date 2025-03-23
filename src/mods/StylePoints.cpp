#include "StylePoints.hpp"

bool StylePoints::mod_enabled = false;
bool StylePoints::tonyHawk = false;
uintptr_t StylePoints::jmp_ret1 = NULL;

struct TrickScore {
    std::string text;
    bool isAlreadyRenamed = false;
    float score = 0.0f;
    float multiplier = 0.0f;
    int styleLetter = 0;
    std::chrono::time_point<std::chrono::steady_clock> timePerformed = std::chrono::steady_clock::now();
};

std::vector<TrickScore> trickScores;

static const float displayDuration = 1.0f;
static const uint32_t maxScores = 5;

// trick recognition
static const uint32_t maxTonyScores = UINT32_MAX;
static const int maxPerRow = 7;
static const int maxRows = 5;
static float comboScore = 0.0f;
static std::chrono::steady_clock::time_point lastTrickTime = std::chrono::steady_clock::now();

// combo recognition
static std::string lastProcessedTrick = "";
static const float trickDisplayDuration = 2.0f;
static std::string detectedCombo = "";
static float comboPopupFade = 0.0f;
static bool comboMatched = false;
static std::chrono::steady_clock::time_point lastMatchTime = std::chrono::steady_clock::now();

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
    {"REBE-combo303",   "Rebellion Combo A"},
    {"REBE-combo304",   "Rebellion Combo A"},
    {"REBE-combo305",   "Rebellion Combo A"},
    {"REBE-stings",     "Stinger"},
    {"REBE-sting",      "Stinger Hit"},
    {"REBE-stab",       "Million Stab"},
	{"REBE-roundt",     "Round Trip"},
    {"REBE-hightime",   "High Time"},
    {"REBE-combo306-1", "Rebellion Combo B"},
    {"REBE-combo306-2", "Rebellion Combo B"},
    {"REBE-combo306-3", "Rebellion Combo B"},
    {"REBE-combo306-1", "Rebellion Combo B"},
    {"REBE-stabf",      "Million Stab End"},
    {"REBE-helmbrkLV1", "Helm Breaker Lv.1"},
    {"REBE-helmbrkLV2", "Helm Breaker Lv.2"},
    {"REBE-helmbrkLV3", "Helm Breaker Lv.3"},
    {"REBE-helmbrkf",   "Helm Breaker"},
    {"REBE-DTstg1",     "DT Stinger"},
    {"REBE-DTstg2",     "DT Stinger"},
    {"REBE-DTstg3",     "DT Stinger"},
    {"REBE-DTstg4",     "DT Stinger"},
    {"REBE-DTstg5",     "DT Stinger"},
    // rebellion swordmaster
    {"REBE-props1",     "Prop"},
    {"REBE-props20",    "Shredder"},
    {"REBE-props21",    "Shredder"},
    {"REBE-dm363",      "Dance Macabre"},
    {"REBE-dm365",      "Dance Macabre"},
    {"REBE-dm366",      "Dance Macabre"},
    {"REBE-dm367",      "Dance Macabre"},
    {"REBE-dm368-1",    "Dance Macabre"},
    {"REBE-dm368-2",    "Dance Macabre"},
    {"REBE-dm368-3",    "Dance Macabre"},
    {"REBE-dm369",      "Dance Macabre"},
    {"REBE-dm370",      "Dance Macabre"},
    {"REBE-dm371",      "Dance Macabre"},
    {"REBE-dm372-1",    "Dance Macabre"},
    {"REBE-dm364",      "Dance Macabre"},
    {"REBE-arial353",   "Aerial Rave"},
    {"REBE-arial354",   "Aerial Rave"},
    {"REBE-arial355",   "Aerial Rave"},
    {"REBE-arial356",   "Aerial Rave"},
	{"REBE-drivesw1",	"Drive"},
	{"REBE-drive1",		"Drive"},
	{"REBE-drive1-2",	"Drive Lv.2"},
	{"REBE-drive1-3",	"Drive Lv.3"},
	{"REBE-drivesw2",	"Overdrive"},
	{"REBE-drive2",		"Overdrive"},
	{"REBE-drive2-2",	"Overdrive Lv.2"},
	{"REBE-drive2-3",	"Overdrive Lv.3"},
	{"REBE-drivesw3",	"Drive"},
	{"REBE-drive3",		"Overdrive"},
	{"REBE-drive3-2",	"Overdrive Lv.2"},
	{"REBE-drive3-3",	"Overdrive Lv.3"},
	{"REBE-qdrive",		"Quick Drive"},
    // gilgamesh
	{"GIL-rblaze",      "Flush"},
	{"GIL-rblaze-f",    "Flush"},
	{"GIL-rblaze-b",    "Flush"},
    {"GIL-klbe",        "Full House"},
	{"GIL-combo403",    "Gilgamesh Combo A"},
	{"GIL-combo404",    "Gilgamesh Combo A"},
	{"GIL-combo405",    "Gilgamesh Combo A"},
	{"GIL-combo406",    "Gilgamesh Combo A"},
	{"GIL-combo407",    "Gilgamesh Combo B"},
	{"GIL-combo409-1",  "Gilgamesh Combo B"},
	{"GIL-combo409-2",  "Gilgamesh Combo B"},
	{"GIL-straight422", "Straight"},
	{"GIL-straight2",   "Straight Lv.2"},
	{"GIL-straight3",   "Straight Lv.3"},
	{"GIL-k13r-01",     "Kick 13"},
	{"GIL-k13r-02",     "Kick 13"},
	{"GIL-k13r-03",     "Kick 13"},
	{"GIL-k13r-04",     "Kick 13"},
	{"GIL-k13r-05",     "Kick 13"},
	{"GIL-k13rDT-01",   "Kick 13 DT"},
	{"GIL-k13rDT-02",   "Kick 13 DT"},
	{"GIL-k13rDT-03",   "Kick 13 DT"},
	{"GIL-k13rDT-04",   "Kick 13 DT"},
	{"GIL-k13rDT-05",   "Kick 13 DT"},
	{"GIL-k13rDT-06",   "Kick 13 DT"},
    // gilgamesh swordmaster
	{"GIL-mgdv450",  	"Beast Uppercut"},
	{"GIL-rgdg460",  	"Rising Dragon"},
	{"GIL-dvdg461",  	"Divine Dragon"},
	{"GIL-dvdg462",  	"Divine Dragon"},
	{"GIL-dvdg462",  	"Divine Dragon"},
	{"GIL-rlimpct-01",  "Real Impact"},
	{"GIL-rlimpct-02",  "Real Impact"},
	{"GIL-rlimpct-03",  "Real Impact"},
	{"GIL-inferno451",  "Shock"},
	{"GIL-inferno470",  "Shock Lv.1"},
	{"GIL-inferno471",  "Shock Lv.2"},

    // lucifer
    {"NORMAL",          "Embed"}, // pins and pandora's normal shot
    {"Bomb",            "Pin Explosion"},
    {"ROSE",            "Rose"},
	{"LUCI-combo503",   "Lucifer A1"},
	{"LUCI-combo504",   "Lucifer A2"},
	{"LUCI-combo505",   "Lucifer A3"},
	{"LUCI-combo506",   "Lucifer A4"},
	{"LUCI-combo506-2", "Lucifer A4"},
	{"LUCI-combo519-1", "Lucifer B1"},
	{"LUCI-combo519-2", "Lucifer B1"},
	{"LUCI-combo508", 	"Lucifer E1"},
	{"LUCI-combo509", 	"Lucifer E2"},
	{"LUCI-combo510-1", "Lucifer E3"},
	{"LUCI-combo510-2", "Lucifer E3"},
    {"LUCI-combo514-1", "Splash"},
    {"LUCI-combo514-2", "Splash"},
    {"LUCI-combo514-3", "Splash"},
    // lucifer swordmaster
	{"funnel", 			"Discipline & Bondage"},
	{"Stand",  			"Climax"},
	{"BombBariier",  	"Climax"},


    // e&i
    {"SHL000",          "E&I"},
    {"SHL00-LEN2",      "E&I"},
    {"SHL00_LEN3",      "E&I"},
    {"CHARGE",          "Charged E&I"},
    {"CHARGE_LEN2",     "Charged E&I"},
    {"CHARGE_LEN3",     "Charged E&I"},

    // e&i gunslinger
    {"RAIN_STORM",      "Rainstorm"},
    {"RAIN_STORM_LEN2", "Rainstorm"},
    {"RAIN_STORM_LEN3", "Rainstorm"},
    {"DT_RAIN_STORM",   "Rainstorm DT"},
    {"DT_RAIN_STORM_L", "Rainstorm DT"},
	{"TWO_SOMETIME",    "Twosome Time"},
    {"TWO_SOMETIME_L2", "Twosome Time"},
    {"TWO_SOMETIME_L3", "Twosome Time"},
	{"DT_TWO_SOMETIME", "Twosome Time DT"},

    // coyote
    {"NRML_ETC_LV1",    "Coyote"},
    {"NRML_CEN_LV1",    "Coyote"},
    {"NRML_ETC_LV2",    "Coyote"},
    {"NRML_CEN_LV2",    "Coyote"},
    {"NRML_ETC_LV3",    "Coyote"},
    {"NRML_CEN_LV3",    "Coyote"},
	{"CHRG_ETC_LV1",    "Coyote DT"},
    {"CHRG_CEN_LV1",    "Coyote DT"},
    {"CHRG_ETC_LV2",    "Coyote DT"},
    {"CHRG_CEN_LV2",    "Coyote DT"},
    {"CHRG_ETC_LV3",    "Coyote DT"},
    {"CHRG_CEN_LV3",    "Coyote DT"},

    // coyote gunslinger
	{"BACK_ETC_LV1",    "Backslide"},
	{"BACK_CEN_LV1",    "Backslide"},
	{"BACK_ETC_LV2",    "Backslide"},
	{"BACK_CEN_LV2",    "Backslide"},
	{"BACK_ETC_LV3",    "Backslide"},
	{"BACK_CEN_LV3",    "Backslide"},
	{"NCHA_STR_LVL1",   "Fireworks"},
	{"NCHA_STR_LVL2",   "Fireworks"},
	{"NCHA_STR_LVL3",   "Fireworks"},
	{"NCHA_WEK_LVL1",   "Fireworks"},
	{"NCHA_WEK_LVL2",   "Fireworks"},
	{"NCHA_WEK_LVL3",   "Fireworks"},
	{"STG-gunstinger",  "Gun Stinger"},
	{"STNG_ETC_LV1",    "Gun Stinger"},
	{"STNG_CEN_LV1",    "Gun Stinger"},
	{"STNG_ETC_LV2",    "Gun Stinger"},
	{"STNG_CEN_LV2",    "Gun Stinger"},
	{"STNG_ETC_LV3",    "Gun Stinger"},
	{"STNG_CEN_LV3",    "Gun Stinger"},
	{"DTST_ETC_LV1",    "Gun Stinger DT"},
	{"DTST_CEN_LV1",    "Gun Stinger DT"},
	{"DTST_ETC_LV2",    "Gun Stinger DT"},
	{"DTST_CEN_LV2",    "Gun Stinger DT"},
	{"DTST_ETC_LV3",    "Gun Stinger DT"},
	{"DTST_CEN_LV3",    "Gun Stinger DT"},

    // pandora
    {"MACHINEGUN",      "Jealousy"},
    {"MACHINEGUN_LV2",  "Jealousy"},
    {"MACHINEGUN_LV3",  "Jealousy"},
    {"DT_MGUN",         "Jealousy DT"},
    {"DT_MGUN_LV2",     "Jealousy DT"},
    {"DT_MGUN_LV3",     "Jealousy DT"},
	{"PDR-ma666s",  	"Omen"},
	{"PDR-ma666s",  	"Omen"},
	{"BOM_NOR",         "Epidemic"},
	{"BOM_NOR2",        "Hatred"},
	{"NORMAL03",        "Revenge"},
	{"BOOMERANG",       "Grief"},
	{"METEOR",        	"Argument"},
	{"BOM_METEOR",      "Argument"},
	
    // pandora gunslinger

    // yamato
	{"YMT-combo1003",   "Yamato Combo A"},
	{"YMT-combo1004",   "Yamato Combo A"},
	{"YMT-combo1005",   "Yamato Combo A"},
    {"YMT-combo1006-1", "Yamato Aerial Rave V"},
    {"YMT-combo1006-2", "Yamato Aerial Rave V"},
    {"YMT-combo1007",   "Yamato Aerial Rave V"},
	{"kuukan",   		"Slash Dimension"},
};

static const size_t MAX_TRICK_HISTORY = 12; // Only check the last 12 entries
static const std::map<std::vector<std::string>, std::string> comboNames = {
    {{"High Time", "Aerial Rave", "Aerial Rave", "Aerial Rave", "Aerial Rave"}, "Very Creative!"},
    {{"E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I", "E&I"}, "Ok Man!"},
};

// update names on visual thread rather than have the game wait on lookups i think this is better
static void UpdateTrickScores() {
    for (auto& score : trickScores) {
        if (!score.isAlreadyRenamed) {
            auto it = textLookupTable.find(score.text);
            if (it != textLookupTable.end()) {
                score.text = it->second;
            }
            score.isAlreadyRenamed = true;
        }
    }
}

static void DrawTrickScores() {
    if (devil4_sdk::get_local_player()) {
        UpdateTrickScores();
        auto now = std::chrono::steady_clock::now();
        ImVec2 screenSize = ImVec2((float)devil4_sdk::get_sRender()->xRes, (float)devil4_sdk::get_sRender()->yRes);
        ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.6f, screenSize.y * 0.4f));
        ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.3f, screenSize.y * 0.3f));
        ImGui::Begin("TrickScoresWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
        ImGui::SetWindowFontScale(2.0f);
        float fontSize = ImGui::GetFontSize();

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
            ImVec4 currentStyleColor = GetStyleColor(score.styleLetter);
            ImVec4 color2(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);

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
    ImVec2 screenSize = ImVec2((float)devil4_sdk::get_sRender()->xRes, (float)devil4_sdk::get_sRender()->yRes);
    UpdateTrickScores();
    auto now = std::chrono::steady_clock::now();

    // trick recognition
    float elapsedSinceLastTrick = std::chrono::duration<float>(now - lastTrickTime).count();
    bool turbo = devil4_sdk::get_sMediator()->turboEnabled;
    float speedMultiplier = turbo ? 1.2f : 1.0f; // why is true first its 0 1 ffs

    float fade = 1.0f - (elapsedSinceLastTrick * speedMultiplier);

    if (fade <= 0.0f) {
        trickScores.clear();
        comboScore = 0.0f;
    }

    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.0f, screenSize.y * 0.8f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 1.0f, screenSize.y * 0.2f));
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
        }
        else {
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

    ImVec4 currentStyleColor = GetStyleColor(lastScore.styleLetter);
    ImVec4 scoreColor(currentStyleColor.x, currentStyleColor.y, currentStyleColor.z, fade);

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

    // Combo recognition
float elapsedSinceLastMatch = std::chrono::duration<float>(now - lastMatchTime).count();
float fadeAlpha = 1.0f - (elapsedSinceLastMatch * speedMultiplier);

if (!trickScores.empty()) {
    const std::string& latestTrick = trickScores.back().text;
    if (lastProcessedTrick != latestTrick) {
        lastProcessedTrick = latestTrick;
    }
}

if (!trickScores.empty()) {
    for (const auto& combo : comboNames) {
        const std::vector<std::string>& comboSequence = combo.first;
        size_t comboLength = comboSequence.size();
        size_t trickScoresSize = trickScores.size();

        if (trickScoresSize >= comboLength) {
            bool match = true;
            for (size_t i = 0; i < comboLength; ++i) {
                const auto& trick = trickScores[trickScoresSize - comboLength + i].text;
                if (trick != comboSequence[i]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                detectedCombo = combo.second;
                lastMatchTime = std::chrono::steady_clock::now();
                fadeAlpha = 1.0f;
                break;
            }
        }
    }
}
    ImGui::SetNextWindowPos(ImVec2(screenSize.x * 0.1f, screenSize.y * 0.6f));
    ImGui::SetNextWindowSize(ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f));
    ImGui::Begin("ScoreRecognitionWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    ImGui::SetWindowFontScale(1.5f);
    if (fadeAlpha > 0.0f) {
        ImVec4 comboColor(1.0f, 1.0f, 0.0f, fadeAlpha);
        ImGui::PushStyleColor(ImGuiCol_Text, comboColor);
        ImGui::Text("%s", detectedCombo.c_str());
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
    newScore.isAlreadyRenamed = false;

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
