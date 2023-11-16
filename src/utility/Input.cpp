#include "Input.hpp"

#include <Windows.h>
#include <utility>
#include <sstream>

#include "imgui.h"
#include "glm/trigonometric.hpp" // is this any different that cmath one, no idea


namespace utility {

    void Input::update()
    {
        for (size_t i = 0; i < m_status.size(); i++)
        {
            bool key_down = GetAsyncKeyState(static_cast<int>(i)) < 0;

            if (key_down && !m_status[i].is_down)
                m_status[i].state = InputState::WENT_DOWN;
            else if (!key_down && m_status[i].is_down)
                m_status[i].state = InputState::WENT_UP;
            else
                m_status[i].state = InputState::IDLE;

            m_status[i].is_down = key_down;
        }
    }


    bool Input::is_down(int vkey) const
    {
        return m_status[vkey].is_down;
    }

    bool Input::went_down(int vkey) const
    {
        return m_status[vkey].state == InputState::WENT_DOWN;
    }

    bool Input::went_up(int vkey) const
    {
        return m_status[vkey].state == InputState::WENT_UP;
    }


    const char* Hotkey::hotkey_name(int hotkey) {
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

            for (size_t i = 0; i < input.get_keys_size(); i++) {
                static constexpr std::array<size_t, 5> ignored_keycodes {
                    0x01, // lmouse
                    0x02, // rmouse
                    // we want r/l specializations
                    0x10, // shift
                    0x11, // ctrl
                    0x12, // alt
                };
                if (std::find(ignored_keycodes.begin(), ignored_keycodes.end(), i) != ignored_keycodes.end()) { continue; }
                if (input.went_down(i)) {
                    m_vk_vec.push_back(i);
                }
            }
        };

        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 290.0f);

        std::ostringstream ss{};

        for (const auto& key : m_vk_vec) {
            ss << hotkey_name(key);
            if ((m_vk_vec.size()) > 1 && (key != m_vk_vec.back())) {
                ss << " + ";
            }
        }
        ImGui::TextWrapped("%s %s - %s", prompt, ss.str().c_str(), m_description.c_str());

        // samelines for algning buttons neatly
        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 44.0f * 3.0f);

        // hsv sine undeline
        if (m_setting) {
            ImU32 col = ImColor::HSV((float)glm::sin(ImGui::GetTime()) * 0.5f + 0.5f, 0.75f, 1.0f);
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();
            min.y = max.y;
            ImGui::GetWindowDrawList()->AddLine(min, max, col, 2.0f);
        }

        ImGui::PopTextWrapPos();

        static ImVec2 btn_size{ 40.0f ,15.0f };

        std::string label_hashed { "set###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(label_hashed));
        if (ImGui::Button("set", btn_size))
        {
            m_setting = true;
            m_vk_vec.clear();

        } ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 44.0f*2.0f);
        ImGui::PopID();

        std::string slabel_hashed{ "save###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(slabel_hashed));
        if (ImGui::Button("save", btn_size)) {
            m_setting = false;
        } ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 44.0f);
        ImGui::PopID();

        std::string clabel_hashed{ "clear1###" + m_config_entry };
        ImGui::PushID(std::hash<std::string>{}(clabel_hashed));
        if (ImGui::Button("clear", btn_size)) {
            m_vk_vec.clear();
            m_setting = false;
            m_vk_vec = m_default_keys;
        }
        ImGui::PopID();
    };

    bool Hotkey::check(utility::Input& input) {
        size_t vk_vec_size = m_vk_vec.size();
        
        if ((vk_vec_size < 1) || m_setting) { return false; }

        uint32_t last = m_vk_vec.back();

        if (vk_vec_size == 1) {
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
                is_modifier |= input.is_down(key); 
            }

            return input.went_down(last) && !is_modifier; 
        }

        bool is_down = true;
        for (size_t i = 0; i < (vk_vec_size - 1); i++) {
            // use bitwise AND to make sure all the keys are down
            // this is done in case hotkey is not a button combo
            is_down &= input.is_down(m_vk_vec[i]);
        }

        return is_down && input.went_down(last);
    }

    void Hotkey::on_config_load(const utility::Config& cfg) {

        std::ostringstream oss{};

        for (const auto& key : m_default_keys) {
            oss << key << ',';
        }

        auto s = cfg.get(m_config_entry).value_or(oss.str());

        std::istringstream ss{ s };
        std::string token{};

        while (std::getline(ss, token, ',')) {
            m_vk_vec.push_back(atoi(token.c_str()));
        }

    };

    void Hotkey::on_config_save(utility::Config& cfg) {

        std::ostringstream ss{};

        for (const auto& key : m_vk_vec) {
            ss << key << ',';
        }

        cfg.set(m_config_entry, ss.str());

    };
}