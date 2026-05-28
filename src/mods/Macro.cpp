#include "Macro.hpp"
#include "CharSwitcher.hpp"
#include "EnemyStepDisplay.hpp"
#include "EnemyTracker.hpp"
#include "HealthSettings.hpp"
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
constexpr short ANALOG_WALK = 80;
constexpr uintptr_t S_DEVIL4_PAD_PTR = 0x00e559c4;
constexpr uintptr_t S_KEYBOARD_PTR = 0x00e559c0;
constexpr uintptr_t S_SAVE_PTR = 0x00e558c8;
constexpr uintptr_t UPDATE_ANALOG_INFO_CALL = 0x007b0250;
constexpr uintptr_t UPLAYER_CURRENT_STYLE_OFFSET = 0x14D98;
constexpr uint32_t MAX_ENEMY_CHAIN_SCAN = 128;
constexpr uint32_t PAD_BUTTON_L1 = 0x0100;
constexpr uint32_t PAD_BUTTON_R1 = 0x0200;
constexpr uint32_t PAD_BUTTON_L2 = 0x0400;
constexpr uint32_t PAD_BUTTON_R2 = 0x0800;
constexpr uint32_t PAD_BUTTON_SELECT = 0x0001;
constexpr uint32_t PAD_BUTTON_L3 = 0x0002;
constexpr uint32_t PAD_BUTTON_R3 = 0x0004;
constexpr uint32_t PAD_BUTTON_START = 0x0008;
constexpr uint32_t PAD_BUTTON_DPAD_UP = 0x0010;
constexpr uint32_t PAD_BUTTON_DPAD_RIGHT = 0x0020;
constexpr uint32_t PAD_BUTTON_DPAD_DOWN = 0x0040;
constexpr uint32_t PAD_BUTTON_DPAD_LEFT = 0x0080;
constexpr uint32_t PAD_BUTTON_Y = 0x1000;
constexpr uint32_t PAD_BUTTON_B = 0x2000;
constexpr uint32_t PAD_BUTTON_A = 0x4000;
constexpr uint32_t PAD_BUTTON_X = 0x8000;
constexpr uint32_t PAD_BUTTON_GLOBAL_ROUTE_MASK = PAD_BUTTON_L3 | PAD_BUTTON_R3;
enum MacroActionIndex : uint32_t {
    MACRO_ACTION_MELEE = 0,
    MACRO_ACTION_GUN,
    MACRO_ACTION_EXCEED,
    MACRO_ACTION_JUMP,
    MACRO_ACTION_BRINGER,
    MACRO_ACTION_STYLE_ACTION,
    MACRO_ACTION_DEVIL_TRIGGER,
    MACRO_ACTION_LOCK_ON,
    MACRO_ACTION_CHANGE_GUN,
    MACRO_ACTION_CHANGE_SWORD,
    MACRO_ACTION_TAUNT,
    MACRO_ACTION_CHANGE_TARGET,
    MACRO_ACTION_RESET_CAMERA,
    MACRO_ACTION_COUNT,
};
static_assert(MACRO_ACTION_COUNT == MACRO_ACTION_SLOT_COUNT);
constexpr uint32_t MACRO_ACTION_BIT(uint32_t action_index) {
    return 1u << action_index;
}
constexpr uint32_t DEFAULT_NERO_ACTION_BUTTON_MAP[MACRO_ACTION_COUNT] = {
    PAD_BUTTON_Y,
    PAD_BUTTON_X,
    PAD_BUTTON_L2,
    PAD_BUTTON_A,
    PAD_BUTTON_B,
    0,
    PAD_BUTTON_L1,
    PAD_BUTTON_R1,
    0,
    0,
    PAD_BUTTON_SELECT,
    PAD_BUTTON_L3,
    PAD_BUTTON_R3,
};
constexpr uint32_t DEFAULT_DANTE_ACTION_BUTTON_MAP[MACRO_ACTION_COUNT] = {
    PAD_BUTTON_Y,
    PAD_BUTTON_X,
    0,
    PAD_BUTTON_A,
    0,
    PAD_BUTTON_B,
    PAD_BUTTON_L1,
    PAD_BUTTON_R1,
    PAD_BUTTON_L2,
    PAD_BUTTON_R2,
    PAD_BUTTON_SELECT,
    PAD_BUTTON_L3,
    PAD_BUTTON_R3,
};
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
constexpr uint32_t DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY = VK_F6;
constexpr uint32_t GAMEPAD_HOTKEY_ACTION_PLAY = 1 << 0;
constexpr uint32_t GAMEPAD_HOTKEY_ACTION_STOP = 1 << 1;
constexpr uint32_t GAMEPAD_HOTKEY_ACTION_CAPTURE_SNAPSHOT = 1 << 2;
constexpr uint32_t GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT = 1 << 3;
constexpr uint32_t GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT_PLAY = 1 << 4;
constexpr uint32_t GAMEPAD_HOTKEY_COUNT = 5;
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
constexpr uint32_t MACRO_WAIT_ANIM_FRAME = 9;
constexpr uint32_t MACRO_WAIT_MOVEID2 = 10;
constexpr uint32_t MACRO_WAIT_MOVEID2_CHANGED = 11;
constexpr uint32_t MACRO_WAIT_FRAME_REACHED_MAX = 12;
constexpr uint32_t MACRO_WAIT_COMPARE_EQ = 1;
constexpr uint32_t MACRO_WAIT_COMPARE_NE = 2;
constexpr uint32_t MACRO_WAIT_COMPARE_GT = 3;
constexpr uint32_t MACRO_WAIT_COMPARE_GTE = 4;
constexpr uint32_t MACRO_WAIT_COMPARE_LT = 5;
constexpr uint32_t MACRO_WAIT_COMPARE_LTE = 6;
constexpr uint32_t POSITION_SNAPSHOT_LOAD_TICKS = 30;
constexpr uint32_t MAX_SNAPSHOT_PLAY_DELAY_TICKS = 6000;
constexpr const char* DEFAULT_PLAYBACK_FILE = "macro.txt";
constexpr const char* CUSTOM_PLAYBACK_LABEL = "Custom";
constexpr uint32_t DEFAULT_GAMEPAD_HOTKEY_BUTTONS[GAMEPAD_HOTKEY_COUNT] = {
    PAD_BUTTON_A,
    PAD_BUTTON_B,
    PAD_BUTTON_X,
    PAD_BUTTON_Y,
    PAD_BUTTON_R1,
};

struct ParsedMacroInput {
    uint32_t buttons = 0;
    uint32_t global_buttons = 0;
    uint32_t actions = 0;
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool walk_up = false;
    bool walk_down = false;
    bool walk_left = false;
    bool walk_right = false;
    bool camera_up = false;
    bool camera_down = false;
    bool camera_left = false;
    bool camera_right = false;
    bool screen_pause = false;
    bool screen_resume = false;
    bool screen_pause_toggle = false;
};

