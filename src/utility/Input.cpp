#include "Input.hpp"

#include <Windows.h>
#include <utility>
#include <sstream>
#include <algorithm>

#include "imgui.h"
#include "glm/trigonometric.hpp" // is this any different that cmath one, no idea

// NOTE(): experimental gamepad hotkey support?
#include "XInputHook.hpp"
#include "XInput.h"

struct XinputTriggers {
    BYTE m_left;
    BYTE m_right;
};

static WORD g_xinput_buttons[XUSER_MAX_COUNT];
static XinputTriggers g_xinput_triggers[XUSER_MAX_COUNT];

namespace utility {
    namespace gamepads {

        static void WINAPI xinput_get_state_callback(DWORD dw_index, void* p_state) {
            assert(dw_index>= 0 && dw_index < XUSER_MAX_COUNT);
            XINPUT_STATE* xinput_state = (XINPUT_STATE*)p_state;
            g_xinput_buttons[dw_index] = xinput_state->Gamepad.wButtons;
            g_xinput_triggers[dw_index].m_left  = xinput_state->Gamepad.bLeftTrigger;
            g_xinput_triggers[dw_index].m_right = xinput_state->Gamepad.bRightTrigger;
        }

        void set_gamepad_callbacks() {
            g_xinput_hook->add_on_get_state_fptr(xinput_get_state_callback);
        }
    } // namespace gamepads
    
    // for mods that have std::vector of hotkeys in them already
    void create_keyboard_hotkey(std::vector<std::unique_ptr<utility::Hotkey>>& mod_hotkeys, 
        std::vector<uint32_t>&& default_keys, const char* desc, const char* cfg_entry) {
        mod_hotkeys.emplace_back(std::make_unique<Hotkey>(default_keys, desc, cfg_entry, Hotkey::HkType::Keyboard));
    }
    void create_gamepad_hotkey(std::vector<std::unique_ptr<utility::Hotkey>>& mod_hotkeys, 
        std::vector<uint32_t>&& default_keys, const char* desc, const char* cfg_entry) {
        mod_hotkeys.emplace_back(std::make_unique<Hotkey>(default_keys, desc, cfg_entry, Hotkey::HkType::Gamepad));
    }
    
    // new functions that return hotkey smart-pointers
    std::unique_ptr<utility::Hotkey> create_keyboard_hotkey(
        std::vector<uint32_t>&& default_keys, const char* desc, const char* cfg_entry ) {
        return std::make_unique<utility::Hotkey>(default_keys, desc, cfg_entry, Hotkey::HkType::Keyboard);
    }
    std::unique_ptr<utility::Hotkey> create_gamepad_hotkey(
        std::vector<uint32_t>&& default_keys, const char* desc, const char* cfg_entry ) {
        return std::make_unique<utility::Hotkey>(default_keys, desc, cfg_entry, Hotkey::HkType::Gamepad);
    }

    // input methods
    void Input::get_state(bool keydown, InputStatus& current_status) {
        if (keydown && !current_status.is_down)
            current_status.state = InputState::WENT_DOWN;
        else if (!keydown && current_status.is_down)
            current_status.state = InputState::WENT_UP;
        else
            current_status.state = InputState::IDLE;

        current_status.is_down = keydown;
    }

    void Input::update_gamepads(/*size_t index*/) { // TODO(): add index for all 4 gamepad sampling
        static constexpr std::array<WORD,14> xinput_button_bits = {
            XINPUT_GAMEPAD_DPAD_UP,
            XINPUT_GAMEPAD_DPAD_DOWN,
            XINPUT_GAMEPAD_DPAD_LEFT,
            XINPUT_GAMEPAD_DPAD_RIGHT,
            XINPUT_GAMEPAD_START,
            XINPUT_GAMEPAD_BACK,
            XINPUT_GAMEPAD_LEFT_THUMB,
            XINPUT_GAMEPAD_RIGHT_THUMB,
            XINPUT_GAMEPAD_LEFT_SHOULDER,
            XINPUT_GAMEPAD_RIGHT_SHOULDER,
            XINPUT_GAMEPAD_A,
            XINPUT_GAMEPAD_B,
            XINPUT_GAMEPAD_X,
            XINPUT_GAMEPAD_Y
        };
        // buttons
        WORD btn_bit = 1;
        size_t btn_range = XIBtn::LEFT_TRIGGER; // from 0 (DPAD_UP) to 14 BUTTON_Y;
        for (size_t i = 0; i < btn_range; i++) {
            bool btn_down = g_xinput_buttons[0] & xinput_button_bits[i]; // only first gamepad for now
            Input::get_state(btn_down, m_gpad_status[i]);
        }
        // left trigger
        Input::get_state(
            g_xinput_triggers[0].m_left > XINPUT_GAMEPAD_TRIGGER_THRESHOLD,
            m_gpad_status[XIBtn::LEFT_TRIGGER]
        );
        // right trigger
        Input::get_state(
            g_xinput_triggers[0].m_right > XINPUT_GAMEPAD_TRIGGER_THRESHOLD,
            m_gpad_status[XIBtn::RIGHT_TRIGGER]
        );
    }

