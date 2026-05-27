#pragma once

#include <string>
#include <vector>

#include "../mod.hpp"
#include "../sdk/Pad.hpp"

class uPlayer;

constexpr uint32_t KEYBOARD_MACRO_CHARACTER_ROLE_COUNT = 2;
constexpr uint32_t KEYBOARD_MACRO_ACTION_SLOT_COUNT = 13;
constexpr uint32_t KEYBOARD_MACRO_CHARACTER_NERO = 0;
constexpr uint32_t KEYBOARD_MACRO_CHARACTER_DANTE = 1;
constexpr uint32_t KEYBOARD_MACRO_CHARACTER_INVALID = 0xFFFFFFFFu;

struct KeyboardMacroFrame {
    uint32_t buttons = 0;
    uint32_t global_buttons = 0;
    uint32_t actions = 0;
    short left_x = 0;
    short left_y = 0;
    bool has_left_analog = false;
    short right_x = 0;
    short right_y = 0;
    bool has_right_analog = false;
    bool exceed = false;
    bool l2_exceed_compat = false;
    bool screen_pause = false;
    bool screen_resume = false;
    bool screen_pause_toggle = false;
    bool change_target = false;
    bool character_switch = false;
    bool one_hit_kill_set = false;
    bool one_hit_kill_value = false;
    bool one_hit_kill_toggle = false;
    bool force_style = false;
    int forced_style = -1;
    uint32_t wait_condition = 0;
    int wait_arg = 0;
    uint32_t wait_compare = 0;
    float wait_value = 0.0f;
    uint32_t wait_value_u32 = 0;
    uint32_t wait_initial_u32 = 0;
    bool wait_initial_valid = false;
    uint32_t wait_max_ticks = 0;
    uint32_t wait_elapsed_ticks = 0;
};

struct KeyboardMacroClip {
    std::string name{};
    std::vector<uint32_t> hotkey_binds{};
    uint32_t gamepad_hotkey_button = 0;
    std::vector<KeyboardMacroFrame> frames{};
    uint32_t character_role = KEYBOARD_MACRO_CHARACTER_INVALID;
    uint32_t header_line = 0;
};

class KeyboardMacro : public Mod {
public:
    KeyboardMacro() = default;
    std::string get_mod_name() override { return "KeyboardMacro"; };

    static bool mod_enabled;
    static bool playback_enabled;
    static bool input_active;
    static uint32_t last_buttons[4];
    static uint32_t playback_frame_index;
    static uint32_t clear_input_frames;
    static uint32_t reload_vkey;
    static uint32_t restart_vkey;
    static uint32_t stop_vkey;
    static uint32_t capture_snapshot_vkey;
    static uint32_t load_snapshot_vkey;
    static uint32_t load_snapshot_play_vkey;
    static uint32_t snapshot_play_delay_ticks;
    static uint32_t action_button_map[KEYBOARD_MACRO_CHARACTER_ROLE_COUNT][KEYBOARD_MACRO_ACTION_SLOT_COUNT];
    static uint32_t gamepad_hotkey_buttons[5];
    static bool auto_reload_file;
    static bool stop_macro_on_game_pause;
    static bool gamepad_hotkeys_enabled;
    static uint32_t playback_slot;
    static uint32_t selected_clip_index;
    static uint32_t loaded_clip_index;
    static uint32_t playback_character_role;
    static bool macro_exceed_active;
    static uint32_t macro_exceed_latch_ticks;
    static bool macro_exceed_player_state_written;
    static uint32_t macro_change_target_latch_ticks;
    static bool screen_pause_active;
    static bool screen_pause_restore_valid;
    static float screen_pause_restore_speed;
    static uint32_t screen_pause_request_count;
    static uint32_t screen_pause_fail_count;
    static char playback_path[260];
    static char loaded_playback_path[260];
    static char playback_status[256];
    static std::vector<KeyboardMacroClip> playback_clips;
    static std::vector<KeyboardMacroFrame> playback_frames;

    static void __stdcall on_pad_update_tick(cPeripheral* peripheral);
    static void __stdcall on_player_pad_update(cPeripheral* peripheral);
    static uint32_t __stdcall on_player_input_tick(uPlayer* player, void* input_state, uint32_t inputs);
    static void __stdcall on_player_input_press_written(uPlayer* player);

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_update_input(utility::Input& input) override;
    bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) override;

private:
    static void write_test_input(cPeripheral* peripheral, uint32_t player_index);
    static void reset_input_state(bool preserve_macro_exceed_request = false);
    static bool load_playback_file();
    static bool reload_playback_file();
    static void restart_playback();
    static void restart_playback_clip(uint32_t clip_index);
    static bool load_snapshot_then_play();
    static void clear_snapshot_play_delay();
    static void tick_snapshot_play_delay();
    static void stop_all_input();
    static void update_input_active();
    static void check_auto_reload_file();
    static void check_pause_interrupt();
    static std::string resolve_playback_path();
    static void handle_hotkey_actions(
        bool reload_pressed,
        bool restart_pressed,
        bool stop_pressed,
        bool capture_snapshot_pressed,
        bool load_snapshot_pressed,
        bool load_snapshot_play_pressed);
    static void poll_raw_keyboard(bool trigger_actions);
    static void poll_gamepad_hotkeys(cPeripheral* peripheral, bool trigger_actions);
    void check_hotkeys();
};