struct HeldMacroInput {
    uint32_t buttons = 0;
    uint32_t global_buttons = 0;
    uint32_t actions = 0;
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool walk_up = false;
    bool walk_down = false;
    bool walk_left = false;
    bool walk_right = false;
    bool camera_up = false;
    bool camera_down = false;
    bool camera_left = false;
    bool camera_right = false;
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
Macro* macro_instance = nullptr;
std::chrono::steady_clock::time_point playback_timer_started_at{};
double playback_elapsed_seconds = 0.0;
bool playback_timer_active = false;
bool playback_timer_pending_start = false;
std::vector<std::string> playback_file_choices{};
EnemyHitSample last_hit_confirmed_enemy{};
bool has_last_hit_confirmed_enemy = false;
PositionSnapshot position_snapshot{};
uint32_t position_snapshot_load_ticks = 0;
uint32_t snapshot_play_pending_ticks = 0;
uint32_t snapshot_play_pending_clip_index = INVALID_CLIP_INDEX;
bool restore_resources_snapshot = false;
bool macro_suspended_for_transition = false;
cPeripheral* last_player_peripheral = nullptr;
uint32_t last_player_index = 0;
uint32_t last_global_routed_buttons[4] = {};
bool last_global_right_analog[4] = {};
std::string known_macro_file_time_path{};
uint64_t known_macro_file_write_time = 0;
uint64_t known_macro_file_size = 0;
bool last_game_pause_state = false;
uint32_t gamepad_hotkey_chord_state = 0;
uint32_t gamepad_hotkey_raw_buttons = 0;
uint32_t capture_gamepad_hotkey_target = 0;

void update_config_hotkey_vkeys(const std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys);
std::string trim_copy(const std::string& value);
bool is_absolute_path(const std::string& path);
std::string get_game_directory();
void refresh_playback_file_choices();
bool parse_integer(const std::string& token, uint32_t& value);
bool parse_hotkey_binds(const std::string& expression, std::vector<uint32_t>& binds);
uint32_t sanitize_macro_button(uint32_t button, uint32_t fallback);
std::string clip_display_label(const MacroClip& clip, uint32_t clip_index);
void clear_playback_timer();
void set_playback_status(const std::string& message);
bool is_boss_enemy_id(int enemy_id);
uDamage_Old* get_enemy_damage_block(uEnemy_Old* enemy);
sArea* get_s_area_safe();
sMediator* get_s_mediator_safe();
uPlayer* get_local_player_safe();
uCameraCtrl* get_local_camera_safe();
sWorkRate* get_work_rate_safe();
sDevil4Pad* get_global_pad_safe();

uint64_t file_time_to_u64(const FILETIME& file_time) {
    ULARGE_INTEGER value{};
    value.LowPart = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    return value.QuadPart;
}

bool get_file_fingerprint(const std::string& path, uint64_t& write_time, uint64_t& file_size) {
    write_time = 0;
    file_size = 0;
    if (path.empty()) {
        return false;
    }

    WIN32_FILE_ATTRIBUTE_DATA data{};
    const auto wide_path = utility::widen(path);
    if (!GetFileAttributesExW(wide_path.c_str(), GetFileExInfoStandard, &data)) {
        return false;
    }

    write_time = file_time_to_u64(data.ftLastWriteTime);
    file_size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
    return write_time != 0;
}

void remember_macro_file_write_time(const std::string& path) {
    uint64_t write_time = 0;
    uint64_t file_size = 0;
    if (!get_file_fingerprint(path, write_time, file_size)) {
        known_macro_file_time_path.clear();
        known_macro_file_write_time = 0;
        known_macro_file_size = 0;
        return;
    }

    known_macro_file_time_path = path;
    known_macro_file_write_time = write_time;
    known_macro_file_size = file_size;
}

bool is_game_paused_safe() {
    __try {
        return devil4_sdk::is_paused();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return true;
    }
}

bool is_game_window_foreground() {
    auto* window = g_framework ? g_framework->get_window_handle() : nullptr;
    return !window || GetForegroundWindow() == window;
}

bool macro_gameplay_ready() {
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

bool macro_runtime_ready() {
    return macro_gameplay_ready() && !is_game_paused_safe();
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

sDevil4Pad* get_global_pad_safe() {
    __try {
        auto** pad_ptr = reinterpret_cast<sDevil4Pad**>(S_DEVIL4_PAD_PTR);
        return pad_ptr ? *pad_ptr : nullptr;
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

int gamepad_button_index(uint32_t button) {
    if (button == 0 || (button & (button - 1)) != 0) {
        return -1;
    }

    for (int index = 0; index < 32; ++index) {
        if ((button & (1u << index)) != 0) {
            return index;
        }
    }
    return -1;
}

bool gamepad_hotkey_prefix_active(uint32_t buttons, const std::chrono::steady_clock::time_point& now) {
    (void)now;
    return (buttons & PAD_BUTTON_SELECT) != 0;
}

uint32_t configured_gamepad_hotkey_button_mask() {
    uint32_t buttons = 0;
    for (uint32_t index = 0; index < GAMEPAD_HOTKEY_COUNT; ++index) {
        buttons |= Macro::gamepad_hotkey_buttons[index];
    }
    return buttons;
}

uint32_t clip_gamepad_hotkey_button_mask() {
    uint32_t buttons = 0;
    for (const auto& clip : Macro::playback_clips) {
        buttons |= clip.gamepad_hotkey_button;
    }
    return buttons;
}

uint32_t gamepad_hotkey_actions_from_rolling_chord(
    uint32_t buttons,
    uint32_t pressed_buttons,
    const std::chrono::steady_clock::time_point& now) {
    if (!gamepad_hotkey_prefix_active(buttons, now)) {
        return 0;
    }

    uint32_t action_buttons = pressed_buttons & ~PAD_BUTTON_SELECT;

    uint32_t actions = 0;
    if ((action_buttons & Macro::gamepad_hotkey_buttons[0]) != 0) {
        actions |= GAMEPAD_HOTKEY_ACTION_PLAY;
    }
    if ((action_buttons & Macro::gamepad_hotkey_buttons[1]) != 0) {
        actions |= GAMEPAD_HOTKEY_ACTION_STOP;
    }
    if ((action_buttons & Macro::gamepad_hotkey_buttons[2]) != 0) {
        actions |= GAMEPAD_HOTKEY_ACTION_CAPTURE_SNAPSHOT;
    }
    if ((action_buttons & Macro::gamepad_hotkey_buttons[3]) != 0) {
        actions |= GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT;
    }
    if ((action_buttons & Macro::gamepad_hotkey_buttons[4]) != 0) {
        actions |= GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT_PLAY;
    }
    return actions;
}

uint32_t gamepad_hotkey_buttons_from_rolling_chord(
    uint32_t buttons,
    uint32_t pressed_buttons,
    const std::chrono::steady_clock::time_point& now) {
    if (!gamepad_hotkey_prefix_active(buttons, now)) {
        return 0;
    }

    return pressed_buttons & ~PAD_BUTTON_SELECT;
}

uint32_t gamepad_hotkey_buttons_from_actions(uint32_t actions) {
    uint32_t buttons = 0;
    if ((actions & GAMEPAD_HOTKEY_ACTION_PLAY) != 0) {
        buttons |= Macro::gamepad_hotkey_buttons[0];
    }
    if ((actions & GAMEPAD_HOTKEY_ACTION_STOP) != 0) {
        buttons |= Macro::gamepad_hotkey_buttons[1];
    }
    if ((actions & GAMEPAD_HOTKEY_ACTION_CAPTURE_SNAPSHOT) != 0) {
        buttons |= Macro::gamepad_hotkey_buttons[2];
    }
    if ((actions & GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT) != 0) {
        buttons |= Macro::gamepad_hotkey_buttons[3];
    }
    if ((actions & GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT_PLAY) != 0) {
        buttons |= Macro::gamepad_hotkey_buttons[4];
    }
    return buttons;
}

uint32_t sanitize_gamepad_hotkey_button(uint32_t button, uint32_t fallback) {
    switch (button) {
    case PAD_BUTTON_L1:
    case PAD_BUTTON_R1:
    case PAD_BUTTON_L2:
    case PAD_BUTTON_R2:
    case PAD_BUTTON_L3:
    case PAD_BUTTON_R3:
    case PAD_BUTTON_START:
    case PAD_BUTTON_DPAD_UP:
    case PAD_BUTTON_DPAD_RIGHT:
    case PAD_BUTTON_DPAD_DOWN:
    case PAD_BUTTON_DPAD_LEFT:
    case PAD_BUTTON_Y:
    case PAD_BUTTON_B:
    case PAD_BUTTON_A:
    case PAD_BUTTON_X:
        return button;
    default:
        return fallback;
    }
}

const char* gamepad_hotkey_button_label(uint32_t button) {
    switch (button) {
    case PAD_BUTTON_L1:
        return "LB / L1";
    case PAD_BUTTON_R1:
        return "RB / R1";
    case PAD_BUTTON_L2:
        return "LT / L2";
    case PAD_BUTTON_R2:
        return "RT / R2";
    case PAD_BUTTON_L3:
        return "LS / L3";
    case PAD_BUTTON_R3:
        return "RS / R3";
    case PAD_BUTTON_START:
        return "START / OPTIONS";
    case PAD_BUTTON_DPAD_UP:
        return "D-Pad Up";
    case PAD_BUTTON_DPAD_RIGHT:
        return "D-Pad Right";
    case PAD_BUTTON_DPAD_DOWN:
        return "D-Pad Down";
    case PAD_BUTTON_DPAD_LEFT:
        return "D-Pad Left";
    case PAD_BUTTON_Y:
        return "Y / TRIANGLE";
    case PAD_BUTTON_B:
        return "B / CIRCLE";
    case PAD_BUTTON_A:
        return "A / CROSS";
    case PAD_BUTTON_X:
        return "X / SQUARE";
    default:
        return "Unknown";
    }
}

uint32_t first_valid_gamepad_hotkey_button(uint32_t buttons) {
    buttons &= ~PAD_BUTTON_SELECT;
    for (int index = 0; index < 32; ++index) {
        const uint32_t button = 1u << index;
        if ((buttons & button) != 0 && sanitize_gamepad_hotkey_button(button, 0) != 0) {
            return button;
        }
    }
    return 0;
}

bool read_gamepad_buttons_safe(uint32_t& buttons) {
    buttons = 0;
    auto* pad = get_global_pad_safe();
    if (!pad) {
        return false;
    }

    __try {
        buttons = pad->mPadInfo[0].mBtn.on;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        buttons = 0;
        return false;
    }
}

bool read_peripheral_player_index_safe(cPeripheral* peripheral, uint32_t& player_index) {
    player_index = 0;
    if (!peripheral) {
        return false;
    }

    __try {
        player_index = *(uint8_t*)((uintptr_t)peripheral + 0x95);
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        player_index = 0;
        return false;
    }
}

void clear_press_values_for_buttons(kPressInfo& press, uint32_t buttons) {
    if ((buttons & PAD_BUTTON_L1) != 0) {
        press.L1 = 0.0f;
    }
    if ((buttons & PAD_BUTTON_L2) != 0) {
        press.L2 = 0.0f;
    }
    if ((buttons & PAD_BUTTON_R1) != 0) {
        press.R1 = 0.0f;
    }
    if ((buttons & PAD_BUTTON_R2) != 0) {
        press.R2 = 0.0f;
    }
    if ((buttons & PAD_BUTTON_SELECT) != 0) {
        press.Select = 0.0f;
    }
    if ((buttons & PAD_BUTTON_L3) != 0) {
        press.L3 = 0.0f;
    }
    if ((buttons & PAD_BUTTON_R3) != 0) {
        press.R3 = 0.0f;
    }
}

void clear_peripheral_buttons(cPeripheral* peripheral, uint32_t buttons) {
    if (!peripheral || buttons == 0) {
        return;
    }

    __try {
        peripheral->mPadBtnOn &= ~buttons;
        peripheral->mPadBtnTrg &= ~buttons;
        peripheral->mPadBtnRel &= ~buttons;
        clear_press_values_for_buttons(*reinterpret_cast<kPressInfo*>(&peripheral->mPadBtnPress), buttons);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    }
}

void consume_gamepad_hotkey_buttons(cPeripheral* peripheral, uint32_t buttons) {
    if (buttons == 0) {
        return;
    }

    auto* pad = get_global_pad_safe();
    if (pad) {
        __try {
            auto& pad_info = pad->mPadInfo[0];
            pad_info.mBtn.on &= ~buttons;
            pad_info.mBtn.trg &= ~buttons;
            pad_info.mBtn.rel &= ~buttons;
            pad_info.mBtn.rep &= ~buttons;
            clear_press_values_for_buttons(pad_info.mPress, buttons);

            auto& pad_data = pad->mPad[0].field10_0x15c;
            pad_data.On &= ~buttons;
            pad_data.Trg &= ~buttons;
            pad_data.Rel &= ~buttons;
            pad_data.Chg &= ~buttons;
            pad_data.Rep &= ~buttons;
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        }
    }

    clear_peripheral_buttons(peripheral, buttons);
}

bool clear_peripheral_output(cPeripheral* peripheral, uint32_t player_index) {
    if (!peripheral) {
        return false;
    }

    __try {
        const uint32_t release_buttons = player_index < 4 ? Macro::last_buttons[player_index] : 0;
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

void set_global_pad_press(kPressInfo& press, uint32_t buttons) {
    if ((buttons & PAD_BUTTON_L1) != 0) {
        press.L1 = std::max(press.L1, 1.0f);
    }
    if ((buttons & PAD_BUTTON_L2) != 0) {
        press.L2 = std::max(press.L2, 1.0f);
    }
    if ((buttons & PAD_BUTTON_R1) != 0) {
        press.R1 = std::max(press.R1, 1.0f);
    }
    if ((buttons & PAD_BUTTON_R2) != 0) {
        press.R2 = std::max(press.R2, 1.0f);
    }
    if ((buttons & PAD_BUTTON_SELECT) != 0) {
        press.Select = std::max(press.Select, 1.0f);
    }
    if ((buttons & PAD_BUTTON_L3) != 0) {
        press.L3 = std::max(press.L3, 1.0f);
    }
    if ((buttons & PAD_BUTTON_R3) != 0) {
        press.R3 = std::max(press.R3, 1.0f);
    }
}

void clear_global_macro_route(uint32_t player_index) {
    if (player_index >= 4) {
        player_index = 0;
    }

    const uint32_t release_buttons = last_global_routed_buttons[player_index];
    const bool release_right_analog = last_global_right_analog[player_index];
    auto* pad = get_global_pad_safe();
    if (pad) {
        __try {
            auto& pad_info = pad->mPadInfo[0];
            pad_info.mBtn.on &= ~release_buttons;
            pad_info.mBtn.trg &= ~release_buttons;
            pad_info.mBtn.rel |= release_buttons;

            if ((release_buttons & PAD_BUTTON_L3) != 0) {
                pad_info.mPress.L3 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_R3) != 0) {
                pad_info.mPress.R3 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_L1) != 0) {
                pad_info.mPress.L1 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_L2) != 0) {
                pad_info.mPress.L2 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_R1) != 0) {
                pad_info.mPress.R1 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_R2) != 0) {
                pad_info.mPress.R2 = 0.0f;
            }
            if ((release_buttons & PAD_BUTTON_SELECT) != 0) {
                pad_info.mPress.Select = 0.0f;
            }
            if (release_right_analog) {
                pad_info.mAnlg[1].x = 0.0f;
                pad_info.mAnlg[1].y = 0.0f;
            }
        }
        __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        }
    }

    last_global_routed_buttons[player_index] = 0;
    last_global_right_analog[player_index] = false;
}

bool clear_last_peripheral_output() {
    if (!clear_peripheral_output(last_player_peripheral, last_player_index)) {
        return false;
    }

    clear_global_macro_route(last_player_index);
    std::fill_n(Macro::last_buttons, 4, 0);
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

void suspend_macro_runtime_for_transition() {
    clear_last_peripheral_output();
    Macro::playback_enabled = false;
    Macro::clear_input_frames = 0;
    Macro::input_active = false;
    position_snapshot_load_ticks = 0;
    snapshot_play_pending_ticks = 0;
    snapshot_play_pending_clip_index = INVALID_CLIP_INDEX;
    clear_playback_timer();

    if (!macro_suspended_for_transition) {
        macro_suspended_for_transition = true;
        set_playback_status("Macro paused while gameplay is unavailable.");
    }
}

void ensure_keyboard_hotkeys(std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys) {
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
    if (hotkeys.size() == 5) {
        utility::create_keyboard_hotkey(
            hotkeys,
            { DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY },
            __("Load Snapshot + Play Macro"),
            "keyboard_macro_load_snapshot_play_key");
    }
}

void ensure_keyboard_hotkey_binds(std::vector<std::unique_ptr<utility::Hotkey>>& hotkeys) {
    if (hotkeys.size() < 6) {
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

    if (Macro::playback_slot != PLAYBACK_SLOT_CUSTOM && Macro::playback_slot < playback_file_choices.size()) {
        return playback_file_choices[Macro::playback_slot];
    }

    return trim_copy(Macro::playback_path);
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

bool is_macro_text_filename(const std::string& filename) {
    const auto lowered = lowercase_copy(filename);
    constexpr const char* suffix = ".txt";

    if (lowered.size() < strlen(suffix) || lowered.compare(lowered.size() - strlen(suffix), strlen(suffix), suffix) != 0) {
        return false;
    }

    // Keep keyboard_macro*.txt as a legacy file name so older tester setups still appear in the Macro file picker.
    return lowered.rfind("macro", 0) == 0 || lowered.rfind("keyboard_macro", 0) == 0;
}

void refresh_playback_file_choices() {
    std::string previous_selection{};
    if (Macro::playback_slot != PLAYBACK_SLOT_CUSTOM && Macro::playback_slot < playback_file_choices.size()) {
        previous_selection = playback_file_choices[Macro::playback_slot];
    }

    std::vector<std::string> discovered{};
    const auto game_directory = get_game_directory();
    if (!game_directory.empty()) {
        auto discover_pattern = [&](const char* pattern_name) {
            const auto pattern = game_directory + "\\" + pattern_name;
            WIN32_FIND_DATAA find_data{};
            HANDLE handle = FindFirstFileA(pattern.c_str(), &find_data);
            if (handle != INVALID_HANDLE_VALUE) {
                do {
                    if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                        is_macro_text_filename(find_data.cFileName)) {
                        discovered.emplace_back(find_data.cFileName);
                    }
                } while (FindNextFileA(handle, &find_data));
                FindClose(handle);
            }
        };

        discover_pattern("macro*.txt");
        discover_pattern("keyboard_macro*.txt");
    }

    if (discovered.empty()) {
        discovered.emplace_back(DEFAULT_PLAYBACK_FILE);
    }

    std::sort(discovered.begin(), discovered.end(), [](const std::string& left, const std::string& right) {
        const auto left_lower = lowercase_copy(left);
        const auto right_lower = lowercase_copy(right);
        const bool left_legacy = left_lower.rfind("keyboard_macro", 0) == 0;
        const bool right_legacy = right_lower.rfind("keyboard_macro", 0) == 0;
        if (left_legacy != right_legacy) {
            return !left_legacy;
        }
        return left_lower < right_lower;
    });
    discovered.erase(std::unique(discovered.begin(), discovered.end(), [](const std::string& left, const std::string& right) {
        return lowercase_copy(left) == lowercase_copy(right);
    }), discovered.end());

    playback_file_choices = std::move(discovered);

    if (Macro::playback_slot == PLAYBACK_SLOT_CUSTOM) {
        return;
    }

    if (!previous_selection.empty()) {
        const auto previous_lower = lowercase_copy(previous_selection);
        for (uint32_t index = 0; index < playback_file_choices.size(); ++index) {
            if (lowercase_copy(playback_file_choices[index]) == previous_lower) {
                Macro::playback_slot = index;
                return;
            }
        }
    }

    if (Macro::playback_slot >= playback_file_choices.size()) {
        Macro::playback_slot = PLAYBACK_SLOT_MAIN;
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
        const auto filename = path_filename_lower(trimmed_path);
        if (find_slot_by_filename(filename)) {
            return true;
        }
        if (filename == lowercase_copy(DEFAULT_PLAYBACK_FILE) && !playback_file_choices.empty()) {
            slot_out = PLAYBACK_SLOT_MAIN;
            return true;
        }
        return false;
    }

    const auto game_directory = lowercase_copy(get_game_directory());
    if (game_directory.empty() || path_directory_lower(trimmed_path) != game_directory) {
        return false;
    }

    const auto filename = path_filename_lower(trimmed_path);
    if (!is_macro_text_filename(filename)) {
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
        {"SELECT", PAD_BUTTON_SELECT},
        {"SELECT_BUTTON", PAD_BUTTON_SELECT},
        {"BACK", PAD_BUTTON_SELECT},
        {"BACK_BUTTON", PAD_BUTTON_SELECT},
        {"VIEW", PAD_BUTTON_SELECT},
        {"SHARE", PAD_BUTTON_SELECT},
        {"SHARE_BUTTON", PAD_BUTTON_SELECT},

        {"LS", PAD_BUTTON_L3},
        {"L3", PAD_BUTTON_L3},
        {"LEFT_STICK_CLICK", PAD_BUTTON_L3},
        {"LEFTSTICK_CLICK", PAD_BUTTON_L3},

        {"RS", PAD_BUTTON_R3},
        {"R3", PAD_BUTTON_R3},
        {"RIGHT_STICK_CLICK", PAD_BUTTON_R3},
        {"RIGHTSTICK_CLICK", PAD_BUTTON_R3},

        {"START", PAD_BUTTON_START},
        {"START_BUTTON", PAD_BUTTON_START},
        {"PAUSE", PAD_BUTTON_START},
        {"PAUSE_MENU", PAD_BUTTON_START},
        {"PAVSE", PAD_BUTTON_START},
        {"PAVSE_MENU", PAD_BUTTON_START},
        {"MENU", PAD_BUTTON_START},
        {"START_MENU", PAD_BUTTON_START},
        {"OPTIONS", PAD_BUTTON_START},
        {"OPTIONS_BUTTON", PAD_BUTTON_START},
        {"ESC", PAD_BUTTON_START},
        {"ESCAPE", PAD_BUTTON_START},

        {"DPAD_UP", PAD_BUTTON_DPAD_UP},
        {"D_PAD_UP", PAD_BUTTON_DPAD_UP},
        {"UP", PAD_BUTTON_DPAD_UP},
        {"CHANGE_STYLE_TS", PAD_BUTTON_DPAD_UP},
        {"CHANGE_STYLE_TRICKSTER", PAD_BUTTON_DPAD_UP},
        {"DPAD_RIGHT", PAD_BUTTON_DPAD_RIGHT},
        {"D_PAD_RIGHT", PAD_BUTTON_DPAD_RIGHT},
        {"RIGHT", PAD_BUTTON_DPAD_RIGHT},
        {"CHANGE_STYLE_SM", PAD_BUTTON_DPAD_RIGHT},
        {"CHANGE_STYLE_SWORDMASTER", PAD_BUTTON_DPAD_RIGHT},
        {"CHANGE_STYLE_SWORD_MASTER", PAD_BUTTON_DPAD_RIGHT},
        {"DPAD_DOWN", PAD_BUTTON_DPAD_DOWN},
        {"D_PAD_DOWN", PAD_BUTTON_DPAD_DOWN},
        {"DOWN", PAD_BUTTON_DPAD_DOWN},
        {"CHANGE_STYLE_RG", PAD_BUTTON_DPAD_DOWN},
        {"CHANGE_STYLE_ROYALGUARD", PAD_BUTTON_DPAD_DOWN},
        {"CHANGE_STYLE_ROYAL_GUARD", PAD_BUTTON_DPAD_DOWN},
        {"DPAD_LEFT", PAD_BUTTON_DPAD_LEFT},
        {"D_PAD_LEFT", PAD_BUTTON_DPAD_LEFT},
        {"LEFT", PAD_BUTTON_DPAD_LEFT},
        {"CHANGE_STYLE_GS", PAD_BUTTON_DPAD_LEFT},
        {"CHANGE_STYLE_GUNSLINGER", PAD_BUTTON_DPAD_LEFT},
        {"CHANGE_STYLE_GUN_SLINGER", PAD_BUTTON_DPAD_LEFT},

        {"L1", PAD_BUTTON_L1},
        {"LB", PAD_BUTTON_L1},
        {"LEFT_BUMPER", PAD_BUTTON_L1},
        {"R1", PAD_BUTTON_R1},
        {"RB", PAD_BUTTON_R1},
        {"RIGHT_BUMPER", PAD_BUTTON_R1},
        {"L2", PAD_BUTTON_L2},
        {"LT", PAD_BUTTON_L2},
        {"LEFT_TRIGGER", PAD_BUTTON_L2},
        {"R2", PAD_BUTTON_R2},
        {"RT", PAD_BUTTON_R2},
        {"RIGHT_TRIGGER", PAD_BUTTON_R2},

        {"Y", PAD_BUTTON_Y},
        {"Y_BUTTON", PAD_BUTTON_Y},
        {"TRIANGLE", PAD_BUTTON_Y},
        {"TRI", PAD_BUTTON_Y},
        {"B", PAD_BUTTON_B},
        {"B_BUTTON", PAD_BUTTON_B},
        {"CIRCLE", PAD_BUTTON_B},
        {"O", PAD_BUTTON_B},
        {"A", PAD_BUTTON_A},
        {"A_BUTTON", PAD_BUTTON_A},
        {"CROSS", PAD_BUTTON_A},
        {"X", PAD_BUTTON_X},
        {"X_BUTTON", PAD_BUTTON_X},
        {"SQUARE", PAD_BUTTON_X},
        {"SQ", PAD_BUTTON_X},
    };

    for (const auto& entry : buttons) {
        if (normalized == entry.first) {
            button = entry.second;
            return true;
        }
    }

    return false;
}

bool parse_gamepad_hotkey_button_expression(const std::string& expression, uint32_t& button) {
    button = 0;
    const auto trimmed_expression = trim_copy(expression);
    if (trimmed_expression.empty()) {
        return true;
    }

    std::istringstream stream{ trimmed_expression };
    std::string token{};
    while (std::getline(stream, token, '/')) {
        token = trim_copy(token);
        if (token.empty()) {
            return false;
        }

        uint32_t parsed_button = 0;
        if (!parse_button_name(token, parsed_button) || sanitize_gamepad_hotkey_button(parsed_button, 0) == 0) {
            return false;
        }

        if (button == 0) {
            button = parsed_button;
        }
        else if (button != parsed_button) {
            return false;
        }
    }

    return button != 0;
}

bool parse_action_name(const std::string& token, uint32_t& action_index) {
    const auto normalized = normalize_button_token(token);

    static const std::pair<const char*, uint32_t> actions[] = {
        {"MELEE", MACRO_ACTION_MELEE},
        {"MELEE_ATTACK", MACRO_ACTION_MELEE},
        {"SWORD_ATTACK", MACRO_ACTION_MELEE},
        {"ATTACK", MACRO_ACTION_MELEE},
        {"GUN", MACRO_ACTION_GUN},
        {"GUN_ATTACK", MACRO_ACTION_GUN},
        {"SHOOT", MACRO_ACTION_GUN},
        {"FIRE", MACRO_ACTION_GUN},
        {"EXCEED", MACRO_ACTION_EXCEED},
        {"NERO_EXCEED", MACRO_ACTION_EXCEED},
        {"EXCEED_INPUT", MACRO_ACTION_EXCEED},
        {"REV", MACRO_ACTION_EXCEED},
        {"MAX_ACT", MACRO_ACTION_EXCEED},
        {"MAXACT", MACRO_ACTION_EXCEED},
        {"JUMP", MACRO_ACTION_JUMP},
        {"BRINGER", MACRO_ACTION_BRINGER},
        {"DEVIL_BRINGER", MACRO_ACTION_BRINGER},
        {"STYLE_ACTION", MACRO_ACTION_STYLE_ACTION},
        {"STYLE_BUTTON", MACRO_ACTION_STYLE_ACTION},
        {"DEVIL_TRIGGER", MACRO_ACTION_DEVIL_TRIGGER},
        {"DT", MACRO_ACTION_DEVIL_TRIGGER},
        {"LOCK_ON", MACRO_ACTION_LOCK_ON},
        {"LOCKON", MACRO_ACTION_LOCK_ON},
        {"LOCK", MACRO_ACTION_LOCK_ON},
        {"TARGET_LOCK", MACRO_ACTION_LOCK_ON},
        {"CHANGE_GUN", MACRO_ACTION_CHANGE_GUN},
        {"GUN_CHANGE", MACRO_ACTION_CHANGE_GUN},
        {"DANTE_CHANGE_GUN", MACRO_ACTION_CHANGE_GUN},
        {"CHANGE_SWORD", MACRO_ACTION_CHANGE_SWORD},
        {"SWORD_CHANGE", MACRO_ACTION_CHANGE_SWORD},
        {"DANTE_CHANGE_SWORD", MACRO_ACTION_CHANGE_SWORD},
        {"TAUNT", MACRO_ACTION_TAUNT},
        {"CHANGE_TARGET", MACRO_ACTION_CHANGE_TARGET},
        {"TARGET_CHANGE", MACRO_ACTION_CHANGE_TARGET},
        {"CYCLE_TARGET", MACRO_ACTION_CHANGE_TARGET},
        {"NEXT_TARGET", MACRO_ACTION_CHANGE_TARGET},
        {"RESET_CAMERA", MACRO_ACTION_RESET_CAMERA},
        {"CAMERA_RESET", MACRO_ACTION_RESET_CAMERA},
        {"CENTER_CAMERA", MACRO_ACTION_RESET_CAMERA},
    };

    for (const auto& entry : actions) {
        if (normalized == entry.first) {
            action_index = entry.second;
            return true;
        }
    }

    return false;
}

bool is_l2_button_token(const std::string& normalized) {
    return normalized == "L2" || normalized == "LT" || normalized == "LEFT_TRIGGER";
}

struct MacroButtonChoice {
    const char* label;
    uint32_t button;
};

const MacroButtonChoice* macro_button_choices(size_t& count) {
    static const MacroButtonChoice choices[] = {
        {"Y / TRIANGLE", PAD_BUTTON_Y},
        {"X / SQUARE", PAD_BUTTON_X},
        {"A / CROSS", PAD_BUTTON_A},
        {"B / CIRCLE", PAD_BUTTON_B},
        {"LB / L1", PAD_BUTTON_L1},
        {"RB / R1", PAD_BUTTON_R1},
        {"LT / L2", PAD_BUTTON_L2},
        {"RT / R2", PAD_BUTTON_R2},
        {"LS / L3", PAD_BUTTON_L3},
        {"RS / R3", PAD_BUTTON_R3},
        {"START / OPTIONS", PAD_BUTTON_START},
        {"BACK / SHARE", PAD_BUTTON_SELECT},
        {"D-Pad Up", PAD_BUTTON_DPAD_UP},
        {"D-Pad Right", PAD_BUTTON_DPAD_RIGHT},
        {"D-Pad Down", PAD_BUTTON_DPAD_DOWN},
        {"D-Pad Left", PAD_BUTTON_DPAD_LEFT},
    };
    count = sizeof(choices) / sizeof(choices[0]);
    return choices;
}

const char* macro_button_label(uint32_t button) {
    size_t count = 0;
    const auto* choices = macro_button_choices(count);
    for (size_t i = 0; i < count; ++i) {
        if (choices[i].button == button) {
            return choices[i].label;
        }
    }
    return "Unknown";
}

uint32_t sanitize_macro_button(uint32_t button, uint32_t fallback) {
    size_t count = 0;
    const auto* choices = macro_button_choices(count);
    for (size_t i = 0; i < count; ++i) {
        if (choices[i].button == button) {
            return button;
        }
    }
    return fallback;
}

struct MacroActionChoice {
    const char* nero_config_key;
    const char* dante_config_key;
    const char* label;
    uint32_t default_nero_button;
    uint32_t default_dante_button;
};

const MacroActionChoice* macro_action_choices(size_t& count) {
    static const MacroActionChoice choices[MACRO_ACTION_COUNT] = {
        {"keyboard_macro_nero_action_melee", "keyboard_macro_dante_action_melee", "MELEE", PAD_BUTTON_Y, PAD_BUTTON_Y},
        {"keyboard_macro_nero_action_gun", "keyboard_macro_dante_action_gun", "GUN", PAD_BUTTON_X, PAD_BUTTON_X},
        {"keyboard_macro_nero_action_exceed", nullptr, "EXCEED", PAD_BUTTON_L2, 0},
        {"keyboard_macro_nero_action_jump", "keyboard_macro_dante_action_jump", "JUMP", PAD_BUTTON_A, PAD_BUTTON_A},
        {"keyboard_macro_nero_action_bringer", nullptr, "BRINGER", PAD_BUTTON_B, 0},
        {nullptr, "keyboard_macro_dante_action_style_action", "STYLE_ACTION", 0, PAD_BUTTON_B},
        {"keyboard_macro_nero_action_devil_trigger", "keyboard_macro_dante_action_devil_trigger", "DEVIL_TRIGGER", PAD_BUTTON_L1, PAD_BUTTON_L1},
        {"keyboard_macro_nero_action_lock_on", "keyboard_macro_dante_action_lock_on", "LOCK_ON", PAD_BUTTON_R1, PAD_BUTTON_R1},
        {nullptr, "keyboard_macro_dante_action_change_gun", "CHANGE_GUN", 0, PAD_BUTTON_L2},
        {nullptr, "keyboard_macro_dante_action_change_sword", "CHANGE_SWORD", 0, PAD_BUTTON_R2},
        {"keyboard_macro_nero_action_taunt", "keyboard_macro_dante_action_taunt", "TAUNT", PAD_BUTTON_SELECT, PAD_BUTTON_SELECT},
        {"keyboard_macro_nero_action_change_target", "keyboard_macro_dante_action_change_target", "CHANGE_TARGET", PAD_BUTTON_L3, PAD_BUTTON_L3},
        {"keyboard_macro_nero_action_reset_camera", "keyboard_macro_dante_action_reset_camera", "RESET_CAMERA", PAD_BUTTON_R3, PAD_BUTTON_R3},
    };
    count = sizeof(choices) / sizeof(choices[0]);
    return choices;
}

const char* character_role_label(uint32_t role) {
    switch (role) {
    case MACRO_CHARACTER_NERO:
        return "Nero";
    case MACRO_CHARACTER_DANTE:
        return "Dante";
    default:
        return "Unknown";
    }
}

bool parse_character_role(const std::string& expression, uint32_t& role) {
    const auto normalized = normalize_button_token(expression);
    if (normalized == "NERO" || normalized == "N") {
        role = MACRO_CHARACTER_NERO;
        return true;
    }
    if (normalized == "DANTE" || normalized == "D") {
        role = MACRO_CHARACTER_DANTE;
        return true;
    }
    return false;
}

uint32_t current_player_character_role() {
    auto* player = get_local_player_safe();
    if (!player) {
        return MACRO_CHARACTER_INVALID;
    }

    __try {
        if (player->controllerID == 1) {
            return MACRO_CHARACTER_NERO;
        }
        if (player->controllerID == 0) {
            return MACRO_CHARACTER_DANTE;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    }
    return MACRO_CHARACTER_INVALID;
}

bool clip_matches_current_character(const MacroClip& clip) {
    const uint32_t current_role = current_player_character_role();
    return current_role != MACRO_CHARACTER_INVALID && clip.character_role == current_role;
}

bool macro_action_valid_for_role(uint32_t action_index, uint32_t role) {
    if (action_index >= MACRO_ACTION_COUNT || role >= MACRO_CHARACTER_ROLE_COUNT) {
        return false;
    }

    size_t action_count = 0;
    const auto* actions = macro_action_choices(action_count);
    if (action_index >= action_count) {
        return false;
    }

    if (role == MACRO_CHARACTER_NERO) {
        return actions[action_index].nero_config_key != nullptr;
    }
    if (role == MACRO_CHARACTER_DANTE) {
        return actions[action_index].dante_config_key != nullptr;
    }
    return false;
}

uint32_t default_action_button_for_role(uint32_t action_index, uint32_t role) {
    if (action_index >= MACRO_ACTION_COUNT || role >= MACRO_CHARACTER_ROLE_COUNT) {
        return 0;
    }

    if (role == MACRO_CHARACTER_NERO) {
        return DEFAULT_NERO_ACTION_BUTTON_MAP[action_index];
    }
    return DEFAULT_DANTE_ACTION_BUTTON_MAP[action_index];
}

uint32_t action_button_for_role(uint32_t action_index, uint32_t role) {
    if (!macro_action_valid_for_role(action_index, role)) {
        return 0;
    }
    return Macro::action_button_map[role][action_index];
}

uint32_t resolve_action_buttons(uint32_t actions, uint32_t role) {
    uint32_t buttons = 0;
    if (role >= MACRO_CHARACTER_ROLE_COUNT) {
        return buttons;
    }

    for (uint32_t action_index = 0; action_index < MACRO_ACTION_COUNT; ++action_index) {
        if ((actions & MACRO_ACTION_BIT(action_index)) != 0) {
            buttons |= action_button_for_role(action_index, role);
        }
    }
    return buttons;
}

uint32_t resolve_global_action_buttons(uint32_t actions, uint32_t role) {
    uint32_t buttons = 0;
    if ((actions & MACRO_ACTION_BIT(MACRO_ACTION_RESET_CAMERA)) != 0) {
        buttons |= action_button_for_role(MACRO_ACTION_RESET_CAMERA, role);
    }
    return buttons;
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

bool parse_wait_compare_operator(const std::string& token, uint32_t& compare) {
    const auto normalized = normalize_button_token(token);
    if (normalized == "==" || normalized == "=" || normalized == "EQ" || normalized == "EQUAL" || normalized == "EQUALS") {
        compare = MACRO_WAIT_COMPARE_EQ;
    }
    else if (normalized == "!=" || normalized == "<>" || normalized == "NE" || normalized == "NOT_EQUAL" || normalized == "NOT_EQUALS") {
        compare = MACRO_WAIT_COMPARE_NE;
    }
    else if (normalized == ">" || normalized == "GT" || normalized == "GREATER" || normalized == "GREATER_THAN") {
        compare = MACRO_WAIT_COMPARE_GT;
    }
    else if (normalized == ">=" || normalized == "=>" || normalized == "GTE" || normalized == "GE" || normalized == "GREATER_EQUAL" ||
        normalized == "GREATER_THAN_OR_EQUAL") {
        compare = MACRO_WAIT_COMPARE_GTE;
    }
    else if (normalized == "<" || normalized == "LT" || normalized == "LESS" || normalized == "LESS_THAN") {
        compare = MACRO_WAIT_COMPARE_LT;
    }
    else if (normalized == "<=" || normalized == "=<" || normalized == "LTE" || normalized == "LE" || normalized == "LESS_EQUAL" ||
        normalized == "LESS_THAN_OR_EQUAL") {
        compare = MACRO_WAIT_COMPARE_LTE;
    }
    else {
        return false;
    }

    return true;
}

bool parse_wait_float_compare(std::istringstream& stream, uint32_t default_compare, uint32_t& compare, float& value) {
    std::string first_token{};
    stream >> first_token;
    if (first_token.empty()) {
        return false;
    }

    compare = default_compare;
    std::string value_token = first_token;
    uint32_t parsed_compare = 0;
    if (parse_wait_compare_operator(first_token, parsed_compare)) {
        compare = parsed_compare;
        stream >> value_token;
        if (value_token.empty()) {
            return false;
        }
    }

    return parse_float(value_token, value);
}

bool parse_wait_u32_compare(std::istringstream& stream, uint32_t default_compare, uint32_t& compare, uint32_t& value) {
    std::string first_token{};
    stream >> first_token;
    if (first_token.empty()) {
        return false;
    }

    compare = default_compare;
    std::string value_token = first_token;
    uint32_t parsed_compare = 0;
    if (parse_wait_compare_operator(first_token, parsed_compare)) {
        compare = parsed_compare;
        stream >> value_token;
        if (value_token.empty()) {
            return false;
        }
    }

    return parse_integer(value_token, value);
}

bool parse_wait_condition(
    std::istringstream& stream,
    uint32_t& condition,
    int& arg,
    uint32_t& compare,
    float& value,
    uint32_t& value_u32,
    uint32_t& max_ticks) {
    condition = MACRO_WAIT_NONE;
    arg = 0;
    compare = 0;
    value = 0.0f;
    value_u32 = 0;
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
    else if (normalized == "CAN_EXCEED" || normalized == "EXCEED_WINDOW") {
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
    else if (normalized == "ANIM_FRAME" || normalized == "ANIMFRAME" || normalized == "ANIMATION_FRAME" || normalized == "FRAME") {
        condition = MACRO_WAIT_ANIM_FRAME;
        if (!parse_wait_float_compare(stream, MACRO_WAIT_COMPARE_GTE, compare, value)) {
            return false;
        }
    }
    else if (normalized == "MOVEID2" || normalized == "MOVE_ID2" || normalized == "MOVE_ID" || normalized == "MOVEID") {
        condition = MACRO_WAIT_MOVEID2;
        if (!parse_wait_u32_compare(stream, MACRO_WAIT_COMPARE_EQ, compare, value_u32)) {
            return false;
        }
    }
    else if (normalized == "MOVEID2_CHANGED" || normalized == "MOVE_ID2_CHANGED" || normalized == "MOVE_CHANGED" || normalized == "MOVE_ID_CHANGED") {
        condition = MACRO_WAIT_MOVEID2_CHANGED;
    }
    else if (normalized == "FRAME_REACHED_MAX" || normalized == "ANIM_FRAME_REACHED_MAX" || normalized == "FRAME_MAX" ||
        normalized == "ANIM_FRAME_MAX") {
        condition = MACRO_WAIT_FRAME_REACHED_MAX;
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
        {"MOVE_UP", {0, ANALOG_MAX}},
        {"MOVE_FORWARD", {0, ANALOG_MAX}},
        {"FORWARD", {0, ANALOG_MAX}},
        {"LSTICK_UP", {0, ANALOG_MAX}},
        {"LS_UP", {0, ANALOG_MAX}},
        {"WALK_UP", {0, ANALOG_WALK}},
        {"WALK_FORWARD", {0, ANALOG_WALK}},
        {"SLOW_UP", {0, ANALOG_WALK}},
        {"SLOW_FORWARD", {0, ANALOG_WALK}},
        {"MOVE_DOWN", {0, ANALOG_MIN}},
        {"MOVE_BACK", {0, ANALOG_MIN}},
        {"BACKWARD", {0, ANALOG_MIN}},
        {"LSTICK_DOWN", {0, ANALOG_MIN}},
        {"LS_DOWN", {0, ANALOG_MIN}},
        {"WALK_DOWN", {0, -ANALOG_WALK}},
        {"WALK_BACK", {0, -ANALOG_WALK}},
        {"WALK_BACKWARD", {0, -ANALOG_WALK}},
        {"SLOW_DOWN", {0, -ANALOG_WALK}},
        {"SLOW_BACK", {0, -ANALOG_WALK}},
        {"SLOW_BACKWARD", {0, -ANALOG_WALK}},
        {"MOVE_RIGHT", {ANALOG_MAX, 0}},
        {"LSTICK_RIGHT", {ANALOG_MAX, 0}},
        {"LS_RIGHT", {ANALOG_MAX, 0}},
        {"WALK_RIGHT", {ANALOG_WALK, 0}},
        {"SLOW_RIGHT", {ANALOG_WALK, 0}},
        {"MOVE_LEFT", {ANALOG_MIN, 0}},
        {"LSTICK_LEFT", {ANALOG_MIN, 0}},
        {"LS_LEFT", {ANALOG_MIN, 0}},
        {"WALK_LEFT", {-ANALOG_WALK, 0}},
        {"SLOW_LEFT", {-ANALOG_WALK, 0}},
        {"MOVE_FORWARD_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"MOVE_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"LSTICK_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"LS_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"WALK_FORWARD_RIGHT", {ANALOG_WALK, ANALOG_WALK}},
        {"WALK_UP_RIGHT", {ANALOG_WALK, ANALOG_WALK}},
        {"SLOW_FORWARD_RIGHT", {ANALOG_WALK, ANALOG_WALK}},
        {"SLOW_UP_RIGHT", {ANALOG_WALK, ANALOG_WALK}},
        {"MOVE_FORWARD_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"MOVE_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"LSTICK_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"LS_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"WALK_FORWARD_LEFT", {-ANALOG_WALK, ANALOG_WALK}},
        {"WALK_UP_LEFT", {-ANALOG_WALK, ANALOG_WALK}},
        {"SLOW_FORWARD_LEFT", {-ANALOG_WALK, ANALOG_WALK}},
        {"SLOW_UP_LEFT", {-ANALOG_WALK, ANALOG_WALK}},
        {"MOVE_BACK_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"MOVE_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"LSTICK_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"LS_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"WALK_BACK_RIGHT", {ANALOG_WALK, -ANALOG_WALK}},
        {"WALK_DOWN_RIGHT", {ANALOG_WALK, -ANALOG_WALK}},
        {"SLOW_BACK_RIGHT", {ANALOG_WALK, -ANALOG_WALK}},
        {"SLOW_DOWN_RIGHT", {ANALOG_WALK, -ANALOG_WALK}},
        {"MOVE_BACK_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"MOVE_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"LSTICK_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"LS_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"WALK_BACK_LEFT", {-ANALOG_WALK, -ANALOG_WALK}},
        {"WALK_DOWN_LEFT", {-ANALOG_WALK, -ANALOG_WALK}},
        {"SLOW_BACK_LEFT", {-ANALOG_WALK, -ANALOG_WALK}},
        {"SLOW_DOWN_LEFT", {-ANALOG_WALK, -ANALOG_WALK}},
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

bool parse_right_analog_name(const std::string& token, int& x, int& y) {
    const auto normalized = normalize_button_token(token);

    static const std::pair<const char*, std::pair<int, int>> directions[] = {
        {"TW", {0, ANALOG_MAX}},
        {"CAM_UP", {0, ANALOG_MAX}},
        {"CAMERA_UP", {0, ANALOG_MAX}},
        {"RSTICK_UP", {0, ANALOG_MAX}},
        {"RS_UP", {0, ANALOG_MAX}},
        {"RIGHT_STICK_UP", {0, ANALOG_MAX}},
        {"TS", {0, ANALOG_MIN}},
        {"CAM_DOWN", {0, ANALOG_MIN}},
        {"CAMERA_DOWN", {0, ANALOG_MIN}},
        {"RSTICK_DOWN", {0, ANALOG_MIN}},
        {"RS_DOWN", {0, ANALOG_MIN}},
        {"RIGHT_STICK_DOWN", {0, ANALOG_MIN}},
        {"TD", {ANALOG_MAX, 0}},
        {"CAM_RIGHT", {ANALOG_MAX, 0}},
        {"CAMERA_RIGHT", {ANALOG_MAX, 0}},
        {"RSTICK_RIGHT", {ANALOG_MAX, 0}},
        {"RS_RIGHT", {ANALOG_MAX, 0}},
        {"RIGHT_STICK_RIGHT", {ANALOG_MAX, 0}},
        {"TA", {ANALOG_MIN, 0}},
        {"CAM_LEFT", {ANALOG_MIN, 0}},
        {"CAMERA_LEFT", {ANALOG_MIN, 0}},
        {"RSTICK_LEFT", {ANALOG_MIN, 0}},
        {"RS_LEFT", {ANALOG_MIN, 0}},
        {"RIGHT_STICK_LEFT", {ANALOG_MIN, 0}},
        {"TW_TD", {ANALOG_MAX, ANALOG_MAX}},
        {"CAM_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"CAMERA_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"RS_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"RSTICK_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"RIGHT_STICK_UP_RIGHT", {ANALOG_MAX, ANALOG_MAX}},
        {"TW_TA", {ANALOG_MIN, ANALOG_MAX}},
        {"CAM_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"CAMERA_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"RS_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"RSTICK_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"RIGHT_STICK_UP_LEFT", {ANALOG_MIN, ANALOG_MAX}},
        {"TS_TD", {ANALOG_MAX, ANALOG_MIN}},
        {"CAM_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"CAMERA_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"RS_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"RSTICK_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"RIGHT_STICK_DOWN_RIGHT", {ANALOG_MAX, ANALOG_MIN}},
        {"TS_TA", {ANALOG_MIN, ANALOG_MIN}},
        {"CAM_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"CAMERA_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"RS_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"RSTICK_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
        {"RIGHT_STICK_DOWN_LEFT", {ANALOG_MIN, ANALOG_MIN}},
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
        if (std::abs(x) < ANALOG_MAX) {
            input.walk_left = true;
        }
        else {
            input.move_left = true;
        }
    }
    else if (x > 0) {
        if (std::abs(x) < ANALOG_MAX) {
            input.walk_right = true;
        }
        else {
            input.move_right = true;
        }
    }

    if (y < 0) {
        if (std::abs(y) < ANALOG_MAX) {
            input.walk_down = true;
        }
        else {
            input.move_down = true;
        }
    }
    else if (y > 0) {
        if (std::abs(y) < ANALOG_MAX) {
            input.walk_up = true;
        }
        else {
            input.move_up = true;
        }
    }
}

void apply_camera_direction_to_input(ParsedMacroInput& input, int x, int y) {
    if (x < 0) {
        input.camera_left = true;
    }
    else if (x > 0) {
        input.camera_right = true;
    }

    if (y < 0) {
        input.camera_down = true;
    }
    else if (y > 0) {
        input.camera_up = true;
    }
}

MacroFrame frame_from_parts(
    uint32_t buttons,
    uint32_t global_buttons,
    uint32_t actions,
    bool move_up,
    bool move_down,
    bool move_left,
    bool move_right,
    bool walk_up,
    bool walk_down,
    bool walk_left,
    bool walk_right,
    bool camera_up,
    bool camera_down,
    bool camera_left,
    bool camera_right,
    bool screen_pause,
    bool screen_resume,
    bool screen_pause_toggle) {
    MacroFrame frame{};
    frame.buttons = buttons;
    frame.global_buttons = global_buttons;
    frame.actions = actions;
    frame.left_x = clamp_analog(
        (move_right ? ANALOG_MAX : 0) + (move_left ? ANALOG_MIN : 0) +
        (walk_right ? ANALOG_WALK : 0) + (walk_left ? -ANALOG_WALK : 0));
    frame.left_y = clamp_analog(
        (move_up ? ANALOG_MAX : 0) + (move_down ? ANALOG_MIN : 0) +
        (walk_up ? ANALOG_WALK : 0) + (walk_down ? -ANALOG_WALK : 0));
    frame.has_left_analog = move_up || move_down || move_left || move_right || walk_up || walk_down || walk_left || walk_right;
    frame.right_x = clamp_analog((camera_right ? ANALOG_MAX : 0) + (camera_left ? ANALOG_MIN : 0));
    frame.right_y = clamp_analog((camera_up ? ANALOG_MAX : 0) + (camera_down ? ANALOG_MIN : 0));
    frame.has_right_analog = camera_up || camera_down || camera_left || camera_right;
    frame.screen_pause = screen_pause;
    frame.screen_resume = screen_resume;
    frame.screen_pause_toggle = screen_pause_toggle;
    return frame;
}

MacroFrame frame_from_input(const ParsedMacroInput& input) {
    return frame_from_parts(
        input.buttons,
        input.global_buttons,
        input.actions,
        input.move_up,
        input.move_down,
        input.move_left,
        input.move_right,
        input.walk_up,
        input.walk_down,
        input.walk_left,
        input.walk_right,
        input.camera_up,
        input.camera_down,
        input.camera_left,
        input.camera_right,
        input.screen_pause,
        input.screen_resume,
        input.screen_pause_toggle);
}

MacroFrame frame_from_forced_style(int style) {
    MacroFrame frame{};
    frame.force_style = true;
    frame.forced_style = style;
    return frame;
}

MacroFrame frame_from_character_switch() {
    MacroFrame frame{};
    frame.character_switch = true;
    return frame;
}

MacroFrame frame_from_one_hit_kill(bool toggle, bool value) {
    MacroFrame frame{};
    frame.one_hit_kill_toggle = toggle;
    frame.one_hit_kill_set = !toggle;
    frame.one_hit_kill_value = value;
    return frame;
}

MacroFrame frame_from_held(const HeldMacroInput& input) {
    return frame_from_parts(
        input.buttons,
        input.global_buttons,
        input.actions,
        input.move_up,
        input.move_down,
        input.move_left,
        input.move_right,
        input.walk_up,
        input.walk_down,
        input.walk_left,
        input.walk_right,
        input.camera_up,
        input.camera_down,
        input.camera_left,
        input.camera_right,
        input.screen_pause,
        input.screen_resume,
        input.screen_pause_toggle);
}

MacroFrame frame_from_wait_condition(
    const HeldMacroInput& input,
    uint32_t condition,
    int arg,
    uint32_t compare,
    float value,
    uint32_t value_u32,
    uint32_t max_ticks) {
    auto frame = frame_from_held(input);
    frame.wait_condition = condition;
    frame.wait_arg = arg;
    frame.wait_compare = compare;
    frame.wait_value = value;
    frame.wait_value_u32 = value_u32;
    frame.wait_initial_u32 = 0;
    frame.wait_initial_valid = false;
    frame.wait_max_ticks = max_ticks;
    frame.wait_elapsed_ticks = 0;
    return frame;
}

bool parse_macro_input(std::string expression, ParsedMacroInput& input, uint32_t character_role) {
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

        uint32_t action_index = 0;
        if (parse_action_name(token, action_index)) {
            if (action_index < MACRO_ACTION_COUNT) {
                if (!macro_action_valid_for_role(action_index, character_role)) {
                    return false;
                }
                input.actions |= MACRO_ACTION_BIT(action_index);
                found_token = true;
                continue;
            }
            return false;
        }

        int direction_x = 0;
        int direction_y = 0;
        if (parse_right_analog_name(token, direction_x, direction_y)) {
            apply_camera_direction_to_input(input, direction_x, direction_y);
            found_token = true;
            continue;
        }

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
        if ((button & PAD_BUTTON_GLOBAL_ROUTE_MASK) != 0) {
            input.global_buttons |= button;
        }
        found_token = true;
    }

    return found_token;
}

bool parse_playback_command(std::string expression, MacroFrame& frame, uint32_t character_role) {
    ParsedMacroInput input{};
    if (!parse_macro_input(std::move(expression), input, character_role)) {
        return false;
    }

    frame = frame_from_input(input);
    return true;
}

void hold_input(HeldMacroInput& held, const ParsedMacroInput& input) {
    held.buttons |= input.buttons;
    held.global_buttons |= input.global_buttons;
    held.actions |= input.actions;
    held.move_up |= input.move_up;
    held.move_down |= input.move_down;
    held.move_left |= input.move_left;
    held.move_right |= input.move_right;
    held.walk_up |= input.walk_up;
    held.walk_down |= input.walk_down;
    held.walk_left |= input.walk_left;
    held.walk_right |= input.walk_right;
    held.camera_up |= input.camera_up;
    held.camera_down |= input.camera_down;
    held.camera_left |= input.camera_left;
    held.camera_right |= input.camera_right;
    held.screen_pause |= input.screen_pause;
    held.screen_resume |= input.screen_resume;
    held.screen_pause_toggle |= input.screen_pause_toggle;
}

void release_input(HeldMacroInput& held, const ParsedMacroInput& input) {
    held.buttons &= ~input.buttons;
    held.global_buttons &= ~input.global_buttons;
    held.actions &= ~input.actions;
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
    if (input.walk_up) {
        held.walk_up = false;
    }
    if (input.walk_down) {
        held.walk_down = false;
    }
    if (input.walk_left) {
        held.walk_left = false;
    }
    if (input.walk_right) {
        held.walk_right = false;
    }
    if (input.camera_up) {
        held.camera_up = false;
    }
    if (input.camera_down) {
        held.camera_down = false;
    }
    if (input.camera_left) {
        held.camera_left = false;
    }
    if (input.camera_right) {
        held.camera_right = false;
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

bool append_playback_frames(std::vector<MacroFrame>& frames, uint32_t frame_count, const MacroFrame& frame) {
    if (frame_count == 0 || frames.size() + frame_count > MAX_PLAYBACK_FRAMES) {
        return false;
    }

    frames.insert(frames.end(), frame_count, frame);
    return true;
}

bool append_tap_macro(
    std::vector<MacroFrame>& frames,
    HeldMacroInput& held,
    const std::string& input_expression,
    uint32_t ticks,
    uint32_t character_role) {
    ParsedMacroInput input{};
    if (ticks == 0 || !parse_macro_input(input_expression, input, character_role)) {
        return false;
    }

    const auto saved_held = held;
    hold_input(held, input);
    const bool ok = append_playback_frames(frames, ticks, frame_from_held(held));
    held = saved_held;
    return ok;
}

bool append_direction_button_macro(
    std::vector<MacroFrame>& frames,
    HeldMacroInput& held,
    const std::string& direction_expression,
    const std::string& input_expression,
    uint32_t ticks,
    uint32_t character_role) {
    ParsedMacroInput direction{};
    ParsedMacroInput input{};
    if (ticks == 0 || !parse_macro_input(direction_expression, direction, character_role) ||
        !parse_macro_input(input_expression, input, character_role)) {
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
    std::vector<MacroFrame>& frames,
    HeldMacroInput& held,
    const std::string& back_expression,
    const std::string& forward_expression,
    const std::string& input_expression,
    uint32_t character_role) {
    ParsedMacroInput back{};
    ParsedMacroInput forward{};
    ParsedMacroInput input{};
    if (!parse_macro_input(back_expression, back, character_role) ||
        !parse_macro_input(forward_expression, forward, character_role) ||
        !parse_macro_input(input_expression, input, character_role)) {
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

bool write_base_player_input_snapshot(cPeripheral* peripheral) {
    if (!peripheral) {
        return false;
    }

    auto* pad = get_global_pad_safe();
    if (!pad) {
        return false;
    }

    __try {
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
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
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

void apply_global_pad_route_for_macro(
    const MacroFrame& frame,
    uint32_t routed_buttons,
    uint32_t previous_routed_buttons,
    uint32_t player_index) {
    if (player_index >= 4) {
        player_index = 0;
    }

    auto* pad = get_global_pad_safe();
    if (!pad) {
        last_global_routed_buttons[player_index] = routed_buttons;
        last_global_right_analog[player_index] = frame.has_right_analog;
        return;
    }

    __try {
        auto& pad_info = pad->mPadInfo[0];
        const uint32_t pressed_buttons = routed_buttons & ~previous_routed_buttons;
        const uint32_t released_buttons = previous_routed_buttons & ~routed_buttons;

        pad_info.mBtn.on &= ~previous_routed_buttons;
        pad_info.mBtn.on |= routed_buttons;
        pad_info.mBtn.trg |= pressed_buttons;
        pad_info.mBtn.rel |= released_buttons;
        set_global_pad_press(pad_info.mPress, routed_buttons);

        if (frame.has_right_analog) {
            pad_info.mAnlg[1].x = std::clamp(
                pad_info.mAnlg[1].x + (static_cast<float>(frame.right_x) / static_cast<float>(ANALOG_MAX)),
                -1.0f,
                1.0f);
            pad_info.mAnlg[1].y = std::clamp(
                pad_info.mAnlg[1].y + (static_cast<float>(frame.right_y) / static_cast<float>(ANALOG_MAX)),
                -1.0f,
                1.0f);
        }
        else if (last_global_right_analog[player_index]) {
            pad_info.mAnlg[1].x = 0.0f;
            pad_info.mAnlg[1].y = 0.0f;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
    }

    last_global_routed_buttons[player_index] = routed_buttons;
    last_global_right_analog[player_index] = frame.has_right_analog;
}

void set_playback_status(const std::string& message) {
    strncpy_s(Macro::playback_status, message.c_str(), _TRUNCATE);
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

bool evaluate_hit_confirmed_wait_condition(const MacroFrame& frame) {
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
    std::fill_n(Macro::last_buttons, 4, 0);
    Macro::clear_input_frames = 2;
    Macro::input_active = Macro::clear_input_frames > 0 || (Macro::mod_enabled && Macro::playback_enabled);
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
    if (!macro_runtime_ready()) {
        set_playback_status("Battle Snapshot capture failed: gameplay is unavailable.");
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
        strncpy_s(Macro::playback_status, "Battle Snapshot capture failed: gameplay objects changed while capturing.", _TRUNCATE);
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

    if (!macro_runtime_ready()) {
        if (update_status) {
            set_playback_status("Battle Snapshot load failed: gameplay is unavailable.");
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
            strncpy_s(Macro::playback_status, "Battle Snapshot load failed: gameplay objects changed while loading.", _TRUNCATE);
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
        ++Macro::screen_pause_fail_count;
        return false;
    }

    __try {
        if (paused) {
            if (!Macro::screen_pause_active) {
                Macro::screen_pause_restore_speed = work_rate->global_speed;
                Macro::screen_pause_restore_valid = true;
            }
            work_rate->global_speed = 0.0f;
            WorkRate::hotkey_paused = true;
            Macro::screen_pause_active = true;
            ++Macro::screen_pause_request_count;
            return true;
        }

        if (Macro::screen_pause_restore_valid) {
            work_rate->global_speed = Macro::screen_pause_restore_speed;
        }
        else {
            work_rate->global_speed = 1.0f;
        }
        WorkRate::hotkey_paused = false;
        Macro::screen_pause_active = false;
        Macro::screen_pause_restore_valid = false;
        ++Macro::screen_pause_request_count;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        ++Macro::screen_pause_fail_count;
        return false;
    }
}

void apply_screen_pause_action(const MacroFrame& frame) {
    if (frame.screen_pause_toggle) {
        set_screen_pause(!Macro::screen_pause_active);
    }
    if (frame.screen_pause) {
        set_screen_pause(true);
    }
    if (frame.screen_resume) {
        set_screen_pause(false);
    }
}

void apply_force_style_action(const MacroFrame& frame) {
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

void apply_character_switch_action(const MacroFrame& frame) {
    if (!frame.character_switch) {
        return;
    }

    if (CharSwitcher::request_macro_switch()) {
        set_playback_status("Character switch requested.");
    }
    else {
        set_playback_status("Character Switcher is not enabled or ready.");
    }
}

void apply_one_hit_kill_action(const MacroFrame& frame) {
    if (frame.one_hit_kill_toggle) {
        HealthSettings::one_hit_kill = !HealthSettings::one_hit_kill;
        set_playback_status(HealthSettings::one_hit_kill ? "One Hit Kill enabled." : "One Hit Kill disabled.");
    }
    else if (frame.one_hit_kill_set) {
        HealthSettings::one_hit_kill = frame.one_hit_kill_value;
        set_playback_status(HealthSettings::one_hit_kill ? "One Hit Kill enabled." : "One Hit Kill disabled.");
    }
}

bool is_player_grounded(uPlayer* player) {
    if (!player) {
        return false;
    }

    if (player->grounded == 2) {
        return false;
    }
    if (player->grounded == 1) {
        return true;
    }

    if (player->grounded2) {
        return true;
    }

    return player->collisionSettings && player->collisionSettings->mLand != 0;
}

bool compare_wait_float(float current, uint32_t compare, float target) {
    switch (compare) {
    case MACRO_WAIT_COMPARE_EQ:
        return std::fabs(current - target) <= 0.001f;
    case MACRO_WAIT_COMPARE_NE:
        return std::fabs(current - target) > 0.001f;
    case MACRO_WAIT_COMPARE_GT:
        return current > target;
    case MACRO_WAIT_COMPARE_GTE:
        return current >= target;
    case MACRO_WAIT_COMPARE_LT:
        return current < target;
    case MACRO_WAIT_COMPARE_LTE:
        return current <= target;
    default:
        return false;
    }
}

bool compare_wait_u32(uint32_t current, uint32_t compare, uint32_t target) {
    switch (compare) {
    case MACRO_WAIT_COMPARE_EQ:
        return current == target;
    case MACRO_WAIT_COMPARE_NE:
        return current != target;
    case MACRO_WAIT_COMPARE_GT:
        return current > target;
    case MACRO_WAIT_COMPARE_GTE:
        return current >= target;
    case MACRO_WAIT_COMPARE_LT:
        return current < target;
    case MACRO_WAIT_COMPARE_LTE:
        return current <= target;
    default:
        return false;
    }
}

bool has_valid_anim_frame_max(float value) {
    return std::isfinite(value) && value > 0.0f && value < 100000.0f;
}

bool wait_for_next_bool_edge(MacroFrame& frame, bool current_active) {
    if (!frame.wait_initial_valid) {
        frame.wait_initial_u32 = current_active ? 0u : 1u;
        frame.wait_initial_valid = true;
        return false;
    }
    if (!current_active) {
        frame.wait_initial_u32 = 1u;
        return false;
    }
    return frame.wait_initial_u32 != 0;
}

bool evaluate_wait_condition(MacroFrame& frame) {
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
            return wait_for_next_bool_edge(frame, player->canExceed == 1);
        case MACRO_WAIT_HITSTOP:
            return player->hitstop || player->hitstopTimer > 0.0f;
        case MACRO_WAIT_HIT_CONFIRMED:
            return evaluate_hit_confirmed_wait_condition(frame);
        case MACRO_WAIT_LOCKED_ON:
            return player->lockedOn;
        case MACRO_WAIT_ANIM_FRAME:
            return std::isfinite(player->animFrame) && compare_wait_float(player->animFrame, frame.wait_compare, frame.wait_value);
        case MACRO_WAIT_MOVEID2:
            return compare_wait_u32(player->moveID2, frame.wait_compare, frame.wait_value_u32);
        case MACRO_WAIT_MOVEID2_CHANGED:
            if (!frame.wait_initial_valid) {
                frame.wait_initial_u32 = player->moveID2;
                frame.wait_initial_valid = true;
                return false;
            }
            return player->moveID2 != frame.wait_initial_u32;
        case MACRO_WAIT_FRAME_REACHED_MAX:
            return std::isfinite(player->animFrame) && has_valid_anim_frame_max(player->animFrameMax) &&
                player->animFrame >= player->animFrameMax - 0.001f;
        default:
            return false;
        }
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

bool should_advance_wait_frame(MacroFrame& frame) {
    if (frame.wait_condition == MACRO_WAIT_NONE) {
        return true;
    }

    if (evaluate_wait_condition(frame)) {
        frame.wait_elapsed_ticks = 0;
        frame.wait_initial_valid = false;
        return true;
    }

    if (frame.wait_max_ticks != 0 && frame.wait_elapsed_ticks >= frame.wait_max_ticks) {
        frame.wait_elapsed_ticks = 0;
        frame.wait_initial_valid = false;
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
    if (macro_instance && capture_hotkey_target >= 1 && capture_hotkey_target <= macro_instance->m_hotkeys.size()) {
        auto& hotkey = macro_instance->m_hotkeys[capture_hotkey_target - 1];
        hotkey->m_binds = binds;
        hotkey->m_setting = false;

        if (capture_hotkey_target == 1) {
            Macro::reload_vkey = binds.back();
        }
        else if (capture_hotkey_target == 2) {
            Macro::restart_vkey = binds.back();
        }
        else if (capture_hotkey_target == 3) {
            Macro::stop_vkey = binds.back();
        }
        else if (capture_hotkey_target == 4) {
            Macro::capture_snapshot_vkey = binds.back();
        }
        else if (capture_hotkey_target == 5) {
            Macro::load_snapshot_vkey = binds.back();
        }
        else if (capture_hotkey_target == 6) {
            Macro::load_snapshot_play_vkey = binds.back();
        }

        update_config_hotkey_vkeys(macro_instance->m_hotkeys);

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

std::string clip_display_label(const MacroClip& clip, uint32_t clip_index) {
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
    if (clip.gamepad_hotkey_button != 0) {
        stream << " [Back/Select + " << gamepad_hotkey_button_label(clip.gamepad_hotkey_button) << "]";
    }
    if (clip.character_role != MACRO_CHARACTER_INVALID) {
        stream << " [" << character_role_label(clip.character_role) << "]";
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
    if (hotkeys.size() < 6) {
        return;
    }

    if (!hotkeys[0]->m_binds.empty()) {
        Macro::reload_vkey = hotkeys[0]->m_binds.back();
    }
    if (!hotkeys[1]->m_binds.empty()) {
        Macro::restart_vkey = hotkeys[1]->m_binds.back();
    }
    if (!hotkeys[2]->m_binds.empty()) {
        Macro::stop_vkey = hotkeys[2]->m_binds.back();
    }
    if (!hotkeys[3]->m_binds.empty()) {
        Macro::capture_snapshot_vkey = hotkeys[3]->m_binds.back();
    }
    if (!hotkeys[4]->m_binds.empty()) {
        Macro::load_snapshot_vkey = hotkeys[4]->m_binds.back();
    }
    if (!hotkeys[5]->m_binds.empty()) {
        Macro::load_snapshot_play_vkey = hotkeys[5]->m_binds.back();
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

bool Macro::mod_enabled = false;
bool Macro::playback_enabled = false;
bool Macro::input_active = false;
uint32_t Macro::last_buttons[4] = {};
uint32_t Macro::playback_frame_index = 0;
uint32_t Macro::clear_input_frames = 0;
uint32_t Macro::reload_vkey = DEFAULT_RELOAD_VKEY;
uint32_t Macro::restart_vkey = DEFAULT_RESTART_VKEY;
uint32_t Macro::stop_vkey = DEFAULT_STOP_VKEY;
uint32_t Macro::capture_snapshot_vkey = DEFAULT_CAPTURE_SNAPSHOT_VKEY;
uint32_t Macro::load_snapshot_vkey = DEFAULT_LOAD_SNAPSHOT_VKEY;
uint32_t Macro::load_snapshot_play_vkey = DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY;
uint32_t Macro::snapshot_play_delay_ticks = POSITION_SNAPSHOT_LOAD_TICKS;
uint32_t Macro::action_button_map[MACRO_CHARACTER_ROLE_COUNT][MACRO_ACTION_COUNT] = {
    {
        PAD_BUTTON_Y,
        PAD_BUTTON_X,
        PAD_BUTTON_L2,
        PAD_BUTTON_A,
        PAD_BUTTON_B,
        0,
        PAD_BUTTON_L1,
        PAD_BUTTON_R1,
        0,
        0,
        PAD_BUTTON_SELECT,
        PAD_BUTTON_L3,
        PAD_BUTTON_R3,
    },
    {
        PAD_BUTTON_Y,
        PAD_BUTTON_X,
        0,
        PAD_BUTTON_A,
        0,
        PAD_BUTTON_B,
        PAD_BUTTON_L1,
        PAD_BUTTON_R1,
        PAD_BUTTON_L2,
        PAD_BUTTON_R2,
        PAD_BUTTON_SELECT,
        PAD_BUTTON_L3,
        PAD_BUTTON_R3,
    },
};
uint32_t Macro::gamepad_hotkey_buttons[GAMEPAD_HOTKEY_COUNT] = {
    PAD_BUTTON_A,
    PAD_BUTTON_B,
    PAD_BUTTON_X,
    PAD_BUTTON_Y,
    PAD_BUTTON_R1,
};
bool Macro::auto_reload_file = true;
bool Macro::stop_macro_on_game_pause = false;
bool Macro::gamepad_hotkeys_enabled = false;
uint32_t Macro::playback_slot = PLAYBACK_SLOT_MAIN;
uint32_t Macro::selected_clip_index = 0;
uint32_t Macro::loaded_clip_index = INVALID_CLIP_INDEX;
uint32_t Macro::playback_character_role = MACRO_CHARACTER_INVALID;
bool Macro::screen_pause_active = false;
bool Macro::screen_pause_restore_valid = false;
float Macro::screen_pause_restore_speed = 1.0f;
uint32_t Macro::screen_pause_request_count = 0;
uint32_t Macro::screen_pause_fail_count = 0;
char Macro::playback_path[260] = "macro.txt";
char Macro::loaded_playback_path[260] = "";
char Macro::playback_status[256] = "No Macro file loaded.";
std::vector<MacroClip> Macro::playback_clips{};
std::vector<MacroFrame> Macro::playback_frames{};

std::optional<std::string> Macro::on_initialize() {
    macro_instance = this;
    ensure_keyboard_hotkeys(m_hotkeys);
    return Mod::on_initialize();
}

void __stdcall Macro::on_pad_update_tick(cPeripheral* peripheral) {
    if (!mod_enabled) {
        poll_raw_keyboard(false);
        poll_gamepad_hotkeys(peripheral, false);
        return;
    }

    if (!macro_runtime_ready()) {
        suspend_macro_runtime_for_transition();
        poll_raw_keyboard(false);
        poll_gamepad_hotkeys(peripheral, false);
        return;
    }

    tick_snapshot_play_delay();

    if (macro_instance) {
        macro_instance->check_hotkeys();
    }
    else {
        poll_raw_keyboard(true);
    }

    poll_gamepad_hotkeys(peripheral, true);
}

void __stdcall Macro::on_player_pad_update(cPeripheral* peripheral) {
    if (!peripheral || !input_active) {
        return;
    }

    if (!macro_runtime_ready()) {
        suspend_macro_runtime_for_transition();
        return;
    }

    uint32_t player_index = 0;
    if (!read_peripheral_player_index_safe(peripheral, player_index)) {
        suspend_macro_runtime_for_transition();
        return;
    }
    if (player_index >= 4) {
        player_index = 0;
    }

    last_player_peripheral = peripheral;
    last_player_index = player_index;
    write_test_input(peripheral, player_index);
}

void Macro::write_test_input(cPeripheral* peripheral, uint32_t player_index) {
    if (!peripheral || !macro_runtime_ready()) {
        suspend_macro_runtime_for_transition();
        return;
    }
    if (player_index >= 4) {
        player_index = 0;
    }

    uint32_t buttons = 0;
    MacroFrame macro_frame{};
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
                buttons = macro_frame.buttons | resolve_action_buttons(macro_frame.actions, playback_character_role);
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

    if (!write_base_player_input_snapshot(peripheral)) {
        suspend_macro_runtime_for_transition();
        return;
    }

    const uint32_t base_buttons = peripheral->mPadBtnOn;
    const uint32_t output_buttons = base_buttons | buttons;
    const uint32_t previous_buttons = last_buttons[player_index];
    const uint32_t routed_global_buttons =
        macro_frame.global_buttons | resolve_global_action_buttons(macro_frame.actions, playback_character_role);
    const uint32_t previous_global_buttons = last_global_routed_buttons[player_index];

    peripheral->mPadBtnOn = output_buttons;
    peripheral->mPadBtnTrg = output_buttons & ~previous_buttons;
    peripheral->mPadBtnRel = previous_buttons & ~output_buttons;
    apply_button_press_values(peripheral, output_buttons);

    apply_global_pad_route_for_macro(macro_frame, routed_global_buttons, previous_global_buttons, player_index);

    if (macro_frame.has_left_analog) {
        peripheral->mAnlgL.x = clamp_analog((int)peripheral->mAnlgL.x + macro_frame.left_x);
        peripheral->mAnlgL.y = clamp_analog((int)peripheral->mAnlgL.y + macro_frame.left_y);
        update_analog_info_for_macro(&peripheral->mAnlgL);
        peripheral->mHoldAnlgL = peripheral->mAnlgL;
        peripheral->mIsHold = false;
    }

    if (macro_frame.has_right_analog) {
        peripheral->mAnlgR.x = clamp_analog((int)peripheral->mAnlgR.x + macro_frame.right_x);
        peripheral->mAnlgR.y = clamp_analog((int)peripheral->mAnlgR.y + macro_frame.right_y);
        update_analog_info_for_macro(&peripheral->mAnlgR);
    }

    apply_screen_pause_action(macro_frame);
    apply_force_style_action(macro_frame);
    apply_character_switch_action(macro_frame);
    apply_one_hit_kill_action(macro_frame);

    last_buttons[player_index] = output_buttons;
    update_input_active();
}

void Macro::reset_input_state() {
    std::fill_n(last_buttons, 4, 0);
    for (uint32_t player_index = 0; player_index < 4; ++player_index) {
        clear_global_macro_route(player_index);
    }
    CharSwitcher::clear_macro_switch_request();
    playback_frame_index = 0;
    for (auto& frame : playback_frames) {
        frame.wait_elapsed_ticks = 0;
        frame.wait_initial_valid = false;
    }
    reset_hit_confirmed_wait_state();
    update_input_active();
}

static bool load_clip_into_playback_state(uint32_t clip_index) {
    if (clip_index >= Macro::playback_clips.size()) {
        Macro::playback_frames.clear();
        Macro::loaded_clip_index = INVALID_CLIP_INDEX;
        Macro::playback_character_role = MACRO_CHARACTER_INVALID;
        return false;
    }

    const auto& clip = Macro::playback_clips[clip_index];
    Macro::playback_frames = clip.frames;
    Macro::playback_frame_index = 0;
    Macro::loaded_clip_index = clip_index;
    Macro::playback_character_role = clip.character_role;
    return !Macro::playback_frames.empty();
}

bool Macro::load_playback_file() {
    playback_frames.clear();
    playback_clips.clear();
    playback_frame_index = 0;
    loaded_clip_index = INVALID_CLIP_INDEX;
    playback_character_role = MACRO_CHARACTER_INVALID;
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
        playback_character_role = MACRO_CHARACTER_INVALID;
        loaded_playback_path[0] = '\0';
    };

    auto fail_with_message = [&](const char* format, uint32_t value) {
        char message[256]{};
        std::snprintf(message, sizeof(message), format, value);
        set_playback_status(message);
        clear_failed_load();
        return false;
    };

    auto ensure_clip_name = [](MacroClip& clip, uint32_t clip_index) {
        if (clip.name.empty()) {
            clip.name = "Clip " + std::to_string(clip_index + 1);
        }
    };

    std::string line{};
    uint32_t line_number = 0;
    HeldMacroInput held_input{};
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
            if (!playback_clips.empty()) {
                auto& previous_clip = playback_clips.back();
                if (previous_clip.frames.empty()) {
                    return fail_with_message("Line %u: section has no playback ticks.", previous_clip.header_line);
                }
            }

            auto read_header_tag = [&](size_t& offset, std::string& value) {
                while (offset < line.size() && std::isspace((unsigned char)line[offset]) != 0) {
                    ++offset;
                }
                if (offset >= line.size() || line[offset] != '[') {
                    return false;
                }
                const auto close = line.find(']', offset + 1);
                if (close == std::string::npos) {
                    return false;
                }
                value = trim_copy(line.substr(offset + 1, close - offset - 1));
                offset = close + 1;
                return true;
            };

            size_t header_offset = 0;
            std::string hotkey_expression{};
            if (!read_header_tag(header_offset, hotkey_expression)) {
                return fail_with_message("Line %u: invalid section header.", line_number);
            }

            std::vector<uint32_t> clip_binds{};
            if (!parse_hotkey_binds(hotkey_expression, clip_binds)) {
                return fail_with_message("Line %u: invalid section hotkey.", line_number);
            }

            std::string second_tag{};
            if (!read_header_tag(header_offset, second_tag)) {
                return fail_with_message("Line %u: section header must include [Nero] or [Dante].", line_number);
            }

            uint32_t character_role = MACRO_CHARACTER_INVALID;
            uint32_t gamepad_hotkey_button = 0;
            if (!parse_character_role(second_tag, character_role)) {
                if (!parse_gamepad_hotkey_button_expression(second_tag, gamepad_hotkey_button)) {
                    return fail_with_message("Line %u: invalid gamepad hotkey or character tag.", line_number);
                }

                std::string role_tag{};
                if (!read_header_tag(header_offset, role_tag) || !parse_character_role(role_tag, character_role)) {
                    return fail_with_message("Line %u: section header must end with [Nero] or [Dante].", line_number);
                }
            }

            for (const auto& clip : playback_clips) {
                if (!clip_binds.empty() && clip.hotkey_binds == clip_binds && clip.character_role == character_role) {
                    return fail_with_message("Line %u: duplicate section hotkey and character.", line_number);
                }
                if (gamepad_hotkey_button != 0 && clip.gamepad_hotkey_button == gamepad_hotkey_button &&
                    clip.character_role == character_role) {
                    return fail_with_message("Line %u: duplicate section gamepad hotkey and character.", line_number);
                }
            }

            playback_clips.emplace_back();
            auto& new_clip = playback_clips.back();
            new_clip.hotkey_binds = std::move(clip_binds);
            new_clip.gamepad_hotkey_button = gamepad_hotkey_button;
            new_clip.character_role = character_role;
            new_clip.name = trim_copy(line.substr(header_offset));
            new_clip.header_line = line_number;
            ensure_clip_name(new_clip, (uint32_t)playback_clips.size() - 1);
            held_input = {};
            continue;
        }

        if (playback_clips.empty()) {
            return fail_with_message("Line %u: script commands must be inside a [Hotkey][Nero/Dante] section.", line_number);
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

            MacroFrame frame{};
            if (!parse_playback_command(button_expression, frame, current_clip.character_role)) {
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
            uint32_t compare = 0;
            float value = 0.0f;
            uint32_t value_u32 = 0;
            uint32_t max_ticks = 0;
            if (!parse_wait_condition(stream, condition, arg, compare, value, value_u32, max_ticks)) {
                return fail_with_message("Line %u: invalid WAIT_UNTIL condition.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_wait_condition(held_input, condition, arg, compare, value, value_u32, max_ticks))) {
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
            if (expression.empty() || !parse_macro_input(expression, input, current_clip.character_role)) {
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
            if (expression.empty() || !parse_macro_input(expression, input, current_clip.character_role)) {
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

            if (!append_tap_macro(current_clip.frames, held_input, input_expression, ticks, current_clip.character_role)) {
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

            if (!append_direction_button_macro(current_clip.frames, held_input, direction_expression, input_expression, ticks, current_clip.character_role)) {
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

            if (!append_back_forward_macro(current_clip.frames, held_input, back_expression, forward_expression, input_expression, current_clip.character_role)) {
                set_playback_status("Macro file is too long or BACK_FORWARD input is invalid.");
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
            if (!parse_macro_input(command, input, current_clip.character_role)) {
                return fail_with_message("Line %u: invalid screen pause command.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_input(input))) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "CHARACTER_SWITCH" || command == "SWITCH_CHARACTER" || command == "CHAR_SWITCH" ||
            command == "CHARACTER_SWAP" || command == "SWAP_CHARACTER") {
            std::string extra{};
            if (stream >> extra) {
                return fail_with_message("Line %u: invalid CHARACTER_SWITCH command.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_character_switch())) {
                set_playback_status("Macro file is too long.");
                clear_failed_load();
                return false;
            }

            continue;
        }

        if (command == "ONE_HIT_KILL" || command == "ONEHITKILL" || command == "OHK") {
            std::string action_token{};
            std::string extra{};
            stream >> action_token >> extra;
            if (!extra.empty()) {
                return fail_with_message("Line %u: invalid ONE_HIT_KILL command.", line_number);
            }

            const auto action = normalize_button_token(action_token);
            bool toggle = false;
            bool value = false;
            if (action.empty() || action == "TOGGLE") {
                toggle = true;
            }
            else if (action == "ON" || action == "ENABLE" || action == "ENABLED" || action == "TRUE" || action == "1") {
                value = true;
            }
            else if (action == "OFF" || action == "DISABLE" || action == "DISABLED" || action == "FALSE" || action == "0") {
                value = false;
            }
            else {
                return fail_with_message("Line %u: invalid ONE_HIT_KILL command.", line_number);
            }

            if (!append_playback_frames(current_clip.frames, 1, frame_from_one_hit_kill(toggle, value))) {
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
            if (expression.empty() || !parse_macro_input(expression, input, current_clip.character_role)) {
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
    remember_macro_file_write_time(file_path);

    return !playback_frames.empty();
}

std::string Macro::resolve_playback_path() {
    return resolve_playback_path_text(get_selected_playback_path_text().c_str());
}

bool Macro::reload_playback_file() {
    clear_snapshot_play_delay();
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

void Macro::restart_playback_clip(uint32_t clip_index) {
    clear_snapshot_play_delay();

    if (screen_pause_active) {
        set_screen_pause(false);
    }

    if (!macro_runtime_ready()) {
        playback_enabled = false;
        clear_playback_timer();
        reset_input_state();
        update_input_active();
        set_playback_status("Playback blocked: gameplay is unavailable.");
        return;
    }

    if (clip_index >= playback_clips.size() || !load_clip_into_playback_state(clip_index)) {
        playback_enabled = false;
        update_input_active();
        set_playback_status("Selected Macro clip is unavailable.");
        return;
    }

    if (!clip_matches_current_character(playback_clips[clip_index])) {
        playback_enabled = false;
        clear_playback_timer();
        update_input_active();
        char message[128]{};
        std::snprintf(message, sizeof(message), "Selected clip is for %s; current character does not match.",
            character_role_label(playback_clips[clip_index].character_role));
        set_playback_status(message);
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

void Macro::restart_playback() {
    if (!load_playback_file()) {
        playback_enabled = false;
        update_input_active();
        return;
    }

    restart_playback_clip(selected_clip_index);
}

void Macro::clear_snapshot_play_delay() {
    snapshot_play_pending_ticks = 0;
    snapshot_play_pending_clip_index = INVALID_CLIP_INDEX;
}

bool Macro::load_snapshot_then_play() {
    if (!macro_runtime_ready()) {
        playback_enabled = false;
        clear_snapshot_play_delay();
        update_input_active();
        set_playback_status("Load Snapshot + Play failed: gameplay is unavailable.");
        return false;
    }

    if (!load_playback_file()) {
        playback_enabled = false;
        clear_snapshot_play_delay();
        update_input_active();
        return false;
    }

    const uint32_t clip_index = selected_clip_index < playback_clips.size() ? selected_clip_index : 0;
    playback_enabled = false;
    clear_playback_timer();
    reset_input_state();
    clear_last_peripheral_output();

    if (!load_position_snapshot()) {
        clear_snapshot_play_delay();
        update_input_active();
        return false;
    }

    if (snapshot_play_delay_ticks == 0) {
        restart_playback_clip(clip_index);
        return true;
    }

    snapshot_play_pending_clip_index = clip_index;
    snapshot_play_pending_ticks = std::min(snapshot_play_delay_ticks, MAX_SNAPSHOT_PLAY_DELAY_TICKS);
    update_input_active();

    const auto clip_label = clip_display_label(playback_clips[clip_index], clip_index);
    char message[256]{};
    std::snprintf(
        message,
        sizeof(message),
        "Snapshot loaded; playback starts in %u ticks: %s.",
        snapshot_play_pending_ticks,
        clip_label.c_str());
    set_playback_status(message);
    return true;
}

void Macro::tick_snapshot_play_delay() {
    if (snapshot_play_pending_clip_index == INVALID_CLIP_INDEX) {
        return;
    }

    if (!mod_enabled || !macro_runtime_ready()) {
        clear_snapshot_play_delay();
        return;
    }

    if (snapshot_play_pending_ticks > 0) {
        --snapshot_play_pending_ticks;
        if (snapshot_play_pending_ticks > 0) {
            return;
        }
    }

    const uint32_t clip_index = snapshot_play_pending_clip_index;
    clear_snapshot_play_delay();
    restart_playback_clip(clip_index);
    DISPLAY_MESSAGE("Snapshot loaded; Macro playback started");
}

void Macro::stop_all_input() {
    clear_snapshot_play_delay();

    if (screen_pause_active) {
        set_screen_pause(false);
    }

    playback_enabled = false;
    finalize_playback_timer();
    CharSwitcher::clear_macro_switch_request();
    playback_frame_index = 0;
    clear_last_peripheral_output();
    clear_input_frames = 4;
    reset_hit_confirmed_wait_state();
    update_input_active();
    set_playback_status("Playback stopped; clearing input.");
}

void Macro::check_auto_reload_file() {
    if (!mod_enabled || playback_enabled || snapshot_play_pending_clip_index != INVALID_CLIP_INDEX) {
        return;
    }

    const std::string file_path = resolve_playback_path();
    uint64_t write_time = 0;
    uint64_t file_size = 0;
    if (!get_file_fingerprint(file_path, write_time, file_size)) {
        return;
    }

    if (known_macro_file_time_path != file_path) {
        if (reload_playback_file()) {
            set_playback_status("Macro file selected; loaded.");
        }
        return;
    }

    if (known_macro_file_write_time == 0) {
        known_macro_file_write_time = write_time;
        known_macro_file_size = file_size;
        return;
    }

    if (write_time == known_macro_file_write_time && file_size == known_macro_file_size) {
        return;
    }

    known_macro_file_write_time = write_time;
    known_macro_file_size = file_size;
    if (reload_playback_file()) {
        set_playback_status("Macro file changed; auto-reloaded.");
    }
}

void Macro::check_pause_interrupt() {
    const bool game_paused = is_game_paused_safe();

    if (!mod_enabled || !stop_macro_on_game_pause) {
        last_game_pause_state = game_paused;
        return;
    }

    if (game_paused && !last_game_pause_state && (playback_enabled || snapshot_play_pending_clip_index != INVALID_CLIP_INDEX)) {
        stop_all_input();
        set_playback_status("Playback stopped because the game paused.");
    }

    last_game_pause_state = game_paused;
}

void Macro::on_frame(fmilliseconds& dt) {
    (void)dt;
    check_auto_reload_file();
    if (mod_enabled && !macro_runtime_ready()) {
        suspend_macro_runtime_for_transition();
        poll_raw_keyboard(false);
        poll_gamepad_hotkeys(nullptr, false);
        return;
    }

    if (macro_suspended_for_transition && macro_runtime_ready()) {
        macro_suspended_for_transition = false;
        set_playback_status("Macro ready.");
    }

    check_pause_interrupt();
    check_hotkeys();
    poll_gamepad_hotkeys(nullptr, mod_enabled);
    if (position_snapshot_load_ticks > 0) {
        if (apply_position_snapshot(false, false)) {
            --position_snapshot_load_ticks;
        }
        else {
            position_snapshot_load_ticks = 0;
        }
    }
}

void Macro::update_input_active() {
    input_active = clear_input_frames > 0 || (mod_enabled && playback_enabled);
}

void Macro::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        macro_instance = this;
        ensure_keyboard_hotkeys(m_hotkeys);
        ensure_keyboard_hotkey_binds(m_hotkeys);

        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Macro"), &mod_enabled)) {
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
                        selected_clip_index = 0;
                        reload_playback_file();
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                const bool custom_selected = playback_slot == PLAYBACK_SLOT_CUSTOM;
                if (ImGui::Selectable(CUSTOM_PLAYBACK_LABEL, custom_selected)) {
                    playback_slot = PLAYBACK_SLOT_CUSTOM;
                    selected_clip_index = 0;
                    reload_playback_file();
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

            if (ImGui::CollapsingHeader(_("Keyboard Hotkeys"), ImGuiTreeNodeFlags_DefaultOpen) && m_hotkeys.size() >= 6) {
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

                draw_macro_hotkey(_("Play Macro"), 2, *m_hotkeys[1]);
                draw_macro_hotkey(_("Stop Macro / Clear Input"), 3, *m_hotkeys[2]);
                draw_macro_hotkey(_("Capture Snapshot"), 4, *m_hotkeys[3]);
                draw_macro_hotkey(_("Load Snapshot"), 5, *m_hotkeys[4]);
                draw_macro_hotkey(_("Load Snapshot + Play Macro"), 6, *m_hotkeys[5]);

                if (capture_hotkey_target != 0) {
                    ImGui::TextWrapped(_("Capturing hotkey: press a non-modifier key. Ctrl, Shift, and Alt are captured as modifiers."));
                }
            }
            if (ImGui::CollapsingHeader(_("Gamepad Hotkeys"), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Checkbox(_("Enable Gamepad Hotkeys"), &gamepad_hotkeys_enabled);
                ImGui::SameLine();
                help_marker(_(
                    "Controller shortcuts require holding Back/Select first, then pressing one shortcut button. While enabled, Back/Select is reserved for shortcuts and will not trigger its original game action, which is Taunt in the default layout. The shortcut input is consumed by the mod so the game will not also receive it."));
                ImGui::TextWrapped(_("All gamepad hotkeys use Back/Select as the fixed prefix. To set or use one, hold Back/Select first, then press the shortcut button."));
                ImGui::TextWrapped(_("While this option is enabled, manual Back/Select is reserved for shortcuts and will not trigger the game's original taunt. Macro playback can still send Back/Select with commands such as HOLD SELECT and RELEASE SELECT."));
                ImGui::TextWrapped(_("Macro clip headers can also define Back/Select shortcuts, for example [V][X/SQUARE][Nero]. A matching clip shortcut starts that clip directly."));

                const char* gamepad_action_labels[GAMEPAD_HOTKEY_COUNT] = {
                    _("Play"),
                    _("Stop"),
                    _("Capture"),
                    _("Load"),
                    _("Load + Play"),
                };

                for (uint32_t index = 0; index < GAMEPAD_HOTKEY_COUNT; ++index) {
                    ImGui::PushID((int)(100 + index));
                    ImGui::Text("%s: Back/Select + %s", gamepad_action_labels[index], gamepad_hotkey_button_label(gamepad_hotkey_buttons[index]));
                    ImGui::SameLine(sameLineWidth);
                    if (ImGui::Button(_("Set"))) {
                        capture_gamepad_hotkey_target = index + 1;
                        gamepad_hotkey_raw_buttons = 0;
                        set_playback_status("Hold Back/Select and press the new gamepad hotkey button.");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(_("Clear"))) {
                        gamepad_hotkey_buttons[index] = DEFAULT_GAMEPAD_HOTKEY_BUTTONS[index];
                        if (capture_gamepad_hotkey_target == index + 1) {
                            capture_gamepad_hotkey_target = 0;
                        }
                        set_playback_status("Gamepad hotkey restored to default.");
                    }
                    ImGui::PopID();
                }

                if (capture_gamepad_hotkey_target != 0) {
                    ImGui::TextWrapped(_("Capturing gamepad hotkey: hold Back/Select first, then press one non-Back controller button."));
                }
            }

            if (ImGui::CollapsingHeader(_("Action Mapping"))) {
                ImGui::TextWrapped(_("Action names use the mapping for the clip character tag. Match Nero and Dante separately to DMC4's in-game controller settings; this does not remap the game by itself."));
                size_t action_count = 0;
                const auto* actions = macro_action_choices(action_count);
                size_t button_count = 0;
                const auto* buttons = macro_button_choices(button_count);
                for (uint32_t role = 0; role < MACRO_CHARACTER_ROLE_COUNT; ++role) {
                    ImGui::PushID((int)(1000 + role));
                    ImGui::Text("%s", character_role_label(role));
                    for (uint32_t action_index = 0; action_index < action_count && action_index < MACRO_ACTION_COUNT; ++action_index) {
                        if (!macro_action_valid_for_role(action_index, role)) {
                            continue;
                        }
                        ImGui::PushID((int)action_index);
                        const auto current_button = action_button_map[role][action_index];
                        if (ImGui::BeginCombo(actions[action_index].label, macro_button_label(current_button))) {
                            for (size_t button_index = 0; button_index < button_count; ++button_index) {
                                const bool is_selected = current_button == buttons[button_index].button;
                                if (ImGui::Selectable(buttons[button_index].label, is_selected)) {
                                    action_button_map[role][action_index] = buttons[button_index].button;
                                    reset_input_state();
                                    reload_playback_file();
                                    set_playback_status("Action mapping updated.");
                                }
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }
                if (ImGui::Button(_("Reset Action Mapping"))) {
                    for (uint32_t role = 0; role < MACRO_CHARACTER_ROLE_COUNT; ++role) {
                        for (uint32_t action_index = 0; action_index < MACRO_ACTION_COUNT; ++action_index) {
                            action_button_map[role][action_index] = default_action_button_for_role(action_index, role);
                        }
                    }
                    reset_input_state();
                    reload_playback_file();
                    set_playback_status("Action mapping restored to default.");
                }
            }

            if (!macro_runtime_ready()) {
                suspend_macro_runtime_for_transition();
                ImGui::TextWrapped(_("Macro status: %s"), playback_status);
                ImGui::Unindent(lineIndent);
                ImGui::EndGroup();
                return;
            }

            ImGui::SeparatorText(_("Playback"));
            ImGui::Checkbox(_("Stop Macro when game pauses"), &stop_macro_on_game_pause);
            ImGui::SameLine();
            help_marker(_("Stop macro playback when DMC4 opens the pause menu so it will not continue after unpausing."));
            ImGui::TextWrapped(_("The selected macro file reloads automatically after saving, as long as playback is stopped."));
            ImGui::TextWrapped(_("Play Macro and Load Snapshot + Play Macro use the selected Macro File and Macro Clip above."));
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
            int snapshot_delay_ticks = (int)std::min(snapshot_play_delay_ticks, MAX_SNAPSHOT_PLAY_DELAY_TICKS);
            ImGui::TextUnformatted(_("Snapshot Play Delay Ticks"));
            ImGui::SameLine();
            help_marker(_("Delay between loading a Battle Snapshot and starting the selected macro clip."));
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputInt("##SnapshotPlayDelayTicks", &snapshot_delay_ticks)) {
                snapshot_play_delay_ticks = (uint32_t)std::clamp(snapshot_delay_ticks, 0, (int)MAX_SNAPSHOT_PLAY_DELAY_TICKS);
            }
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
            ImGui::SameLine();
            if (ImGui::Button(_("Load Snapshot + Play Macro"))) {
                if (load_snapshot_then_play()) {
                    DISPLAY_MESSAGE("Snapshot loaded; Macro playback queued");
                }
            }
            ImGui::TextWrapped(_("%s"), position_snapshot_label().c_str());
            if (position_snapshot_load_ticks > 0) {
                ImGui::Text(_("Snapshot restore ticks: %u"), position_snapshot_load_ticks);
            }
            if (snapshot_play_pending_clip_index != INVALID_CLIP_INDEX) {
                ImGui::Text(_("Snapshot playback delay: %u"), snapshot_play_pending_ticks);
            }

            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
}

void Macro::handle_hotkey_actions(
    bool reload_pressed,
    bool restart_pressed,
    bool stop_pressed,
    bool capture_snapshot_pressed,
    bool load_snapshot_pressed,
    bool load_snapshot_play_pressed) {
    if (!mod_enabled) {
        return;
    }

    if (stop_pressed) {
        stop_all_input();
        DISPLAY_MESSAGE("Macro stopped");
        return;
    }

    if (reload_pressed) {
        clear_snapshot_play_delay();
        reload_playback_file();
        DISPLAY_MESSAGE("Macro file reloaded");
    }

    if (load_snapshot_play_pressed) {
        if (load_snapshot_then_play()) {
            DISPLAY_MESSAGE("Snapshot loaded; Macro playback queued");
        }
        return;
    }

    if (restart_pressed) {
        restart_playback();
        DISPLAY_MESSAGE("Macro playback started");
    }

    if (capture_snapshot_pressed) {
        clear_snapshot_play_delay();
        if (capture_position_snapshot()) {
            DISPLAY_MESSAGE("Battle Snapshot captured");
        }
    }

    if (load_snapshot_pressed) {
        clear_snapshot_play_delay();
        if (load_position_snapshot()) {
            DISPLAY_MESSAGE("Battle Snapshot loaded");
        }
    }
}

void Macro::poll_raw_keyboard(bool trigger_actions) {
    bool reload_pressed = false;
    bool restart_pressed = false;
    bool stop_pressed = false;
    bool capture_snapshot_pressed = false;
    bool load_snapshot_pressed = false;
    bool load_snapshot_play_pressed = false;
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
            if (macro_instance && macro_instance->m_hotkeys.size() >= 6) {
                auto& restart_hotkey = *macro_instance->m_hotkeys[1];
                auto& stop_hotkey = *macro_instance->m_hotkeys[2];
                auto& capture_snapshot_hotkey = *macro_instance->m_hotkeys[3];
                auto& load_snapshot_hotkey = *macro_instance->m_hotkeys[4];
                auto& load_snapshot_play_hotkey = *macro_instance->m_hotkeys[5];

                restart_pressed |= hotkey_message_matches(restart_hotkey, vkey);
                stop_pressed |= hotkey_message_matches(stop_hotkey, vkey);
                capture_snapshot_pressed |= hotkey_message_matches(capture_snapshot_hotkey, vkey);
                load_snapshot_pressed |= hotkey_message_matches(load_snapshot_hotkey, vkey);
                load_snapshot_play_pressed |= hotkey_message_matches(load_snapshot_play_hotkey, vkey);

                restart_pressed |= vkey == DEFAULT_RESTART_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(restart_hotkey, DEFAULT_RESTART_VKEY);
                stop_pressed |= vkey == DEFAULT_STOP_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(stop_hotkey, DEFAULT_STOP_VKEY);
                capture_snapshot_pressed |= vkey == DEFAULT_CAPTURE_SNAPSHOT_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(capture_snapshot_hotkey, DEFAULT_CAPTURE_SNAPSHOT_VKEY);
                load_snapshot_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_VKEY && !any_hotkey_modifier_down() && hotkey_is_default_or_unbound(load_snapshot_hotkey, DEFAULT_LOAD_SNAPSHOT_VKEY);
                load_snapshot_play_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY && !any_hotkey_modifier_down() &&
                    hotkey_is_default_or_unbound(load_snapshot_play_hotkey, DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY);
            }
            else {
                restart_pressed |= vkey == DEFAULT_RESTART_VKEY && !any_hotkey_modifier_down();
                stop_pressed |= vkey == DEFAULT_STOP_VKEY && !any_hotkey_modifier_down();
                capture_snapshot_pressed |= vkey == DEFAULT_CAPTURE_SNAPSHOT_VKEY && !any_hotkey_modifier_down();
                load_snapshot_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_VKEY && !any_hotkey_modifier_down();
                load_snapshot_play_pressed |= vkey == DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY && !any_hotkey_modifier_down();
            }

            const uint32_t current_role = current_player_character_role();
            for (uint32_t clip_index = 0; clip_index < playback_clips.size(); ++clip_index) {
                const auto& binds = playback_clips[clip_index].hotkey_binds;
                if (playback_clips[clip_index].character_role == current_role &&
                    hotkey_binds_match_message(binds, vkey) && binds.size() > clip_hotkey_score) {
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
            load_snapshot_play_pressed = false;
            clip_hotkey_index = INVALID_CLIP_INDEX;
        }

        handle_hotkey_actions(reload_pressed, restart_pressed, stop_pressed, capture_snapshot_pressed, load_snapshot_pressed, load_snapshot_play_pressed);
        if (!reload_pressed && !restart_pressed && !stop_pressed && !capture_snapshot_pressed && !load_snapshot_pressed && !load_snapshot_play_pressed &&
            clip_hotkey_index != INVALID_CLIP_INDEX) {
            restart_playback_clip(clip_hotkey_index);
            DISPLAY_MESSAGE("Macro clip playback started");
        }
    }
}

void Macro::poll_gamepad_hotkeys(cPeripheral* peripheral, bool trigger_actions) {
    if (!trigger_actions) {
        gamepad_hotkey_chord_state = 0;
        gamepad_hotkey_raw_buttons = 0;
        return;
    }

    if (!peripheral && capture_gamepad_hotkey_target == 0) {
        return;
    }

    uint32_t player_index = 0;
    if (peripheral && !read_peripheral_player_index_safe(peripheral, player_index)) {
        gamepad_hotkey_chord_state = 0;
        return;
    }

    if (player_index != 0) {
        return;
    }

    uint32_t buttons = 0;
    if (!read_gamepad_buttons_safe(buttons)) {
        gamepad_hotkey_chord_state = 0;
        gamepad_hotkey_raw_buttons = 0;
        return;
    }

    const uint32_t pressed_buttons = buttons & ~gamepad_hotkey_raw_buttons;
    gamepad_hotkey_raw_buttons = buttons;
    const auto now = std::chrono::steady_clock::now();
    const bool hotkey_context = gamepad_hotkeys_enabled || capture_gamepad_hotkey_target != 0;
    const bool prefix_active = gamepad_hotkey_prefix_active(buttons, now);

    if (hotkey_context) {
        uint32_t consume_buttons = buttons & PAD_BUTTON_SELECT;
        if (prefix_active) {
            consume_buttons |= buttons & (configured_gamepad_hotkey_button_mask() | clip_gamepad_hotkey_button_mask());
        }
        consume_gamepad_hotkey_buttons(peripheral, consume_buttons);
    }

    if (capture_gamepad_hotkey_target != 0) {
        if (!prefix_active) {
            return;
        }

        uint32_t captured_button = first_valid_gamepad_hotkey_button(pressed_buttons);

        consume_gamepad_hotkey_buttons(peripheral, (buttons & PAD_BUTTON_SELECT) | (buttons & captured_button));
        if (captured_button == 0) {
            return;
        }

        const uint32_t target_index = capture_gamepad_hotkey_target - 1;
        if (target_index < GAMEPAD_HOTKEY_COUNT) {
            gamepad_hotkey_buttons[target_index] =
                sanitize_gamepad_hotkey_button(captured_button, DEFAULT_GAMEPAD_HOTKEY_BUTTONS[target_index]);
            set_playback_status("Gamepad hotkey updated.");
        }
        capture_gamepad_hotkey_target = 0;
        return;
    }

    if (!gamepad_hotkeys_enabled || capture_hotkey_target != 0) {
        gamepad_hotkey_chord_state = 0;
        return;
    }

    const uint32_t chord_buttons = gamepad_hotkey_buttons_from_rolling_chord(buttons, pressed_buttons, now);
    uint32_t clip_hotkey_index = INVALID_CLIP_INDEX;
    uint32_t clip_hotkey_button = 0;
    if (chord_buttons != 0 && is_game_window_foreground()) {
        const uint32_t current_role = current_player_character_role();
        for (uint32_t clip_index = 0; clip_index < playback_clips.size(); ++clip_index) {
            const auto& clip = playback_clips[clip_index];
            if (clip.character_role == current_role && clip.gamepad_hotkey_button != 0 &&
                (chord_buttons & clip.gamepad_hotkey_button) != 0) {
                clip_hotkey_index = clip_index;
                clip_hotkey_button = clip.gamepad_hotkey_button;
                break;
            }
        }
    }

    if (clip_hotkey_index != INVALID_CLIP_INDEX) {
        consume_gamepad_hotkey_buttons(peripheral, (buttons & PAD_BUTTON_SELECT) | (buttons & clip_hotkey_button));
        restart_playback_clip(clip_hotkey_index);
        DISPLAY_MESSAGE("Macro clip playback started");
        return;
    }

    const uint32_t chord_actions = gamepad_hotkey_actions_from_rolling_chord(buttons, pressed_buttons, now);
    if (chord_actions != 0) {
        consume_gamepad_hotkey_buttons(peripheral, (buttons & PAD_BUTTON_SELECT) | (buttons & gamepad_hotkey_buttons_from_actions(chord_actions)));
    }

    uint32_t pressed_actions = chord_actions & ~gamepad_hotkey_chord_state;
    gamepad_hotkey_chord_state = chord_actions;

    if (pressed_actions == 0) {
        return;
    }

    bool restart_pressed = (pressed_actions & GAMEPAD_HOTKEY_ACTION_PLAY) != 0;
    bool stop_pressed = (pressed_actions & GAMEPAD_HOTKEY_ACTION_STOP) != 0;
    bool capture_snapshot_pressed = (pressed_actions & GAMEPAD_HOTKEY_ACTION_CAPTURE_SNAPSHOT) != 0;
    bool load_snapshot_pressed = (pressed_actions & GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT) != 0;
    bool load_snapshot_play_pressed = (pressed_actions & GAMEPAD_HOTKEY_ACTION_LOAD_SNAPSHOT_PLAY) != 0;

    if (!is_game_window_foreground()) {
        restart_pressed = false;
        capture_snapshot_pressed = false;
        load_snapshot_pressed = false;
        load_snapshot_play_pressed = false;
    }

    if (playback_enabled || snapshot_play_pending_clip_index != INVALID_CLIP_INDEX) {
        restart_pressed = false;
        load_snapshot_play_pressed = false;
    }

    handle_hotkey_actions(
        false,
        restart_pressed,
        stop_pressed,
        capture_snapshot_pressed,
        load_snapshot_pressed,
        load_snapshot_play_pressed);
}

void Macro::check_hotkeys() {
    ensure_keyboard_hotkeys(m_hotkeys);
    ensure_keyboard_hotkey_binds(m_hotkeys);
    if (!mod_enabled) {
        if (capture_hotkey_target != 0) {
            capture_hotkey_target = 0;
            for (auto& hotkey : m_hotkeys) {
                if (hotkey) {
                    hotkey->m_setting = false;
                }
            }
        }
        capture_gamepad_hotkey_target = 0;
        poll_raw_keyboard(false);
        return;
    }

    poll_raw_keyboard(true);
}

void Macro::on_update_input(utility::Input& input) {
    (void)input;
    ensure_keyboard_hotkeys(m_hotkeys);
    ensure_keyboard_hotkey_binds(m_hotkeys);
}

bool Macro::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
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

void Macro::on_config_load(const utility::Config& cfg) {
    ensure_keyboard_hotkeys(m_hotkeys);
    // Legacy keyboard_macro_* keys are intentionally preserved so existing tester configs keep working.
    mod_enabled = cfg.get<bool>("keyboard_macro_enabled").value_or(false);
    playback_enabled = false;
    clear_playback_timer();
    loaded_playback_path[0] = '\0';
    reload_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_reload_vkey").value_or(DEFAULT_RELOAD_VKEY), 1u, 255u);
    restart_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_restart_vkey").value_or(DEFAULT_RESTART_VKEY), 1u, 255u);
    stop_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_stop_vkey").value_or(DEFAULT_STOP_VKEY), 1u, 255u);
    capture_snapshot_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_capture_snapshot_vkey").value_or(DEFAULT_CAPTURE_SNAPSHOT_VKEY), 1u, 255u);
    load_snapshot_vkey = std::clamp(cfg.get<uint32_t>("keyboard_macro_load_snapshot_vkey").value_or(DEFAULT_LOAD_SNAPSHOT_VKEY), 1u, 255u);
    load_snapshot_play_vkey =
        std::clamp(cfg.get<uint32_t>("keyboard_macro_load_snapshot_play_vkey").value_or(DEFAULT_LOAD_SNAPSHOT_PLAY_VKEY), 1u, 255u);
    snapshot_play_delay_ticks = std::min(
        cfg.get<uint32_t>("keyboard_macro_snapshot_play_delay_ticks").value_or(POSITION_SNAPSHOT_LOAD_TICKS),
        MAX_SNAPSHOT_PLAY_DELAY_TICKS);
    restore_resources_snapshot = cfg.get<bool>("keyboard_macro_restore_resources").value_or(false);
    auto_reload_file = true;
    stop_macro_on_game_pause = cfg.get<bool>("keyboard_macro_stop_on_game_pause").value_or(false);
    gamepad_hotkeys_enabled = cfg.get<bool>("keyboard_macro_gamepad_hotkeys").value_or(false);
    gamepad_hotkey_buttons[0] = sanitize_gamepad_hotkey_button(
        cfg.get<uint32_t>("keyboard_macro_gamepad_play_button").value_or(DEFAULT_GAMEPAD_HOTKEY_BUTTONS[0]),
        DEFAULT_GAMEPAD_HOTKEY_BUTTONS[0]);
    gamepad_hotkey_buttons[1] = sanitize_gamepad_hotkey_button(
        cfg.get<uint32_t>("keyboard_macro_gamepad_stop_button").value_or(DEFAULT_GAMEPAD_HOTKEY_BUTTONS[1]),
        DEFAULT_GAMEPAD_HOTKEY_BUTTONS[1]);
    gamepad_hotkey_buttons[2] = sanitize_gamepad_hotkey_button(
        cfg.get<uint32_t>("keyboard_macro_gamepad_capture_snapshot_button").value_or(DEFAULT_GAMEPAD_HOTKEY_BUTTONS[2]),
        DEFAULT_GAMEPAD_HOTKEY_BUTTONS[2]);
    gamepad_hotkey_buttons[3] = sanitize_gamepad_hotkey_button(
        cfg.get<uint32_t>("keyboard_macro_gamepad_load_snapshot_button").value_or(DEFAULT_GAMEPAD_HOTKEY_BUTTONS[3]),
        DEFAULT_GAMEPAD_HOTKEY_BUTTONS[3]);
    gamepad_hotkey_buttons[4] = sanitize_gamepad_hotkey_button(
        cfg.get<uint32_t>("keyboard_macro_gamepad_load_snapshot_play_button").value_or(DEFAULT_GAMEPAD_HOTKEY_BUTTONS[4]),
        DEFAULT_GAMEPAD_HOTKEY_BUTTONS[4]);
    last_game_pause_state = is_game_paused_safe();
    size_t action_count = 0;
    const auto* actions = macro_action_choices(action_count);
    for (uint32_t role = 0; role < MACRO_CHARACTER_ROLE_COUNT; ++role) {
        for (uint32_t action_index = 0; action_index < action_count && action_index < MACRO_ACTION_COUNT; ++action_index) {
            const char* config_key = role == MACRO_CHARACTER_NERO ?
                actions[action_index].nero_config_key : actions[action_index].dante_config_key;
            const uint32_t default_button = default_action_button_for_role(action_index, role);
            if (!config_key) {
                action_button_map[role][action_index] = 0;
                continue;
            }
            action_button_map[role][action_index] = sanitize_macro_button(
                cfg.get<uint32_t>(config_key).value_or(default_button),
                default_button);
        }
    }
    if (m_hotkeys.size() >= 6) {
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
        if (!cfg.get("keyboard_macro_load_snapshot_play_key")) {
            m_hotkeys[5]->m_default_keys = { load_snapshot_play_vkey };
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

void Macro::on_config_save(utility::Config& cfg) {
    // Save legacy keyboard_macro_* keys for compatibility with earlier macro test builds.
    cfg.set<bool>("keyboard_macro_enabled", mod_enabled);
    ensure_keyboard_hotkeys(m_hotkeys);
    if (m_hotkeys.size() >= 6) {
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
        if (!m_hotkeys[5]->m_binds.empty()) {
            load_snapshot_play_vkey = m_hotkeys[5]->m_binds.back();
        }
    }
    cfg.set<uint32_t>("keyboard_macro_reload_vkey", reload_vkey);
    cfg.set<uint32_t>("keyboard_macro_restart_vkey", restart_vkey);
    cfg.set<uint32_t>("keyboard_macro_stop_vkey", stop_vkey);
    cfg.set<uint32_t>("keyboard_macro_capture_snapshot_vkey", capture_snapshot_vkey);
    cfg.set<uint32_t>("keyboard_macro_load_snapshot_vkey", load_snapshot_vkey);
    cfg.set<uint32_t>("keyboard_macro_load_snapshot_play_vkey", load_snapshot_play_vkey);
    cfg.set<uint32_t>("keyboard_macro_snapshot_play_delay_ticks", snapshot_play_delay_ticks);
    cfg.set<bool>("keyboard_macro_restore_resources", restore_resources_snapshot);
    cfg.set<bool>("keyboard_macro_auto_reload_file", true);
    cfg.set<bool>("keyboard_macro_stop_on_game_pause", stop_macro_on_game_pause);
    cfg.set<bool>("keyboard_macro_gamepad_hotkeys", gamepad_hotkeys_enabled);
    cfg.set<uint32_t>("keyboard_macro_gamepad_play_button", gamepad_hotkey_buttons[0]);
    cfg.set<uint32_t>("keyboard_macro_gamepad_stop_button", gamepad_hotkey_buttons[1]);
    cfg.set<uint32_t>("keyboard_macro_gamepad_capture_snapshot_button", gamepad_hotkey_buttons[2]);
    cfg.set<uint32_t>("keyboard_macro_gamepad_load_snapshot_button", gamepad_hotkey_buttons[3]);
    cfg.set<uint32_t>("keyboard_macro_gamepad_load_snapshot_play_button", gamepad_hotkey_buttons[4]);
    size_t action_count = 0;
    const auto* actions = macro_action_choices(action_count);
    for (uint32_t role = 0; role < MACRO_CHARACTER_ROLE_COUNT; ++role) {
        for (uint32_t action_index = 0; action_index < action_count && action_index < MACRO_ACTION_COUNT; ++action_index) {
            const char* config_key = role == MACRO_CHARACTER_NERO ?
                actions[action_index].nero_config_key : actions[action_index].dante_config_key;
            if (config_key) {
                cfg.set<uint32_t>(config_key, action_button_map[role][action_index]);
            }
        }
    }
    cfg.set<uint32_t>("keyboard_macro_slot", playback_slot);
    cfg.set<uint32_t>("keyboard_macro_clip_index", selected_clip_index);
    cfg.set("keyboard_macro_custom_path", playback_path);
    cfg.set("keyboard_macro_playback_path", get_selected_playback_path_text());
}