    void Input::update() {
        // keyboard keycode to InputState
        for (size_t i = 0; i < m_status.size(); i++) {
            bool key_down = GetAsyncKeyState((int)i) < 0;
            Input::get_state(key_down, m_status[i]);
        }
        // same but for gamepadfs
        update_gamepads();
    }

    bool Input::is_down(int vkey, bool is_gamepad) const {
        assert(vkey >= 0);
        if (is_gamepad) {
            return m_gpad_status[vkey].is_down;
        }
        return m_status[vkey].is_down;
    }

    bool Input::went_down(int vkey, bool is_gamepad) const {
        assert(vkey >= 0);
        if (is_gamepad) {
            return m_gpad_status[vkey].state == InputState::WENT_DOWN;
        }
        return m_status[vkey].state == InputState::WENT_DOWN;
    }

    bool Input::went_up(int vkey, bool is_gamepad) const {
        assert(vkey >= 0);
        if (is_gamepad) {
            return m_gpad_status[vkey].state == InputState::WENT_UP;
        }
        return m_status[vkey].state == InputState::WENT_UP;
    }

    // hotkey methods
    const char* Hotkey::hotkey_name(int hotkey) {
        // gamepad path
        if (m_is_gamepad) {
            assert(hotkey >= 0 && hotkey <= XIBtn::RIGHT_TRIGGER);
            static constexpr std::array names = {
                "XI_DPAD_UP",
                "XI_DPAD_DOWN",
                "XI_DPAD_LEFT",
                "XI_DPAD_RIGHT",
                "XI_START",
                "XI_BACK",
                "XI_LEFT_THUMB",
                "XI_RIGHT_THUMB",
                "XI_LEFT_SHOULDER",
                "XI_RIGHT_SHOULDER",
                "XI_A",
                "XI_B",
                "XI_X",
                "XI_Y",
                "XI_LT",
                "XI_RT",
            };
            return names.at(hotkey);
        }

        // keyboard path
        switch (hotkey) {
        case 0x10: // 16
            return "Shift";
        case 0x11: // 17
            return "Ctrl";
        case 0x21: // 33
            return "Page Up";
        case 0x22: // 34
            return "Page Down";
        case 0x23: // 35
            return "End";
        case 0x24: // 36
            return "Home";
        case 0x2D:
            return "Insert";
        case 0x2E:
            return "Delete";
        case 0x70: // 112
            return "F1";
        case 0x71: // 113
            return "F2";
        case 0x72: // 114
            return "F3";
        case 0x73: // 115
            return "F4";
        case 0x74: // 116
            return "F5";
        case 0x75: // 117
            return "F6";
        case 0x76: // 118
            return "F7";
        case 0x77: // 119
            return "F8";
        case 0x78: // 120
            return "F9";
        case 0x79: // 121
            return "F10";
        case 0x7A: // 122
            return "F11";
        case 0x7B: // 123
            return "F12";
        case 0xBB: // 187
            return "=/+";
        case 0x91:
            return "Scroll Lock";
        case 0xA0:
            return "LShift";
        case 0xA1:
            return "RShift";
        case 0xA2:
            return "LCtrl";
        case 0xA3:
            return "RCtrl";
        case 0xA4:
            return "LAlt";
        case 0xA5:
            return "RAlt";
        }
        // TODO(): probably wont work on non us keyboard layouts?
        // *A cause its too much work screwing with unicode support
        // and imgui font atlases
        char res = (char)(MapVirtualKeyA(hotkey, MAPVK_VK_TO_CHAR));
        if (!res) { return "?"; }

        m_tmp_buf[0] = res;
        m_tmp_buf[1] = 0;

        return m_tmp_buf;
    }

    void Hotkey::draw(utility::Input& input) {

        char prompt[6];
        ZeroMemory(prompt, sizeof(prompt));

        if (m_setting) { 
            sprintf_s(prompt, 6, "SET->");
            // gamepad path
            if (m_is_gamepad) {
                for (WORD i = 0; i < input.get_btns_size(); i++) {
                    if(input.went_down(i, m_is_gamepad)) {
                        auto predicate =[i](uint32_t elem) { return elem == i; };
                        bool should_add = std::none_of(m_binds.begin(), m_binds.end(), predicate);
                        if (should_add) {
                            m_binds.push_back(i);
                        }
                    }
                }
            }
            // keyboard path
            else {
                for (size_t i = 0; i < input.get_keys_size(); i++) {
                    static constexpr std::array<size_t, 5> ignored_keycodes{
                        0x01, // lmouse
                        0x02, // rmouse
                        // we want r/l specializations
                        0x10, // shift
                        0x11, // ctrl
                        0x12, // alt
                    };
                    if (std::find(ignored_keycodes.begin(), ignored_keycodes.end(), i) != ignored_keycodes.end()) {
                        continue;
                    }
                    if (input.went_down(i, m_is_gamepad)) {
                        m_binds.push_back(i);
                    }
                }
            }
        };

        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);
        std::ostringstream ss{};
        
