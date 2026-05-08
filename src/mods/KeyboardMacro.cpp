#include "KeyboardMacro.hpp"
#include "EnemyStepDisplay.hpp"
#include "EnemyTracker.hpp"
#include "WorkRate.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "../sdk/Devil4.hpp"
#include "../sdk/aGame.hpp"
#include "../sdk/sArea.hpp"
#include "../sdk/sMediator.hpp"
#include "../sdk/sWorkRate.hpp"
#include "../sdk/uDamage.hpp"
#include "../sdk/uEnemy.hpp"
#include "../utility/MessageDisplay.hpp"
#include "../utility/String.hpp"

namespace {
constexpr uint32_t MAX_PLAYBACK_FRAMES = 600000;
constexpr short ANALOG_MIN = -127;
constexpr short ANALOG_MAX = 127;
constexpr uintptr_t S_DEVIL4_PAD_PTR = 0x00e559c4;
constexpr uintptr_t S_KEYBOARD_PTR = 0x00e559c0;
constexpr uintptr_t S_SAVE_PTR = 0x00e558c8;
constexpr uintptr_t UPDATE_ANALOG_INFO_CALL = 0x007b0250;
constexpr uintptr_t UPLAYER_EXCEED_INPUT_OFFSET = 0x1444;
constexpr uintptr_t UPLAYER_CURRENT_STYLE_OFFSET = 0x14D98;
constexpr uint32_t MACRO_EXCEED_LATCH_TICKS = 4;
constexpr uint32_t MAX_ENEMY_CHAIN_SCAN = 128;
constexpr uint32_t PAD_BUTTON_L1 = 0x0100;
constexpr uint32_t PAD_BUTTON_R1 = 0x0200;
constexpr uint32_t PAD_BUTTON_L2 = 0x0400;
constexpr uint32_t PAD_BUTTON_R2 = 0x0800;
constexpr uint32_t PAD_BUTTON_SELECT = 0x0001;
constexpr uint32_t PAD_BUTTON_L3 = 0x0002;
constexpr uint32_t PAD_BUTTON_R3 = 0x0004;
constexpr int DANTE_STYLE_RG = 3;
constexpr size_t PAD_PRESS_L1 = 8;
constexpr size_t PAD_PRESS_L2 = 9;
constexpr size_t PAD_PRESS_R1 = 10;
constexpr size_t PAD_PRESS_R2 = 11;
constexpr size_t PAD_PRESS_SELECT = 12;
constexpr size_t PAD_PRESS_L3 = 13;
constexpr size_t PAD_PRESS_R3 = 14;
constexpr uint32_t DEFAULT_RELOAD_VKEY = VK_F9;
constexpr uint32_t DEFAULT_RESTART_VKEY = VK_F10;
constexpr uint32_t DEFAULT_STOP_VKEY = VK_F11;
constexpr uint32_t DEFAULT_CAPTURE_SNAPSHOT_VKEY = VK_F7;
constexpr uint32_t DEFAULT_LOAD_SNAPSHOT_VKEY = VK_F8;
constexpr uint32_t PLAYBACK_SLOT_MAIN = 0;
constexpr uint32_t PLAYBACK_SLOT_CUSTOM = UINT32_MAX;
constexpr uint32_t INVALID_CLIP_INDEX = UINT32_MAX;
constexpr uint32_t MACRO_WAIT_NONE = 0;
constexpr uint32_t MACRO_WAIT_ENEMY_STEP = 1;
constexpr uint32_t MACRO_WAIT_GROUNDED = 2;
constexpr uint32_t MACRO_WAIT_AIRBORNE = 3;
constexpr uint32_t MACRO_WAIT_STYLE = 4;
constexpr uint32_t MACRO_WAIT_CAN_EXCEED = 5;
constexpr uint32_t MACRO_WAIT_HITSTOP = 6;
constexpr uint32_t MACRO_WAIT_LOCKED_ON = 7;
constexpr uint32_t MACRO_WAIT_HIT_CONFIRMED = 8;
constexpr uint32_t POSITION_SNAPSHOT_LOAD_TICKS = 30;
constexpr const char* DEFAULT_PLAYBACK_FILE = "keyboard_macro.txt";
constexpr const char* CUSTOM_PLAYBACK_LABEL = "Custom";

struct ParsedMacroInput {
    uint32_t buttons = 0;
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool exceed = false;
    bool screen_pause = false;
    bool screen_resume = false;
    bool screen_pause_toggle = false;
};

struct HeldMacroInput {
    uint32_t buttons = 0;
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool exceed = false;
    bool screen_pause = false;
    bool screen_resume = false;
    bool screen_pause_toggle = false;
};

struct BattleEnemySnapshot {
    uintptr_t ptr = 0;
    int id = -1;
    bool is_active = false;
    bool in_battle = false;
    int team = 0;
    Vector3f position{};
    Vector3f velocity{};
    Vector3f rotation{};
    bool dt = false;
    float dt_timer = 0.0f;
    float berial_fire = 0.0f;
    float berial_fire_timer = 0.0f;
    float gladius_timer = 0.0f;
    bool gladius_buried = false;
    float faust_cloak = 0.0f;
    float faust_cloak_timer = 0.0f;
    float blitz_electric = 0.0f;
    float blitz_electric_timer = 0.0f;
    float blitz_electric_suicide_timer = 0.0f;
    float angelo_shield = 0.0f;
    float credo_shield = 0.0f;
    float credo_shield_timer = 0.0f;
    float sanctus_shield_m11 = 0.0f;
    float sanctus_shield_timer_m11 = 0.0f;
    float sanctus_shield_m20 = 0.0f;
    float sanctus_shield_timer_m20 = 0.0f;
    bool damage_valid = false;
    float hp = 0.0f;
    float hp_max = 0.0f;
    float hp_taken = 0.0f;
    float prev_damage_resist = 0.0f;
    int stun[5]{};
    int displacement[5]{};
    bool is_dead = false;
    float stun_reset_timer = 0.0f;
};

struct PositionSnapshot {
    bool valid = false;
    bool camera_valid = false;
    bool player_camera_valid = false;
    uint32_t mission_id = 0;
    uint32_t room_id = 0;
    uint32_t mediator_enemy_count0 = 0;
    uint32_t mediator_enemy_count1 = 0;
    uint32_t mediator_enemy_count2 = 0;
    uint32_t enemy_chain_count = 0;
    uint32_t active_enemy_count = 0;
    int locked_enemy_index = -1;
    bool player_locked_on = false;
    Vector3f player_position{};
    Vector3f player_velocity{};
    float player_rotation2 = 0.0f;
    float player_rotation3 = 0.0f;
    float player_facing_direction = 0.0f;
    float player_ground_inertia_x = 0.0f;
    float player_inertia_y = 0.0f;
    float player_ground_inertia_z = 0.0f;
    int player_current_style = 0;
    float player_hp = 0.0f;
    float player_hp_max = 0.0f;
    float player_hp_taken = 0.0f;
    float player_prev_damage_resist = 0.0f;
    float player_dt = 0.0f;
    float player_max_dt = 0.0f;
    bool player_dt_active = false;
    uint8_t player_exceed_level = 0;
    float player_exceed_timer = 0.0f;
    bool player_guard_req1 = false;
    bool player_guard_req2 = false;
    float player_guard_timer = 0.0f;
    float player_revenge_gauge = 0.0f;
    float player_disaster_gauge = 0.0f;
    int player_dreadnaught = 0;
    float camera_near_clip = 0.0f;
    float camera_fov = 0.0f;
    Vector3f camera_position{};
    Vector3f camera_up{};
    Vector3f camera_lookat{};
    Vector3f player_camera_position{};
    Vector3f player_camera_lookat{};
    float player_camera_near_clip = 0.0f;
    float player_camera_angle = 0.0f;
    float player_camera_distance = 0.0f;
    float player_camera_distance_lockon = 0.0f;
    float player_camera_fov = 0.0f;
    float player_camera_fov_battle = 0.0f;
    std::vector<BattleEnemySnapshot> enemies{};
};

struct EnemyHitSample {
    bool valid = false;
    uintptr_t ptr = 0;
    bool damage_valid = false;
    float hp = 0.0f;
    float hp_taken = 0.0f;
    int stun = 0;
    int displacement = 0;
};

bool raw_key_down[256] = {};
uint32_t capture_hotkey_target = 0;
KeyboardMacro* keyboard_macro_instance = nullptr;
std::chrono::steady_clock::time_point playback_timer_started_at{};
double playback_elapsed_seconds = 0.0;
bool playback_timer_active = false;
bool playback_timer_pending_start = false;
std::vector<std::string> playback_file_choices{};
EnemyHitSample last_hit_confirmed_enemy{};
bool has_last_hit_confirmed_enemy = false;
PositionSnapshot position_snapshot{};
uint32_t position_snapshot_load_ticks = 0;
bool restore_resources_snapshot = false;
bool keyboard_macro_suspended_for_transition = false;
cPeripheral* last_player_peripheral = nullptr;
uint32_t last_player_index = 0;

void update_config_hotkey_vkeys(const std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys);
std::string trim_copy(const std::string& value);
bool is_absolute_path(const std::string& path);
std::string get_game_directory();
void refresh_playback_file_choices();
bool parse_integer(const std::string& token, uint32_t& value);
bool parse_hotkey_binds(const std::string& expression, std::vector<uint32_t>& binds);
std::string clip_display_label(const KeyboardMacroClip& clip, uint32_t clip_index);
void clear_playback_timer();
void set_playback_status(const std::string& message);
bool is_boss_enemy_id(int enemy_id);
uDamage_Old* get_enemy_damage_block(uEnemy_Old* enemy);
sArea* get_s_area_safe();
sMediator* get_s_mediator_safe();
uPlayer* get_local_player_safe();
uCameraCtrl* get_local_camera_safe();
sWorkRate* get_work_rate_safe();

bool is_game_window_foreground() {
    auto* window = g_framework ? g_framework->get_window_handle() : nullptr;
    return !window || GetForegroundWindow() == window;
}

bool keyboard_macro_gameplay_ready() {
    __try {
        auto* area = get_s_area_safe();
        if (!area || !area->aGamePtr || area->aGamePtr->init_jump != 0) {
            return false;
        }

        auto* mediator = get_s_mediator_safe();
        if (!mediator || !mediator->player_ptr || !mediator->camera1) {
            return false;
        }

        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

sArea* get_s_area_safe() {
    __try {
        return devil4_sdk::get_sArea();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

sMediator* get_s_mediator_safe() {
    __try {
        return devil4_sdk::get_sMediator();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

uPlayer* get_local_player_safe() {
    __try {
        return devil4_sdk::get_local_player();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

uCameraCtrl* get_local_camera_safe() {
    __try {
        return devil4_sdk::get_local_camera();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

sWorkRate* get_work_rate_safe() {
    __try {
        return devil4_sdk::get_work_rate();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

sKeyboard* get_keyboard_safe() {
    __try {
        auto** keyboard_ptr = reinterpret_cast<sKeyboard**>(S_KEYBOARD_PTR);
        return keyboard_ptr ? *keyboard_ptr : nullptr;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

bool read_saved_key_binding(uintptr_t offset, uint32_t& key) {
    key = 0;

    __try {
        key = *reinterpret_cast<uint32_t*>(S_SAVE_PTR + offset);
        return key < 256;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        key = 0;
        return false;
    }
}

bool keyboard_input_down(sKeyboard* keyboard, uint32_t key, uint8_t fallback_mask) {
    if (!keyboard) {
        return false;
    }

    __try {
        bool key_down = false;
        if (key < 256) {
            key_down = ((keyboard->mState.on[key >> 5] >> (key & 0x1f)) & 1) != 0;
        }

        const bool fallback_down = (reinterpret_cast<uint8_t*>(&keyboard->mState.on[3])[0] & fallback_mask) != 0;
        return key_down || fallback_down;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

bool clear_peripheral_output(cPeripheral* peripheral, uint32_t player_index) {
    if (!peripheral) {
        return false;
    }

    __try {
        const uint32_t release_buttons = player_index < 4 ? KeyboardMacro::last_buttons[player_index] : 0;
        peripheral->mPadBtnOn = 0;
        peripheral->mPadBtnTrg = 0;
        peripheral->mPadBtnRel = release_buttons;
        std::memset(&peripheral->mPadBtnPress, 0, sizeof(float) * 15);
        peripheral->mAnlgL = {};
        peripheral->mAnlgR = {};
        peripheral->mHoldAnlgL = {};
        peripheral->mIsHold = false;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

bool clear_last_peripheral_output() {
    if (!clear_peripheral_output(last_player_peripheral, last_player_index)) {
        return false;
    }

    std::fill_n(KeyboardMacro::last_buttons, 4, 0);
    return true;
}

bool is_nero_player_safe(uPlayer* player) {
    __try {
        return player && player->controllerID == 1;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

void suspend_keyboard_macro_runtime_for_transition() {
    clear_last_peripheral_output();
    KeyboardMacro::playback_enabled = false;
    KeyboardMacro::clear_input_frames = 0;
    KeyboardMacro::input_active = false;
    KeyboardMacro::macro_exceed_active = false;
    KeyboardMacro::macro_exceed_latch_ticks = 0;
    position_snapshot_load_ticks = 0;
    clear_playback_timer();

    if (!keyboard_macro_suspended_for_transition) {
        keyboard_macro_suspended_for_transition = true;
        set_playback_status("Keyboard Macro paused while gameplay objects are rebuilding.");
    }
}

void update_macro_exceed_active_from_latch() {
    KeyboardMacro::macro_exceed_active = KeyboardMacro::macro_exceed_latch_ticks > 0;
}

void queue_macro_exceed_request() {
    KeyboardMacro::macro_exceed_latch_ticks = std::max(KeyboardMacro::macro_exceed_latch_ticks, MACRO_EXCEED_LATCH_TICKS);
    update_macro_exceed_active_from_latch();
}

void ensure_keyboard_macro_hotkeys(std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys) {
    if (hotkeys.empty()) {
        utility::create_keyboard_hotkey(hotkeys, { DEFAULT_RELOAD_VKEY }, __("Reload Macro File"), "keyboard_macro_reload_file_key");
    }
    if (hotkeys.size() == 1) {
        utility::create_keyboard_hotkey(hotkeys, { DEFAULT_RESTART_VKEY }, __("Play Macro File"), "keyboard_macro_restart_key");
    }
    if (hotkeys.size() == 2) {
        utility::create_keyboard_hotkey(hotkeys, { DEFAULT_STOP_VKEY }, __("Stop Macro / Clear Input"), "keyboard_macro_stop_clear_key");
    }
    if (hotkeys.size() == 3) {
        utility::create_keyboard_hotkey(hotkeys, { DEFAULT_CAPTURE_SNAPSHOT_VKEY }, __("Capture Snapshot"), "keyboard_macro_capture_snapshot_key");
    }
    if (hotkeys.size() == 4) {
        utility::create_keyboard_hotkey(hotkeys, { DEFAULT_LOAD_SNAPSHOT_VKEY }, __("Load Snapshot"), "keyboard_macro_load_snapshot_key");
    }
}

void ensure_keyboard_macro_hotkey_binds(std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys) {
    if (hotkeys.size() < 5) {
        return;
    }

    for (auto& hotkey : hotkeys) {
        if (hotkey && !hotkey->m_setting && hotkey->m_binds.empty()) {
            hotkey->m_binds = hotkey->m_default_keys;
        }
    }

    update_config_hotkey_vkeys(hotkeys);
}

const char* get_playback_slot_label(uint32_t slot) {
    refresh_playback_file_choices();

    if (slot == PLAYBACK_SLOT_CUSTOM) {
        return CUSTOM_PLAYBACK_LABEL;
    }

    if (slot < playback_file_choices.size()) {
        return playback_file_choices[slot].c_str();
    }

    return DEFAULT_PLAYBACK_FILE;
}

std::string get_selected_playback_path_text() {
    refresh_playback_file_choices();

    if (KeyboardMacro::playback_slot != PLAYBACK_SLOT_CUSTOM && KeyboardMacro::playback_slot < playback_file_choices.size()) {
        return playback_file_choices[KeyboardMacro::playback_slot];
    }

    return trim_copy(KeyboardMacro::playback_path);
}

std::string lowercase_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return (char)std::tolower(c);
    });
    return value;
}

std::string path_filename_lower(std::string path) {
    path = trim_copy(path);
    const auto slash = path.find_last_of("\\/");
    if (slash != std::string::npos) {
        path = path.substr(slash + 1);
    }
    return lowercase_copy(path);
}

std::string path_directory_lower(std::string path) {
    path = trim_copy(path);
    const auto slash = path.find_last_of("\\/");
    if (slash == std::string::npos) {
        return {};
    }
    return lowercase_copy(path.substr(0, slash));
}

bool is_keyboard_macro_text_filename(const std::string& filename) {
    const auto lowered = lowercase_copy(filename);
    constexpr const char* prefix = "keyboard_macro";
    constexpr const char* suffix = ".txt";

    if (lowered.rfind(prefix, 0) != 0 || lowered.size() < strlen(prefix) + strlen(suffix)) {
        return false;
    }

    return lowered.compare(lowered.size() - strlen(suffix), strlen(suffix), suffix) == 0;
}

void refresh_playback_file_choices() {
    std::string previous_selection{};
    if (KeyboardMacro::playback_slot != PLAYBACK_SLOT_CUSTOM && KeyboardMacro::playback_slot < playback_file_choices.size()) {
        previous_selection = playback_file_choices[KeyboardMacro::playback_slot];
    }

    std::vector<std::string> discovered{};
    const auto game_directory = get_game_directory();
    if (!game_directory.empty()) {
        const auto pattern = game_directory + "\\keyboard_macro*.txt";
        WIN32_FIND_DATAA find_data{};
        HANDLE handle = FindFirstFileA(pattern.c_str(), &find_data);
        if (handle != INVALID_HANDLE_VALUE) {
            do {
                if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                    is_keyboard_macro_text_filename(find_data.cFileName)) {
                    discovered.emplace_back(find_data.cFileName);
                }
            } while (FindNextFileA(handle, &find_data));
            FindClose(handle);
        }
    }

    if (discovered.empty()) {
        discovered.emplace_back(DEFAULT_PLAYBACK_FILE);
    }

    std::sort(discovered.begin(), discovered.end(), [](const std::string& left, const std::string& right) {
        return lowercase_copy(left) < lowercase_copy(right);
    });
    discovered.erase(std::unique(discovered.begin(), discovered.end(), [](const std::string& left, const std::string& right) {
        return lowercase_copy(left) == lowercase_copy(right);
    }), discovered.end());

    playback_file_choices = std::move(discovered);

    if (KeyboardMacro::playback_slot == PLAYBACK_SLOT_CUSTOM) {
        return;
    }

    if (!previous_selection.empty()) {
        const auto previous_lower = lowercase_copy(previous_selection);
        for (uint32_t index = 0; index < playback_file_choices.size(); ++index) {
            if (lowercase_copy(playback_file_choices[index]) == previous_lower) {
                KeyboardMacro::playback_slot = index;
                return;
            }
        }
    }

    if (KeyboardMacro::playback_slot >= playback_file_choices.size()) {
        KeyboardMacro::playback_slot = PLAYBACK_SLOT_MAIN;
    }
}

void queue_playback_timer_start() {
    playback_elapsed_seconds = 0.0;
    playback_timer_started_at = {};
    playback_timer_active = false;
    playback_timer_pending_start = true;
}

void begin_playback_timer_if_pending() {
    if (!playback_timer_pending_start) {
        return;
    }

    playback_timer_started_at = std::chrono::steady_clock::now();
    playback_timer_active = true;
    playback_timer_pending_start = false;
}

void finalize_playback_timer() {
    if (playback_timer_pending_start) {
        playback_timer_pending_start = false;
        return;
    }

    if (!playback_timer_active) {
        return;
    }

    playback_elapsed_seconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - playback_timer_started_at).count();
    playback_timer_active = false;
}

void clear_playback_timer() {
    playback_elapsed_seconds = 0.0;
    playback_timer_started_at = {};
    playback_timer_active = false;
    playback_timer_pending_start = false;
}

double get_playback_elapsed_seconds() {
    if (!playback_timer_active) {
        return playback_elapsed_seconds;
    }

    return playback_elapsed_seconds + std::chrono::duration<double>(std::chrono::steady_clock::now() - playback_timer_started_at).count();
}

std::string format_playback_time(double seconds) {
    if (seconds < 0.0) {
        seconds = 0.0;
    }

    const auto total_milliseconds = (uint64_t)std::llround(seconds * 1000.0);
    const auto milliseconds = total_milliseconds % 1000;
    const auto total_seconds = total_milliseconds / 1000;
    const auto secs = total_seconds % 60;
    const auto total_minutes = total_seconds / 60;
    const auto minutes = total_minutes % 60;
    const auto hours = total_minutes / 60;

    char buffer[32]{};
    if (hours > 0) {
        std::snprintf(buffer, sizeof(buffer), "%llu:%02llu:%02llu.%03llu",
            (unsigned long long)hours,
            (unsigned long long)minutes,
            (unsigned long long)secs,
            (unsigned long long)milliseconds);
    }
    else {
        std::snprintf(buffer, sizeof(buffer), "%02llu:%02llu.%03llu",
            (unsigned long long)minutes,
            (unsigned long long)secs,
            (unsigned long long)milliseconds);
    }

    return buffer;
}

std::string format_vec3_short(const Vector3f& value) {
    char buffer[96]{};
    std::snprintf(buffer, sizeof(buffer), "%.1f / %.1f / %.1f", value.x, value.y, value.z);
    return buffer;
}

float distance_between(const Vector3f& a, const Vector3f& b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;
    return std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
}

bool infer_playback_slot_from_path(const std::string& saved_path, uint32_t& slot_out) {
    refresh_playback_file_choices();

    const auto trimmed_path = trim_copy(saved_path);
    if (trimmed_path.empty()) {
        slot_out = PLAYBACK_SLOT_MAIN;
        return true;
    }

    auto find_slot_by_filename = [&](const std::string& filename) {
        const auto filename_lower = lowercase_copy(filename);
        for (uint32_t slot = 0; slot < playback_file_choices.size(); ++slot) {
            if (lowercase_copy(playback_file_choices[slot]) == filename_lower) {
                slot_out = slot;
                return true;
            }
        }

        return false;
    };

    if (!is_absolute_path(trimmed_path)) {
        return find_slot_by_filename(path_filename_lower(trimmed_path));
    }

    const auto game_directory = lowercase_copy(get_game_directory());
    if (game_directory.empty() || path_directory_lower(trimmed_path) != game_directory) {
        return false;
    }

    const auto filename = path_filename_lower(trimmed_path);
    if (!is_keyboard_macro_text_filename(filename)) {
        return false;
    }

    return find_slot_by_filename(filename);
}

std::string trim_copy(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });

    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();

    if (first >= last) {
        return {};
    }

    return std::string(first, last);
}

std::string normalize_button_token(std::string token) {
    token = trim_copy(token);
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char c) {
        return (char)std::toupper(c);
    });

    for (auto& c : token) {
        if (c == '-' || c == ' ') {
            c = '_';
        }
    }

    return token;
}

bool is_modifier_vkey(uint32_t vkey) {
    return vkey == VK_SHIFT || vkey == VK_CONTROL || vkey == VK_MENU ||
        vkey == VK_LSHIFT || vkey == VK_RSHIFT ||
        vkey == VK_LCONTROL || vkey == VK_RCONTROL ||
        vkey == VK_LMENU || vkey == VK_RMENU;
}

bool parse_hotkey_token(const std::string& token, uint32_t& vkey) {
    const auto normalized = normalize_button_token(token);
    if (normalized.empty()) {
        return false;
    }

    if (normalized.size() == 1) {
        const auto c = normalized[0];
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
            vkey = (uint32_t)c;
            return true;
        }
    }

    if (normalized[0] == 'F' && normalized.size() <= 3) {
        uint32_t function_index = 0;
        if (parse_integer(normalized.substr(1), function_index) && function_index >= 1 && function_index <= 24) {
            vkey = VK_F1 + function_index - 1;
            return true;
        }
    }

    if (normalized.rfind("NUMPAD", 0) == 0 && normalized.size() == 7) {
        const auto c = normalized[6];
        if (c >= '0' && c <= '9') {
            vkey = VK_NUMPAD0 + (c - '0');
            return true;
        }
    }

    static const std::pair<const char*, uint32_t> hotkey_tokens[] = {
        {"CTRL", VK_CONTROL},
        {"CONTROL", VK_CONTROL},
        {"LCTRL", VK_LCONTROL},
        {"LCONTROL", VK_LCONTROL},
        {"RCTRL", VK_RCONTROL},
        {"RCONTROL", VK_RCONTROL},
        {"SHIFT", VK_SHIFT},
        {"LSHIFT", VK_LSHIFT},
        {"RSHIFT", VK_RSHIFT},
        {"ALT", VK_MENU},
        {"LALT", VK_LMENU},
        {"RALT", VK_RMENU},
        {"SPACE", VK_SPACE},
        {"TAB", VK_TAB},
        {"ENTER", VK_RETURN},
        {"RETURN", VK_RETURN},
        {"ESC", VK_ESCAPE},
        {"ESCAPE", VK_ESCAPE},
        {"BACKSPACE", VK_BACK},
        {"INSERT", VK_INSERT},
        {"DELETE", VK_DELETE},
        {"HOME", VK_HOME},
        {"END", VK_END},
        {"PAGEUP", VK_PRIOR},
        {"PGUP", VK_PRIOR},
        {"PAGEDOWN", VK_NEXT},
        {"PGDOWN", VK_NEXT},
        {"UP", VK_UP},
        {"DOWN", VK_DOWN},
        {"LEFT", VK_LEFT},
        {"RIGHT", VK_RIGHT},
        {"PAUSE", VK_PAUSE},
    };

    for (const auto& entry : hotkey_tokens) {
        if (normalized == entry.first) {
            vkey = entry.second;
            return true;
        }
    }

    return false;
}

bool parse_hotkey_binds(const std::string& expression, std::vector<uint32_t>& binds) {
    binds.clear();

    const auto trimmed_expression = trim_copy(expression);
    if (trimmed_expression.empty()) {
        return true;
    }

    std::vector<uint32_t> modifiers{};
    std::vector<uint32_t> triggers{};
    std::istringstream stream{ trimmed_expression };
    std::string token{};
    while (std::getline(stream, token, '+')) {
        token = trim_copy(token);
        if (token.empty()) {
            return false;
        }

        uint32_t vkey = 0;
        if (!parse_hotkey_token(token, vkey)) {
            return false;
        }

        auto push_unique = [](std::vector<uint32_t>& target, uint32_t value) {
            if (std::find(target.begin(), target.end(), value) == target.end()) {
                target.push_back(value);
            }
        };

        if (is_modifier_vkey(vkey)) {
            push_unique(modifiers, vkey);
        }
        else {
            push_unique(triggers, vkey);
        }
    }

    if (triggers.empty()) {
        return false;
    }

    binds = modifiers;
    binds.insert(binds.end(), triggers.begin(), triggers.end());
    return true;
}

bool parse_integer(const std::string& token, uint32_t& value) {
    if (token.empty()) {
        return false;
    }

    char* end = nullptr;
    const auto parsed = std::strtoul(token.c_str(), &end, 0);
    if (end == token.c_str() || *end != '\0') {
        return false;
    }

    value = (uint32_t)parsed;
    return true;
}

bool parse_float(const std::string& token, float& value) {
    if (token.empty()) {
        return false;
    }

    char* end = nullptr;
    const auto parsed = std::strtof(token.c_str(), &end);
    if (end == token.c_str() || *end != '\0') {
        return false;
    }

    value = parsed;
    return true;
}

bool parse_button_name(const std::string& token, uint32_t& button) {
    const auto normalized = normalize_button_token(token);

    if (normalized.empty() || normalized == "NONE" || normalized == "0") {
        button = 0;
        return true;
    }

    if (normalized.rfind("0X", 0) == 0) {
        return parse_integer(normalized, button);
    }

    static const std::pair<const char*, uint32_t> buttons[] = {
        {"SELECT", 0x0001},
        {"L3", 0x0002},
        {"R3", 0x0004},
        {"START", 0x0008},
        {"PAUSE", 0x0008},
        {"PAUSE_MENU", 0x0008},
        {"PAVSE", 0x0008},
        {"PAVSE_MENU", 0x0008},
        {"MENU", 0x0008},
        {"START_MENU", 0x0008},
        {"OPTIONS", 0x0008},
        {"ESC", 0x0008},
        {"ESCAPE", 0x0008},
        {"DPAD_UP", 0x0010},
        {"UP", 0x0010},
        {"DPAD_RIGHT", 0x0020},
        {"RIGHT", 0x0020},
        {"DPAD_DOWN", 0x0040},
        {"DOWN", 0x0040},
        {"DPAD_LEFT", 0x0080},
        {"LEFT", 0x0080},
        {"L1", 0x0100},
        {"R1", 0x0200},
        {"L2", 0x0400},
        {"R2", 0x0800},
        {"Y", 0x1000},
        {"B", 0x2000},
        {"A", 0x4000},
        {"X", 0x8000},
    };

    for (const auto& entry : buttons) {
        if (normalized == entry.first) {
            button = entry.second;
            return true;
        }
    }

    return false;
}

bool parse_style_name(const std::string& token, int& style) {
    const auto normalized = normalize_button_token(token);

    uint32_t numeric_style = 0;
    if (parse_integer(normalized, numeric_style) && numeric_style <= 4) {
        style = (int)numeric_style;
        return true;
    }

    static const std::pair<const char*, int> styles[] = {
        {"SM", 0},
        {"SWORDMASTER", 0},
        {"SWORD_MASTER", 0},
        {"GS", 1},
        {"GUNSLINGER", 1},
        {"GUN_SLINGER", 1},
        {"TS", 2},
        {"TRICKSTER", 2},
        {"TRICK_STAR", 2},
        {"RG", 3},
        {"ROYALGUARD", 3},
        {"ROYAL_GUARD", 3},
        {"DS", 4},
        {"DARKSLAYER", 4},
        {"DARK_SLAYER", 4},
    };

    for (const auto& entry : styles) {
        if (normalized == entry.first) {
            style = entry.second;
            return true;
        }
    }

    return false;
}

bool parse_wait_condition(std::istringstream& stream, uint32_t& condition, int& arg, uint32_t& max_ticks) {
    condition = MACRO_WAIT_NONE;
    arg = 0;
    max_ticks = 0;

    std::string condition_token{};
    stream >> condition_token;
    const auto normalized = normalize_button_token(condition_token);
    if (normalized.empty()) {
        return false;
    }

    if (normalized == "ENEMY_STEP" || normalized == "ENEMYSTEP" || normalized == "JC" || normalized == "JUMP_CANCEL") {
        condition = MACRO_WAIT_ENEMY_STEP;
    }
    else if (normalized == "GROUNDED" || normalized == "GROUND") {
        condition = MACRO_WAIT_GROUNDED;
    }
    else if (normalized == "AIRBORNE" || normalized == "AIR" || normalized == "IN_AIR") {
        condition = MACRO_WAIT_AIRBORNE;
    }
    else if (normalized == "STYLE") {
        std::string style_token{};
        stream >> style_token;
        int style = -1;
        if (!parse_style_name(style_token, style)) {
            return false;
        }
        condition = MACRO_WAIT_STYLE;
        arg = style;
    }
    else if (normalized == "CAN_EXCEED" || normalized == "EXCEED_WINDOW" || normalized == "MAX_ACT") {
        condition = MACRO_WAIT_CAN_EXCEED;
    }
    else if (normalized == "HITSTOP" || normalized == "HIT_STOP") {
        condition = MACRO_WAIT_HITSTOP;
    }
    else if (normalized == "HIT_CONFIRMED" || normalized == "HITCONFIRMED" || normalized == "DAMAGE_REGISTERED" ||
        normalized == "DAMAGE" || normalized == "ENEMY_HIT" || normalized == "HIT") {
        condition = MACRO_WAIT_HIT_CONFIRMED;
    }
    else if (normalized == "LOCKED_ON" || normalized == "LOCKON" || normalized == "LOCK_ON") {
        condition = MACRO_WAIT_LOCKED_ON;
    }
    else {
        return false;
    }

    std::string max_ticks_token{};
    stream >> max_ticks_token;
    if (!max_ticks_token.empty() && (!parse_integer(max_ticks_token, max_ticks) || max_ticks == 0)) {
        return false;
    }

    std::string extra{};
    return !(stream >> extra);
}

short clamp_analog(int value) {
    return (short)std::clamp(value, (int)ANALOG_MIN, (int)ANALOG_MAX);
}

bool parse_left_analog_name(const std::string& token, int& x, int& y) {
    const auto normalized = normalize_button_token(token);

    static const std::pair<const char*, std::pair<int, int>> directions[] = {
        {"W", {0, ANALOG_MAX}},
        {"MW", {0, ANALOG_MAX}},
        {"MOVE_W", {0, ANALOG_MAX}},
        {"MOVE_UP", {0, ANALOG_MAX}},
        {"MOVE_FORWARD", {0, ANALOG_MAX}},
        {"FORWARD", {0, ANALOG_MAX}},
        {"LSTICK_UP", {0, ANALOG_MAX}},
        {"LS_UP", {0, ANALOG_MAX}},
        {"S", {0, ANALOG_MIN}},
        {"MS", {0, ANALOG_MIN}},
        {"MOVE_S", {0, ANALOG_MIN}},
        {"MOVE_DOWN", {0, ANALOG_MIN}},
        {"MOVE_BACK", {0, ANALOG_MIN}},
        {"BACK", {0, ANALOG_MIN}},
        {"BACKWARD", {0, ANALOG_MIN}},
        {"LSTICK_DOWN", {0, ANALOG_MIN}},
        {"LS_DOWN", {0, ANALOG_MIN}},
        {"D", {ANALOG_MAX, 0}},
        {"MD", {ANALOG_MAX, 0}},
        {"MOVE_D", {ANALOG_MAX, 0}},
        {"MOVE_RIGHT", {ANALOG_MAX, 0}},
        {"LSTICK_RIGHT", {ANALOG_MAX, 0}},
        {"LS_RIGHT", {ANALOG_MAX, 0}},
        {"MA", {ANALOG_MIN, 0}},
        {"MOVE_A", {ANALOG_MIN, 0}},
        {"MOVE_LEFT", {ANALOG_MIN, 0}},
        {"LSTICK_LEFT", {ANALOG_MIN, 0}},
        {"LS_LEFT", {ANALOG_MIN, 0}},
        {"MW_MD", {ANALOG_MAX, ANALOG_MAX}},
        {"MOVE_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"LSTICK_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"LS_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"MW_MA", {ANALOG_MIN, ANALOG_MAX}},
        {"MOVE_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"LSTICK_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"LS_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"MS_MD", {ANALOG_MAX, ANALOG_MIN}},
        {"MOVE_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"LSTICK_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"LS_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"MS_MA", {ANALOG_MIN, ANALOG_MIN}},
        {"MOVE_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"LSTICK_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"LS_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
    };

    for (const auto& entry : directions) {
        if (normalized == entry.first) {
            x = entry.second.first;
            y = entry.second.second;
            return true;
        }
    }

    return false;
}

void apply_direction_to_input(ParsedMacroInput& input, int x, int y) {
    if (x < 0) {
        input.move_left = true;
    }
    else if (x > 0) {
        input.move_right = true;
    }

    if (y < 0) {
        input.move_down = true;
    }
    else if (y > 0) {
        input.move_up = true;
    }
}

KeyboardMacroFrame frame_from_parts(
    uint32_t buttons,
    bool move_up,
    bool move_down,
    bool move_left,
    bool move_right,
    bool exceed,
    bool screen_pause,
    bool screen_resume,
    bool screen_pause_toggle) {
    KeyboardMacroFrame frame{};
    frame.buttons = buttons;
    frame.left_x = clamp_analog((move_right ? ANALOG_MAX : 0) + (move_left ? ANALOG_MIN : 0));
    frame.left_y = clamp_analog((move_up ? ANALOG_MAX : 0) + (move_down ? ANALOG_MIN : 0));
    frame.has_left_analog = move_up || move_down || move_left || move_right;
    frame.exceed = exceed;
    frame.screen_pause = screen_pause;
    frame.screen_resume = screen_resume;
    frame.screen_pause_toggle = screen_pause_toggle;
    return frame;
}

KeyboardMacroFrame frame_from_input(const ParsedMacroInput& input) {
    return frame_from_parts(
        input.buttons,
        input.move_up,
        input.move_down,
        input.move_left,
        input.move_right,
        input.exceed,
        input.screen_pause,
        input.screen_resume,
        input.screen_pause_toggle);
}

KeyboardMacroFrame frame_from_forced_style(int style) {
    KeyboardMacroFrame frame{};
    frame.force_style = true;
    frame.forced_style = style;
    return frame;
}

KeyboardMacroFrame frame_from_held(const HeldMacroInput& input) {
    return frame_from_parts(
        input.buttons,
        input.move_up,
        input.move_down,
        input.move_left,
        input.move_right,
        input.exceed,
        input.screen_pause,
        input.screen_resume,
        input.screen_pause_toggle);
}

KeyboardMacroFrame frame_from_wait_condition(
    const HeldMacroInput& input,
    uint32_t condition,
    int arg,
    uint32_t max_ticks) {
    auto frame = frame_from_held(input);
    frame.wait_condition = condition;
    frame.wait_arg = arg;
    frame.wait_max_ticks = max_ticks;
    frame.wait_elapsed_ticks = 0;
    return frame;
}

bool parse_macro_input(std::string expression, ParsedMacroInput& input) {
    input = {};

    for (auto& c : expression) {
        if (c == '+' || c == '|' || c == ',') {
            c = ' ';
        }
    }

    std::istringstream stream{ expression };
    std::string token{};
    bool found_token = false;
    while (stream >> token) {
        const auto normalized = normalize_button_token(token);
        if (normalized.empty() || normalized == "NONE" || normalized == "WAIT" || normalized == "0") {
            found_token = true;
            continue;
        }

        if (normalized == "EXCEED" || normalized == "EX" || normalized == "REV" ||
            normalized == "MAX_ACT" || normalized == "MAXACT") {
            input.exceed = true;
            found_token = true;
            continue;
        }

        if (normalized == "FREEZE" || normalized == "SCREEN_FREEZE" || normalized == "SCREEN_PAUSE" ||
            normalized == "TRAINER_PAUSE" || normalized == "FRAME_PAUSE") {
            input.screen_pause = true;
            found_token = true;
            continue;
        }

        if (normalized == "UNFREEZE" || normalized == "RESUME" || normalized == "UNPAUSE" ||
            normalized == "SCREEN_RESUME" || normalized == "SCREEN_UNPAUSE") {
            input.screen_resume = true;
            found_token = true;
            continue;
        }

        if (normalized == "TOGGLE_FREEZE" || normalized == "TOGGLE_PAUSE" || normalized == "SCREEN_PAUSE_TOGGLE") {
            input.screen_pause_toggle = true;
            found_token = true;
            continue;
        }

        int direction_x = 0;
        int direction_y = 0;
        if (parse_left_analog_name(token, direction_x, direction_y)) {
            apply_direction_to_input(input, direction_x, direction_y);
            found_token = true;
            continue;
        }

        uint32_t button = 0;
        if (!parse_button_name(token, button)) {
            return false;
        }
        input.buttons |= button;
        found_token = true;
    }

    return found_token;
}

bool parse_playback_command(std::string expression, KeyboardMacroFrame& frame) {
    ParsedMacroInput input{};
    if (!parse_macro_input(std::move(expression), input)) {
        return false;
    }

    frame = frame_from_input(input);
    return true;
}

void hold_input(HeldMacroInput& held, const ParsedMacroInput& input) {
    held.buttons |= input.buttons;
    held.move_up |= input.move_up;
    held.move_down |= input.move_down;
    held.move_left |= input.move_left;
    held.move_right |= input.move_right;
    held.exceed |= input.exceed;
    held.screen_pause |= input.screen_pause;
    held.screen_resume |= input.screen_resume;
    held.screen_pause_toggle |= input.screen_pause_toggle;
}

void release_input(HeldMacroInput& held, const ParsedMacroInput& input) {
    held.buttons &= ~input.buttons;
    if (input.move_up) {
        held.move_up = false;
    }
    if (input.move_down) {
        held.move_down = false;
    }
    if (input.move_left) {
        held.move_left = false;
    }
    if (input.move_right) {
        held.move_right = false;
    }
    if (input.exceed) {
        held.exceed = false;
    }
    if (input.screen_pause) {
        held.screen_pause = false;
    }
    if (input.screen_resume) {
        held.screen_resume = false;
    }
    if (input.screen_pause_toggle) {
        held.screen_pause_toggle = false;
    }
}

bool append_playback_frames(std::vector<KeyboardMacroFrame>& frames, uint32_t frame_count, const KeyboardMacroFrame& frame) {
    if (frame_count == 0 || frames.size() + frame_count > MAX_PLAYBACK_FRAMES) {
        return false;
    }

    frames.insert(frames.end(), frame_count, frame);
    return true;
}

bool append_tap_macro(std::vector<KeyboardMacroFrame>& frames, HeldMacroInput& held, const std::string& input_expression, uint32_t ticks) {
    ParsedMacroInput input{};
    if (ticks == 0 || !parse_macro_input(input_expression, input)) {
        return false;
    }

    const auto saved_held = held;
    hold_input(held, input);
    const bool ok = append_playback_frames(frames, ticks, frame_from_held(held));
    held = saved_held;
    return ok;
}

bool append_direction_button_macro(
    std::vector<KeyboardMacroFrame>& frames,
    HeldMacroInput& held,
    const std::string& direction_expression,
    const std::string& input_expression,
    uint32_t ticks) {
    ParsedMacroInput direction{};
    ParsedMacroInput input{};
    if (ticks == 0 || !parse_macro_input(direction_expression, direction) || !parse_macro_input(input_expression, input)) {
        return false;
    }

    const auto saved_held = held;
    hold_input(held, direction);
    hold_input(held, input);
    const bool ok = append_playback_frames(frames, ticks, frame_from_held(held));
    held = saved_held;
    return ok;
}

bool append_back_forward_macro(
    std::vector<KeyboardMacroFrame>& frames,
    HeldMacroInput& held,
    const std::string& back_expression,
    const std::string& forward_expression,
    const std::string& input_expression) {
    ParsedMacroInput back{};
    ParsedMacroInput forward{};
    ParsedMacroInput input{};
    if (!parse_macro_input(back_expression, back) || !parse_macro_input(forward_expression, forward) || !parse_macro_input(input_expression, input)) {
        return false;
    }

    const auto saved_held = held;
    bool ok = true;

    hold_input(held, back);
    ok &= append_playback_frames(frames, 1, frame_from_held(held));

    hold_input(held, forward);
    release_input(held, back);
    ok &= append_playback_frames(frames, 1, frame_from_held(held));

    release_input(held, forward);
    hold_input(held, input);
    ok &= append_playback_frames(frames, 1, frame_from_held(held));

    held = saved_held;
    return ok;
}

void update_analog_info_for_macro(kAnlg* analog) {
    _asm {
            pushad
            mov esi, [analog]
            call UPDATE_ANALOG_INFO_CALL
            popad
    }
}

void get_pad_analog_level(sDevil4Pad* pad, kAnlg* analog, bool right_side) {
    if (!pad || !analog) {
        return;
    }

    const float analog_x = pad->mPadInfo[0].mAnlg[right_side].x;
    const float analog_y = pad->mPadInfo[0].mAnlg[right_side].y;

    if ((0.24f <= std::abs(analog_x)) || (0.24f <= std::abs(analog_y))) {
        analog->x = (short)(int)(analog_x * 127.0f);
        analog->y = (short)(int)(analog_y * 127.0f);
    }
    else {
        analog->x = 0;
        analog->y = 0;
    }
}

void keyboard_to_analog(kAnlg* input, int char_id, int key_id) {
    if (!input) {
        return;
    }

    sKeyboard* keyboard = get_keyboard_safe();
    if (!keyboard) {
        return;
    }

    short analog = input->y;
    short analog_shift = analog >> 0x1f;
    if ((int)((analog ^ analog_shift) - analog_shift) < 0x40) {
        uint32_t key = 0;
        if (char_id == 0) {
            read_saved_key_binding(0x1b0 + key_id * 16, key);
        }
        else if (char_id == 1) {
            read_saved_key_binding(0x298 + key_id * 16, key);
        }
        else {
            key = 0;
        }

        if (keyboard_input_down(keyboard, key, 0x40)) {
            input->x = ANALOG_MAX;
        }

        if (char_id == 0) {
            read_saved_key_binding(0x1b4 + key_id * 16, key);
        }
        else if (char_id == 1) {
            read_saved_key_binding(0x29c + key_id * 16, key);
        }
        else {
            key = 0;
        }

        if (keyboard_input_down(keyboard, key, 0x10)) {
            input->y = ANALOG_MIN;
        }
    }

    analog = input->x;
    analog_shift = analog >> 0x1f;
    if ((int)((analog ^ analog_shift) - analog_shift) < 0x40) {
        uint32_t key = 0;
        if (char_id == 0) {
            read_saved_key_binding(0x1bc + key_id * 16, key);
        }
        else if (char_id == 1) {
            read_saved_key_binding(0x2a4 + key_id * 16, key);
        }
        else {
            key = 0;
        }

        if (keyboard_input_down(keyboard, key, 0x40)) {
            input->x = ANALOG_MAX;
        }

        if (char_id == 0) {
            read_saved_key_binding(0x1b8 + key_id * 4, key);
        }
        else if (char_id == 1) {
            read_saved_key_binding(0x2a0 + key_id * 4, key);
        }
        else {
            key = 0;
        }

        if (keyboard_input_down(keyboard, key, 0x10)) {
            input->x = ANALOG_MIN;
        }
    }
}

void write_base_player_input_snapshot(cPeripheral* peripheral) {
    if (!peripheral) {
        return;
    }

    sDevil4Pad** pad_ptr = (sDevil4Pad**)S_DEVIL4_PAD_PTR;
    sDevil4Pad* pad = pad_ptr ? *pad_ptr : nullptr;
    if (!pad) {
        return;
    }

    peripheral->mPadBtnOn = pad->mPadInfo[0].mBtn.on;
    peripheral->mPadBtnTrg = pad->mPadInfo[0].mBtn.trg;
    peripheral->mPadBtnRel = pad->mPadInfo[0].mBtn.rel;
    std::memcpy(&peripheral->mPadBtnPress, &pad->mPadInfo[0].mPress, sizeof(float) * 15);

    peripheral->mAnlgL = {};
    get_pad_analog_level(pad, &peripheral->mAnlgL, false);
    keyboard_to_analog(&peripheral->mAnlgL, 0, 0);
    update_analog_info_for_macro(&peripheral->mAnlgL);

    peripheral->mAnlgR = {};
    get_pad_analog_level(pad, &peripheral->mAnlgR, true);
    keyboard_to_analog(&peripheral->mAnlgR, 0, 1);
    update_analog_info_for_macro(&peripheral->mAnlgR);

    peripheral->mHoldAnlgL = peripheral->mAnlgL;
    peripheral->mIsHold = false;
}

void set_pad_press(cPeripheral* peripheral, uint32_t buttons, uint32_t button_mask, size_t press_index) {
    if ((buttons & button_mask) == 0 || press_index >= 15) {
        return;
    }

    peripheral->mPadBtnPress[press_index] = std::max(peripheral->mPadBtnPress[press_index], 1.0f);
}

void apply_button_press_values(cPeripheral* peripheral, uint32_t buttons) {
    if (!peripheral) {
        return;
    }

    set_pad_press(peripheral, buttons, PAD_BUTTON_L1, PAD_PRESS_L1);
    set_pad_press(peripheral, buttons, PAD_BUTTON_L2, PAD_PRESS_L2);
    set_pad_press(peripheral, buttons, PAD_BUTTON_R1, PAD_PRESS_R1);
    set_pad_press(peripheral, buttons, PAD_BUTTON_R2, PAD_PRESS_R2);
    set_pad_press(peripheral, buttons, PAD_BUTTON_SELECT, PAD_PRESS_SELECT);
    set_pad_press(peripheral, buttons, PAD_BUTTON_L3, PAD_PRESS_L3);
    set_pad_press(peripheral, buttons, PAD_BUTTON_R3, PAD_PRESS_R3);
}

void set_playback_status(const std::string& message) {
    strncpy_s(KeyboardMacro::playback_status, message.c_str(), _TRUNCATE);
}

uEnemy_Old* select_hit_confirm_enemy(uPlayer* player) {
    if (!player) {
        return nullptr;
    }

    uEnemy_Old* candidates[] = {
        player->lockOnTargetPtr3,
        player->lockOnTargetPtr1,
        player->lockOnTargetPtr2,
        player->lockOnTargetPtr4,
    };

    for (auto* enemy : candidates) {
        if (enemy) {
            return enemy;
        }
    }

    uint32_t visited = 0;
    for (uEnemy_Old* enemy = devil4_sdk::get_uEnemies(); enemy && visited < MAX_ENEMY_CHAIN_SCAN; enemy = enemy->nextEnemy, ++visited) {
        if (enemy->isActive) {
            return enemy;
        }
    }

    return nullptr;
}

EnemyHitSample capture_enemy_hit_sample(uPlayer* player) {
    EnemyHitSample sample{};

    __try {
        auto* enemy = select_hit_confirm_enemy(player);
        if (!enemy) {
            return sample;
        }

        sample.valid = true;
        sample.ptr = reinterpret_cast<uintptr_t>(enemy);

        if (auto* damage = get_enemy_damage_block(enemy)) {
            sample.damage_valid = true;
            sample.hp = damage->HP;
            sample.hp_taken = damage->HPTaken;
            sample.stun = damage->stun[0];
            sample.displacement = damage->displacement[0];
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        std::memset(&sample, 0, sizeof(sample));
    }

    return sample;
}

void reset_hit_confirmed_wait_state() {
    last_hit_confirmed_enemy = {};
    has_last_hit_confirmed_enemy = false;
}

bool is_hit_confirmed_delta(const EnemyHitSample& previous, const EnemyHitSample& current) {
    if (!previous.valid || !current.valid || previous.ptr != current.ptr ||
        !previous.damage_valid || !current.damage_valid) {
        return false;
    }

    const float hp_delta = current.hp - previous.hp;
    const float hp_taken_delta = current.hp_taken - previous.hp_taken;
    const int stun_delta = current.stun - previous.stun;
    const int displacement_delta = current.displacement - previous.displacement;

    if (hp_delta < -0.01f) {
        return true;
    }
    if (hp_taken_delta > 0.01f) {
        return true;
    }
    if (stun_delta > 0) {
        return true;
    }
    if (previous.displacement == 0 && current.displacement != 0) {
        return true;
    }
    return displacement_delta > 0;
}

bool evaluate_hit_confirmed_wait_condition(const KeyboardMacroFrame& frame) {
    auto* player = get_local_player_safe();
    if (!player) {
        reset_hit_confirmed_wait_state();
        return false;
    }

    const auto sample = capture_enemy_hit_sample(player);
    if (!sample.valid || !sample.damage_valid) {
        reset_hit_confirmed_wait_state();
        return false;
    }

    if (frame.wait_elapsed_ticks == 0 || !has_last_hit_confirmed_enemy ||
        !last_hit_confirmed_enemy.valid || last_hit_confirmed_enemy.ptr != sample.ptr) {
        last_hit_confirmed_enemy = sample;
        has_last_hit_confirmed_enemy = true;
        return false;
    }

    const bool confirmed = is_hit_confirmed_delta(last_hit_confirmed_enemy, sample);
    last_hit_confirmed_enemy = sample;
    has_last_hit_confirmed_enemy = true;
    return confirmed;
}

uEnemy_Old* get_enemy_chain_head_safe() {
    __try {
        return devil4_sdk::get_uEnemies();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

bool read_enemy_chain_link_safe(uEnemy_Old* enemy, uEnemy_Old*& next_enemy) {
    next_enemy = nullptr;

    __try {
        if (!enemy) {
            return false;
        }

        next_enemy = enemy->nextEnemy;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        next_enemy = nullptr;
        return false;
    }
}

std::vector<uEnemy_Old*> collect_enemy_chain() {
    std::vector<uEnemy_Old*> enemies{};
    for (uEnemy_Old* enemy = get_enemy_chain_head_safe(); enemy && enemies.size() < MAX_ENEMY_CHAIN_SCAN;) {
        enemies.push_back(enemy);

        uEnemy_Old* next_enemy = nullptr;
        if (!read_enemy_chain_link_safe(enemy, next_enemy)) {
            break;
        }
        enemy = next_enemy;
    }
    return enemies;
}

bool is_boss_enemy_id(int enemy_id) {
    switch (enemy_id) {
    case BERIAL:
    case BAEL:
    case ECHIDNA:
    case CREDO:
    case AGNUS:
    case SAVIOR:
    case SANCTUS_M11:
    case SANCTUS_M20:
        return true;
    default:
        return false;
    }
}

uDamage_Old* get_enemy_damage_block(uEnemy_Old* enemy) {
    if (!enemy) {
        return nullptr;
    }

    const int enemy_id = (int)enemy->ID;
    if (is_boss_enemy_id(enemy_id)) {
        return nullptr;
    }

    const int damage_offset = EnemyTracker::get_enemy_specific_damage_offset(enemy_id);
    if (damage_offset == 0) {
        return nullptr;
    }

    return (uDamage_Old*)((char*)enemy + damage_offset);
}

bool capture_enemy_snapshot(uEnemy_Old* enemy, BattleEnemySnapshot& snapshot) {
    std::memset(&snapshot, 0, sizeof(snapshot));
    snapshot.id = -1;

    if (!enemy) {
        return false;
    }

    __try {
        snapshot.ptr = reinterpret_cast<uintptr_t>(enemy);
        snapshot.id = (int)enemy->ID;
        snapshot.is_active = enemy->isActive;
        snapshot.in_battle = enemy->inBattle;
    snapshot.team = enemy->team;
    snapshot.position = enemy->position;
    snapshot.velocity = enemy->velocity;
    snapshot.rotation = enemy->rotation;
    snapshot.dt = enemy->DT;
    snapshot.dt_timer = enemy->DTTimer;

        if (!is_boss_enemy_id(snapshot.id)) {
            if (snapshot.id == BERIAL) {
                snapshot.berial_fire = enemy->berialFire;
                snapshot.berial_fire_timer = enemy->berialFireTimer;
            }
            if (snapshot.id == GLADIUS) {
                snapshot.gladius_timer = enemy->gladiusTimer;
                snapshot.gladius_buried = enemy->gladiusBuried;
            }
            if (snapshot.id == MEPHISTO || snapshot.id == FAUST) {
                snapshot.faust_cloak = enemy->faustCloak;
                snapshot.faust_cloak_timer = enemy->faustCloakTimer;
            }
            if (snapshot.id == BLITZ) {
                snapshot.blitz_electric = enemy->blitzElectric;
                snapshot.blitz_electric_timer = enemy->blitzElectricTimer;
                snapshot.blitz_electric_suicide_timer = enemy->blitzElectricSuicideTimer;
            }
            if (snapshot.id == ANGELO_BIANCO || snapshot.id == ANGELO_ALTO) {
                snapshot.angelo_shield = enemy->angeloShield;
            }
        }

        snapshot.damage_valid = false;
        if (!is_boss_enemy_id(snapshot.id)) {
            if (auto* damage = get_enemy_damage_block(enemy)) {
                snapshot.damage_valid = true;
                snapshot.hp = damage->HP;
                snapshot.hp_max = damage->HPMax;
                snapshot.hp_taken = damage->HPTaken;
                snapshot.prev_damage_resist = damage->prevDamageResist;
                std::copy(std::begin(damage->stun), std::end(damage->stun), std::begin(snapshot.stun));
                std::copy(std::begin(damage->displacement), std::end(damage->displacement), std::begin(snapshot.displacement));
                snapshot.is_dead = damage->isDead;
                snapshot.stun_reset_timer = damage->stunResetTimer;
            }
        }

        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        std::memset(&snapshot, 0, sizeof(snapshot));
        snapshot.id = -1;
        return false;
    }
}

void apply_enemy_stable_snapshot(uEnemy_Old* enemy, const BattleEnemySnapshot& snapshot) {
    if (!enemy) {
        return;
    }

    enemy->position = snapshot.position;
    enemy->velocity = snapshot.velocity;
    enemy->rotation = snapshot.rotation;

    if (snapshot.damage_valid) {
        if (auto* damage = get_enemy_damage_block(enemy)) {
            damage->HP = snapshot.hp;
            damage->HPMax = snapshot.hp_max;
            damage->HPTaken = snapshot.hp_taken;
            damage->prevDamageResist = snapshot.prev_damage_resist;
            std::copy(std::begin(snapshot.stun), std::end(snapshot.stun), std::begin(damage->stun));
            std::copy(std::begin(snapshot.displacement), std::end(snapshot.displacement), std::begin(damage->displacement));
            damage->isDead = snapshot.is_dead;
            damage->stunResetTimer = snapshot.stun_reset_timer;
        }
    }
}

void apply_enemy_combat_snapshot(uEnemy_Old* enemy, const BattleEnemySnapshot& snapshot) {
    if (!enemy) {
        return;
    }

    if (is_boss_enemy_id(snapshot.id)) {
        return;
    }

    enemy->DT = snapshot.dt;
    enemy->DTTimer = snapshot.dt_timer;
    if (snapshot.id == BERIAL) {
        enemy->berialFire = snapshot.berial_fire;
        enemy->berialFireTimer = snapshot.berial_fire_timer;
    }
    if (snapshot.id == GLADIUS) {
        enemy->gladiusTimer = snapshot.gladius_timer;
        enemy->gladiusBuried = snapshot.gladius_buried;
    }
    if (snapshot.id == MEPHISTO || snapshot.id == FAUST) {
        enemy->faustCloak = snapshot.faust_cloak;
        enemy->faustCloakTimer = snapshot.faust_cloak_timer;
    }
    if (snapshot.id == BLITZ) {
        enemy->blitzElectric = snapshot.blitz_electric;
        enemy->blitzElectricTimer = snapshot.blitz_electric_timer;
        enemy->blitzElectricSuicideTimer = snapshot.blitz_electric_suicide_timer;
    }
    if (snapshot.id == ANGELO_BIANCO || snapshot.id == ANGELO_ALTO) {
        enemy->angeloShield = snapshot.angelo_shield;
    }
}

std::string position_snapshot_label() {
    if (!position_snapshot.valid) {
        return "Battle Snapshot: <none>";
    }

    char buffer[320]{};
    std::snprintf(
        buffer,
        sizeof(buffer),
        "Battle Snapshot: mission/room %u/%u, enemies %u chain / %u active, player %s, camera %s",
        position_snapshot.mission_id,
        position_snapshot.room_id,
        position_snapshot.enemy_chain_count,
        position_snapshot.active_enemy_count,
        format_vec3_short(position_snapshot.player_position).c_str(),
        position_snapshot.camera_valid ? "yes" : "no");
    return buffer;
}

void capture_player_resource_snapshot(uPlayer* player) {
    __try {
        if (!player) {
            return;
        }

        position_snapshot.player_current_style = player->currentStyle;
        position_snapshot.player_hp = player->damageStruct.HP;
        position_snapshot.player_hp_max = player->damageStruct.HPMax;
        position_snapshot.player_hp_taken = player->damageStruct.HPTaken;
        position_snapshot.player_prev_damage_resist = player->damageStruct.prevDamageResist;
        position_snapshot.player_dt = player->DT;
        position_snapshot.player_max_dt = player->maxDT;
        position_snapshot.player_dt_active = player->dtActive;
        position_snapshot.player_exceed_level = player->exceedLevel;
        position_snapshot.player_exceed_timer = player->exceedTimer;
        position_snapshot.player_guard_req1 = player->guardReq1;
        position_snapshot.player_guard_req2 = player->guardReq2;
        position_snapshot.player_guard_timer = player->guardTimer;
        position_snapshot.player_revenge_gauge = player->revengeGauge;
        position_snapshot.player_disaster_gauge = player->disasterGauge;
        position_snapshot.player_dreadnaught = player->dreadnaught;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return;
    }
}

void clear_player_input_snapshot(uPlayer* player) {
    if (!player) {
        return;
    }

    std::fill(std::begin(player->inputHold), std::end(player->inputHold), 0);
    std::fill(std::begin(player->inputPress), std::end(player->inputPress), 0);
    std::fill(std::begin(player->inputRelease), std::end(player->inputRelease), 0);
    player->buttonInputRaw = 0;
    player->stickInputRaw = 0;
    player->stickDirection = 0.0f;
    player->stickPastThreshold = false;
    player->bufferedJump = 0;
    player->tiltForward = false;
    player->tiltBack = false;
    player->tiltBackForward = false;
    std::fill_n(KeyboardMacro::last_buttons, 4, 0);
    KeyboardMacro::macro_exceed_active = false;
    KeyboardMacro::macro_exceed_latch_ticks = 0;
    KeyboardMacro::clear_input_frames = 2;
    KeyboardMacro::input_active = KeyboardMacro::clear_input_frames > 0 || (KeyboardMacro::mod_enabled && KeyboardMacro::playback_enabled);
}

void apply_player_resource_snapshot(uPlayer* player) {
    if (!restore_resources_snapshot || !player) {
        return;
    }

    clear_player_input_snapshot(player);

    player->currentStyle = position_snapshot.player_current_style;
    player->damageStruct.HP = position_snapshot.player_hp;
    player->damageStruct.HPMax = position_snapshot.player_hp_max;
    player->damageStruct.HPTaken = position_snapshot.player_hp_taken;
    player->damageStruct.prevDamageResist = position_snapshot.player_prev_damage_resist;
    player->DT = position_snapshot.player_dt;
    player->maxDT = position_snapshot.player_max_dt;
    player->dtActive = position_snapshot.player_dt_active;
    player->dtOutfit = position_snapshot.player_dt_active;
    player->exceedLevel = position_snapshot.player_exceed_level;
    player->exceedTimer = position_snapshot.player_exceed_timer;
    player->guardReq1 = position_snapshot.player_guard_req1;
    player->guardReq2 = position_snapshot.player_guard_req2;
    player->guardTimer = position_snapshot.player_guard_timer;
    player->revengeGauge = position_snapshot.player_revenge_gauge;
    player->disasterGauge = position_snapshot.player_disaster_gauge;
    player->dreadnaught = position_snapshot.player_dreadnaught;
}

bool capture_position_snapshot_unsafe() {
    if (!keyboard_macro_gameplay_ready()) {
        set_playback_status("Battle Snapshot capture failed: gameplay objects are rebuilding.");
        return false;
    }

    auto* player = get_local_player_safe();
    if (!player) {
        set_playback_status("Battle Snapshot capture failed: player is unavailable.");
        return false;
    }

    auto* mediator = get_s_mediator_safe();
    if (!mediator) {
        set_playback_status("Battle Snapshot capture failed: mediator is unavailable.");
        return false;
    }

    const auto current_enemies = collect_enemy_chain();

    position_snapshot.valid = true;
    position_snapshot.mission_id = mediator->missionID;
    position_snapshot.room_id = mediator->roomID;
    position_snapshot.mediator_enemy_count0 = mediator->enemyCount[0];
    position_snapshot.mediator_enemy_count1 = mediator->enemyCount[1];
    position_snapshot.mediator_enemy_count2 = mediator->enemyCount[2];
    position_snapshot.enemy_chain_count = (uint32_t)current_enemies.size();
    position_snapshot.active_enemy_count = 0;
    position_snapshot.locked_enemy_index = -1;
    position_snapshot.player_locked_on = false;
    position_snapshot.enemies.clear();
    position_snapshot.enemies.reserve(current_enemies.size());

    for (uint32_t index = 0; index < current_enemies.size(); ++index) {
        uEnemy_Old* enemy = current_enemies[index];

        BattleEnemySnapshot enemy_snapshot{};
        if (!capture_enemy_snapshot(enemy, enemy_snapshot)) {
            position_snapshot.valid = false;
            set_playback_status("Battle Snapshot capture failed: enemy chain changed while capturing.");
            return false;
        }
        if (enemy_snapshot.is_active) {
            ++position_snapshot.active_enemy_count;
        }
        position_snapshot.enemies.push_back(enemy_snapshot);
    }

    position_snapshot.player_position = player->mPos;
    position_snapshot.player_velocity = player->m_d_velocity;
    position_snapshot.player_rotation2 = player->rotation2;
    position_snapshot.player_rotation3 = player->rotation3;
    position_snapshot.player_facing_direction = player->facingDirection;
    position_snapshot.player_ground_inertia_x = player->groundInertiaX;
    position_snapshot.player_inertia_y = player->inertiaY;
    position_snapshot.player_ground_inertia_z = player->groundInertiaZ;
    capture_player_resource_snapshot(player);

    position_snapshot.camera_valid = false;
    position_snapshot.player_camera_valid = false;
    if (auto* camera = get_local_camera_safe()) {
        position_snapshot.camera_valid = true;
        position_snapshot.camera_near_clip = camera->mNearPlane;
        position_snapshot.camera_fov = camera->mFov;
        position_snapshot.camera_position = camera->mCameraPos;
        position_snapshot.camera_up = camera->mCameraUp;
        position_snapshot.camera_lookat = camera->mTargetPos;

        if (auto* player_camera = camera->mpCamPlayer) {
            position_snapshot.player_camera_valid = true;
            position_snapshot.player_camera_position = player_camera->mCameraPos;
            position_snapshot.player_camera_lookat = player_camera->mTargetPos;
            position_snapshot.player_camera_near_clip = player_camera->mNearPlane;
            position_snapshot.player_camera_fov = player_camera->mFov;
        }
    }

    set_playback_status("Battle Snapshot captured.");
    return true;
}

bool capture_position_snapshot() {
    __try {
        return capture_position_snapshot_unsafe();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        position_snapshot.valid = false;
        strncpy_s(KeyboardMacro::playback_status, "Battle Snapshot capture failed: gameplay objects changed while capturing.", _TRUNCATE);
        return false;
    }
}

bool apply_position_snapshot_unsafe(bool update_status, bool include_resources) {
    if (!position_snapshot.valid) {
        if (update_status) {
            set_playback_status("Battle Snapshot load failed: no state captured.");
        }
        return false;
    }

    if (!keyboard_macro_gameplay_ready()) {
        if (update_status) {
            set_playback_status("Battle Snapshot load failed: gameplay objects are rebuilding.");
        }
        return false;
    }

    auto* player = get_local_player_safe();
    if (!player) {
        if (update_status) {
            set_playback_status("Battle Snapshot load failed: player is unavailable.");
        }
        return false;
    }

    auto* mediator = get_s_mediator_safe();
    if (!mediator) {
        if (update_status) {
            set_playback_status("Battle Snapshot load failed: mediator is unavailable.");
        }
        return false;
    }

    if (mediator->missionID != position_snapshot.mission_id || mediator->roomID != position_snapshot.room_id) {
        if (update_status) {
            char buffer[160]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Battle Snapshot load failed: mission/room is %u/%u, state expects %u/%u.",
                mediator->missionID,
                mediator->roomID,
                position_snapshot.mission_id,
                position_snapshot.room_id);
            set_playback_status(buffer);
        }
        return false;
    }

    const auto current_enemies = collect_enemy_chain();
    if (current_enemies.size() != position_snapshot.enemies.size()) {
        if (update_status) {
            char buffer[160]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Battle Snapshot load failed: enemy chain has %u entries, state expects %u.",
                (uint32_t)current_enemies.size(),
                (uint32_t)position_snapshot.enemies.size());
            set_playback_status(buffer);
        }
        return false;
    }

    for (uint32_t index = 0; index < current_enemies.size(); ++index) {
        uEnemy_Old* current_enemy = current_enemies[index];
        const auto& enemy_snapshot = position_snapshot.enemies[index];
        if (reinterpret_cast<uintptr_t>(current_enemy) != enemy_snapshot.ptr || (int)current_enemy->ID != enemy_snapshot.id) {
            if (update_status) {
                char buffer[192]{};
                std::snprintf(
                    buffer,
                    sizeof(buffer),
                    "Battle Snapshot load failed: enemy %u mismatch. Current %p id %d, state %p id %d.",
                    index,
                    (void*)current_enemy,
                    current_enemy ? (int)current_enemy->ID : -1,
                    (void*)enemy_snapshot.ptr,
                    enemy_snapshot.id);
                set_playback_status(buffer);
            }
            return false;
        }
    }

    if (update_status) {
        clear_player_input_snapshot(player);
    }

    player->mPos = position_snapshot.player_position;
    player->m_d_velocity = position_snapshot.player_velocity;
    player->rotation2 = position_snapshot.player_rotation2;
    player->rotation3 = position_snapshot.player_rotation3;
    player->facingDirection = position_snapshot.player_facing_direction;
    player->groundInertiaX = position_snapshot.player_ground_inertia_x;
    player->inertiaY = position_snapshot.player_inertia_y;
    player->groundInertiaZ = position_snapshot.player_ground_inertia_z;
    for (uint32_t index = 0; index < current_enemies.size(); ++index) {
        apply_enemy_stable_snapshot(current_enemies[index], position_snapshot.enemies[index]);
    }

    if (include_resources) {
        apply_player_resource_snapshot(player);
        if (restore_resources_snapshot) {
            for (uint32_t index = 0; index < current_enemies.size(); ++index) {
                apply_enemy_combat_snapshot(current_enemies[index], position_snapshot.enemies[index]);
            }
        }
    }

    if (position_snapshot.camera_valid) {
        if (auto* camera = get_local_camera_safe()) {
            camera->mNearPlane = position_snapshot.camera_near_clip;
            camera->mFov = position_snapshot.camera_fov;
            camera->mCameraPos = position_snapshot.camera_position;
            camera->mCameraUp = position_snapshot.camera_up;
            camera->mTargetPos = position_snapshot.camera_lookat;

            if (position_snapshot.player_camera_valid) {
                if (auto* player_camera = camera->mpCamPlayer) {
                    player_camera->mCameraPos = position_snapshot.player_camera_position;
                    player_camera->mTargetPos = position_snapshot.player_camera_lookat;
                    player_camera->mNearPlane = position_snapshot.player_camera_near_clip;
                    player_camera->mFov = position_snapshot.player_camera_fov;
                }
            }
        }
    }

    if (update_status) {
        if (restore_resources_snapshot) {
            set_playback_status("Snapshot loaded with resources.");
        }
        else {
            set_playback_status("Battle Snapshot loaded.");
        }
    }
    return true;
}

bool apply_position_snapshot(bool update_status, bool include_resources) {
    __try {
        return apply_position_snapshot_unsafe(update_status, include_resources);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        position_snapshot_load_ticks = 0;
        if (update_status) {
            strncpy_s(KeyboardMacro::playback_status, "Battle Snapshot load failed: gameplay objects changed while loading.", _TRUNCATE);
        }
        return false;
    }
}

bool load_position_snapshot() {
    if (!apply_position_snapshot(true, true)) {
        position_snapshot_load_ticks = 0;
        return false;
    }

    position_snapshot_load_ticks = POSITION_SNAPSHOT_LOAD_TICKS;
    return true;
}

bool set_screen_pause(bool paused) {
    auto* work_rate = get_work_rate_safe();
    if (!work_rate) {
        ++KeyboardMacro::screen_pause_fail_count;
        return false;
    }

    __try {
        if (paused) {
            if (!KeyboardMacro::screen_pause_active) {
                KeyboardMacro::screen_pause_restore_speed = work_rate->global_speed;
                KeyboardMacro::screen_pause_restore_valid = true;
            }
            work_rate->global_speed = 0.0f;
            WorkRate::hotkey_paused = true;
            KeyboardMacro::screen_pause_active = true;
            ++KeyboardMacro::screen_pause_request_count;
            return true;
        }

        if (KeyboardMacro::screen_pause_restore_valid) {
            work_rate->global_speed = KeyboardMacro::screen_pause_restore_speed;
        }
        else {
            work_rate->global_speed = 1.0f;
        }
        WorkRate::hotkey_paused = false;
        KeyboardMacro::screen_pause_active = false;
        KeyboardMacro::screen_pause_restore_valid = false;
        ++KeyboardMacro::screen_pause_request_count;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        ++KeyboardMacro::screen_pause_fail_count;
        return false;
    }
}

void apply_screen_pause_action(const KeyboardMacroFrame& frame) {
    if (frame.screen_pause_toggle) {
        set_screen_pause(!KeyboardMacro::screen_pause_active);
    }
    if (frame.screen_pause) {
        set_screen_pause(true);
    }
    if (frame.screen_resume) {
        set_screen_pause(false);
    }
}

void apply_force_style_action(const KeyboardMacroFrame& frame) {
    if (!frame.force_style || frame.forced_style < 0 || frame.forced_style > 4) {
        return;
    }

    auto* player = get_local_player_safe();
    if (!player) {
        return;
    }

    __try {
        *reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(player) + UPLAYER_CURRENT_STYLE_OFFSET) = frame.forced_style;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return;
    }
}

bool is_player_grounded(uPlayer* player) {
    if (!player) {
        return false;
    }

    if (player->grounded != 0 || player->grounded2) {
        return true;
    }

    return player->collisionSettings && player->collisionSettings->mLand != 0;
}

bool evaluate_wait_condition(const KeyboardMacroFrame& frame) {
    if (frame.wait_condition == MACRO_WAIT_NONE) {
        return true;
    }

    if (frame.wait_condition == MACRO_WAIT_ENEMY_STEP) {
        EnemyStepDisplay::update_jc_possible();
        return EnemyStepDisplay::jc_possible;
    }

    auto* player = get_local_player_safe();
    if (!player) {
        return false;
    }

    __try {
        switch (frame.wait_condition) {
        case MACRO_WAIT_GROUNDED:
            return is_player_grounded(player);
        case MACRO_WAIT_AIRBORNE:
            return !is_player_grounded(player);
        case MACRO_WAIT_STYLE:
            return player->currentStyle == frame.wait_arg;
        case MACRO_WAIT_CAN_EXCEED:
            return player->canExceed != 0;
        case MACRO_WAIT_HITSTOP:
            return player->hitstop || player->hitstopTimer > 0.0f;
        case MACRO_WAIT_HIT_CONFIRMED:
            return evaluate_hit_confirmed_wait_condition(frame);
        case MACRO_WAIT_LOCKED_ON:
            return player->lockedOn;
        default:
            return false;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

bool should_advance_wait_frame(KeyboardMacroFrame& frame) {
    if (frame.wait_condition == MACRO_WAIT_NONE) {
        return true;
    }

    if (evaluate_wait_condition(frame)) {
        frame.wait_elapsed_ticks = 0;
        return true;
    }

    if (frame.wait_max_ticks != 0 && frame.wait_elapsed_ticks >= frame.wait_max_ticks) {
        frame.wait_elapsed_ticks = 0;
        set_playback_status("Conditional wait timed out; continuing playback.");
        return true;
    }

    ++frame.wait_elapsed_ticks;
    return false;
}

bool is_absolute_path(const std::string& path) {
    if (path.size() >= 3 && std::isalpha((unsigned char)path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }

    return path.rfind("\\\\", 0) == 0 || path.rfind("//", 0) == 0;
}

std::string get_game_directory() {
    char exe_path[MAX_PATH]{};
    if (GetModuleFileNameA(nullptr, exe_path, sizeof(exe_path)) == 0) {
        return {};
    }

    std::string directory = exe_path;
    const auto slash = directory.find_last_of("\\/");
    if (slash == std::string::npos) {
        return {};
    }

    return directory.substr(0, slash);
}

std::string resolve_playback_path_text(const char* raw_path) {
    std::string path = trim_copy(raw_path ? raw_path : "");
    if (path.empty() || is_absolute_path(path)) {
        return path;
    }

    const auto game_directory = get_game_directory();
    if (game_directory.empty()) {
        return path;
    }

    return game_directory + "\\" + path;
}

bool is_mouse_vkey(uint32_t vkey) {
    return vkey == VK_LBUTTON || vkey == VK_RBUTTON || vkey == VK_MBUTTON || vkey == VK_XBUTTON1 || vkey == VK_XBUTTON2;
}

bool is_modifier_only_vkey(uint32_t vkey) {
    return vkey == VK_SHIFT || vkey == VK_CONTROL || vkey == VK_MENU ||
        vkey == VK_LSHIFT || vkey == VK_RSHIFT ||
        vkey == VK_LCONTROL || vkey == VK_RCONTROL ||
        vkey == VK_LMENU || vkey == VK_RMENU;
}

bool is_scannable_vkey(uint32_t vkey) {
    return vkey > 0 && vkey < 256 && !is_mouse_vkey(vkey) && !is_modifier_only_vkey(vkey);
}

bool is_down_vkey(uint32_t vkey) {
    return (GetAsyncKeyState((int)vkey) & 0x8000) != 0;
}

bool is_modifier_bind_down(uint32_t vkey) {
    switch (vkey) {
    case VK_SHIFT:
        return is_down_vkey(VK_SHIFT) || is_down_vkey(VK_LSHIFT) || is_down_vkey(VK_RSHIFT);
    case VK_CONTROL:
        return is_down_vkey(VK_CONTROL) || is_down_vkey(VK_LCONTROL) || is_down_vkey(VK_RCONTROL);
    case VK_MENU:
        return is_down_vkey(VK_MENU) || is_down_vkey(VK_LMENU) || is_down_vkey(VK_RMENU);
    default:
        return is_down_vkey(vkey);
    }
}

bool any_hotkey_modifier_down() {
    return is_modifier_bind_down(VK_CONTROL) || is_modifier_bind_down(VK_SHIFT) || is_modifier_bind_down(VK_MENU);
}

std::vector<uint32_t> collect_hotkey_binds(uint32_t trigger_vkey) {
    std::vector<uint32_t> binds{};

    if (is_down_vkey(VK_CONTROL)) {
        binds.push_back(VK_CONTROL);
    }
    if (is_down_vkey(VK_SHIFT)) {
        binds.push_back(VK_SHIFT);
    }
    if (is_down_vkey(VK_MENU)) {
        binds.push_back(VK_MENU);
    }

    binds.push_back(trigger_vkey);
    return binds;
}

std::string vkey_label(uint32_t vkey) {
    if (vkey >= '0' && vkey <= '9') {
        return std::string(1, (char)vkey);
    }

    if (vkey >= 'A' && vkey <= 'Z') {
        return std::string(1, (char)vkey);
    }

    if (vkey >= VK_F1 && vkey <= VK_F24) {
        return "F" + std::to_string((vkey - VK_F1) + 1);
    }

    switch (vkey) {
    case VK_INSERT:
        return "Insert";
    case VK_DELETE:
        return "Delete";
    case VK_HOME:
        return "Home";
    case VK_END:
        return "End";
    case VK_PRIOR:
        return "PageUp";
    case VK_NEXT:
        return "PageDown";
    case VK_SPACE:
        return "Space";
    case VK_TAB:
        return "Tab";
    case VK_RETURN:
        return "Enter";
    case VK_BACK:
        return "Backspace";
    case VK_ESCAPE:
        return "Escape";
    case VK_SHIFT:
        return "Shift";
    case VK_CONTROL:
        return "Ctrl";
    case VK_MENU:
        return "Alt";
    case VK_LSHIFT:
        return "LShift";
    case VK_RSHIFT:
        return "RShift";
    case VK_LCONTROL:
        return "LCtrl";
    case VK_RCONTROL:
        return "RCtrl";
    case VK_LMENU:
        return "LAlt";
    case VK_RMENU:
        return "RAlt";
    case VK_PAUSE:
        return "Pause";
    case VK_SCROLL:
        return "ScrollLock";
    case VK_VOLUME_MUTE:
        return "VolumeMute";
    case VK_VOLUME_DOWN:
        return "VolumeDown";
    case VK_VOLUME_UP:
        return "VolumeUp";
    case VK_MEDIA_PLAY_PAUSE:
        return "MediaPlayPause";
    default:
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "VK 0x%02X", vkey);
        return buffer;
    }
}

std::string hotkey_binds_label(const std::vector<uint32_t>& binds) {
    if (binds.empty()) {
        return "Unbound";
    }

    std::ostringstream stream{};
    for (size_t i = 0; i < binds.size(); ++i) {
        if (i > 0) {
            stream << " + ";
        }
        stream << vkey_label(binds[i]);
    }
    return stream.str();
}

bool capture_pending_hotkey(uint32_t vkey) {
    if (capture_hotkey_target == 0) {
        return false;
    }

    if (!is_scannable_vkey(vkey)) {
        return false;
    }

    const auto binds = collect_hotkey_binds(vkey);
    if (keyboard_macro_instance && capture_hotkey_target >= 1 && capture_hotkey_target <= keyboard_macro_instance->m_hotkeys.size()) {
        auto& hotkey = keyboard_macro_instance->m_hotkeys[capture_hotkey_target - 1];
        hotkey->m_binds = binds;
        hotkey->m_setting = false;

        if (capture_hotkey_target == 1) {
            KeyboardMacro::reload_vkey = binds.back();
        }
        else if (capture_hotkey_target == 2) {
            KeyboardMacro::restart_vkey = binds.back();
        }
        else if (capture_hotkey_target == 3) {
            KeyboardMacro::stop_vkey = binds.back();
        }
        else if (capture_hotkey_target == 4) {
            KeyboardMacro::capture_snapshot_vkey = binds.back();
        }
        else if (capture_hotkey_target == 5) {
            KeyboardMacro::load_snapshot_vkey = binds.back();
        }

        update_config_hotkey_vkeys(keyboard_macro_instance->m_hotkeys);

        const auto key_label = hotkey_binds_label(binds);
        char message[128]{};
        std::snprintf(message, sizeof(message), "Captured hotkey: %s.", key_label.c_str());
        set_playback_status(message);
        capture_hotkey_target = 0;
        return true;
    }

    set_playback_status("Hotkey capture failed: Macro UI instance is not ready.");
    capture_hotkey_target = 0;
    return true;
}

std::string clip_display_label(const KeyboardMacroClip& clip, uint32_t clip_index) {
    std::ostringstream stream{};
    stream << (clip_index + 1) << ". ";
    if (!clip.name.empty()) {
        stream << clip.name;
    }
    else {
        stream << "Clip " << (clip_index + 1);
    }

    if (!clip.hotkey_binds.empty()) {
        stream << " [" << hotkey_binds_label(clip.hotkey_binds) << "]";
    }

    return stream.str();
}

bool hotkey_binds_match_message(const std::vector<uint32_t>& binds, WPARAM key) {
    if (binds.empty() || binds.back() != (uint32_t)key) {
        return false;
    }

    if (binds.size() == 1 && any_hotkey_modifier_down()) {
        return false;
    }

    for (size_t i = 0; i + 1 < binds.size(); ++i) {
        const auto bind = binds[i];
        if (bind >= 256 || !is_modifier_bind_down(bind)) {
            return false;
        }
    }

    return true;
}

void update_config_hotkey_vkeys(const std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys) {
    if (hotkeys.size() < 5) {
        return;
    }

    if (!hotkeys[0]->m_binds.empty()) {
        KeyboardMacro::reload_vkey = hotkeys[0]->m_binds.back();
    }
    if (!hotkeys[1]->m_binds.empty()) {
        KeyboardMacro::restart_vkey = hotkeys[1]->m_binds.back();
    }
    if (!hotkeys[2]->m_binds.empty()) {
        KeyboardMacro::stop_vkey = hotkeys[2]->m_binds.back();
    }
    if (!hotkeys[3]->m_binds.empty()) {
        KeyboardMacro::capture_snapshot_vkey = hotkeys[3]->m_binds.back();
    }
    if (!hotkeys[4]->m_binds.empty()) {
        KeyboardMacro::load_snapshot_vkey = hotkeys[4]->m_binds.back();
    }
}

bool hotkey_message_matches(const utility::Hotkey& hotkey, WPARAM key) {
    if (hotkey.m_setting || hotkey.m_is_gamepad || hotkey.m_binds.empty()) {
        return false;
    }

    if (hotkey.m_binds.back() != (uint32_t)key) {
        return false;
    }

    if (hotkey.m_binds.size() == 1 && any_hotkey_modifier_down()) {
        return false;
    }

    for (size_t i = 0; i + 1 < hotkey.m_binds.size(); ++i) {
        const auto bind = hotkey.m_binds[i];
        if (bind >= 256 || !is_modifier_bind_down(bind)) {
            return false;
        }
    }

    return true;
}

bool hotkey_is_default_or_unbound(const utility::Hotkey& hotkey, uint32_t default_vkey) {
    if (hotkey.m_setting) {
        return false;
    }

    return hotkey.m_binds.empty() || (hotkey.m_binds.size() == 1 && hotkey.m_binds[0] == default_vkey);
}

}

bool KeyboardMacro::mod_enabled = false;
bool KeyboardMacro::playback_enabled = false;
bool KeyboardMacro::input_active = false;
uint32_t KeyboardMacro::last_buttons[4] = {};
uint32_t KeyboardMacro::playback_frame_index = 0;
uint32_t KeyboardMacro::clear_input_frames = 0;
uint32_t KeyboardMacro::reload_vkey = DEFAULT_RELOAD_VKEY;
uint32_t KeyboardMacro::restart_vkey = DEFAULT_RESTART_VKEY;
uint32_t KeyboardMacro::stop_vkey = DEFAULT_STOP_VKEY;
uint32_t KeyboardMacro::capture_snapshot_vkey = DEFAULT_CAPTURE_SNAPSHOT_VKEY;
uint32_t KeyboardMacro::load_snapshot_vkey = DEFAULT_LOAD_SNAPSHOT_VKEY;
uint32_t KeyboardMacro::playback_slot = PLAYBACK_SLOT_MAIN;
uint32_t KeyboardMacro::selected_clip_index = 0;
uint32_t KeyboardMacro::loaded_clip_index = INVALID_CLIP_INDEX;
bool KeyboardMacro::macro_exceed_active = false;
uint32_t KeyboardMacro::macro_exceed_latch_ticks = 0;
bool KeyboardMacro::screen_pause_active = false;
bool KeyboardMacro::screen_pause_restore_valid = false;
float KeyboardMacro::screen_pause_restore_speed = 1.0f;
uint32_t KeyboardMacro::screen_pause_request_count = 0;
uint32_t KeyboardMacro::screen_pause_fail_count = 0;
char KeyboardMacro::playback_path[260] = "keyboard_macro.txt";
char KeyboardMacro::loaded_playback_path[260] = "";
char KeyboardMacro::playback_status[256] = "No Macro file loaded.";
std::vector<KeyboardMacroClip> KeyboardMacro::playback_clips{};
std::vector<KeyboardMacroFrame> KeyboardMacro::playback_frames{};

std::optional<std::string> KeyboardMacro::on_initialize() {
    keyboard_macro_instance = this;
    ensure_keyboard_macro_hotkeys(m_hotkeys);
    return Mod::on_initialize();
}

void __stdcall KeyboardMacro::on_pad_update_tick(cPeripheral* peripheral) {
    if (macro_exceed_latch_ticks > 0) {
        --macro_exceed_latch_ticks;
        update_macro_exceed_active_from_latch();
    }

    if (keyboard_macro_instance) {
        keyboard_macro_instance->check_hotkeys();
    }
    else {
        poll_raw_keyboard(mod_enabled);
    }

}

void __stdcall KeyboardMacro::on_player_pad_update(cPeripheral* peripheral) {
    if (!peripheral || !input_active) {
        return;
    }

    uint32_t player_index = *(uint8_t*)((uintptr_t)peripheral + 0x95);
    if (player_index >= 4) {
        player_index = 0;
    }

    last_player_peripheral = peripheral;
    last_player_index = player_index;
    write_test_input(peripheral, player_index);
}

uint32_t __stdcall KeyboardMacro::on_player_input_tick(uPlayer* player, void* input_state, uint32_t inputs) {
    (void)input_state;
    if (!player || !input_state || !macro_exceed_active) {
        return inputs;
    }

    __try {
        if (player->controllerID != 1) {
            return inputs;
        }

        *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(player) + UPLAYER_EXCEED_INPUT_OFFSET) = 1.0f;
        player->isExceeding = 1;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return inputs;
    }
    return inputs;
}

void KeyboardMacro::write_test_input(cPeripheral* peripheral, uint32_t player_index) {
    uint32_t buttons = 0;
    KeyboardMacroFrame macro_frame{};
    const bool was_clearing_input = clear_input_frames > 0;

    if (was_clearing_input) {
        buttons = 0;
        --clear_input_frames;
    }
    else if (playback_enabled) {
        if (playback_frames.empty()) {
            buttons = 0;
        }
        else {
            begin_playback_timer_if_pending();

            bool found_frame = false;
            while (playback_frame_index < playback_frames.size()) {
                auto& candidate_frame = playback_frames[playback_frame_index];
                if (candidate_frame.wait_condition != MACRO_WAIT_NONE) {
                    if (should_advance_wait_frame(candidate_frame)) {
                        ++playback_frame_index;
                        continue;
                    }
                }
                else {
                    ++playback_frame_index;
                }

                macro_frame = candidate_frame;
                buttons = macro_frame.buttons;
                found_frame = true;
                break;
            }

            if (!found_frame) {
                playback_enabled = false;
                finalize_playback_timer();
                buttons = 0;
                set_playback_status("Playback finished.");
                update_input_active();
            }
        }
    }

    write_base_player_input_snapshot(peripheral);

    auto* player = get_local_player_safe();
    const bool is_nero = is_nero_player_safe(player);
    const bool l2_requested = (buttons & PAD_BUTTON_L2) != 0;
    const bool exceed_requested = macro_frame.exceed || (is_nero && l2_requested);

    const uint32_t base_buttons = peripheral->mPadBtnOn;
    uint32_t injected_buttons = buttons;
    if (is_nero && l2_requested) {
        injected_buttons &= ~PAD_BUTTON_L2;
    }
    const uint32_t output_buttons = base_buttons | injected_buttons;
    const uint32_t previous_buttons = last_buttons[player_index];

    peripheral->mPadBtnOn = output_buttons;
    peripheral->mPadBtnTrg = output_buttons & ~previous_buttons;
    peripheral->mPadBtnRel = previous_buttons & ~output_buttons;
    apply_button_press_values(peripheral, output_buttons);

    if (exceed_requested) {
        queue_macro_exceed_request();
    }

    if (macro_frame.has_left_analog) {
        peripheral->mAnlgL.x = clamp_analog((int)peripheral->mAnlgL.x + macro_frame.left_x);
        peripheral->mAnlgL.y = clamp_analog((int)peripheral->mAnlgL.y + macro_frame.left_y);
        update_analog_info_for_macro(&peripheral->mAnlgL);
        peripheral->mHoldAnlgL = peripheral->mAnlgL;
        peripheral->mIsHold = false;
    }

    apply_screen_pause_action(macro_frame);
    apply_force_style_action(macro_frame);

    last_buttons[player_index] = output_buttons;
    update_input_active();
}

void KeyboardMacro::reset_input_state() {
    std::fill_n(last_buttons, 4, 0);
    macro_exceed_active = false;
    macro_exceed_latch_ticks = 0;
    playback_frame_index = 0;
    for (auto& frame : playback_frames) {
        frame.wait_elapsed_ticks = 0;
    }
    reset_hit_confirmed_wait_state();
    update_input_active();
}

static bool load_clip_into_playback_state(uint32_t clip_index) {
    if (clip_index >= KeyboardMacro::playback_clips.size()) {
        KeyboardMacro::playback_frames.clear();
        KeyboardMacro::loaded_clip_index = INVALID_CLIP_INDEX;
        return false;
    }

    const auto& clip = KeyboardMacro::playback_clips[clip_index];
    KeyboardMacro::playback_frames = clip.frames;
    KeyboardMacro::playback_frame_index = 0;
    KeyboardMacro::loaded_clip_index = clip_index;
    return !KeyboardMacro::playback_frames.empty();
}

bool KeyboardMacro::load_playback_file() {
    playback_frames.clear();
    playback_clips.clear();
    playback_frame_index = 0;
    loaded_clip_index = INVALID_CLIP_INDEX;
    loaded_playback_path[0] = '\0';

    const std::string file_path = resolve_playback_path();
    std::ifstream file{ utility::widen(file_path) };
    if (!file) {
        set_playback_status("Could not open Macro file.");
        return false;
    }

    auto clear_failed_load = [&]() {
        playback_frames.clear();
        playback_clips.clear();
        loaded_clip_index = INVALID_CLIP_INDEX;
        loaded_playback_path[0] = '\0';
    };

    auto fail_with_message = [&](const char* format, uint32_t value) {
        char message[256]{};
        std::snprintf(message, sizeof(message), format, value);
        set_playback_status(message);
        clear_failed_load();
        return false;
    };

    auto ensure_clip_name = [](KeyboardMacroClip& clip, uint32_t clip_index) {
        if (clip.name.empty()) {
            clip.name = "Clip " + std::to_string(clip_index + 1);
        }
    };

    std::string line{};
    uint32_t line_number = 0;
    HeldMacroInput held_input{};
    playback_clips.emplace_back();
    playback_clips.back().name = "Full Script";
    playback_clips.back().header_line = 1;
    while (std::getline(file, line)) {
        ++line_number;

        const auto comment = line.find_first_of("#;");
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        line = trim_copy(line);
        if (line.empty()) {
            continue;
        }

        if (line[0] == '[') {
            const auto close = line.find(']');
            if (close == std::string::npos) {
                return fail_with_message("Line %u: invalid section header.", line_number);
            }

            auto& previous_clip = playback_clips.back();
            if (playback_clips.size() > 1 || !previous_clip.frames.empty()) {
                if (previous_clip.frames.empty()) {
                    return fail_with_message("Line %u: section has no playback ticks.", previous_clip.header_line);
                }
            }
            else if (playback_clips.size() == 1 && previous_clip.frames.empty() && previous_clip.hotkey_binds.empty() &&
                previous_clip.name == "Full Script") {
                playback_clips.clear();
            }

            const auto hotkey_expression = trim_copy(line.substr(1, close - 1));
            std::vector<uint32_t> clip_binds{};
            if (!parse_hotkey_binds(hotkey_expression, clip_binds)) {
                return fail_with_message("Line %u: invalid section hotkey.", line_number);
            }

            for (const auto& clip : playback_clips) {
                if (!clip_binds.empty() && clip.hotkey_binds == clip_binds) {
                    return fail_with_message("Line %u: duplicate section hotkey.", line_number);
                }
            }

            playback_clips.emplace_back();
            auto& new_clip = playback_clips.back();
            new_clip.hotkey_binds = std::move(clip_binds);
            new_clip.name = trim_copy(line.substr(close + 1));
            new_clip.header_line = line_number;
            ensure_clip_name(new_clip, (uint32_t)playback_clips.size() - 1);
            held_input = {};
            continue;
        }

        std::istringstream stream{ line };
        std::string first_token{};
        stream >> first_token;
        auto& current_clip = playback_clips.back();

        uint32_t frame_count = 0;
        if (parse_integer(first_token, frame_count)) {
            if (frame_count == 0) {
                return fail_with_message("Line %u: invalid frame count.", line_number);
            }

            std::string button_expression{};
            std::getline(stream, button_expression);
            button_expression = trim_copy(button_expression);
            if (button_expression.empty()) {
                button_expression = "NONE";
            }

            KeyboardMacroFrame frame{};
            if (!parse_playback_command(button_expression, frame)) {
                return fail_with_message("Line %u: invalid input token.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, frame_count, frame)) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        const auto command = normalize_button_token(first_token);
        if (command == "WAIT") {
            std::string frame_token{};
            stream >> frame_token;
            if (!parse_integer(frame_token, frame_count) || frame_count == 0) {
                return fail_with_message("Line %u: invalid WAIT frame count.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, frame_count, frame_from_held(held_input))) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "WAIT_UNTIL" || command == "WAITUNTIL" || command == "UNTIL") {
            uint32_t condition = MACRO_WAIT_NONE;
            int arg = 0;
            uint32_t max_ticks = 0;
            if (!parse_wait_condition(stream, condition, arg, max_ticks)) {
                return fail_with_message("Line %u: invalid WAIT_UNTIL condition.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_wait_condition(held_input, condition, arg, max_ticks))) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "HOLD" || command == "PRESS") {
            std::string expression{};
            std::getline(stream, expression);
            expression = trim_copy(expression);
            ParsedMacroInput input{};
            if (expression.empty() || !parse_macro_input(expression, input)) {
                return fail_with_message("Line %u: invalid HOLD input.", line_number);
            }

            hold_input(held_input, input);
            continue;
        }

        if (command == "RELEASE" || command == "REL") {
            std::string expression{};
            std::getline(stream, expression);
            expression = trim_copy(expression);
            const auto normalized_expression = normalize_button_token(expression);
            if (normalized_expression == "ALL" || normalized_expression == "NONE" || normalized_expression == "0") {
                held_input = {};
                continue;
            }

            ParsedMacroInput input{};
            if (expression.empty() || !parse_macro_input(expression, input)) {
                return fail_with_message("Line %u: invalid RELEASE input.", line_number);
            }

            release_input(held_input, input);
            continue;
        }

        if (command == "CLEAR") {
            held_input = {};
            continue;
        }

        if (command == "TAP") {
            std::string input_expression{};
            std::string ticks_token{};
            stream >> input_expression >> ticks_token;

            uint32_t ticks = 1;
            if (input_expression.empty() || (!ticks_token.empty() && !parse_integer(ticks_token, ticks))) {
                return fail_with_message("Line %u: invalid TAP command.", line_number);
            }

            if (!append_tap_macro(current_clip.frames, held_input, input_expression, ticks)) {
                set_playback_status("Macro file is too long or TAP input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "DIR" || command == "DIRECTION") {
            std::string direction_expression{};
            std::string input_expression{};
            std::string ticks_token{};
            stream >> direction_expression >> input_expression >> ticks_token;

            uint32_t ticks = 34;
            if (direction_expression.empty() || input_expression.empty() || (!ticks_token.empty() && !parse_integer(ticks_token, ticks))) {
                return fail_with_message("Line %u: invalid DIR command.", line_number);
            }

            if (!append_direction_button_macro(current_clip.frames, held_input, direction_expression, input_expression, ticks)) {
                set_playback_status("Macro file is too long or DIR input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "BACK_FORWARD" || command == "BACKFORWARD" || command == "BF") {
            std::string back_expression{};
            std::string forward_expression{};
            std::string input_expression{};
            stream >> back_expression >> forward_expression >> input_expression;

            if (back_expression.empty() || forward_expression.empty() || input_expression.empty()) {
                return fail_with_message("Line %u: invalid BACK_FORWARD command.", line_number);
            }

            if (!append_back_forward_macro(current_clip.frames, held_input, back_expression, forward_expression, input_expression)) {
                set_playback_status("Macro file is too long or BACK_FORWARD input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "CALIBUR_RIGHT" || command == "CALIBUR_R" || command == "RIGHT_CALIBUR" ||
            command == "HIEN_RIGHT" || command == "HIEN_R") {
            if (!append_back_forward_macro(current_clip.frames, held_input, "MD", "MA", "Y")) {
                set_playback_status("Macro file is too long or CALIBUR_RIGHT input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "CALIBUR_LEFT" || command == "CALIBUR_L" || command == "LEFT_CALIBUR" ||
            command == "HIEN_LEFT" || command == "HIEN_L") {
            if (!append_back_forward_macro(current_clip.frames, held_input, "MA", "MD", "Y")) {
                set_playback_status("Macro file is too long or CALIBUR_LEFT input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "SHUFFLE_RIGHT" || command == "SHUFFLE_R" || command == "RIGHT_SHUFFLE") {
            if (!append_back_forward_macro(current_clip.frames, held_input, "MD", "MA", "Y")) {
                set_playback_status("Macro file is too long or SHUFFLE_RIGHT input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "SHUFFLE_LEFT" || command == "SHUFFLE_L" || command == "LEFT_SHUFFLE") {
            if (!append_back_forward_macro(current_clip.frames, held_input, "MA", "MD", "Y")) {
                set_playback_status("Macro file is too long or SHUFFLE_LEFT input is invalid.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "STYLE" || command == "FORCE_STYLE" || command == "SET_STYLE") {
            std::string style_token{};
            stream >> style_token;

            int style = -1;
            if (!parse_style_name(style_token, style)) {
                return fail_with_message("Line %u: invalid STYLE target.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_forced_style(style))) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "FREEZE" || command == "SCREEN_FREEZE" || command == "SCREEN_PAUSE" ||
            command == "TRAINER_PAUSE" || command == "FRAME_PAUSE" ||
            command == "UNFREEZE" || command == "RESUME" || command == "UNPAUSE" ||
            command == "SCREEN_RESUME" || command == "SCREEN_UNPAUSE" ||
            command == "TOGGLE_FREEZE" || command == "TOGGLE_PAUSE" || command == "SCREEN_PAUSE_TOGGLE") {
            ParsedMacroInput input{};
            if (!parse_macro_input(command, input)) {
                return fail_with_message("Line %u: invalid screen pause command.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_input(input))) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "SET") {
            std::string expression{};
            std::getline(stream, expression);
            expression = trim_copy(expression);
            ParsedMacroInput input{};
            if (expression.empty() || !parse_macro_input(expression, input)) {
                return fail_with_message("Line %u: invalid SET input.", line_number);
            }

            held_input = {};
            hold_input(held_input, input);
            continue;
        }

        {
            return fail_with_message("Line %u: unknown command.", line_number);
        }
    }

    if (playback_clips.empty()) {
        set_playback_status("Macro file has no clips.");
        clear_failed_load();
        return false;
    }

    auto& last_clip = playback_clips.back();
    if (last_clip.frames.empty()) {
        return fail_with_message("Line %u: section has no playback ticks.", last_clip.header_line);
    }

    for (uint32_t clip_index = 0; clip_index < playback_clips.size(); ++clip_index) {
        ensure_clip_name(playback_clips[clip_index], clip_index);
    }

    if (selected_clip_index >= playback_clips.size()) {
        selected_clip_index = 0;
    }

    if (!load_clip_into_playback_state(selected_clip_index)) {
        set_playback_status("Selected Macro clip has no playback ticks.");
        clear_failed_load();
        return false;
    }

    char message[256]{};
    std::snprintf(message, sizeof(message), "Loaded %u clips; selected clip has %u ticks.", (uint32_t)playback_clips.size(), (uint32_t)playback_frames.size());
    set_playback_status(message);
    strncpy_s(loaded_playback_path, file_path.c_str(), _TRUNCATE);

    return !playback_frames.empty();
}

std::string KeyboardMacro::resolve_playback_path() {
    return resolve_playback_path_text(get_selected_playback_path_text().c_str());
}

bool KeyboardMacro::reload_playback_file() {
    const bool was_playing = playback_enabled;

    if (!load_playback_file()) {
        finalize_playback_timer();
        playback_enabled = false;
        update_input_active();
        return false;
    }

    reset_input_state();

    playback_enabled = was_playing;
    if (was_playing) {
        queue_playback_timer_start();
    }
    else {
        clear_playback_timer();
    }
    update_input_active();

    char message[256]{};
    std::snprintf(message, sizeof(message), "Reloaded %u clips; selected clip has %u ticks.", (uint32_t)playback_clips.size(), (uint32_t)playback_frames.size());
    set_playback_status(message);
    return true;
}

void KeyboardMacro::restart_playback_clip(uint32_t clip_index) {
    if (screen_pause_active) {
        set_screen_pause(false);
    }

    if (clip_index >= playback_clips.size() || !load_clip_into_playback_state(clip_index)) {
        playback_enabled = false;
        update_input_active();
        set_playback_status("Selected Macro clip is unavailable.");
        return;
    }

    selected_clip_index = clip_index;
    mod_enabled = true;
    playback_enabled = true;
    reset_input_state();
    queue_playback_timer_start();
    update_input_active();

    const auto clip_label = clip_display_label(playback_clips[clip_index], clip_index);
    char message[256]{};
    std::snprintf(message, sizeof(message), "Playback started: %s.", clip_label.c_str());
    set_playback_status(message);
}

void KeyboardMacro::restart_playback() {
    if (!load_playback_file()) {
        playback_enabled = false;
        update_input_active();
        return;
    }

    restart_playback_clip(selected_clip_index);
}

void KeyboardMacro::stop_all_input() {
    if (screen_pause_active) {
        set_screen_pause(false);
    }

    playback_enabled = false;
    finalize_playback_timer();
    macro_exceed_active = false;
    macro_exceed_latch_ticks = 0;
    playback_frame_index = 0;
    clear_last_peripheral_output();
    clear_input_frames = 4;
    reset_hit_confirmed_wait_state();
    update_input_active();
    set_playback_status("Playback stopped; clearing input.");
}

void KeyboardMacro::on_frame(fmilliseconds& dt) {
    (void)dt;
    if (mod_enabled && !keyboard_macro_gameplay_ready()) {
        suspend_keyboard_macro_runtime_for_transition();
        poll_raw_keyboard(false);
        return;
    }

    if (keyboard_macro_suspended_for_transition && keyboard_macro_gameplay_ready()) {
        keyboard_macro_suspended_for_transition = false;
        set_playback_status("Keyboard Macro ready.");
    }

    check_hotkeys();
    if (position_snapshot_load_ticks > 0) {
        if (apply_position_snapshot(false, false)) {
            --position_snapshot_load_ticks;
        }
        else {
            position_snapshot_load_ticks = 0;
        }
    }
}

void KeyboardMacro::update_input_active() {
    input_active = clear_input_frames > 0 || (mod_enabled && playback_enabled);
}

void KeyboardMacro::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        keyboard_macro_instance = this;
        ensure_keyboard_macro_hotkeys(m_hotkeys);
        ensure_keyboard_macro_hotkey_binds(m_hotkeys);

        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Keyboard Macro"), &mod_enabled)) {
            if (!mod_enabled) {
                stop_all_input();
            } else {
                update_input_active();
            }
        }

        if (mod_enabled) {
            ImGui::Indent(lineIndent);

            refresh_playback_file_choices();
            if (ImGui::BeginCombo(_("Macro File"), get_playback_slot_label(playback_slot))) {
                for (uint32_t slot = 0; slot < playback_file_choices.size(); ++slot) {
                    const bool is_selected = playback_slot == slot;
                    if (ImGui::Selectable(get_playback_slot_label(slot), is_selected)) {
                        playback_slot = slot;
                        set_playback_status("Macro file selected.");
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                const bool custom_selected = playback_slot == PLAYBACK_SLOT_CUSTOM;
                if (ImGui::Selectable(CUSTOM_PLAYBACK_LABEL, custom_selected)) {
                    playback_slot = PLAYBACK_SLOT_CUSTOM;
                    set_playback_status("Custom macro file selected.");
                }
                if (custom_selected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            if (playback_slot == PLAYBACK_SLOT_CUSTOM) {
                ImGui::InputText(_("Custom Macro File"), playback_path, sizeof(playback_path));
            }

            if (!playback_clips.empty()) {
                uint32_t preview_clip_index = selected_clip_index;
                if (preview_clip_index >= playback_clips.size()) {
                    preview_clip_index = 0;
                }

                const auto preview_label = clip_display_label(playback_clips[preview_clip_index], preview_clip_index);
                if (ImGui::BeginCombo(_("Macro Clip"), preview_label.c_str())) {
                    for (uint32_t clip_index = 0; clip_index < playback_clips.size(); ++clip_index) {
                        const bool is_selected = clip_index == selected_clip_index;
                        const auto clip_label  = clip_display_label(playback_clips[clip_index], clip_index);
                        if (ImGui::Selectable(clip_label.c_str(), is_selected)) {
                            selected_clip_index = clip_index;
                            set_playback_status("Macro clip selected.");
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::SeparatorText(_("Hotkeys"));
            if (m_hotkeys.size() >= 5) {
                auto draw_macro_hotkey = [&](const char* action_label, uint32_t target, utility::Hotkey& hotkey) {
                    ImGui::PushID((int)target);
                    const auto label = hotkey_binds_label(hotkey.m_binds);
                    ImGui::Text("%s: %s", action_label, label.c_str());
                    ImGui::SameLine(sameLineWidth);
                    if (ImGui::Button(_("Set"))) {
                        capture_hotkey_target = target;
                        hotkey.m_setting      = true;
                        std::fill_n(raw_key_down, 256, false);
                        set_playback_status("Press the new macro hotkey.");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(_("Clear"))) {
                        hotkey.m_binds   = hotkey.m_default_keys;
                        hotkey.m_setting = false;
                        if (capture_hotkey_target == target) {
                            capture_hotkey_target = 0;
                        }
                        update_config_hotkey_vkeys(m_hotkeys);
                        set_playback_status("Macro hotkey restored to default.");
                    }
                    ImGui::PopID();
                };

                draw_macro_hotkey(_("Reload Macro File"), 1, *m_hotkeys[0]);
                draw_macro_hotkey(_("Play Macro"), 2, *m_hotkeys[1]);
                draw_macro_hotkey(_("Stop Macro / Clear Input"), 3, *m_hotkeys[2]);
                draw_macro_hotkey(_("Capture Snapshot"), 4, *m_hotkeys[3]);
                draw_macro_hotkey(_("Load Snapshot"), 5, *m_hotkeys[4]);

                if (capture_hotkey_target != 0) {
                    ImGui::TextWrapped(_("Capturing hotkey: press a non-modifier key. Ctrl, Shift, and Alt are captured as modifiers."));
                }
            }

            if (!keyboard_macro_gameplay_ready()) {
                suspend_keyboard_macro_runtime_for_transition();
                ImGui::TextWrapped(_("Macro status: %s"), playback_status);
                ImGui::Unindent(lineIndent);
                ImGui::EndGroup();
                return;
            }

            ImGui::SeparatorText(_("Playback"));
            if (ImGui::Button(_("Reload Macro File"))) {
                reload_playback_file();
            }
            ImGui::SameLine();
            if (ImGui::Button(_("Play Macro"))) {
                restart_playback();
            }
            ImGui::SameLine();
            if (ImGui::Button(_("Stop Macro"))) {
                stop_all_input();
            }
            ImGui::Text(_("Macro tick: %u / %u"), playback_frame_index, (uint32_t)playback_frames.size());
            const auto playback_time = format_playback_time(get_playback_elapsed_seconds());
            ImGui::Text(_("Macro time: %s"), playback_time.c_str());
            ImGui::TextWrapped(_("Macro status: %s"), playback_status);

            ImGui::SeparatorText(_("Battle Snapshot"));
            ImGui::Checkbox(_("Restore Resources"), &restore_resources_snapshot);
            if (ImGui::Button(_("Capture Snapshot"))) {
                if (capture_position_snapshot()) {
                    DISPLAY_MESSAGE("Snapshot captured");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(_("Load Snapshot"))) {
                if (load_position_snapshot()) {
                    DISPLAY_MESSAGE("Snapshot loaded");
                }
            }
            ImGui::TextWrapped(_("%s"), position_snapshot_label().c_str());
            if (position_snapshot_load_ticks > 0) {
                ImGui::Text(_("Snapshot restore ticks: %u"), position_snapshot_load_ticks);
            }

            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
}

void KeyboardMacro::handle_hotkey_actions(
    bool reload_pressed,
    bool restart_pressed,
    bool stop_pressed,
    bool capture_snapshot_pressed,
    bool load_snapshot_pressed) {
    if (!mod_enabled) {
        return;
    }

    if (reload_pressed) {
        reload_playback_file();
        DISPLAY_MESSAGE("Keyboard Macro file reloaded");
    }

    if (restart_pressed) {
        restart_playback();
        DISPLAY_MESSAGE("Keyboard Macro playback started");
    }

    if (stop_pressed) {
        stop_all_input();
        DISPLAY_MESSAGE("Keyboard Macro stopped");
    }

    if (capture_snapshot_pressed) {
        if (capture_position_snapshot()) {
            DISPLAY_MESSAGE("Battle Snapshot captured");
        }
    }

    if (load_snapshot_pressed) {
        if (load_position_snapshot()) {
            DISPLAY_MESSAGE("Battle Snapshot loaded");
        }
    }
}

void KeyboardMacro::poll_raw_keyboard(bool trigger_actions) {
    bool reload_pressed = false;
    bool restart_pressed = false;
    bool stop_pressed = false;
    bool capture_snapshot_pressed = false;
    bool load_snapshot_pressed = false;
    uint32_t clip_hotkey_index = INVALID_CLIP_INDEX;
    uint32_t clip_hotkey_score = 0;

    for (uint32_t vkey = 1; vkey < 256; ++vkey) {
        const bool is_down = (GetAsyncKeyState((int)vkey) & 0x8000) != 0;
        const bool went_down = is_down && !raw_key_down[vkey];
        raw_key_down[vkey] = is_down;

        if (!went_down || !is_scannable_vkey(vkey)) {
            continue;
        }

        if (trigger_actions && capture_hotkey_target != 0) {
            if (!is_game_window_foreground()) {
                continue;
            }
            capture_pending_hotkey(vkey);
            continue;
        }

        if (trigger_actions) {
            if (keyboard_macro_instance && keyboard_macro_instance->m_hotkeys.size() >= 5) {
                auto& reload_hotkey = *keyboard_macro_instance->m_hotkeys[0];
                auto& restart_hotkey = *keyboard_macro_instance->m_hotkeys[1];
                auto& stop_hotkey = *keyboard_macro_instance->m_hotkeys[2];
                auto& capture_snapshot_hotkey = *keyboard_macro_instance->m_hotkeys[3];
                auto& load_snapshot_hotkey = *keyboard_macro_instance->m_hotkeys[4];

                reload_pressed |= hotkey_message_matches(reload_hotkey, vkey);
                restart_pressed |= hotkey_message_matches(restart_hotkey, vkey);
                stop_pressed |= hotkey_message_matches(stop_hotkey, vkey);
                capture_snapshot_pressed |= hotkey_message_matches(capture_snapshot_hotkey, vkey);
                load_snapshot_pressed |= hotkey_message_matches(load_snapshot_hotkey, vkey);

                reload_pressed |= vkey == DEFAULT_RELOAD_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(reload_hotkey, DEFAULT_RELOAD_VKEY);
                restart_pressed |= vkey == DEFAULT_RESTART_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(restart_hotkey, DEFAULT_RESTART_VKEY);
                stop_pressed |= vkey == DEFAULT_STOP_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(stop_hotkey, DEFAULT_STOP_VKEY);
                capture_snapshot_pressed |= vkey == DEFAULT_CAPTURE_SNAPSHOT_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(capture_snapshot_hotkey, DEFAULT_CAPTURE_SNAPSHOT_VKEY);
                load_snapshot_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(load_snapshot_hotkey, DEFAULT_LOAD_SNAPSHOT_VKEY);
            }
            else {
                reload_pressed |= vkey == DEFAULT_RELOAD_VKEY && !any_hotkey_modifier_down();
                restart_pressed |= vkey == DEFAULT_RESTART_VKEY && !any_hotkey_modifier_down();
                stop_pressed |= vkey == DEFAULT_STOP_VKEY && !any_hotkey_modifier_down();
                capture_snapshot_pressed |= vkey == DEFAULT_CAPTURE_SNAPSHOT_VKEY && !any_hotkey_modifier_down();
                load_snapshot_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_VKEY && !any_hotkey_modifier_down();
            }

            for (uint32_t clip_index = 0; clip_index < playback_clips.size(); ++clip_index) {
                const auto& binds = playback_clips[clip_index].hotkey_binds;
                if (hotkey_binds_match_message(binds, vkey) && binds.size() > clip_hotkey_score) {
                    clip_hotkey_index = clip_index;
                    clip_hotkey_score = (uint32_t)binds.size();
                }
            }
        }
    }

    if (trigger_actions) {
        if (!is_game_window_foreground()) {
            reload_pressed = false;
            restart_pressed = false;
            capture_snapshot_pressed = false;
            load_snapshot_pressed = false;
            clip_hotkey_index = INVALID_CLIP_INDEX;
        }

        handle_hotkey_actions(reload_pressed, restart_pressed, stop_pressed, capture_snapshot_pressed, load_snapshot_pressed);
        if (!reload_pressed && !restart_pressed && !stop_pressed && !capture_snapshot_pressed && !load_snapshot_pressed && clip_hotkey_index != INVALID_CLIP_INDEX) {
            restart_playback_clip(clip_hotkey_index);
            DISPLAY_MESSAGE("Keyboard Macro clip playback started");
        }
    }
}

void KeyboardMacro::check_hotkeys() {
    ensure_keyboard_macro_hotkeys(m_hotkeys);
    ensure_keyboard_macro_hotkey_binds(m_hotkeys);
    if (!mod_enabled) {
        if (capture_hotkey_target != 0) {
            capture_hotkey_target = 0;
            for (auto& hotkey : m_hotkeys) {
                if (hotkey) {
                    hotkey->m_setting = false;
                }
            }
        }
        poll_raw_keyboard(false);
        return;
    }

    poll_raw_keyboard(true);
}

void KeyboardMacro::on_update_input(utility::Input& input) {
    (void)input;
    ensure_keyboard_macro_hotkeys(m_hotkeys);
    ensure_keyboard_macro_hotkey_binds(m_hotkeys);
}

bool KeyboardMacro::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    (void)wnd;

    if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN) {
        return true;
    }

    if ((l_param & (1 << 30)) != 0) {
        return true;
    }

    if (mod_enabled && capture_hotkey_target != 0 && w_param < 256) {
        capture_pending_hotkey((uint32_t)w_param);
    }

    return true;
}

void KeyboardMacro::on_config_load(const utility::Config& cfg) {
    ensure_keyboard_macro_hotkeys(m_hotkeys);
    mod_enabled = cfg.get<bool>("keyboard_macro_enabled").value_or(false);
    playback_enabled = false;
    clear_playback_timer();
    loaded_playback_path[0] = '\0';
    reload_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_reload_vkey").value_or(DEFAULT_RELOAD_VKEY), 1u, 255u);
    restart_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_restart_vkey").value_or(DEFAULT_RESTART_VKEY), 1u, 255u);
    stop_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_stop_vkey").value_or(DEFAULT_STOP_VKEY), 1u, 255u);
    capture_snapshot_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_capture_snapshot_vkey").value_or(DEFAULT_CAPTURE_SNAPSHOT_VKEY), 1u, 255u);
    load_snapshot_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_load_snapshot_vkey").value_or(DEFAULT_LOAD_SNAPSHOT_VKEY), 1u, 255u);
    restore_resources_snapshot = cfg.get<bool>("keyboard_macro_restore_resources").value_or(false);
    if (m_hotkeys.size() >= 5) {
        if (!cfg.get("keyboard_macro_reload_file_key")) {
            m_hotkeys[0]->m_default_keys = { reload_vkey };
        }
        if (!cfg.get("keyboard_macro_restart_key")) {
            m_hotkeys[1]->m_default_keys = { restart_vkey };
        }
        if (!cfg.get("keyboard_macro_stop_clear_key")) {
            m_hotkeys[2]->m_default_keys = { stop_vkey };
        }
        if (!cfg.get("keyboard_macro_capture_snapshot_key")) {
            m_hotkeys[3]->m_default_keys = { capture_snapshot_vkey };
        }
        if (!cfg.get("keyboard_macro_load_snapshot_key")) {
            m_hotkeys[4]->m_default_keys = { load_snapshot_vkey };
        }
    }
    refresh_playback_file_choices();

    const auto saved_path = cfg.get("keyboard_macro_playback_path").value_or(DEFAULT_PLAYBACK_FILE);
    const auto custom_path = cfg.get("keyboard_macro_custom_path").value_or(saved_path);
    strncpy_s(playback_path, custom_path.c_str(), _TRUNCATE);
    selected_clip_index = cfg.get<uint32_t>("keyboard_macro_clip_index").value_or(0u);
    loaded_clip_index = INVALID_CLIP_INDEX;

    uint32_t inferred_slot = PLAYBACK_SLOT_MAIN;
    if (infer_playback_slot_from_path(saved_path, inferred_slot)) {
        playback_slot = inferred_slot;
    }
    else {
        playback_slot = PLAYBACK_SLOT_CUSTOM;
    }

    load_playback_file();
    reset_input_state();
    update_input_active();
}

void KeyboardMacro::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("keyboard_macro_enabled", mod_enabled);
    ensure_keyboard_macro_hotkeys(m_hotkeys);
    if (m_hotkeys.size() >= 5) {
        if (!m_hotkeys[0]->m_binds.empty()) {
            reload_vkey = m_hotkeys[0]->m_binds.back();
        }
        if (!m_hotkeys[1]->m_binds.empty()) {
            restart_vkey = m_hotkeys[1]->m_binds.back();
        }
        if (!m_hotkeys[2]->m_binds.empty()) {
            stop_vkey = m_hotkeys[2]->m_binds.back();
        }
        if (!m_hotkeys[3]->m_binds.empty()) {
            capture_snapshot_vkey = m_hotkeys[3]->m_binds.back();
        }
        if (!m_hotkeys[4]->m_binds.empty()) {
            load_snapshot_vkey = m_hotkeys[4]->m_binds.back();
        }
    }
    cfg.set<uint32_t>("keyboard_macro_reload_vkey", reload_vkey);
    cfg.set<uint32_t>("keyboard_macro_restart_vkey", restart_vkey);
    cfg.set<uint32_t>("keyboard_macro_stop_vkey", stop_vkey);
    cfg.set<uint32_t>("keyboard_macro_capture_snapshot_vkey", capture_snapshot_vkey);
    cfg.set<uint32_t>("keyboard_macro_load_snapshot_vkey", load_snapshot_vkey);
    cfg.set<bool>("keyboard_macro_restore_resources", restore_resources_snapshot);
    cfg.set<uint32_t>("keyboard_macro_slot", playback_slot);
    cfg.set<uint32_t>("keyboard_macro_clip_index", selected_clip_index);
    cfg.set("keyboard_macro_custom_path", playback_path);
    cfg.set("keyboard_macro_playback_path", get_selected_playback_path_text());
}