        // print keyboard or gamepad buttons
        for (uint32_t hotkey : m_binds) {
            ss << hotkey_name(hotkey);
            if ((m_binds.size()) > 1 && (hotkey != m_binds.back())) {
                ss << " + ";
            }
        }

        ImGui::TextWrapped("%s %s - %s", prompt, ss.str().c_str(), m_description.c_str());

        static ImVec2 btn_size{ ImGui::GetTextLineHeight() * 4.0f , ImGui::GetTextLineHeight() * 1.5f };
        // samelines for algning buttons neatly
        float x_offset = ImGui::GetWindowContentRegionMax().x - ((btn_size.x * 1.1f));
        ImGui::SameLine(x_offset);

        // hsv sine undeline
        if (m_setting) {
            ImU32 col = ImColor::HSV((float)glm::sin(ImGui::GetTime()) * 0.5f + 0.5f, 0.75f, 1.0f);
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            min.y = max.y;
            ImGui::GetWindowDrawList()->AddLine(min, max, col, 2.0f);
        }

        ImGui::PopTextWrapPos();

        std::string label_hashed { "set###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(label_hashed));
        if (ImGui::Button("set", btn_size)) {
            m_setting = true;
            m_binds.clear();
        }
        x_offset -= btn_size.x * 1.1f;
        ImGui::SameLine(x_offset);
        ImGui::PopID();

        std::string slabel_hashed{ "save###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(slabel_hashed));
        if (ImGui::Button("save", btn_size)) {
            m_setting = false;
        }
        x_offset -= btn_size.x * 1.1f;
        ImGui::SameLine(x_offset);
        ImGui::PopID();

        std::string clabel_hashed{ "clear1###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(clabel_hashed));
        if (ImGui::Button("clear", btn_size)) {
            m_binds.clear();
            m_setting = false;
            m_binds = m_default_keys;
        }
        ImGui::PopID();
    };

    bool Hotkey::check(utility::Input& input) {
#if 0
        // TODO: move gamepad stuffs into separate class or idk
        // gamepad path
        if (m_is_gamepad) {
            size_t bsize = m_binds.size();
            if ((bsize < 1) || m_setting) { return false; }
            uint16_t last = m_binds.back();
            
            if (bsize == 1) {
                return input.went_down((int)last);
            }
            bool is_down = true;
            for (size_t i = 0; i < (bsize - 1); i++) {
                // use bitwise AND to make sure all the keys are down
                // this is done in case hotkey is not a button combo
                is_down &= input.is_down(m_binds[i]);
            }

            return is_down && input.went_down(last);
        }
#endif
        // keyboard path
        size_t binds_size = m_binds.size();
        if ((binds_size < 1) || m_setting) { return false; }
        uint32_t last = m_binds.back();

        if (binds_size == 1) {
            if (m_is_gamepad) {
                return input.went_down((int)last, m_is_gamepad);
            }
            bool is_modifier = false;
            // FIXME(deep): hacky shit so i dont have to check for cases like when one
            // mod binds f1 and other one ctrl + f1 and both get activated
            // fixing this properly is for the next refuqtor im tired
            static constexpr std::array<size_t, 11> modifiers {
                0x01, // lmouse
                0x02, // rmouse
                0x10, // shift
                0x11, // ctrl
                0x12, // alt
                0xA0, // LShift
                0xA1, // RShift
                0xA2, // LCtrl
                0xA3, // RCtrl
                0xA4, // LAlt
                0xA5, // RAlt
            };

            for (const auto key : modifiers) {
                is_modifier |= input.is_down(key, m_is_gamepad); 
            }

            return input.went_down(last, m_is_gamepad) && !is_modifier; 
        }

        bool is_down = true;
        for (size_t i = 0; i < (binds_size - 1); i++) {
            // use bitwise AND to make sure all the keys are down
            // this is done in case hotkey is not a button combo
            is_down &= input.is_down(m_binds[i], m_is_gamepad);
        }

        return is_down && input.went_down(last, m_is_gamepad);
    }

    void Hotkey::on_config_load(const utility::Config& cfg) {

        std::ostringstream oss{};

        for (uint32_t bind : m_default_keys) {
            oss << bind << ',';
        }

        auto s = cfg.get(m_config_entry).value_or(oss.str());

        std::istringstream ss{ s };
        std::string token{};

        while (std::getline(ss, token, ',')) {
            m_binds.push_back(::atoi(token.c_str()));
        }

    };

    void Hotkey::on_config_save(utility::Config& cfg) {

        std::ostringstream ss{};

        for (uint32_t bind : m_binds) {
            ss << bind << ',';
        }
#if 0
        // TODO(): saving gamepad buttons
        if (m_is_gamepad) {
            // gamepad
            for (const auto& btn : m_bt_vec) {
                ss << btn << ',';
            }
        } 
        else {
            // keyboard
            for (const auto& key : m_vk_vec) {
                ss << key << ',';
            }
        }
#endif

        cfg.set(m_config_entry, ss.str());

    };
}